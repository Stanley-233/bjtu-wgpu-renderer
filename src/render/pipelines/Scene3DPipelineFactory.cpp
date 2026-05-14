#include "Scene3DPipelineFactory.h"

#include <iostream>
#include <vector>

#include "resource/legacy/LegacyResourcePaths.h"
#include "resource/legacy/LegacyShaderLoader.h"
#include "render/RenderContext.h"

using namespace wgpu;

static Scene3DPipelineFactory::Pipeline CreatePipeline(
    RenderContext&          ctx,
    const char*             label,
    const WGPUPrimitiveTopology topology,
    const WGPUCullMode      cullMode,
    const bool              depthWriteEnabled,
    const WGPUCompareFunction depthCompare,
    const WGPUColorWriteMaskFlags colorWriteMask) {
    std::cout << "[" << label << "] Creating shader module..." << std::endl;
    ShaderModule shaderModule = LegacyShaderLoader::Load(
        LegacyResourcePaths::ResolveShader("shader3d.wgsl"),
        *ctx.GetDevice());
    std::cout << "[" << label << "] Shader module: " << shaderModule << std::endl;

    if (shaderModule == nullptr) {
        std::cerr << "[" << label << "] Could not load shader!" << std::endl;
        std::exit(1);
    }

    RenderPipelineDescriptor pipelineDesc{};
    std::vector<VertexAttribute> vertexAttribs(2);
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format         = VertexFormat::Float32x3;
    vertexAttribs[0].offset         = 0;
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format         = VertexFormat::Float32x3;
    vertexAttribs[1].offset         = 3 * sizeof(float);

    VertexBufferLayout vertexBufferLayout{};
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes     = vertexAttribs.data();
    vertexBufferLayout.arrayStride    = 6 * sizeof(float);
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

    FragmentState fragmentState{};
    fragmentState.module        = shaderModule;
    fragmentState.entryPoint    = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants     = nullptr;

    BlendState blendState{};
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;
    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;

    ColorTargetState colorTarget{};
    colorTarget.format    = ctx.GetSurfaceFormat();
    colorTarget.blend     = &blendState;
    colorTarget.writeMask = colorWriteMask;

    fragmentState.targetCount                       = 1;
    fragmentState.targets                           = &colorTarget;
    pipelineDesc.fragment                           = &fragmentState;

    DepthStencilState depthStencil{};
    depthStencil.format              = TextureFormat::Depth24Plus;
    depthStencil.depthWriteEnabled   = depthWriteEnabled;
    depthStencil.depthCompare        = depthCompare;
    depthStencil.stencilFront.compare = CompareFunction::Always;
    depthStencil.stencilBack.compare  = CompareFunction::Always;
    depthStencil.stencilReadMask     = 0;
    depthStencil.stencilWriteMask    = 0;
    depthStencil.depthBias           = 0;
    depthStencil.depthBiasSlopeScale = 0.0f;
    depthStencil.depthBiasClamp      = 0.0f;
    pipelineDesc.depthStencil        = &depthStencil;
    pipelineDesc.multisample.count   = 1;
    pipelineDesc.multisample.mask    = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    BindGroupLayoutEntry bindingLayout  = Default;
    bindingLayout.binding               = 0;
    bindingLayout.visibility            = ShaderStage::Vertex;
    bindingLayout.buffer.type           = BufferBindingType::Uniform;

    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries    = &bindingLayout;

    Scene3DPipelineFactory::Pipeline result;
    result.bindGroupLayout = ctx.GetDevice()->createBindGroupLayout(bindGroupLayoutDesc);

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts     = reinterpret_cast<WGPUBindGroupLayout*>(result.bindGroupLayout.Ptr());
    result.layout                   = ctx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline     = ctx.GetDevice()->createRenderPipeline(pipelineDesc);

    shaderModule.release();
    return result;
}

Scene3DPipelineFactory::Pipeline Scene3DPipelineFactory::CreateForwardPipeline(RenderContext& ctx) {
    return CreatePipeline(
        ctx,
        "Scene3DPipelineFactory/Forward",
        PrimitiveTopology::TriangleList,
        CullMode::None,
        true,
        CompareFunction::Less,
        ColorWriteMask::All);
}

Scene3DPipelineFactory::Pipeline Scene3DPipelineFactory::CreateWireframePipeline(RenderContext& ctx) {
    return CreatePipeline(
        ctx,
        "Scene3DPipelineFactory/Wireframe",
        PrimitiveTopology::LineList,
        CullMode::None,
        false,
        CompareFunction::LessEqual,
        ColorWriteMask::All);
}

Scene3DPipelineFactory::Pipeline Scene3DPipelineFactory::CreateWireframeDepthPrepassPipeline(RenderContext& ctx) {
    return CreatePipeline(
        ctx,
        "Scene3DPipelineFactory/WireframeDepthPrepass",
        PrimitiveTopology::TriangleList,
        CullMode::Back,
        true,
        CompareFunction::Less,
        ColorWriteMask::None);
}
