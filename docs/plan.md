# 项目组织架构
## 源代码层级
src
├── main                         # 程序入口，创建 Application 并启动主循环
├── webgpu-raii                  # WebGPU C++ RAII 辅助封装，偏底层工具
│
├── app                          # 应用层：生命周期、主循环、场景切换、输入和渲染编排
│   └── Application              # 程序总入口对象，持有 RenderContext / Renderer / SceneManager / InputManager
│
├── input                        # 输入系统：统一收集窗口输入，并分发给 GUI、相机、物体控制等策略
│   ├── InputManager             # 输入系统入口，接收平台事件，更新 InputState，并驱动策略处理
│   ├── InputState               # 当前帧输入状态快照，例如按键、鼠标位置、滚轮、拖拽状态
│   ├── InputEventBus            # 输入事件总线，用于发布 / 订阅输入事件，降低模块耦合
│   ├── InputEventLogger         # 输入事件调试日志，可用于观察事件流
│   ├── gui                      # GUI 输入适配层
│   │   └── GuiInputController   # 判断输入是否被 ImGui 捕获，并协调 GUI 与场景输入
│   └── policies                 # 输入策略层：把输入解释成具体行为
│       ├── InputPolicy          # 输入策略接口
│       ├── AppHotkeyPolicy      # 应用级快捷键策略，例如退出、切换场景、开关 Debug UI
│       ├── CameraMovePolicy     # 相机移动策略，处理 WASD / 鼠标视角等
│       ├── Transform2DPolicy    # 2D 对象变换控制策略
│       └── Transform3DPolicy    # 3D 对象变换控制策略
│
├── math                         # 数学工具层：变换、矩阵、向量相关的轻量封装
│   ├── Transform2D              # 2D 变换，负责平移、旋转、缩放以及矩阵生成
│   └── Transform3D              # 3D 变换，负责平移、旋转、缩放以及 model matrix 生成
│
├── resource                     # 资源层：资产路径、模型 / 场景 / Shader 加载，不直接持有 GPU 状态
│   ├── ResourceManager          # 资源管理入口，负责统一加载、缓存和查询 CPU 侧资源
│   ├── ResourcePaths            # 资源路径工具，统一处理 shader、model、texture 等路径
│   ├── loaders                  # 新版资源加载器接口和实现
│   │   ├── IModelLoader         # 模型加载器接口，例如 OBJ / glTF 都可以实现它
│   │   ├── ISceneLoader         # 场景加载器接口，例如 TOML / glTF scene 都可以实现它
│   │   ├── LegacyTxtGeometryLoader # 旧 txt 几何数据加载器
│   │   ├── ObjLoader            # OBJ 模型加载器
│   │   └── ShaderLoader         # Shader 文件加载器，读取 WGSL / SPIR-V 等 shader 源文件
│   └── legacy                   # 旧资源格式兼容层
│       ├── LegacyTomlSceneLoader # 旧 TOML 场景加载器，服务旧 Scene2D / Scene3D
│       └── LegacyMeshData3D      # 旧 CPU 网格数据结构
│
├── scene                        # CPU 侧运行时场景层，不直接接触 WebGPU
│   ├── IScene                   # Application 可切换的逻辑场景接口
│   ├── SceneManager             # 管理当前 IScene，负责 Scene2D / Scene3D / LogicScene 切换
│   ├── LogicScene               # 新版 3D 场景入口，内部持有 World，并把 World 提取成 RenderScene
│   ├── World                    # 运行时世界，内部持有 entt::registry，管理 Entity / Component / Update / 主相机
│   ├── Entity                   # 对 entt::entity 的轻量封装，类似 GameObject / Actor 句柄
│   │
│   ├── camera                   # CPU 侧相机模型工具类
│   │   ├── Camera               # 相机基类，提供 view / projection 相关接口
│   │   ├── PerspectiveCamera    # 透视相机，负责透视投影矩阵生成
│   │   └── OrthographicCamera   # 正交相机，负责正交投影矩阵生成
│   │
│   ├── components               # 挂在 Entity 上的运行时组件，利用 EnTT 的 Has 做 GetComponent
│   │   ├── TransformComponent   # 表示 Entity 的位置、旋转、缩放、父子层级变换
│   │   ├── StaticMeshComponent  # 表示这个 Entity 有一个可渲染静态网格
│   │   ├── CameraComponent      # 表示这个 Entity 是一个相机，持有 / 引用 Camera
│   │   ├── DirectionalLightComponent # 【先不做】平行光组件
│   │   ├── PointLightComponent       # 【先不做】点光源组件
│   │   └── MaterialComponent         # 【先不做】材质组件
│   │
│   └── legacy                   # 旧场景系统兼容层
│       ├── Object3D             # 旧版 3D 对象，后续由 Entity + Component 替代
│       ├── Scene2D              # 旧版 2D 场景
│       ├── Scene3D              # 旧版 3D 场景
│       └── SceneDescription     # 旧版场景描述结构，主要服务 TOML 场景加载
│
└── render                       # GPU 渲染层，负责把 RenderScene 画出来
    ├── RenderContext            # WebGPU 设备、队列、Surface、窗口尺寸、depth texture 等上下文
    ├── PipelineLibrary          # 管理 / 创建 RenderPipeline，避免到处手写 pipeline
    ├── Renderer                 # 渲染器总入口，调度 Pass、管理 GpuResourceCache
    │
    ├── frame                    # 单帧渲染临时对象
    │   └── RenderFrame          # 当前帧的 encoder、surfaceView、depthView 等临时资源
    │
    ├── scene                    # 渲染层数据快照，不拥有 CPU World 逻辑
    │   ├── RenderScene          # 一帧要渲染的对象、相机、光源等集合
    │   ├── RenderObject         # 单个可渲染物体的渲染提交数据
    │   ├── RenderCamera         # 渲染用相机矩阵，例如 view / projection / viewProjection
    │   ├── RenderLight          # 【先不做】渲染用光源数据
    │   └── RenderMaterial       # 【先不做】渲染用材质数据
    │
    ├── gpu                      # GPU 资源层，凡是 wgpu::Buffer / Texture / BindGroup 都放这里
    │   ├── GpuBuffer            # 对 wgpu::Buffer 的薄封装
    │   ├── GpuMesh              # GPU 侧网格，持有 vertex / index buffer
    │   ├── GpuResourceCache     # CPU 资产到 GPU 资源的缓存和上传管理
    │   ├── GpuMaterial          # 【先不做】GPU 侧材质资源，持有 material uniform / bind group
    │   └── GpuTexture           # 【先不做】GPU 侧纹理资源，持有 texture / view / sampler
    │
    ├── passes                   # 具体渲染 Pass
    │   ├── IRenderPass          # Pass 接口
    │   ├── ForwardPass          # 主渲染 Pass，负责正常模型绘制
    │   ├── DepthPrepass         # Pre-Z Pass，用于提前写入深度，后续可迁移
    │   ├── WireframePass        # 线框 Pass，用于 Debug 显示
    │   └── GuiPass              # ImGui Pass，后续可接管 GuiRenderer
    │
    └── legacy                   # 旧渲染系统兼容层
        ├── GuiRenderer          # 旧 ImGui 渲染器，后续可迁移到 GuiPass
        └── LegacyRenderer3D     # 旧 3D 渲染器，服务旧 Scene3D

## 渲染过程
CPU 逻辑世界
World
├── Entity A
│   ├── TransformComponent
│   └── StaticMeshComponent
├── Entity B
│   ├── TransformComponent
│   └── CameraComponent
└── Entity C
├── TransformComponent
└── StaticMeshComponent

提取为渲染快照
RenderScene
├── RenderCamera
└── RenderObject[]
├── worldMatrix
├── mesh handle / mesh asset ref
└── render mode

GPU 渲染执行
Renderer
├── GpuResourceCache: MeshAsset -> GpuMesh
├── ForwardPass: draw normal objects
├── WireframePass: draw debug wireframe
└── GuiPass: draw ImGui