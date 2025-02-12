#include "./glfw_wgpu.hpp"
#include "./shape.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cmath>
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

constexpr size_t SCREEN_WIDTH = 600;
constexpr size_t SCREEN_HEIGHT = 600;
constexpr size_t GRID_WIDTH = 4;
constexpr size_t GRID_HEIGHT = 4;
constexpr size_t SQUARE_COUNT = GRID_WIDTH * GRID_HEIGHT;

struct UniformData {
    array<Mat4, SQUARE_COUNT> model_transformations;
    array<Vec4, SQUARE_COUNT> model_colors;
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
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
#ifdef LINUX
        glfwWindowHint(GLFW_POSITION_X, 2800);
        glfwWindowHint(GLFW_POSITION_Y, 500);
#elif defined(WINDOWS)
        glfwWindowHint(GLFW_POSITION_X, 2200);
        glfwWindowHint(GLFW_POSITION_Y, 200);
#endif

        glfwSetErrorCallback([](int error_code, const char *description) {
            println(stderr, "glfw err {}, {}", error_code, description);
        });
        auto window = glfwCreateWindow(
            SCREEN_WIDTH, SCREEN_HEIGHT, "Block", nullptr, nullptr
        );
        if (!window) {
            println("window failed to open properly");
            return 1;
        }

        glfwSetWindowAttrib(window, GLFW_FOCUS_ON_SHOW, GLFW_FALSE);

        glfwSetWindowCloseCallback(window, [](GLFWwindow *) {
            println("window close event detected");
        });
        glfwSetMouseButtonCallback(
            window,
            [](GLFWwindow *window, int button, int action, int) {
                if (button != GLFW_MOUSE_BUTTON_1 || action != GLFW_PRESS) {
                    return;
                }

                double x_pos;
                double y_pos;
                glfwGetCursorPos(window, &x_pos, &y_pos);

                constexpr float SEGMENT_WIDTH =
                    (float)SCREEN_WIDTH / GRID_WIDTH;
                constexpr float SEGMENT_HEIGHT =
                    (float)SCREEN_HEIGHT / GRID_HEIGHT;

                size_t x_seg = x_pos / SEGMENT_WIDTH;
                size_t y_seg = y_pos / SEGMENT_HEIGHT;

                float x_wgsl = (x_pos / (SCREEN_WIDTH / 2.0)) - 1;
                float y_wgsl = 1 - (y_pos / (SCREEN_HEIGHT / 2.0));
                println(
                    "clicked: [{}, {}], [{}, {}], [{}, {}]",
                    x_pos,
                    y_pos,
                    x_seg,
                    y_seg,
                    x_wgsl,
                    y_wgsl
                );
            }
        );

        auto instance = wgpuCreateInstance(nullptr);
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
            .width = SCREEN_WIDTH,
            .height = SCREEN_HEIGHT,
            .presentMode = WGPUPresentMode_Fifo,
        };

        wgpuAdapterRelease(adapter);
        wgpuSurfaceConfigure(surface, &surface_config);

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
        WGPURenderPipelineDescriptor pipeline_desc = {
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

        /** Render pipeline */

        auto render_pipeline =
            wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

        /** Vertex data */

        auto triangle_data = vector{
            SquareModel(),
        };
        auto triangle_buffer_size =
            triangle_data.size() * sizeof(decltype(triangle_data)::value_type);
        WGPUBufferDescriptor vertex_buffer_desc = {
            .nextInChain = nullptr,
            .label = "vertex_buffer",
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .size = triangle_buffer_size,
            .mappedAtCreation = false,
        };
        auto vertex_buffer =
            wgpuDeviceCreateBuffer(device, &vertex_buffer_desc);
        wgpuQueueWriteBuffer(
            queue, vertex_buffer, 0, triangle_data.data(), triangle_buffer_size
        );

        /** Uniform data */

        constexpr float WGSL_WIDTH = 2.0 / GRID_WIDTH;
        constexpr float WGSL_HEIGHT = 2.0 / GRID_HEIGHT;
        constexpr float TEMP_TRANSLATE_X = GRID_WIDTH / 2.0 - 1;
        constexpr float TEMP_TRANSLATE_Y = GRID_HEIGHT / 2.0 - 1;
        auto grid_origin_matrix =
            scale_mat4(mat4(), {WGSL_WIDTH, WGSL_HEIGHT, 1.0});
        grid_origin_matrix = translate_mat4(
            grid_origin_matrix,
            {-WGSL_WIDTH * TEMP_TRANSLATE_X, WGSL_HEIGHT * TEMP_TRANSLATE_Y, 0.0
            }
        );

        auto matrix_data = array<Mat4, SQUARE_COUNT>();
        for (size_t j = 0; j < GRID_WIDTH; j++) {
            for (size_t i = 0; i < GRID_HEIGHT; i++) {
                auto model_matrix = translate_mat4(
                    grid_origin_matrix, {WGSL_WIDTH * i, -WGSL_HEIGHT * j, 0.0}
                );
                matrix_data[j * GRID_WIDTH + i] = model_matrix;
            }
        }

        UniformData uniform_data = {
            .model_transformations = matrix_data,
            .model_colors = {{
                {1.0, 0.0, 0.0, 1},
                {0.0, 1.0, 0.0, 1},
                {0.0, 0.0, 1.0, 1},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
                {0.0, 0.0, 0.0, 0},
            }},
        };

        WGPUBufferDescriptor uniform_buffer_desc = {
            .nextInChain = nullptr,
            .label = "uniform_buffer",
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
            .size = sizeof(UniformData),
            .mappedAtCreation = false,
        };
        auto uniform_buffer =
            wgpuDeviceCreateBuffer(device, &uniform_buffer_desc);
        WGPUBindGroupEntry bind_group_entry = {
            .binding = 0,
            .buffer = uniform_buffer,
            .offset = 0,
            .size = sizeof(UniformData),
        };
        WGPUBindGroupDescriptor uniform_bind_group_descriptor = {
            .label = "uniform_bind_group",
            .layout = wgpuRenderPipelineGetBindGroupLayout(render_pipeline, 0),
            .entryCount = 1,
            .entries = &bind_group_entry,
        };
        auto uniform_bind_group =
            wgpuDeviceCreateBindGroup(device, &uniform_bind_group_descriptor);

        wgpuQueueWriteBuffer(
            queue, uniform_buffer, 0, &uniform_data, sizeof(UniformData)
        );

        println("running...");
        // bool key_release = false;
        // auto w_key_release = optional(false);
        // optional<bool> w_key_release = nullopt;
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            //     model1_matrix = scale_mat4(model1_matrix, {0.95, 0.95, 1});
            //     uniform_data = {
            //         .model_matrix =
            //             {
            //                 model1_matrix,
            //                 // model2_matrix,
            //             },
            //     };
            //     wgpuQueueWriteBuffer(
            //         queue, uniform_buffer, 0, &uniform_data,
            //         sizeof(UniformData)
            //     );
            // }
            // if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            //     model1_matrix = scale_mat4(model1_matrix, {1.05, 1.05, 1});
            //     uniform_data = {
            //         .model_matrix =
            //             {
            //                 model1_matrix,
            //                 // model2_matrix,
            //             },
            //     };
            //     wgpuQueueWriteBuffer(
            //         queue, uniform_buffer, 0, &uniform_data,
            //         sizeof(UniformData)
            //     );
            // }
            // if (!w_key_release.has_value() &&
            //     glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            //     w_key_release = optional(false);
            // }
            // if (w_key_release.has_value() && !w_key_release.value() &&
            //     glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) {
            //     w_key_release = optional(true);
            // }
            // if (w_key_release.has_value() && w_key_release.value()) {
            //     w_key_release = nullopt;
            //     println("release");
            //     constexpr float ROTATION = numbers::pi / 20;
            //     auto cosx = cos(ROTATION);
            //     auto siny = sin(ROTATION);
            //     model1_matrix = rotate_mat4(model1_matrix, {cosx, siny});
            //     uniform_data = {
            //         .model_matrix =
            //             {
            //                 model1_matrix,
            //                 // model2_matrix,
            //             },
            //     };
            //     wgpuQueueWriteBuffer(
            //         queue, uniform_buffer, 0, &uniform_data,
            //         sizeof(UniformData)
            //     );
            // }

            // if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            //     // if (!tick) {
            //     tick = true;
            //     constexpr float ROTATION = numbers::pi / 2;
            //     auto cosx = cos(ROTATION);
            //     auto siny = sin(ROTATION);
            //     model1_matrix = rotate_mat4(model1_matrix, {cosx, siny});
            //     uniform_data = {
            //         .model_matrix =
            //             {
            //                 model1_matrix,
            //                 model2_matrix,
            //             },
            //     };
            //     wgpuQueueWriteBuffer(
            //         queue, uniform_buffer, 0, &uniform_data,
            //         sizeof(UniformData)
            //     );
            //     // }
            // }
            // if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            //     println("space");
            //     triangle_data[0].translate({0.0, 0.0, -0.01});
            //     wgpuQueueWriteBuffer(
            //         queue,
            //         vertex_buffer,
            //         0,
            //         triangle_data.data(),
            //         sizeof(Triangle)
            //     );
            // }
            // if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            //     println("enter");
            //     triangle_data[0].translate({0.0, 0.0, 0.01});
            //     wgpuQueueWriteBuffer(
            //         queue,
            //         vertex_buffer,
            //         0,
            //         triangle_data.data(),
            //         sizeof(Triangle)
            //     );
            // }

            WGPUSurfaceTexture surface_texture = {};
            wgpuSurfaceGetCurrentTexture(surface, &surface_texture);
            if (!surface_texture.texture) {
                std::this_thread::sleep_for(chrono::seconds(1));
                continue;
            }

            WGPUTextureViewDescriptor texture_view_desc = {
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

            auto texture_view = wgpuTextureCreateView(
                surface_texture.texture, &texture_view_desc
            );
#ifdef TEXTURE_MANUAL_RELEASE
            wgpuTextureRelease(surface_texture.texture);
#endif

            WGPURenderPassColorAttachment renderPassColorAttachment = {
                .view = texture_view,
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

            auto command_encoder =
                wgpuDeviceCreateCommandEncoder(device, nullptr);
            auto render_pass = wgpuCommandEncoderBeginRenderPass(
                command_encoder, &renderPassDesc
            );

            wgpuRenderPassEncoderSetPipeline(render_pass, render_pipeline);
            wgpuRenderPassEncoderSetVertexBuffer(
                render_pass, 0, vertex_buffer, 0, triangle_buffer_size
            );
            wgpuRenderPassEncoderSetBindGroup(
                render_pass, 0, uniform_bind_group, 0, nullptr
            );

            // auto temp = triangle_data
            wgpuRenderPassEncoderDraw(
                // @todo: generic vert count
                // render_pass, triangle_data.size() * 3, 1, 0, 0
                render_pass,
                triangle_data.size() * 6,
                SQUARE_COUNT,
                0,
                0
            );
            wgpuRenderPassEncoderEnd(render_pass);
            wgpuRenderPassEncoderRelease(render_pass);

            auto command_buffer =
                wgpuCommandEncoderFinish(command_encoder, nullptr);
            wgpuCommandEncoderRelease(command_encoder);

            wgpuQueueSubmit(queue, 1, &command_buffer);
            wgpuCommandBufferRelease(command_buffer);

            wgpuSurfacePresent(surface);

            wgpuTextureViewRelease(texture_view);
        }

        /* Cleanup */

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
