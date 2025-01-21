#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <print>
#include <webgpu/webgpu.h>

int main() {
  WGPUInstanceDescriptor desc = {};
  WGPUInstance instance = wgpuCreateInstance(&desc);

  // std::println("desc {}", (void *)desc.nextInChain);
}
