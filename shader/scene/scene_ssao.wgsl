struct VertexOutput {
    @builtin(position) position: vec4f,
};

@group(0) @binding(0) var uSceneDepth: texture_depth_2d;

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
fn fs_main() -> @location(0) vec4f {
    let depthSize = textureDimensions(uSceneDepth);
    if (depthSize.x == 0u || depthSize.y == 0u) {
        discard;
    }
    return vec4f(1.0, 1.0, 1.0, 1.0);
}
