struct VertexOut {
  @builtin(position) position : vec4f,
  @location(0) color : vec4f,
}

struct Uniforms {
    translation: vec4f,
    scale: vec4f,
}

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

@vertex
fn vs_main(@location(0) position: vec4f, @location(1) color: vec4f) -> VertexOut {
        var output : VertexOut;
        output.color = color;
        // output.position = vec4(position.xy * uniforms.translation.xy, 0, 1);
        // var temp_x: f32 = uniforms.translation.x;
        output.position = position;
        // output.position = vec4(output.position.xy + uniforms.translation.xy,  0, 1);
        // output.position = vec4(output.position.xyz + uniforms.translation.xyz, 1);
        output.position = output.position * uniforms.scale;
        output.position = output.position + uniforms.translation;
        // output.position = vec4(output.position.xy * 2, 0, 1);
        return output;
}

@fragment
fn fs_main(fragData: VertexOut) -> @location(0) vec4<f32> {
    return fragData.color;
    // return vec4f(0, 1, 0, 1);
}
