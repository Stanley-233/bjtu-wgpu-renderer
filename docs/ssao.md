当前 SSAO 的实际流程是：

1. `Renderer` 每帧会先准备两张和 SSAO 直接相关的中间纹理：
   `sceneDepth`（`Depth24Plus`）和 `sceneNormal`（`RGBA16Float`）
2. `DepthPrepass` 先把场景几何深度写进 `sceneDepth`
3. `SceneNormalPass` 再在同样的几何可见性基础上，把世界空间法线写进 `sceneNormal`
4. `SSAOPass` 本身不重新画场景 mesh，它是一个全屏三角形 pass；输入是：
   `sceneDepthView`、`sceneNormalView` 和一份 `SsaoUniformData`
5. `SsaoUniformData` 里现在主要放：
   投影矩阵、逆投影矩阵、viewport 尺寸、采样半径、bias、强度、sampleCount
6. 真正的 SSAO 计算写在 `shader/scene/scene_ssao.wgsl` 里：
   shader 会从 depth + normal 重建局部空间关系，估算环境遮蔽，再输出到 `sceneAo`
7. `sceneAo` 的格式是 `R8Unorm`，本质上是一张单通道 AO 纹理
8. 后面的 `ForwardOpaquePass` 和 `PBRPass` 会把 `sceneAo` 当作输入纹理采样，用它去压暗 ambient 部分
9. 如果关闭 SSAO，`SSAOPass` 仍然会创建 render pass，但会直接把 `sceneAo` 清成 1，相当于“不做遮蔽”

可以把它理解成：

`DepthPrepass -> sceneDepth`

`SceneNormalPass -> sceneNormal`

`sceneDepth + sceneNormal + ssao uniform -> SSAOPass -> sceneAo`

`sceneAo -> ForwardOpaque/PBR ambient lighting`
