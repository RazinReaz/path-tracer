#pragma once 

#include <cuda_runtime.h>

#include <iostream>

#define CUDA_CHECK(err) do { \
    cudaError_t err_ = (err); \
    if (err_ != cudaSuccess) { \
        std::cerr << "CUDA Error: " << cudaGetErrorString(err_) \
                  << " at line " << __LINE__ \
                  << " of file " << __FILE__ << std::endl; \
        exit(1); \
    } \
} while(0)