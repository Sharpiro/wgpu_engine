let fpsHistory: number[] = [];
let time = performance.now();
let clickState = false;
// let mouseX = 0;
// let mouseY = 0;

if (!navigator.gpu) {
  throw Error("WebGPU not supported.");
}

const adapter = await navigator.gpu.requestAdapter();
if (!adapter) {
  throw Error("Couldn't request WebGPU adapter.");
}

const device = await adapter.requestDevice();

const canvas = document.querySelector("#gpuCanvas") as HTMLCanvasElement;
const context = canvas.getContext("webgpu")!;
const debugCanvas = document.querySelector("#debugCanvas") as HTMLCanvasElement;
const context2d = debugCanvas.getContext("2d")!;

canvas.addEventListener("click", (event: any) => {
  const rect = canvas.getBoundingClientRect();

  const mouseX = event.clientX - rect.left;
  const mouseY = event.clientY - rect.top;
  const ndcX = (mouseX / canvas.width) * 2 - 1;
  const ndcY = -((mouseY / canvas.height) * 2 - 1);

  console.log(
    `Normalized Device Coordinates: (${ndcX.toFixed(2)}, ${ndcY.toFixed(2)})`
  );
  clickState = !clickState;
});

const shaders = `
struct VertexOut {
  @builtin(position) position : vec4f,
  @location(0) color : vec4f
}

@vertex
fn vertex_main(@location(0) position: vec4f,
               @location(1) color: vec4f) -> VertexOut
{
  var output : VertexOut;
  output.position = position;
  output.color = color;
//   output.color = vec4(0, 1, 0, 0);
  return output;
}

@fragment
fn fragment_main(fragData: VertexOut) -> @location(0) vec4f
{
  return fragData.color;
}
`;

const vertexBuffers = [
  {
    attributes: [
      {
        shaderLocation: 0, // position
        offset: 0,
        format: "float32x4",
      },
      {
        shaderLocation: 1, // color
        offset: 16,
        format: "float32x4",
      },
    ],
    arrayStride: 32,
    stepMode: "vertex",
  },
];

const shaderModule = device.createShaderModule({
  code: shaders,
});

const pipelineDescriptor = {
  vertex: {
    module: shaderModule,
    entryPoint: "vertex_main",
    buffers: vertexBuffers,
  },
  fragment: {
    module: shaderModule,
    entryPoint: "fragment_main",
    targets: [
      {
        format: navigator.gpu.getPreferredCanvasFormat(),
      },
    ],
  },
  primitive: {
    topology: "triangle-list",
  },
  layout: "auto",
};

// @ts-expect-error @todo
const renderPipeline = device.createRenderPipeline(pipelineDescriptor);

// function doGpuStuff(x: number, y: number) {
function drawTriangleList(triangleList: Float32Array) {
  context.configure({
    device: device,
    format: navigator.gpu.getPreferredCanvasFormat(),
    alphaMode: "premultiplied",
  });

  const vertexBuffer = device.createBuffer({
    size: triangleList.byteLength,
    usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
  });

  device.queue.writeBuffer(
    vertexBuffer,
    0,
    triangleList,
    0,
    triangleList.length
  );

  const commandEncoder = device.createCommandEncoder();

  const renderPassDescriptor = {
    colorAttachments: [
      {
        clearValue: { r: 0.0, g: 0.0, b: 0.0, a: 1.0 },
        loadOp: "clear",
        storeOp: "store",
        view: context.getCurrentTexture().createView(),
      },
    ],
  };

  // @ts-expect-error @todo
  const passEncoder = commandEncoder.beginRenderPass(renderPassDescriptor);

  passEncoder.setPipeline(renderPipeline);
  passEncoder.setVertexBuffer(0, vertexBuffer);
  const triangleListLength = triangleList.length / 8;
  passEncoder.draw(triangleListLength);

  passEncoder.end();
  device.queue.submit([commandEncoder.finish()]);
}

type Vec2 = [number, number];

type Vec4 = [number, number, number, number];

// type Vertex2D = {
//   x: number;
//   y: number;
//   z: number;
//   w: number;
//   r: number;
//   g: number;
//   b: number;
//   a: number;
// };

function vec2(): Vec2 {
  return [0, 0];
}

function vec4(): Vec4 {
  return [0, 0, 0, 0];
}

class Triangle2D {
  aPos: Vec2 = vec2();
  aColor: Vec4 = vec4();
  bPos: Vec2 = vec2();
  bColor: Vec4 = vec4();
  cPos: Vec2 = vec2();
  cColor: Vec4 = vec4();

  getVertices(): number[] {
    return [
      ...this.aPos,
      0,
      1,
      ...this.aColor,
      ...this.bPos,
      0,
      1,
      ...this.bColor,
      ...this.cPos,
      0,
      1,
      ...this.cColor,
    ];
  }
}

// class Square {
//   x: number;
//   y: number;
//   width: number;

//   constructor(x: number, y: number, width: number) {
//     this.x = x;
//     this.y = y;
//     this.width = width;
//   }
// }

function translateVec2(point: Vec2, translation: Vec2): Vec2 {
  const pointVec4 = [...point, 0, 1] as const;
  //   const matrix = [
  //     [1.0, 0.0, 0.0, translation[0]],
  //     [0.0, 1.0, 0.0, translation[1]],
  //     [0.0, 0.0, 1.0, 0.0],
  //     [0.0, 0.0, 0.0, 1.0],
  //   ] as const;

  const x = translation[0] + pointVec4[0];
  const y = translation[1] + pointVec4[1];

  return [x, y];
}

function scaleVec2(point: Vec2, scale: Vec2): Vec2 {
  const pointVec4 = [...point, 0, 1] as const;
  //   const matrix = [
  //     [scaleX, 0.0, 0.0, 0.0],
  //     [0.0, scaleY, 0.0, 0.0],
  //     [0.0, 0.0, 1.0, 0.0],
  //     [0.0, 0.0, 0.0, 1.0],
  //   ] as const;

  const x = scale[0] * pointVec4[0];
  const y = scale[1] * pointVec4[1];

  return [x, y];
}

function run() {
  /* 2D */

  const now = performance.now();
  const msPerFrame = now - time;
  const fps = 1_000 / msPerFrame;
  fpsHistory.push(fps);
  if (fpsHistory.length > 120) {
    [, ...fpsHistory] = fpsHistory;
  }
  const fpsAverage = fpsHistory.reduce((a, b) => a + b);
  const fpsDisplay = fpsAverage > 55 ? "60" : "low";
  context2d.clearRect(0, 0, canvas.width, canvas.height);
  context2d.fillStyle = "black";
  context2d.fillRect(0, 0, canvas.width, canvas.height);
  context2d.font = "20px Arial";
  context2d.fillStyle = "white";
  context2d.fillText(`clicked: ${clickState}`, 10, 25);
  context2d.fillText(`fps: ${fpsDisplay}`, 10, 50);

  /* 3D */

  const triangle1 = new Triangle2D();
  triangle1.aPos = [-1.0, 1.0];
  triangle1.aColor = [1.0, 0.0, 0.0, 1.0];
  triangle1.bPos = [1.0, 1.0];
  triangle1.bColor = [0.0, 1.0, 0.0, 1.0];
  triangle1.cPos = [1.0, -1.0];
  triangle1.cColor = [0.0, 0.0, 1.0, 1.0];

  triangle1.aPos = scaleVec2(triangle1.aPos, [0.5, 0.5]);
  triangle1.bPos = scaleVec2(triangle1.bPos, [0.5, 0.5]);
  triangle1.cPos = scaleVec2(triangle1.cPos, [0.5, 0.5]);

  triangle1.aPos = translateVec2(triangle1.aPos, [-0.5, 0.5]);
  triangle1.bPos = translateVec2(triangle1.bPos, [-0.5, 0.5]);
  triangle1.cPos = translateVec2(triangle1.cPos, [-0.5, 0.5]);

  const triangle1Vertices = triangle1.getVertices();

  const triangle2 = new Triangle2D();
  triangle2.aPos = [-1.0, 1.0];
  triangle2.aColor = [1.0, 0.0, 0.0, 1.0];
  triangle2.bPos = [1.0, -1.0];
  triangle2.bColor = [0.0, 1.0, 0.0, 1.0];
  triangle2.cPos = [-1.0, -1.0];
  triangle2.cColor = [0.0, 0.0, 1.0, 1.0];

  triangle2.aPos = scaleVec2(triangle2.aPos, [0.5, 0.5]);
  triangle2.bPos = scaleVec2(triangle2.bPos, [0.5, 0.5]);
  triangle2.cPos = scaleVec2(triangle2.cPos, [0.5, 0.5]);

  triangle2.aPos = translateVec2(triangle2.aPos, [-0.5, 0.5]);
  triangle2.bPos = translateVec2(triangle2.bPos, [-0.5, 0.5]);
  triangle2.cPos = translateVec2(triangle2.cPos, [-0.5, 0.5]);
  const triangle2Vertices = triangle2.getVertices();

  const triangleList = new Float32Array(
    [triangle1Vertices, triangle2Vertices].flat()
  );
  drawTriangleList(triangleList);

  time = now;
  requestAnimationFrame(run);
}

requestAnimationFrame(run);

export default function App() {
  return (
    <>
      <></>
    </>
  );
}
