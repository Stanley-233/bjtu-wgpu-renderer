struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv0: vec2f,
    @location(3) color: vec4f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv0: vec2f,
    @location(1) color: vec4f,
};

struct SceneUniform {
    model: mat4x4f,
    view: mat4x4f,
    projection: mat4x4f,
};

struct MaterialUniform {
    baseColorFactor: vec4f,
};

@group(0) @binding(0) var<uniform> uScene: SceneUniform;
@group(0) @binding(1) var<uniform> uMaterial: MaterialUniform;
@group(0) @binding(2) var uBaseColorTexture: texture_2d<f32>;
@group(0) @binding(3) var uBaseColorSampler: sampler;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = uScene.projection * uScene.view * uScene.model * vec4f(in.position, 1.0);
    out.uv0 = in.uv0;
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let sampled = textureSample(uBaseColorTexture, uBaseColorSampler, in.uv0);
    return sampled * uMaterial.baseColorFactor * in.color;
}
