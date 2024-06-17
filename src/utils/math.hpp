#pragma once
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <immintrin.h>

#define load256 _mm256_load_ps
#define store256 _mm256_store_ps

inline void glm_mat4_mul_avx(glm::mat4 const& m1, glm::mat4 const& m2, glm::mat4& dst) {
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

    store256(&dst[0][0], _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(y2, y6), _mm256_mul_ps(y3, y7)),
                                       _mm256_add_ps(_mm256_mul_ps(y4, y8), _mm256_mul_ps(y5, y9))));

    /* n n n n i i i i */
    /* p p p p k k k k */
    /* m m m m j j j j */
    /* o o o o l l l l */
    y6 = _mm256_permutevar_ps(y1, _mm256_set_epi32(1, 1, 1, 1, 0, 0, 0, 0));
    y7 = _mm256_permutevar_ps(y1, _mm256_set_epi32(3, 3, 3, 3, 2, 2, 2, 2));
    y8 = _mm256_permutevar_ps(y1, _mm256_set_epi32(0, 0, 0, 0, 1, 1, 1, 1));
    y9 = _mm256_permutevar_ps(y1, _mm256_set_epi32(2, 2, 2, 2, 3, 3, 3, 3));

    store256(&dst[2][0], _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(y2, y6), _mm256_mul_ps(y3, y7)),
                                       _mm256_add_ps(_mm256_mul_ps(y4, y8), _mm256_mul_ps(y5, y9))));
}

inline void fast_mat4_mul(glm::mat4 const& m1, glm::mat4 const& m2, glm::mat4& dst) {
    if constexpr (0) {
        dst = m1 * m2;
    } else {
        glm_mat4_mul_avx(m1, m2, dst);
    }
}

inline void fast_t_matrix(glm::vec3 const& translation, glm::mat4& trs) {
    if constexpr (0) {
        trs[3][0] = translation.x;
        trs[3][1] = translation.y;
        trs[3][2] = translation.z;
    } else {
        memcpy(&trs[3][0], &translation, sizeof(glm::vec3));
    }
}

inline void fast_r_matrix(glm::quat const& rotation, glm::mat4& trs) {
    glm::mat3 rotation_3x3 = glm::toMat3(rotation);
    if constexpr (0) {
        trs[0][0] = rotation_3x3[0][0];
        trs[0][1] = rotation_3x3[0][1];
        trs[0][2] = rotation_3x3[0][2];
        trs[1][0] = rotation_3x3[1][0];
        trs[1][1] = rotation_3x3[1][1];
        trs[1][2] = rotation_3x3[1][2];
        trs[2][0] = rotation_3x3[2][0];
        trs[2][1] = rotation_3x3[2][1];
        trs[2][2] = rotation_3x3[2][2];
    } else if constexpr (1) {
        memcpy(&trs[0][0], &rotation_3x3[0][0], sizeof(glm::vec3));
        memcpy(&trs[1][0], &rotation_3x3[1][0], sizeof(glm::vec3));
        memcpy(&trs[2][0], &rotation_3x3[2][0], sizeof(glm::vec3));
    } else {
        memcpy(&trs[0][0], &rotation_3x3[0][0], sizeof(glm::vec3) * 3);
    }
}

inline void fast_s_matrix(glm::vec3 const& scale, glm::mat4& trs) {
    trs[0][0] *= scale.x;
    trs[1][1] *= scale.y;
    trs[2][2] *= scale.z;
}

inline void fast_trs_matrix(glm::vec3 const& translation, glm::quat const& rotation, glm::vec3 const& scale, glm::mat4& trs) {
    // Manually copying and setting object matrix because multiplying mat4 is slow
    fast_r_matrix(rotation, trs);
    fast_t_matrix(translation, trs);
    fast_s_matrix(scale, trs);

    trs[0][3] = 0;
    trs[1][3] = 0;
    trs[2][3] = 0;
    trs[3][3] = 1;
}

class NewAABB {
  public:
    glm::vec3 max() { return _max; }
    glm::vec3 min() { return _min; }
    glm::vec3 length();
    glm::vec3 centerpoint();

    void update(glm::vec3 const& new_bounds);
    void update(NewAABB const& new_bounds);
    void update(glm::vec4 const& new_bounds);

  private:
    glm::vec3 _max{-MAXFLOAT};
    glm::vec3 _min{MAXFLOAT};
    glm::vec3 _length;
    glm::vec3 _centerpoint;
    bool length_cached = false;
    bool centerpoint_cached = false;
};
