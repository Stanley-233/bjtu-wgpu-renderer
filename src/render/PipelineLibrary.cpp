#include "PipelineLibrary.h"

#include <iostream>
#include <vector>

#include "RenderContext.h"
#include "../math/Transform2D.h"
#include "../resource/ResourceManager.h"
#include "../resource/ResourcePaths.h"

using namespace wgpu;

PipelineLibrary::Pipeline2D PipelineLibrary::CreateColor2D(RenderContext& ctx) {
    std::cout << "Creating shader module..." << std::endl;
    ShaderModule shaderModule = ResourceManager::LoadShaderModule(
        ResourcePaths::Resolve("shader.wgsl"),
        *ctx.GetDevice());
    std::cout << "Shader module: " << shaderModule << std::endl;

    if (shaderModule == nullptr) {
        std::cerr << "Could not load shader!" << std::endl;
        std::exit(1);
    }

    RenderPipelineDescriptor pipelineDesc;

    std::vector<VertexAttribute> vertexAttribs(2);
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format         = VertexFormat::Float32x2;
    vertexAttribs[0].offset         = 0;
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format         = VertexFormat::Float32x3;
    vertexAttribs[1].offset         = 2 * sizeof(float);

    VertexBufferLayout vertexBufferLayout;
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

    FragmentState fragmentState;
    fragmentState.module        = shaderModule;
    fragmentState.entryPoint    = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants     = nullptr;

    BlendState blendState;
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;
    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;

    ColorTargetState colorTarget;
    colorTarget.format    = ctx.GetSurfaceFormat();
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
    // bindingLayout.buffer.minBindingSize = 4 * sizeof(float);

    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries    = &bindingLayout;

    Pipeline2D result;
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

// 3D 渲染管线创建
PipelineLibrary::Pipeline3D PipelineLibrary::CreateColor3D(RenderContext& ctx) {
    // Shader
    std::cout << "[PipelineLibrary] Creating 3d shader module..." << std::endl;
    ShaderModule shaderModule = ResourceManager::LoadShaderModule(
        ResourcePaths::Resolve("shader3d.wgsl"),
        *ctx.GetDevice());
    std::cout << "[PipelineLibrary] Shader module: " << shaderModule << std::endl;

    if (shaderModule == nullptr) {
        std::cerr << "[PipelineLibrary] Could not load shader!" << std::endl;
        std::exit(1);
    }

    RenderPipelineDescriptor pipelineDesc{};
    // vertex layout
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
    colorTarget.format    = ctx.GetSurfaceFormat();
    colorTarget.blend     = &blendState;
    colorTarget.writeMask = ColorWriteMask::All;

    fragmentState.targetCount                       = 1;
    fragmentState.targets                           = &colorTarget;
    pipelineDesc.fragment                           = &fragmentState;

    // 深度测试与模板测试
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

    // Bind group layout
    BindGroupLayoutEntry bindingLayout  = Default;
    bindingLayout.binding               = 0;
    bindingLayout.visibility            = ShaderStage::Vertex;
    bindingLayout.buffer.type           = BufferBindingType::Uniform;

    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries    = &bindingLayout;

    Pipeline3D result;
    result.bindGroupLayout = ctx.GetDevice()->createBindGroupLayout(bindGroupLayoutDesc);

    // Pipeline layout
    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts     = reinterpret_cast<WGPUBindGroupLayout*>(result.bindGroupLayout.Ptr());
    result.layout                   = ctx.GetDevice()->createPipelineLayout(layoutDesc);

    // Render Pipeline
    pipelineDesc.layout = *result.layout;
    result.pipeline     = ctx.GetDevice()->createRenderPipeline(pipelineDesc);

    shaderModule.release();
    return result;
}
