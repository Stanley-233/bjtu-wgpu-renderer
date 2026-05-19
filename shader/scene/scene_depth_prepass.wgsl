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

@group(0) @binding(0) var<uniform> uScene: SceneUniform;
@group(1) @binding(0) var<uniform> uObject: ObjectUniform;

@vertex
fn vs_main(in: VertexInput) -> @builtin(position) vec4f {
    let worldPosition = uObject.model * vec4f(in.position, 1.0);
    return uScene.projection * uScene.view * worldPosition;
}
