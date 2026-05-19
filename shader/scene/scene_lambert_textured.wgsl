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

struct MaterialUniform {
    baseColorFactor: vec4f,
    surfaceOptions: vec4u,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv0: vec2f,
    @location(1) color: vec4f,
    @location(2) worldPos: vec3f,
    @location(3) normalWS: vec3f,
};

@group(0) @binding(0) var<uniform> uScene: SceneUniform;
@group(1) @binding(0) var<uniform> uObject: ObjectUniform;
@group(2) @binding(0) var<uniform> uMaterial: MaterialUniform;
@group(2) @binding(1) var uBaseColorTexture: texture_2d<f32>;
@group(2) @binding(2) var uBaseColorSampler: sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let worldPosition = uObject.model * vec4f(in.position, 1.0);
    out.position = uScene.projection * uScene.view * worldPosition;
    out.uv0 = in.uv0;
    out.color = in.color;
    out.worldPos = worldPosition.xyz;
    out.normalWS = normalize((uObject.normalMatrix * vec4f(in.normal, 0.0)).xyz);
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let sampled = textureSample(uBaseColorTexture, uBaseColorSampler, in.uv0);
    var baseColor = sampled * uMaterial.baseColorFactor;
    if (uMaterial.surfaceOptions.y != 0u) {
        baseColor *= in.color;
    }

    var diffuseLighting = vec3f(0.3, 0.3, 0.3);
    if (uScene.lightCounts.x > 0u) {
        let normal = normalize(in.normalWS);
        let lightDir = normalize(-uScene.directionalLight.direction.xyz);
        let ndotl = max(dot(normal, lightDir), 0.0);
        let ambientFactor = 0.15;
        diffuseLighting = uScene.directionalLight.color.rgb * ambientFactor;
        diffuseLighting += uScene.directionalLight.color.rgb * ndotl;
    }

    return vec4f(baseColor.rgb * diffuseLighting, baseColor.a);
}
