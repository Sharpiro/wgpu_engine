// #define GLFW_EXPOSE_NATIVE_WGL
#ifdef WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif defined(LINUX)
#define GLFW_EXPOSE_NATIVE_X11
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <webgpu/webgpu.h>

WGPUSurface glfwCreateWindowWGPUSurface(
    WGPUInstance instance, GLFWwindow *window
);
