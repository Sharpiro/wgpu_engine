const ASPECT_RATIO = 800.0 / 600.0;

struct VertexIn {
    @location(0) position : vec4f,
    @location(1) color : vec4f,
}

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) color : vec4f,
}

struct UniformData {
    model_transformations: array<mat4x4f, 16>,
    model_colors: array<vec4f, 16>,
}

@group(0) @binding(0)
var<uniform> uniform_data: UniformData;

@vertex
fn vs_main(vertex_in: VertexIn, @builtin(instance_index) instance_index: u32) -> VertexOut {
    let model_transformation = uniform_data.model_transformations[instance_index];
    let model_color = uniform_data.model_colors[instance_index];
    let position = model_transformation * vertex_in.position;
    let color = select(
        model_color, vertex_in.color, dot(model_color, model_color) == 0
    );
    return VertexOut(position, color);
}
    
@fragment
fn fs_main(vertex_info: VertexOut) -> @location(0) vec4f {
    return vertex_info.color;
} 
