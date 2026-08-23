大概是世界上最好的最大延拓kn时空渲染着色器，在NPGS/Sources/Engine/Shaders/BlackHole_common.glsl
（下面的介绍是我直接从自己的shadertoy搬过来的）
本Shader实现了基于物理的克尔-纽曼时空（带电旋转黑洞）光线追踪。
This shader implements physically based ray-tracing in Kerr-Newman spacetime(charged rotating black hole).

实现了完整的时空拓扑：外视界、内视界、内外能层、奇环，以及反宇宙。
Implements complete spacetime topology: outer/inner event horizons,
outer/inner ergospheres, ringularity, and the antiverse.

实现了完整的克尔-纽曼时空最大延拓解。光线可以穿过内视界进入
全新的时空域（白洞区域），并最终抵达另一个宇宙。
Implements the complete maximal extension of Kerr-Newman spacetime. 
Rays can traverse the inner horizon into new spacetime domains (white hole regions) 
and eventually emerge into another  universe.

实现了基于物理的偏振（极化）模拟。通过在弯曲时空中计算
Walker-Penrose常数，精确模拟吸积盘发光在强引力场下的偏振态演化。
Implements physically-based polarization simulation. By calculating 
Walker-Penrose constants in curved spacetime, it accurately simulates the 
evolution of polarization states from the accretion disk under strong gravity.

支持裸奇点。支持切换静态观者、自由落体观者以及自定义坐标速度观者。
Supports naked singularities. Supports Static, Free-falling, and Custom-velocity observers.

完全拟真广义相对论效应，包含吸积盘、相对论喷流、引力透镜、
多普勒频移、引力红移以及热浪折射。
Fully simulates GR effects: accretion disk, relativistic jets, gravitational
lensing, Doppler shift, gravitational redshift, and heat haze refraction.


完整可运行的在x64里那个压缩包

---

## 构建（Windows / Linux）

项目使用 **CMake** 作为跨平台构建系统（已移除 Visual Studio 的 .sln/.vcxproj），
依赖由 **vcpkg**（manifest 模式，见 `NPGS/vcpkg.json`）统一管理，C++23 标准。

### 前置要求

C/C++ 包依赖全部由项目级 vcpkg 管理；系统只需构建工具、显卡驱动，以及 Linux 上 GLFW 所需的 X11 开发头：

| 依赖                                          | Windows                                               | Linux (Ubuntu/Debian)                                                                |
| --------------------------------------------- | ----------------------------------------------------- | ------------------------------------------------------------------------------------ |
| CMake ≥ 3.23                                  | 安装 CMake                                            | `sudo apt install cmake ninja-build`                                                 |
| 编译器                                        | Visual Studio 2022 或 2026（含 MSVC）                 | `sudo apt install g++`（**GCC ≥ 14**，C++23 `std::println` 需要）                    |
| Python ≥ 3.9                                  | 可使用仓库内嵌版本                                    | `sudo apt install python3`                                                           |
| Vulkan 运行时                                 | 显卡驱动自带（NVIDIA/AMD/Intel 驱动含 Vulkan loader） | `sudo apt install mesa-vulkan-drivers`                                               |
| X11 开发库（仅 Linux，vcpkg 构建 glfw3 需要） | —                                                     | `sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev` |

其余全部依赖（Vulkan 头文件与导入库、Vulkan-Hpp、SPIRV-Cross、VMA、imgui、glfw3、assimp、
boost、spdlog、glslc 着色器编译器(经 shaderc) 等）由 **项目内 vcpkg** 提供：
```text
vcpkg 本体       -> third_party/vcpkg          （bootstrap 脚本自动克隆）
依赖安装产物      -> NPGS/vcpkg_installed       （manifest 模式）
下载缓存         -> third_party/vcpkg/downloads （默认在用户目录，已被收进项目内）
```

### 构建步骤

```bash
# 1.（仅首次）克隆并引导项目内 vcpkg，安装全部依赖
powershell -ExecutionPolicy Bypass -File scripts/bootstrap-vcpkg.ps1   # Windows
./scripts/bootstrap-vcpkg.sh                                            # Linux

# 2. 配置
cmake --preset windows-msvc-vs2022 # Windows · VS 2022
cmake --preset windows-msvc        # Windows · VS 2026
cmake --preset linux-gcc-release   # Linux · GCC 14+

# 3. 构建（会自动编译 GLSL 着色器到 NPGS/Assets/Shaders）
cmake --build --preset windows-msvc-vs2022-release # Windows · VS 2022
cmake --build --preset windows-msvc-release        # Windows · VS 2026
cmake --build --preset linux-gcc-release           # Linux

# 4. 测试
ctest --test-dir build/windows-vs2022 -C Release --output-on-failure
ctest --test-dir build/windows -C Release --output-on-failure
ctest --test-dir build/linux-release --output-on-failure

# 5. 运行（资源路径自动探测，可在任意目录运行）
./build/windows-vs2022/bin/NPGS.exe
./build/windows/bin/NPGS.exe      # 以实际输出目录为准
./build/linux-release/bin/NPGS
```

- 出包版本（文件日志，等价旧 VS 的 Package 配置）：先运行 `cmake --preset windows-msvc-package`，再运行 `cmake --build --preset windows-msvc-package-release`；完整产物位于 `build/windows-package/bin`。
- 标准归档包：完成 Package 构建后运行 `cpack --config build/windows-package/CPackConfig.cmake -C Release`；Linux 使用对应构建目录的 `CPackConfig.cmake`。归档包含可执行文件、运行库、Assets、LICENSE 与 README。
- 直接配置时必须显式传入 vcpkg toolchain，例如：`cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake`。
- 资产根目录可通过环境变量 `NPGS_ASSET_ROOT` 显式指定（默认从可执行文件目录或当前目录向上查找 `Assets/`）。

> 提示1：`NPGS/Assets/Shaders/*.spv` 不入库，首次构建时由 `Tools/ShaderCompiler/CompileShaders.py` 自动生成（glslc 由 shaderc port 提供）；缺少 Python/glslc 或任一着色器编译失败都会终止构建。仅在明确不需要可运行产物时使用 `-DNPGS_BUILD_SHADERS=OFF`。
> 提示2：bootstrap 脚本会将 vcpkg 本体与 ports 一起固定到 `NPGS/vcpkg.json` 的 `builtin-baseline`。更新依赖前请在独立分支运行 `vcpkg x-update-baseline`，并通过两平台 CI。
> 提示3：如需清理，删除整个 `third_party/vcpkg`、`NPGS/vcpkg_installed` 与 `build/` 即可，不残留任何系统级安装。

---

操作：首先shift关输入法（笑
然后，绕转模式下wasd是横向和纵向环绕，鼠标拖动也是。
此外，绕转模式下鼠标右键拖动是偏头，中键回正（只在绕转模式）
自由模式下ws前后   ad左右  rf上下   qe滚转   鼠标转视角 T切换模式 G使得相机沿测地线移动 H隐藏ui o……（）
第三个左侧按钮可以输指令，tp，格式是/tp x y z t ux uy uz
