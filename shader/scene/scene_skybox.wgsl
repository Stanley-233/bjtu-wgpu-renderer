struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) ndc: vec2f,
};

struct SkyboxUniform {
    invViewRotation: mat4x4f,
    invProjection: mat4x4f,
};

@group(0) @binding(0) var<uniform> uSkybox: SkyboxUniform;
@group(0) @binding(1) var uSkyboxTexture: texture_cube<f32>;
@group(0) @binding(2) var uSkyboxSampler: sampler;

@vertex
fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
    // 全屏三角形
    var positions = array<vec2f, 3>(
        vec2f(-1.0, -3.0),
        vec2f(-1.0, 1.0),
        vec2f(3.0, 1.0),
    );

    var out: VertexOutput;
    out.position = vec4f(positions[vertexIndex], 0.0, 1.0);
    out.ndc = positions[vertexIndex];
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    // 先从屏幕 NDC 反推观察空间方向，再乘去掉平移的 view 逆矩阵得到世界方向
    let clipPosition = vec4f(in.ndc, 1.0, 1.0);
    let viewDirectionH = uSkybox.invProjection * clipPosition;
    let viewDirection = normalize(viewDirectionH.xyz / max(viewDirectionH.w, 1.0e-4));
    let worldDirection = normalize((uSkybox.invViewRotation * vec4f(viewDirection, 0.0)).xyz);
    // 天空盒只做背景显示，所以直接输出线性 HDR 颜色，交给后续 tone mapping
    let skyColor = textureSampleLevel(uSkyboxTexture, uSkyboxSampler, worldDirection, 0.0);
    return vec4f(skyColor.rgb, 1.0);
}
