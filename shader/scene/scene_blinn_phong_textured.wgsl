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
    // position.xyz = world position, position.w = reserved
    position: vec4f,
    // color.rgb = light color * intensity, color.w = reserved
    color: vec4f,
    // attenuation.xyz = constant / linear / quadratic, attenuation.w = reserved
    attenuation: vec4f,
};

struct SpotLightData {
    // position.xyz = world position, position.w = reserved
    position: vec4f,
    // direction.xyz = normalized world direction, direction.w = reserved
    direction: vec4f,
    // color.rgb = light color * intensity, color.w = reserved
    color: vec4f,
    // angles.xy = cos(inner) / cos(outer), angles.z = range, angles.w = reserved
    angles: vec4f,
};

struct SceneUniform {
    view: mat4x4f,
    projection: mat4x4f,
    // cameraPosition.xyz = world-space camera position, cameraPosition.w = reserved
    cameraPosition: vec4f,
    // lightCounts.x = directional count, y = point count, z = spot count, w = reserved
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
    specularParams: vec4f,
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

const kAmbientStrength = 0.2;
const kSpecularStrength = 0.35;
const kShininess = 32.0;

fn SelectUv(texCoordSet: u32, uv0: vec2f, uv1: vec2f) -> vec2f {
    return select(uv0, uv1, texCoordSet == 1u);
}

fn ResolveFaceSign(frontFacing: bool) -> f32 {
    return select(-1.0, 1.0, frontFacing);
}

fn Saturate(value: f32) -> f32 {
    return clamp(value, 0.0, 1.0);
}

fn ComputeSpecular(lightDir: vec3f, viewDir: vec3f, normal: vec3f) -> f32 {
    let halfDir = normalize(lightDir + viewDir);
    return pow(Saturate(dot(normal, halfDir)), kShininess);
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

fn ComputeDistanceAttenuation(attenuationParams: vec4f, distance: f32) -> f32 {
    let d = max(distance, 0.0);
    let denominator = attenuationParams.x + attenuationParams.y * d + attenuationParams.z * d * d;
    return 1.0 / max(denominator, 1.0e-4);
}

fn ComputeBlinnPhongLight(lightColor: vec3f, lightDir: vec3f, viewDir: vec3f, normal: vec3f) -> vec3f {
    let ndotl = Saturate(dot(normal, lightDir));
    if (ndotl <= 0.0) {
        return vec3f(0.0);
    }

    let diffuse = lightColor * ndotl;
    let specular = lightColor * ComputeSpecular(lightDir, viewDir, normal) * kSpecularStrength;
    return diffuse + specular;
}

fn ComputePointLightBlinnPhong(light: PointLightData, worldPos: vec3f, viewDir: vec3f, normal: vec3f) -> vec3f {
    let lightVector = light.position.xyz - worldPos;
    let distance = length(lightVector);
    if (distance <= 1.0e-4) {
        return vec3f(0.0, 0.0, 0.0);
    }

    let lightDir = lightVector / distance;
    let attenuation = ComputeDistanceAttenuation(light.attenuation, distance);
    return ComputeBlinnPhongLight(light.color.rgb, lightDir, viewDir, normal) * attenuation;
}

fn ComputeSpotLightBlinnPhong(light: SpotLightData, worldPos: vec3f, viewDir: vec3f, normal: vec3f) -> vec3f {
    let lightVector = light.position.xyz - worldPos;
    let distance = length(lightVector);
    if (distance <= 1.0e-4) {
        return vec3f(0.0, 0.0, 0.0);
    }

    let lightDir = lightVector / distance;
    let spotDirection = normalize(light.direction.xyz);
    let theta = dot(-lightDir, spotDirection);
    let innerCos = light.angles.x;
    let outerCos = light.angles.y;
    let angleRange = max(innerCos - outerCos, 1.0e-4);
    let spotFactor = clamp((theta - outerCos) / angleRange, 0.0, 1.0);
    if (spotFactor <= 0.0) {
        return vec3f(0.0, 0.0, 0.0);
    }

    let range = max(light.angles.z, 0.1);
    let attenuationParams = vec4f(1.0, 4.5 / range, 75.0 / (range * range), 0.0);
    let attenuation = ComputeDistanceAttenuation(attenuationParams, distance);
    return ComputeBlinnPhongLight(light.color.rgb, lightDir, viewDir, normal) * attenuation * spotFactor;
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
    let viewDir = normalize(uScene.cameraPosition.xyz - in.worldPos);

    let aoSize = max(vec2f(textureDimensions(uAmbientOcclusionTexture)), vec2f(1.0, 1.0));
    let aoUv = clamp(in.position.xy / aoSize, vec2f(0.0, 0.0), vec2f(1.0, 1.0));
    let ambientOcclusion = textureSample(uAmbientOcclusionTexture, uAmbientOcclusionSampler, aoUv).r;

    var shadowFactor = 1.0;
    if (uDirectionalShadow.shadowParams.x > 0.0) {
        shadowFactor = SampleDirectionalShadow(in.worldPos, normal);
    }

    var lighting = vec3f(0.25) * ambientOcclusion;
    if (uScene.lightCounts.x > 0u) {
        let directionalLightDir = normalize(-uScene.directionalLight.direction.xyz);
        let ambient = uScene.directionalLight.color.rgb * kAmbientStrength * ambientOcclusion;
        let direct = ComputeBlinnPhongLight(uScene.directionalLight.color.rgb, directionalLightDir, viewDir, normal);
        lighting = ambient + direct * shadowFactor;
    }

    for (var lightIndex: u32 = 0u; lightIndex < uScene.lightCounts.y; lightIndex = lightIndex + 1u) {
        lighting += ComputePointLightBlinnPhong(uScene.pointLights[lightIndex], in.worldPos, viewDir, normal);
    }

    for (var lightIndex: u32 = 0u; lightIndex < uScene.lightCounts.z; lightIndex = lightIndex + 1u) {
        lighting += ComputeSpotLightBlinnPhong(uScene.spotLights[lightIndex], in.worldPos, viewDir, normal);
    }

    return vec4f(baseColor.rgb * lighting, baseColor.a);
}
