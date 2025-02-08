#include <array>

using namespace std;

using Vec4 = array<float, 4>;

using Mat4 = array<Vec4, 4>;

struct Vertex {
    Vec4 pos;
    Vec4 color;
};

struct Triangle {
    Vertex vertices[3];

    static Triangle get_default();
    void translate(Vec4 rotation);
};
