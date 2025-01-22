#include <cstdio>
#include <format>
#include <magic_enum.hpp>
#include <print>
#include <vector>
#include <webgpu/webgpu.h>

using namespace std;

WGPUAdapter get_adapter(WGPUInstance instance) {
    WGPUAdapter adapter = nullptr;
    auto adapter_callback = [](WGPURequestAdapterStatus,
                               WGPUAdapter adapter,
                               char const *,
                               WGPU_NULLABLE void *userdata) {
        WGPUAdapter *const local_adapter = static_cast<WGPUAdapter *>(userdata);
        *local_adapter = adapter;
    };
    wgpuInstanceRequestAdapter(instance, nullptr, adapter_callback, &adapter);
    return adapter;
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

    auto adapter = get_adapter(instance);
    if (!adapter) {
        println(stderr, "expected adapter");
        return 1;
    }
    wgpuInstanceRelease(instance);

    size_t feature_count = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    println("features: {}", feature_count);
    auto features = vector<WGPUFeatureName>(feature_count);
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    WGPUAdapterInfo adapter_info;
    wgpuAdapterGetInfo(adapter, &adapter_info);
    println(
        "device: {}\nbackend: {}\nadapter: {}",
        adapter_info.device,
        magic_enum::enum_name(adapter_info.backendType),
        magic_enum::enum_name(adapter_info.adapterType)
    );

    /* Cleanup */

    wgpuAdapterRelease(adapter);
    println("done");
}
