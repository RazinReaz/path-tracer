

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

## Next steps
- Now my program can support multiple triangles. I had this confusion about why the scene had to be a double pointer. I still don't understand it. ChatGPT is not great as a learning tool (or I just can't prompt for shit)
- load from wavefront obj files **DONE**
Today I used `tinyobjloader` to load the obj into the scene.The hardest part was to learn how to put the array of triangles and the scene data pointers to the gpu. difficulty 5/10. But I was stuck for a while just because I didn't put a semicolon in.
I loaded the `suzanne` monkey, implemented sky color and drew the surface using color from normals.
I also wrote feature for taking screenshots

## loading the Cornell box
`The cornell box` obj file that I downloaded DOES NOT HAVE VERTEX NORMALS!!! instead, they defined quads and expects me to compute the normal. How do I know if they are oriented in counter clockwise direcction or not?? I think this a valid breakdown I am going through. Nevertheless, I (or chatGPT, although incorrectly) wrote the support for those kinds of files. There was a problem with the faces having gradient colors (weird), but I was not assigning the normal to the fourth vertex. assigning that vertext normal solved it. Now on to the materials.

## Materials
To simulate physically based light scattering, I have to generate uniformly distributed random unit vectors along a hemisphere, or according to sources, where the unit vectors are distributed more along the normal. I learned about cosine weighted sampling but I am trying to learn why it is physically based and why does that calculation generate vectors along the normal more densely.
two steps:
- firstly, we sample a point on a disk.
- Then, we project that point on a unit sphere to get the final vector.
### step 1: 
we could sample two uniform RV `u`, `v`in [-1,1] and assign `r=u` and `theta=2*PI*v` to get polar coordinates $(r, \theta)$. but this will bundle the sampled points near the origin. Bcause r is being sampled uniformly, smaller values of r have the same probability of appearing as larger values of r. For the same change of theta, the points with smaller valued r will be closer together and the points with larger valued r will be further apart.
Therefore, we need to take $r = \sqrt u$, so that the smaller values of `u` are transformed further into the radius of the disk. 
### step 2: 
this is easy. since we already have an $(x, y)$ on the disk, we can get $z$ from $\sqrt{1 - x^2 - y^2}$

Next, since the hemisphere is aligned with the positive z axis, we need to transform that frame onto the surface normal. To do that, we can create an orthonormal basis with the normal as the z axis, and then scale the unit basis with the components of the random unit vector 
$$v_x \times tangent_1 + v_y \times tangent_2 + v_z \times normal$$

# Anti-aliasing
Just put some randomness into the ray direction so that it doesn't shoot the same point across frames



## look out for
 - I am passing the pointer to the global camera object and accessing it in each thread. is that wasteful? 
 ChatGPT said, since the camera is changing, I should not use `__constant__` or pass the pointer. dereferencing pointer takes up time.
 - if I am determining the type of material (metal, lambertian, dielectric) from the `.mtl` file values, then how do I determine the albedo? is it the ambient color or is it the diffuse color?