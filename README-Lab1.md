# README-实验1
## 启动
默认程序启动后进入 **ScenePlayground**，为**透视**摄像机视角

## 通用键位（场景管理）
- `1`：切换到 Scene2D
- `2`：切换到 ScenePlayground
- `3`：切换到 SceneRoom

## 3D 场景键位（实验1重点）

### 1. 相机控制
- `W / S`：前进 / 后退
- `A / D`：左移 / 右移
- `Q / E`：上移 / 下移
- `C`：切换投影模式（透视 / 正交）

### 2. 物体变换
- `H / J`：X 轴平移
- `U / O`：Y 轴平移
- `I / K`：Z 轴平移

`Shift + (H J U O I K)`：旋转
- `H / J`：绕 X 轴旋转
- `U / O`：绕 Y 轴旋转
- `I / K`：绕 Z 轴旋转

`Alt + (H J U O I K)`（Mac上为`Option`键）：缩放
- `H / J`：X 轴缩小 / 放大
- `U / O`：Y 轴放大 / 缩小
- `I / K`：Z 轴放大 / 缩小

其他操作：
- `R`：重置 3D 物体到场景初始姿态

## Scene2D 键位
- `H / J`：X 轴平移
- `U / O`：Y 轴平移
- `I / K`：逆时针 / 顺时针旋转

`Alt + (H J U O)`：缩放
- `H / J`：X 轴缩小 / 放大
- `U / O`：Y 轴放大 / 缩小

`Ctrl + (H J U O)`：错切(Shear)
- `H / J`：X 向Shear
- `U / O`：Y 向Shear

其它操作：
- `N`：关于 X 轴反转
- `M`：关于 Y 轴反转
- `R`：重置 2D 变换

## 备注
- 所有连续变换都与 `dt`（帧间隔）结合，按住时会持续变化。
- 可通过 `resources/config.toml` 中的 `Debug.input = true` 打开输入调试日志。
