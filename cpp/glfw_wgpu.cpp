#include "./glfw_wgpu.hpp"
#include "webgpu/webgpu.h"
#include <GLFW/glfw3native.h>

// @todo: windows-only

// WGPUSurface glfwCreateWindowWGPUSurface(
//     WGPUInstance instance, GLFWwindow *window
// ) {
//     auto hwnd = glfwGetWin32Window(window);
//     auto hinstance = GetModuleHandle(nullptr);
//     auto hwind_descriptor = WGPUSurfaceDescriptorFromWindowsHWND{
//         .chain =
//             {
//                 .sType = WGPUSType_SurfaceDescriptorFromWindowsHWND,
//             },
//         .hinstance = hinstance,
//         .hwnd = hwnd,
//     };
//     auto surface_descriptor = WGPUSurfaceDescriptor{
//         .nextInChain = &hwind_descriptor.chain,
//     };
//     auto surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);

//     return surface;
//     return nullptr;
// }

// // @todo: wayland-only

// WGPUSurface glfwCreateWindowWGPUSurface(
//     WGPUInstance instance, GLFWwindow *window
// ) {
//     wl_display *display = glfwGetWaylandDisplay();
//     wl_surface *wayland_window = glfwGetWaylandWindow(window);
//     WGPUSurfaceDescriptorFromWaylandSurface x = {
//         .chain =
//             {
//                 .sType = WGPUSType_SurfaceDescriptorFromWaylandSurface,
//             },
//         .display = display,
//         .surface = wayland_window,
//     };
//     auto surface_descriptor = WGPUSurfaceDescriptor{
//         .nextInChain = &x.chain,
//     };
//     auto surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);

//     return surface;
// }

// @todo: x11-only

WGPUSurface glfwCreateWindowWGPUSurface(
    WGPUInstance instance, GLFWwindow *window
) {
    Display *display = glfwGetX11Display();
    Window x11_window = glfwGetX11Window(window);
    WGPUSurfaceDescriptorFromXlibWindow x_descriptor = {
        .chain =
            {
                .sType = WGPUSType_SurfaceDescriptorFromXlibWindow,
            },
        .display = display,
        .window = x11_window,
    };
    auto surface_descriptor = WGPUSurfaceDescriptor{
        .nextInChain = &x_descriptor.chain,
    };
    auto surface = wgpuInstanceCreateSurface(instance, &surface_descriptor);

    return surface;
}
