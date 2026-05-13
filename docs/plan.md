# 项目组织架构

## 当前边界

- `Application` 负责应用生命周期、主循环编排、场景切换、输入系统接线和 GUI 帧驱动。
- `WindowContext` 负责 GLFW 窗口生命周期、事件轮询和 drawable size 查询。
- `RenderContext` 负责 WebGPU device、queue、surface 以及 surface acquire / submit / present。
- `Renderer` 负责新 3D 渲染路径的整帧编排，内部调度 `ForwardPass`、`WireframePass`、`GuiPass`。
- `LegacyGuiRenderer`、`LegacyFrameContext`、`LegacyPipeline2D`、`LegacyRenderer3D` 都是兼容层，不扩散到新 3D 渲染架构内部。

## 源代码层级

```text
src
├── main                         # 程序入口，创建 Application 并启动主循环
├── webgpu-raii                  # WebGPU C++ RAII 辅助封装
│
├── app                          # 应用 / 平台层
│   ├── Application              # 持有 WindowContext / RenderContext / LegacyGuiRenderer / SceneManager / InputManager
│   └── WindowContext            # GLFW 窗口、事件循环、drawable size、高 DPI 画布配置
│
├── input                        # 输入系统
│   ├── InputManager             # 输入系统入口，发出输入事件
│   ├── InputEventBus            # 输入事件总线
│   ├── InputEventLogger         # 输入调试日志
│   ├── InputState               # 输入状态快照
│   ├── gui
│   │   └── GuiInputController   # GUI 捕获判断与输入协调
│   └── policies
│       ├── AppHotkeyPolicy      # 应用级快捷键策略
│       ├── CameraMovePolicy     # 相机移动策略
│       ├── InputPolicy          # 输入策略接口
│       ├── Transform2DPolicy    # 2D 变换控制策略
│       └── Transform3DPolicy    # 3D 变换控制策略
│
├── math                         # 数学工具层
│   ├── Transform2D              # 2D 变换与矩阵生成
│   └── Transform3D              # 3D 变换与 model matrix 生成
│
├── resource                     # CPU 侧资源层
│   ├── ResourceManager          # 资源加载入口
│   ├── ResourcePaths            # 资源路径解析
│   ├── loaders
│   │   ├── IModelLoader         # 模型加载器接口
│   │   ├── ISceneLoader         # 场景加载器接口
│   │   ├── LegacyTxtGeometryLoader # 旧 txt 几何加载器
│   │   ├── ObjLoader            # OBJ 加载器
│   │   └── ShaderLoader         # Shader 文件加载器
│   └── legacy
│       ├── LegacyMeshData3D     # 旧 CPU 网格数据结构
│       └── LegacyTomlSceneLoader # 旧 TOML 场景加载器
│
├── scene                        # CPU 侧运行时场景层
│   ├── IScene                   # Application 可切换场景接口
│   ├── SceneManager             # 管理当前激活场景
│   ├── LogicScene               # 新版 3D 场景入口，提取 RenderScene 给 Renderer
│   ├── World                    # EnTT 世界与主相机管理
│   ├── Entity                   # 对 entt::entity 的轻量封装
│   ├── camera
│   │   ├── Camera               # 相机基类
│   │   ├── OrthographicCamera   # 正交相机
│   │   └── PerspectiveCamera    # 透视相机
│   ├── components
│   │   ├── CameraComponent      # 相机组件
│   │   ├── NameComponent        # 名称组件
│   │   ├── StaticMeshComponent  # 静态网格组件
│   │   └── TransformComponent   # 变换组件
│   └── legacy                   # 旧场景兼容层
│       ├── Object3D             # 旧 3D 对象
│       ├── Scene2D              # 旧 2D 场景，使用 LegacyPipeline2D + LegacyFrameContext
│       ├── Scene3DLegacy        # 旧 3D 场景，使用 LegacyRenderer3D 适配到新 Renderer
│       └── SceneDescription     # 旧场景描述结构
│
└── render                       # GPU 渲染层
    ├── RenderContext            # 纯 GPU / surface 上下文，不拥有窗口生命周期
    ├── Renderer                 # 新 3D 渲染器总入口
    ├── frame
    │   ├── SurfaceFrame         # 一次 surface acquire 的结果：texture、view、surface size 快照
    │   └── RenderFrame          # 新渲染路径单帧对象：SurfaceFrame + encoder + depthView + clearColor
    ├── gpu
    │   ├── GpuMesh              # GPU 侧网格资源
    │   └── GpuResourceCache     # CPU 网格到 GPU 资源的缓存和上传
    ├── passes
    │   ├── IRenderPass          # Pass 接口
    │   ├── ForwardPass          # 主 3D 绘制 pass
    │   ├── GuiPass              # 包装 LegacyGuiRenderer 的 pass
    │   ├── PreparedDrawItem     # 供 pass 使用的预处理绘制项
    │   └── WireframePass        # 线框绘制 pass
    ├── pipelines
    │   └── Scene3DPipelineFactory # 新 3D 路径专用 pipeline 工厂
    ├── scene
    │   ├── RenderCamera         # 渲染用相机矩阵
    │   ├── RenderObject         # 单个可渲染物体的提交数据
    │   └── RenderScene          # 一帧要绘制的渲染快照
    └── legacy                   # legacy 渲染兼容层
        ├── LegacyFrameContext   # 旧式 acquire / encoder / submit / present 薄兼容层
        ├── LegacyGuiRenderer    # 旧 ImGui 渲染器与 frame 生命周期管理
        ├── LegacyPipeline2D     # 旧 2D 专用 pipeline 工厂
        └── LegacyRenderer3D     # 旧 3D 场景到新 Renderer 的桥接器
```

## 关键调用关系

### 应用主循环

1. `Application` 调用 `WindowContext::PollEvents()`。
2. `Application` 计算 `deltaTime` 并更新当前场景。
3. `Application` 调用 `LegacyGuiRenderer::BeginFrame(...)`，构建 UI，随后 `EndFrame()`。
4. `Application` 调用 `SceneManager::RenderActive(...)`。

### 新 3D 渲染路径

1. `LogicScene` 或 `LegacyRenderer3D` 先整理出 `RenderScene`。
2. `Renderer` 调用 `RenderContext::AcquireSurfaceFrame()`。
3. `Renderer` 创建 `RenderFrame`，附带 encoder、depthView 和 clearColor。
4. `ForwardPass`、`WireframePass`、`GuiPass` 顺序写入同一个 encoder。
5. `Renderer` 调用 `RenderContext::Submit(...)` 和 `RenderContext::Present(...)`。

### legacy 兼容路径

- `Scene2D` 不直接接触 `RenderFrame` 和 pass 体系，只通过 `LegacyFrameContext` 获取旧式帧入口。
- `Scene3DLegacy` 不再维护独立 GPU 生命周期，只通过 `LegacyRenderer3D` 把旧场景对象同步为 `RenderScene`，再委托 `Renderer` 完成绘制。
- `GuiPass` 只负责在新帧系统中调用 `LegacyGuiRenderer::Render(...)`，ImGui frame 的 begin / end 仍由 `Application` 驱动。

## 设计约束

- `WindowContext` 负责窗口和事件循环，`RenderContext` 不再暴露平台生命周期接口。
- `RenderContext` 只提供 GPU 与 surface 原语，不再承担“整帧完成”的组合语义。
- `Scene3DPipelineFactory` 只服务新 3D 路径，`LegacyPipeline2D` 只服务 `Scene2D`。
- legacy 类型都放在 `render/legacy` 或 `scene/legacy` 下，命名上明确其兼容层身份。
