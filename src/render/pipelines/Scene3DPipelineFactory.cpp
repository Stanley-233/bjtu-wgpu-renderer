#include "Scene3DPipelineFactory.h"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "asset/ShaderPaths.h"
#include "asset/types/AssetVertex3D.h"
#include "render/RenderContext.h"
#include "render/ShaderLoader.h"
#include "render/scene/RenderUniformData.h"

using namespace wgpu;

#ifndef WEBGPU_BACKEND_EMSCRIPTEN
static void FlushDeviceValidation(RenderContext& renderCtx) {
#ifdef WEBGPU_BACKEND_DAWN
    renderCtx.GetDevice()->tick();
#elif WEBGPU_BACKEND_WGPU
    renderCtx.GetDevice()->poll(false);
#endif
}

[[noreturn]] static void FailFastOnValidationError(const char* label, const WGPUErrorType type, const char* message) {
    std::cerr << "[" << label << "] Validation scope error: type " << type;
    if (message != nullptr) {
        std::cerr << " (" << message << ")";
    }
    std::cerr << std::endl;
    std::exit(EXIT_FAILURE);
}

static void PopValidationScope(RenderContext& renderCtx, const char* label) {
    wgpuDevicePopErrorScope(
        *renderCtx.GetDevice(),
        [](const WGPUErrorType type, const char* message, void* userdata) {
            if (type == WGPUErrorType_NoError) {
                return;
            }
            FailFastOnValidationError(static_cast<const char*>(userdata), type, message);
        },
        const_cast<char*>(label));
    FlushDeviceValidation(renderCtx);
}
#endif

static ShaderModule LoadShaderModule(
    RenderContext&               renderCtx,
    const std::filesystem::path& shaderPath,
    const char*                  label) {
    std::cout << "[" << label << "] Creating shader module..." << std::endl;
    ShaderModule shaderModule = ShaderLoader::Load(shaderPath, *renderCtx.GetDevice());
    std::cout << "[" << label << "] Shader module: " << shaderModule << std::endl;

    if (shaderModule == nullptr) {
        std::cerr << "[" << label << "] Could not load shader!" << std::endl;
        std::exit(1);
    }
    return shaderModule;
}

static void BuildMeshVertexState(
    std::array<VertexAttribute, 6>& attributes,
    VertexBufferLayout&             layout) {
    attributes[0].shaderLocation = 0;
    attributes[0].format = VertexFormat::Float32x3;
    attributes[0].offset = offsetof(AssetVertex3D, position);
    attributes[1].shaderLocation = 1;
    attributes[1].format = VertexFormat::Float32x3;
    attributes[1].offset = offsetof(AssetVertex3D, normal);
    attributes[2].shaderLocation = 2;
    attributes[2].format = VertexFormat::Float32x2;
    attributes[2].offset = offsetof(AssetVertex3D, uv0);
    attributes[3].shaderLocation = 3;
    attributes[3].format = VertexFormat::Float32x2;
    attributes[3].offset = offsetof(AssetVertex3D, uv1);
    attributes[4].shaderLocation = 4;
    attributes[4].format = VertexFormat::Float32x4;
    attributes[4].offset = offsetof(AssetVertex3D, color);
    attributes[5].shaderLocation = 5;
    attributes[5].format = VertexFormat::Float32x4;
    attributes[5].offset = offsetof(AssetVertex3D, tangent);

    layout.attributeCount = static_cast<uint32_t>(attributes.size());
    layout.attributes = attributes.data();
    layout.arrayStride = sizeof(AssetVertex3D);
    layout.stepMode = VertexStepMode::Vertex;
}

static raii::BindGroupLayout CreateSceneForwardLitBindGroupLayout(RenderContext& renderCtx) {
    std::array<BindGroupLayoutEntry, 6> bindings{};
    bindings[0].binding = 0;
    bindings[0].visibility = ShaderStage::Vertex | ShaderStage::Fragment;
    bindings[0].buffer.type = BufferBindingType::Uniform;
    bindings[0].buffer.minBindingSize = sizeof(SceneUniformData);
    bindings[1].binding = 1;
    bindings[1].visibility = ShaderStage::Vertex | ShaderStage::Fragment;
    bindings[1].buffer.type = BufferBindingType::Uniform;
    bindings[1].buffer.minBindingSize = sizeof(DirectionalShadowUniformData);
    bindings[2].binding = 2;
    bindings[2].visibility = ShaderStage::Fragment;
    bindings[2].texture.sampleType = TextureSampleType::Depth;
    bindings[2].texture.viewDimension = TextureViewDimension::_2D;
    bindings[3].binding = 3;
    bindings[3].visibility = ShaderStage::Fragment;
    bindings[3].sampler.type = SamplerBindingType::Comparison;
    bindings[4].binding = 4;
    bindings[4].visibility = ShaderStage::Fragment;
    bindings[4].texture.sampleType = TextureSampleType::Float;
    bindings[4].texture.viewDimension = TextureViewDimension::_2D;
    bindings[5].binding = 5;
    bindings[5].visibility = ShaderStage::Fragment;
    bindings[5].sampler.type = SamplerBindingType::Filtering;

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/ForwardLitSceneBindGroupLayout";
    desc.entryCount = static_cast<uint32_t>(bindings.size());
    desc.entries = bindings.data();
    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static raii::BindGroupLayout CreateSceneUniformBindGroupLayout(RenderContext& renderCtx) {
    BindGroupLayoutEntry binding{};
    binding.binding = 0;
    binding.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
    binding.buffer.type = BufferBindingType::Uniform;
    binding.buffer.minBindingSize = sizeof(SceneUniformData);

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/SceneUniformBindGroupLayout";
    desc.entryCount = 1;
    desc.entries = &binding;
    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static raii::BindGroupLayout CreateShadowSceneBindGroupLayout(RenderContext& renderCtx) {
    BindGroupLayoutEntry binding{};
    binding.binding = 0;
    binding.visibility = ShaderStage::Vertex;
    binding.buffer.type = BufferBindingType::Uniform;
    binding.buffer.minBindingSize = sizeof(DirectionalShadowUniformData);

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/ShadowSceneBindGroupLayout";
    desc.entryCount = 1;
    desc.entries = &binding;
    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static raii::BindGroupLayout CreateObjectBindGroupLayout(RenderContext& renderCtx, const uint64_t minBindingSize) {
    BindGroupLayoutEntry binding{};
    binding.binding = 0;
    binding.visibility = ShaderStage::Vertex;
    binding.buffer.type = BufferBindingType::Uniform;
    binding.buffer.minBindingSize = minBindingSize;

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/ObjectBindGroupLayout";
    desc.entryCount = 1;
    desc.entries = &binding;
    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static raii::BindGroupLayout CreateMaterialBindGroupLayout(RenderContext& renderCtx) {
    std::array<BindGroupLayoutEntry, 5> bindings{};
    bindings[0].binding = 0;
    bindings[0].visibility = ShaderStage::Fragment;
    bindings[0].buffer.type = BufferBindingType::Uniform;
    bindings[0].buffer.minBindingSize = sizeof(MaterialUniformData);
    bindings[1].binding = 1;
    bindings[1].visibility = ShaderStage::Fragment;
    bindings[1].texture.sampleType = TextureSampleType::Float;
    bindings[1].texture.viewDimension = TextureViewDimension::_2D;
    bindings[2].binding = 2;
    bindings[2].visibility = ShaderStage::Fragment;
    bindings[2].texture.sampleType = TextureSampleType::Float;
    bindings[2].texture.viewDimension = TextureViewDimension::_2D;
    bindings[3].binding = 3;
    bindings[3].visibility = ShaderStage::Fragment;
    bindings[3].texture.sampleType = TextureSampleType::Float;
    bindings[3].texture.viewDimension = TextureViewDimension::_2D;
    bindings[4].binding = 4;
    bindings[4].visibility = ShaderStage::Fragment;
    bindings[4].sampler.type = SamplerBindingType::Filtering;

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/MaterialBindGroupLayout";
    desc.entryCount = static_cast<uint32_t>(bindings.size());
    desc.entries = bindings.data();
    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static raii::BindGroupLayout CreatePbrDebugBindGroupLayout(RenderContext& renderCtx) {
    BindGroupLayoutEntry binding{};
    binding.binding = 0;
    binding.visibility = ShaderStage::Fragment;
    binding.buffer.type = BufferBindingType::Uniform;
    binding.buffer.minBindingSize = sizeof(PbrDebugUniformData);

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/PbrDebugBindGroupLayout";
    desc.entryCount = 1;
    desc.entries = &binding;
    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static raii::BindGroupLayout CreateSsaoBindGroupLayout(RenderContext& renderCtx) {
    std::array<BindGroupLayoutEntry, 3> bindings{};

    // binding 0: scene depth
    bindings[0].binding = 0;
    bindings[0].visibility = ShaderStage::Fragment;
    bindings[0].texture.sampleType = TextureSampleType::Depth;
    bindings[0].texture.viewDimension = TextureViewDimension::_2D;
    bindings[0].texture.multisampled = false;
    // binding 1: scene normal
    bindings[1].binding = 1;
    bindings[1].visibility = ShaderStage::Fragment;
    bindings[1].texture.sampleType = TextureSampleType::Float;
    bindings[1].texture.viewDimension = TextureViewDimension::_2D;
    bindings[1].texture.multisampled = false;
    // binding 2: ssao uniform
    bindings[2].binding = 2;
    bindings[2].visibility = ShaderStage::Fragment;
    bindings[2].buffer.type = BufferBindingType::Uniform;
    bindings[2].buffer.hasDynamicOffset = false;
    bindings[2].buffer.minBindingSize = sizeof(SsaoUniformData);

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/SsaoBindGroupLayout";
    desc.entryCount = static_cast<uint32_t>(bindings.size());
    desc.entries = bindings.data();

    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static raii::BindGroupLayout CreateSceneColorBindGroupLayout(RenderContext& renderCtx) {
    std::array<BindGroupLayoutEntry, 2> bindings{};
    bindings[0].binding = 0;
    bindings[0].visibility = ShaderStage::Fragment;
    bindings[0].texture.sampleType = TextureSampleType::Float;
    bindings[0].texture.viewDimension = TextureViewDimension::_2D;
    bindings[1].binding = 1;
    bindings[1].visibility = ShaderStage::Fragment;
    bindings[1].sampler.type = SamplerBindingType::Filtering;

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/CompositeSceneColorBindGroupLayout";
    desc.entryCount = static_cast<uint32_t>(bindings.size());
    desc.entries = bindings.data();
    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static raii::BindGroupLayout CreateSkyboxBindGroupLayout(RenderContext& renderCtx) {
    std::array<BindGroupLayoutEntry, 3> bindings{};
    bindings[0].binding = 0;
    bindings[0].visibility = ShaderStage::Fragment;
    bindings[0].buffer.type = BufferBindingType::Uniform;
    bindings[0].buffer.minBindingSize = sizeof(SkyboxUniformData);
    bindings[1].binding = 1;
    bindings[1].visibility = ShaderStage::Fragment;
    bindings[1].texture.sampleType = TextureSampleType::Float;
    bindings[1].texture.viewDimension = TextureViewDimension::Cube;
    bindings[2].binding = 2;
    bindings[2].visibility = ShaderStage::Fragment;
    bindings[2].sampler.type = SamplerBindingType::Filtering;

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/SkyboxBindGroupLayout";
    desc.entryCount = static_cast<uint32_t>(bindings.size());
    desc.entries = bindings.data();
    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static raii::BindGroupLayout CreateEquirectToCubemapBindGroupLayout(RenderContext& renderCtx) {
    std::array<BindGroupLayoutEntry, 3> bindings{};
    bindings[0].binding = 0;
    bindings[0].visibility = ShaderStage::Compute;
    bindings[0].texture.sampleType = TextureSampleType::Float;
    bindings[0].texture.viewDimension = TextureViewDimension::_2D;
    bindings[1].binding = 1;
    bindings[1].visibility = ShaderStage::Compute;
    bindings[1].sampler.type = SamplerBindingType::Filtering;
    bindings[2].binding = 2;
    bindings[2].visibility = ShaderStage::Compute;
    bindings[2].storageTexture.access = StorageTextureAccess::WriteOnly;
    bindings[2].storageTexture.format = TextureFormat::RGBA16Float;
    bindings[2].storageTexture.viewDimension = TextureViewDimension::_2DArray;

    BindGroupLayoutDescriptor desc{};
    desc.label = "Scene3D/EquirectToCubemapBindGroupLayout";
    desc.entryCount = static_cast<uint32_t>(bindings.size());
    desc.entries = bindings.data();
    return renderCtx.GetDevice()->createBindGroupLayout(desc);
}

static void SetCommonPrimitiveState(
    RenderPipelineDescriptor& pipelineDesc,
    const WGPUPrimitiveTopology topology,
    const WGPUCullMode          cullMode) {
    pipelineDesc.primitive.topology = topology;
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace = FrontFace::CCW;
    pipelineDesc.primitive.cullMode = cullMode;
    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;
}

static DepthStencilState BuildDepthStencilState(
    const bool                depthWriteEnabled,
    const WGPUCompareFunction depthCompare) {
    DepthStencilState depthStencil{};
    depthStencil.format = TextureFormat::Depth24Plus;
    depthStencil.depthWriteEnabled = depthWriteEnabled;
    depthStencil.depthCompare = depthCompare;
    depthStencil.stencilFront.compare = CompareFunction::Always;
    depthStencil.stencilBack.compare = CompareFunction::Always;
    depthStencil.stencilReadMask = 0;
    depthStencil.stencilWriteMask = 0;
    depthStencil.depthBias = 0;
    depthStencil.depthBiasSlopeScale = 0.0f;
    depthStencil.depthBiasClamp = 0.0f;
    return depthStencil;
}

static WGPUTextureFormat ToNativeTextureFormat(TextureFormat format) {
    return format;
}

static Scene3DPipelineFactory::ForwardPipeline CreateForwardPipeline(
    RenderContext&               renderCtx,
    const std::filesystem::path& shaderPath,
    const char*                  label,
    const TextureFormat          colorTargetFormat,
    const WGPUCullMode           cullMode,
    const bool                   usesLitSceneBindings) {
    ShaderModule shaderModule = LoadShaderModule(renderCtx, shaderPath, label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    std::array<VertexAttribute, 6> vertexAttributes{};
    VertexBufferLayout             vertexBufferLayout{};
    BuildMeshVertexState(vertexAttributes, vertexBufferLayout);

    RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    SetCommonPrimitiveState(pipelineDesc, PrimitiveTopology::TriangleList, cullMode);

    DepthStencilState depthStencil = BuildDepthStencilState(false, CompareFunction::LessEqual);
    pipelineDesc.depthStencil = &depthStencil;

    BlendState blendState{};
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;
    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;

    ColorTargetState colorTarget{};
    colorTarget.format = ToNativeTextureFormat(colorTargetFormat);
    colorTarget.blend = &blendState;
    colorTarget.writeMask = ColorWriteMask::All;

    FragmentState fragmentState{};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    Scene3DPipelineFactory::ForwardPipeline result;
    result.sceneBindGroupLayout = usesLitSceneBindings
                                      ? CreateSceneForwardLitBindGroupLayout(renderCtx)
                                      : CreateSceneUniformBindGroupLayout(renderCtx);
    result.objectBindGroupLayout = CreateObjectBindGroupLayout(renderCtx, sizeof(ObjectUniformData));
    result.materialBindGroupLayout = CreateMaterialBindGroupLayout(renderCtx);

    std::array<WGPUBindGroupLayout, 3> bindGroupLayouts{
        *result.sceneBindGroupLayout,
        *result.objectBindGroupLayout,
        *result.materialBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/ForwardPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}

Scene3DPipelineFactory::ForwardPipeline
Scene3DPipelineFactory::CreateUnlitForwardPipeline(
    RenderContext& renderCtx,
    const TextureFormat colorTargetFormat,
    const wgpu::CullMode cullMode) {
    return CreateForwardPipeline(
        renderCtx,
        ShaderPaths::Resolve("scene/scene_unlit_textured.wgsl"),
        "Scene3DPipelineFactory/ForwardUnlit",
        colorTargetFormat,
        cullMode,
        false);
}

Scene3DPipelineFactory::ForwardPipeline
Scene3DPipelineFactory::CreateLambertForwardPipeline(
    RenderContext& renderCtx,
    const TextureFormat colorTargetFormat,
    const wgpu::CullMode cullMode) {
    return CreateForwardPipeline(
        renderCtx,
        ShaderPaths::Resolve("scene/scene_lambert_textured.wgsl"),
        "Scene3DPipelineFactory/ForwardLambert",
        colorTargetFormat,
        cullMode,
        true);
}

Scene3DPipelineFactory::PbrPipeline
Scene3DPipelineFactory::CreatePbrForwardPipeline(
    RenderContext& renderCtx,
    const TextureFormat colorTargetFormat,
    const wgpu::CullMode cullMode) {
    constexpr const char* label = "Scene3DPipelineFactory/ForwardPbr";
    ShaderModule shaderModule = LoadShaderModule(renderCtx, ShaderPaths::Resolve("scene/scene_pbr_textured.wgsl"), label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    std::array<VertexAttribute, 6> vertexAttributes{};
    VertexBufferLayout             vertexBufferLayout{};
    BuildMeshVertexState(vertexAttributes, vertexBufferLayout);

    RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    SetCommonPrimitiveState(pipelineDesc, PrimitiveTopology::TriangleList, cullMode);

    DepthStencilState depthStencil = BuildDepthStencilState(false, CompareFunction::LessEqual);
    pipelineDesc.depthStencil = &depthStencil;

    BlendState blendState{};
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;
    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;

    ColorTargetState colorTarget{};
    colorTarget.format = ToNativeTextureFormat(colorTargetFormat);
    colorTarget.blend = &blendState;
    colorTarget.writeMask = ColorWriteMask::All;

    FragmentState fragmentState{};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    PbrPipeline result;
    result.sceneBindGroupLayout = CreateSceneForwardLitBindGroupLayout(renderCtx);
    result.objectBindGroupLayout = CreateObjectBindGroupLayout(renderCtx, sizeof(ObjectUniformData));
    result.materialBindGroupLayout = CreateMaterialBindGroupLayout(renderCtx);
    result.debugBindGroupLayout = CreatePbrDebugBindGroupLayout(renderCtx);

    std::array<WGPUBindGroupLayout, 4> bindGroupLayouts{
        *result.sceneBindGroupLayout,
        *result.objectBindGroupLayout,
        *result.materialBindGroupLayout,
        *result.debugBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/ForwardPbrPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}

Scene3DPipelineFactory::DepthPrepassPipeline Scene3DPipelineFactory::CreateDepthPrepassPipeline(RenderContext& renderCtx) {
    constexpr const char* label = "Scene3DPipelineFactory/DepthPrepass";
    ShaderModule shaderModule = LoadShaderModule(renderCtx, ShaderPaths::Resolve("scene/scene_depth_prepass.wgsl"), label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    std::array<VertexAttribute, 6> vertexAttributes{};
    VertexBufferLayout             vertexBufferLayout{};
    BuildMeshVertexState(vertexAttributes, vertexBufferLayout);

    RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    pipelineDesc.fragment = nullptr;
    SetCommonPrimitiveState(pipelineDesc, PrimitiveTopology::TriangleList, CullMode::None);

    DepthStencilState depthStencil = BuildDepthStencilState(true, CompareFunction::Less);
    pipelineDesc.depthStencil = &depthStencil;

    DepthPrepassPipeline result;
    result.sceneBindGroupLayout = CreateSceneUniformBindGroupLayout(renderCtx);
    result.objectBindGroupLayout = CreateObjectBindGroupLayout(renderCtx, sizeof(ObjectUniformData));

    std::array<WGPUBindGroupLayout, 2> bindGroupLayouts{
        *result.sceneBindGroupLayout,
        *result.objectBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/DepthPrepassPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}

Scene3DPipelineFactory::SceneNormalPipeline
Scene3DPipelineFactory::CreateSceneNormalPipeline(RenderContext& renderCtx, const TextureFormat colorTargetFormat) {
    constexpr const char* label = "Scene3DPipelineFactory/SceneNormal";
    ShaderModule shaderModule = LoadShaderModule(renderCtx, ShaderPaths::Resolve("scene/scene_normal_prepass.wgsl"), label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    std::array<VertexAttribute, 6> vertexAttributes{};
    VertexBufferLayout             vertexBufferLayout{};
    BuildMeshVertexState(vertexAttributes, vertexBufferLayout);

    RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    SetCommonPrimitiveState(pipelineDesc, PrimitiveTopology::TriangleList, CullMode::None);

    DepthStencilState depthStencil = BuildDepthStencilState(false, CompareFunction::LessEqual);
    pipelineDesc.depthStencil = &depthStencil;

    ColorTargetState colorTarget{};
    colorTarget.format = ToNativeTextureFormat(colorTargetFormat);
    colorTarget.blend = nullptr;
    colorTarget.writeMask = ColorWriteMask::All;

    FragmentState fragmentState{};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    SceneNormalPipeline result;
    result.sceneBindGroupLayout = CreateSceneUniformBindGroupLayout(renderCtx);
    result.objectBindGroupLayout = CreateObjectBindGroupLayout(renderCtx, sizeof(ObjectUniformData));

    std::array<WGPUBindGroupLayout, 2> bindGroupLayouts{
        *result.sceneBindGroupLayout,
        *result.objectBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/SceneNormalPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}

Scene3DPipelineFactory::SsaoPipeline Scene3DPipelineFactory::CreateSsaoPipeline(RenderContext& renderCtx) {
    constexpr const char* label = "Scene3DPipelineFactory/SSAO";
    ShaderModule shaderModule = LoadShaderModule(renderCtx, ShaderPaths::Resolve("scene/scene_ssao.wgsl"), label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    SetCommonPrimitiveState(pipelineDesc, PrimitiveTopology::TriangleList, CullMode::None);
    pipelineDesc.depthStencil = nullptr;

    ColorTargetState colorTarget{};
    colorTarget.format = TextureFormat::R8Unorm;
    colorTarget.blend = nullptr;
    colorTarget.writeMask = ColorWriteMask::All;

    FragmentState fragmentState{};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    SsaoPipeline result;
    result.ssaoBindGroupLayout = CreateSsaoBindGroupLayout(renderCtx);
    std::array<WGPUBindGroupLayout, 1> bindGroupLayouts{
        *result.ssaoBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/SsaoPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}

Scene3DPipelineFactory::CompositePipeline
Scene3DPipelineFactory::CreateCompositePipeline(RenderContext& renderCtx, const TextureFormat colorTargetFormat) {
    constexpr const char* label = "Scene3DPipelineFactory/Composite";
    ShaderModule shaderModule = LoadShaderModule(renderCtx, ShaderPaths::Resolve("scene/scene_composite.wgsl"), label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    SetCommonPrimitiveState(pipelineDesc, PrimitiveTopology::TriangleList, CullMode::None);
    pipelineDesc.depthStencil = nullptr;

    ColorTargetState colorTarget{};
    colorTarget.format = ToNativeTextureFormat(colorTargetFormat);
    colorTarget.blend = nullptr;
    colorTarget.writeMask = ColorWriteMask::All;

    FragmentState fragmentState{};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    CompositePipeline result;
    result.sceneColorBindGroupLayout = CreateSceneColorBindGroupLayout(renderCtx);

    std::array<WGPUBindGroupLayout, 1> bindGroupLayouts{
        *result.sceneColorBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/CompositePipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}

Scene3DPipelineFactory::ToneMapPipeline
Scene3DPipelineFactory::CreateToneMapPipeline(RenderContext& renderCtx, const TextureFormat colorTargetFormat) {
    constexpr const char* label = "Scene3DPipelineFactory/ToneMap";
    ShaderModule shaderModule = LoadShaderModule(renderCtx, ShaderPaths::Resolve("scene/scene_tone_map.wgsl"), label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    SetCommonPrimitiveState(pipelineDesc, PrimitiveTopology::TriangleList, CullMode::None);
    pipelineDesc.depthStencil = nullptr;

    ColorTargetState colorTarget{};
    colorTarget.format = ToNativeTextureFormat(colorTargetFormat);
    colorTarget.blend = nullptr;
    colorTarget.writeMask = ColorWriteMask::All;

    FragmentState fragmentState{};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    ToneMapPipeline result;
    result.sceneColorBindGroupLayout = CreateSceneColorBindGroupLayout(renderCtx);

    std::array<WGPUBindGroupLayout, 1> bindGroupLayouts{
        *result.sceneColorBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/ToneMapPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}

Scene3DPipelineFactory::SkyboxPipeline
Scene3DPipelineFactory::CreateSkyboxPipeline(RenderContext& renderCtx, const TextureFormat colorTargetFormat) {
    constexpr const char* label = "Scene3DPipelineFactory/Skybox";
    ShaderModule shaderModule = LoadShaderModule(renderCtx, ShaderPaths::Resolve("scene/scene_skybox.wgsl"), label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    SetCommonPrimitiveState(pipelineDesc, PrimitiveTopology::TriangleList, CullMode::None);
    pipelineDesc.depthStencil = nullptr;

    ColorTargetState colorTarget{};
    colorTarget.format = ToNativeTextureFormat(colorTargetFormat);
    colorTarget.blend = nullptr;
    colorTarget.writeMask = ColorWriteMask::All;

    FragmentState fragmentState{};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    SkyboxPipeline result;
    result.skyboxBindGroupLayout = CreateSkyboxBindGroupLayout(renderCtx);

    std::array<WGPUBindGroupLayout, 1> bindGroupLayouts{
        *result.skyboxBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/SkyboxPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}

Scene3DPipelineFactory::EquirectToCubemapComputePipeline
Scene3DPipelineFactory::CreateEquirectToCubemapComputePipeline(RenderContext& renderCtx) {
    constexpr const char* label = "Scene3DPipelineFactory/EquirectToCubemap";
    ShaderModule shaderModule =
        LoadShaderModule(renderCtx, ShaderPaths::Resolve("compute/equirect_to_cubemap.wgsl"), label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    EquirectToCubemapComputePipeline result;
    result.bindGroupLayout = CreateEquirectToCubemapBindGroupLayout(renderCtx);

    std::array<WGPUBindGroupLayout, 1> bindGroupLayouts{
        *result.bindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/EquirectToCubemapPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    ComputePipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.layout = *result.layout;
    pipelineDesc.compute.module = shaderModule;
    pipelineDesc.compute.entryPoint = "cs_main";
    pipelineDesc.compute.constantCount = 0;
    pipelineDesc.compute.constants = nullptr;
    result.pipeline = renderCtx.GetDevice()->createComputePipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}

Scene3DPipelineFactory::ShadowPipeline Scene3DPipelineFactory::CreateDirectionalShadowPipeline(RenderContext& renderCtx) {
    constexpr const char* label = "Scene3DPipelineFactory/DirectionalShadow";
    ShaderModule shaderModule =
        LoadShaderModule(renderCtx, ShaderPaths::Resolve("scene/scene_directional_shadow.wgsl"), label);

    wgpuDevicePushErrorScope(*renderCtx.GetDevice(), WGPUErrorFilter_Validation);

    std::array<VertexAttribute, 6> vertexAttributes{};
    VertexBufferLayout             vertexBufferLayout{};
    BuildMeshVertexState(vertexAttributes, vertexBufferLayout);

    RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.label = label;
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    pipelineDesc.fragment = nullptr;
    SetCommonPrimitiveState(pipelineDesc, PrimitiveTopology::TriangleList, CullMode::None);

    DepthStencilState depthStencil = BuildDepthStencilState(true, CompareFunction::Less);
    depthStencil.depthBias = 2;
    depthStencil.depthBiasSlopeScale = 2.0f;
    pipelineDesc.depthStencil = &depthStencil;

    ShadowPipeline result;
    result.sceneBindGroupLayout = CreateShadowSceneBindGroupLayout(renderCtx);
    result.objectBindGroupLayout = CreateObjectBindGroupLayout(renderCtx, sizeof(ShadowObjectUniformData));

    std::array<WGPUBindGroupLayout, 2> bindGroupLayouts{
        *result.sceneBindGroupLayout,
        *result.objectBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/ShadowPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    result.layout = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(renderCtx, label);
#endif
    return result;
}
