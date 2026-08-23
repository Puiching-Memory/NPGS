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
$ManifestConfig = Get-Content (Join-Path $Manifest "vcpkg.json") -Raw | ConvertFrom-Json
$VcpkgRevision = $ManifestConfig.'builtin-baseline'

if ($VcpkgRevision -notmatch '^[0-9a-f]{40}$') {
    throw "NPGS/vcpkg.json 缺少有效的 builtin-baseline，无法固定 vcpkg 版本"
}

Write-Host "[1/4] 获取 vcpkg -> $VcpkgDir" -ForegroundColor Cyan
if (-not (Test-Path (Join-Path $VcpkgDir ".git"))) {
    if ((Test-Path $VcpkgDir) -and (Get-ChildItem $VcpkgDir -Force | Select-Object -First 1)) {
        throw "$VcpkgDir 已存在但不是 Git 仓库，请先移走该目录后重试"
    }
    New-Item -ItemType Directory -Path $VcpkgDir -Force | Out-Null
    git -C $VcpkgDir init
    if ($LASTEXITCODE -ne 0) { throw "初始化 vcpkg 仓库失败" }
    git -C $VcpkgDir remote add origin https://github.com/microsoft/vcpkg.git
    if ($LASTEXITCODE -ne 0) { throw "配置 vcpkg 远程仓库失败" }
} else {
    git -C $VcpkgDir remote set-url origin https://github.com/microsoft/vcpkg.git
    if ($LASTEXITCODE -ne 0) { throw "更新 vcpkg 远程仓库失败" }
}

$CurrentRevision = git -C $VcpkgDir rev-parse HEAD 2>$null
if ($LASTEXITCODE -ne 0 -or $CurrentRevision -ne $VcpkgRevision) {
    Write-Host "固定 vcpkg 到 $VcpkgRevision"
    git -C $VcpkgDir fetch --depth 1 origin $VcpkgRevision
    if ($LASTEXITCODE -ne 0) { throw "拉取固定版本的 vcpkg 失败，请检查网络" }
    git -C $VcpkgDir checkout --detach FETCH_HEAD
    if ($LASTEXITCODE -ne 0) { throw "切换到固定版本的 vcpkg 失败" }
} else {
    Write-Host "vcpkg 已是固定版本 $VcpkgRevision"
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
