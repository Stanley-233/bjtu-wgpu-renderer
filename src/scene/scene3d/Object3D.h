#ifndef BJTU_WGPU_RENDERER_OBJECT3D_H
#define BJTU_WGPU_RENDERER_OBJECT3D_H

#include "../../math/Transform3D.h"
#include "../../resource/models/MeshData3D.h"

class Object3D {
public:
    enum class ERenderMode {
        Solid,
        Wireframe,
    };

    void SetTransform(const Transform3D& transform);

    [[nodiscard]] Transform3D& Transform();

    [[nodiscard]] const Transform3D& Transform() const;

    void SetMesh(const MeshData3D& mesh);

    [[nodiscard]] const MeshData3D& Mesh() const;

    void SetRenderMode(ERenderMode mode);

    [[nodiscard]] ERenderMode RenderMode() const;

private:
    // Transform3D 本质上就是 Model 矩阵，把模型空间变换到世界空间
    Transform3D m_transform{};
    // 存储模型空间的Mesh信息
    MeshData3D  m_mesh{};
    ERenderMode m_renderMode{ERenderMode::Solid};
};

#endif // BJTU_WGPU_RENDERER_OBJECT3D_H
