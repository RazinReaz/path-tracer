# Windows setup script to bootstrap dependencies and folders for this project
# Run from the project root:
#   powershell -ExecutionPolicy Bypass -File scripts/setup.ps1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-CommandAvailable {
    param([Parameter(Mandatory=$true)][string]$Name)
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Ensure-Directory {
    param([Parameter(Mandatory=$true)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType Directory -Force -Path $Path | Out-Null
    }
}

function Download-File {
    param(
        [Parameter(Mandatory=$true)][string]$Url,
        [Parameter(Mandatory=$true)][string]$OutFile
    )
    Write-Host "Downloading: $Url" -ForegroundColor Cyan
    Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $OutFile
}

function Copy-IfExists {
    param([string]$Source, [string]$Destination)
    if (Test-Path -LiteralPath $Source) {
        Copy-Item -Recurse -Force -Path $Source -Destination $Destination
        return $true
    }
    return $false
}

Write-Host "=== raytracing-gpu: Windows setup ===" -ForegroundColor Green

# Basic tool checks
$hasNvcc = Test-CommandAvailable -Name 'nvcc'
if (-not $hasNvcc) {
    Write-Warning "nvcc not found on PATH. Install NVIDIA CUDA Toolkit and reopen this shell."
}
$hasCl = Test-CommandAvailable -Name 'cl'
if (-not $hasCl) {
    Write-Warning "MSVC cl.exe not found. Use 'x64 Native Tools Command Prompt for VS' or install Build Tools."
}

# Create required directories
Ensure-Directory -Path 'dependencies'
Ensure-Directory -Path 'dependencies/include'
Ensure-Directory -Path 'dependencies/lib'
Ensure-Directory -Path 'assets/models'
Ensure-Directory -Path 'assets/shaders'
Ensure-Directory -Path 'assets/textures'
Ensure-Directory -Path 'logs'
Ensure-Directory -Path 'bin'
Ensure-Directory -Path 'obj'

# Prepare temp dir
$tempRoot = Join-Path $env:TEMP 'rtgpu-setup'
if (Test-Path -LiteralPath $tempRoot) { Remove-Item -Recurse -Force $tempRoot }
New-Item -ItemType Directory -Path $tempRoot | Out-Null

# 1) GLFW prebuilt (Windows x64)
try {
    $glfwUrl = 'https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip'
    $glfwZip = Join-Path $tempRoot 'glfw.zip'
    Download-File -Url $glfwUrl -OutFile $glfwZip
    $glfwOut = Join-Path $tempRoot 'glfw'
    Expand-Archive -Path $glfwZip -DestinationPath $glfwOut -Force

    # Try vc2022 first, then vc2019
    $vc2022Lib = Get-ChildItem -Recurse -Filter 'glfw3.lib' -Path (Join-Path $glfwOut '*\lib-vc2022') -ErrorAction SilentlyContinue | Select-Object -First 1
    $vc2019Lib = Get-ChildItem -Recurse -Filter 'glfw3.lib' -Path (Join-Path $glfwOut '*\lib-vc2019') -ErrorAction SilentlyContinue | Select-Object -First 1
    $glfwLib = $vc2022Lib
    if (-not $glfwLib) { $glfwLib = $vc2019Lib }
    if ($glfwLib) {
        Copy-Item -Force $glfwLib.FullName 'dependencies/lib/glfw3.lib'
        Write-Host "Copied glfw3.lib" -ForegroundColor Green
    } else {
        Write-Warning 'Could not locate glfw3.lib in the downloaded archive. You may need to copy it manually.'
    }

    # Copy GLFW headers
    $glfwIncludeDir = Get-ChildItem -Directory -Recurse -Path $glfwOut -Filter 'GLFW' -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($glfwIncludeDir) {
        Ensure-Directory -Path 'dependencies/include/GLFW'
        Copy-Item -Recurse -Force $glfwIncludeDir.FullName 'dependencies/include'
        Write-Host "Copied GLFW headers" -ForegroundColor Green
    } else {
        Write-Warning 'Could not locate GLFW headers in the downloaded archive.'
    }
} catch {
    Write-Warning "GLFW download/setup failed: $($_.Exception.Message)"
}

# 2) Headers: stb, tinyobjloader, tinygltf, nlohmann/json
try {
    $deps = @(
        @{ url='https://raw.githubusercontent.com/nothings/stb/master/stb_image.h'; out='dependencies/include/stb_image.h' },
        @{ url='https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h'; out='dependencies/include/stb_image_write.h' },
        @{ url='https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/release/tiny_obj_loader.h'; out='dependencies/include/tiny_obj_loader.h' },
        @{ url='https://raw.githubusercontent.com/syoyo/tinygltf/master/tiny_gltf.h'; out='dependencies/include/tiny_gltf.h' },
        @{ url='https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp'; out='dependencies/include/json.hpp' }
    )
    foreach ($d in $deps) {
        Download-File -Url $d.url -OutFile $d.out
        Write-Host "Saved $(Split-Path $d.out -Leaf)" -ForegroundColor Green
    }
} catch {
    Write-Warning "Header downloads failed: $($_.Exception.Message)"
}

# 3) Placeholders for models directory
$modelsInfo = @'
assets/models is gitignored. Add your models here matching paths used in the code,
e.g., assets/models/obj/bunny-pbr-small/bunny-pbr-small.obj and its .mtl. You can
change the paths in src/main.cu if you prefer different models.
'@
if (-not (Test-Path 'assets/models/README.txt')) {
    $modelsInfo | Out-File -FilePath 'assets/models/README.txt' -Encoding UTF8 -Force
}

Write-Host "\nSetup complete. Next steps:" -ForegroundColor Green
Write-Host "1) Open 'x64 Native Tools Command Prompt for VS' (so cl is available)."
Write-Host "2) Ensure nvcc is on PATH (nvcc --version)."
Write-Host "3) Build:  make bin/main.run"
Write-Host "4) Run:    make run-main"


