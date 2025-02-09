#include "./shape.hpp"
#include <stdexcept>

Vec4 vec4f() {
    return {0.0, 0.0, 0.0, 0.0};
}

Vec4 mat_multiply(Mat4 matrix, Vec4 vec) {
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

Mat4 mat4() {
    return {{
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }};
}

Mat4 translate(Mat4 matrix, Vec3 translation) {
    Mat4 translation_matrix = {{
        {1.0, 0.0, 0.0, translation[0]},
        {0.0, 1.0, 0.0, translation[1]},
        {0.0, 0.0, 1.0, translation[2]},
        {0.0, 0.0, 0.0, 1.0},
    }};
    // auto result = mat_multiply(translation_matrix, translation);
    throw runtime_error("unimp");
}

Triangle Triangle::get_default() {
    return Triangle{
        .vertices =
            {
                Vertex{
                    .pos = {0.0, 0.0, 0.5, 1.0},
                    .color = {1.0, 0.0, 0.0, 1.0},
                },
                Vertex{
                    .pos = {0.0, 1.0, 0.5, 1.0},
                    .color = {0.0, 1.0, 0.0, 1.0},
                },
                Vertex{
                    .pos = {-1.0, 0.0, 0.5, 1.0},
                    .color = {0.0, 0.0, 1.0, 1.0},
                },
            },
    };
}

void Triangle::translate(Vec3 translation) {
    Mat4 translation_matrix = {{
        {1.0, 0.0, 0.0, translation[0]},
        {0.0, 1.0, 0.0, translation[1]},
        {0.0, 0.0, 1.0, translation[2]},
        {0.0, 0.0, 0.0, 1.0},
    }};

    this->vertices[0].pos =
        mat_multiply(translation_matrix, this->vertices[0].pos);
    this->vertices[1].pos =
        mat_multiply(translation_matrix, this->vertices[1].pos);
    this->vertices[2].pos =
        mat_multiply(translation_matrix, this->vertices[2].pos);
}
