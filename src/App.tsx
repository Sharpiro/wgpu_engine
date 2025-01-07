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

type Vec3 = [number, number, number];

type Vec4 = [number, number, number, number];

type Mat4 = [Vec4, Vec4, Vec4, Vec4];

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

function vec2(): Vec2;
function vec2(vec: Vec3): Vec2;
function vec2(vec: Vec4): Vec2;
function vec2(vec?: Vec3 | Vec4): Vec2 {
  if (!vec) {
    return [0, 0];
  } else {
    const [x, y] = vec;
    return [x, y];
  }
}

function vec4(): Vec4;
function vec4(vec: Vec2): Vec4;
function vec4(vec: Vec3): Vec4;
function vec4(vec?: Vec2 | Vec3): Vec4 {
  if (!vec) {
    return [0, 0, 0, 0];
  } else if (vec.length == 2) {
    return [...vec, 0, 1];
  } else {
    return [...vec, 1];
  }
}

class Triangle2D {
  aPos: Vec2 = vec2();
  aColor: Vec4 = vec4();
  bPos: Vec2 = vec2();
  bColor: Vec4 = vec4();
  cPos: Vec2 = vec2();
  cColor: Vec4 = vec4();

  constructor() {
    this.aPos = [-1.0, 1.0];
    this.aColor = [1.0, 0.0, 0.0, 1.0];
    this.bPos = [1.0, 1.0];
    this.bColor = [0.0, 1.0, 0.0, 1.0];
    this.cPos = [1.0, -1.0];
    this.cColor = [0.0, 0.0, 1.0, 1.0];
  }

  scale(scale: Vec2) {
    this.aPos = scaleVec2(this.aPos, scale);
    this.bPos = scaleVec2(this.bPos, scale);
    this.cPos = scaleVec2(this.cPos, scale);
  }

  translate(translation: Vec2) {
    this.aPos = translateVec2(this.aPos, translation);
    this.bPos = translateVec2(this.bPos, translation);
    this.cPos = translateVec2(this.cPos, translation);
  }

  rotate(theta: number) {
    this.aPos = rotateVec2(this.aPos, theta);
    this.bPos = rotateVec2(this.bPos, theta);
    this.cPos = rotateVec2(this.cPos, theta);
  }

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

function matMult(point: Vec4, matrix: Mat4): Vec4 {
  const vecResult = vec4();
  for (let i = 0; i < matrix.length; i++) {
    const currMatVec = matrix[i];
    for (let j = 0; j < currMatVec.length; j++) {
      const currMatVecVal = currMatVec[j];
      const currPointVal = point[j];
      const multResult = currMatVecVal * currPointVal;
      vecResult[i] += multResult;
    }
  }
  return vecResult;
}

function translateVec2(point: Vec2, translation: Vec2): Vec2 {
  const matrix: Mat4 = [
    [1.0, 0.0, 0.0, translation[0]],
    [0.0, 1.0, 0.0, translation[1]],
    [0.0, 0.0, 1.0, 0.0],
    [0.0, 0.0, 0.0, 1.0],
  ];

  const translatedPoint = vec2(matMult(vec4(point), matrix));
  return translatedPoint;
}

function scaleVec2(point: Vec2, scale: Vec2): Vec2 {
  const s1 = scale[0];
  const s2 = scale[1];
  const matrix: Mat4 = [
    [s1, 0.0, 0.0, 0.0],
    [0.0, s2, 0.0, 0.0],
    [0.0, 0.0, 1.0, 0.0],
    [0.0, 0.0, 0.0, 1.0],
  ];

  const translatedPoint = vec2(matMult(vec4(point), matrix));
  return translatedPoint;
}

function rotateVec2(point: Vec2, theta: number): Vec2 {
  const cos = Math.cos(theta);
  const sin = Math.sin(theta);

  // @todo: identity
  let matrix: Mat4 = [
    [1.0, 0.0, 0.0, 0.0],
    [0.0, 1.0, 0.0, 0.0],
    [0.0, 0.0, 1.0, 0.0],
    [0.0, 0.0, 0.0, 1.0],
  ];
  // @todo: z rotation
  matrix = [
    [cos, -sin, 0.0, 0.0],
    [sin, cos, 0.0, 0.0],
    [0.0, 0.0, 1.0, 0.0],
    [0.0, 0.0, 0.0, 1.0],
  ];

  // @todo: y rotation
  // matrix = [
  //   [cos, 0.0, sin, 0.0],
  //   [0.0, 1.0, 0.0, 0.0],
  //   [-sin, 0.0, cos, 0.0],
  //   [0.0, 0.0, 0.0, 1.0],
  // ];

  // @todo: x rotation
  // matrix = [
  //   [1.0, 0.0, 0.0, 0.0],
  //   [0.0, cos, -sin, 0.0],
  //   [0.0, sin, cos, 0.0],
  //   [0.0, 0.0, 0.0, 1.0],
  // ];

  const translatedPoint = vec2(matMult(vec4(point), matrix));
  return translatedPoint;
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
  triangle1.scale([0.5, 0.5]);
  triangle1.translate([-0.5, 0.5]);

  const triangle1Vertices = triangle1.getVertices();

  const triangle2 = new Triangle2D();
  triangle2.scale([0.5, 0.5]);
  // triangle2.rotate(-Math.PI / 1.5);
  triangle2.rotate(now / 1000);
  // triangle2.translate([-0.5, 0.5]);

  const triangle2Vertices = triangle2.getVertices();

  const triangleList = new Float32Array(
    [
      //
      // triangle1Vertices,
      triangle2Vertices,
    ].flat()
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
