struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv0: vec2f,
    @location(3) uv1: vec2f,
    @location(4) color: vec4f,
    @location(5) tangent: vec4f,
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

struct PbrDebugUniform {
    options: vec4u,
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
    pbrParams: vec4f,
    textureCoordSets: vec4u,
    surfaceOptions: vec4u,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv0: vec2f,
    @location(1) uv1: vec2f,
    @location(2) color: vec4f,
    @location(3) worldPos: vec3f,
    @location(4) normalWS: vec3f,
    @location(5) tangentWS: vec4f,
};

@group(0) @binding(0) var<uniform> uScene: SceneUniform;
@group(0) @binding(1) var<uniform> uDirectionalShadow: DirectionalShadowUniform;
@group(0) @binding(2) var uDirectionalShadowMap: texture_depth_2d;
@group(0) @binding(3) var uDirectionalShadowSampler: sampler_comparison;
@group(0) @binding(4) var uAmbientOcclusionTexture: texture_2d<f32>;
@group(0) @binding(5) var uAmbientOcclusionSampler: sampler;
@group(1) @binding(0) var<uniform> uObject: ObjectUniform;
@group(2) @binding(0) var<uniform> uMaterial: MaterialUniform;
@group(2) @binding(1) var uBaseColorTexture: texture_2d<f32>;
@group(2) @binding(2) var uNormalTexture: texture_2d<f32>;
@group(2) @binding(3) var uMetallicRoughnessTexture: texture_2d<f32>;
@group(2) @binding(4) var uMaterialSampler: sampler;
@group(3) @binding(0) var<uniform> uPbrDebug: PbrDebugUniform;

fn SelectUv(texCoordSet: u32, uv0: vec2f, uv1: vec2f) -> vec2f {
    return select(uv0, uv1, texCoordSet == 1u);
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let worldPosition = uObject.model * vec4f(in.position, 1.0);
    out.position = uScene.projection * uScene.view * worldPosition;
    out.uv0 = in.uv0;
    out.uv1 = in.uv1;
    out.color = in.color;
    out.worldPos = worldPosition.xyz;
    out.normalWS = normalize((uObject.normalMatrix * vec4f(in.normal, 0.0)).xyz);
    let tangentWS = (uObject.normalMatrix * vec4f(in.tangent.xyz, 0.0)).xyz;
    out.tangentWS = vec4f(normalize(tangentWS), in.tangent.w);
    return out;
}

fn BuildCotangentFrame(normalWS: vec3f, worldPos: vec3f, uv: vec2f) -> mat3x3f {
    let dpdxPos = dpdx(worldPos);
    let dpdyPos = dpdy(worldPos);
    let dpdxUv = dpdx(uv);
    let dpdyUv = dpdy(uv);

    let dp2Perp = cross(dpdyPos, normalWS);
    let dp1Perp = cross(normalWS, dpdxPos);
    var tangent = dp2Perp * dpdxUv.x + dp1Perp * dpdyUv.x;
    var bitangent = dp2Perp * dpdxUv.y + dp1Perp * dpdyUv.y;
    let invScale = inverseSqrt(max(max(dot(tangent, tangent), dot(bitangent, bitangent)), 1.0e-8));
    tangent *= invScale;
    bitangent *= invScale;
    return mat3x3f(tangent, bitangent, normalWS);
}

fn SampleNormalWS(in: VertexOutput) -> vec3f {
    let normalUv = SelectUv(uMaterial.textureCoordSets.y, in.uv0, in.uv1);
    let sampledNormal = textureSample(uNormalTexture, uMaterialSampler, normalUv).xyz * 2.0 - vec3f(1.0, 1.0, 1.0);
    let normalScale = select(uMaterial.pbrParams.z, 1.0, uMaterial.pbrParams.z <= 0.0);
    let scaledXY = sampledNormal.xy * normalScale;
    let normalTS = normalize(vec3f(
        scaledXY.x,
        scaledXY.y,
        max(sampledNormal.z, 1.0e-4),
    ));
    let geometricNormal = normalize(in.normalWS);
    var tbn = BuildCotangentFrame(geometricNormal, in.worldPos, normalUv);
    if (length(in.tangentWS.xyz) > 1.0e-5) {
        let tangentWS = normalize(in.tangentWS.xyz);
        let bitangentWS = normalize(cross(geometricNormal, tangentWS) * in.tangentWS.w);
        tbn = mat3x3f(tangentWS, bitangentWS, geometricNormal);
    }
    return normalize(tbn * normalTS);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let baseColorUv = SelectUv(uMaterial.textureCoordSets.x, in.uv0, in.uv1);
    let sampled = textureSample(uBaseColorTexture, uMaterialSampler, baseColorUv);
    var baseColor = sampled * uMaterial.baseColorFactor;
    if (uMaterial.surfaceOptions.y != 0u) {
        baseColor *= in.color;
    }
    let geometricNormal = normalize(in.normalWS);
    let normal = SampleNormalWS(in);
    let pbrDebugView = uPbrDebug.options.x;
    if (pbrDebugView == 1u) {
        return vec4f(geometricNormal * 0.5 + vec3f(0.5, 0.5, 0.5), 1.0);
    }
    if (pbrDebugView == 2u) {
        return vec4f(normal * 0.5 + vec3f(0.5, 0.5, 0.5), 1.0);
    }
    if (pbrDebugView == 3u) {
        return vec4f(abs(normal - geometricNormal), 1.0);
    }

    let aoSize = max(vec2f(textureDimensions(uAmbientOcclusionTexture)), vec2f(1.0, 1.0));
    let aoUv = clamp(in.position.xy / aoSize, vec2f(0.0, 0.0), vec2f(1.0, 1.0));
    let ambientOcclusion = textureSample(uAmbientOcclusionTexture, uAmbientOcclusionSampler, aoUv).r;

    var lighting = vec3f(0.25) * ambientOcclusion;
    if (uScene.lightCounts.x > 0u) {
        let lightDir = normalize(-uScene.directionalLight.direction.xyz);
        let ndotl = max(dot(normal, lightDir), 0.0);

        let ambientFactor = 0.2;
        let ambient = uScene.directionalLight.color.rgb * ambientFactor;
        let direct = uScene.directionalLight.color.rgb * ndotl;

        lighting = ambient * ambientOcclusion + direct;
    }
    return vec4f(baseColor.rgb * lighting, baseColor.a);
}
