#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <objc/runtime.h>
#include <cstdio>
#include <cstring>
#include "Metal.hpp"

// The C++ bridge owns returned errors and releases them with NSObject_release.
// An error must survive draining Metal's autorelease pool before that release.
int main() {
  @autoreleasepool {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
      std::fprintf(stderr, "No Metal device available\n");
      return 1;
    }
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    const char source[] = "this is not valid Metal source";
    obj_handle_t error = 0;
    obj_handle_t library = MTLDevice_newLibraryWithSource(
        (obj_handle_t)device, source, sizeof(source) - 1, &error);
    if (library || !error) {
      std::fprintf(stderr, "Invalid source did not return a Metal error\n");
      return 1;
    }
    [pool drain];
    const char *name = object_getClassName((id)error);
    if (std::strncmp(name, "_NSZombie_", 10) == 0) {
      std::fprintf(stderr, "Metal error was deallocated before ownership release\n");
      return 1;
    }
    if (![(NSError *)error localizedDescription].length) {
      std::fprintf(stderr, "Metal error lost its description\n");
      return 1;
    }
    NSObject_release(error);

    // Reproduce the failed pipeline path that originally crashed a worker's
    // implicit autorelease pool during thread exit.
    WMT::Device bridge((obj_handle_t)device);
    WMT::Reference<WMT::Error> pipeline_error;
    const char vertex_source[] =
        "#include <metal_stdlib>\nusing namespace metal;\n"
        "vertex float4 test_vertex(float4 p [[attribute(0)]]) { return p; }";
    auto vertex_library = bridge.newLibraryWithSource(vertex_source, pipeline_error);
    if (!vertex_library || pipeline_error) {
      std::fprintf(stderr, "Valid vertex source failed to compile\n");
      return 1;
    }
    auto function = vertex_library.newFunction("test_vertex");
    WMTRenderPipelineInfo info{};
    WMT::InitializeRenderPipelineInfo(info);
    info.vertex_function = function;
    info.colors[0].pixel_format = WMTPixelFormatBGRA8Unorm;
    info.colors[0].write_mask = WMTColorWriteMaskAll;
    pool = [[NSAutoreleasePool alloc] init];
    auto pipeline = bridge.newRenderPipelineState(info, pipeline_error);
    if (pipeline || !pipeline_error) {
      std::fprintf(stderr, "Missing vertex descriptor did not return a Metal error\n");
      return 1;
    }
    obj_handle_t first_error = pipeline_error;
    [pool drain];
    if (std::strncmp(object_getClassName((id)first_error), "_NSZombie_", 10) == 0) {
      std::fprintf(stderr, "Pipeline error was deallocated before ownership release\n");
      return 1;
    }
    // Reusing an error out-parameter must release its previous owned value.
    pool = [[NSAutoreleasePool alloc] init];
    auto second_pipeline = bridge.newRenderPipelineState(info, pipeline_error);
    if (second_pipeline || !pipeline_error ||
        std::strncmp(object_getClassName((id)first_error), "_NSZombie_", 10) != 0) {
      std::fprintf(stderr, "Reused error out-parameter leaked its previous value\n");
      return 1;
    }
    pipeline_error = nullptr;
    [pool drain];
    [device release];
    std::puts("Metal error ownership passed");
  }
  return 0;
}
