

# learning from
[Drawing triangles using modern opengl](https://learnopengl.com/Getting-started/Hello-Triangle)


# journey
## How to interop cuda with opengl
yikes, I do not remember how exactly I was able to do this, what errors I faced. There were linking errors, MSVC errors, and whatnot. but I pulled through. I decided to write this after I was able to successfully run the code. I will try to write down the steps I took to get this working.

## '`GLM requires CUDA version 7.0 or higher`'
firstly I wanted to see if the kernel could actually use objects. I passed in a glm object but then the errors popped up. The code block that gives the error is at `glm/simd/platform.h`. I tried renaming the extensions of the cpp files to `.cu`, and running them with nvcc but it didn't work. Neither did defining extra preprocessor definitions like `GLM_FORCE_CUDA`.
I fixed it by removing `glm` altogether. Defined a new class by myself that works on the GPU.

## after shooting rays, the triangle is flipped in the y axis
firstly, everything was flipped. `A` went right, `D` went left. pressing `W` made the triangle smaller. but the shrinking was not uniform. i realized that even though the position of the camera was changing, the `topleft` corner vector was not. so, the fov of the rays became larger and the total percentageof the rays that hit the triangle became very low and the triangle appeared small.
then, after fixing that, i noticed that the y axis was still flipped. pageup made me go down even though printing out the position showed y is increasing. so did the mouse movement and everything else.
then, after assigning the color of the pixel according to the y values, I saw that the lower values were darker. So, I learned that **in CUDA, the y values increase from down to up. Not like traditional C indexing where the rows go from top to bottom.** 