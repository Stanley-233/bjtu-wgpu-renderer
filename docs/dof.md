# DoF

## 当前实现

1. `DepthPrepass` 先写 `sceneDepth`
2. `SkyboxPass`、`ForwardOpaquePass`、`PBRPass` 写 HDR `sceneColor`
3. `DofPass` 读取 `sceneDepth + sceneColor`
4. `DofPass` 先输出 `sceneCoc`，再做一次横向 blur 到 `sceneDofPing`，最后做一次纵向 blur 到 `sceneDofColor`
5. `ToneMapPass` 从 `sceneDofColor` 取样；如果 DoF 关闭，则仍直接从 `sceneColor` 取样

## CoC

- 这版使用美术参数，不使用物理相机公式
- 线性深度由 `invProjection` 从深度纹理重建 view-space 位置，再取 `linearDepth = -viewPos.z`
- CoC 公式：
  `signedCoC = clamp((linearDepth - focusDistance) / max(focusRange, 1e-4), -1, 1)`
- `signedCoC < 0` 表示焦平面前，`signedCoC > 0` 表示焦平面后

## Blur 与 Debug

- blur 是可分离的，两次全屏三角形 pass，共享同一套 blur pipeline
- 模糊半径由 `abs(signedCoC) * maxBlurRadiusPixels` 控制
- 为减少前后景串色，blur 只累积与中心 CoC 同号或接近 0 的样本
- Debug 模式不是独立视图，而是在最终 DoF 结果上把焦平面附近颜色混成紫色
