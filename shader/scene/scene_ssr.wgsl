struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

struct SsrUniform {
    projection: mat4x4f,
    invProjection: mat4x4f,
    viewport: vec4f,
    params: vec4f,
};

@group(0) @binding(0) var uSceneDepth: texture_depth_2d;
@group(0) @binding(1) var uSceneNormal: texture_2d<f32>;
@group(0) @binding(2) var uSceneReflectivity: texture_2d<f32>;
@group(0) @binding(3) var uSceneColor: texture_2d<f32>;
@group(0) @binding(4) var uSceneSampler: sampler;
@group(0) @binding(5) var<uniform> uSsr: SsrUniform;

fn saturate(v: f32) -> f32 {
    return clamp(v, 0.0, 1.0);
}

fn reconstructViewPosition(uv: vec2f, depth: f32) -> vec3f {
    let clipPosition = vec4f(
        uv.x * 2.0 - 1.0,
        1.0 - uv.y * 2.0,
        depth,
        1.0
    );
    let viewPosition = uSsr.invProjection * clipPosition;
    return viewPosition.xyz / viewPosition.w;
}

fn projectViewPositionToUv(positionVS: vec3f) -> vec2f {
    let clipPosition = uSsr.projection * vec4f(positionVS, 1.0);
    let ndc = clipPosition.xy / max(clipPosition.w, 1.0e-5);
    return vec2f(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
}

fn fresnelSchlick(cosTheta: f32, f0: vec3f) -> vec3f {
    let oneMinusCos = 1.0 - saturate(cosTheta);
    let oneMinusCos2 = oneMinusCos * oneMinusCos;
    let oneMinusCos5 = oneMinusCos2 * oneMinusCos2 * oneMinusCos;
    return f0 + (vec3f(1.0) - f0) * oneMinusCos5;
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
    let pixelCoord = vec2i(in.uv * uSsr.viewport.xy);
    let color = textureSampleLevel(uSceneColor, uSceneSampler, clamp(in.uv, vec2f(0.0), vec2f(1.0)), 0.0);
    let depthDimensions = textureDimensions(uSceneDepth);

    if (pixelCoord.x < 0
        || pixelCoord.y < 0
        || pixelCoord.x >= i32(depthDimensions.x)
        || pixelCoord.y >= i32(depthDimensions.y)) {
        return color;
    }

    let centerDepth = textureLoad(uSceneDepth, pixelCoord, 0);
    if (centerDepth >= 0.9999) {
        return color;
    }

    let normalVs = normalize(textureLoad(uSceneNormal, pixelCoord, 0).xyz);
    let reflectivity = textureLoad(uSceneReflectivity, pixelCoord, 0);
    let f0 = reflectivity.rgb;
    let roughness = clamp(reflectivity.a, 0.045, 1.0);
    if (dot(f0, f0) < 1.0e-6) {
        return color;
    }

    let centerPositionVs = reconstructViewPosition(in.uv, centerDepth);
    let viewDir = normalize(-centerPositionVs);
    let ndotv = saturate(dot(normalVs, viewDir));
    let reflectionDir = normalize(reflect(-viewDir, normalVs));
    if (reflectionDir.z >= -1.0e-4) {
        return color;
    }

    let stepCount = max(i32(round(uSsr.params.w)), 1);
    let maxDistance = max(uSsr.params.y, 0.1);
    let stepLength = maxDistance / f32(stepCount);
    let thickness = max(uSsr.params.z, 0.001);

    var hitUv = vec2f(0.0);
    var hitT = 0.0;
    var hit = false;

    for (var step = 1; step <= stepCount; step = step + 1) {
        let t = f32(step) * stepLength;
        let rayPositionVs = centerPositionVs + reflectionDir * t;
        let rayUv = projectViewPositionToUv(rayPositionVs);
        if (any(rayUv < vec2f(0.0)) || any(rayUv > vec2f(1.0))) {
            break;
        }

        let samplePixelCoord = vec2i(clamp(
            rayUv * uSsr.viewport.xy,
            vec2f(0.0),
            uSsr.viewport.xy - vec2f(1.0)
        ));
        let sampleDepth = textureLoad(uSceneDepth, samplePixelCoord, 0);
        if (sampleDepth >= 0.9999) {
            continue;
        }

        let samplePositionVs = reconstructViewPosition(rayUv, sampleDepth);
        let depthDelta = samplePositionVs.z - rayPositionVs.z;
        if (depthDelta >= 0.0 && depthDelta <= thickness) {
            hitUv = rayUv;
            hitT = t;
            hit = true;
            break;
        }
    }

    if (!hit) {
        return color;
    }

    let reflectedColor = textureSampleLevel(
        uSceneColor,
        uSceneSampler,
        clamp(hitUv, vec2f(0.0), vec2f(1.0)),
        0.0
    ).rgb;
    let fresnel = fresnelSchlick(ndotv, f0);
    let roughnessWeight = pow(1.0 - roughness, 1.25);
    let edgeUv = abs(hitUv * 2.0 - 1.0);
    let edgeFade = saturate(1.0 - max(edgeUv.x, edgeUv.y));
    let distanceFade = saturate(1.0 - hitT / maxDistance);
    let weight = fresnel * (uSsr.params.x * roughnessWeight * edgeFade * distanceFade);
    let finalColor = mix(color.rgb, reflectedColor, clamp(weight, vec3f(0.0), vec3f(1.0)));
    return vec4f(finalColor, color.a);
}
