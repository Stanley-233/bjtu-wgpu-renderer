const PI: f32 = 3.14159265358979323846;

@group(0) @binding(0) var uEquirectTexture: texture_2d<f32>;
@group(0) @binding(1) var uEquirectSampler: sampler;
@group(0) @binding(2) var uCubemapTexture: texture_storage_2d_array<rgba16float, write>;

// 把某个 cubemap 面上的 [-1, 1]x[-1, 1] UV 还原成对应的方向向量。
// 面顺序固定为 +X, -X, +Y, -Y, +Z, -Z。
fn CubeFaceDirection(faceIndex: u32, uv: vec2f) -> vec3f {
    switch (faceIndex) {
        case 0u: {
            return normalize(vec3f(1.0, -uv.y, -uv.x));
        }
        case 1u: {
            return normalize(vec3f(-1.0, -uv.y, uv.x));
        }
        case 2u: {
            return normalize(vec3f(uv.x, 1.0, uv.y));
        }
        case 3u: {
            return normalize(vec3f(uv.x, -1.0, -uv.y));
        }
        case 4u: {
            return normalize(vec3f(uv.x, -uv.y, 1.0));
        }
        default: {
            return normalize(vec3f(-uv.x, -uv.y, -1.0));
        }
    }
}

// 世界方向转回经纬展开图 UV。
fn DirectionToEquirectUv(direction: vec3f) -> vec2f {
    let longitude = atan2(direction.z, direction.x);
    let latitude = acos(clamp(direction.y, -1.0, 1.0));
    return vec2f(
        longitude / (2.0 * PI) + 0.5,
        latitude / PI,
    );
}

@compute @workgroup_size(8, 8, 1)
fn cs_main(
    @builtin(global_invocation_id) globalInvocationId: vec3u,
) {
    let faceSize = textureDimensions(uCubemapTexture).xy;
    if (globalInvocationId.x >= faceSize.x || globalInvocationId.y >= faceSize.y || globalInvocationId.z >= 6u) {
        return;
    }

    // 以像素中心采样，避免边缘出现半像素偏移。
    let uv = ((vec2f(globalInvocationId.xy) + vec2f(0.5, 0.5)) / vec2f(faceSize)) * 2.0 - vec2f(1.0, 1.0);
    let direction = CubeFaceDirection(globalInvocationId.z, uv);
    let equirectUv = DirectionToEquirectUv(direction);
    let color = textureSampleLevel(uEquirectTexture, uEquirectSampler, equirectUv, 0.0);
    // 每个 invocation 负责写 cubemap 的一个 texel。
    textureStore(uCubemapTexture, vec2i(globalInvocationId.xy), i32(globalInvocationId.z), color);
}
