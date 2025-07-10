#include <iostream>

__global__ void hello_from_gpu() {
    printf("Hello, world! from GPU\n");
}

int main() {
    hello_from_gpu<<<1, 1>>>();
    cudaDeviceSynchronize();
    printf("Hello, world! from CPU\n");
    return 0;
}