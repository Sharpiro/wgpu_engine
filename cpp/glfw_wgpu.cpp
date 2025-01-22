// #define GLFW_EXPOSE_NATIVE_WGL 1
#define GLFW_EXPOSE_NATIVE_WIN32 1

#include "./glfw_wgpu.hpp"
#include "webgpu/webgpu.h"
#include <GLFW/glfw3native.h>

WGPUSurface glfwCreateWindowWGPUSurface(
    WGPUInstance instance, GLFWwindow *window
) {
    auto hwnd = glfwGetWin32Window(window);
    auto hinstance = GetModuleHandle(nullptr);
    auto hwind_descriptor = WGPUSurfaceDescriptorFromWindowsHWND{
        .chain =
            {
                .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
            },
        .hinstance = hinstance,
        .hwnd = hwnd,
    };
    auto surface_descriptor = WGPUSurfaceDescriptor{
        .nextInChain = &hwind_descriptor.chain,
    };
    auto surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);

    return surface;
}
