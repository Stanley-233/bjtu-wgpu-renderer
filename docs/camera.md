从输入到移动的实际流程是：

1. GLFW 回调进 Application::HandleKey()，如果 GUI 没有抢键盘，就转给 InputManager
2. InputManager::EmitKeyEvent() 先更新 InputState，然后按顺序跑各个 policy
   AppHotkeyPolicy -> CameraMovePolicy -> Transform2DPolicy -> Transform3DPolicy
3. CameraMovePolicy 不直接移动相机，它只是把 W/A/S/D/Q/E 汇总成一个 3 轴输入 CameraMoveInputEvent {forward, right, up}，通过 InputEventBus 发出去
4. SceneManager 在切换 active scene 时，负责给当前 scene 注册/反注册输入 sink，所以只有激活场景会收到这个事件
5. 现在所有新版 3D 场景都继承 LogicScene 基类；基类统一订阅 CameraMoveInputEvent 和 CameraLookInputEvent，并把输入转交给 FreeCameraController
6. 真正的相机位移和视角变化发生在 LogicScene::Update(dt) 里：
   它从 World 找主相机 entity，拿到 CameraComponent.camera，再调用 CameraController::Update(dt, camera)
7. 渲染时 LogicScene::BuildRenderScene() 继续从同一个主相机取 View/Projection 填进 RenderScene，所以 ScenePlayground、SceneRoom 共用同一套相机输入与渲染提交流程
