#!/usr/bin/env sh
# POSIX-friendly setup script to bootstrap dependencies and folders
# Usage (from project root):
#   sh scripts/setup.sh

set -e

echo "=== raytracing-gpu: setup (sh) ==="

has_cmd() {
  command -v "$1" >/dev/null 2>&1
}

download() {
  url="$1"
  out="$2"
  echo "Downloading: $url"
  if has_cmd curl; then
    curl -L -o "$out" "$url"
  elif has_cmd wget; then
    wget -O "$out" "$url"
  else
    echo "Error: need curl or wget to download $url" >&2
    exit 1
  fi
}

ensure_dir() {
  [ -d "$1" ] || mkdir -p "$1"
}

# Create required directories
ensure_dir dependencies/include
ensure_dir dependencies/lib
ensure_dir assets/models
ensure_dir assets/shaders
ensure_dir assets/textures
ensure_dir logs
ensure_dir bin
ensure_dir obj

# Temp dir
TMPDIR="${TMPDIR:-/tmp}"
WORKDIR="$TMPDIR/rtgpu-setup-$$"
rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"

# 1) GLFW (best-effort)
# - On Linux/macOS, prefer system package managers. We only auto-fetch the prebuilt Windows lib for Git Bash/MSYS.
UNAME_S="$(uname -s 2>/dev/null || echo unknown)"
case "$UNAME_S" in
  MINGW*|MSYS*|CYGWIN*)
    GLFW_ZIP_URL="https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip"
    GLFW_ZIP="$WORKDIR/glfw.zip"
    download "$GLFW_ZIP_URL" "$GLFW_ZIP"
    if has_cmd unzip; then
      unzip -q "$GLFW_ZIP" -d "$WORKDIR/glfw"
      # prefer vc2022 then vc2019
      GLFW_LIB_PATH="$(ls -1 "$WORKDIR"/glfw/*/lib-vc2022/glfw3.lib 2>/dev/null | head -n1)"
      [ -z "$GLFW_LIB_PATH" ] && GLFW_LIB_PATH="$(ls -1 "$WORKDIR"/glfw/*/lib-vc2019/glfw3.lib 2>/dev/null | head -n1)"
      if [ -n "$GLFW_LIB_PATH" ]; then
        cp -f "$GLFW_LIB_PATH" dependencies/lib/glfw3.lib
        echo "Copied glfw3.lib"
      else
        echo "Warning: could not locate glfw3.lib in GLFW archive" >&2
      fi
      GLFW_INC_DIR="$(ls -1d "$WORKDIR"/glfw/*/include/GLFW 2>/dev/null | head -n1)"
      if [ -n "$GLFW_INC_DIR" ]; then
        ensure_dir dependencies/include/GLFW
        cp -rf "$GLFW_INC_DIR" "dependencies/include/.." >/dev/null 2>&1 || true
        echo "Copied GLFW headers"
      else
        echo "Warning: could not locate GLFW headers in GLFW archive" >&2
      fi
    else
      echo "Warning: 'unzip' not found; skipping GLFW auto-setup. Install unzip or set up GLFW manually." >&2
    fi
    ;;
  *)
    echo "Note: On $UNAME_S, install GLFW via your package manager (e.g., Ubuntu: 'sudo apt install libglfw3-dev')." ;;
esac

# 2) Single-header libraries
download "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" dependencies/include/stb_image.h
download "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h" dependencies/include/stb_image_write.h
download "https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/release/tiny_obj_loader.h" dependencies/include/tiny_obj_loader.h
download "https://raw.githubusercontent.com/syoyo/tinygltf/master/tiny_gltf.h" dependencies/include/tiny_gltf.h
download "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp" dependencies/include/json.hpp

# 3) Models note
if [ ! -f assets/models/README.txt ]; then
  cat > assets/models/README.txt <<'EOF'
assets/models is gitignored. Add your models here matching paths used in the code,
e.g., assets/models/obj/bunny-pbr-small/bunny-pbr-small.obj and its .mtl. You can
change the paths in src/main.cu if you prefer different models.
EOF
fi

echo ""
echo "Setup complete. Next steps:"
echo "1) Ensure your compiler toolchain and CUDA (nvcc) are installed."
echo "2) Build:  make bin/main.run"
echo "3) Run:    make run-main"


