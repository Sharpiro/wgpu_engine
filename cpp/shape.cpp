#include "./shape.hpp"
#include <cstdint>
#include <stdexcept>

/* Column-major for WGSL */

Vec4 vec4f() {
    return {0.0, 0.0, 0.0, 0.0};
}

Mat4 mat4() {
    return {{
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }};
}

Vec4 mat_multiply(const Mat4 matrix, const Vec4 vec) {
    auto vec_result = vec4f();
    for (size_t i = 0; i < matrix.size(); i++) {
        Vec4 curr_mat_vec = matrix[i];
        for (size_t j = 0; j < curr_mat_vec.size(); j++) {
            auto curr_mat_vec_val = curr_mat_vec[j];
            auto curr_point_val = vec[j];
            auto mult_result = curr_mat_vec_val * curr_point_val;
            vec_result[i] += mult_result;
        }
    }
    return vec_result;
}

Mat4 translate_mat4(const Mat4 &matrix, const Vec3 &translation) {
    /*
     * {{
     *     {1.0, 0.0, 0.0, 0.0},
     *     {0.0, 1.0, 0.0, 0.0},
     *     {0.0, 0.0, 1.0, 0.0},
     *     {t[0], t[1], t[2], 1.0},
     * }};
     */

    auto result = mat4();
    memcpy(&result, &matrix, sizeof(Mat4));
    result[3][0] += translation[0];
    result[3][1] += translation[1];
    result[3][2] += translation[2];
    return result;
}

Mat4 scale_mat4(const Mat4 &matrix, const Vec3 &scale) {
    /*
     * {{
     *     {s[0], 0.0, 0.0, 0.0},
     *     {0.0, s[1], 0.0, 0.0},
     *     {0.0, 0.0, s[2], 0.0},
     *     {0.0, 0.0, 0.0, 1.0},
     * }};
     */

    auto result = mat4();
    memcpy(&result, &matrix, sizeof(Mat4));
    result[0][0] *= scale[0];
    result[1][1] *= scale[1];
    result[2][2] *= scale[2];
    return result;
}

Mat4 rotate_mat4(const Mat4 &matrix, const Vec2 &rotation) {
    /*
     * {{
     *     {r[0], r[1], 0.0, 0.0},
     *     {-r[1], r[0], 0.0, 0.0},
     *     {0.0, 0.0, 1.0, 0.0},
     *     {0.0, 0.0, 0.0, 1.0},
     * }};
     */

    auto result = mat4();
    auto rotation_matrix = mat4();
    // memcpy(&result, &matrix, sizeof(Mat4));
    rotation_matrix[0][0] = rotation[0];
    rotation_matrix[0][1] = rotation[1];
    rotation_matrix[1][0] = -rotation[1];
    rotation_matrix[1][1] = rotation[0];

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result[i][j] += matrix[i][k] * rotation_matrix[k][j];
            }
        }
    }

    // result[0][0] = rotation[0];
    // result[0][1] = -rotation[1];
    // result[1][0] = rotation[1];
    // result[1][1] = rotation[0];
    return result;
}

Vec4 translate_vec4(const Vec4 point, const Vec3 translation) {
    Mat4 translation_matrix = {{
        {1.0, 0.0, 0.0, translation[0]},
        {0.0, 1.0, 0.0, translation[1]},
        {0.0, 0.0, 1.0, translation[2]},
        {0.0, 0.0, 0.0, 1.0},
    }};
    auto result = mat_multiply(translation_matrix, point);
    return result;
}

Triangle::Triangle(uint32_t triangle_index) {
    vertices[0] = Vertex{
        .pos = {0.0, 0.0, 0.5, 1.0},
        .color = {1.0, 0.0, 0.0, 1.0},
        .triangle_index = triangle_index
    };
    vertices[1] = Vertex{
        .pos = {0.0, 1.0, 0.5, 1.0},
        .color = {0.0, 1.0, 0.0, 1.0},
        .triangle_index = triangle_index
    };
    vertices[2] = Vertex{
        .pos = {-1.0, 0.0, 0.5, 1.0},
        .color = {0.0, 0.0, 1.0, 1.0},
        .triangle_index = triangle_index
    };
};

Triangle Triangle::from(Mat4 model_matrix) {
    // auto triangle = Triangle::get_default();
    throw runtime_error("unimp");
}

void Triangle::translate(Vec3 translation) {
    this->vertices[0].pos = translate_vec4(this->vertices[0].pos, translation);
    this->vertices[1].pos = translate_vec4(this->vertices[1].pos, translation);
    this->vertices[2].pos = translate_vec4(this->vertices[2].pos, translation);
}
