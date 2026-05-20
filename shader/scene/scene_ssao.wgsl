struct VertexOutput {
    @builtin(position) position: vec4f,
};

struct SsaoUniform {
    // projection: view space -> clip space
    projection: mat4x4f,
    // invProjection: clip space -> view space
    invProjection: mat4x4f,
    // x: viewport width
    // y: viewport height
    // z: radius in view-space units
    // w: unused
    viewportSizeAndRadius: vec4f,
    // x: bias
    // y: power / intensity
    // z: sample count
    // w: unused
    aoParams: vec4f,
};

const kKernelSize: u32 = 16u;

var<private> kKernel: array<vec3f, 16> = array<vec3f, kKernelSize>(
    vec3f(0.5381,  0.1856, 0.4319),
    vec3f(0.1379,  0.2486, 0.4430),
    vec3f(0.3371,  0.5679, 0.1287),
    vec3f(-0.6999, -0.0451, 0.0019),

    vec3f(0.0689, -0.1598, 0.8547),
    vec3f(0.0560,  0.0069, 0.1843),
    vec3f(-0.0146, 0.1402, 0.0762),
    vec3f(0.0100, -0.1924, 0.0344),

    vec3f(-0.3577, -0.5301, 0.4358),
    vec3f(-0.3169,  0.1063, 0.0158),
    vec3f(0.0103, -0.5869, 0.0046),
    vec3f(-0.0897, -0.4940, 0.3287),

    vec3f(0.7119, -0.0154, 0.0918),
    vec3f(-0.0533, 0.0596, 0.5411),
    vec3f(0.0352, -0.0631, 0.5460),
    vec3f(-0.4776, 0.2847, 0.0271),
);

@group(0) @binding(0) var uSceneDepth: texture_depth_2d;
@group(0) @binding(1) var uSceneNormal: texture_2d<f32>;
@group(0) @binding(2) var<uniform> uSsao: SsaoUniform;

fn hash12(p: vec2f) -> f32 {
    let h = dot(p, vec2f(127.1, 311.7));
    return fract(sin(h) * 43758.5453123);
}

fn rotateAroundNormal(v: vec3f, n: vec3f, angle: f32) -> vec3f {
    let s = sin(angle);
    let c = cos(angle);
    // Rodrigues rotation formula
    return v * c + cross(n, v) * s + n * dot(n, v) * (1.0 - c);
}

// WebGPU texture / fragCoord uv:
//   uv.x: left  -> right
//   uv.y: top   -> bottom
// NDC:
//   x: left  -> right
//   y: bottom -> top
//   z: 0 -> 1 in WebGPU
// 所以 uv.y 和 ndc.y 之间需要翻转
fn reconstructViewPosition(uv: vec2f, depth: f32) -> vec3f {
    let clipPosition = vec4f(
        uv.x * 2.0 - 1.0,
        1.0 - uv.y * 2.0,
        depth,
        1.0
    );
    let viewPosition = uSsao.invProjection * clipPosition;
    return viewPosition.xyz / viewPosition.w;
}

// NDC [-1,1] -> UV [0,1]
fn projectViewPositionToUv(positionVS: vec3f) -> vec2f {
    let clipPosition = uSsao.projection * vec4f(positionVS, 1.0);
    let ndc = clipPosition.xy / clipPosition.w;
    return vec2f(
        ndc.x * 0.5 + 0.5,
        0.5 - ndc.y * 0.5
    );
}

fn loadNormalVS(pixelCoord: vec2i) -> vec3f {
    let n = textureLoad(uSceneNormal, pixelCoord, 0).xyz;
    return normalize(n);
}

// 在当前像素处构造一个 以 normalVS 为 z 轴的局部坐标系，即 TBN
fn buildBasis(normalVS: vec3f) -> mat3x3f {
    let referenceAxis = select(
        vec3f(0.0, 0.0, 1.0),
        vec3f(0.0, 1.0, 0.0),
        abs(normalVS.z) > 0.999
    );
    let tangent = normalize(cross(referenceAxis, normalVS));
    let bitangent = normalize(cross(normalVS, tangent));
    return mat3x3f(tangent, bitangent, normalVS);
}

// 全屏三角形
@vertex
fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
    var positions = array<vec2f, 3>(
        vec2f(-1.0, -3.0),
        vec2f(-1.0, 1.0),
        vec2f(3.0, 1.0),
    );

    var out: VertexOutput;
    let clipPosition = positions[vertexIndex];
    out.position = vec4f(clipPosition, 0.0, 1.0);
    return out;
}

@fragment
fn fs_main(@builtin(position) fragCoord: vec4f) -> @location(0) vec4f {
    let depthDimU = textureDimensions(uSceneDepth);
    let normalDimU = textureDimensions(uSceneNormal);

    if (depthDimU.x == 0u || depthDimU.y == 0u) {
        discard;
    }

    let depthSize = vec2f(depthDimU);
    let normalSize = vec2f(normalDimU);

    let pixelCoord = vec2i(fragCoord.xy);

    if (
        pixelCoord.x < 0 ||
        pixelCoord.y < 0 ||
        pixelCoord.x >= i32(depthDimU.x) ||
        pixelCoord.y >= i32(depthDimU.y)
    ) {
        discard;
    }

    let uv = (vec2f(pixelCoord) + vec2f(0.5)) / depthSize;

    let centerDepth = textureLoad(uSceneDepth, pixelCoord, 0);

    // Background / skybox: no occlusion
    if (centerDepth >= 0.9999) {
        return vec4f(vec3f(1.0), 1.0);
    }

    let normalPixelCoord = vec2i(clamp(
        vec2f(pixelCoord),
        vec2f(0.0),
        normalSize - vec2f(1.0)
    ));

    let centerNormalVS = loadNormalVS(normalPixelCoord);

    if (length(centerNormalVS) < 1e-4) {
        return vec4f(vec3f(1.0), 1.0);
    }

    let centerPositionVS = reconstructViewPosition(uv, centerDepth);

    let radius = max(uSsao.viewportSizeAndRadius.z, 1e-4);
    let bias = uSsao.aoParams.x;
    let power = max(uSsao.aoParams.y, 1e-4);
    let sampleCount = clamp(u32(round(uSsao.aoParams.z)), 1u, kKernelSize);

    let basis = buildBasis(centerNormalVS);

    let randomAngle = hash12(vec2f(pixelCoord)) * 6.28318530718;

    var occlusion = 0.0;
    var validSamples = 0.0;

    for (var i: u32 = 0u; i < sampleCount; i = i + 1u) {
        // 在采样的 TBN 进行旋转
        var sampleVectorVS = basis * kKernel[i];
        // 让每个像素的 kernel 绕 normal 随机旋转一下，减少条纹
        sampleVectorVS = rotateAroundNormal(sampleVectorVS, centerNormalVS, randomAngle);
        // 确保采样在法线半球方向
        if (dot(sampleVectorVS, centerNormalVS) < 0.0) {
            sampleVectorVS = -sampleVectorVS;
        }

        let samplePositionVS = centerPositionVS + sampleVectorVS * radius;
        let sampleUv = projectViewPositionToUv(samplePositionVS);

        let samplePixelCoord = vec2i(clamp(
            sampleUv * depthSize,
            vec2f(0.0),
            depthSize - vec2f(1.0)
        ));

        let sampleDepth = textureLoad(uSceneDepth, samplePixelCoord, 0);
        // 天空盒和背景不需要 AO
        if (sampleDepth >= 0.9999) {
            continue;
        }

        let sampleScenePositionVS = reconstructViewPosition(sampleUv, sampleDepth);
        // 越靠近相机，z 越大，如果 sampleScenePositionVS.z 比 samplePositionVS.z 更靠近相机，
        // 说明 sample ray 方向上被真实几何挡住了。
        let isOccluded = sampleScenePositionVS.z >= samplePositionVS.z + bias;
        let depthDiff = abs(centerPositionVS.z - sampleScenePositionVS.z);
        let rangeWeight = smoothstep(0.0, 1.0, radius / max(depthDiff, 1e-4));

        occlusion += select(0.0, rangeWeight, isOccluded);
        validSamples += 1.0;
    }

    if (validSamples < 0.5) {
        return vec4f(vec3f(1.0), 1.0);
    }

    // 遮挡比例转亮度
    let rawAo = 1.0 - occlusion / validSamples;
    // 非线性调整 AO 曲线
    let ao = pow(clamp(rawAo, 0.0, 1.0), power);
    return vec4f(vec3f(ao), 1.0);
}
