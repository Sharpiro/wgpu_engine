export const shaders = `

// const BOARD_TILES: f32 = 4;

struct VertexOut {
  @builtin(position) position : vec4f,
  @location(0) color : vec4f
};

struct Uniforms {
  board_tiles: f32,
  test_color: vec4f,
  triangle_scale: vec4f,
  triangle_translation: vec4f,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vertex_main(@location(0) position: vec4f,
               @location(1) color: vec4f) -> VertexOut
{
  var output : VertexOut;
  // output.position = position * uniforms.triangle_scale;
  // output.position = position * uniforms.triangle_scale;
  output.position = vec4(position.xy / uniforms.board_tiles, 0, 1);
  var temp_x: f32 = 2 / uniforms.board_tiles * 0 - 1;
  var temp_y: f32 = 2 / uniforms.board_tiles * 0 + 1;
  // output.position = output.position + vec4(temp_x, 0, 0, 0);
  // output.position =  vec4(temp_x, temp_y, 0, 0);
  output.color = color;
  // output.color = uniforms.test_color;
  return output;
}

@fragment
fn fragment_main(fragData: VertexOut) -> @location(0) vec4f
{
  return fragData.color;
}
`;
