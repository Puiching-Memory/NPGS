#include "GetAssetFullPath.h"

#include "Engine/Core/Base/Assert.h"

#include <cstdlib>
#include <filesystem>

#if defined(_WIN32)
#   include <Windows.h>
#else
#   include <limits.h>
#   include <unistd.h>
#endif

_NPGS_BEGIN
_RUNTIME_BEGIN
_ASSET_BEGIN

namespace
{
    std::filesystem::path GetExecutableDirectory()
    {
#if defined(_WIN32)
        std::wstring Buffer(MAX_PATH, L'\0');
        const DWORD Length = GetModuleFileNameW(nullptr, Buffer.data(), static_cast<DWORD>(Buffer.size()));
        if (Length == 0)
        {
            return {};
        }
        Buffer.resize(Length);
        return std::filesystem::path(Buffer).parent_path();
#else
        char Buffer[PATH_MAX];
        const ssize_t Length = readlink("/proc/self/exe", Buffer, sizeof(Buffer) - 1);
        if (Length <= 0)
        {
            return {};
        }
        Buffer[Length] = '\0';
        return std::filesystem::path(Buffer).parent_path();
#endif
    }

    bool IsAssetRoot(const std::filesystem::path& Directory)
    {
        std::error_code Error;
        return std::filesystem::is_directory(Directory / "Assets", Error);
    }

    // 从 Directory 开始向上查找包含 Assets 的目录。
    // 兼容两种仓库布局：<root>/Assets 与 <root>/NPGS/Assets。
    std::filesystem::path FindAssetRootFrom(const std::filesystem::path& Directory)
    {
        if (Directory.empty())
        {
            return {};
        }

        std::filesystem::path Current = Directory;
        for (int i = 0; i < 8; ++i)
        {
            if (IsAssetRoot(Current))
            {
                return Current;
            }
            if (IsAssetRoot(Current / "NPGS"))
            {
                return Current / "NPGS";
            }

            const std::filesystem::path Parent = Current.parent_path();
            if (Parent == Current)
            {
                break;
            }
            Current = Parent;
        }
        return {};
    }

    std::filesystem::path FindAssetRoot()
    {
        // 1. 环境变量显式覆盖
        if (const char* Env = std::getenv("NPGS_ASSET_ROOT"))
        {
            return std::filesystem::path(Env);
        }

        // 2. 可执行文件所在目录（及其上级）
        if (std::filesystem::path Root = FindAssetRootFrom(GetExecutableDirectory()); !Root.empty())
        {
            return Root;
        }

        // 3. 当前工作目录（及其上级）
        std::error_code Error;
        const std::filesystem::path Cwd = std::filesystem::current_path(Error);
        if (!Error)
        {
            if (std::filesystem::path Root = FindAssetRootFrom(Cwd); !Root.empty())
            {
                return Root;
            }
        }

        // 4. 回退到当前目录（保持旧行为，让文件打开失败时给出明确错误）
        return Cwd.empty() ? std::filesystem::path(".") : Cwd;
    }
}

std::string GetAssetFullPath(EAssetType Type, const std::string& Filename)
{
    namespace fs = std::filesystem;

    static const fs::path kAssetRoot = FindAssetRoot();

    auto AssetFolderName = [Type]() -> std::string
    {
        switch (Type)
        {
        case EAssetType::kBinaryShader:
            return "Cache/Shaders/";
        case EAssetType::kDataTable:
            return "DataTables/";
        case EAssetType::kFont:
            return "Fonts/";
        case EAssetType::kLanguage:
            return "Lang/";
        case EAssetType::kModel:
            return "Models/";
        case EAssetType::kShader:
            return "Shaders/";
        case EAssetType::kTexture:
            return "Textures/";
        case EAssetType::kUI:
            return "UI/";
        default:
            NpgsAssert(false, "Invalid asset type");
            return "";
        }
    }();

    if (Type == EAssetType::kBinaryShader)
    {
        return (kAssetRoot / "Cache" / "Shaders" / Filename).generic_string();
    }
    return (kAssetRoot / "Assets" / AssetFolderName / Filename).generic_string();
}

_ASSET_END
_RUNTIME_END
_NPGS_END
