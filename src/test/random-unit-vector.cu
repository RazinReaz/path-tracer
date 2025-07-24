// this code is to test the correctness of the 
// unit vector on hemisphere function 


#include <math.h>
#include <curand_kernel.h>
#include "math/defines.h"
#include "math/vec3.h"
#include "utils/vec3_utils.h"

__global__ void init_rng(curandState *states, unsigned long seed) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    curand_init(seed, idx, 0, &states[idx]);
}

__global__ void test_kernel(vec3 *output, curandState *states, vec3 normal) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    curandState localState = states[idx];
    output[idx] = scatter_along(normal, &localState);
    states[idx] = localState;
}


int main() {
    const int count = 10000;
    vec3 *d_output, *h_output;
    curandState *d_states;

    // Allocate
    cudaMalloc(&d_output, count * sizeof(vec3));
    cudaMalloc(&d_states, count * sizeof(curandState));
    h_output = new vec3[count];

    // Init RNG
    init_rng<<<(count + 255) / 256, 256>>>(d_states, 42);
    cudaDeviceSynchronize();

    vec3 normal(1, 0, 1);
    normal.normalize_self();
    std::cout << normal << std::endl;

    // Run kernel
    test_kernel<<<(count + 255) / 256, 256>>>(d_output, d_states, normal);
    cudaMemcpy(h_output, d_output, count * sizeof(vec3), cudaMemcpyDeviceToHost);

    // Analyze result
    int non_unit = 0, neg = 0;
    float error = 0;
    for (int i = 0; i < count; ++i) {
        float len = h_output[i].length();
        float d = h_output[i].dot(normal);
        float e = fabs(len - 1.0f);
        if (e > 0.01f) {
            non_unit++;
            error += e;
        }
        if(d < 0) {
            neg++;
        }
    }

    printf("Testing the scatter function in CUDA");
    printf("Found %d non unit samples out of %d\n", non_unit, count);
    printf("average error :%f\n", error / count);
    printf("Found %d neg samples out of %d\n", neg, count);

    // Cleanup
    cudaFree(d_output);
    cudaFree(d_states);
    delete[] h_output;
    return 0;
}
