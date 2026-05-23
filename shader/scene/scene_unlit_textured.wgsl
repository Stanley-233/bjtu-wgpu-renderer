struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv0: vec2f,
    @location(3) uv1: vec2f,
    @location(4) color: vec4f,
};

struct DirectionalLightData {
    direction: vec4f,
    color: vec4f,
};

struct PointLightData {
    position: vec4f,
    color: vec4f,
    attenuation: vec4f,
};

struct SpotLightData {
    position: vec4f,
    direction: vec4f,
    color: vec4f,
    angles: vec4f,
};

struct SceneUniform {
    view: mat4x4f,
    projection: mat4x4f,
    cameraPosition: vec4f,
    lightCounts: vec4u,
    directionalLight: DirectionalLightData,
    pointLights: array<PointLightData, 8>,
    spotLights: array<SpotLightData, 8>,
};

struct ObjectUniform {
    model: mat4x4f,
    normalMatrix: mat4x4f,
};

struct MaterialUniform {
    baseColorFactor: vec4f,
    pbrParams: vec4f,
    specularParams: vec4f,
    textureCoordSets: vec4u,
    surfaceOptions: vec4u,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv0: vec2f,
    @location(1) uv1: vec2f,
    @location(2) color: vec4f,
};

@group(0) @binding(0) var<uniform> uScene: SceneUniform;
@group(1) @binding(0) var<uniform> uObject: ObjectUniform;
@group(2) @binding(0) var<uniform> uMaterial: MaterialUniform;
@group(2) @binding(1) var uBaseColorTexture: texture_2d<f32>;
@group(2) @binding(2) var uNormalTexture: texture_2d<f32>;
@group(2) @binding(3) var uMetallicRoughnessTexture: texture_2d<f32>;
@group(2) @binding(4) var uMaterialSampler: sampler;

fn SelectUv(texCoordSet: u32, uv0: vec2f, uv1: vec2f) -> vec2f {
    return select(uv0, uv1, texCoordSet == 1u);
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let worldPosition = uObject.model * vec4f(in.position, 1.0);
    out.position = uScene.projection * uScene.view * worldPosition;
    out.uv0 = in.uv0;
    out.uv1 = in.uv1;
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let baseColorUv = SelectUv(uMaterial.textureCoordSets.x, in.uv0, in.uv1);
    var surfaceColor = textureSample(uBaseColorTexture, uMaterialSampler, baseColorUv) * uMaterial.baseColorFactor;
    if (uMaterial.surfaceOptions.y != 0u) {
        surfaceColor *= in.color;
    }
    return surfaceColor;
}
