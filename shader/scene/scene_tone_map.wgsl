struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

@group(0) @binding(0) var uSceneColor: texture_2d<f32>;
@group(0) @binding(1) var uSceneColorSampler: sampler;

// ACES 曲线拟合中的中间有理函数
fn RrtAndOdtFit(color: vec3f) -> vec3f {
    let a = color * (color + vec3f(0.0245786)) - vec3f(0.000090537);
    let b = color * (vec3f(0.983729) * color + vec3f(0.4329510)) + vec3f(0.238081);
    return a / b;
}

fn AcesFitted(color: vec3f) -> vec3f {
    // WGSL 的矩阵按列构造，这里必须使用转置后的常见 ACES 矩阵常量
    // 否则会出现明显的洋红/粉色色偏
    let inputMatrix = mat3x3f(
        vec3f(0.59719, 0.07600, 0.02840),
        vec3f(0.35458, 0.90834, 0.13383),
        vec3f(0.04823, 0.01566, 0.83777),
    );
    let outputMatrix = mat3x3f(
        vec3f(1.60475, -0.10208, -0.00327),
        vec3f(-0.53108, 1.10813, -0.07276),
        vec3f(-0.07367, -0.00605, 1.07602),
    );
    let fitted = outputMatrix * RrtAndOdtFit(inputMatrix * color);
    return clamp(fitted, vec3f(0.0), vec3f(1.0));
}

fn LinearToSrgb(l: vec3f) -> vec3f {
    // tone mapping 后理论上应为非负值，这里做一次保护，避免对负数做 pow
    let safeLinear = max(l, vec3f(0.0));
    let cutoff = vec3f(0.0031308);
    let low = safeLinear * vec3f(12.92);
    let high = vec3f(1.055) * pow(safeLinear, vec3f(1.0 / 2.4)) - vec3f(0.055);
    return select(low, high, safeLinear > cutoff);
}

@vertex
fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
    var positions = array<vec2f, 3>(
        vec2f(-1.0, -3.0),
        vec2f(-1.0, 1.0),
        vec2f(3.0, 1.0),
    );
    var uvs = array<vec2f, 3>(
        vec2f(0.0, 2.0),
        vec2f(0.0, 0.0),
        vec2f(2.0, 0.0),
    );

    var out: VertexOutput;
    out.position = vec4f(positions[vertexIndex], 0.0, 1.0);
    out.uv = uvs[vertexIndex];
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    // 这里输入的是 HDR 线性空间颜色
    let hdrColor = textureSample(uSceneColor, uSceneColorSampler, in.uv).rgb;
    // 首版曝光固定为 1.0，后续如果要做调参，可以在这里乘 exposure
    let exposed = hdrColor;
    let mapped = AcesFitted(exposed);
    let srgb = LinearToSrgb(mapped);
    return vec4f(srgb, 1.0);
}
