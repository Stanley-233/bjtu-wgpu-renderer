struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv0: vec2f,
    @location(3) uv1: vec2f,
    @location(4) color: vec4f,
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
};

struct FragmentInput {
    @builtin(position) position: vec4f,
    @builtin(front_facing) frontFacing: bool,
    @location(0) uv0: vec2f,
    @location(1) uv1: vec2f,
    @location(2) color: vec4f,
    @location(3) worldPos: vec3f,
    @location(4) normalWS: vec3f,
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

fn SelectUv(texCoordSet: u32, uv0: vec2f, uv1: vec2f) -> vec2f {
    return select(uv0, uv1, texCoordSet == 1u);
}

fn ResolveFaceSign(frontFacing: bool) -> f32 {
    return select(-1.0, 1.0, frontFacing);
}

fn SampleDirectionalShadow(worldPos: vec3f, normal: vec3f) -> f32 {
    let lightSpace = uDirectionalShadow.lightViewProjection * vec4f(worldPos, 1.0);
    let projected = lightSpace.xyz / max(lightSpace.w, 1e-6);
    let inShadowFrustum = lightSpace.w > 0.0 &&
        projected.x >= -1.0 && projected.x <= 1.0 &&
        projected.y >= -1.0 && projected.y <= 1.0 &&
        projected.z >= 0.0 && projected.z <= 1.0;
    let shadowUv = clamp(
        vec2f(projected.x * 0.5 + 0.5, 0.5 - projected.y * 0.5),
        vec2f(0.0),
        vec2f(1.0));
    let lightDir = normalize(-uScene.directionalLight.direction.xyz);
    let ndotl = max(dot(normal, lightDir), 0.0);
    let bias = max(uDirectionalShadow.shadowParams.y * (1.0 - ndotl), uDirectionalShadow.shadowParams.y * 0.25);
    let compareDepth = clamp(projected.z - bias, 0.0, 1.0);
    let shadowMapSize = textureDimensions(uDirectionalShadowMap);
    let texelSize = 1.0 / vec2f(f32(shadowMapSize.x), f32(shadowMapSize.y));

    var visibility = 0.0;
    for (var y = -1; y <= 1; y = y + 1) {
        for (var x = -1; x <= 1; x = x + 1) {
            visibility += textureSampleCompare(
                uDirectionalShadowMap,
                uDirectionalShadowSampler,
                shadowUv + vec2f(f32(x), f32(y)) * texelSize,
                compareDepth);
        }
    }
    return select(1.0, visibility / 9.0, inShadowFrustum);
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
    return out;
}

@fragment
fn fs_main(in: FragmentInput) -> @location(0) vec4f {
    let baseColorUv = SelectUv(uMaterial.textureCoordSets.x, in.uv0, in.uv1);
    let sampled = textureSample(uBaseColorTexture, uMaterialSampler, baseColorUv);
    var baseColor = sampled * uMaterial.baseColorFactor;
    if (uMaterial.surfaceOptions.y != 0u) {
        baseColor *= in.color;
    }
    let faceSign = ResolveFaceSign(in.frontFacing);
    let isDoubleSided = uMaterial.surfaceOptions.w != 0u;
    let normal = normalize(select(in.normalWS, in.normalWS * faceSign, isDoubleSided));

    let aoSize = max(vec2f(textureDimensions(uAmbientOcclusionTexture)), vec2f(1.0, 1.0));
    let aoUv = clamp(in.position.xy / aoSize, vec2f(0.0, 0.0), vec2f(1.0, 1.0));
    let ambientOcclusion = textureSample(uAmbientOcclusionTexture, uAmbientOcclusionSampler, aoUv).r;

    var shadowFactor = 1.0;
    if (uDirectionalShadow.shadowParams.x > 0.0) {
        shadowFactor = SampleDirectionalShadow(in.worldPos, normal);
    }
    var lighting = vec3f(0.25) * ambientOcclusion;
    if (uScene.lightCounts.x > 0u) {
        let lightDir = normalize(-uScene.directionalLight.direction.xyz);
        let ndotl = max(dot(normal, lightDir), 0.0);

        let ambientFactor = 0.2;
        let ambient = uScene.directionalLight.color.rgb * ambientFactor;
        let direct = uScene.directionalLight.color.rgb * ndotl;

        lighting = ambient * ambientOcclusion + direct * shadowFactor;
    }
    return vec4f(baseColor.rgb * lighting, baseColor.a);
}
