#include "./glfw_wgpu.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdio>
#include <format>
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include <print>
#include <sstream>
#include <thread>
#include <vector>
#include <webgpu/webgpu.h>

using namespace std;

constexpr size_t WIDTH = 600;
constexpr size_t HEIGHT = 600;

struct Vertex {
    float pos[4];
    float color[4];
};

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
    try {
        /* Init */
        println("starting");

#ifdef WINDOWS
// @todo: catches null refs but messes up try/catch
// AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS e) -> LONG {
//     println(stderr, "fatal error");
//     std::exit(EXIT_FAILURE);
// });
#endif

        glfwInit();

        int32_t major;
        int32_t minor;
        int32_t rev;
        glfwGetVersion(&major, &minor, &rev);
        println("glfw v{}.{}.{}", major, minor, rev);

        auto x11_support = glfwPlatformSupported(GLFW_PLATFORM_X11);
        auto wayland_support = glfwPlatformSupported(GLFW_PLATFORM_WAYLAND);
        auto windows_support = glfwPlatformSupported(GLFW_PLATFORM_WIN32);
        println("x11     support: {}", (bool)x11_support);
        println("wayland support: {}", (bool)wayland_support);
        println("windows support: {}", (bool)windows_support);

        auto platform = glfwGetPlatform();
        if (platform == GLFW_PLATFORM_WAYLAND) {
            println("Using Wayland backend");
        } else if (platform == GLFW_PLATFORM_X11) {
            println("Using X11 backend");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
        glfwWindowHint(GLFW_POSITION_X, 2800);
        glfwWindowHint(GLFW_POSITION_Y, 500);

        glfwSetErrorCallback([](int error_code, const char *description) {
            println(stderr, "glfw err {}, {}", error_code, description);
        });
        auto window =
            glfwCreateWindow(WIDTH, HEIGHT, "Block", nullptr, nullptr);
        if (!window) {
            println("window failed to open properly");
            return 1;
        }

        glfwSetWindowAttrib(window, GLFW_FOCUS_ON_SHOW, GLFW_FALSE);

        glfwSetWindowFocusCallback(window, [](GLFWwindow *, int focused) {
            println("focus change: {}", focused);
        });
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
        size_t device_feature_count =
            wgpuDeviceEnumerateFeatures(device, nullptr);
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

        auto file = ifstream("shader.wgsl");
        if (!file.is_open()) {
            throw runtime_error("shader not found");
        }
        auto buffer = stringstream();
        buffer << file.rdbuf();
        auto shader_code = buffer.str();

        WGPUShaderModuleWGSLDescriptor shader_code_desc = {
            .chain =
                {
                    .sType = WGPUSType_ShaderModuleWGSLDescriptor,
                },
            .code = shader_code.data(),
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

        /** Vertex buffers */

        vector<Vertex> vertex_data = {
            Vertex{
                .pos = {-0.5, -0.5, 0.0, 1.0},
                .color = {1.0, 0.0, 0.0, 1.0},
            },
            Vertex{
                .pos = {0.5, -0.5, 0.0, 1.0},
                .color = {0.0, 1.0, 0.0, 1.0},
            },
            Vertex{
                .pos = {0.0, 0.5, 0.0, 1.0},
                .color = {0.0, 0.0, 1.0, 1.0},
            },
        };

        auto vertex_buffer_size = vertex_data.size() * sizeof(Vertex);
        WGPUBufferDescriptor vertex_buffer_desc = {
            .nextInChain = nullptr,
            .label = "vertex_buffer",
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .size = vertex_buffer_size,
            .mappedAtCreation = false,
        };
        auto vertex_buffer =
            wgpuDeviceCreateBuffer(device, &vertex_buffer_desc);
        wgpuQueueWriteBuffer(
            queue, vertex_buffer, 0, vertex_data.data(), vertex_buffer_size
        );

        WGPUVertexAttribute vertex_attributes[] = {
            {
                .format = WGPUVertexFormat_Float32x4,
                .offset = 0,
                .shaderLocation = 0,
            },
            {
                .format = WGPUVertexFormat_Float32x4,
                .offset = 16,
                .shaderLocation = 1,
            },
        };

        WGPUVertexBufferLayout vertex_buffer_layout = {
            .arrayStride = sizeof(Vertex),
            .stepMode = WGPUVertexStepMode_Vertex,
            .attributeCount =
                sizeof(vertex_attributes) / sizeof(WGPUVertexAttribute),
            .attributes = vertex_attributes,
        };
        WGPURenderPipelineDescriptor pipelineDesc = {
            .vertex =
                {
                    .module = shader_module,
                    .entryPoint = "vs_main",
                    .bufferCount = 1,
                    .buffers = &vertex_buffer_layout,
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
        auto pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);

        auto command_encoder =
            wgpuDeviceCreateCommandEncoder(device, &command_encoder_desc);

        WGPURenderPassEncoder render_pass =
            wgpuCommandEncoderBeginRenderPass(command_encoder, &renderPassDesc);

        /** Render */

        wgpuRenderPassEncoderSetPipeline(render_pass, pipeline);
        wgpuRenderPassEncoderSetVertexBuffer(
            render_pass, 0, vertex_buffer, 0, vertex_buffer_size
        );
        wgpuRenderPassEncoderDraw(render_pass, vertex_data.size(), 1, 0, 0);
        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);

        WGPUCommandBufferDescriptor command_buffer_descriptor = {
            .nextInChain = nullptr,
            .label = "Command buffer",
        };
        auto command = wgpuCommandEncoderFinish(
            command_encoder, &command_buffer_descriptor
        );
        wgpuCommandEncoderRelease(command_encoder);

        println("submitting command...");
        wgpuQueueSubmit(queue, 1, &command);
        wgpuCommandBufferRelease(command);
        println("submitting command...done");

        println("presenting surface...");
        wgpuSurfacePresent(surface);
        println("presenting surface...done");

        println("opening window...");
        this_thread::sleep_for(chrono::seconds(1));
        glfwShowWindow(window);

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
    } catch (runtime_error &err) {
        println("{}", err.what());
    }
}
