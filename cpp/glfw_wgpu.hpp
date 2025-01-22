#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>

WGPUSurface glfwCreateWindowWGPUSurface(
    WGPUInstance instance, GLFWwindow *window
);
