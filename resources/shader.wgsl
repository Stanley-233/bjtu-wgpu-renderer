struct VertexInput {
    @location(0) position: vec2f,
    @location(1) color: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

@group(0) @binding(0) var<uniform> uTransform: mat3x3f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let ratio = 640.0 / 480.0;
    
    // Apply transform matrix to position
    var offset = vec2f(-0.6875, -0.463);
    var transformed = uTransform * vec3f(in.position+offset, 1.0);
    var ndc_position = vec2f(transformed.x, (transformed.y) * ratio);
    
    out.position = vec4f(ndc_position, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
//    let linear_color = pow(in.color, vec3f(2.2));
//    return vec4f(linear_color, 1.0);
    return vec4f(in.color, 1.0);
}