#include "Renderer.h"

#include <algorithm>

#include <glm/matrix.hpp>

#include "RenderContext.h"

namespace {
SceneUniformData BuildSceneUniformData(const RenderScene& scene) {
    SceneUniformData uniformData{};
    if (scene.camera.has_value()) {
        uniformData.view = scene.camera->view;
        uniformData.projection = scene.camera->projection;
        uniformData.cameraPosition = glm::vec4{scene.camera->position, 1.0f};
    }
    uniformData.lightCounts = glm::uvec4{
        scene.lights.directionalLightCount,
        scene.lights.pointLightCount,
        scene.lights.spotLightCount,
        0U,
    };
    uniformData.directionalLight = scene.lights.directionalLight;
    uniformData.pointLights = scene.lights.pointLights;
    uniformData.spotLights = scene.lights.spotLights;
    return uniformData;
}

ObjectUniformData BuildObjectUniformData(const glm::mat4& worldMatrix) {
    return ObjectUniformData{
        .model = worldMatrix,
        .normalMatrix = glm::transpose(glm::inverse(worldMatrix)),
    };
}
} // namespace

void Renderer::Initialize(RenderContext& ctx) {
    m_forwardPass.Initialize(ctx);
}

void Renderer::EnsureDepthResources(RenderContext& ctx, const int width, const int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (m_depthTexture && m_depthWidth == width && m_depthHeight == height) {
        return;
    }

    wgpu::TextureDescriptor depthDesc{};
    depthDesc.dimension = wgpu::TextureDimension::_2D;
    depthDesc.size.width = static_cast<uint32_t>(width);
    depthDesc.size.height = static_cast<uint32_t>(height);
    depthDesc.size.depthOrArrayLayers = 1;
    depthDesc.sampleCount = 1;
    depthDesc.mipLevelCount = 1;
    depthDesc.format = wgpu::TextureFormat::Depth24Plus;
    depthDesc.usage = wgpu::TextureUsage::RenderAttachment;
    m_depthTexture = ctx.GetDevice()->createTexture(depthDesc);
    m_depthView = m_depthTexture->createView();
    m_depthWidth = width;
    m_depthHeight = height;
}

RenderFrame Renderer::BeginRenderFrame(RenderContext& ctx) {
    RenderFrame frame{};
    frame.clearColor = m_clearColor;
    frame.surfaceFrame = ctx.AcquireSurfaceFrame();
    if (!frame.surfaceFrame.view) {
        return frame;
    }

    EnsureDepthResources(
        ctx,
        std::max(1, frame.surfaceFrame.surfaceWidth),
        std::max(1, frame.surfaceFrame.surfaceHeight));

    frame.encoder = ctx.CreateCommandEncoder();
    if (m_depthView) {
        frame.depthView = *m_depthView;
    }
    return frame;
}

void Renderer::EnsureSceneResources(RenderContext& ctx) {
    if (!m_sceneResources.uniformBuffer) {
        wgpu::BufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.size = sizeof(SceneUniformData);
        uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        uniformBufferDesc.mappedAtCreation = false;
        m_sceneResources.uniformBuffer = ctx.GetDevice()->createBuffer(uniformBufferDesc);
    }
}

void Renderer::EnsureObjectResources(RenderContext& ctx, const std::size_t objectCount) {
    if (m_objectResources.size() < objectCount) {
        m_objectResources.resize(objectCount);
    }

    for (std::size_t i = 0; i < objectCount; ++i) {
        ObjectResources& resources = m_objectResources[i];
        if (!resources.uniformBuffer) {
            wgpu::BufferDescriptor uniformBufferDesc{};
            uniformBufferDesc.size = sizeof(ObjectUniformData);
            uniformBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
            uniformBufferDesc.mappedAtCreation = false;
            resources.uniformBuffer = ctx.GetDevice()->createBuffer(uniformBufferDesc);
        }
    }
}

void Renderer::BuildSceneResources(RenderContext& ctx, const RenderScene& scene) {
    EnsureSceneResources(ctx);
    if (!m_sceneResources.uniformBuffer || !m_forwardPass.GetSceneBindGroupLayout()) {
        return;
    }

    const SceneUniformData uniformData = BuildSceneUniformData(scene);
    ctx.GetQueue()->writeBuffer(*m_sceneResources.uniformBuffer, 0, &uniformData, sizeof(SceneUniformData));

    wgpu::BindGroupEntry binding{};
    binding.binding = 0;
    binding.buffer = *m_sceneResources.uniformBuffer;
    binding.offset = 0;
    binding.size = sizeof(SceneUniformData);

    wgpu::BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = *m_forwardPass.GetSceneBindGroupLayout();
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &binding;
    m_sceneResources.sceneBindGroup = ctx.GetDevice()->createBindGroup(bindGroupDesc);
}

void Renderer::BuildPreparedDrawItems(RenderContext& ctx, const RenderScene& scene) {
    EnsureObjectResources(ctx, scene.objects.size());

    m_preparedDrawItems.clear();
    m_preparedDrawItems.reserve(scene.objects.size());

    for (std::size_t i = 0; i < scene.objects.size(); ++i) {
        const RenderObject& object = scene.objects[i];
        const GpuMesh* gpuMesh = m_resourceCache.SyncMesh(ctx, scene.assetServer, object);
        const GpuResourceCache::GpuMaterialResources* materialResources =
            m_resourceCache.SyncMaterial(ctx, scene.assetServer, object);
        if (gpuMesh == nullptr || materialResources == nullptr) {
            continue;
        }

        ObjectResources& resources = m_objectResources[i];
        if (!resources.uniformBuffer
            || !m_forwardPass.GetObjectBindGroupLayout()
            || !m_forwardPass.GetMaterialBindGroupLayout()) {
            continue;
        }

        const ObjectUniformData objectUniformData = BuildObjectUniformData(object.worldMatrix);
        ctx.GetQueue()->writeBuffer(*resources.uniformBuffer, 0, &objectUniformData, sizeof(ObjectUniformData));

        wgpu::BindGroupEntry objectBinding{};
        objectBinding.binding = 0;
        objectBinding.buffer = *resources.uniformBuffer;
        objectBinding.offset = 0;
        objectBinding.size = sizeof(ObjectUniformData);

        wgpu::BindGroupDescriptor objectBindGroupDesc{};
        objectBindGroupDesc.layout = *m_forwardPass.GetObjectBindGroupLayout();
        objectBindGroupDesc.entryCount = 1;
        objectBindGroupDesc.entries = &objectBinding;
        resources.objectBindGroup = ctx.GetDevice()->createBindGroup(objectBindGroupDesc);

        wgpu::BindGroupEntry materialBindings[3]{};
        materialBindings[0].binding = 0;
        materialBindings[0].buffer = *materialResources->uniformBuffer;
        materialBindings[0].offset = 0;
        materialBindings[0].size = sizeof(MaterialUniformData);
        materialBindings[1].binding = 1;
        materialBindings[1].textureView = materialResources->textureView;
        materialBindings[2].binding = 2;
        materialBindings[2].sampler = materialResources->sampler;

        wgpu::BindGroupDescriptor materialBindGroupDesc{};
        materialBindGroupDesc.layout = *m_forwardPass.GetMaterialBindGroupLayout();
        materialBindGroupDesc.entryCount = 3;
        materialBindGroupDesc.entries = materialBindings;
        resources.materialBindGroup = ctx.GetDevice()->createBindGroup(materialBindGroupDesc);

        m_preparedDrawItems.push_back(PreparedDrawItem{
            .shadingModel = static_cast<EMaterialShadingModel>(materialResources->uniformData.surfaceOptions.x),
            .model = object.worldMatrix,
            .vertexBuffer = *gpuMesh->vertexBuffer,
            .indexBuffer = *gpuMesh->indexBuffer,
            .objectBindGroup = resources.objectBindGroup ? *resources.objectBindGroup : nullptr,
            .materialBindGroup = resources.materialBindGroup ? *resources.materialBindGroup : nullptr,
            .vertexBufferSize = gpuMesh->vertexBufferSize,
            .indexBufferSize = gpuMesh->indexBufferSize,
            .indexCount = gpuMesh->indexCount,
        });
    }
}

void Renderer::Render(RenderContext& ctx, const RenderScene& scene, LegacyGuiRenderer& guiRenderer) {
    RenderFrame frame = BeginRenderFrame(ctx);
    if (!frame.surfaceFrame.view || !frame.encoder) {
        return;
    }

    BuildSceneResources(ctx, scene);
    BuildPreparedDrawItems(ctx, scene);

    const PassContext passContext{
        .camera = scene.camera,
        .lights = scene.lights,
        .drawItems = m_preparedDrawItems,
        .sceneBindGroup = m_sceneResources.sceneBindGroup ? *m_sceneResources.sceneBindGroup : nullptr,
        .guiRenderer = &guiRenderer,
        .queue = &*ctx.GetQueue(),
    };
    m_forwardPass.Render(frame, passContext);
    m_guiPass.Render(frame, passContext);
    ctx.Submit(frame.encoder);
    ctx.Present(frame.surfaceFrame);
}

void Renderer::SetClearColor(const double r, const double g, const double b, const double a) {
    m_clearColor = wgpu::Color{r, g, b, a};
}
