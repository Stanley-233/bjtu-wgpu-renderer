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

struct FragmentInput {
    @builtin(position) position: vec4f,
    @builtin(front_facing) frontFacing: bool,
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

const PI: f32 = 3.14159265;

fn SelectUv(texCoordSet: u32, uv0: vec2f, uv1: vec2f) -> vec2f {
    return select(uv0, uv1, texCoordSet == 1u);
}

fn Saturate(value: f32) -> f32 {
    return clamp(value, 0.0, 1.0);
}

fn ResolveFaceSign(frontFacing: bool) -> f32 {
    return select(-1.0, 1.0, frontFacing);
}

fn SampleDirectionalShadow(worldPos: vec3f, normal: vec3f) -> f32 {
    let lightSpace = uDirectionalShadow.lightViewProjection * vec4f(worldPos, 1.0);
    if (lightSpace.w <= 0.0) {
        return 1.0;
    }

    let projected = lightSpace.xyz / lightSpace.w;
    if (projected.x < -1.0 || projected.x > 1.0 ||
        projected.y < -1.0 || projected.y > 1.0 ||
        projected.z < 0.0 || projected.z > 1.0) {
        return 1.0;
    }

    let shadowUv = vec2f(projected.x * 0.5 + 0.5, 0.5 - projected.y * 0.5);
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
    return visibility / 9.0;
}

fn SampleBaseColor(in: FragmentInput) -> vec4f {
    let baseColorUv = SelectUv(uMaterial.textureCoordSets.x, in.uv0, in.uv1);
    let sampled = textureSample(uBaseColorTexture, uMaterialSampler, baseColorUv);
    var baseColor = sampled * uMaterial.baseColorFactor;
    if (uMaterial.surfaceOptions.y != 0u) {
        baseColor *= in.color;
    }
    return baseColor;
}

fn SampleMetallicRoughness(in: FragmentInput) -> vec2f {
    let mrUv = SelectUv(uMaterial.textureCoordSets.z, in.uv0, in.uv1);
    let mrSample = textureSample(uMetallicRoughnessTexture, uMaterialSampler, mrUv);
    // glTF 金属粗糙度约定：
    // roughness = texture.g * roughnessFactor
    // metallic  = texture.b * metallicFactor
    let metallic = Saturate(mrSample.b * uMaterial.pbrParams.x);
    let roughness = clamp(mrSample.g * uMaterial.pbrParams.y, 0.045, 1.0);
    return vec2f(metallic, roughness);
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

fn ResolveGeometricNormal(in: FragmentInput) -> vec3f {
    let faceSign = ResolveFaceSign(in.frontFacing);
    let isDoubleSided = uMaterial.surfaceOptions.w != 0u;
    return normalize(select(in.normalWS, in.normalWS * faceSign, isDoubleSided));
}

fn SampleNormalWS(in: FragmentInput, geometricNormal: vec3f) -> vec3f {
    if (uMaterial.surfaceOptions.z == 0u) {
        return geometricNormal;
    }

    let normalUv = SelectUv(uMaterial.textureCoordSets.y, in.uv0, in.uv1);
    let sampledNormal = textureSample(uNormalTexture, uMaterialSampler, normalUv).xyz * 2.0 - vec3f(1.0, 1.0, 1.0);
    let normalScale = select(uMaterial.pbrParams.z, 1.0, uMaterial.pbrParams.z <= 0.0);
    let scaledXY = sampledNormal.xy * normalScale;
    let normalTS = normalize(vec3f(
        scaledXY.x,
        scaledXY.y,
        max(sampledNormal.z, 1.0e-4),
    ));
    var tbn = BuildCotangentFrame(geometricNormal, in.worldPos, normalUv);
    if (length(in.tangentWS.xyz) > 1.0e-5) {
        let faceSign = ResolveFaceSign(in.frontFacing);
        let tangentHandedness = select(in.tangentWS.w, in.tangentWS.w * faceSign, uMaterial.surfaceOptions.w != 0u);
        let tangentWS = normalize(in.tangentWS.xyz);
        let bitangentWS = normalize(cross(geometricNormal, tangentWS) * tangentHandedness);
        tbn = mat3x3f(tangentWS, bitangentWS, geometricNormal);
    }
    return normalize(tbn * normalTS);
}

fn DistributionGGX(normal: vec3f, halfVector: vec3f, roughness: f32) -> f32 {
    // D：GGX / Trowbridge-Reitz 法线分布函数
    // 原公式：
    // D = a^2 / (PI * (NdotH^2 * (a^2 - 1) + 1)^2)
    // 其中：
    // a = roughness^2
    // NdotH = saturate(dot(N, H))
    // 含义：
    // D 描述微表面法线与半角向量 H 的对齐程度，表面越光滑，D 的峰值越尖，高光越集中

    // 防止 roughness = 0，alpha2 = 0，高光会退化得很极端
    let r = clamp(roughness, 0.045, 1.0);
    let alpha = r * r;
    let alpha2 = alpha * alpha;
    let ndoth = Saturate(dot(normal, halfVector));
    let ndoth2 = ndoth * ndoth;
    let denom = ndoth2 * (alpha2 - 1.0) + 1.0;
    return alpha2 / max(PI * denom * denom, 1.0e-7);
}

fn Pow5(v: f32) -> f32 {
    return v * v * v * v * v;
}

fn FresnelSchlick(cosTheta: f32, f0: vec3f) -> vec3f {
    // F：Schlick Fresnel 菲涅尔近似
    // 原公式：
    // F = F0 + (1 - F0) * (1 - cosTheta)^5
    // 其中：
    // cosTheta = saturate(dot(H, V))
    // F0 是垂直入射时的基础反射率
    // 视线越接近掠射角，反射越强
    let oneMinusCos = 1.0 - Saturate(cosTheta);
    return f0 + (vec3f(1.0, 1.0, 1.0) - f0) * Pow5(oneMinusCos);
}

fn GeometrySchlickGGX(ndotX: f32, roughness: f32) -> f32 {
    // G1：Schlick-GGX 单边几何项
    // 原公式：
    // G1(X) = NdotX / (NdotX * (1 - k) + k)
    // k = (roughness + 1)^2 / 8
    // 其中，X 可以是 V（视线方向）或 L（光线方向）
    // 用于描述近似微表面的自遮挡与自掩蔽
    let r = roughness + 1.0;
    let k = (r * r) / 8.0;
    return ndotX / max(ndotX * (1.0 - k) + k, 1.0e-4);
}

fn GeometrySmith(normal: vec3f, viewDir: vec3f, lightDir: vec3f, roughness: f32) -> f32 {
    // G：Smith 几何项
    // 数学公式：
    // G = G1(V) * G1(L)
    // 其中：
    // G1(V) = GeometrySchlickGGX(NdotV, roughness)
    // G1(L) = GeometrySchlickGGX(NdotL, roughness)

    // 这里直接使用 normal map 扰动后的 N
    // 因此 normal map 会影响 NdotV / NdotL，也就会影响 G 项
    let ndotv = Saturate(dot(normal, viewDir));
    let ndotl = Saturate(dot(normal, lightDir));
    let ggxV = GeometrySchlickGGX(ndotv, roughness);
    let ggxL = GeometrySchlickGGX(ndotl, roughness);
    return ggxV * ggxL;
}

@fragment
fn fs_main(in: FragmentInput) -> @location(0) vec4f {
    let baseColor = SampleBaseColor(in);
    let metallicRoughness = SampleMetallicRoughness(in);
    let metallic = metallicRoughness.x;
    let roughness = metallicRoughness.y;
    let geometricNormal = ResolveGeometricNormal(in);
    let normal = SampleNormalWS(in, geometricNormal);
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

    let viewDir = normalize(uScene.cameraPosition.xyz - in.worldPos);

    // F0 混合规则：
    // F0 = mix(vec3(0.04), baseColor.rgb, metallic)
    // 非金属通常取 0.04，金属的镜面反射颜色来自 baseColor
    let f0 = mix(vec3f(0.04, 0.04, 0.04), baseColor.rgb, vec3f(metallic, metallic, metallic));

    // TODO: 接入 IBL 后，这里应替换为更合理的环境漫反射/环境镜面反射模型
    // 当前先故意提高 ambient 来补偿缺失的间接光，因此这里不严格满足能量守恒
    // ambient ~= constant * baseColor * AO
    let ambient = vec3f(0.2, 0.2, 0.2) * baseColor.rgb * ambientOcclusion;
    var lighting = ambient;
    if (uScene.lightCounts.x > 0u) {
        let lightDir = normalize(-uScene.directionalLight.direction.xyz);
        let halfVector = normalize(viewDir + lightDir);

        let ndotv = Saturate(dot(normal, viewDir));
        let ndotl = Saturate(dot(normal, lightDir));
        let hdotv = Saturate(dot(halfVector, viewDir));

        if (ndotv > 0.0 && ndotl > 0.0) {
            let d = DistributionGGX(normal, halfVector, roughness);
            let f = FresnelSchlick(hdotv, f0);
            let g = GeometrySmith(normal, viewDir, lightDir, roughness);

            // Cook-Torrance 镜面项：
            // specular = (D * F * G) / (4 * NdotV * NdotL)
            let specularNumerator = d * g * f;
            let specularDenominator = max(4.0 * ndotv * ndotl, 1.0e-4);
            let specular = specularNumerator / specularDenominator;

            // 能量守恒：
            // kS = F
            // kD = (1 - kS) * (1 - metallic)
            // diffuse = kD * baseColor / PI
            // 镜面反射越强，漫反射可分配的能量越少；金属表面通常没有传统意义上的漫反射，因此乘上 (1 - metallic)
            let kS = f;
            let kD = (vec3f(1.0, 1.0, 1.0) - kS) * (1.0 - metallic);
            let diffuse = kD * baseColor.rgb / PI;

            let radiance = uScene.directionalLight.color.rgb;
            var shadowFactor = 1.0;
            if (uDirectionalShadow.shadowParams.x > 0.0) {
                shadowFactor = SampleDirectionalShadow(in.worldPos, geometricNormal);
            }
            // 直接光照：
            // Lo = (diffuse + specular) * radiance * NdotL
            let direct = (diffuse + specular) * radiance * ndotl * shadowFactor;
            lighting += direct;
        }
    }
    return vec4f(lighting, baseColor.a);
}
