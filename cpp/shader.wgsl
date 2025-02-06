const ASPECT_RATIO = 800.0 / 600.0;

struct VertexIn {
    @location(0) position : vec4f,
    @location(1) color : vec4f,
}

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) color : vec4f,
}

@vertex
fn vs_main(vertex_in: VertexIn) -> VertexOut {
    var vertex_out: VertexOut;
    vertex_out.position = vertex_in.position;
    vertex_out.position.y *= ASPECT_RATIO;
    vertex_out.color = vertex_in.color;
    return vertex_out;
}
    
@fragment
fn fs_main(vertex_info: VertexOut) -> @location(0) vec4f {
    return vertex_info.color;
} 
