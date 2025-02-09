#include <array>

using namespace std;

struct Vec3 {
    array<float, 3> data;

    template <typename... Args>
    constexpr Vec3(Args... args)
        requires(sizeof...(Args) == 3)
        : data{static_cast<float>(args)...} {
    }

    float &operator[](size_t index) {
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

    float &operator[](size_t index) {
        return data[index];
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
};

struct Triangle {
    Vertex vertices[3];

    static Triangle get_default();
    void translate(Vec3 rotation);
};

Mat4 mat4();
