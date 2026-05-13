#ifndef BJTU_WGPU_RENDERER_RENDERER3D_H
#define BJTU_WGPU_RENDERER_RENDERER3D_H

#include "render/Renderer.h"
#include "render/scene/RenderScene.h"
#include "scene/legacy/Object3D.h"

class Camera;
class RenderContext;
class LegacyGuiRenderer;

class LegacyRenderer3D {
public:
    void Initialize(RenderContext& ctx);

    void SyncScene(RenderContext& ctx, const std::vector<Object3D>& objects, const Camera& camera);

    void RenderFrame(RenderContext& ctx, LegacyGuiRenderer& guiRenderer);

    void SetClearColor(double r, double g, double b, double a);

    void ResetGpuResources();

private:
    Renderer    m_renderer{};
    RenderScene m_scene{};
};

#endif // BJTU_WGPU_RENDERER_RENDERER3D_H
