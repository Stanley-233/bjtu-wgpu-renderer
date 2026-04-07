#ifndef BJTU_WGPU_RENDERER_RESOURCEMANAGER_H
#define BJTU_WGPU_RENDERER_RESOURCEMANAGER_H

#include <vector>
#include <filesystem>
#include <webgpu/webgpu.hpp>

struct MeshData3D;
struct SceneDescription;

class ResourceManager {
public:
    /**
     * Load a file from `path` using our ad-hoc format and populate the `pointData`
     * and `indexData` vectors.
     */
    static bool LoadGeometry(
        const std::filesystem::path& path,
        std::vector<float>&          pointData,
        std::vector<uint16_t>&       indexData
    );

    static bool LoadGeometry3DFromObj(
        const std::filesystem::path& path,
        MeshData3D&                  outMesh
    );

    static bool LoadSceneFromToml(
        const std::filesystem::path& path,
        SceneDescription&            outScene
    );

    /**
     * Create a shader module for a given WebGPU `device` from a WGSL shader source
     * loaded from file `path`.
     */
    static wgpu::ShaderModule LoadShaderModule(
        const std::filesystem::path& path,
        wgpu::Device                 device
    );
};


#endif //BJTU_WGPU_RENDERER_RESOURCEMANAGER_H
