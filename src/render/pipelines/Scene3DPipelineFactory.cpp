#include "Scene3DPipelineFactory.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <iostream>

#include "asset/ShaderPaths.h"
#include "asset/types/AssetVertex3D.h"
#include "render/RenderContext.h"
#include "render/ShaderLoader.h"

using namespace wgpu;

namespace {
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
void FlushDeviceValidation(RenderContext& ctx) {
#ifdef WEBGPU_BACKEND_DAWN
    ctx.GetDevice()->tick();
#elif WEBGPU_BACKEND_WGPU
    ctx.GetDevice()->poll(false);
#endif
}

void PopValidationScope(RenderContext& ctx, const char* label) {
    wgpuDevicePopErrorScope(
        *ctx.GetDevice(),
        [](const WGPUErrorType type, const char* message, void* userdata) {
            if (type == WGPUErrorType_NoError) {
                return;
            }
            std::cerr << "[" << static_cast<const char*>(userdata) << "] Validation scope error: type " << type;
            if (message != nullptr) {
                std::cerr << " (" << message << ")";
            }
            std::cerr << std::endl;
        },
        const_cast<char*>(label));
    FlushDeviceValidation(ctx);
}
#endif
} // namespace

static VertexBufferLayout CreateSceneVertexBufferLayout(std::array<VertexAttribute, 4>& vertexAttribs) {
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format         = VertexFormat::Float32x3;
    vertexAttribs[0].offset         = offsetof(AssetVertex3D, position);
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format         = VertexFormat::Float32x3;
    vertexAttribs[1].offset         = offsetof(AssetVertex3D, normal);
    vertexAttribs[2].shaderLocation = 2;
    vertexAttribs[2].format         = VertexFormat::Float32x2;
    vertexAttribs[2].offset         = offsetof(AssetVertex3D, uv0);
    vertexAttribs[3].shaderLocation = 3;
    vertexAttribs[3].format         = VertexFormat::Float32x4;
    vertexAttribs[3].offset         = offsetof(AssetVertex3D, color);

    VertexBufferLayout vertexBufferLayout{};
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes     = vertexAttribs.data();
    vertexBufferLayout.arrayStride    = sizeof(AssetVertex3D);
    vertexBufferLayout.stepMode       = VertexStepMode::Vertex;
    return vertexBufferLayout;
}

static Scene3DPipelineFactory::Pipeline CreatePipeline(
    RenderContext&                ctx,
    const std::filesystem::path&  shaderPath,
    const char*                   label,
    const bool                    hasFragmentStage,
    const WGPUPrimitiveTopology   topology,
    const WGPUCullMode            cullMode,
    const bool                    depthWriteEnabled,
    const WGPUCompareFunction     depthCompare,
    const WGPUColorWriteMaskFlags colorWriteMask) {
    std::cout << "[" << label << "] Creating shader module..." << std::endl;
    ShaderModule shaderModule = ShaderLoader::Load(shaderPath, *ctx.GetDevice());
    std::cout << "[" << label << "] Shader module: " << shaderModule << std::endl;

    if (shaderModule == nullptr) {
        std::cerr << "[" << label << "] Could not load shader!" << std::endl;
        std::exit(1);
    }

    wgpuDevicePushErrorScope(*ctx.GetDevice(), WGPUErrorFilter_Validation);

    RenderPipelineDescriptor      pipelineDesc{};
    pipelineDesc.label = label;
    std::array<VertexAttribute, 4> vertexAttribs{};
    const VertexBufferLayout       vertexBufferLayout = CreateSceneVertexBufferLayout(vertexAttribs);

    pipelineDesc.vertex.bufferCount   = 1;
    pipelineDesc.vertex.buffers       = &vertexBufferLayout;
    pipelineDesc.vertex.module        = shaderModule;
    pipelineDesc.vertex.entryPoint    = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants     = nullptr;

    pipelineDesc.primitive.topology         = topology;
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace        = FrontFace::CCW;
    pipelineDesc.primitive.cullMode         = cullMode;

    FragmentState    fragmentState{};
    BlendState       blendState{};
    ColorTargetState colorTarget{};
    if (hasFragmentStage) {
        fragmentState.module        = shaderModule;
        fragmentState.entryPoint    = "fs_main";
        fragmentState.constantCount = 0;
        fragmentState.constants     = nullptr;

        blendState.color.srcFactor = BlendFactor::SrcAlpha;
        blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
        blendState.color.operation = BlendOperation::Add;
        blendState.alpha.srcFactor = BlendFactor::Zero;
        blendState.alpha.dstFactor = BlendFactor::One;
        blendState.alpha.operation = BlendOperation::Add;

        colorTarget.format    = ctx.GetSurfaceFormat();
        colorTarget.blend     = &blendState;
        colorTarget.writeMask = colorWriteMask;

        fragmentState.targetCount = 1;
        fragmentState.targets     = &colorTarget;
        pipelineDesc.fragment     = &fragmentState;
    } else {
        pipelineDesc.fragment = nullptr;
    }

    DepthStencilState depthStencil{};
    depthStencil.format                             = TextureFormat::Depth24Plus;
    depthStencil.depthWriteEnabled                  = depthWriteEnabled;
    depthStencil.depthCompare                       = depthCompare;
    depthStencil.stencilFront.compare               = CompareFunction::Always;
    depthStencil.stencilBack.compare                = CompareFunction::Always;
    depthStencil.stencilReadMask                    = 0;
    depthStencil.stencilWriteMask                   = 0;
    depthStencil.depthBias                          = 0;
    depthStencil.depthBiasSlopeScale                = 0.0f;
    depthStencil.depthBiasClamp                     = 0.0f;
    pipelineDesc.depthStencil                       = &depthStencil;
    pipelineDesc.multisample.count                  = 1;
    pipelineDesc.multisample.mask                   = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    Scene3DPipelineFactory::Pipeline result;
    {
        std::array<BindGroupLayoutEntry, 4> sceneBindings{};
        sceneBindings[0].binding = 0;
        sceneBindings[0].visibility = ShaderStage::Vertex | ShaderStage::Fragment;
        sceneBindings[0].buffer.type = BufferBindingType::Uniform;
        sceneBindings[1].binding = 1;
        sceneBindings[1].visibility = ShaderStage::Vertex | ShaderStage::Fragment;
        sceneBindings[1].buffer.type = BufferBindingType::Uniform;
        sceneBindings[2].binding = 2;
        sceneBindings[2].visibility = ShaderStage::Fragment;
        sceneBindings[2].texture.sampleType = TextureSampleType::Depth;
        sceneBindings[2].texture.viewDimension = TextureViewDimension::_2D;
        sceneBindings[3].binding = 3;
        sceneBindings[3].visibility = ShaderStage::Fragment;
        sceneBindings[3].sampler.type = SamplerBindingType::Comparison;

        BindGroupLayoutDescriptor sceneBindGroupLayoutDesc{};
        sceneBindGroupLayoutDesc.label = "Scene3D/SceneBindGroupLayout";
        sceneBindGroupLayoutDesc.entryCount = static_cast<uint32_t>(sceneBindings.size());
        sceneBindGroupLayoutDesc.entries = sceneBindings.data();
        result.sceneBindGroupLayout = ctx.GetDevice()->createBindGroupLayout(sceneBindGroupLayoutDesc);
    }
    {
        BindGroupLayoutEntry objectBinding{};
        objectBinding.binding = 0;
        objectBinding.visibility = ShaderStage::Vertex;
        objectBinding.buffer.type = BufferBindingType::Uniform;

        BindGroupLayoutDescriptor objectBindGroupLayoutDesc{};
        objectBindGroupLayoutDesc.label = "Scene3D/ObjectBindGroupLayout";
        objectBindGroupLayoutDesc.entryCount = 1;
        objectBindGroupLayoutDesc.entries = &objectBinding;
        result.objectBindGroupLayout = ctx.GetDevice()->createBindGroupLayout(objectBindGroupLayoutDesc);
    }
    {
        std::array<BindGroupLayoutEntry, 3> materialBindings{};
        materialBindings[0].binding = 0;
        materialBindings[0].visibility = ShaderStage::Fragment;
        materialBindings[0].buffer.type = BufferBindingType::Uniform;
        materialBindings[1].binding = 1;
        materialBindings[1].visibility = ShaderStage::Fragment;
        materialBindings[1].texture.sampleType = TextureSampleType::Float;
        materialBindings[1].texture.viewDimension = TextureViewDimension::_2D;
        materialBindings[2].binding = 2;
        materialBindings[2].visibility = ShaderStage::Fragment;
        materialBindings[2].sampler.type = SamplerBindingType::Filtering;

        BindGroupLayoutDescriptor materialBindGroupLayoutDesc{};
        materialBindGroupLayoutDesc.label = "Scene3D/MaterialBindGroupLayout";
        materialBindGroupLayoutDesc.entryCount = static_cast<uint32_t>(materialBindings.size());
        materialBindGroupLayoutDesc.entries = materialBindings.data();
        result.materialBindGroupLayout = ctx.GetDevice()->createBindGroupLayout(materialBindGroupLayoutDesc);
    }

    std::array<WGPUBindGroupLayout, 3> bindGroupLayouts{
        *result.sceneBindGroupLayout,
        *result.objectBindGroupLayout,
        *result.materialBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/PipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts     = bindGroupLayouts.data();
    result.layout                   = ctx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline     = ctx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(ctx, label);
#endif
    return result;
}

static Scene3DPipelineFactory::ShadowPipeline CreateShadowPipeline(
    RenderContext&               ctx,
    const std::filesystem::path& shaderPath,
    const char*                  label,
    const WGPUPrimitiveTopology  topology,
    const WGPUCullMode           cullMode) {
    std::cout << "[" << label << "] Creating shader module..." << std::endl;
    ShaderModule shaderModule = ShaderLoader::Load(shaderPath, *ctx.GetDevice());
    std::cout << "[" << label << "] Shader module: " << shaderModule << std::endl;

    if (shaderModule == nullptr) {
        std::cerr << "[" << label << "] Could not load shader!" << std::endl;
        std::exit(1);
    }

    wgpuDevicePushErrorScope(*ctx.GetDevice(), WGPUErrorFilter_Validation);

    RenderPipelineDescriptor      pipelineDesc{};
    pipelineDesc.label = label;
    std::array<VertexAttribute, 4> vertexAttribs{};
    const VertexBufferLayout       vertexBufferLayout = CreateSceneVertexBufferLayout(vertexAttribs);
    pipelineDesc.vertex.bufferCount   = 1;
    pipelineDesc.vertex.buffers       = &vertexBufferLayout;
    pipelineDesc.vertex.module        = shaderModule;
    pipelineDesc.vertex.entryPoint    = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants     = nullptr;
    pipelineDesc.fragment             = nullptr;

    pipelineDesc.primitive.topology         = topology;
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace        = FrontFace::CCW;
    pipelineDesc.primitive.cullMode         = cullMode;

    DepthStencilState depthStencil{};
    depthStencil.format                             = TextureFormat::Depth24Plus;
    depthStencil.depthWriteEnabled                  = true;
    depthStencil.depthCompare                       = CompareFunction::Less;
    depthStencil.stencilFront.compare               = CompareFunction::Always;
    depthStencil.stencilBack.compare                = CompareFunction::Always;
    depthStencil.stencilReadMask                    = 0;
    depthStencil.stencilWriteMask                   = 0;
    depthStencil.depthBias                          = 0;
    depthStencil.depthBiasSlopeScale                = 0.0f;
    depthStencil.depthBiasClamp                     = 0.0f;
    pipelineDesc.depthStencil                       = &depthStencil;
    pipelineDesc.multisample.count                  = 1;
    pipelineDesc.multisample.mask                   = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    Scene3DPipelineFactory::ShadowPipeline result;
    {
        BindGroupLayoutEntry sceneBinding{};
        sceneBinding.binding = 0;
        sceneBinding.visibility = ShaderStage::Vertex;
        sceneBinding.buffer.type = BufferBindingType::Uniform;

        BindGroupLayoutDescriptor sceneBindGroupLayoutDesc{};
        sceneBindGroupLayoutDesc.label = "Scene3D/ShadowSceneBindGroupLayout";
        sceneBindGroupLayoutDesc.entryCount = 1;
        sceneBindGroupLayoutDesc.entries = &sceneBinding;
        result.sceneBindGroupLayout = ctx.GetDevice()->createBindGroupLayout(sceneBindGroupLayoutDesc);
    }
    {
        BindGroupLayoutEntry objectBinding{};
        objectBinding.binding = 0;
        objectBinding.visibility = ShaderStage::Vertex;
        objectBinding.buffer.type = BufferBindingType::Uniform;

        BindGroupLayoutDescriptor objectBindGroupLayoutDesc{};
        objectBindGroupLayoutDesc.label = "Scene3D/ShadowObjectBindGroupLayout";
        objectBindGroupLayoutDesc.entryCount = 1;
        objectBindGroupLayoutDesc.entries = &objectBinding;
        result.objectBindGroupLayout = ctx.GetDevice()->createBindGroupLayout(objectBindGroupLayoutDesc);
    }

    std::array<WGPUBindGroupLayout, 2> bindGroupLayouts{
        *result.sceneBindGroupLayout,
        *result.objectBindGroupLayout,
    };

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.label = "Scene3D/ShadowPipelineLayout";
    layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    layoutDesc.bindGroupLayouts     = bindGroupLayouts.data();
    result.layout                   = ctx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline     = ctx.GetDevice()->createRenderPipeline(pipelineDesc);
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
    PopValidationScope(ctx, label);
#endif
    return result;
}

Scene3DPipelineFactory::Pipeline Scene3DPipelineFactory::CreateUnlitForwardPipeline(RenderContext& ctx) {
    return CreatePipeline(
        ctx,
        ShaderPaths::Resolve("scene/scene_unlit_textured.wgsl"),
        "Scene3DPipelineFactory/ForwardUnlit",
        true,
        PrimitiveTopology::TriangleList,
        CullMode::None,
        true,
        CompareFunction::Less,
        ColorWriteMask::All);
}

Scene3DPipelineFactory::Pipeline Scene3DPipelineFactory::CreateLambertForwardPipeline(RenderContext& ctx) {
    return CreatePipeline(
        ctx,
        ShaderPaths::Resolve("scene/scene_lambert_textured.wgsl"),
        "Scene3DPipelineFactory/ForwardLambert",
        true,
        PrimitiveTopology::TriangleList,
        CullMode::None,
        true,
        CompareFunction::Less,
        ColorWriteMask::All);
}

Scene3DPipelineFactory::ShadowPipeline Scene3DPipelineFactory::CreateDirectionalShadowPipeline(RenderContext& ctx) {
    return CreateShadowPipeline(
        ctx,
        ShaderPaths::Resolve("scene/scene_directional_shadow.wgsl"),
        "Scene3DPipelineFactory/DirectionalShadow",
        PrimitiveTopology::TriangleList,
        CullMode::None);
}
