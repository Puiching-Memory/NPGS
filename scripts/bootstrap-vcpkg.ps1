# bootstrap-vcpkg.ps1
# 将 vcpkg 与全部依赖安装到项目内部（不污染系统）：
#   - vcpkg 本体          -> <repo>/third_party/vcpkg
#   - 依赖安装产物        -> <repo>/NPGS/vcpkg_installed
#   - 下载缓存            -> <repo>/third_party/vcpkg/downloads
#
# 用法:
#   powershell -ExecutionPolicy Bypass -File scripts/bootstrap-vcpkg.ps1
#   可选参数: -Triplet x64-windows   -SkipInstall(仅克隆+bootstrap, 不预装)

[CmdletBinding()]
param(
    [string]$Triplet = "x64-windows",
    [switch]$SkipInstall
)

$ErrorActionPreference = "Stop"

$RepoRoot   = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$VcpkgDir   = Join-Path $RepoRoot "third_party\vcpkg"
$CacheDir   = Join-Path $VcpkgDir "downloads"
$Manifest   = Join-Path $RepoRoot "NPGS"
$InstallDir = Join-Path $RepoRoot "NPGS\vcpkg_installed"

Write-Host "[1/4] 获取 vcpkg -> $VcpkgDir" -ForegroundColor Cyan
$SubmoduleOk = $false
if (Test-Path (Join-Path $RepoRoot ".gitmodules")) {
    Push-Location $RepoRoot
    try {
        git submodule update --init --recursive third_party/vcpkg
        if ($LASTEXITCODE -eq 0) { $SubmoduleOk = $true }
    } finally {
        Pop-Location
    }
}
if (-not $SubmoduleOk -and -not (Test-Path (Join-Path $VcpkgDir "vcpkg.exe")) -and -not (Test-Path (Join-Path $VcpkgDir "vcpkg"))) {
    New-Item -ItemType Directory -Path $VcpkgDir -Force | Out-Null
    git clone --depth 1 https://github.com/microsoft/vcpkg.git $VcpkgDir
    if ($LASTEXITCODE -ne 0) { throw "git clone vcpkg 失败，请检查网络" }
} else {
    Write-Host "已存在，跳过克隆"
}

Write-Host "[2/4] bootstrap vcpkg" -ForegroundColor Cyan
Push-Location $VcpkgDir
try {
    & (Join-Path $VcpkgDir "bootstrap-vcpkg.bat") -disableMetrics
    if ($LASTEXITCODE -ne 0) { throw "bootstrap-vcpkg 失败" }
} finally {
    Pop-Location
}

# 下载缓存也放在项目内（默认在 %LOCALAPPDATA%\vcpkg，会污染用户目录）
$env:VCPKG_DOWNLOADS = $CacheDir
Write-Host "[3/4] 下载缓存 -> $CacheDir" -ForegroundColor Cyan
New-Item -ItemType Directory -Path $CacheDir -Force | Out-Null

if (-not $SkipInstall) {
    Write-Host "[4/4] 预装 manifest 依赖 ($Triplet)..." -ForegroundColor Cyan
    & (Join-Path $VcpkgDir "vcpkg.exe") install `
        --x-manifest-root="$Manifest" `
        --x-install-root="$InstallDir" `
        --triplet="$Triplet"
    if ($LASTEXITCODE -ne 0) { throw "vcpkg install 失败" }
} else {
    Write-Host "[4/4] 跳过依赖预装（-SkipInstall）" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "完成！后续构建（示例）：" -ForegroundColor Green
Write-Host "  cmake --preset windows-msvc"
Write-Host "  cmake --build --preset windows-msvc-release"
Write-Host "提示：本脚本已把下载缓存设在项目内；若在其他终端直接构建，请先执行："
Write-Host "  `$env:VCPKG_DOWNLOADS = `"$CacheDir`""
