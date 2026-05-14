从输入到移动的实际流程是：

1. GLFW 回调进 Application::HandleKey()，如果 GUI 没有抢键盘，就转给 InputManager
   见 src/app/Application.cpp:130
2. InputManager::EmitKeyEvent() 先更新 InputState，然后按顺序跑各个 policy
   AppHotkeyPolicy -> CameraMovePolicy -> Transform2DPolicy -> Transform3DPolicy
   见 src/input/InputManager.cpp:73
3. CameraMovePolicy 不直接移动相机，它只是把 W/A/S/D/Q/E 汇总成一个 3 轴输入 CameraMoveInputEvent {forward, right, up}，通过 InputEventBus 发出去
   见 src/input/policies/CameraMovePolicy.h:26
4. SceneManager 在切换 active scene 时，负责给当前 scene 注册/反注册输入 sink，所以只有激活场景会收到这个事件
   见 src/scene/SceneManager.cpp:18
5. LogicScene 订阅 CameraMoveInputEvent，但事件回调只做一件事：把输入缓存到 m_moveForward / m_moveRight / m_moveUp
   见 src/scene/LogicScene.cpp:148
6. 真正的相机位移发生在 LogicScene::Update(dt)：
   它从 World 找主相机 entity，拿到 CameraComponent.camera，按 position/target/up 算出 forward/right/worldUp，然后依据缓存输入和 dt 算增量，再 SetPose(position + delta,
   target + delta, up)
   见 src/scene/LogicScene.cpp:103
7. 渲染时 LogicScene::BuildRenderScene() 再从同一个主相机取 View/Projection 填进 RenderScene
   见 src/scene/LogicScene.cpp:162