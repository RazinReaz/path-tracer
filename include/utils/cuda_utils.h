#pragma once 

#include <cuda_runtime.h>

#include <iostream>

#define CUDA_CHECK(err)                                                                                 \
    if ((err) != cudaSuccess)                                                                           \
    {                                                                                                   \
        std::cerr << "CUDA Error: " << cudaGetErrorString(err) << " at line " << __LINE__ << " of file " << __FILE__ << std::endl; \
        exit(1);                                                                                        \
    }
