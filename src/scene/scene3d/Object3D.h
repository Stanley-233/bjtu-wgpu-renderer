#ifndef BJTU_WGPU_RENDERER_OBJECT3D_H
#define BJTU_WGPU_RENDERER_OBJECT3D_H

#include "../../math/Transform3D.h"
#include "../../resource/models/MeshData3D.h"

class Object3D {
public:
    void SetTransform(const Transform3D& transform);

    [[nodiscard]] Transform3D& Transform();

    [[nodiscard]] const Transform3D& Transform() const;

    void SetMesh(const MeshData3D& mesh);

    [[nodiscard]] const MeshData3D& Mesh() const;

private:
    Transform3D m_transform{};
    MeshData3D  m_mesh{};
};

#endif // BJTU_WGPU_RENDERER_OBJECT3D_H
