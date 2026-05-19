struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv0: vec2f,
    @location(3) color: vec4f,
};

struct DirectionalShadowUniform {
    lightViewProjection: mat4x4f,
    shadowParams: vec4f,
};

struct ShadowObjectUniform {
    model: mat4x4f,
};

@group(0) @binding(0) var<uniform> uDirectionalShadow: DirectionalShadowUniform;
@group(1) @binding(0) var<uniform> uObject: ShadowObjectUniform;

@vertex
fn vs_main(in: VertexInput) -> @builtin(position) vec4f {
    let worldPosition = uObject.model * vec4f(in.position, 1.0);
    // TODO: [Shadow] 接入 depth bias 配置，降低 shadow acne
    // 当前只按普通静态网格处理阴影投射
    return uDirectionalShadow.lightViewProjection * worldPosition;
}
