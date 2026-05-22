当前 Cubemap / 天空盒的实际流程是：

1. `LogicScene` 初始化时先通过 `AssetServer::LoadHdrImage()` 加载 `grasslands_sunset_2k.hdr`，拿到一个 `HdrImageAsset`
2. 这个 HDR 资产不是直接拿来采样天空盒，而是先交给 `RenderContext` 里的 `EnvironmentMapCache`
3. `EnvironmentMapCache` 会先把 HDR equirect 图上传成一张 2D `RGBA16Float` 纹理
4. 然后它再创建一张 `512 x 512 x 6` 的 `RGBA16Float` cubemap 纹理，并创建两个 view：
   一个 `2DArray view` 给 compute shader 写入，一个 `Cube view` 给后面的 skybox pass 采样
5. 真正的 equirect -> cubemap 转换在 `shader/compute/equirect_to_cubemap.wgsl` 里做：
   每个 compute invocation 根据 `faceIndex + 像素坐标` 还原方向向量，再映射回 equirect UV 采样 HDR 图
6. 这一步不是每帧都算；`EnvironmentMapCache` 按 `sourcePath + faceSize` 做 GPU 级缓存，所以同一张 HDR 只会在首次需要时预计算一次
7. 渲染时 `Renderer` 从 `RenderScene.skybox` 拿到 skybox 描述，再从 `AssetServer` 取出对应 `HdrImageAsset`，最后从 `EnvironmentMapCache` 拿到已经准备好的 cubemap GPU 资源
8. `SkyboxPass` 不画一个真实 cube mesh，而是画一个全屏三角形；在 fragment shader 里通过 `inverseProjection + inverseViewRotation` 从屏幕位置反推世界方向，再采样 `texture_cube<f32>`
9. Skybox 采样结果会先写进 HDR 的 `sceneColor` 中间纹理，最后和场景其它颜色一起交给 `ToneMapPass`

可以把它理解成：

`HDR equirect asset -> GPU 2D HDR texture -> compute shader 预计算 cubemap -> SkyboxPass 采样 cubemap -> HDR sceneColor`
