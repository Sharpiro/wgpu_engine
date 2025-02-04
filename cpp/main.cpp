#include "./glfw_wgpu.hpp"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <print>
#include <vector>
#include <webgpu/webgpu.h>

using namespace std;

constexpr size_t WIDTH = 600;
constexpr size_t HEIGHT = 600;

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
        .deviceLostCallback =
            [](WGPUDeviceLostReason reason, char const *message, void *) {
                println(
                    "Uncaptured device error: type {}",
                    magic_enum::enum_name(reason)
                );
                if (message) {
                    println("{}", message);
                }
            },
    };
    wgpuAdapterRequestDevice(adapter, &descriptor, callback, &device);
    return device;
}

int main() {
    /* Init */
    println("starting");

#ifdef WINDOWS
    AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS) -> LONG {
        println(stderr, "fatal error");
        std::exit(EXIT_FAILURE);
    });
#endif

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
    auto window = glfwCreateWindow(WIDTH, HEIGHT, "Block", nullptr, nullptr);
    if (!window || glfwWindowShouldClose(window)) {
        println("window failed to open properly");
        return 1;
    }

    glfwSetWindowCloseCallback(window, [](GLFWwindow *) {
        println("window close event detected");
    });

    WGPUInstanceDescriptor desc = {};
    auto instance = wgpuCreateInstance(&desc);
    if (!instance) {
        println(stderr, "expected instance");
        return 1;
    }

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

    auto queue = wgpuDeviceGetQueue(device);
    wgpuQueueOnSubmittedWorkDone(
        queue,
        [](WGPUQueueWorkDoneStatus status, WGPU_NULLABLE void *) {
            println("queued work done {}", magic_enum::enum_name(status));
        },
        nullptr
    );

    WGPUSurfaceCapabilities capabilities = {};
    wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);
    auto texture_format = capabilities.formats[0];
    WGPUSurfaceConfiguration surface_config = {
        .device = device,
        .format = texture_format,
        .usage = WGPUTextureUsage_RenderAttachment,
        .alphaMode = WGPUCompositeAlphaMode_Auto,
        .width = WIDTH,
        .height = HEIGHT,
        .presentMode = WGPUPresentMode_Fifo,
    };

    wgpuAdapterRelease(adapter);
    wgpuSurfaceConfigure(surface, &surface_config);

    WGPUSurfaceTexture surface_texture = {};
    wgpuSurfaceGetCurrentTexture(surface, &surface_texture);
    WGPUTextureViewDescriptor tv_descriptor = {
        .nextInChain = nullptr,
        .label = "Surface texture view",
        .format = wgpuTextureGetFormat(surface_texture.texture),
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = WGPUTextureAspect_All,

    };
    auto texture_view =
        wgpuTextureCreateView(surface_texture.texture, &tv_descriptor);

    WGPUCommandEncoderDescriptor command_encoder_desc = {
        .nextInChain = nullptr,
        .label = "My command encoder",
    };
    auto command_encoder =
        wgpuDeviceCreateCommandEncoder(device, &command_encoder_desc);

    WGPURenderPassColorAttachment renderPassColorAttachment = {
        .view = texture_view,
        .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        .resolveTarget = nullptr,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = WGPUColor{0.0, 0.0, 0.0, 1.0},
    };
    WGPURenderPassDescriptor renderPassDesc = {
        .colorAttachmentCount = 1,
        .colorAttachments = &renderPassColorAttachment,
        .depthStencilAttachment = nullptr,
        .timestampWrites = nullptr,
    };
    WGPURenderPassEncoder renderPass =
        wgpuCommandEncoderBeginRenderPass(command_encoder, &renderPassDesc);

    const char *shader_code = R"(
    @vertex
    fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
        var p = vec2f(0.0, 0.0);
        if (in_vertex_index == 0u) {
            p = vec2f(-0.5, -0.5);
        } else if (in_vertex_index == 1u) {
            p = vec2f(0.5, -0.5);
        } else {
            p = vec2f(0.0, 0.5);
        }
        return vec4f(p, 0.0, 1.0);
    }
    
    @fragment
    fn fs_main() -> @location(0) vec4f {
        return vec4f(0.0, 1.0, 0.0, 1.0);
    } 
)";

    WGPUShaderModuleWGSLDescriptor shader_code_desc = {
        .chain =
            {
                .sType = WGPUSType_ShaderModuleWGSLDescriptor,
            },
        .code = shader_code,
    };
    auto shader_descriptor = WGPUShaderModuleDescriptor{
        .nextInChain = &shader_code_desc.chain,
    };
    auto shader_module =
        wgpuDeviceCreateShaderModule(device, &shader_descriptor);

    WGPUBlendState blend_state = {
        .color =
            {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            },
        .alpha =
            {
                .operation = WGPUBlendOperation_Add,
                .srcFactor = WGPUBlendFactor_Zero,
                .dstFactor = WGPUBlendFactor_One,
            },
    };
    WGPUColorTargetState color_target = {
        .format = texture_format,
        .blend = &blend_state,
        .writeMask = WGPUColorWriteMask_All,
    };
    WGPUFragmentState fragment_state{
        .module = shader_module,
        .entryPoint = "fs_main",
        .targetCount = 1,
        .targets = &color_target,
    };
    WGPURenderPipelineDescriptor pipelineDesc = {
        .vertex =
            {
                .module = shader_module,
                .entryPoint = "vs_main",
            },
        .primitive =
            {
                .topology = WGPUPrimitiveTopology_TriangleList,
                .stripIndexFormat = WGPUIndexFormat_Undefined,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_None,
            },
        .multisample =
            {
                .count = 1,
                .mask = ~0u,
                .alphaToCoverageEnabled = false,
            },
        .fragment = &fragment_state,
    };
    WGPURenderPipeline pipeline =
        wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
    wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
    wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);
    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);

    WGPUCommandBufferDescriptor command_buffer_descriptor = {
        .nextInChain = nullptr,
        .label = "Command buffer",
    };
    auto command =
        wgpuCommandEncoderFinish(command_encoder, &command_buffer_descriptor);
    wgpuCommandEncoderRelease(command_encoder);

    println("submitting command...");
    wgpuQueueSubmit(queue, 1, &command);
    wgpuCommandBufferRelease(command);
    println("submitting command...done");

    println("presenting surface...");
    wgpuSurfacePresent(surface);
    println("presenting surface...done");

    println("running...");
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    /* Cleanup */

    wgpuTextureViewRelease(texture_view);
    wgpuTextureRelease(surface_texture.texture);
    wgpuQueueRelease(queue);
    wgpuDeviceRelease(device);
    wgpuSurfaceRelease(surface);
    glfwDestroyWindow(window);
    glfwTerminate();
    println("done");
}
