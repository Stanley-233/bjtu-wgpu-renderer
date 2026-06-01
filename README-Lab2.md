# README-实验2
## 实验说明
本实验对应仓库中的新版 3D 渲染流程，重点围绕以下内容展开：
- 多光源光照：方向光、点光源、聚光灯
- 材质着色：`Blinn-Phong` 与 `PBR`
- 阴影与后处理：阴影贴图、`SSAO`、`SSR`、`DoF`、`Tone Mapping`
- 场景组织与调试：多场景切换、实时调参 GUI

参考实验报告：`计算机图形学+第二次实验报告.pdf`

## 启动
程序默认启动后进入 `ScenePlayground`，实验 2 的主要内容集中在两个 3D 场景：
- `ScenePlayground`：用于观察基础光照、点光源开关、材质模式切换
- `SceneRoom`：用于观察室内场景、聚光灯、后处理和曝光效果

## 场景切换
- `1`：切换到 `Scene2D`（保留的旧场景）
- `2`：切换到 `ScenePlayground`
- `3`：切换到 `SceneRoom`

也可以通过左上角 ImGui 调试面板中的按钮切换场景。

## 3D 场景基础操作
### 相机移动
- `W / S`：前进 / 后退
- `A / D`：左移 / 右移
- `Q / E`：下降 / 上升

### 相机观察
- 按住鼠标右键拖动：旋转视角

## 调试面板说明
实验 2 的大部分功能通过 ImGui 调试面板实时调整。

### General
- `Switch Scene2D / Switch Playground / Switch Room`：切换场景
- `FOV`：调整透视相机视场角
- `Lighting`
  - `Yaw / Pitch`：调整方向光方向
  - `Intensity`：调整方向光强度
  - `Color`：调整方向光颜色
- `Shading`
  - `Blinn-Phong`：切换到传统高光模型
  - `PBR`：切换到基于物理的着色模型
  - `PBR Normal Debug`：查看法线相关调试结果

### ScenePlayground 专属
- `Magenta Point Light`：开关品红色点光源
- `Blue Point Light`：开关蓝色点光源

### SceneRoom 专属
- `Room Spot Light`：开关室内聚光灯

### Postprocessing
- `SSAO`
  - `Enable SSAO`：开关屏幕空间环境光遮蔽
- `SSR`
  - `Enable SSR`：开关屏幕空间反射
  - `SSR Strength`：控制反射强度
  - `SSR Max Distance`：控制最大步进距离
  - `SSR Thickness`：控制深度厚度容差
- `EV`
  - `Exposure Mode`：切换 `Manual EV / Auto Exposure`
  - `Exposure EV`：手动曝光值
- `DoF`
  - `Enable DoF`：开关景深
  - `Focus Distance`：焦点距离
  - `Focus Range`：清晰范围
  - `Max Blur Radius`：最大模糊半径
  - `DoF Debug`：景深调试模式
  - `Debug Plane Thickness`：焦平面调试厚度

## 观察建议
- 在 `ScenePlayground` 中切换 `Blinn-Phong / PBR`，对比不同材质模型的高光与能量表现。
- 开关两个点光源，观察局部补光和阴影层次变化。
- 在 `SceneRoom` 中开启 `SSAO`、`SSR` 和 `DoF`，更容易看到后处理叠加效果。
- 调整方向光 `Yaw / Pitch` 与曝光参数，可明显观察阴影方向和整体亮度变化。

## 备注
- 程序默认进入 `ScenePlayground`。
- `SceneRoom` 在切换进入时会默认使用手动曝光，`Exposure EV = 1.25`。
- 实验 2 主要交互已迁移到 GUI 面板，较少依赖实验 1 那种键盘直接操控物体的方式。
