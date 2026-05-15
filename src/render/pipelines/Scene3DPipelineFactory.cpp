#include "Scene3DPipelineFactory.h"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <vector>

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

static Scene3DPipelineFactory::Pipeline CreatePipeline(
    RenderContext&                ctx,
    const std::filesystem::path&  shaderPath,
    const char*                   label,
    const bool                    usesMaterialGroup,
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

    RenderPipelineDescriptor     pipelineDesc{};
    pipelineDesc.label = label;
    std::vector<VertexAttribute> vertexAttribs(4);
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

    std::vector<BindGroupLayoutEntry> bindGroupBindings{};
    bindGroupBindings.reserve(4);

    BindGroupLayoutEntry objectBindingLayout = Default;
    objectBindingLayout.binding = 0;
    objectBindingLayout.visibility = ShaderStage::Vertex;
    objectBindingLayout.buffer.type = BufferBindingType::Uniform;
    bindGroupBindings.push_back(objectBindingLayout);

    if (usesMaterialGroup) {
        BindGroupLayoutEntry materialUniformBinding = Default;
        materialUniformBinding.binding = 1;
        materialUniformBinding.visibility = ShaderStage::Fragment;
        materialUniformBinding.buffer.type = BufferBindingType::Uniform;
        bindGroupBindings.push_back(materialUniformBinding);

        BindGroupLayoutEntry textureBinding = Default;
        textureBinding.binding = 2;
        textureBinding.visibility = ShaderStage::Fragment;
        textureBinding.texture.sampleType = TextureSampleType::Float;
        textureBinding.texture.viewDimension = TextureViewDimension::_2D;
        bindGroupBindings.push_back(textureBinding);

        BindGroupLayoutEntry samplerBinding = Default;
        samplerBinding.binding = 3;
        samplerBinding.visibility = ShaderStage::Fragment;
        samplerBinding.sampler.type = SamplerBindingType::Filtering;
        bindGroupBindings.push_back(samplerBinding);
    }

    BindGroupLayoutDescriptor objectBindGroupLayoutDesc{};
    objectBindGroupLayoutDesc.label = "Scene3D/ObjectBindGroupLayout";
    objectBindGroupLayoutDesc.entryCount = static_cast<uint32_t>(bindGroupBindings.size());
    objectBindGroupLayoutDesc.entries = bindGroupBindings.data();

    Scene3DPipelineFactory::Pipeline result;
    result.objectBindGroupLayout = ctx.GetDevice()->createBindGroupLayout(objectBindGroupLayoutDesc);

    std::vector<WGPUBindGroupLayout> bindGroupLayouts;
    bindGroupLayouts.reserve(1);
    bindGroupLayouts.push_back(*result.objectBindGroupLayout);

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

Scene3DPipelineFactory::Pipeline Scene3DPipelineFactory::CreateForwardPipeline(RenderContext& ctx) {
    return CreatePipeline(
        ctx,
        ShaderPaths::Resolve("scene/scene_unlit_textured.wgsl"),
        "Scene3DPipelineFactory/Forward",
        true,
        true,
        PrimitiveTopology::TriangleList,
        CullMode::None,
        true,
        CompareFunction::Less,
        ColorWriteMask::All);
}
