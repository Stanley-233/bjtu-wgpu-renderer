struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv0: vec2f,
    @location(3) uv1: vec2f,
    @location(4) color: vec4f,
    @location(5) tangent: vec4f,
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

struct ObjectUniform {
    model: mat4x4f,
    normalMatrix: mat4x4f,
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
    @location(2) worldPos: vec3f,
    @location(3) normalWS: vec3f,
    @location(4) tangentWS: vec4f,
    @location(5) color: vec4f,
};

struct FragmentOutput {
    @location(0) normalVs: vec4f,
    @location(1) reflectivity: vec4f,
};

@group(0) @binding(0) var<uniform> uScene: SceneUniform;
@group(1) @binding(0) var<uniform> uObject: ObjectUniform;
@group(2) @binding(0) var<uniform> uMaterial: MaterialUniform;
@group(2) @binding(1) var uBaseColorTexture: texture_2d<f32>;
@group(2) @binding(2) var uNormalTexture: texture_2d<f32>;
@group(2) @binding(3) var uMetallicRoughnessTexture: texture_2d<f32>;
@group(2) @binding(4) var uMaterialSampler: sampler;

fn SelectUv(texCoordSet: u32, uv0: vec2f, uv1: vec2f) -> vec2f {
    return select(uv0, uv1, texCoordSet == 1u);
}

fn Saturate(value: f32) -> f32 {
    return clamp(value, 0.0, 1.0);
}

fn ResolveFaceSign(frontFacing: bool) -> f32 {
    return select(-1.0, 1.0, frontFacing);
}

fn SampleBaseColor(in: VertexOutput) -> vec4f {
    let baseColorUv = SelectUv(uMaterial.textureCoordSets.x, in.uv0, in.uv1);
    var baseColor = textureSample(uBaseColorTexture, uMaterialSampler, baseColorUv) * uMaterial.baseColorFactor;
    if (uMaterial.surfaceOptions.y != 0u) {
        baseColor *= in.color;
    }
    return baseColor;
}

fn SampleMetallicRoughness(in: VertexOutput) -> vec2f {
    let mrUv = SelectUv(uMaterial.textureCoordSets.z, in.uv0, in.uv1);
    let mrSample = textureSample(uMetallicRoughnessTexture, uMaterialSampler, mrUv);
    let metallic = Saturate(mrSample.b * uMaterial.pbrParams.x);
    let roughness = clamp(mrSample.g * uMaterial.pbrParams.y, 0.045, 1.0);
    return vec2f(metallic, roughness);
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

fn ResolveGeometricNormal(in: VertexOutput, frontFacing: bool) -> vec3f {
    let faceSign = ResolveFaceSign(frontFacing);
    let isDoubleSided = uMaterial.surfaceOptions.w != 0u;
    return normalize(select(in.normalWS, in.normalWS * faceSign, isDoubleSided));
}

fn SampleNormalWS(in: VertexOutput, geometricNormal: vec3f, frontFacing: bool) -> vec3f {
    if (uMaterial.surfaceOptions.z == 0u) {
        return geometricNormal;
    }

    let normalUv = SelectUv(uMaterial.textureCoordSets.y, in.uv0, in.uv1);
    let sampledNormal = textureSample(uNormalTexture, uMaterialSampler, normalUv).xyz * 2.0 - vec3f(1.0, 1.0, 1.0);
    let normalScale = select(uMaterial.pbrParams.z, 1.0, uMaterial.pbrParams.z <= 0.0);
    let normalTS = normalize(vec3f(
        sampledNormal.xy * normalScale,
        max(sampledNormal.z, 1.0e-4),
    ));

    var tbn = BuildCotangentFrame(geometricNormal, in.worldPos, normalUv);
    if (length(in.tangentWS.xyz) > 1.0e-5) {
        let faceSign = ResolveFaceSign(frontFacing);
        let tangentHandedness = select(in.tangentWS.w, in.tangentWS.w * faceSign, uMaterial.surfaceOptions.w != 0u);
        let tangentWS = normalize(in.tangentWS.xyz);
        let bitangentWS = normalize(cross(geometricNormal, tangentWS) * tangentHandedness);
        tbn = mat3x3f(tangentWS, bitangentWS, geometricNormal);
    }
    return normalize(tbn * normalTS);
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let worldPosition = uObject.model * vec4f(in.position, 1.0);
    out.position = uScene.projection * uScene.view * worldPosition;
    out.uv0 = in.uv0;
    out.uv1 = in.uv1;
    out.worldPos = worldPosition.xyz;
    out.normalWS = normalize((uObject.normalMatrix * vec4f(in.normal, 0.0)).xyz);
    let tangentWS = (uObject.normalMatrix * vec4f(in.tangent.xyz, 0.0)).xyz;
    out.tangentWS = vec4f(normalize(tangentWS), in.tangent.w);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput, @builtin(front_facing) frontFacing: bool) -> FragmentOutput {
    let geometricNormal = ResolveGeometricNormal(in, frontFacing);
    let normalWs = SampleNormalWS(in, geometricNormal, frontFacing);
    let normalVs = normalize((uScene.view * vec4f(normalWs, 0.0)).xyz);

    let baseColor = SampleBaseColor(in);
    let metallicRoughness = SampleMetallicRoughness(in);
    let metallic = metallicRoughness.x;
    let roughness = metallicRoughness.y;
    let shadingModel = uMaterial.surfaceOptions.x;
    let f0 = mix(vec3f(0.04, 0.04, 0.04), baseColor.rgb, vec3f(metallic));

    var out: FragmentOutput;
    out.normalVs = vec4f(normalVs, 1.0);
    out.reflectivity = select(
        vec4f(0.0, 0.0, 0.0, 1.0),
        vec4f(f0, roughness),
        shadingModel == 2u);
    return out;
}
