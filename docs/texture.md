我建议你在这个项目里按这个顺序演进：

1. 先把“几何”和“材质”拆开。
   Vertex3D 不再把 color 当唯一颜色来源；RenderObject 增加 material 指针/handle。
2. 给 shader 分两组 uniform/bind group。
   group(0) 放 scene/view/light，group(1) 放 object/model，group(2) 放 material + textures。现在 src/render/pipelines/Scene3DPipelineFactory.cpp:12 只有一个 group，后面会不
   够用。
3. 把“颜色来源”做成统一规则。
   baseColor = texture ? sample(baseColorMap) : baseColorFactor;
   baseColor *= useVertexColor ? vertexColor : 1;
   这样纯色 glTF 的 baseColorFactor、OBJ 顶点色、以后贴图，都能共存。
4. Phong 先单独做一个材质模型，不要一开始写 mega shader。
   推荐按 pipeline key = vertex layout + shading model + render mode 建 pipeline 变体，而不是在一个 fragment shader 里堆很多 if/switch。对你现在这个项目规模，这样最清晰。
5. 灯光也独立抽象。
   不要把光照参数塞进材质。材质回答“表面怎么反应”，灯光回答“场景里有什么光”。Phong 先支持 1 个 directional/point light 就够了，后面再扩。

对应 Cornell Box 这类纯色模型，落地上就是：

- 网格至少要有 position + normal
- 材质只有 baseColorFactor
- Phong shader 用 baseColorFactor 参与漫反射/高光
- 没有 UV、没有贴图也完全成立

一句话总结：把“纯色”视为“没有纹理的材质实例”，而不是视为“另一种模型格式”或“另一条渲染路径”。
如果你愿意，我下一步可以直接按你当前代码结构，给你出一版最小改造清单，甚至直接把 Unlit + Phong 的材质骨架接进去。