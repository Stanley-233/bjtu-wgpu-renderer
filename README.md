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

## 键位文档
- 实验1: 使用说明（含完整键位）见：`README-Lab1.md`
- 实验2: 使用说明（含场景与后处理调参）见：`README-Lab2.md`
