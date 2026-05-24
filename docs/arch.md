# 项目组织架构

## 当前边界

- `Application` 负责应用生命周期、主循环编排、场景切换、输入系统接线和 GUI 帧驱动。
- `WindowContext` 负责 GLFW 窗口生命周期、事件轮询和 drawable size 查询。
- `RenderContext` 负责 WebGPU `device / queue / surface`，以及 `acquire / submit / present` 和共享 GPU 缓存入口。
- `Renderer` 负责新版 3D 渲染路径的整帧编排，内部调度 shadow、prepass、SSAO、skybox、forward、PBR、DoF、tone mapping 和 GUI。
- `AssetServer` 负责 CPU 侧资产缓存与 importer 分发，当前覆盖 glTF 模型和 HDR 图像。
- `LegacyGuiRenderer`、`LegacyFrameContext`、`LegacyPipeline2D`、`LegacyRenderer3D` 都是兼容层，不扩散到新 3D 渲染架构内部。

## 源代码层级

```text
src
├── main                         # 程序入口，解析 config、选择 surface format、创建并启动 Application
├── webgpu-raii                  # WebGPU C++ RAII 辅助封装，统一管理 WebGPU 句柄生命周期
│
├── app                          # 应用 / 平台层
│   ├── Application              # 顶层协调者：初始化子系统、驱动主循环、切场景、驱动 GUI 和输入分发
│   ├── ConfigPaths              # config 根目录解析，供 main/Application 读取运行配置
│   └── WindowContext            # GLFW 窗口与平台层包装：窗口生命周期、事件轮询、drawable size、高 DPI 限制
│
├── asset                        # CPU 侧资产层
│   ├── AssetId                  # 轻量资产句柄类型，解耦运行时引用和真实资产存储
│   ├── AssetPaths               # assets 根目录解析，统一处理原生 / Web 资源路径
│   ├── AssetServer              # CPU 资产缓存与 importer 入口，负责按路径加载、缓存、返回 AssetId<T>
│   ├── ShaderPaths              # shader 根目录解析，供 pipeline/shader loader 定位 WGSL 文件
│   ├── importers
│   │   ├── GltfImporter         # glTF / GLB 导入器，生成 mesh/material/image/model 等 CPU 资产
│   │   └── HdrImageImporter     # HDR 图像导入器，把 .hdr 解码成浮点像素的 HdrImageAsset
│   └── types
│       ├── AssetVertex3D        # 新渲染路径统一使用的顶点布局，匹配 WGSL vertex 输入
│       ├── HdrImageAsset        # HDR 图像资产：保存 source path、尺寸和 float RGBA 像素
│       ├── ImageAsset           # 8-bit 图像资产：用于常规 baseColor/normal/metallicRoughness 纹理
│       ├── MaterialAsset        # CPU 材质描述：颜色、PBR 参数、纹理引用、shading model
│       ├── MeshAsset            # CPU 网格数据：顶点、索引、primitive range
│       └── ModelAsset           # 模型层级结果：节点变换、primitive 组合、mesh/material 引用
│
├── input                        # 输入系统
│   ├── InputManager             # 输入系统入口：维护当前按键/鼠标状态，并把平台事件转换成领域事件
│   ├── InputEventBus            # 输入事件总线，基于 entt::dispatcher 解耦 Application 与 Scene
│   ├── InputEventLogger         # 输入调试日志工具，便于观察事件发射顺序和参数
│   ├── InputState               # 输入状态快照，保存当前键盘/鼠标状态供 policy 查询
│   ├── gui
│   │   └── GuiInputController   # GUI 与游戏输入的协调层，负责 ImGui 捕获判断和调试面板构建
│   └── policies
│       ├── AppHotkeyPolicy      # 应用级快捷键策略：场景切换、模式切换等全局命令
│       ├── CameraLookPolicy     # 鼠标/视角输入到相机朝向事件的映射策略
│       ├── CameraMovePolicy     # 键盘输入到相机平移事件的映射策略
│       ├── InputPolicy          # 输入策略接口，约束“平台输入 -> 领域事件”的映射方式
│       ├── Transform2DPolicy    # 旧 2D 场景对象控制策略
│       └── Transform3DPolicy    # 旧 3D 对象控制策略
│
├── math                         # 数学工具层
│   ├── Transform2D              # 2D 平移/旋转/缩放和矩阵生成工具
│   └── Transform3D              # 3D TRS 与 model matrix 生成工具
│
├── resource                     # legacy CPU 资源兼容层
│   ├── loaders
│   │   ├── ISceneLoader         # 旧场景描述加载接口
│   │   ├── LegacyObjLoader      # 旧 OBJ 资源加载器
│   │   └── LegacyTxtGeometryLoader # 旧 txt 几何格式加载器
│   └── legacy
│       ├── LegacyMeshData3D      # 旧版 CPU 网格结构
│       ├── LegacyResourceManager # 旧资源组织入口
│       ├── LegacyResourcePaths   # 旧资源路径解析
│       ├── LegacyShaderLoader    # 旧 shader 文件加载器
│       └── LegacyTomlSceneLoader # 旧 TOML 场景描述加载器
│
├── scene                        # CPU 侧运行时场景层
│   ├── IScene                   # Application 可切换场景接口，统一 update/render/input 注册生命周期
│   ├── SceneManager             # 管理场景工厂、激活场景实例、场景切换和输入接线
│   ├── LogicScene               # 新版 3D 场景基类：装配 World、持有 AssetServer、生成 RenderScene
│   ├── World                    # EnTT 世界封装：实体创建、层级关系、主相机/主光源查询、world matrix 计算
│   ├── Entity                   # 对 entt::entity 的轻量封装，提供组件增删改查和父子关系辅助
│   ├── camera
│   │   ├── Camera               # 相机抽象基类，提供位姿、view/projection 接口
│   │   ├── CameraController     # 相机控制器接口
│   │   ├── FreeCameraController # 自由相机控制器，实现移动/观察输入响应
│   │   ├── OrthographicCamera   # 正交相机实现
│   │   └── PerspectiveCamera    # 透视相机实现
│   ├── components
│   │   ├── CameraComponent      # 把 Camera 实例挂到实体上，并标记 primary camera
│   │   ├── ChildrenComponent    # 子节点列表
│   │   ├── NameComponent        # 实体调试名
│   │   ├── ParentComponent      # 父节点引用
│   │   ├── StaticMeshComponent  # 可渲染静态网格引用和 shading model override
│   │   ├── TransformComponent   # 本地变换
│   │   └── light/*              # Directional / Point / Spot Light 组件
│   └── legacy                   # 旧场景兼容层
│       ├── Object3D             # 旧 3D 对象表示
│       ├── Scene2D              # 旧 2D 场景
│       ├── LegacyScene3D        # 旧 3D 场景
│       └── SceneDescription     # 旧场景描述结构
│
└── render                       # GPU 渲染层
    ├── RenderContext            # WebGPU device / queue / surface 封装，以及 acquire/submit/present 和环境贴图 GPU 缓存入口
    ├── Renderer                 # 新 3D 渲染器总入口：准备帧资源、构建 draw item、顺序执行各 pass
    ├── ShaderLoader             # WGSL 文件读取器，供 pipeline factory 统一创建 shader module
    ├── frame
    │   ├── SurfaceFrame         # 一次 surface acquire 的结果：surface texture、surface view、surface size 快照
    │   └── RenderFrame          # 单帧上下文：SurfaceFrame + command encoder + 各中间目标 view
    ├── gpu
    │   ├── EnvironmentMapCache  # HDR equirect 上传、compute 预计算 cubemap、环境贴图 GPU 级缓存
    │   ├── GpuMesh              # GPU 顶点/索引缓冲封装
    │   └── GpuResourceCache     # CPU mesh/material/image 到 GPU buffer/texture/sampler 的缓存和上传
    ├── passes
    │   ├── IRenderPass          # 统一 pass 接口和 PassContext 结构
    │   ├── DepthPrepass         # 主场景深度预通道，输出 scene depth
    │   ├── DofPass              # 基础景深 pass：CoC 计算 + 双向 blur，输出 HDR DoF 结果
    │   ├── ForwardOpaquePass    # Unlit/Blinn-Phong 前向不透明 pass，输出到 HDR scene color
    │   ├── GuiPass              # 在最终 surface 上叠加 ImGui
    │   ├── PBRPass              # PBR 不透明 pass，输出到 HDR scene color
    │   ├── PreparedDrawItem     # RenderObject 预处理结果，汇总 GPU buffer、bind group、uniform 数据
    │   ├── SSAOPass             # SSAO 屏幕空间环境遮蔽 pass，输出 scene AO
    │   ├── SurfacePrepass       # 表面属性预通道，输出 scene normal / scene reflectivity
    │   ├── ShadowPass           # 方向光 shadow map pass
    │   ├── SkyboxPass           # cubemap 天空盒 pass，先写 HDR scene color 背景
    │   └── ToneMapPass          # HDR scene color -> surface 的末端 tone mapping pass
    ├── pipelines
    │   └── Scene3DPipelineFactory # 所有新 3D render/compute pipeline、bind group layout、pipeline layout 的工厂
    ├── scene
    │   ├── RenderCamera         # 渲染用相机快照：view、projection、position
    │   ├── RenderLightSet       # 渲染用灯光快照：directional/point/spot light 数据
    │   ├── RenderObject         # 单个可渲染物体的提交数据：mesh/material/shading/world matrix
    │   ├── RenderScene          # 一帧要绘制的渲染快照：camera、lights、objects、skybox、assetServer
    │   └── RenderUniformData    # CPU 与 WGSL 对齐的 uniform/constant 数据布局
    └── legacy                   # legacy 渲染兼容层
        ├── LegacyFrameContext   # 旧式帧上下文封装
        ├── LegacyGuiRenderer    # 旧 ImGui 渲染器
        ├── LegacyPipeline2D     # 旧 2D pipeline 工厂
        └── LegacyRenderer3D     # 旧 3D 场景到新 Renderer 的桥接层
```

## 关键调用关系

### 应用主循环

1. `Application` 调用 `WindowContext::PollEvents()`。
2. `Application` 计算 `deltaTime` 并更新当前场景。
3. `Application` 调用 `LegacyGuiRenderer::BeginFrame(...)`，构建 UI，随后 `EndFrame()`。
4. `Application` 调用 `SceneManager::RenderActive(...)`。

### CPU 资产到运行时场景

1. `LogicScene` 持有自己的 `AssetServer`。
2. `AssetServer` 通过 importer 加载 glTF、HDR 等 CPU 资产，并缓存 `AssetId<T>`。
3. `LogicScene` 把 `StaticMeshComponent`、光源和相机状态整理为 `RenderScene`。
4. `RenderScene` 只携带本帧渲染所需的轻量快照和资产句柄，不负责 GPU 所有权。

### 新 3D 渲染路径

1. `Renderer` 调用 `RenderContext::AcquireSurfaceFrame()` 获取 swapchain 当前帧。
2. `Renderer` 在 `EnsureFrameResources(...)` 中按窗口尺寸准备中间目标：
   `sceneDepth(Depth24Plus)`、`sceneAo(R8Unorm)`、`sceneColor(RGBA16Float)`、`sceneNormal(RGBA16Float)`、`sceneCoc(R16Float)`、`sceneDofPing(RGBA16Float)`、`sceneDofColor(RGBA16Float)`。
3. `Renderer` 通过 `GpuResourceCache` 把 `RenderObject` 依赖的 CPU mesh/material/image 同步到 GPU。
4. 如果 `RenderScene` 带 skybox，`Renderer` 通过 `EnvironmentMapCache` 把 `HdrImageAsset` 上传为 HDR 2D 纹理，并在首次需要时用 compute shader 预计算 cubemap。
5. 所有 pass 顺序写入同一个 command encoder：
   - `ShadowPass`
   - `DepthPrepass`
   - `SurfacePrepass`
   - `SSAOPass`
   - `SkyboxPass`
   - `ForwardOpaquePass`
   - `PBRPass`
   - `DofPass`
   - `ToneMapPass`
   - `GuiPass`
6. `Renderer` 调用 `RenderContext::Submit(...)` 和 `RenderContext::Present(...)`。

### 中间目标与最终上屏

- `DirectionalShadowTexture`
  - 格式：`Depth24Plus`
  - 用途：主方向光 shadow map
- `SceneDepthTexture`
  - 格式：`Depth24Plus`
  - 用途：主场景深度、SSAO 输入、后续几何 pass 深度测试
- `SceneNormalTexture`
  - 格式：`RGBA16Float`
  - 用途：法线预通道，供 SSAO 采样
- `SceneAoTexture`
  - 格式：`R8Unorm`
  - 用途：AO 结果，供前向/PBR pass 采样
- `SceneColorTexture`
  - 格式：`RGBA16Float`
  - 用途：HDR 主颜色缓冲，skybox/forward/PBR 都写入这里，DoF 关闭时直接进入 tone mapping
- `SceneCocTexture`
  - 格式：`R16Float`
  - 用途：保存带符号 CoC，供 DoF blur 判断前后景与模糊半径
- `SceneDofPingTexture`
  - 格式：`RGBA16Float`
  - 用途：DoF 横向 blur 临时结果
- `SceneDofColorTexture`
  - 格式：`RGBA16Float`
  - 用途：DoF 最终 HDR 输出，供 `ToneMapPass` 采样
- `Surface Texture`
  - 格式：取决于 `render.surface_format`，通常是 `BGRA8Unorm` 或 `RGBA8Unorm`
  - 用途：`ToneMapPass` 把 HDR `sceneColor` 或 `sceneDofColor` 转到 surface，`GuiPass` 再叠加 GUI

## 设计约束

- `WindowContext` 负责窗口和事件循环，`RenderContext` 不承担平台生命周期。
- `RenderContext` 只提供 GPU 与 surface 原语，以及共享 GPU 级缓存入口。
- CPU 资产加载放在 `asset/*`，render/gpu 层不直接读磁盘文件格式。
- `Scene3DPipelineFactory` 同时负责 render pipeline 和 compute pipeline 的集中创建。
- 新 3D 路径当前是 HDR 中间颜色缓冲 + 末端 tone mapping，不再直接把几何 pass 写到 surface。
- legacy 类型都放在 `render/legacy` 或 `scene/legacy` 下，命名上明确其兼容层身份。
