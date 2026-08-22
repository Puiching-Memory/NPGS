#pragma once

#include <string>
#include "Engine/Core/Base/Base.h"

_NPGS_BEGIN
_RUNTIME_BEGIN
_ASSET_BEGIN

enum class EAssetType
{
    kBinaryShader, // 二进制着色器程序（不是 SPIR-V）
    kDataTable,    // 数据表
    kFont,         // 字体
    kLanguage,     // 本地化文本
    kModel,        // 模型
    kShader,       // 着色器
    kTexture,      // 纹理
    kUI            // UI 布局
};

std::string GetAssetFullPath(EAssetType Type, const std::string& Filename);

_ASSET_END
_RUNTIME_END
_NPGS_END
