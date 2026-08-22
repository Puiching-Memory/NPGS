#pragma once

#include "xstdafx.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <fast-cpp-csv-parser/csv.h>

#define GLFW_INCLUDE_VULKAN
#define NOMINMAX
// 平台相关的 Vulkan surface 宏，需与 GLFW 后端保持一致：
// Windows -> Win32；Linux(X11) -> Xlib（默认）；Wayland/XCB 可预先定义覆盖
#if defined(_WIN32)
#   ifndef VK_USE_PLATFORM_WIN32_KHR
#       define VK_USE_PLATFORM_WIN32_KHR
#   endif
#elif !defined(VK_USE_PLATFORM_XLIB_KHR) && !defined(VK_USE_PLATFORM_XCB_KHR) && !defined(VK_USE_PLATFORM_WAYLAND_KHR)
#   define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <GLFW/glfw3.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spirv_cross/spirv_reflect.hpp>
#include <stb_image.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
// Vulkan-Hpp 1.3.283+ 才拆分出以下头；旧版本这些内容在 vulkan.hpp 内
#if __has_include(<vulkan/vulkan_format_traits.hpp>) && __has_include(<vulkan/vulkan_to_string.hpp>)
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_to_string.hpp>
#endif

#if defined(_WIN32)
#   include <Windows.h>
#endif

#ifndef _RELEASE
#define NPGS_ENABLE_CONSOLE_LOGGER
#else
#define GLM_FORCE_INLINE
#define NPGS_ENABLE_FILE_LOGGER
#endif // _RELEASE
#include "Engine/Utils/Logger.h"
