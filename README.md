# Real Time Path Tracer using CUDA and OpenGL




# Setup and Running (Windows/MSVC + CUDA)
**THIS SECTION IS WRITTEN SOLELY BY AI**

## Requirements
- **NVIDIA CUDA Toolkit**: 11.x or newer installed and on PATH (for `nvcc`).
- **MSVC (Visual Studio Build Tools)** with C++ toolchain (for `cl`).
- **GLFW** prebuilt lib provided in `dependencies/lib` and headers in `dependencies/include` (already in repo).
- **glad** source is included (`src/glad.c`).

## Build
- Open a Developer Command Prompt for VS (x64) so `cl` is available.
- Ensure `nvcc` is accessible (`nvcc --version`).
- From the project root, run:

```bash
make bin/main.run
```

This compiles `src/main.cu` and links with OpenGL/GLFW.

### Run
```bash
make run-main
```

or run the produced binary:

```bash
bin/main.run
```

## One-time automated setup

### Windows (PowerShell)
Run:
```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup.ps1
```

### Linux/macOS (sh)
Run:
```sh
sh scripts/setup.sh
```

Both scripts will:
- Create missing directories (`dependencies/include`, `dependencies/lib`, `assets/models`, `logs`, `bin`, `obj`).
- Download header-only deps: `stb_image.h`, `stb_image_write.h`, `tiny_obj_loader.h`, `tiny_gltf.h`, `json.hpp` into `dependencies/include`.
- Add a note under `assets/models/` describing where to place models.
- Attempt GLFW setup (Windows auto-fetches prebuilt `glfw3.lib`; Linux/macOS will prompt to install via package manager).

## Gitignored content you must supply
The repository’s `.gitignore` excludes several directories/files, so a fresh clone may be missing them. Create/populate as follows:

- `dependencies/` (ignored)
  - Create `dependencies/include` and `dependencies/lib`.
  - Place headers for third-party libs under `dependencies/include` (e.g., `GLFW/`, `stb_image.h`, `stb_image_write.h`, `tiny_obj_loader.h`, `tiny_gltf.h`, etc.).
  - Place `glfw3.lib` in `dependencies/lib` (prebuilt Windows x64 from GLFW release).
  - If you already see these in your local checkout, you can skip this step; otherwise, populate them as above.

- `assets/models/` (ignored)
  - Add your OBJ/GLTF models under `assets/models/obj` or `assets/models/gltf` matching expected paths in the code (default in `src/main.cu` uses `assets/models/obj/bunny-pbr-small/`).
  - You can change the model paths in code to point to models you have locally.

- `logs/` (ignored)
  - Optional; will be created at runtime when saving performance logs/screenshots. If it doesn’t exist, create `logs/` or ensure the app has permission to create it.

- `bin/` and `obj/` (ignored)
  - Build and object output directories. The Makefile creates these automatically when needed.

Tip: If building fails with missing includes or libraries, double-check the `dependencies/` structure and contents.

## Other Targets
- `make bin/6.pbr.run` and `make run-6.pbr` to build/run the original demo.
- Predefined test/benchmark targets exist under `bin/test/*.run` and `bin/benchmark/*.run` with matching `run-*` helpers.

## Notes
- Assets are expected under `assets/` as committed (OBJ models, shaders, textures).
- The Makefile is Windows-oriented (MSVC + nvcc). For Linux, adapt `GL_LIBS` and compiler settings.