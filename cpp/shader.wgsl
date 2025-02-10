const ASPECT_RATIO = 800.0 / 600.0;

struct VertexIn {
    @location(0) position : vec4f,
    @location(1) color : vec4f,
    @location(2) triangle_index: u32,
}

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) color : vec4f,
}

struct UniformData {
    model_matrix: array<mat4x4f, 2>,
}

@group(0) @binding(0)
var<uniform> uniform_data: UniformData;

@vertex
fn vs_main(vertex_in: VertexIn) -> VertexOut {
    var vertex_out: VertexOut;
    var model_matrix = uniform_data.model_matrix[vertex_in.triangle_index];
    vertex_out.position = model_matrix * vertex_in.position;
    vertex_out.color = vertex_in.color;
    return vertex_out;
}
    
@fragment
fn fs_main(vertex_info: VertexOut) -> @location(0) vec4f {
    return vertex_info.color;
} 
