// #define GLFW_EXPOSE_NATIVE_WGL
// #define GLFW_EXPOSE_NATIVE_WIN32
// #define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_X11

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <webgpu/webgpu.h>

WGPUSurface glfwCreateWindowWGPUSurface(
    WGPUInstance instance, GLFWwindow *window
);
