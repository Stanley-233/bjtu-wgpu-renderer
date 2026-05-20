#include "LegacyPipeline2D.h"

#include <iostream>
#include <vector>

#include "resource/legacy/LegacyResourcePaths.h"
#include "resource/legacy/LegacyShaderLoader.h"
#include "render/RenderContext.h"

using namespace wgpu;

LegacyPipeline2D::Pipeline LegacyPipeline2D::Create(RenderContext& renderCtx) {
    std::cout << "[LegacyPipeline2D] Creating shader module..." << std::endl;
    ShaderModule shaderModule = LegacyShaderLoader::Load(
        LegacyResourcePaths::ResolveShader("shader.wgsl"),
        *renderCtx.GetDevice());
    std::cout << "[LegacyPipeline2D] Shader module: " << shaderModule << std::endl;

    if (shaderModule == nullptr) {
        std::cerr << "[LegacyPipeline2D] Could not load shader!" << std::endl;
        std::exit(1);
    }

    RenderPipelineDescriptor pipelineDesc{};

    std::vector<VertexAttribute> vertexAttribs(2);
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format         = VertexFormat::Float32x2;
    vertexAttribs[0].offset         = 0;
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format         = VertexFormat::Float32x3;
    vertexAttribs[1].offset         = 2 * sizeof(float);

    VertexBufferLayout vertexBufferLayout{};
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes     = vertexAttribs.data();
    vertexBufferLayout.arrayStride    = 5 * sizeof(float);
    vertexBufferLayout.stepMode       = VertexStepMode::Vertex;

    pipelineDesc.vertex.bufferCount   = 1;
    pipelineDesc.vertex.buffers       = &vertexBufferLayout;
    pipelineDesc.vertex.module        = shaderModule;
    pipelineDesc.vertex.entryPoint    = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants     = nullptr;

    pipelineDesc.primitive.topology         = PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace        = FrontFace::CCW;
    pipelineDesc.primitive.cullMode         = CullMode::None;

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
    colorTarget.format    = renderCtx.GetSurfaceFormat();
    colorTarget.blend     = &blendState;
    colorTarget.writeMask = ColorWriteMask::All;

    fragmentState.targetCount                       = 1;
    fragmentState.targets                           = &colorTarget;
    pipelineDesc.fragment                           = &fragmentState;
    pipelineDesc.depthStencil                       = nullptr;
    pipelineDesc.multisample.count                  = 1;
    pipelineDesc.multisample.mask                   = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    BindGroupLayoutEntry bindingLayout  = Default;
    bindingLayout.binding               = 0;
    bindingLayout.visibility            = ShaderStage::Vertex;
    bindingLayout.buffer.type           = BufferBindingType::Uniform;

    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries    = &bindingLayout;

    Pipeline result;
    result.bindGroupLayout = renderCtx.GetDevice()->createBindGroupLayout(bindGroupLayoutDesc);

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts     = reinterpret_cast<WGPUBindGroupLayout*>(result.bindGroupLayout.Ptr());
    result.layout                   = renderCtx.GetDevice()->createPipelineLayout(layoutDesc);

    pipelineDesc.layout = *result.layout;
    result.pipeline     = renderCtx.GetDevice()->createRenderPipeline(pipelineDesc);

    shaderModule.release();
    return result;
}
