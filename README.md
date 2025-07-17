

# learning from
[Drawing triangles using modern opengl](https://learnopengl.com/Getting-started/Hello-Triangle)


# journey
## How to interop cuda with opengl
yikes, I do not remember how exactly I was able to do this, what errors I faced. There were linking errors, MSVC errors, and whatnot. but I pulled through. I decided to write this after I was able to successfully run the code. I will try to write down the steps I took to get this working.

## '`GLM requires CUDA version 7.0 or higher`'
firstly I wanted to see if the kernel could actually use objects. I passed in a glm object but then the errors popped up. The code block that gives the error is at `glm/simd/platform.h`. I tried renaming the extensions of the cpp files to `.cu`, and running them with nvcc but it didn't work. Neither did defining extra preprocessor definitions like `GLM_FORCE_CUDA`.
I fixed it by removing `glm` altogether. Defined a new class by myself that works on the GPU.