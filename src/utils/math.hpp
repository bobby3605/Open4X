#pragma once
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include <immintrin.h>

#define load256 _mm256_load_ps
#define store256 _mm256_store_ps

inline void glm_mat4_mul_avx(glm::mat4 const& m1, glm::mat4 const& m2, glm::mat4& dest) {
    // https://github.com/recp/cglm/blob/master/include/cglm/simd/avx/mat4.h
    /* D = R * L (Column-Major) */

    __m256 y0, y1, y2, y3, y4, y5, y6, y7, y8, y9;

    y0 = load256(&m2[0][0]); /* h g f e d c b a */
    y1 = load256(&m2[2][0]); /* p o n m l k j i */

    y2 = load256(&m1[0][0]); /* h g f e d c b a */
    y3 = load256(&m1[2][0]); /* p o n m l k j i */

    /* 0x03: 0b00000011 */
    y4 = _mm256_permute2f128_ps(y2, y2, 0x03); /* d c b a h g f e */
    y5 = _mm256_permute2f128_ps(y3, y3, 0x03); /* l k j i p o n m */

    /* f f f f a a a a */
    /* h h h h c c c c */
    /* e e e e b b b b */
    /* g g g g d d d d */
    y6 = _mm256_permutevar_ps(y0, _mm256_set_epi32(1, 1, 1, 1, 0, 0, 0, 0));
    y7 = _mm256_permutevar_ps(y0, _mm256_set_epi32(3, 3, 3, 3, 2, 2, 2, 2));
    y8 = _mm256_permutevar_ps(y0, _mm256_set_epi32(0, 0, 0, 0, 1, 1, 1, 1));
    y9 = _mm256_permutevar_ps(y0, _mm256_set_epi32(2, 2, 2, 2, 3, 3, 3, 3));

    store256(&dest[0][0], _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(y2, y6), _mm256_mul_ps(y3, y7)),
                                        _mm256_add_ps(_mm256_mul_ps(y4, y8), _mm256_mul_ps(y5, y9))));

    /* n n n n i i i i */
    /* p p p p k k k k */
    /* m m m m j j j j */
    /* o o o o l l l l */
    y6 = _mm256_permutevar_ps(y1, _mm256_set_epi32(1, 1, 1, 1, 0, 0, 0, 0));
    y7 = _mm256_permutevar_ps(y1, _mm256_set_epi32(3, 3, 3, 3, 2, 2, 2, 2));
    y8 = _mm256_permutevar_ps(y1, _mm256_set_epi32(0, 0, 0, 0, 1, 1, 1, 1));
    y9 = _mm256_permutevar_ps(y1, _mm256_set_epi32(2, 2, 2, 2, 3, 3, 3, 3));

    store256(&dest[2][0], _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(y2, y6), _mm256_mul_ps(y3, y7)),
                                        _mm256_add_ps(_mm256_mul_ps(y4, y8), _mm256_mul_ps(y5, y9))));
}

inline void fast_mat4_mul(glm::mat4 const& m1, glm::mat4 const& m2, glm::mat4& dest) { glm_mat4_mul_avx(m1, m2, dest); }
