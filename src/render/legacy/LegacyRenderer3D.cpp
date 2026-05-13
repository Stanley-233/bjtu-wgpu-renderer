#include "LegacyRenderer3D.h"

#include <algorithm>

#include "render/GuiRenderer.h"
#include "render/RenderContext.h"
#include "scene/camera/Camera.h"

void LegacyRenderer3D::Initialize(RenderContext& ctx) {
    m_renderer.Initialize(ctx);
}

void LegacyRenderer3D::SyncScene(RenderContext& ctx, const std::vector<Object3D>& objects, const Camera& camera) {
    int surfaceWidth = 0;
    int surfaceHeight = 0;
    ctx.GetDrawableSize(surfaceWidth, surfaceHeight);
    const float aspect = static_cast<float>(std::max(1, surfaceWidth)) / static_cast<float>(std::max(1, surfaceHeight));

    m_scene.camera = RenderCamera{
        .view = camera.View(),
        .projection = camera.Projection(aspect),
    };
    m_scene.objects.clear();
    m_scene.objects.reserve(objects.size());
    for (const Object3D& object : objects) {
        m_scene.objects.push_back(RenderObject{
            .worldMatrix = object.Transform().Matrix(),
            .mesh = &object.Mesh(),
            .renderMode = object.RenderMode(),
        });
    }
}

void LegacyRenderer3D::RenderFrame(RenderContext& ctx, GuiRenderer& guiRenderer) {
    m_renderer.Render(ctx, m_scene, guiRenderer);
}

void LegacyRenderer3D::SetClearColor(const double r, const double g, const double b, const double a) {
    m_renderer.SetClearColor(r, g, b, a);
}

void LegacyRenderer3D::ResetGpuResources() {
    m_renderer = Renderer{};
    m_scene = {};
}
