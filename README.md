# bjtu_wgpu_renderer
基于 WebGPU 的渲染器

## 编译与运行

### 依赖环境
- CMake
- C++20 编译器（Clang/GCC/MSVC）
- `just`（可选，仅 Web 一键命令）
- Emscripten SDK 3.1.61（仅 Web 构建需要）

### 原生桌面构建（macOS/Linux/Windows）
```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --target bjtu_wgpu_renderer -j 8
./cmake-build-release/bjtu_wgpu_renderer
```

### Web 构建（Emscripten）
方式 A：直接使用 CMake + emcmake
```bash
emcmake cmake -S . -B cmake-build-release-emscripten -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release-emscripten --target bjtu_wgpu_renderer -j 8
cd cmake-build-release-emscripten
python3 -m http.server 8000
```
打开：`http://localhost:8000/bjtu_wgpu_renderer.html`

方式 B：使用 `just` 一键启动
```bash
just dev-web
```

## 配置说明
- 全局配置：`resources/config.toml`
  - `window.width/height`
  - `render.surface_format`
  - `render.max_device_pixel_ratio`（Web 端生效，限制高 DPI 渲染分辨率；建议 1.5~2.0）
  - `Debug.application`
  - `Debug.input`
- 3D 场景配置：`resources/scene3d.toml`
  - 相机投影类型与初始姿态
  - 场景对象、变换、网格来源（OBJ 或内联顶点/索引）

## 架构设计
项目采用分层结构，核心链路为：

`Application -> RenderContext/InputManager/SceneManager -> Scene(2D/3D) -> Renderer/Pipeline -> WebGPU`

### 1. 应用层
- `Application` 负责生命周期：初始化、主循环、事件回调、场景切换
- 将 GLFW 键盘输入分发给 `InputManager`
- 当前支持快捷切换场景：`1`（2D场景）、`2`（Playground）、`3`（Room）

### 2. 渲染层
- `RenderContext`：封装 WebGPU 设备与窗口上下文，负责初始化 `Instance/Adapter/Device/Queue/Surface`，并提供每帧的渲染结果、命令提交入口
- `PipelineLibrary`：集中创建渲染管线，Vertex Layout、Shader 入口、Blend/DepthStencil 与 BindGroupLayout 配置。
- `Renderer3D`：负责 3D 场景的 GPU 同步与渲染
  - `SyncScene`：从场景对象同步 Mesh、Model 矩阵和相机 View/Projection 矩阵，按窗口尺寸准备深度缓冲
  - `RenderFrame`：构建 render pass，绑定 pipeline/buffer/bind group，执行逐对象 `drawIndexed`
- 渲染数据流：`Scene3D -> Renderer3D::SyncScene -> Renderer3D::RenderFrame -> RenderContext::SubmitAndPresent`

### 3. 输入系统
- `InputManager` 维护输入状态（按键与修饰键）并通过 `entt::dispatcher` 分发事件
- 策略类负责**按键 -> 变换事件**的映射：
  - `Transform2DPolicy`
  - `Transform3DPolicy`
  - `CameraMovePolicy`
- 场景通过 sink 接口订阅输入事件，实现输入层解耦

### 4. 场景系统
- `SceneManager` 管理场景注册、激活、更新、渲染
- `Scene2D`：2D 几何与矩阵变换演示
- `LogicScene`：新版 3D 场景基类，负责共享相机输入、世界装配与 RenderScene 提取
- `ScenePlayground / SceneRoom`：两个具体 3D 场景

### 5. 资源与加载层
- `ResourceManager` 提供统一入口：
  - WGSL Shader 加载
  - 2D legacy 文本几何加载
  - OBJ 网格加载
  - TOML 场景加载
- 默认 3D 场景配置在 `resources/scene3d.toml`

## 键位文档

实验1使用说明（含完整键位）见：`README-实验1.md`
