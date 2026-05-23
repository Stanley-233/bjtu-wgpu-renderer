struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

struct DofUniform {
    invProjection: mat4x4f,
    viewportAndBlur: vec4f,
    focusParams: vec4f,
    blurDirection: vec4f,
};

@group(0) @binding(0) var uSceneDepth: texture_depth_2d;
@group(0) @binding(1) var<uniform> uDof: DofUniform;

fn reconstructViewPosition(uv: vec2f, depth: f32, invProjection: mat4x4f) -> vec3f {
    let clipPosition = vec4f(
        uv.x * 2.0 - 1.0,
        1.0 - uv.y * 2.0,
        depth,
        1.0
    );
    let viewPosition = invProjection * clipPosition;
    return viewPosition.xyz / viewPosition.w;
}

@vertex
fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
    var positions = array<vec2f, 3>(
        vec2f(-1.0, -3.0),
        vec2f(-1.0, 1.0),
        vec2f(3.0, 1.0),
    );
    var uvs = array<vec2f, 3>(
        vec2f(0.0, 2.0),
        vec2f(0.0, 0.0),
        vec2f(2.0, 0.0),
    );

    var out: VertexOutput;
    out.position = vec4f(positions[vertexIndex], 0.0, 1.0);
    out.uv = uvs[vertexIndex];
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) f32 {
    let depthDim = textureDimensions(uSceneDepth);
    let pixelCoord = vec2i(clamp(
        in.uv * vec2f(depthDim),
        vec2f(0.0),
        vec2f(depthDim) - vec2f(1.0)
    ));
    let depth = textureLoad(uSceneDepth, pixelCoord, 0);

    if (depth >= 0.999999) {
        let farViewPos = reconstructViewPosition(in.uv, depth, uDof.invProjection);
        let farDepth = max(-farViewPos.z, uDof.focusParams.x + uDof.focusParams.y);
        let signedCoc = clamp(
            (farDepth - uDof.focusParams.x) / max(uDof.focusParams.y, 1e-4),
            -1.0,
            1.0);
        return signedCoc;
    }

    let viewPosition = reconstructViewPosition(in.uv, depth, uDof.invProjection);
    let linearDepth = max(-viewPosition.z, 0.0);
    let signedCoc = clamp(
        (linearDepth - uDof.focusParams.x) / max(uDof.focusParams.y, 1e-4),
        -1.0,
        1.0);
    return signedCoc;
}
