cmake_minimum_required(VERSION 3.21)

if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

project(NPGS VERSION 1.0.0 LANGUAGES C CXX)

include(CTest)

# ===============================================================
# 全局设置
# ===============================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 依赖统一安装到项目内 (NPGS/vcpkg_installed)，即使手动配置也不污染系统
set(VCPKG_INSTALLED_DIR "${CMAKE_SOURCE_DIR}/NPGS/vcpkg_installed" CACHE PATH "vcpkg 依赖安装目录(项目内)")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# 可执行文件统一输出到 <build>/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(CFG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CFG} CFG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} "${CMAKE_BINARY_DIR}/bin")
endforeach()

# 打包模式（等价于旧 VS 工程的 Package 配置：文件日志、Release 强制内联）
option(NPGS_PACKAGE "NPGS_PACKAGE 打包版本（文件日志，等价于旧的 Package 配置）" OFF)
option(NPGS_BUILD_SHADERS "构建时将 GLSL 编译为 SPIR-V" ON)
option(NPGS_ENABLE_WARNINGS "启用项目级编译器警告" ON)
option(NPGS_WARNINGS_AS_ERRORS "将项目级编译器警告视为错误" OFF)
option(NPGS_ENABLE_SANITIZERS "启用 AddressSanitizer 与 UndefinedBehaviorSanitizer" OFF)

# ===============================================================
# 第三方依赖
# 默认全部通过项目内 vcpkg manifest 模式安装（见 CMakePresets.json 与
# scripts/bootstrap-vcpkg.*）；不依赖 Vulkan SDK / 系统包管理器。
# 也兼容：现有系统安装的库（手动 cmake 配置时自动探测）。
# ===============================================================
# Vulkan 头文件与导入库由 vcpkg 的 vulkan / vulkan-hpp port 提供；
# 找不到时再回退到系统安装（VULKAN_SDK / 发行版开发包）。
find_package(Vulkan QUIET)
if(NOT Vulkan_FOUND)
    find_path(NPGS_VULKAN_INCLUDE_DIR vulkan/vulkan.hpp)
    find_library(NPGS_VULKAN_LIBRARY NAMES vulkan-1 vulkan)
    if(NPGS_VULKAN_INCLUDE_DIR AND NPGS_VULKAN_LIBRARY)
        add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
        set_target_properties(Vulkan::Vulkan PROPERTIES
            IMPORTED_LOCATION "${NPGS_VULKAN_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${NPGS_VULKAN_INCLUDE_DIR}")
        set(Vulkan_FOUND TRUE)
    else()
        message(FATAL_ERROR
            "未找到 Vulkan。请通过项目内 vcpkg 安装（首次先运行 scripts/bootstrap-vcpkg.*），\n"
            "或安装 Vulkan SDK（Windows，需设置 VULKAN_SDK）/ 系统 Vulkan 开发包（Linux）。")
    endif()
endif()
find_package(Threads REQUIRED)

# imgui（vcpkg 提供 imgui::imgui，含 glfw/vulkan/opengl3 绑定）
find_package(imgui CONFIG QUIET)
if(NOT TARGET imgui::imgui)
    message(FATAL_ERROR
        "未找到 imgui（需要 IMGUI 的 glfw-binding / vulkan-binding 特性）。\n"
        "请通过 vcpkg manifest 模式配置（参见 CMakePresets.json），或安装带 CMake 配置的 imgui 包。")
endif()

# spdlog（vcpkg 提供 spdlog::spdlog）
find_package(spdlog CONFIG QUIET)
if(NOT TARGET spdlog::spdlog)
    message(FATAL_ERROR "未找到 spdlog，请通过 vcpkg 安装（spdlog 依赖 fmt）。")
endif()

# glfw（通常由 imgui::imgui 传递提供；显式查找以直接链接 GLFW API）
find_package(glfw3 CONFIG QUIET)

# ---- 仅头文件的依赖：手动定位 include 目录 ----
find_path(NPGS_GLM_INCLUDE_DIR   glm/glm.hpp)
find_path(NPGS_GLI_INCLUDE_DIR   gli/gli.hpp)
find_path(NPGS_STB_INCLUDE_DIR   stb_image.h)
find_path(NPGS_MAGIC_ENUM_INCLUDE_DIR magic_enum/magic_enum.hpp)
find_path(NPGS_BOOST_INCLUDE_DIR boost/multiprecision/cpp_int.hpp)
find_path(NPGS_CSV_INCLUDE_DIR   fast-cpp-csv-parser/csv.h)
find_path(NPGS_ASSIMP_INCLUDE_DIR assimp/Importer.hpp)
find_path(NPGS_SPIRV_CROSS_INCLUDE_DIR spirv_cross/spirv_reflect.hpp)
find_path(NPGS_VMA_INCLUDE_DIR   vma/vk_mem_alloc.h)
if(NOT NPGS_VMA_INCLUDE_DIR)
    find_path(NPGS_VMA_INCLUDE_DIR vk_mem_alloc.h)
endif()

# ---- SPIRV-Cross 库（Vulkan SDK / vcpkg / 系统包 均可，名称兼容 debug(d) 后缀） ----
find_library(NPGS_SPIRV_CROSS_CORE     NAMES spirv-cross-core spirv-cross-cored)
find_library(NPGS_SPIRV_CROSS_GLSL     NAMES spirv-cross-glsl spirv-cross-glsld)
find_library(NPGS_SPIRV_CROSS_CPP      NAMES spirv-cross-cpp spirv-cross-cppd)
find_library(NPGS_SPIRV_CROSS_REFLECT  NAMES spirv-cross-reflect spirv-cross-reflectd)

# ---- 缺失依赖的统一检查 ----
set(NPGS_MISSING_DEPS "")
macro(_npgs_require_hdr NAME VAR HINT)
    if(NOT ${VAR})
        string(APPEND NPGS_MISSING_DEPS "  - ${NAME} (${HINT})\n")
    endif()
endmacro()
macro(_npgs_require_lib NAME VAR HINT)
    if(NOT ${VAR})
        string(APPEND NPGS_MISSING_DEPS "  - ${NAME} (${HINT})\n")
    endif()
endmacro()

_npgs_require_hdr("GLM"                  NPGS_GLM_INCLUDE_DIR          "glm/glm.hpp")
_npgs_require_hdr("GLI"                  NPGS_GLI_INCLUDE_DIR          "gli/gli.hpp")
_npgs_require_hdr("STB"                  NPGS_STB_INCLUDE_DIR          "stb_image.h")
_npgs_require_hdr("MAGIC_ENUM"           NPGS_MAGIC_ENUM_INCLUDE_DIR  "magic_enum/magic_enum.hpp")
_npgs_require_hdr("BOOST_MULTIPRECISION" NPGS_BOOST_INCLUDE_DIR       "boost/multiprecision/cpp_int.hpp")
_npgs_require_hdr("FAST_CPP_CSV_PARSER"  NPGS_CSV_INCLUDE_DIR         "fast-cpp-csv-parser/csv.h")
_npgs_require_hdr("ASSIMP"               NPGS_ASSIMP_INCLUDE_DIR      "assimp/Importer.hpp")
_npgs_require_hdr("SPIRV_CROSS"          NPGS_SPIRV_CROSS_INCLUDE_DIR "spirv_cross/spirv_reflect.hpp")
_npgs_require_hdr("VMA"                  NPGS_VMA_INCLUDE_DIR         "vma/vk_mem_alloc.h")
_npgs_require_lib("SPIRV_CROSS_CORE"     NPGS_SPIRV_CROSS_CORE        "spirv-cross-core")
_npgs_require_lib("SPIRV_CROSS_GLSL"     NPGS_SPIRV_CROSS_GLSL        "spirv-cross-glsl")
_npgs_require_lib("SPIRV_CROSS_CPP"      NPGS_SPIRV_CROSS_CPP         "spirv-cross-cpp")
_npgs_require_lib("SPIRV_CROSS_REFLECT"  NPGS_SPIRV_CROSS_REFLECT     "spirv-cross-reflect")

if(NPGS_MISSING_DEPS)
    message(FATAL_ERROR
        "缺失以下第三方依赖，请通过项目内 vcpkg 安装（首次先运行 scripts/bootstrap-vcpkg.*）：\n${NPGS_MISSING_DEPS}")
endif()

# ===============================================================
# NPGS 目标
# ===============================================================
set(NPGS_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/AssetLoaders/AssetManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/AssetLoaders/GetAssetFullPath.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/AssetLoaders/Shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/AssetLoaders/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/Graphics/Renderers/PipelineManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/Graphics/Vulkan/Context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/Graphics/Vulkan/Core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/Graphics/Vulkan/ExtFunctionsImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/Graphics/Vulkan/Resources.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/Graphics/Vulkan/ShaderResourceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/Graphics/Vulkan/Wrappers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/Graphics/Vulkan/VulkanUIRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Runtime/Threads/ThreadPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/Generators/CivilizationGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/Generators/OrbitalGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/Generators/StellarGenerator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/Spatial/Camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/CelestialInfoPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/CinematicInfoPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/CollapsedMainButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/GlobalTooltip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/HackerTextHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/InputField.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/LogPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/NeuralMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/NeuralParticleView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/PulsarButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/ScreenVignette.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/TechBorderPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/TechButton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/TechInteractivePanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/TechText.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/components/TimeControlPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/ScreenManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/Screens/GameScreen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/Screens/MainMenuScreen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/UILoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/UITool/AstroDataBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/ui_framework.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/System/UI/I18n/I18nManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Types/Entries/Astro/CelestialObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Types/Entries/Astro/Planet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Types/Entries/Astro/Star.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Types/Entries/Astro/StellarSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Types/Entries/NpgsObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Types/Properties/Intelli/Artifact.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Types/Properties/Intelli/Civilization.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Core/Types/Properties/StellarClass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Utils/Logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Utils/TooolfuncForStarMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Engine/Utils/Utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/ExternalImpl/stb_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/ExternalImpl/vma_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Program/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Program/Universe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/stdafx.cpp
)

add_library(NPGSCore STATIC ${NPGS_CORE_SOURCES})
add_library(NPGS::Core ALIAS NPGSCore)
add_executable(NPGS
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/Program/main.cpp)

target_include_directories(NPGSCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources
    ${NPGS_VULKAN_INCLUDE_DIR}
    ${NPGS_GLM_INCLUDE_DIR}
    ${NPGS_GLI_INCLUDE_DIR}
    ${NPGS_STB_INCLUDE_DIR}
    ${NPGS_MAGIC_ENUM_INCLUDE_DIR}
    ${NPGS_BOOST_INCLUDE_DIR}
    ${NPGS_CSV_INCLUDE_DIR}
    ${NPGS_ASSIMP_INCLUDE_DIR}
    ${NPGS_SPIRV_CROSS_INCLUDE_DIR}
    ${NPGS_VMA_INCLUDE_DIR}
)

# 预编译头仍按目标隔离，避免应用入口与核心库互相隐藏编译依赖。
target_precompile_headers(NPGSCore PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/stdafx.h")
target_precompile_headers(NPGS PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Sources/stdafx.h")

function(npgs_configure_cpp_target TARGET_NAME)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<BOOL:${NPGS_PACKAGE}>:_RELEASE>
    )
    if(MSVC)
        target_compile_definitions(${TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
        # /utf-8：源码含中文注释；/bigobj：大型翻译单元；/permissive-：标准一致性。
        target_compile_options(${TARGET_NAME} PRIVATE /utf-8 /bigobj /permissive-)
    endif()

    if(NPGS_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(${TARGET_NAME} PRIVATE /W4)
        else()
            target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()

    if(NPGS_WARNINGS_AS_ERRORS)
        if(MSVC)
            target_compile_options(${TARGET_NAME} PRIVATE /WX)
        else()
            target_compile_options(${TARGET_NAME} PRIVATE -Werror)
        endif()
    endif()

    if(NPGS_ENABLE_SANITIZERS)
        if(MSVC)
            target_compile_options(${TARGET_NAME} PRIVATE /fsanitize=address)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${TARGET_NAME} PRIVATE -fsanitize=address,undefined)
        else()
            message(FATAL_ERROR "当前编译器不支持 NPGS_ENABLE_SANITIZERS")
        endif()
    endif()
endfunction()

npgs_configure_cpp_target(NPGSCore)
npgs_configure_cpp_target(NPGS)

target_link_libraries(NPGSCore PUBLIC
    Vulkan::Vulkan
    Threads::Threads
    imgui::imgui
    spdlog::spdlog
    ${NPGS_SPIRV_CROSS_CPP}
    ${NPGS_SPIRV_CROSS_GLSL}
    ${NPGS_SPIRV_CROSS_REFLECT}
    ${NPGS_SPIRV_CROSS_CORE}
)
if(TARGET glfw)
    target_link_libraries(NPGSCore PUBLIC glfw)
endif()
target_link_libraries(NPGS PRIVATE NPGS::Core)

# Package 构建生成可脱离源码仓库运行的目录布局。
if(NPGS_PACKAGE)
    add_custom_command(TARGET NPGS POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Assets"
                "$<TARGET_FILE_DIR:NPGS>/Assets"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "$<TARGET_FILE_DIR:NPGS>/Cache/Shaders"
        COMMENT "Copying NPGS runtime assets"
        VERBATIM)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Cache")
        add_custom_command(TARGET NPGS POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Cache"
                    "$<TARGET_FILE_DIR:NPGS>/Cache"
            COMMENT "Copying NPGS runtime cache"
            VERBATIM)
    endif()
endif()

# ===============================================================
# 着色器编译（GLSL -> SPIR-V）
# Assets/Shaders 下的 .spv 不入库，需要在构建时生成
# ===============================================================
if(NPGS_BUILD_SHADERS)
    if(WIN32 AND NOT Python3_EXECUTABLE AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Tools/ShaderCompiler/python-embed/python.exe")
        set(Python3_EXECUTABLE
            "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Tools/ShaderCompiler/python-embed/python.exe"
            CACHE FILEPATH "Project-local Python interpreter")
    endif()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    find_program(NPGS_GLSLC NAMES glslc glslc.exe
        HINTS
            ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/shaderc
            ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/glslang)
    if(NOT NPGS_GLSLC)
        message(FATAL_ERROR
            "NPGS_BUILD_SHADERS=ON，但未找到 glslc。请先运行 scripts/bootstrap-vcpkg.*。")
    endif()

    add_custom_target(npgs-shaders
        COMMAND "${CMAKE_COMMAND}" -E env "NPGS_GLSLC=${NPGS_GLSLC}"
                "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Tools/ShaderCompiler/CompileShaders.py"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Tools/ShaderCompiler"
        USES_TERMINAL
        COMMENT "Compiling GLSL shaders (glslc)"
        VERBATIM
    )
    add_dependencies(NPGS npgs-shaders)
    message(STATUS "着色器编译已启用：${NPGS_GLSLC}")
else()
    message(WARNING "NPGS_BUILD_SHADERS=OFF：不会生成运行所需的 SPIR-V 着色器")
endif()

# ===============================================================
# 测试
# ===============================================================
if(BUILD_TESTING)
    if(NOT Python3_Interpreter_FOUND)
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
    endif()
    add_test(
        NAME npgs.shader_compiler
        COMMAND "${Python3_EXECUTABLE}" -m unittest discover
                -s "${CMAKE_CURRENT_SOURCE_DIR}/tests/python"
                -p "test_*.py"
    )
    set_tests_properties(npgs.shader_compiler PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(
        NAME npgs.repository_hygiene
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_repository.py")
    set_tests_properties(npgs.repository_hygiene PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

    add_executable(NPGSCoreTests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/CoreTests.cpp")
    target_link_libraries(NPGSCoreTests PRIVATE NPGS::Core)
    npgs_configure_cpp_target(NPGSCoreTests)
    add_test(
        NAME npgs.core
        COMMAND NPGSCoreTests "${CMAKE_CURRENT_BINARY_DIR}/npgs-core-test.csv")
endif()

# ===============================================================
# 安装与归档发布
# ===============================================================
include(GNUInstallDirs)
install(TARGETS NPGS
    RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*[Ss]ystem32/.*" "^/lib/.*" "^/usr/lib/.*"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/NPGS/Assets/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/Assets")
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    DESTINATION ".")

set(CPACK_PACKAGE_NAME "NPGS")
set(CPACK_PACKAGE_VENDOR "NPGS Contributors")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "NPGS-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
