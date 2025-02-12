#pragma once

#include <array>
#include <cstdint>

using namespace std;

struct Vec2 {
    array<float, 2> data;

    template <typename... Args>
    constexpr Vec2(Args... args)
        requires(sizeof...(Args) == 2)
        : data{static_cast<float>(args)...} {
    }

    const float &operator[](size_t index) const {
        return data[index];
    }

    size_t size() {
        return data.size();
    }
};

struct Vec3 {
    array<float, 3> data;

    template <typename... Args>
    constexpr Vec3(Args... args)
        requires(sizeof...(Args) == 3)
        : data{static_cast<float>(args)...} {
    }

    const float &operator[](size_t index) const {
        return data[index];
    }

    size_t size() {
        return data.size();
    }
};

struct Vec4 {
    array<float, 4> data;

    template <typename... Args>
    constexpr Vec4(Args... args)
        requires(sizeof...(Args) == 4)
        : data{static_cast<float>(args)...} {
    }

    Vec4() {
    }

    float &operator[](size_t index) {
        return data[index];
    }

    const float &operator[](size_t index) const {
        return data[index];
    }

    bool operator==(Vec4 &other) {
        return data == other.data;
    }

    size_t size() {
        return data.size();
    }

    Vec3 xyz() {
        return {data[0], data[1], data[2]};
    }
};

using Mat4 = array<Vec4, 4>;

struct Vertex {
    Vec4 pos;
    Vec4 color;
    uint32_t model_index;
};

struct Triangle {
    Vertex vertices[3];

    Triangle(uint32_t triangle_index);
};

struct SquareModel {
    Vertex vertices[6];

    SquareModel(uint32_t triangle_index);
};

Vec4 translate_vec4(Vec4 point, Vec3 translation);

Mat4 scale_mat4(const Mat4 &matrix, const Vec3 &scale);

Mat4 rotate_mat4(const Mat4 &matrix, const Vec2 &rotation);

Mat4 mat4(float d = 1);

Mat4 translate_mat4(const Mat4 &matrix, const Vec3 &translation);

Vec4 mat_multiply(const Mat4 matrix, const Vec4 vec);
