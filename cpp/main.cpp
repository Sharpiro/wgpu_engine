#include "./glfw_wgpu copy.hpp"
#include <GLFW/glfw3.h>
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
        // .compatibleSurface = surface,
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

void error_callback(int error_code, const char *description) {
    println(stderr, "glfw err {}, {}", error_code, description);
}

// Window close callback function
void windowCloseCallback(GLFWwindow *window) {
    println("Window close event detected");
}

int main() {
    /* Init */
    println("starting");

    WGPUInstanceDescriptor desc = {};
    WGPUInstance instance = nullptr;
    instance = wgpuCreateInstance(&desc);
    if (!instance) {
        println(stderr, "expected instance");
        return 1;
    }

    glfwInit();
    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(600, 600, "Block", nullptr, nullptr);
    if (!window || glfwWindowShouldClose(window)) {
        println("window failed to open properly");
        return 1;
    }
    glfwPollEvents();
    glfwSetWindowCloseCallback(window, windowCloseCallback);

    auto surface = glfwCreateWindowWGPUSurface(instance, window);

    // auto adapter = get_adapter(instance, surface);
    auto adapter = get_adapter(instance, nullptr);
    if (!adapter) {
        println(stderr, "expected adapter");
        return 1;
    }
    // wgpuInstanceRelease(instance);

    size_t adapter_feature_count =
        wgpuAdapterEnumerateFeatures(adapter, nullptr);
    println("adapter features: {}", adapter_feature_count);
    auto adapter_features = vector<WGPUFeatureName>(adapter_feature_count);
    wgpuAdapterEnumerateFeatures(adapter, adapter_features.data());

    WGPUAdapterInfo adapter_info;
    wgpuAdapterGetInfo(adapter, &adapter_info);
    println(
        "device: {}\nbackend: {}\nadapter: {}",
        adapter_info.device,
        magic_enum::enum_name(adapter_info.backendType),
        magic_enum::enum_name(adapter_info.adapterType)
    );

    auto device = get_device(adapter);
    if (!device) {
        println(stderr, "expected device");
        return 1;
    }
    // wgpuAdapterRelease(adapter);

    size_t device_feature_count = wgpuDeviceEnumerateFeatures(device, nullptr);
    println("device features: {}", device_feature_count);
    auto device_features = vector<WGPUFeatureName>(device_feature_count);
    wgpuDeviceEnumerateFeatures(device, device_features.data());

    WGPUSupportedLimits limits;
    wgpuDeviceGetLimits(device, &limits);

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

    // glfwInit();
    // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // auto window = glfwCreateWindow(600, 600, "Block", nullptr, nullptr);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    /* Cleanup */

    // wgpuQueueRelease(queue);
    // glfwDestroyWindow(window);
    // glfwTerminate();
    // wgpuDeviceRelease(device);
    println("done");
}
