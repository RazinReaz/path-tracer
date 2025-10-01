#pragma once

#include "vec3.h"

class mat4 {
    public:
    float m[16];
    mat4();
    mat4(float diagonal);
    mat4(float data[16]);
    mat4(const mat4 &other);
}

mat4::mat4() {
    for (int i = 0; i < 16; i++) {
        m[i] = 0.0f;
    }
}

mat4::mat4(float diagonal) {
    for (int i = 0; i < 16; i++) {
        m[i] = 0.0f;
    }
    m[0] = diagonal;
    m[5] = diagonal;
    m[10] = diagonal;
    m[15] = diagonal;
}

mat4::mat4(float data[16]) {
    for (int i = 0; i < 16; i++) {
        m[i] = data[i];
    }
}


// Multiply a mat4 with a vec3 (treat vec3 as point, w=1)
__host__ __device__ inline vec3 mat4_mul_point(const mat4 &M, const vec3 &p) {
    vec3 r;
    r.x = M.m[0] * p.x + M.m[4] * p.y + M.m[8]  * p.z + M.m[12];
    r.y = M.m[1] * p.x + M.m[5] * p.y + M.m[9]  * p.z + M.m[13];
    r.z = M.m[2] * p.x + M.m[6] * p.y + M.m[10] * p.z + M.m[14];
    return r;
}

// Multiply a mat4 with a direction vector (w=0, ignores translation)
__host__ __device__ inline vec3 mat4_mul_vector(const mat4 &M, const vec3 &v) {
    vec3 r;
    r.x = M.m[0] * v.x + M.m[4] * v.y + M.m[8]  * v.z;
    r.y = M.m[1] * v.x + M.m[5] * v.y + M.m[9]  * v.z;
    r.z = M.m[2] * v.x + M.m[6] * v.y + M.m[10] * v.z;
    return r;
}

// Multiply two 4x4 matrices
inline mat4 mat4_mul(const mat4 &A, const mat4 &B) {
    mat4 R;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            R.m[c*4 + r] =
                A.m[0*4 + r] * B.m[c*4 + 0] +
                A.m[1*4 + r] * B.m[c*4 + 1] +
                A.m[2*4 + r] * B.m[c*4 + 2] +
                A.m[3*4 + r] * B.m[c*4 + 3];
        }
    }
    return R;
}
