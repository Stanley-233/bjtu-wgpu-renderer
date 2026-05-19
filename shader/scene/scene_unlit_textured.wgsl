struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv0: vec2f,
    @location(3) color: vec4f,
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

struct DirectionalShadowUniform {
    lightViewProjection: mat4x4f,
    shadowParams: vec4f,
};

struct MaterialUniform {
    baseColorFactor: vec4f,
    surfaceOptions: vec4u,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv0: vec2f,
    @location(1) color: vec4f,
};

@group(0) @binding(0) var<uniform> uScene: SceneUniform;
@group(0) @binding(1) var<uniform> uDirectionalShadow: DirectionalShadowUniform;
@group(0) @binding(2) var uDirectionalShadowMap: texture_depth_2d;
@group(0) @binding(3) var uDirectionalShadowSampler: sampler_comparison;
@group(1) @binding(0) var<uniform> uObject: ObjectUniform;
@group(2) @binding(0) var<uniform> uMaterial: MaterialUniform;
@group(2) @binding(1) var uBaseColorTexture: texture_2d<f32>;
@group(2) @binding(2) var uBaseColorSampler: sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = uScene.projection * uScene.view * uObject.model * vec4f(in.position, 1.0);
    out.uv0 = in.uv0;
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    var surfaceColor = textureSample(uBaseColorTexture, uBaseColorSampler, in.uv0) * uMaterial.baseColorFactor;
    if (uMaterial.surfaceOptions.y != 0u) {
        surfaceColor *= in.color;
    }
    // TODO: [Shadow] Unlit 材质通常不参与阴影，后续按材质策略决定是否忽略 directional shadow。
    let _unusedShadowEnabled = uDirectionalShadow.shadowParams.x;
    return surfaceColor;
}
