#include "./glfw_wgpu.hpp"
#include <GLFW/glfw3.h>
// #include <Windows.h>
#include <cstdio>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <print>
#include <vector>
#include <webgpu/webgpu.h>

using namespace std;

WGPUAdapter get_adapter(WGPUInstance instance, WGPUSurface surface) {
    WGPUAdapter adapter = nullptr;
    auto callback = [](WGPURequestAdapterStatus,
                       WGPUAdapter adapter,
                       char const *,
                       WGPU_NULLABLE void *user_data_v) {
        WGPUAdapter *const user_data = static_cast<WGPUAdapter *>(user_data_v);
        *user_data = adapter;
    };
    auto options = WGPURequestAdapterOptions{
        .compatibleSurface = surface,
    };
    wgpuInstanceRequestAdapter(instance, &options, callback, &adapter);
    return adapter;
}

WGPUDevice get_device(WGPUAdapter adapter) {
    WGPUDevice device = nullptr;
    auto callback = [](WGPURequestDeviceStatus status,
                       WGPUDevice device,
                       char const *,
                       WGPU_NULLABLE void *user_data_v) {
        if (status !=
            WGPURequestDeviceStatus::WGPURequestDeviceStatus_Success) {
            println(stderr, "failed getting device");
        }
        WGPUDevice *const user_data = static_cast<WGPUDevice *>(user_data_v);
        *user_data = device;
    };

    auto descriptor = WGPUDeviceDescriptor{
        .label = "device_1",
        .defaultQueue = {.label = "queue_1"},
    };
    wgpuAdapterRequestDevice(adapter, &descriptor, callback, &device);
    return device;
}

int main() {
    /* Init */
    println("starting");

    // @todo: windows-only

    // AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS) -> LONG {
    //     println(stderr, "fatal error");
    //     std::exit(EXIT_FAILURE);
    // });

    WGPUInstanceDescriptor desc = {};
    auto instance = wgpuCreateInstance(&desc);
    if (!instance) {
        println(stderr, "expected instance");
        return 1;
    }

    glfwInit();
    auto platform = glfwGetPlatform();
    if (platform == GLFW_PLATFORM_WAYLAND) {
        println("Using Wayland backend");
    } else if (platform == GLFW_PLATFORM_X11) {
        println("Using X11 backend");
    }
    auto x11_support = glfwPlatformSupported(GLFW_PLATFORM_X11);
    auto wayland_support = glfwPlatformSupported(GLFW_PLATFORM_WAYLAND);
    auto windows_support = glfwPlatformSupported(GLFW_PLATFORM_WIN32);
    println("x11     support: {}", (bool)x11_support);
    println("wayland support: {}", (bool)wayland_support);
    println("windows support: {}", (bool)windows_support);

    glfwSetErrorCallback([](int error_code, const char *description) {
        println(stderr, "glfw err {}, {}", error_code, description);
    });
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(600, 600, "Block", nullptr, nullptr);
    if (!window || glfwWindowShouldClose(window)) {
        println("window failed to open properly");
        return 1;
    }

    glfwSetWindowCloseCallback(window, [](GLFWwindow *) {
        println("window close event detected");
    });

    println("getting surface...");
    auto surface = glfwCreateWindowWGPUSurface(instance, window);
    println("getting surface...done");

    println("getting adapter...");
    auto adapter = get_adapter(instance, surface);
    println("getting adapter...done");
    if (!adapter) {
        println(stderr, "expected adapter");
        return 1;
    }
    wgpuInstanceRelease(instance);

    size_t adapter_feature_count =
        wgpuAdapterEnumerateFeatures(adapter, nullptr);
    println("adapter features: {}", adapter_feature_count);
    auto adapter_features = vector<WGPUFeatureName>(adapter_feature_count);
    wgpuAdapterEnumerateFeatures(adapter, adapter_features.data());

    WGPUAdapterInfo adapter_info = {};
    wgpuAdapterGetInfo(adapter, &adapter_info);
    println(
        "{}, {}, {}",
        adapter_info.device,
        magic_enum::enum_name(adapter_info.backendType),
        magic_enum::enum_name(adapter_info.adapterType)
    );

    auto device = get_device(adapter);
    if (!device) {
        println(stderr, "expected device");
        return 1;
    }
    wgpuAdapterRelease(adapter);

    println("getting features...");
    size_t device_feature_count = wgpuDeviceEnumerateFeatures(device, nullptr);
    println("device features: {}", device_feature_count);
    auto device_features = vector<WGPUFeatureName>(device_feature_count);
    wgpuDeviceEnumerateFeatures(device, device_features.data());
    println("getting features...done");

    println("getting limits...");
    WGPUSupportedLimits limits = {};
    wgpuDeviceGetLimits(device, &limits);
    println("getting limits...done");

    // @todo: device errors, changed? push/pop error scope?

    // auto onDeviceError =
    //     [](WGPUErrorType type, char const *message, void * /* pUserData */) {
    //         println(
    //             "Uncaptured device error: type {}",
    //             magic_enum::enum_name(type)
    //         );
    //         if (message) {
    //             println("{}", message);
    //         }
    //     };
    // wgpuDeviceSetUncapturedErrorCallback(
    //     device, onDeviceError, nullptr /* pUserData */
    // );

    // auto queue = wgpuDeviceGetQueue(device);

    // auto shader_descriptor = WGPUShaderModuleDescriptor{
    //     .label = "hi",
    // };
    // wgpuDeviceCreateShaderModule(device, &shader_descriptor);

    println("running...");
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    /* Cleanup */

    // wgpuQueueRelease(queue);
    wgpuDeviceRelease(device);
    glfwDestroyWindow(window);
    glfwTerminate();
    println("done");
}
