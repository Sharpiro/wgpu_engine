import { shaders as shaderCode } from "./shader_code";

const BOARD_X = 4;
const BOARD_Y = 4;
let fpsHistory: number[] = [];
let time = performance.now();
let clickState = false;
let debugString = "";

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

  const canvasCoords = `(${mouseX.toFixed(2)}, ${mouseY.toFixed(2)})`;
  const glCords = `(${ndcX.toFixed(2)}, ${ndcY.toFixed(2)})`;
  console.log(`${canvasCoords}, ${glCords}`);
  clickState = !clickState;
});

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
  code: shaderCode,
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

function drawTriangleList(
  triangleList: Float32Array,
  triangleTransformations: Float32Array
) {
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

  if (true) {
    const uniformValues = new Float32Array(
      [
        [4.0, 0.0, 0.0, 0.0], // @note: requires 16 byte padding!
        [0.0, 0.0, 1.0, 1.0],
        [0.0, 0.0, 0.0, 0.0],
        [0.0, 0.0, 0.0, 0.0],
      ].flat()
    );
    const uniformBuffer = device.createBuffer({
      size: uniformValues.byteLength,
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    });

    const bindGroup = device.createBindGroup({
      label: "triangle bind group",
      layout: renderPipeline.getBindGroupLayout(0),
      entries: [{ binding: 0, resource: { buffer: uniformBuffer } }],
    });
    device.queue.writeBuffer(uniformBuffer, 0, uniformValues);
    passEncoder.setBindGroup(0, bindGroup);
  }

  passEncoder.setPipeline(renderPipeline);
  passEncoder.setVertexBuffer(0, vertexBuffer);
  passEncoder.draw(triangleList.length / 8);

  passEncoder.end();
  device.queue.submit([commandEncoder.finish()]);
}

type Vec2 = [number, number];

type Vec3 = [number, number, number];

type Vec4 = [number, number, number, number];

type Mat4 = [Vec4, Vec4, Vec4, Vec4];

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

function mat4(diagonal: number): Mat4;
function mat4(diagonal: number | undefined): Mat4 {
  if (diagonal != undefined) {
    return [
      [diagonal, 0.0, 0.0, 0.0],
      [0.0, diagonal, 0.0, 0.0],
      [0.0, 0.0, diagonal, 0.0],
      [0.0, 0.0, 0.0, diagonal],
    ];
  }

  return [
    [0.0, 0.0, 0.0, 0.0],
    [0.0, 0.0, 0.0, 0.0],
    [0.0, 0.0, 0.0, 0.0],
    [0.0, 0.0, 0.0, 0.0],
  ];
}

function getDistance2D(posA: Vec2, posB: Vec2): number {
  return Math.sqrt((posA[0] - posB[0]) ** 2 + (posA[1] - posB[1]) ** 2);
}

function debug(vec: Vec2) {
  return `(${vec[0].toFixed(2)}, ${vec[1].toFixed(2)})`;
}

// class Board {
//   x: number = 4;
//   y: number = 4;
// }

class Triangle2DH {
  x: number;
  y: number;
  transformation: Mat4;

  constructor(x: number, y: number) {
    this.x = x;
    this.y = y;
    this.transformation = mat4(1.0);
  }
}

// class Triangle2DTemp {
//   transformation: Mat4;
//   constructor(transformation: Mat4 = mat4()) {
//     this.transformation = transformation;
//   }
// }

class Triangle2D {
  posA: Vec2 = vec2();
  colorA: Vec4 = vec4();
  posB: Vec2 = vec2();
  colorB: Vec4 = vec4();
  posC: Vec2 = vec2();
  colorC: Vec4 = vec4();

  constructor() {
    this.posA = [-1.0, 1.0];
    this.colorA = [1.0, 0.0, 0.0, 1.0];
    this.posB = [1.0, 1.0];
    this.colorB = [0.0, 1.0, 0.0, 1.0];
    this.posC = [1.0, -1.0];
    this.colorC = [0.0, 0.0, 1.0, 1.0];
  }

  scale(scale: Vec2) {
    this.posA = scaleVec2(this.posA, scale);
    this.posB = scaleVec2(this.posB, scale);
    this.posC = scaleVec2(this.posC, scale);
  }

  translate(translation: Vec2) {
    this.posA = translateVec2(this.posA, translation);
    this.posB = translateVec2(this.posB, translation);
    this.posC = translateVec2(this.posC, translation);
  }

  rotate(theta: number) {
    this.posA = rotateVec2(this.posA, theta);
    this.posB = rotateVec2(this.posB, theta);
    this.posC = rotateVec2(this.posC, theta);
  }

  getVertices(): number[] {
    return [
      ...this.posA,
      0,
      1,
      ...this.colorA,
      ...this.posB,
      0,
      1,
      ...this.colorB,
      ...this.posC,
      0,
      1,
      ...this.colorC,
    ];
  }
}

class Square {
  triangleA: Triangle2D;
  triangleB: Triangle2D;

  constructor() {
    const SCALE: Vec2 = [0.25, 0.25];

    this.triangleA = new Triangle2D();

    this.triangleB = new Triangle2D();
    this.triangleB.rotate(Math.PI);

    this.scale(SCALE);
  }

  translate(vec: Vec2) {
    this.triangleA.translate(vec);
    this.triangleB.translate(vec);
  }

  scale(vec: Vec2) {
    this.triangleA.scale(vec);
    this.triangleB.scale(vec);
  }

  getVertices(): number[] {
    return [...this.triangleA.getVertices(), ...this.triangleB.getVertices()];
  }
}

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

  // @todo: z rotation
  const matrix: Mat4 = [
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
  context2d.fillText(`distance: ${debugString}`, 10, 75);

  /* 3D */

  // const square1 = new Square();
  // const obj1x = new Triangle2DH(1, 1);
  const obj1 = new Triangle2D();
  // square1.translate([-0.75, 0.75]);
  // square1.translate([0.75, -0.75]);
  // const square2 = new Square();
  // square2.translate([-0.25, 0.75]);
  // const square3 = new Square();
  // square3.translate([0.25, 0.75]);
  // const square4 = new Square();
  // square4.translate([0.75, 0.75]);
  // const square5 = new Square();
  // square5.translate([-0.75, 0.25]);
  // const square6 = new Square();
  // square6.translate([-0.25, 0.25]);
  // const square7 = new Square();
  // square7.translate([0.25, 0.25]);
  // const square8 = new Square();
  // square8.translate([0.75, 0.25]);

  const triangleList = new Float32Array(
    [
      obj1.getVertices(),
      // square2.getVertices(),
      // square3.getVertices(),
      // square4.getVertices(),
      // square5.getVertices(),
      // square6.getVertices(),
      // square7.getVertices(),
      // square8.getVertices(),
    ].flat()
  );
  const triangleTransformations = new Float32Array();
  drawTriangleList(triangleList, triangleTransformations);

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
