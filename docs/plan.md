# 重构与优化扩展能力架构

## 场景逻辑层
src/scene
├── Scene.h
├── SceneManager.cpp
├── SceneManager.h
├── ModernScene.cpp
└── ModernScene.h
├── legacy
    ├── Object3D.cpp
    ├── Object3D.h
    ├── Scene3D.cpp
    └── Scene3D.h
└── world
    ├── World.cpp
    ├── World.h
    ├── Actor.cpp
    ├── Actor.h
    ├── ActorComponent.h
└── components
    ├── StaticMeshComponent.h
    ├── CameraComponent.h
    ├── LightComponent.h
    ├── DirectionalLightComponent.h
    ├── PointLightComponent.h
    └── Material.h

## 