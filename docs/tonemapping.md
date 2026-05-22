当前 tone mapping 的实际流程是：

1. 几何相关 pass 不直接写屏幕，而是先把结果写进 `Renderer` 持有的 `sceneColor` 中间纹理
2. 这张中间纹理的格式是 `RGBA16Float`，也就是 HDR 线性空间颜色；创建位置在 `Renderer::EnsureFrameResources()`
3. `SkyboxPass`、`ForwardOpaquePass`、`PBRPass` 都往这张 `sceneColor` 写颜色，所以 tone mapping 发生在所有主场景颜色合成之后
4. `ToneMapPass::Render()` 会把 `sceneColorView` 绑定成一个 `texture_2d<f32>`，再开一个全屏三角形 render pass，目标是当前 surface texture
5. 真正的 tone mapping 逻辑写在 `shader/scene/scene_tone_map.wgsl`：
   先采样 HDR 线性颜色，再走一套 ACES fitted 曲线，最后做 `LinearToSrgb`
6. 现在曝光参数还是写死的，等价于 `exposure = 1.0`；shader 里目前只是 `let exposed = hdrColor`
7. `ToneMapPass` 写完 surface 之后，`GuiPass` 再在同一个 surface 上用 `LoadOp::Load` 叠加 ImGui，所以 GUI 不参与 tone mapping

可以把它理解成：

`HDR sceneColor (RGBA16Float, offscreen) -> ToneMapPass/ACES -> surface (BGRA8Unorm 或 RGBA8Unorm) -> GuiPass`
