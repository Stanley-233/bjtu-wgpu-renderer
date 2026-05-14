• 重构计划

先定边界：

- legacy 收拢所有旧格式、旧 shader、旧样例资产。
- asset 只服务新版 LogicScene 和后续 glTF/Cornell Box 流程。
- cornel-box-original 不放进 legacy，作为新资产管线的第一个输入。
- SimpleMeshes、SimpleTexture 也按新资产管线接，不走 legacy。

目标目录

建议最终整理成这样：

src
├── asset
│   ├── AssetHandle.h
│   ├── AssetId.h
│   ├── AssetPaths.h
│   ├── AssetServer.h/.cpp
│   ├── types
│   │   ├── MeshAsset.h
│   │   ├── ImageAsset.h
│   │   ├── MaterialAsset.h
│   │   └── ModelAsset.h
│   └── importers
│       ├── GltfImporter.h/.cpp
│       ├── ObjImporter.h/.cpp
│       └── CornellBoxImporter.h/.cpp
│
├── render
│   ├── scene
│   ├── gpu
│   ├── passes
│   ├── pipelines
│   └── legacy
│       └── ...
│
├── scene
│   ├── LogicScene
│   └── legacy
│       └── ...
│
└── legacy
├── resource
│   ├── LegacyResourcePaths.h/.cpp
│   ├── LegacyResourceManager.h/.cpp
│   ├── loaders
│   └── ...
└── shaders

磁盘资源建议：

assets/
├── cornel-box-original/
├── gltf-test/
│   ├── SimpleMeshes.gltf
│   ├── SimpleTexture.gltf
│   └── ...

legacy/
├── assets/
│   ├── teapot/
│   ├── webgpu.txt
│   ├── scene3d.toml
│   └── ...
└── shaders/
├── shader.wgsl
└── shader3d.wgsl

执行顺序

1. 先做目录与命名迁移。
    - 新建 src/asset
    - 现有 src/resource 内容拆成两部分：
        - 旧 txt/obj/toml loader、旧场景描述、旧路径工具放到 src/legacy/resource
        - 新资产系统放到 src/asset
    - 现有 resources/shader.wgsl、resources/shader3d.wgsl 挪到 legacy/shaders/
2. 再改路径层。
    - 现有 src/resource/ResourcePaths.h:1 不够用了，要拆成：
        - AssetPaths：只管 assets/
        - LegacyResourcePaths：只管 legacy/assets/ 和 legacy/shaders/
        - ConfigPaths：如果 config.toml 还保留独立位置，就单独一套
    - 避免继续用一个 Resolve() 混所有类型资源。
3. 拆 ResourceManager。
    - 现有 src/resource/ResourceManager.h:1 是杂项入口，后面会越堆越乱。
    - 改成：
        - LegacyResourceManager 只给旧 Scene2D、LegacyScene3D 用
        - AssetServer 给 LogicScene 用
        - shader 文件读取也不要再挂在通用 ResourceManager 名下
4. 建新版 asset 数据结构。
    - MeshAsset：顶点、索引、子网格范围
    - ImageAsset：像素、宽高、格式
    - MaterialAsset：baseColorFactor、baseColorTexture
    - ModelAsset：mesh + material + node transform + primitive list
    - 第一版可以先不做太复杂的层级动画，只保留静态模型需要的数据
5. 改场景组件边界。
    - 现有 src/scene/components/StaticMeshComponent.h:1 直接持有 LegacyMeshData3D，这一步要改
    - 建议改成：

   struct StaticMeshComponent {
   AssetHandle<MeshAsset> mesh;
   AssetHandle<MaterialAsset> material;
   Object3D::ERenderMode renderMode = Object3D::ERenderMode::Solid;
   };
    - 这样 LogicScene 不再存 CPU 网格本体，只存资产引用

6. 改 LogicScene 装配流程。
    - 现有 src/scene/LogicScene.cpp:81 里是手搓 cube + 直接塞组件
    - 改成：
        - AssetServer.LoadModel(...)
        - 遍历 ModelAsset.primitives
        - 每个 primitive 建一个 entity
        - TransformComponent + StaticMeshComponent
    - 第一阶段先保留 debug cube，但也把它做成内建 MeshAsset
7. 改渲染提交结构。
    - 现有 src/render/scene/RenderObject.h:1 还在传 LegacyMeshData3D*
    - 应改成传资产引用，例如：

   struct RenderObject {
   glm::mat4 worldMatrix{1.0f};
   const MeshAsset* mesh = nullptr;
   const MaterialAsset* material = nullptr;
   Object3D::ERenderMode renderMode = Object3D::ERenderMode::Solid;
   };

8. 改 GPU cache。
    - 现有 src/render/gpu/GpuResourceCache.h:11 以 LegacyMeshData3D* 做 key，不适合新资产系统
    - 改成：
        - AssetId<MeshAsset> + renderMode
        - 之后纹理缓存再单独做 AssetId<ImageAsset>
9. 处理 shader 归档。
    - 既然你要求 shader 也进 legacy，那当前 src/render/pipelines/Scene3DPipelineFactory.cpp:21 如果还继续吃 shader3d.wgsl，要明确：
        - 这是“旧 3D shader”还是“新 3D shader”
    - 如果它是旧 shader，就迁到 legacy/shaders/shader3d.wgsl
    - 如果 LogicScene 后面也要继续用它，那建议立刻分叉：
        - legacy/shaders/shader3d_legacy.wgsl
        - assets 管线未来自己的 shaders/scene_pbr.wgsl 或 shaders/scene_unlit.wgsl
    - 不然名字相同但语义不同，后面会很乱
10. 接入顺序。
- SimpleMeshes.gltf
- SimpleTexture.gltf

建议的里程碑

1. legacy 目录和 asset 目录都建好，路径系统拆开，项目还能编译。
2. LogicScene 改成 asset handle，不再依赖 LegacyMeshData3D。
3. RenderObject 和 GpuResourceCache 完成 asset 化。
4. SimpleMeshes 和 SimpleTexture 加载到 LogicScene 跑通。

关键决策
- shader3d.wgsl 如果未来要继续服务 LogicScene，最好现在就和 legacy 版本分家，否则目录迁完还会再迁一次。
