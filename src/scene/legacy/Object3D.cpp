#include "Object3D.h"

void Object3D::SetTransform(const Transform3D& transform) {
    m_transform = transform;
}

Transform3D& Object3D::Transform() {
    return m_transform;
}

const Transform3D& Object3D::Transform() const {
    return m_transform;
}

void Object3D::SetMesh(const LegacyMeshData3D& mesh) {
    m_mesh = mesh;
}

const LegacyMeshData3D& Object3D::Mesh() const {
    return m_mesh;
}
