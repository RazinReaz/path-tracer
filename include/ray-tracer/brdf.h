#pragma once

#include <curand_kernel.h>
#include "math/vec3.h"
#include "utils/vec3_utils.h"
#include "ray-tracer/materials.h"


#ifndef MIN_DIELECTRIC_F0
#define MIN_DIELECTRIC_F0 0.04f
#endif



__device__ __forceinline__ vec3 evalFresnelSchlick(const vec3& F0, const vec3& F90, const float& VdotH) {
    return F0 + (F90 - F0) * powf(1.0f - VdotH, 5.0f);
}

__device__ __forceinline__ vec3 shadowedF90(const vec3& F0) {
    return vec3(1.0f, 1.0f, 1.0f);
}

__device__ __forceinline__ float Smith_G1_GGX(const float& alpha, const float& NdotS, const float& alphaSquared, const float& NdotSSquared) {
	return 2.0f / (sqrt(((alphaSquared * (1.0f - NdotSSquared)) + NdotSSquared) / NdotSSquared) + 1.0f);
}

__device__ __forceinline__ float Smith_G2_Over_G1_Height_Correlated(const float& alpha, const float& alphaSq, const float& NdotL, const float& NdotV) {
    float G1V = Smith_G1_GGX(alpha, NdotV, alphaSq, NdotV * NdotV);
    float G1L = Smith_G1_GGX(alpha, NdotL, alphaSq, NdotL * NdotL);
	return G1L / (G1V + G1L - G1V * G1L);
}

__device__ __forceinline__ float specularSampleWeight(const float& alpha, const float& alphaSq, const float& NdotL, const float& NdotV) {
    return Smith_G2_Over_G1_Height_Correlated(alpha, alphaSq, NdotL, NdotV);
}

// Samples a microfacet normal for the GGX distribution using VNDF method.
// Source: "Sampling the GGX Distribution of Visible Normals" by Heitz
// Source: "Sampling Visible GGX Normals with Spherical Caps" by Dupuy & Benyoub
// Random variables 'u' must be in <0;1) interval
// PDF is 'G1(NdotV) * D'
__device__ __forceinline__ vec3 sampleGGXVNDF(const vec3& Vlocal, const float& alphax, const float& alphay, curandState_t *state) {
    /* 
        The imlementation uses vec2(u, v) to sample the direction. 
        I am only passing one state, that I will use to generate two random uniform numbers
    */
    //! sqrtf and rsqrtf
    vec3 Vh = vec3(Vlocal.x * alphax, Vlocal.y * alphay, Vlocal.z);
    Vh.normalize_self();
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = lensq > 0.0f ? vec3(-Vh.y, Vh.x, 0.0f) * rsqrtf(lensq) : vec3(1.0f, 0.0f, 0.0f);
	vec3 T2 = Vh.cross(T1);

    vec3 t = random_vec_on_disk(state);
    float s = 0.5f * (1.0f + Vh.z);
    float t2 = lerp(sqrtf(1.0f - t.x * t.x), t.y, s);

    vec3 Nh = t.x * T1 + t2 * T2 + sqrtf(fmaxf(0.0f, 1.0f - t.x * t.x - t2 * t2)) * Vh;
    Nh.scale(alphax, alphay, 1.0f);
    if (Nh.z < 0.0f) Nh.z = 0.0f;
    Nh.normalize_self();
    return Nh;
}



// Will calculate the weight and the new ray Direction according to the BRDF of the material
__device__ void evalBRDF(const vec3& N, const vec3& V, const Material& mat, curandState_t *state, vec3& newdir, vec3& weight) {
    /*
        N: surface normal
        V: -ray.direction
        mat: material
        state: random state
        newdir: ray direction after bounce (passed by reference)
        weight: weight of the new ray (passed by reference)
    */
    // parameters calc
    float alpha = mat.roughness * mat.roughness;
    float alphaSq = alpha * alpha;
    vec3 specF0 = lerp(vec3(MIN_DIELECTRIC_F0, MIN_DIELECTRIC_F0, MIN_DIELECTRIC_F0), mat.albedo, mat.metalness);
    
    // convert V to Local space
    vec3 tangent, bitangent, Vlocal, Hlocal, Llocal, Nlocal;
    //! if VdotN < 0.0f then N = -1 * N ?
    orthonormal_basis_frisvad(N, tangent, bitangent);
    Vlocal.setXYZ(V.dot(tangent), V.dot(bitangent), V.dot(N));
    Nlocal.setXYZ(0.0f, 0.0f, 1.0f);


    if (mat.type == MaterialType::DIFFUSE) {   
        Llocal = unit_vec3_on_hemisphere(state);
        Hlocal = sampleGGXVNDF(Vlocal, alpha, alpha, state);
        float VdotH = fmaxf(0.00001f, fminf(1.0f, Vlocal.dot(Hlocal)));
        vec3 refl = mat.albedo * (1 - mat.metalness);
        weight = refl * ( vec3(1.0f, 1.0f, 1.0f) - evalFresnelSchlick(specF0, shadowedF90(specF0), VdotH) );
        newdir = Llocal.x * tangent + Llocal.y * bitangent + Llocal.z * N;
        return;
    } else if (mat.type == MaterialType::SPECULAR) {
        if (alpha < 1e-4f) {
            Hlocal.setXYZ(0.0f, 0.0f, 1.0f);
        } else {
            Hlocal = sampleGGXVNDF(Vlocal, alpha, alpha, state);
        }
        Llocal = reflect(-Vlocal, Hlocal);

        float HdotL = fmaxf(0.00001f, fminf(1.0f, Hlocal.dot(Llocal)));
        float NdotL = fmaxf(0.00001f, fminf(1.0f, Nlocal.dot(Llocal)));
        float NdotV = fmaxf(0.00001f, fminf(1.0f, Nlocal.dot(Vlocal)));

        vec3 F = evalFresnelSchlick(specF0, shadowedF90(specF0), HdotL);
        weight = F * specularSampleWeight(alpha, alphaSq, NdotL, NdotV);
        newdir = Llocal.x * tangent + Llocal.y * bitangent + Llocal.z * N;
        return;
    }
}