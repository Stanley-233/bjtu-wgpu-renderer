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

@group(0) @binding(0) var uSceneColor: texture_2d<f32>;
@group(0) @binding(1) var uSceneCoc: texture_2d<f32>;
@group(0) @binding(2) var uSceneColorSampler: sampler;
@group(0) @binding(3) var uSceneCocSampler: sampler;
@group(0) @binding(4) var<uniform> uDof: DofUniform;
@group(0) @binding(5) var uSceneDepth: texture_depth_2d;

const kWeight1 = 0.1945946;
const kWeight2 = 0.1216216;
const kWeight3 = 0.054054;
const kWeight4 = 0.016216;

fn absCoCToRadiusPx(absCoC: f32, maxRadiusPx: f32) -> f32 {
    return clamp(absCoC, 0.0, 1.0) * maxRadiusPx;
}

fn clampUv(uv: vec2f) -> vec2f {
    return clamp(uv, vec2f(0.0), vec2f(1.0));
}

fn sampleSignedCoc(uv: vec2f) -> f32 {
    return textureSample(uSceneCoc, uSceneCocSampler, clampUv(uv)).r;
}

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

fn accumulateTap(
    accumColor: ptr<function, vec3f>,
    accumWeight: ptr<function, f32>,
    centerCoc: f32,
    uv: vec2f,
    weightBase: f32
) {
    let coc = sampleSignedCoc(uv);
    let acceptTap = abs(coc) < 0.05 || sign(coc) == sign(centerCoc);
    if (!acceptTap) {
        return;
    }

    let color = textureSample(uSceneColor, uSceneColorSampler, uv).rgb;
    let cocWeight = clamp(abs(coc), 0.15, 1.0);
    *accumColor += color * weightBase * cocWeight;
    *accumWeight += weightBase * cocWeight;
}

fn accumulateSymmetricTap(
    accumColor: ptr<function, vec3f>,
    accumWeight: ptr<function, f32>,
    centerCoc: f32,
    centerUv: vec2f,
    offset: vec2f,
    weightBase: f32
) {
    accumulateTap(accumColor, accumWeight, centerCoc, clampUv(centerUv + offset), weightBase);
    accumulateTap(accumColor, accumWeight, centerCoc, clampUv(centerUv - offset), weightBase);
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
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let texelSize = 1.0 / max(uDof.viewportAndBlur.xy, vec2f(1.0));
    let centerColor = textureSample(uSceneColor, uSceneColorSampler, in.uv);
    let centerCoc = sampleSignedCoc(in.uv);
    let centerRadiusPx = absCoCToRadiusPx(abs(centerCoc), uDof.viewportAndBlur.z);

    var finalColor = centerColor;

    if (centerRadiusPx >= 0.5) {
        var accumColor = centerColor.rgb * 0.227027;
        var accumWeight = 0.227027;

        if (abs(uDof.blurDirection.x) > 0.5) {
            let tapRadiusPx1 = centerRadiusPx * 0.25;
            let tapOffset1 = vec2f(tapRadiusPx1 * texelSize.x, 0.0);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, tapOffset1, kWeight1);

            let tapRadiusPx2 = centerRadiusPx * 0.5;
            let tapOffset2 = vec2f(tapRadiusPx2 * texelSize.x, 0.0);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, tapOffset2, kWeight2);

            let tapRadiusPx3 = centerRadiusPx * 0.75;
            let tapOffset3 = vec2f(tapRadiusPx3 * texelSize.x, 0.0);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, tapOffset3, kWeight3);

            let tapRadiusPx4 = centerRadiusPx;
            let tapOffset4 = vec2f(tapRadiusPx4 * texelSize.x, 0.0);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, tapOffset4, kWeight4);
        } else {
            let tapRadiusPx1 = centerRadiusPx * 0.35;
            let verticalOffset1 = vec2f(0.0, tapRadiusPx1 * texelSize.y);
            let diagonalOffset1 = vec2f(tapRadiusPx1 * texelSize.x * 0.7, tapRadiusPx1 * texelSize.y * 0.7);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, verticalOffset1, kWeight1);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, diagonalOffset1, kWeight1 * 0.85);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, vec2f(-diagonalOffset1.x, diagonalOffset1.y), kWeight1 * 0.85);

            let tapRadiusPx2 = centerRadiusPx * 0.65;
            let verticalOffset2 = vec2f(0.0, tapRadiusPx2 * texelSize.y);
            let diagonalOffset2 = vec2f(tapRadiusPx2 * texelSize.x * 0.7, tapRadiusPx2 * texelSize.y * 0.7);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, verticalOffset2, kWeight2);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, diagonalOffset2, kWeight2 * 0.85);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, vec2f(-diagonalOffset2.x, diagonalOffset2.y), kWeight2 * 0.85);

            let tapRadiusPx3 = centerRadiusPx;
            let verticalOffset3 = vec2f(0.0, tapRadiusPx3 * texelSize.y);
            let horizontalOffset3 = vec2f(tapRadiusPx3 * texelSize.x * 0.6, 0.0);
            let diagonalOffset3 = vec2f(tapRadiusPx3 * texelSize.x * 0.7, tapRadiusPx3 * texelSize.y * 0.7);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, verticalOffset3, kWeight3);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, horizontalOffset3, kWeight3 * 0.65);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, diagonalOffset3, kWeight4);
            accumulateSymmetricTap(&accumColor, &accumWeight, centerCoc, in.uv, vec2f(-diagonalOffset3.x, diagonalOffset3.y), kWeight4);
        }

        finalColor = vec4f(accumColor / max(accumWeight, 1e-4), centerColor.a);
    }

    if (u32(uDof.focusParams.w) == 1u && abs(uDof.blurDirection.y) > 0.5) {
        let depthDim = textureDimensions(uSceneDepth);
        let pixelCoord = vec2i(clamp(
            in.uv * vec2f(depthDim),
            vec2f(0.0),
            vec2f(depthDim) - vec2f(1.0)
        ));
        let depth = textureLoad(uSceneDepth, pixelCoord, 0);
        let viewPosition = reconstructViewPosition(in.uv, depth, uDof.invProjection);
        let linearDepth = max(-viewPosition.z, 0.0);
        if (abs(linearDepth - uDof.focusParams.x) <= uDof.focusParams.z) {
            finalColor = vec4f(
                mix(finalColor.rgb, vec3f(0.72, 0.25, 0.95), 0.55),
                finalColor.a);
        }
    }
    return finalColor;
}
