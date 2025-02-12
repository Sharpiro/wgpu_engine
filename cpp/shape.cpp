#include "./shape.hpp"
#include <cstring>

/* Column-major for WGSL */

Vec4 vec4f() {
    return {0.0, 0.0, 0.0, 0.0};
}

Mat4 mat4(float d) {
    return {{
        {d, 0.0, 0.0, 0.0},
        {0.0, d, 0.0, 0.0},
        {0.0, 0.0, d, 0.0},
        {0.0, 0.0, 0.0, d},
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

Mat4 mat_multiply(const Mat4 matrix_a, const Mat4 matrix_b) {
    Mat4 result = mat4(0);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                result[i][j] += matrix_a[i][k] * matrix_b[k][j];
            }
        }
    }
    return result;
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

    auto rotation_matrix = mat4();
    rotation_matrix[0][0] = rotation[0];
    rotation_matrix[0][1] = rotation[1];
    rotation_matrix[1][0] = -rotation[1];
    rotation_matrix[1][1] = rotation[0];

    auto result = mat_multiply(matrix, rotation_matrix);
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

Triangle::Triangle() {
    vertices[0] = Vertex{
        .pos = {0.0, 0.0, 0.5, 1.0},
        .color = {1.0, 0.0, 0.0, 1.0},
    };
    vertices[1] = Vertex{
        .pos = {0.0, 1.0, 0.5, 1.0},
        .color = {0.0, 1.0, 0.0, 1.0},
    };
    vertices[2] = Vertex{
        .pos = {-1.0, 0.0, 0.5, 1.0},
        .color = {0.0, 0.0, 1.0, 1.0},
    };
};

SquareModel::SquareModel() {
    /* Triangle 1*/
    vertices[0] = Vertex{
        .pos = {0.0, 0.0, 0.5, 1.0},
        .color = {1.0, 0.0, 0.0, 1.0},
    };
    vertices[1] = Vertex{
        .pos = {0.0, 1.0, 0.5, 1.0},
        .color = {0.0, 1.0, 0.0, 1.0},
    };
    vertices[2] = Vertex{
        .pos = {-1.0, 0.0, 0.5, 1.0},
        .color = {0.0, 0.0, 1.0, 1.0},
    };

    /* Triangle 2*/
    vertices[3] = Vertex{
        .pos = {-1.0, 1.0, 0.5, 1.0},
        .color = {1.0, 0.0, 0.0, 1.0},
    };
    vertices[4] = Vertex{
        .pos = {-1.0, 0.0, 0.5, 1.0},
        .color = {0.0, 1.0, 0.0, 1.0},
    };
    vertices[5] = Vertex{
        .pos = {0.0, 1.0, 0.5, 1.0},
        .color = {0.0, 0.0, 1.0, 1.0},
    };
};
