const svgNS = "http://www.w3.org/2000/svg";
const CANVAS_WIDTH = 1200;
const CANVAS_HEIGHT = 900;
const artwork = document.querySelector("#artwork");
const drawing = document.querySelector("#drawing");
const layers = [
  { name: "Layer 1", visible: true, shapes: [] }
];

let activeLayer = 0;
let activeTool = "select";
let currentSelectMode = "select"; // "select" hoặc "group_select"
let selected = null;
let selectedItems = []; // Mảng chứa các { layer, shape } khi chọn nhiều hình
let showRulers = localStorage.getItem("mini-vector-cad-show-rulers") !== "false";
let snapToGrid = false;
let pointerAction = null;
let previewShape = null;
let copiedShape = null;
let zoom = 100;
let renderFrame = 0;
let brushWidth = 6;
let brushOpacity = 1;
let eraserWidth = 18;
let brushColor = "#334155";
let theme = localStorage.getItem("mini-vector-cad-theme") === "light" ? "light" : "dark";
let documentTitle = localStorage.getItem("mini-vector-cad-title") || "Untitled";
let backendConnected = false;
let backendSyncTimer = 0;
let backendSyncInProgress = false;
let backendSyncPending = false;
let backendReconnectTimer = 0;
let hasPendingLocalChanges = false;
let projectRevision = 0;
let layerSearchQuery = "";
const undoStack = [];
const redoStack = [];

function wavePathData(x1, y1, x2, y2, amplitude = 15, cycles = 3) {
  const dx = x2 - x1;
  const dy = y2 - y1;
  const L = Math.hypot(dx, dy);
  if (L < 1e-4) return `M ${x1} ${y1} L ${x2} ${y2}`;
  const ux = dx / L, uy = dy / L;
  const nx = -uy, ny = ux;
  const halfWaves = Math.max(2, Math.round(cycles) * 2);
  const step = L / halfWaves;
  let d = `M ${x1} ${y1}`;
  for (let i = 0; i < halfWaves; i++) {
    const sign = (i % 2 === 0) ? 1 : -1;
    const endDist = (i + 1) * step;
    const ctrlDist = (i + 0.5) * step;
    const endX = x1 + endDist * ux;
    const endY = y1 + endDist * uy;
    const ctrlX = x1 + ctrlDist * ux + sign * (amplitude * 2) * nx;
    const ctrlY = y1 + ctrlDist * uy + sign * (amplitude * 2) * ny;
    d += ` Q ${ctrlX} ${ctrlY} ${endX} ${endY}`;
  }
  return d;
}

function starPoints(cx, cy, rx, ry) {
  const pts = [];
  const innerRx = rx * 0.4;
  const innerRy = ry * 0.4;
  for (let i = 0; i < 10; i++) {
    const angle = -Math.PI / 2 + (i * Math.PI) / 5;
    const rX = i % 2 === 0 ? rx : innerRx;
    const rY = i % 2 === 0 ? ry : innerRy;
    pts.push([
      Math.round((cx + rX * Math.cos(angle)) * 10) / 10,
      Math.round((cy + rY * Math.sin(angle)) * 10) / 10
    ]);
  }
  return pts;
}

function arrowPathData(x1, y1, x2, y2, headLength = 16) {
  const dx = x2 - x1;
  const dy = y2 - y1;
  const L = Math.hypot(dx, dy);
  if (L < 1e-4) return `M ${x1} ${y1} L ${x2} ${y2}`;
  const ux = dx / L;
  const uy = dy / L;
  const nx = -uy;
  const ny = ux;
  const actualHead = Math.min(headLength, Math.max(8, L * 0.35));
  const headWidth = actualHead * 0.52;
  const bx = x2 - actualHead * ux;
  const by = y2 - actualHead * uy;
  const c1x = bx + headWidth * nx;
  const c1y = by + headWidth * ny;
  const c2x = bx - headWidth * nx;
  const c2y = by - headWidth * ny;
  return `M ${x1} ${y1} L ${x2} ${y2} M ${c1x} ${c1y} L ${x2} ${y2} L ${c2x} ${c2y}`;
}

function calligraphyPathData(points, width) {
  if (!points || !points.length) return "";
  const w = Math.max(1, width);
  if (points.length === 1) {
    const [x, y] = points[0];
    const half = w * 0.5;
    return `M ${x - half * 0.707} ${y - half * 0.707} L ${x + half * 0.707} ${y + half * 0.707} Z`;
  }
  const angle = Math.PI / 4; // Góc nghiêng 45 độ
  const vx = (w * 0.5) * Math.cos(angle);
  const vy = (w * 0.5) * Math.sin(angle);
  const topPts = [];
  const botPts = [];
  for (const [x, y] of points) {
    topPts.push([x + vx, y + vy]);
    botPts.push([x - vx, y - vy]);
  }
  let d = `M ${topPts[0][0]} ${topPts[0][1]}`;
  for (let i = 1; i < topPts.length; i++) {
    d += ` L ${topPts[i][0]} ${topPts[i][1]}`;
  }
  for (let i = botPts.length - 1; i >= 0; i--) {
    d += ` L ${botPts[i][0]} ${botPts[i][1]}`;
  }
  d += " Z";
  return d;
}

function taperPathData(points, width) {
  if (!points || !points.length) return "";
  const w = Math.max(1, width);
  if (points.length === 1) {
    const [x, y] = points[0];
    const r = Math.max(0.5, w * 0.15);
    return `M ${x - r} ${y} A ${r} ${r} 0 1 0 ${x + r} ${y} A ${r} ${r} 0 1 0 ${x - r} ${y} Z`;
  }
  const n = points.length;
  const dists = [0];
  let totalDist = 0;
  for (let i = 1; i < n; i++) {
    const segDist = Math.hypot(points[i][0] - points[i - 1][0], points[i][1] - points[i - 1][1]);
    totalDist += segDist;
    dists.push(totalDist);
  }
  if (totalDist < 1e-3) {
    const [x, y] = points[0];
    const r = Math.max(0.5, w * 0.15);
    return `M ${x - r} ${y} A ${r} ${r} 0 1 0 ${x + r} ${y} A ${r} ${r} 0 1 0 ${x - r} ${y} Z`;
  }

  function factor(s) {
    const taperPortion = 0.28;
    if (s < taperPortion) {
      return 0.12 + 0.88 * Math.sin((s / taperPortion) * (Math.PI / 2));
    } else if (s > 1 - taperPortion) {
      return 0.12 + 0.88 * Math.sin(((1 - s) / taperPortion) * (Math.PI / 2));
    }
    return 1.0;
  }

  const leftPts = [];
  const rightPts = [];

  for (let i = 0; i < n; i++) {
    const s = dists[i] / totalDist;
    const r = Math.max(0.5, (w / 2) * factor(s));

    let tx, ty;
    if (i === 0) {
      tx = points[1][0] - points[0][0];
      ty = points[1][1] - points[0][1];
    } else if (i === n - 1) {
      tx = points[n - 1][0] - points[n - 2][0];
      ty = points[n - 1][1] - points[n - 2][1];
    } else {
      tx = points[i + 1][0] - points[i - 1][0];
      ty = points[i + 1][1] - points[i - 1][1];
    }
    const len = Math.hypot(tx, ty) || 1;
    const nx = -ty / len;
    const ny = tx / len;

    leftPts.push([points[i][0] + nx * r, points[i][1] + ny * r]);
    rightPts.push([points[i][0] - nx * r, points[i][1] - ny * r]);
  }

  let d = `M ${leftPts[0][0]} ${leftPts[0][1]}`;
  for (let i = 1; i < leftPts.length; i++) {
    d += ` L ${leftPts[i][0]} ${leftPts[i][1]}`;
  }
  const endR = Math.max(0.5, (w / 2) * factor(1));
  d += ` A ${endR} ${endR} 0 0 1 ${rightPts[n - 1][0]} ${rightPts[n - 1][1]}`;

  for (let i = rightPts.length - 2; i >= 0; i--) {
    d += ` L ${rightPts[i][0]} ${rightPts[i][1]}`;
  }
  const startR = Math.max(0.5, (w / 2) * factor(0));
  d += ` A ${startR} ${startR} 0 0 1 ${leftPts[0][0]} ${leftPts[0][1]}`;
  d += " Z";
  return d;
}

let activeBrushType = "round";
const brushLabels = {
  round: "Cọ vẽ",
  calligraphy: "Bút lông",
  taper: "Bút vuốt nét"
};

const toolLabels = {
  select: "Chọn",
  group_select: "Quét chọn nhóm",
  rect: "Hình chữ nhật",
  circle: "Hình tròn",
  triangle: "Tam giác",
  star: "Ngôi sao",
  ellipse: "Hình elip",
  parallelogram: "Hình bình hành",
  trapezoid: "Hình thang",
  line: "Đoạn thẳng",
  arrow: "Mũi tên",
  curve: "Đoạn cong",
  wave: "Đoạn sóng",
  brush: "Nét bút",
  text: "Văn bản",
  image: "Ảnh"
};

let currentTextFontFamily = "Segoe UI";
let currentTextFontSize = 24;
let currentTextColor = "#000000";
let currentTextBold = false;
let currentTextItalic = false;
let currentTextUnderline = false;
let currentEditingTextTarget = null;

function calcFitFontSize(text, boxWidth, boxHeight, isBold = false) {
  const lines = (text || "Aa").split("\n");
  const maxLineLen = Math.max(...lines.map(l => l.length), 1);
  const numLines = Math.max(lines.length, 1);

  const charWidthFactor = isBold ? 0.70 : 0.60;
  const lineHeightFactor = 1.25;

  const availableW = Math.max(8, boxWidth - 6);
  const availableH = Math.max(8, boxHeight - 4);

  const maxByWidth = availableW / (maxLineLen * charWidthFactor);
  const maxByHeight = availableH / (numLines * lineHeightFactor);

  const fitSize = Math.min(maxByWidth, maxByHeight);
  return Math.max(10, Math.min(220, Math.round(fitSize)));
}

function snapshot(syncBackend = true) {
  undoStack.push(JSON.stringify({ layers, activeLayer }));
  if (undoStack.length > 80) undoStack.shift();
  redoStack.length = 0;
  updateHistoryButtons();
  if (syncBackend) scheduleCppSync();
}

function restore(snapshotText) {
  const state = JSON.parse(snapshotText);
  layers.splice(0, layers.length, ...state.layers);
  activeLayer = Math.min(state.activeLayer, layers.length - 1);
  selected = null;
  render();
  updateHistoryButtons();
  scheduleCppSync();
}

function updateHistoryButtons() {
  document.querySelector("#undo").disabled = undoStack.length === 0;
  document.querySelector("#redo").disabled = redoStack.length === 0;
}

function makeSvg(tag, attrs) {
  const element = document.createElementNS(svgNS, tag);
  for (const [key, value] of Object.entries(attrs)) element.setAttribute(key, value);
  return element;
}

function shapeElement(shape, layerIndex, shapeIndex) {
  let element;
  const bounds = shapeBounds(shape);
  const centerX = bounds.x + bounds.w / 2;
  const centerY = bounds.y + bounds.h / 2;
  const common = {
    class: "shape",
    "data-layer": layerIndex,
    "data-shape": shapeIndex,
    stroke: shape.stroke,
    "stroke-width": shape.width,
    "stroke-linejoin": "round",
    "stroke-linecap": "round",
    fill: ["line", "curve", "wave", "arrow"].includes(shape.type) ? "none" : shape.fill,
    transform: `rotate(${shape.rotation || 0} ${centerX} ${centerY})`
  };
  if (shape.type === "rect") {
    element = makeSvg("rect", { ...common, x: shape.x, y: shape.y, width: shape.w, height: shape.h, rx: 2 });
  } else if (shape.type === "circle") {
    element = makeSvg("circle", { ...common, cx: shape.x, cy: shape.y, r: shape.r });
  } else if (shape.type === "ellipse") {
    element = makeSvg("ellipse", { ...common, cx: shape.cx, cy: shape.cy, rx: shape.rx, ry: shape.ry });
  } else if (shape.type === "line") {
    element = makeSvg("line", { ...common, x1: shape.x1, y1: shape.y1, x2: shape.x2, y2: shape.y2 });
  } else if (shape.type === "arrow") {
    element = makeSvg("path", {
      ...common,
      d: arrowPathData(shape.x1, shape.y1, shape.x2, shape.y2),
      fill: "none"
    });
  } else if (shape.type === "curve") {
    element = makeSvg("path", {
      ...common,
      d: `M ${shape.x1} ${shape.y1} Q ${shape.cx} ${shape.cy} ${shape.x2} ${shape.y2}`,
      fill: "none"
    });
  } else if (shape.type === "wave") {
    element = makeSvg("path", {
      ...common,
      d: wavePathData(shape.x1, shape.y1, shape.x2, shape.y2, shape.amplitude || 15, shape.cycles || 3),
      fill: "none"
    });
  } else if (shape.type === "brush") {
    const style = shape.brushStyle || "round";
    if (style === "calligraphy") {
      element = makeSvg("path", {
        ...common,
        d: calligraphyPathData(shape.points, shape.width),
        fill: shape.stroke,
        stroke: shape.stroke,
        "stroke-width": 0.5,
        "stroke-linejoin": "round",
        opacity: shape.opacity
      });
    } else if (style === "taper") {
      element = makeSvg("path", {
        ...common,
        d: taperPathData(shape.points, shape.width),
        fill: shape.stroke,
        stroke: shape.stroke,
        "stroke-width": 0.5,
        "stroke-linejoin": "round",
        opacity: shape.opacity
      });
    } else {
      element = makeSvg("path", {
        ...common,
        d: pathData(shape.points),
        fill: "none",
        opacity: shape.opacity
      });
    }
  } else if (shape.type === "image") {
    element = makeSvg("image", {
      class: common.class,
      "data-layer": common["data-layer"],
      "data-shape": common["data-shape"],
      x: shape.x, y: shape.y,
      width: shape.w, height: shape.h,
      href: shape.src,
      opacity: shape.opacity ?? 1,
      preserveAspectRatio: "none",
      transform: common.transform
    });
  } else if (shape.type === "text") {
    const isEditing = currentEditingTextTarget && currentEditingTextTarget.shape === shape;
    element = makeSvg("text", {
      class: common.class,
      "data-layer": common["data-layer"],
      "data-shape": common["data-shape"],
      x: shape.x,
      y: shape.y,
      fill: shape.color || "#000000",
      "font-size": `${shape.fontSize || 24}px`,
      "font-family": shape.fontFamily || "Segoe UI, sans-serif",
      "font-weight": shape.bold ? "bold" : "normal",
      "font-style": shape.italic ? "italic" : "normal",
      "text-decoration": shape.underline ? "underline" : "none",
      "dominant-baseline": "hanging",
      opacity: isEditing ? 0 : (shape.opacity ?? 1),
      visibility: isEditing ? "hidden" : "visible",
      transform: common.transform,
      style: isEditing ? "display: none;" : "user-select: none; cursor: move;"
    });
    const lines = (shape.text || " ").split("\n");
    const lineHeight = (shape.fontSize || 24) * 1.25;
    lines.forEach((line, i) => {
      const tspan = makeSvg("tspan", {
        x: shape.x,
        dy: i === 0 ? 0 : lineHeight
      });
      tspan.textContent = line || " ";
      element.append(tspan);
    });

    element.addEventListener("dblclick", event => {
      event.stopPropagation();
      openInlineTextEditor(shape, layerIndex, shapeIndex);
    });
  } else if (shape.type === "group") {
    element = makeSvg("g", {
      class: common.class,
      "data-layer": common["data-layer"],
      "data-shape": common["data-shape"],
      transform: common.transform
    });
    (shape.children || []).forEach(child => {
      const childEl = shapeElement(child, layerIndex, -1);
      childEl.removeAttribute("class");
      childEl.removeAttribute("data-layer");
      childEl.removeAttribute("data-shape");
      element.append(childEl);
    });
  } else {
    element = makeSvg("polygon", { ...common, points: shape.points.map(point => point.join(",")).join(" ") });
  }
  const isThisSelected = (selected && selected.layer === layerIndex && selected.shape === shapeIndex) ||
    selectedItems.some(item => item.layer === layerIndex && item.shape === shapeIndex);
  if (isThisSelected) element.classList.add("selected");
  element.addEventListener("pointerdown", event => {
    if (event.button !== 0) return;
    if (activeTool !== "select" && activeTool !== "group_select") return;
    event.stopPropagation();
    event.preventDefault();

    const clickedItem = { layer: layerIndex, shape: shapeIndex };
    const isAlreadyInSelection = selectedItems.some(i => i.layer === layerIndex && i.shape === shapeIndex);

    if (event.shiftKey) {
      if (isAlreadyInSelection) {
        selectedItems = selectedItems.filter(i => !(i.layer === layerIndex && i.shape === shapeIndex));
        selected = selectedItems[0] || null;
      } else {
        selectedItems.push(clickedItem);
        selected = clickedItem;
      }
      activeLayer = layerIndex;
      render();
      setStatus(`Đã chọn ${selectedItems.length} hình`);
      return;
    }

    if (!isAlreadyInSelection) {
      selected = clickedItem;
      selectedItems = [clickedItem];
      activeLayer = layerIndex;
    } else {
      selected = clickedItem;
      activeLayer = layerIndex;
    }

    if (layers[layerIndex].locked) {
      setStatus("Layer đang bị khóa, không thể di chuyển hình.");
      return;
    }

    if (selectedItems.length > 1) {
      pointerAction = {
        mode: "move_multi",
        items: selectedItems.map(item => ({
          layer: item.layer,
          shape: item.shape,
          original: structuredClone(layers[item.layer].shapes[item.shape])
        })),
        start: pointFromEvent(event),
        moved: false
      };
    } else {
      pointerAction = {
        mode: "move",
        layer: layerIndex,
        shape: shapeIndex,
        start: pointFromEvent(event),
        original: structuredClone(shape),
        moved: false
      };
    }
    drawing.setPointerCapture(event.pointerId);
    render();
    setStatus("Đang kéo hình — thả chuột để đặt vào vị trí mới");
  });
  return element;
}

function shapeBounds(shape) {
  if (shape.type === "group") {
    if (!shape.children || !shape.children.length) return { x: 0, y: 0, w: 1, h: 1 };
    const bboxes = shape.children.map(c => shapeBounds(c));
    const minX = Math.min(...bboxes.map(b => b.x));
    const minY = Math.min(...bboxes.map(b => b.y));
    const maxX = Math.max(...bboxes.map(b => b.x + b.w));
    const maxY = Math.max(...bboxes.map(b => b.y + b.h));
    return { x: minX, y: minY, w: Math.max(1, maxX - minX), h: Math.max(1, maxY - minY) };
  }
  if (shape.type === "rect" || shape.type === "image") return { x: shape.x, y: shape.y, w: shape.w, h: shape.h };
  if (shape.type === "text") {
    const lines = (shape.text || " ").split("\n");
    const fontSize = shape.fontSize || 24;
    const maxLineLen = Math.max(...lines.map(l => l.length), 1);
    const w = Math.max(20, Math.round(maxLineLen * fontSize * 0.58));
    const h = Math.max(fontSize, Math.round(lines.length * fontSize * 1.25));
    return { x: shape.x, y: shape.y, w, h };
  }
  if (shape.type === "circle") return { x: shape.x - shape.r, y: shape.y - shape.r, w: shape.r * 2, h: shape.r * 2 };
  if (shape.type === "ellipse") return { x: shape.cx - shape.rx, y: shape.cy - shape.ry, w: shape.rx * 2, h: shape.ry * 2 };
  if (shape.type === "curve") {
    const xs = [shape.x1, shape.cx, shape.x2];
    const ys = [shape.y1, shape.cy, shape.y2];
    const minX = Math.min(...xs), minY = Math.min(...ys);
    return { x: minX, y: minY, w: Math.max(1, Math.max(...xs) - minX), h: Math.max(1, Math.max(...ys) - minY) };
  }
  if (shape.type === "wave") {
    const dx = shape.x2 - shape.x1, dy = shape.y2 - shape.y1;
    const L = Math.hypot(dx, dy) || 1;
    const nx = -dy / L, ny = dx / L;
    const amp = shape.amplitude || 15;
    const xs = [shape.x1 - nx * amp, shape.x1 + nx * amp, shape.x2 - nx * amp, shape.x2 + nx * amp];
    const ys = [shape.y1 - ny * amp, shape.y1 + ny * amp, shape.y2 - ny * amp, shape.y2 + ny * amp];
    const minX = Math.min(...xs), minY = Math.min(...ys);
    return { x: minX, y: minY, w: Math.max(1, Math.max(...xs) - minX), h: Math.max(1, Math.max(...ys) - minY) };
  }
  const points = (shape.type === "line" || shape.type === "arrow") ? [[shape.x1, shape.y1], [shape.x2, shape.y2]] : shape.points;
  const xs = points.map(point => point[0]);
  const ys = points.map(point => point[1]);
  const minX = Math.min(...xs);
  const minY = Math.min(...ys);
  return { x: minX, y: minY, w: Math.max(...xs) - minX, h: Math.max(...ys) - minY };
}

function multiSelectionHandles(items) {
  const shapes = items.map(item => layers[item.layer]?.shapes[item.shape]).filter(Boolean);
  if (!shapes.length) return;
  const bboxes = shapes.map(s => shapeBounds(s));
  const minX = Math.min(...bboxes.map(b => b.x));
  const minY = Math.min(...bboxes.map(b => b.y));
  const maxX = Math.max(...bboxes.map(b => b.x + b.w));
  const maxY = Math.max(...bboxes.map(b => b.y + b.h));
  const w = maxX - minX;
  const h = maxY - minY;

  const controlGroup = makeSvg("g", { class: "selection-controls multi-selection-controls" });
  controlGroup.append(makeSvg("rect", {
    class: "multi-selection-bounds",
    x: minX, y: minY, width: w, height: h,
    fill: "rgba(56, 189, 248, 0.08)", "pointer-events": "none"
  }));
  artwork.append(controlGroup);
}

function selectionHandles(shape) {
  const bounds = shapeBounds(shape);
  const centerX = bounds.x + bounds.w / 2;
  const centerY = bounds.y + bounds.h / 2;
  const controlGroup = makeSvg("g", {
    class: "selection-controls",
    transform: `rotate(${shape.rotation || 0} ${centerX} ${centerY})`
  });
  controlGroup.append(makeSvg("rect", {
    class: "selection-bounds",
    x: bounds.x, y: bounds.y, width: bounds.w, height: bounds.h,
    fill: "none", "pointer-events": "none"
  }));
  controlGroup.append(makeSvg("line", {
    class: "rotation-stem",
    x1: centerX, y1: bounds.y, x2: centerX, y2: bounds.y - 25,
    "pointer-events": "none"
  }));
  addHandle("rotate", centerX, bounds.y - 25, controlGroup);
  if (shape.type === "line" || shape.type === "wave" || shape.type === "arrow") {
    [["start", shape.x1, shape.y1], ["end", shape.x2, shape.y2]].forEach(([handle, x, y]) => {
      addHandle(handle, x, y, controlGroup);
    });
  } else if (shape.type === "curve") {
    controlGroup.append(makeSvg("line", {
      x1: shape.x1, y1: shape.y1, x2: shape.cx, y2: shape.cy,
      stroke: "#94a3b8", "stroke-width": 1, "stroke-dasharray": "3 3", "pointer-events": "none"
    }));
    controlGroup.append(makeSvg("line", {
      x1: shape.x2, y1: shape.y2, x2: shape.cx, y2: shape.cy,
      stroke: "#94a3b8", "stroke-width": 1, "stroke-dasharray": "3 3", "pointer-events": "none"
    }));
    [["start", shape.x1, shape.y1], ["control", shape.cx, shape.cy], ["end", shape.x2, shape.y2]].forEach(([handle, x, y]) => {
      addHandle(handle, x, y, controlGroup);
    });
  } else if (shape.type === "circle") {
    addHandle("radius", shape.x + shape.r, shape.y, controlGroup);
  } else {
    [
      ["nw", bounds.x, bounds.y],
      ["ne", bounds.x + bounds.w, bounds.y],
      ["sw", bounds.x, bounds.y + bounds.h],
      ["se", bounds.x + bounds.w, bounds.y + bounds.h]
    ].forEach(([handle, x, y]) => addHandle(handle, x, y, controlGroup));
  }
  artwork.append(controlGroup);
}

function addHandle(name, x, y, parent) {
  const handle = makeSvg("circle", {
    class: `resize-handle${name === "rotate" ? " rotation-handle" : ""}`,
    cx: x, cy: y, r: name === "rotate" ? 7 : 6, "data-handle": name
  });
  handle.addEventListener("pointerdown", event => {
    if (event.button !== 0 || !selected) return;
    event.stopPropagation();
    event.preventDefault();
    const shape = layers[selected.layer].shapes[selected.shape];
    if (layers[selected.layer].locked) {
      setStatus("Layer đang bị khóa, không thể chỉnh sửa hình.");
      return;
    }
    const point = pointFromEvent(event);
    const bounds = shapeBounds(shape);
    const center = { x: bounds.x + bounds.w / 2, y: bounds.y + bounds.h / 2 };
    pointerAction = {
      mode: name === "rotate" ? "rotate" : "resize",
      handle: name,
      layer: selected.layer,
      shape: selected.shape,
      start: point,
      center,
      startAngle: Math.atan2(point.y - center.y, point.x - center.x),
      initialRotation: shape.rotation || 0,
      original: structuredClone(shape),
      moved: false
    };
    drawing.setPointerCapture(event.pointerId);
  });
  const title = document.createElementNS(svgNS, "title");
  title.textContent = name === "rotate" ? "Kéo để xoay" : "Kéo để chỉnh kích thước";
  handle.append(title);
  parent.append(handle);
}

function render() {
  document.documentElement.dataset.theme = theme;
  artwork.replaceChildren();
  layers.forEach((layer, layerIndex) => {
    if (!layer.visible) return;
    const group = makeSvg("g", { "data-layer": layerIndex });
    layer.shapes.forEach((shape, shapeIndex) => group.append(shapeElement(shape, layerIndex, shapeIndex)));
    artwork.append(group);
  });
  if (selectedItems.length > 1) {
    multiSelectionHandles(selectedItems);
  } else if (selected && layers[selected.layer]?.visible) {
    const selectedShape = layers[selected.layer].shapes[selected.shape];
    const isEditingThis = currentEditingTextTarget && currentEditingTextTarget.shape === selectedShape;
    if (selectedShape && !isEditingThis) selectionHandles(selectedShape);
  }
  renderPreview();
  renderLayers();
  renderProperties();
  renderSelectionToolbar();
  renderRulers();
  document.querySelector("#canvas-hint").textContent = activeTool === "select" || activeTool === "group_select"
    ? selectedItems.length > 1
      ? `Đã chọn ${selectedItems.length} hình — Kéo để di chuyển cả nhóm hoặc bấm Nhóm (Ctrl+G)`
      : selected ? "Kéo hình để di chuyển · nút góc để đổi kích thước · nút trên để xoay" : "Chọn công cụ hình để bắt đầu vẽ · Hoặc quét chuột để chọn nhiều hình"
    : activeTool === "eraser" ? "Kéo trên nét vẽ hoặc hình để gôm"
      : activeTool === "brush" ? "Giữ chuột trái và kéo để vẽ nét tự do"
        : `Kéo trên canvas để vẽ ${toolLabels[activeTool].toLowerCase()}`;
  drawing.classList.toggle("draw-mode", activeTool !== "select" && activeTool !== "group_select");
}

function scheduleRender() {
  if (renderFrame) return;
  renderFrame = requestAnimationFrame(() => {
    renderFrame = 0;
    render();
  });
}

function flushRender() {
  if (renderFrame) {
    cancelAnimationFrame(renderFrame);
    renderFrame = 0;
  }
  render();
}

function renderSelectionToolbar() {
  const toolbar = document.querySelector("#selection-toolbar");
  const shape = selected && layers[selected.layer]?.shapes[selected.shape];
  const hasMulti = selectedItems.length > 1;
  toolbar.hidden = !shape && !hasMulti;
  document.querySelector("#paste-shape").disabled = !copiedShape;

  const groupBtn = document.querySelector("#group-shapes");
  const ungroupBtn = document.querySelector("#ungroup-shapes");
  if (groupBtn) groupBtn.hidden = !hasMulti;
  if (ungroupBtn) ungroupBtn.hidden = !(shape && shape.type === "group");

  if (!shape && !hasMulti) return;

  let leftEdge = 0, rightEdge = 0, topEdge = 0, bottomEdge = 0;
  const stage = document.querySelector("#canvas-stage").getBoundingClientRect();

  if (hasMulti) {
    const validElements = selectedItems
      .map(item => artwork.querySelector(`.shape[data-layer="${item.layer}"][data-shape="${item.shape}"]`))
      .filter(Boolean);
    if (!validElements.length) return;
    const allCorners = [];
    validElements.forEach(el => {
      const b = el.getBBox();
      const m = el.getScreenCTM();
      if (!m) return;
      [[b.x, b.y], [b.x + b.width, b.y], [b.x, b.y + b.height], [b.x + b.width, b.y + b.height]].forEach(([x, y]) => {
        const pt = drawing.createSVGPoint();
        pt.x = x; pt.y = y;
        allCorners.push(pt.matrixTransform(m));
      });
    });
    if (!allCorners.length) return;
    leftEdge = Math.min(...allCorners.map(p => p.x)) - stage.left;
    rightEdge = Math.max(...allCorners.map(p => p.x)) - stage.left;
    topEdge = Math.min(...allCorners.map(p => p.y)) - stage.top;
    bottomEdge = Math.max(...allCorners.map(p => p.y)) - stage.top;
  } else {
    const shapeElement = artwork.querySelector(
      `.shape[data-layer="${selected.layer}"][data-shape="${selected.shape}"]`
    );
    if (!shapeElement) return;
    const bounds = shapeElement.getBBox();
    const matrix = shapeElement.getScreenCTM();
    if (!matrix) return;
    const corners = [
      [bounds.x, bounds.y],
      [bounds.x + bounds.width, bounds.y],
      [bounds.x, bounds.y + bounds.height],
      [bounds.x + bounds.width, bounds.y + bounds.height]
    ].map(([x, y]) => {
      const point = drawing.createSVGPoint();
      point.x = x;
      point.y = y;
      return point.matrixTransform(matrix);
    });
    leftEdge = Math.min(...corners.map(point => point.x)) - stage.left;
    rightEdge = Math.max(...corners.map(point => point.x)) - stage.left;
    topEdge = Math.min(...corners.map(point => point.y)) - stage.top;
    bottomEdge = Math.max(...corners.map(point => point.y)) - stage.top;
  }

  const toolbarWidth = toolbar.offsetWidth || 300;
  const toolbarHeight = toolbar.offsetHeight || 38;
  const gap = 12;
  const left = Math.max(8, Math.min(
    (leftEdge + rightEdge - toolbarWidth) / 2,
    stage.width - toolbarWidth - 8
  ));
  const top = topEdge >= toolbarHeight + gap
    ? topEdge - toolbarHeight - gap
    : Math.min(bottomEdge + gap, stage.height - toolbarHeight - 8);
  toolbar.style.left = `${left}px`;
  toolbar.style.top = `${Math.max(8, top)}px`;
  toolbar.style.transform = "none";
}

function renderPreview() {
  const previous = artwork.querySelector(".drawing-preview");
  if (previous) previous.remove();
  if (!previewShape) return;
  const shape = previewShape;
  const common = {
    fill: shape.type === "line" ? "none" : shape.fill,
    stroke: shape.stroke,
    "stroke-width": shape.width,
    "stroke-dasharray": "6 4",
    opacity: "0.75",
    "pointer-events": "none"
  };
  let preview;
  if (shape.type === "brush") {
    const style = shape.brushStyle || activeBrushType || "round";
    if (style === "calligraphy") {
      preview = makeSvg("path", {
        ...common,
        d: calligraphyPathData(shape.points, shape.width),
        fill: shape.stroke,
        stroke: shape.stroke,
        "stroke-width": 0.5,
        "stroke-linejoin": "round",
        opacity: shape.opacity
      });
    } else if (style === "taper") {
      preview = makeSvg("path", {
        ...common,
        d: taperPathData(shape.points, shape.width),
        fill: shape.stroke,
        stroke: shape.stroke,
        "stroke-width": 0.5,
        "stroke-linejoin": "round",
        opacity: shape.opacity
      });
    } else {
      preview = makeSvg("path", { ...common, d: pathData(shape.points), fill: "none", opacity: shape.opacity });
    }
    preview.removeAttribute("stroke-dasharray");
  } else if (shape.type === "rect") {
    preview = makeSvg("rect", { ...common, x: shape.x, y: shape.y, width: shape.w, height: shape.h });
  } else if (shape.type === "circle") {
    preview = makeSvg("circle", { ...common, cx: shape.x, cy: shape.y, r: shape.r });
  } else if (shape.type === "ellipse") {
    preview = makeSvg("ellipse", { ...common, cx: shape.cx, cy: shape.cy, rx: shape.rx, ry: shape.ry });
  } else if (shape.type === "line") {
    preview = makeSvg("line", { ...common, x1: shape.x1, y1: shape.y1, x2: shape.x2, y2: shape.y2 });
  } else if (shape.type === "curve") {
    preview = makeSvg("path", { ...common, d: `M ${shape.x1} ${shape.y1} Q ${shape.cx} ${shape.cy} ${shape.x2} ${shape.y2}`, fill: "none" });
  } else if (shape.type === "wave") {
    preview = makeSvg("path", { ...common, d: wavePathData(shape.x1, shape.y1, shape.x2, shape.y2, shape.amplitude || 15, shape.cycles || 3), fill: "none" });
  } else if (shape.type === "arrow") {
    preview = makeSvg("path", { ...common, d: arrowPathData(shape.x1, shape.y1, shape.x2, shape.y2), fill: "none" });
  } else if (shape.type === "star") {
    preview = makeSvg("polygon", { ...common, points: shape.points.map(point => point.join(",")).join(" ") });
  } else if (shape.type === "text_box_preview") {
    const g = makeSvg("g", { class: "drawing-preview" });
    const rect = makeSvg("rect", {
      x: shape.x, y: shape.y, width: shape.w, height: shape.h,
      fill: "none",
      stroke: theme === "dark" ? "#84cc16" : "#2563eb",
      "stroke-width": 1.5,
      "stroke-dasharray": "5 3",
      rx: 2
    });
    const fitSize = calcFitFontSize("Văn bản mới", shape.w, shape.h, currentTextBold);
    const text = makeSvg("text", {
      x: shape.x + 4,
      y: shape.y + 2,
      fill: currentTextColor || "#000000",
      "font-size": `${fitSize}px`,
      "font-family": currentTextFontFamily || "Segoe UI",
      "font-weight": currentTextBold ? "bold" : "normal",
      "font-style": currentTextItalic ? "italic" : "normal",
      "text-decoration": currentTextUnderline ? "underline" : "none",
      "dominant-baseline": "hanging",
      opacity: "0.9"
    });
    text.textContent = "Văn bản mới";
    g.append(rect);
    g.append(text);
    artwork.append(g);
    return;
  } else {
    preview = makeSvg("polygon", { ...common, points: shape.points.map(point => point.join(",")).join(" ") });
  }
  preview.classList.add("drawing-preview");
  artwork.append(preview);
}

function renderLayers() {
  const list = document.querySelector("#layer-list");
  list.replaceChildren();

  const eyeOpenSvg = `<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>`;
  const eyeClosedSvg = `<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"/><line x1="1" y1="1" x2="23" y2="23"/></svg>`;

  const filtered = layers
    .map((layer, index) => ({ layer, index }))
    .filter(item => !layerSearchQuery || item.layer.name.toLowerCase().includes(layerSearchQuery));

  if (filtered.length === 0) {
    const empty = document.createElement("div");
    empty.className = "layer-empty-search";
    empty.textContent = "Không tìm thấy layer nào";
    list.append(empty);
    return;
  }

  const layerBoxSvg = `<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" style="vertical-align: middle;"><rect x="3" y="3" width="18" height="18" rx="2"/></svg>`;
  const layerDeleteSvg = `<svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2.5" stroke-linecap="round" style="vertical-align: middle;"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>`;

  [...filtered].reverse().forEach(({ layer, index }) => {
    const row = document.createElement("div");
    row.className = `layer-row${activeLayer === index ? " active" : ""}${layer.visible ? "" : " layer-hidden"}`;
    row.innerHTML = `<span class="layer-icon">${layerBoxSvg}</span><span class="layer-name" title="Nhấp đúp để đổi tên"></span><span class="layer-count">${layer.shapes.length}</span><button class="layer-toggle${layer.visible ? "" : " off"}" title="${layer.visible ? "Ẩn layer" : "Hiện layer"}" aria-label="${layer.visible ? "Ẩn layer" : "Hiện layer"}">${layer.visible ? eyeOpenSvg : eyeClosedSvg}</button><button class="layer-delete" title="Xóa layer" aria-label="Xóa layer" ${layers.length === 1 ? "disabled" : ""}>${layerDeleteSvg}</button>`;
    const nameEl = row.querySelector(".layer-name");
    nameEl.textContent = layer.name;

    let clickTimer = null;

    // Đổi tên khi double-click (hủy timer single-click để tránh render phá DOM)
    nameEl.addEventListener("dblclick", event => {
      event.stopPropagation();
      if (clickTimer) { clearTimeout(clickTimer); clickTimer = null; }

      const oldName = layer.name;
      const input = document.createElement("input");
      input.className = "layer-name-input";
      input.value = oldName;
      nameEl.replaceWith(input);
      input.focus();
      input.select();

      let committed = false;
      function commitRename() {
        if (committed) return;
        committed = true;
        const newName = input.value.trim() || oldName;
        if (newName !== oldName) {
          snapshot();
          layer.name = newName;
          setStatus(`Đã đổi tên layer thành "${newName}"`);
        }
        renderLayers();
      }
      input.addEventListener("blur", commitRename);
      input.addEventListener("keydown", e => {
        if (e.key === "Enter")  { e.preventDefault(); input.blur(); }
        if (e.key === "Escape") {
          e.preventDefault();
          committed = true; // chặn blur commit
          renderLayers();
        }
      });
    });

    row.addEventListener("click", event => {
      if (event.target.closest(".layer-toggle")) {
        snapshot();
        layer.visible = !layer.visible;
        render();
        setStatus(layer.visible ? `Đã hiện layer "${layer.name}"` : `Đã ẩn layer "${layer.name}"`);
        return;
      }
      if (event.target.closest(".layer-delete")) {
        if (layers.length <= 1) return;
        snapshot();
        layers.splice(index, 1);
        if (activeLayer === index) activeLayer = Math.max(0, index - 1);
        else if (activeLayer > index) activeLayer -= 1;
        if (selected?.layer === index) selected = null;
        else if (selected?.layer > index) selected.layer -= 1;
        render();
        setStatus(`Đã xóa layer "${layer.name}"`);
        return;
      }
      if (event.target.classList.contains("layer-name-input")) return;

      // Delay render để dblclick kịp hủy trước khi DOM bị xây lại
      if (clickTimer) { clearTimeout(clickTimer); clickTimer = null; }
      clickTimer = setTimeout(() => {
        clickTimer = null;
        activeLayer = index;
        selected = null;
        render();
        setStatus(`Layer hiện tại: ${layer.name}`);
      }, 220);
    });

    list.append(row);
  });
}

function renderProperties() {
  const controls = document.querySelector("#property-controls");
  const empty = document.querySelector("#property-empty");
  const brushControls = document.querySelector("#brush-controls");
  const eraserControls = document.querySelector("#eraser-controls");
  const imageControls = document.querySelector("#image-controls");
  const title = document.querySelector("#selection-title");
  const shape = selected ? layers[selected.layer]?.shapes[selected.shape] : null;
  const usingBrush = activeTool === "brush";
  const usingEraser = activeTool === "eraser";
  const isImage = shape?.type === "image";
  const isBrushShape = shape?.type === "brush";
  const isText = shape?.type === "text";
  const textControls = document.querySelector("#text-controls");
  controls.hidden = !shape || usingBrush || usingEraser || isImage || isBrushShape || isText;
  brushControls.hidden = !usingBrush && !isBrushShape;
  eraserControls.hidden = !usingEraser;
  imageControls.hidden = !isImage;
  if (textControls) textControls.hidden = !isText;
  empty.hidden = Boolean(shape) || usingBrush || usingEraser;
  title.textContent = usingBrush
    ? (brushLabels[activeBrushType] || "Bút vẽ")
    : usingEraser
    ? "Gôm"
    : isBrushShape
    ? (brushLabels[shape.brushStyle || "round"] || "Nét bút")
    : isText
    ? "Văn bản"
    : shape
    ? toolLabels[shape.type] || "Hình"
    : "Chưa chọn hình";

  const brushTypeSelect = document.querySelector("#brush-type-select");
  if (usingBrush) {
    if (brushTypeSelect) brushTypeSelect.value = activeBrushType;
    document.querySelector("#brush-color").value = brushColor;
    document.querySelector("#brush-width").value = brushWidth;
    document.querySelector("#brush-width-value").value = brushWidth;
    document.querySelector("#brush-opacity").value = Math.round(brushOpacity * 100);
    document.querySelector("#brush-opacity-value").value = `${Math.round(brushOpacity * 100)}%`;
  } else if (isBrushShape) {
    if (brushTypeSelect) brushTypeSelect.value = shape.brushStyle || "round";
    document.querySelector("#brush-color").value = shape.stroke;
    document.querySelector("#brush-width").value = shape.width;
    document.querySelector("#brush-width-value").value = shape.width;
    const op = Math.round((shape.opacity ?? 1) * 100);
    document.querySelector("#brush-opacity").value = op;
    document.querySelector("#brush-opacity-value").value = `${op}%`;
  }
  if (usingEraser) {
    document.querySelector("#eraser-width").value = eraserWidth;
    document.querySelector("#eraser-width-value").value = eraserWidth;
  }
  if (isImage) {
    const opVal = Math.round((shape.opacity ?? 1) * 100);
    document.querySelector("#image-opacity").value = opVal;
    document.querySelector("#image-opacity-value").value = `${opVal}%`;
    return;
  }
  if (isText) {
    const textContentInput = document.querySelector("#text-content-input");
    const textFontFamily = document.querySelector("#text-font-family");
    const textFontSize = document.querySelector("#text-font-size");
    const textSizeValue = document.querySelector("#text-size-value");
    const textColor = document.querySelector("#text-color");
    const textBoldBtn = document.querySelector("#text-bold-btn");
    const textItalicBtn = document.querySelector("#text-italic-btn");
    const textUnderlineBtn = document.querySelector("#text-underline-btn");

    if (textContentInput) textContentInput.value = shape.text || "";
    if (textFontFamily) textFontFamily.value = shape.fontFamily || "Segoe UI";
    if (textFontSize) {
      textFontSize.value = shape.fontSize || 24;
      if (textSizeValue) textSizeValue.value = `${shape.fontSize || 24}px`;
    }
    if (textColor) textColor.value = shape.color || "#000000";
    if (textBoldBtn) textBoldBtn.classList.toggle("active", Boolean(shape.bold));
    if (textItalicBtn) textItalicBtn.classList.toggle("active", Boolean(shape.italic));
    if (textUnderlineBtn) textUnderlineBtn.classList.toggle("active", Boolean(shape.underline));
    return;
  }
  if (!shape || usingBrush || usingEraser || isBrushShape) return;
  document.querySelector("#stroke-color").value = shape.stroke;
  const fillTransparentCheckbox = document.querySelector("#fill-transparent-checkbox");
  const isNoneFill = !shape.fill || shape.fill === "none";
  if (fillTransparentCheckbox) fillTransparentCheckbox.checked = isNoneFill;
  if (!isNoneFill) {
    document.querySelector("#fill-color").value = shape.fill;
  }
  document.querySelector("#stroke-width").value = shape.width;
  document.querySelector("#width-value").value = shape.width;
}

function setStatus(message) {
  document.querySelector("#status-message").textContent = message;
}

function pointFromEvent(event) {
  const point = drawing.createSVGPoint();
  point.x = event.clientX;
  point.y = event.clientY;
  const local = point.matrixTransform(drawing.getScreenCTM().inverse());
  return { x: Math.max(0, Math.min(CANVAS_WIDTH, local.x)), y: Math.max(0, Math.min(CANVAS_HEIGHT, local.y)) };
}

function pathData(points) {
  if (points.length === 1) return `M ${points[0][0]} ${points[0][1]} L ${points[0][0]} ${points[0][1]}`;
  return points.map(([x, y], index) => `${index ? "L" : "M"} ${x} ${y}`).join(" ");
}

function createShape(type, start, end) {
  const stroke = document.querySelector("#stroke-color")?.value || "#000000";
  const fillTransparent = document.querySelector("#fill-transparent-checkbox")?.checked ?? true;
  const fill = fillTransparent ? "none" : (document.querySelector("#fill-color")?.value || "#ffffff");
  const width = Number(document.querySelector("#stroke-width")?.value || 2);
  const x = Math.min(start.x, end.x);
  const y = Math.min(start.y, end.y);
  const w = Math.abs(end.x - start.x);
  const h = Math.abs(end.y - start.y);
  if (type === "rect" && w > 2 && h > 2) return { type, x, y, w, h, fill, stroke, width };
  if (type === "circle") {
    const r = Math.hypot(end.x - start.x, end.y - start.y);
    if (r > 2) return { type, x: start.x, y: start.y, r, fill, stroke, width };
  }
  if (type === "ellipse" && w > 2 && h > 2) {
    return { type, cx: (start.x + end.x) / 2, cy: (start.y + end.y) / 2, rx: w / 2, ry: h / 2, fill, stroke, width };
  }
  if (type === "line" && Math.hypot(w, h) > 2) return { type, x1: start.x, y1: start.y, x2: end.x, y2: end.y, fill: "none", stroke, width };
  if (type === "arrow" && Math.hypot(w, h) > 2) return { type, x1: start.x, y1: start.y, x2: end.x, y2: end.y, fill: "none", stroke, width };
  if (type === "curve" && Math.hypot(w, h) > 2) {
    const midX = (start.x + end.x) / 2;
    const midY = (start.y + end.y) / 2;
    const dx = end.x - start.x;
    const dy = end.y - start.y;
    const L = Math.hypot(dx, dy) || 1;
    const cx = midX - (dy / L) * Math.min(60, L * 0.35);
    const cy = midY + (dx / L) * Math.min(60, L * 0.35);
    return { type, x1: start.x, y1: start.y, cx, cy, x2: end.x, y2: end.y, fill: "none", stroke, width };
  }
  if (type === "wave" && Math.hypot(w, h) > 2) {
    const L = Math.hypot(end.x - start.x, end.y - start.y);
    const cycles = Math.max(1, Math.min(10, Math.round(L / 60)));
    return { type, x1: start.x, y1: start.y, x2: end.x, y2: end.y, amplitude: Math.min(25, Math.max(8, L * 0.1)), cycles, fill: "none", stroke, width };
  }
  if (type === "triangle" && w > 2 && h > 2) {
    return { type, points: [[start.x, end.y], [(start.x + end.x) / 2, start.y], [end.x, end.y]], fill, stroke, width };
  }
  if (type === "star" && w > 2 && h > 2) {
    const cx = (start.x + end.x) / 2;
    const cy = (start.y + end.y) / 2;
    return { type, points: starPoints(cx, cy, w / 2, h / 2), fill, stroke, width };
  }
  if (type === "parallelogram" && w > 2 && h > 2) {
    const shift = w * 0.25;
    return {
      type,
      points: [
        [x + shift, y],
        [x + w, y],
        [x + w - shift, y + h],
        [x, y + h]
      ],
      fill, stroke, width
    };
  }
  if (type === "trapezoid" && w > 2 && h > 2) {
    const inset = w * 0.2;
    return {
      type,
      points: [
        [x + inset, y],
        [x + w - inset, y],
        [x + w, y + h],
        [x, y + h]
      ],
      fill, stroke, width
    };
  }
  if (type === "text" && (w > 2 || h > 2)) {
    const fontSize = calcFitFontSize("Văn bản mới", w, h, currentTextBold);
    return {
      type: "text_box_preview",
      x, y,
      w: Math.max(w, 20),
      h: Math.max(h, 20),
      fontSize,
      fill: "none",
      stroke: theme === "dark" ? "#84cc16" : "#2563eb",
      width: 1.5
    };
  }
  return null;
}

function translateShape(shape, dx, dy) {
  if (shape.type === "rect" || shape.type === "circle" || shape.type === "image" || shape.type === "text") {
    shape.x += dx;
    shape.y += dy;
  } else if (shape.type === "ellipse") {
    shape.cx += dx;
    shape.cy += dy;
  } else if (shape.type === "line" || shape.type === "wave" || shape.type === "arrow") {
    shape.x1 += dx;
    shape.y1 += dy;
    shape.x2 += dx;
    shape.y2 += dy;
  } else if (shape.type === "curve") {
    shape.x1 += dx;
    shape.y1 += dy;
    shape.cx += dx;
    shape.cy += dy;
    shape.x2 += dx;
    shape.y2 += dy;
  } else {
    shape.points = shape.points.map(([x, y]) => [x + dx, y + dy]);
  }
}

function resizeShape(original, handle, point) {
  const shape = structuredClone(original);
  if (shape.type === "line" || shape.type === "wave" || shape.type === "arrow") {
    if (handle === "start") {
      shape.x1 = point.x;
      shape.y1 = point.y;
    } else {
      shape.x2 = point.x;
      shape.y2 = point.y;
    }
    return shape;
  }
  if (shape.type === "curve") {
    if (handle === "start") {
      shape.x1 = point.x;
      shape.y1 = point.y;
    } else if (handle === "control") {
      shape.cx = point.x;
      shape.cy = point.y;
    } else {
      shape.x2 = point.x;
      shape.y2 = point.y;
    }
    return shape;
  }
  if (shape.type === "circle") {
    shape.r = Math.max(1, Math.hypot(point.x - shape.x, point.y - shape.y));
    return shape;
  }

  const bounds = shapeBounds(original);
  const left = handle.includes("w") ? point.x : bounds.x;
  const right = handle.includes("e") ? point.x : bounds.x + bounds.w;
  const top = handle.includes("n") ? point.y : bounds.y;
  const bottom = handle.includes("s") ? point.y : bounds.y + bounds.h;
  const next = {
    x: Math.min(left, right),
    y: Math.min(top, bottom),
    w: Math.max(1, Math.abs(right - left)),
    h: Math.max(1, Math.abs(bottom - top))
  };
  if (shape.type === "rect" || shape.type === "image") {
    shape.x = next.x;
    shape.y = next.y;
    shape.w = next.w;
    shape.h = next.h;
  } else if (shape.type === "ellipse") {
    shape.cx = next.x + next.w / 2;
    shape.cy = next.y + next.h / 2;
    shape.rx = next.w / 2;
    shape.ry = next.h / 2;
  } else if (shape.type === "text") {
    shape.x = next.x;
    shape.y = next.y;
    const fitSize = calcFitFontSize(shape.text, next.w, next.h, shape.bold);
    shape.fontSize = fitSize;
    currentTextFontSize = shape.fontSize;
    const textSizeVal = document.querySelector("#text-size-value");
    const textSizeInput = document.querySelector("#text-font-size");
    if (textSizeVal) textSizeVal.value = `${shape.fontSize}px`;
    if (textSizeInput) textSizeInput.value = shape.fontSize;
  } else {
    shape.points = original.points.map(([x, y]) => [
      next.x + ((x - bounds.x) / (bounds.w || 1)) * next.w,
      next.y + ((y - bounds.y) / (bounds.h || 1)) * next.h
    ]);
  }
  return shape;
}

function pointInShapeCoordinates(point, shape) {
  const rotation = (shape.rotation || 0) * Math.PI / 180;
  if (rotation === 0) return point;
  const bounds = shapeBounds(shape);
  const centerX = bounds.x + bounds.w / 2;
  const centerY = bounds.y + bounds.h / 2;
  const dx = point.x - centerX;
  const dy = point.y - centerY;
  const cos = Math.cos(rotation);
  const sin = Math.sin(rotation);
  return {
    x: centerX + dx * cos + dy * sin,
    y: centerY - dx * sin + dy * cos
  };
}

function distanceToSegment(point, start, end) {
  const dx = end.x - start.x;
  const dy = end.y - start.y;
  const lengthSquared = dx * dx + dy * dy;
  if (!lengthSquared) return Math.hypot(point.x - start.x, point.y - start.y);
  const t = Math.max(0, Math.min(1, ((point.x - start.x) * dx + (point.y - start.y) * dy) / lengthSquared));
  return Math.hypot(point.x - (start.x + t * dx), point.y - (start.y + t * dy));
}

function pointHitsShape(point, shape, radius) {
  const p = pointInShapeCoordinates(point, shape);
  const threshold = radius + (shape.width || 1) / 2;
  if (shape.type === "circle") return Math.hypot(p.x - shape.x, p.y - shape.y) <= shape.r + threshold;
  if (shape.type === "ellipse") {
    const rx = shape.rx + threshold;
    const ry = shape.ry + threshold;
    const dx = p.x - shape.cx;
    const dy = p.y - shape.cy;
    return (dx * dx) / (rx * rx) + (dy * dy) / (ry * ry) <= 1;
  }
  if (shape.type === "line" || shape.type === "arrow") {
    return distanceToSegment(p, { x: shape.x1, y: shape.y1 }, { x: shape.x2, y: shape.y2 }) <= threshold;
  }
  if (shape.type === "curve") {
    const steps = 30;
    let prev = { x: shape.x1, y: shape.y1 };
    for (let i = 1; i <= steps; i++) {
      const t = i / steps;
      const inv = 1 - t;
      const curr = {
        x: inv * inv * shape.x1 + 2 * inv * t * shape.cx + t * t * shape.x2,
        y: inv * inv * shape.y1 + 2 * inv * t * shape.cy + t * t * shape.y2
      };
      if (distanceToSegment(p, prev, curr) <= threshold) return true;
      prev = curr;
    }
    return false;
  }
  if (shape.type === "wave") {
    const dx = shape.x2 - shape.x1;
    const dy = shape.y2 - shape.y1;
    const L = Math.hypot(dx, dy);
    if (L < 1e-4) return Math.hypot(p.x - shape.x1, p.y - shape.y1) <= threshold;
    const ux = dx / L, uy = dy / L;
    const nx = -uy, ny = ux;
    const cycles = shape.cycles || 3;
    const amplitude = shape.amplitude || 15;
    const halfWaves = Math.max(2, Math.round(cycles) * 2);
    const step = L / halfWaves;
    for (let i = 0; i < halfWaves; i++) {
      const sign = (i % 2 === 0) ? 1 : -1;
      const startX = shape.x1 + i * step * ux;
      const startY = shape.y1 + i * step * uy;
      const endX = shape.x1 + (i + 1) * step * ux;
      const endY = shape.y1 + (i + 1) * step * uy;
      const ctrlX = shape.x1 + (i + 0.5) * step * ux + sign * (amplitude * 2) * nx;
      const ctrlY = shape.y1 + (i + 0.5) * step * uy + sign * (amplitude * 2) * ny;
      let prev = { x: startX, y: startY };
      const subSteps = 8;
      for (let s = 1; s <= subSteps; s++) {
        const t = s / subSteps;
        const inv = 1 - t;
        const curr = {
          x: inv * inv * startX + 2 * inv * t * ctrlX + t * t * endX,
          y: inv * inv * startY + 2 * inv * t * ctrlY + t * t * endY
        };
        if (distanceToSegment(p, prev, curr) <= threshold) return true;
        prev = curr;
      }
    }
    return false;
  }
  const bounds = shapeBounds(shape);
  if (shape.type === "rect" || shape.type === "image" || shape.type === "text") {
    return p.x >= bounds.x - threshold && p.x <= bounds.x + bounds.w + threshold
      && p.y >= bounds.y - threshold && p.y <= bounds.y + bounds.h + threshold;
  }
  if (shape.type === "triangle" || shape.type === "polygon" || shape.type === "star" || shape.type === "parallelogram" || shape.type === "trapezoid") {
    const vertices = shape.points.map(([x, y]) => ({ x, y }));
    if (vertices.some((vertex, index) => distanceToSegment(p, vertex, vertices[(index + 1) % vertices.length]) <= threshold)) return true;
    let inside = false;
    for (let i = 0, j = vertices.length - 1; i < vertices.length; j = i++) {
      const a = vertices[i];
      const b = vertices[j];
      if ((a.y > p.y) !== (b.y > p.y)
        && p.x < ((b.x - a.x) * (p.y - a.y)) / (b.y - a.y) + a.x) inside = !inside;
    }
    return inside;
  }
  return false;
}

function eraseAtPoint(point, radius) {
  let anyChanged = false;
  for (const layer of layers) {
    if (!layer.visible || layer.locked) continue;
    const shapes = layer.shapes;
    let changed = false;
    const remaining = [];
    for (const shape of shapes) {
      if (shape.type === "image") { remaining.push(shape); continue; }
      if (shape.type !== "brush") {
        if (pointHitsShape(point, shape, radius)) changed = true;
        else remaining.push(shape);
        continue;
      }
      // Gôm brush: đánh dấu từng đoạn thẳng giữa 2 điểm liên tiếp
      const pts = shape.points;
      const erased = new Array(pts.length).fill(false);
      const halfW = shape.width / 2;
      for (let i = 0; i < pts.length; i++) {
        if (Math.hypot(point.x - pts[i][0], point.y - pts[i][1]) <= radius + halfW) erased[i] = true;
      }
      for (let i = 0; i < pts.length - 1; i++) {
        if (distanceToSegment(point, { x: pts[i][0], y: pts[i][1] }, { x: pts[i+1][0], y: pts[i+1][1] }) <= radius + halfW) {
          erased[i] = true; erased[i + 1] = true;
        }
      }
      if (!erased.some(Boolean)) { remaining.push(shape); continue; }
      changed = true;
      // Tách thành các đoạn con liên tục chưa bị gôm
      let seg = [];
      for (let i = 0; i < pts.length; i++) {
        if (!erased[i]) {
          seg.push(pts[i]);
        } else {
          if (seg.length >= 2) remaining.push({ ...shape, points: seg });
          else if (seg.length === 1) remaining.push({ ...shape, points: [seg[0], seg[0]] });
          seg = [];
        }
      }
      if (seg.length >= 2) remaining.push({ ...shape, points: seg });
      else if (seg.length === 1) remaining.push({ ...shape, points: [seg[0], seg[0]] });
    }
    if (changed) {
      shapes.splice(0, shapes.length, ...remaining);
      selected = null;
      anyChanged = true;
    }
  }
  return anyChanged;
}

function eraseAlong(start, end, radius) {
  const distance = Math.hypot(end.x - start.x, end.y - start.y);
  // Bước nhỏ hơn = mượt hơn (radius/3 thay vì radius*0.45)
  const steps = Math.max(1, Math.ceil(distance / Math.max(1, radius / 3)));
  let changed = false;
  for (let step = 1; step <= steps; step++) {
    const ratio = step / steps;
    changed = eraseAtPoint({
      x: start.x + (end.x - start.x) * ratio,
      y: start.y + (end.y - start.y) * ratio
    }, radius) || changed;
  }
  return changed;
}

let marqueeBoxEl = null;

function updateMarqueeBox(start, current) {
  const stage = document.querySelector("#canvas-stage");
  if (!stage) return;
  if (!marqueeBoxEl) {
    marqueeBoxEl = document.createElement("div");
    marqueeBoxEl.className = "marquee-box";
    stage.append(marqueeBoxEl);
  }
  const stageRect = stage.getBoundingClientRect();
  const drawingRect = drawing.getBoundingClientRect();
  const scaleX = drawingRect.width / CANVAS_WIDTH;
  const scaleY = drawingRect.height / CANVAS_HEIGHT;

  const minX = Math.min(start.x, current.x);
  const minY = Math.min(start.y, current.y);
  const maxX = Math.max(start.x, current.x);
  const maxY = Math.max(start.y, current.y);

  const left = (minX * scaleX) + (drawingRect.left - stageRect.left);
  const top = (minY * scaleY) + (drawingRect.top - stageRect.top);
  const width = Math.max(2, (maxX - minX) * scaleX);
  const height = Math.max(2, (maxY - minY) * scaleY);

  marqueeBoxEl.style.left = `${left}px`;
  marqueeBoxEl.style.top = `${top}px`;
  marqueeBoxEl.style.width = `${width}px`;
  marqueeBoxEl.style.height = `${height}px`;
  marqueeBoxEl.style.display = "block";
}

function removeMarqueeBox() {
  if (marqueeBoxEl) {
    marqueeBoxEl.remove();
    marqueeBoxEl = null;
  }
}

drawing.addEventListener("pointerdown", event => {
  if (event.button !== 0) return;
  event.preventDefault();
  closeInlineTextEditor(true);

  let start = pointFromEvent(event);
  if (snapToGrid) {
    start = { x: Math.round(start.x / 20) * 20, y: Math.round(start.y / 20) * 20 };
  }

  if (activeTool === "select" || activeTool === "group_select") {
    if (event.target === drawing || event.target.classList.contains("grid-bg")) {
      pointerAction = { mode: "marquee", start, moved: false };
      drawing.setPointerCapture(event.pointerId);
      return;
    }
  }

  if (activeTool === "eraser") {
    const hasAvailableLayer = layers.some(l => l.visible && !l.locked);
    if (!hasAvailableLayer) {
      setStatus("Tất cả layer đều bị ẩn hoặc khóa.");
      return;
    }
  } else {
    if (!layers[activeLayer].visible) {
      setStatus("Layer đang bị ẩn. Hãy bật hiển thị trước khi vẽ.");
      return;
    }
    if (layers[activeLayer].locked) {
      setStatus("Layer đang bị khóa. Hãy chọn layer khác để vẽ.");
      return;
    }
  }

  if (activeTool === "brush") {
    pointerAction = {
      mode: "brush",
      shape: { type: "brush", brushStyle: activeBrushType, points: [[start.x, start.y]], stroke: brushColor, width: brushWidth, opacity: brushOpacity, fill: "none" }
    };
    previewShape = pointerAction.shape;
    renderPreview();
  } else if (activeTool === "eraser") {
    pointerAction = {
      mode: "eraser",
      originalState: JSON.stringify({ layers, activeLayer }),
      previous: start,
      changed: eraseAtPoint(start, eraserWidth / 2)
    };
    if (pointerAction.changed) scheduleRender();
  } else {
    pointerAction = { mode: "draw", start, moved: false };
  }
  drawing.setPointerCapture(event.pointerId);
});

drawing.addEventListener("contextmenu", event => event.preventDefault());

// ── Con trỏ tròn cho gôm ─────────────────────────────────
let eraserCursor = null;
function updateEraserCursor(point) {
  if (activeTool !== "eraser") { removeEraserCursor(); return; }
  if (!eraserCursor) {
    eraserCursor = makeSvg("circle", {
      class: "eraser-cursor",
      r: eraserWidth / 2,
      fill: "rgba(255,255,255,0.15)",
      stroke: "#ffffff",
      "stroke-width": "1.5",
      "stroke-dasharray": "4 3",
      "pointer-events": "none"
    });
    artwork.append(eraserCursor);
  }
  eraserCursor.setAttribute("cx", point.x);
  eraserCursor.setAttribute("cy", point.y);
  eraserCursor.setAttribute("r", eraserWidth / 2);
}
function removeEraserCursor() {
  if (eraserCursor) { eraserCursor.remove(); eraserCursor = null; }
}
drawing.addEventListener("pointerleave", () => {
  removeEraserCursor();
  renderRulers(null);
});
drawing.addEventListener("pointerenter", event => {
  if (activeTool === "eraser") updateEraserCursor(pointFromEvent(event));
});

drawing.addEventListener("pointermove", event => {
  let point = pointFromEvent(event);
  if (snapToGrid) {
    point = { x: Math.round(point.x / 20) * 20, y: Math.round(point.y / 20) * 20 };
  }
  document.querySelector("#cursor-position").textContent = `X: ${Math.round(point.x)}   Y: ${Math.round(point.y)}`;
  updateEraserCursor(point);
  renderRulers(point);
  if (!pointerAction) return;

  if (pointerAction.mode === "marquee") {
    pointerAction.moved = true;
    updateMarqueeBox(pointerAction.start, point);
    return;
  }

  if (pointerAction.mode === "brush") {
    const last = pointerAction.shape.points[pointerAction.shape.points.length - 1];
    if (Math.hypot(point.x - last[0], point.y - last[1]) >= 0.7) {
      pointerAction.shape.points.push([point.x, point.y]);
      previewShape = pointerAction.shape;
      renderPreview();
    }
    return;
  }
  if (pointerAction.mode === "eraser") {
    if (eraseAlong(pointerAction.previous, point, eraserWidth / 2)) {
      pointerAction.changed = true;
      scheduleRender();
    }
    pointerAction.previous = point;
    return;
  }
  if (pointerAction.mode === "draw") {
    const shape = createShape(activeTool, pointerAction.start, point);
    if (shape) {
      previewShape = shape;
      renderPreview();
    }
    return;
  }

  let dx = point.x - pointerAction.start.x;
  let dy = point.y - pointerAction.start.y;
  if (snapToGrid) {
    dx = Math.round(dx / 20) * 20;
    dy = Math.round(dy / 20) * 20;
  }
  if (Math.hypot(dx, dy) > 0.5) pointerAction.moved = true;
  if (!pointerAction.moved) return;

  if (pointerAction.mode === "move_multi") {
    pointerAction.items.forEach(item => {
      layers[item.layer].shapes[item.shape] = structuredClone(item.original);
      translateShape(layers[item.layer].shapes[item.shape], dx, dy);
    });
    scheduleRender();
    return;
  }

  const { layer, shape: shapeIndex, original } = pointerAction;
  if (pointerAction.mode === "move") {
    layers[layer].shapes[shapeIndex] = structuredClone(original);
    translateShape(layers[layer].shapes[shapeIndex], dx, dy);
  } else if (pointerAction.mode === "rotate") {
    const currentAngle = Math.atan2(point.y - pointerAction.center.y, point.x - pointerAction.center.x);
    let delta = (currentAngle - pointerAction.startAngle) * 180 / Math.PI;
    if (delta > 180) delta -= 360;
    if (delta < -180) delta += 360;
    layers[layer].shapes[shapeIndex] = {
      ...structuredClone(original),
      rotation: pointerAction.initialRotation + delta
    };
  } else {
    layers[layer].shapes[shapeIndex] = resizeShape(original, pointerAction.handle, pointInShapeCoordinates(point, original));
  }
  scheduleRender();
});

drawing.addEventListener("pointerup", event => {
  if (!pointerAction) return;
  const action = pointerAction;
  pointerAction = null;

  if (action.mode === "marquee") {
    removeMarqueeBox();
    const end = pointFromEvent(event);
    const rect = {
      x: Math.min(action.start.x, end.x),
      y: Math.min(action.start.y, end.y),
      w: Math.abs(end.x - action.start.x),
      h: Math.abs(end.y - action.start.y)
    };
    if (rect.w >= 3 && rect.h >= 3) {
      const found = [];
      layers.forEach((layer, lIdx) => {
        if (!layer.visible || layer.locked) return;
        layer.shapes.forEach((shape, sIdx) => {
          const b = shapeBounds(shape);
          const intersects = !(b.x + b.w < rect.x || b.x > rect.x + rect.w || b.y + b.h < rect.y || b.y > rect.y + rect.h);
          if (intersects) {
            found.push({ layer: lIdx, shape: sIdx });
          }
        });
      });
      selectedItems = found;
      selected = found[0] || null;
      render();
      if (found.length > 0) {
        setStatus(`Đã quét chọn ${found.length} hình — kéo để di chuyển hoặc bấm Nhóm (Ctrl+G)`);
      } else {
        setStatus("Không có hình nào trong vùng chọn");
      }
    } else {
      selected = null;
      selectedItems = [];
      renderProperties();
      render();
    }
    return;
  }

  if (action.mode === "brush") {
    const shape = action.shape;
    if (shape.points.length === 1) shape.points.push([...shape.points[0]]);
    snapshot();
    layers[activeLayer].shapes.push(shape);
    selected = { layer: activeLayer, shape: layers[activeLayer].shapes.length - 1 };
    selectedItems = [selected];
    previewShape = null;
    render();
    setStatus("Đã thêm nét bút");
    return;
  }
  if (action.mode === "eraser") {
    if (action.changed) {
      undoStack.push(action.originalState);
      if (undoStack.length > 80) undoStack.shift();
      redoStack.length = 0;
      updateHistoryButtons();
      scheduleCppSync();
      selected = null;
      selectedItems = [];
      render();
      setStatus("Đã gôm nét vẽ");
    } else {
      render();
    }
    return;
  }
  if (action.mode === "draw") {
    let end = pointFromEvent(event);
    if (snapToGrid) {
      end = { x: Math.round(end.x / 20) * 20, y: Math.round(end.y / 20) * 20 };
    }
    if (activeTool === "text") {
      previewShape = null;
      const start = action.start;
      const w = Math.abs(end.x - start.x);
      const h = Math.abs(end.y - start.y);
      const isDrag = w >= 8 || h >= 8;
      const x = Math.min(start.x, end.x);
      const y = Math.min(start.y, end.y);
      const fontSize = isDrag
        ? calcFitFontSize("Văn bản mới", w, h, currentTextBold)
        : (currentTextFontSize || 24);
      currentTextFontSize = fontSize;

      const defaultText = "Văn bản mới";
      const newTextShape = {
        type: "text",
        x,
        y,
        text: defaultText,
        fontSize,
        fontFamily: currentTextFontFamily || "Segoe UI",
        color: currentTextColor || "#000000",
        bold: currentTextBold,
        italic: currentTextItalic,
        underline: currentTextUnderline,
        rotation: 0
      };
      snapshot();
      layers[activeLayer].shapes.push(newTextShape);
      selected = { layer: activeLayer, shape: layers[activeLayer].shapes.length - 1 };
      selectedItems = [selected];
      setTool("select");
      setStatus("Đã tạo văn bản — kéo các góc để chỉnh cỡ chữ hoặc nhấp đúp để sửa");
      openInlineTextEditor(newTextShape, selected.layer, selected.shape, isDrag ? { w, h } : null);
      return;
    }

    const shape = createShape(activeTool, action.start, end);
    previewShape = null;
    if (!shape) {
      renderPreview();
      return;
    }
    snapshot();
    layers[activeLayer].shapes.push(shape);
    selected = { layer: activeLayer, shape: layers[activeLayer].shapes.length - 1 };
    selectedItems = [selected];
    render();
    setStatus(`Đã thêm ${toolLabels[activeTool].toLowerCase()}`);
    return;
  }
  if (action.moved) {
    if (action.mode === "move_multi") {
      undoStack.push(JSON.stringify({ layers: layers.map((layer, index) => ({
        ...layer,
        shapes: layer.shapes.map((shape, shapeIndex) => {
          const found = action.items.find(i => i.layer === index && i.shape === shapeIndex);
          return found ? found.original : shape;
        })
      })), activeLayer }));
      if (undoStack.length > 80) undoStack.shift();
      redoStack.length = 0;
      updateHistoryButtons();
      scheduleCppSync();
      setStatus(`Đã di chuyển ${action.items.length} hình`);
    } else {
      undoStack.push(JSON.stringify({ layers: layers.map((layer, index) => ({
        ...layer,
        shapes: layer.shapes.map((shape, shapeIndex) =>
          index === action.layer && shapeIndex === action.shape ? action.original : shape)
      })), activeLayer }));
      if (undoStack.length > 80) undoStack.shift();
      redoStack.length = 0;
      updateHistoryButtons();
      scheduleCppSync();
      setStatus(action.mode === "move" ? "Đã di chuyển hình" : action.mode === "rotate" ? "Đã xoay hình" : "Đã đổi kích thước hình");
    }
  }
  flushRender();
});

drawing.addEventListener("pointercancel", () => {
  removeMarqueeBox();
  if (pointerAction?.mode === "draw" || pointerAction?.mode === "brush") previewShape = null;
  if (pointerAction?.mode === "eraser") {
    const state = JSON.parse(pointerAction.originalState);
    layers.splice(0, layers.length, ...state.layers);
    activeLayer = state.activeLayer;
  }
  if (pointerAction?.mode !== "draw" && pointerAction?.moved) {
    if (pointerAction.mode === "move_multi") {
      pointerAction.items.forEach(item => {
        layers[item.layer].shapes[item.shape] = item.original;
      });
    } else if (pointerAction.layer !== undefined && pointerAction.shape !== undefined) {
      const { layer, shape, original } = pointerAction;
      layers[layer].shapes[shape] = original;
    }
  }
  pointerAction = null;
  render();
});

const selectMenuBtn = document.querySelector("#select-menu-btn");
const selectFlyoutBoard = document.querySelector("#select-flyout-board");
const selectToolIcon = document.querySelector("#select-tool-icon");
const rulerToolBtn = document.querySelector("#ruler-tool-btn");
const snapGridBtn = document.querySelector("#snap-grid-btn");

const shapeTools = ["rect", "circle", "ellipse", "triangle", "star", "parallelogram", "trapezoid"];
let currentShapeTool = "rect";
const shapesMenuBtn = document.querySelector("#shapes-menu-btn");
const shapesFlyoutBoard = document.querySelector("#shapes-flyout-board");

const lineTools = ["line", "arrow", "curve", "wave"];
let currentLineTool = "line";
const linesMenuBtn = document.querySelector("#lines-menu-btn");
const linesFlyoutBoard = document.querySelector("#lines-flyout-board");

const brushMenuBtn = document.querySelector("#brush-menu-btn");
const brushFlyoutBoard = document.querySelector("#brush-flyout-board");
const brushTypeSelect = document.querySelector("#brush-type-select");

function updateSelectToolIcon(mode) {
  if (!selectToolIcon) return;
  if (mode === "group_select") {
    selectToolIcon.innerHTML = `<rect x="2" y="2" width="20" height="20" rx="3" stroke="#ffffff" stroke-width="2" stroke-dasharray="3 3"/><path d="M7 7l6 14 2.5-6 6-2.5L7 7z" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>`;
  } else {
    selectToolIcon.innerHTML = `<path d="M3 3l7 18 3-7 7-3L3 3z" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/><path d="M13 13l6 6" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>`;
  }
}

function setTool(toolName) {
  activeTool = toolName;

  const isSelect = toolName === "select" || toolName === "group_select";
  const isShape = shapeTools.includes(toolName);
  const isLine = lineTools.includes(toolName);

  if (isSelect) {
    currentSelectMode = toolName;
    updateSelectToolIcon(toolName);
  }
  if (isShape) currentShapeTool = toolName;
  if (isLine) currentLineTool = toolName;

  document.querySelectorAll(".tool[data-tool]").forEach(btn => {
    if (btn.id === "select-menu-btn") {
      btn.classList.toggle("active", isSelect);
    } else if (btn.id === "shapes-menu-btn") {
      btn.classList.toggle("active", isShape);
    } else if (btn.id === "lines-menu-btn") {
      btn.classList.toggle("active", isLine);
    } else if (btn.id === "brush-menu-btn") {
      btn.classList.toggle("active", toolName === "brush");
    } else if (btn.id === "ruler-tool-btn") {
      // Thước kẻ là công cụ toggle độc lập
    } else {
      btn.classList.toggle("active", btn.dataset.tool === toolName);
    }
  });

  document.querySelectorAll("#select-flyout-board .shape-opt-btn").forEach(btn => {
    btn.classList.toggle("active", btn.dataset.selectMode === toolName);
  });
  document.querySelectorAll("#shapes-flyout-board .shape-opt-btn").forEach(btn => {
    btn.classList.toggle("active", btn.dataset.shape === toolName);
  });
  document.querySelectorAll("#lines-flyout-board .shape-opt-btn").forEach(btn => {
    btn.classList.toggle("active", btn.dataset.lineTool === toolName);
  });
  document.querySelectorAll("#brush-flyout-board .shape-opt-btn").forEach(btn => {
    btn.classList.toggle("active", btn.dataset.brushType === activeBrushType);
  });

  if (activeTool !== "eraser") removeEraserCursor();
  render();
  const label = toolName === "brush" ? (brushLabels[activeBrushType] || "Bút vẽ") : (toolLabels[activeTool] || activeTool);
  setStatus(activeTool === "select" ? "Di chuyển tự do (V)" : activeTool === "group_select" ? "Quét chọn nhóm hình (G)" : `Công cụ: ${label}`);
}

// Click nút Select menu flyout
if (selectMenuBtn) {
  selectMenuBtn.addEventListener("click", event => {
    event.stopPropagation();
    if (shapesFlyoutBoard) shapesFlyoutBoard.hidden = true;
    if (linesFlyoutBoard) linesFlyoutBoard.hidden = true;
    if (brushFlyoutBoard) brushFlyoutBoard.hidden = true;
    const willOpen = selectFlyoutBoard ? selectFlyoutBoard.hidden : false;
    if (selectFlyoutBoard) selectFlyoutBoard.hidden = !willOpen;
    if (activeTool !== "select" && activeTool !== "group_select") {
      setTool(currentSelectMode);
    }
  });
}

document.querySelectorAll("#select-flyout-board .shape-opt-btn").forEach(btn => {
  btn.addEventListener("click", event => {
    event.stopPropagation();
    const mode = btn.dataset.selectMode;
    setTool(mode);
    if (selectFlyoutBoard) selectFlyoutBoard.hidden = true;
  });
});

// Click nút Shapes: toggle board shapes, đóng các board khác
if (shapesMenuBtn) {
  shapesMenuBtn.addEventListener("click", event => {
    event.stopPropagation();
    if (selectFlyoutBoard) selectFlyoutBoard.hidden = true;
    if (linesFlyoutBoard) linesFlyoutBoard.hidden = true;
    if (brushFlyoutBoard) brushFlyoutBoard.hidden = true;
    const willOpen = shapesFlyoutBoard.hidden;
    shapesFlyoutBoard.hidden = !willOpen;
    if (willOpen && !shapeTools.includes(activeTool)) {
      setTool(currentShapeTool);
    }
  });
}

// Click nút Lines: toggle board lines, đóng các board khác
if (linesMenuBtn) {
  linesMenuBtn.addEventListener("click", event => {
    event.stopPropagation();
    if (selectFlyoutBoard) selectFlyoutBoard.hidden = true;
    if (shapesFlyoutBoard) shapesFlyoutBoard.hidden = true;
    if (brushFlyoutBoard) brushFlyoutBoard.hidden = true;
    const willOpen = linesFlyoutBoard.hidden;
    linesFlyoutBoard.hidden = !willOpen;
    if (willOpen && !lineTools.includes(activeTool)) {
      setTool(currentLineTool);
    }
  });
}

// Click nút Brush: toggle board brush, đóng các board khác
if (brushMenuBtn) {
  brushMenuBtn.addEventListener("click", event => {
    event.stopPropagation();
    if (selectFlyoutBoard) selectFlyoutBoard.hidden = true;
    if (shapesFlyoutBoard) shapesFlyoutBoard.hidden = true;
    if (linesFlyoutBoard) linesFlyoutBoard.hidden = true;
    const willOpen = brushFlyoutBoard.hidden;
    brushFlyoutBoard.hidden = !willOpen;
    setTool("brush");
  });
}

// Click nút Thước kẻ CAD (dưới cây bút, trên văn bản)
if (rulerToolBtn) {
  rulerToolBtn.classList.toggle("active", Boolean(showRulers));
  rulerToolBtn.addEventListener("click", () => {
    showRulers = !showRulers;
    localStorage.setItem("mini-vector-cad-show-rulers", showRulers);
    rulerToolBtn.classList.toggle("active", showRulers);
    renderRulers();
    setStatus(showRulers ? "Đã bật Thước đo CAD (vạch chia tỉ lệ & vị trí con trỏ)" : "Đã tắt Thước đo CAD");
  });
}

// Click nút Bắt điểm lưới (Snap to Grid)
if (snapGridBtn) {
  snapGridBtn.addEventListener("click", () => {
    snapToGrid = !snapToGrid;
    snapGridBtn.classList.toggle("active", snapToGrid);
    setStatus(snapToGrid ? "Đã bật Bắt điểm lưới CAD (20px)" : "Đã tắt Bắt điểm lưới");
  });
}

// Chọn một hình trong board shapes
document.querySelectorAll("#shapes-flyout-board .shape-opt-btn").forEach(btn => {
  btn.addEventListener("click", event => {
    event.stopPropagation();
    const shape = btn.dataset.shape;
    setTool(shape);
    if (shapesFlyoutBoard) shapesFlyoutBoard.hidden = true;
  });
});

// Chọn một nét trong board lines
document.querySelectorAll("#lines-flyout-board .shape-opt-btn").forEach(btn => {
  btn.addEventListener("click", event => {
    event.stopPropagation();
    const lineTool = btn.dataset.lineTool;
    setTool(lineTool);
    if (linesFlyoutBoard) linesFlyoutBoard.hidden = true;
  });
});

// Chọn một loại bút trong board brush
document.querySelectorAll("#brush-flyout-board .shape-opt-btn").forEach(btn => {
  btn.addEventListener("click", event => {
    event.stopPropagation();
    activeBrushType = btn.dataset.brushType;
    document.querySelectorAll("#brush-flyout-board .shape-opt-btn").forEach(b => {
      b.classList.toggle("active", b.dataset.brushType === activeBrushType);
    });
    if (brushTypeSelect) brushTypeSelect.value = activeBrushType;
    if (brushFlyoutBoard) brushFlyoutBoard.hidden = true;
    setTool("brush");
    setStatus(`Đã chọn loại bút: ${brushLabels[activeBrushType]}`);
  });
});

// Thay đổi loại bút qua dropdown trong inspector
if (brushTypeSelect) {
  brushTypeSelect.addEventListener("change", event => {
    activeBrushType = event.target.value;
    document.querySelectorAll("#brush-flyout-board .shape-opt-btn").forEach(b => {
      b.classList.toggle("active", b.dataset.brushType === activeBrushType);
    });
    if (selected && layers[selected.layer].shapes[selected.shape]?.type === "brush") {
      snapshot();
      layers[selected.layer].shapes[selected.shape].brushStyle = activeBrushType;
      render();
    }
    setStatus(`Đã chọn loại bút: ${brushLabels[activeBrushType]}`);
  });
}

// Click ngoài thì đóng cả 4 flyout boards
document.addEventListener("click", event => {
  if (!event.target.closest(".tool-flyout-wrapper")) {
    if (selectFlyoutBoard) selectFlyoutBoard.hidden = true;
    if (shapesFlyoutBoard) shapesFlyoutBoard.hidden = true;
    if (linesFlyoutBoard) linesFlyoutBoard.hidden = true;
    if (brushFlyoutBoard) brushFlyoutBoard.hidden = true;
  }
});

// Các tool còn lại trên thanh công cụ (eraser, text)
document.querySelectorAll(".tool[data-tool]").forEach(button => {
  if (button.dataset.tool === "shapes" || button.dataset.tool === "lines" || button.id === "brush-menu-btn" || button.id === "select-menu-btn" || button.id === "ruler-tool-btn") return;
  button.addEventListener("click", () => {
    if (selectFlyoutBoard) selectFlyoutBoard.hidden = true;
    if (shapesFlyoutBoard) shapesFlyoutBoard.hidden = true;
    if (linesFlyoutBoard) linesFlyoutBoard.hidden = true;
    if (brushFlyoutBoard) brushFlyoutBoard.hidden = true;
    setTool(button.dataset.tool);
  });
});

// ── Xử lý Nhóm (Group) & Rã nhóm (Ungroup) ────────────────
function groupSelectedShapes() {
  if (selectedItems.length < 2) return;
  snapshot();
  const targetLayer = selectedItems[0].layer;
  if (layers[targetLayer].locked) {
    setStatus("Layer đang bị khóa, không thể nhóm hình.");
    return;
  }
  const groupedShapes = [];
  const sorted = [...selectedItems].sort((a, b) => b.shape - a.shape);
  sorted.forEach(item => {
    groupedShapes.unshift(...layers[item.layer].shapes.splice(item.shape, 1));
  });
  const newGroup = {
    type: "group",
    children: groupedShapes,
    rotation: 0
  };
  layers[targetLayer].shapes.push(newGroup);
  selected = { layer: targetLayer, shape: layers[targetLayer].shapes.length - 1 };
  selectedItems = [selected];
  render();
  setStatus(`Đã nhóm ${groupedShapes.length} hình thành 1 nhóm`);
}

function ungroupSelectedShape() {
  if (!selected) return;
  const shape = layers[selected.layer]?.shapes[selected.shape];
  if (!shape || shape.type !== "group") return;
  if (layers[selected.layer].locked) {
    setStatus("Layer đang bị khóa, không thể rã nhóm.");
    return;
  }
  snapshot();
  const children = shape.children || [];
  layers[selected.layer].shapes.splice(selected.shape, 1);
  const startIdx = layers[selected.layer].shapes.length;
  layers[selected.layer].shapes.push(...children);
  selectedItems = children.map((_, i) => ({ layer: selected.layer, shape: startIdx + i }));
  selected = selectedItems[0] || null;
  render();
  setStatus(`Đã rã nhóm thành ${children.length} hình riêng lẻ`);
}

const groupShapesBtn = document.querySelector("#group-shapes");
if (groupShapesBtn) groupShapesBtn.addEventListener("click", groupSelectedShapes);

const ungroupShapesBtn = document.querySelector("#ungroup-shapes");
if (ungroupShapesBtn) ungroupShapesBtn.addEventListener("click", ungroupSelectedShape);

// ── Xử lý Đổi thứ tự lớp (Z-Order) ────────────────────────
function bringToFront() {
  if (!selected) return;
  const layer = layers[selected.layer];
  if (layer.locked) { setStatus("Layer đang bị khóa."); return; }
  snapshot();
  const item = layer.shapes.splice(selected.shape, 1)[0];
  layer.shapes.push(item);
  selected.shape = layer.shapes.length - 1;
  selectedItems = [selected];
  render();
  setStatus("Đã đưa hình lên trên cùng");
}

function sendToBack() {
  if (!selected) return;
  const layer = layers[selected.layer];
  if (layer.locked) { setStatus("Layer đang bị khóa."); return; }
  snapshot();
  const item = layer.shapes.splice(selected.shape, 1)[0];
  layer.shapes.unshift(item);
  selected.shape = 0;
  selectedItems = [selected];
  render();
  setStatus("Đã đưa hình xuống dưới cùng");
}

const bringFrontBtn = document.querySelector("#bring-front");
if (bringFrontBtn) bringFrontBtn.addEventListener("click", bringToFront);

const sendBackBtn = document.querySelector("#send-back");
if (sendBackBtn) sendBackBtn.addEventListener("click", sendToBack);

// ── Xử lý Lật hình (Flip) ─────────────────────────────────
function flipShapeObj(shape, axis, cx, cy) {
  if (shape.type === "group") {
    (shape.children || []).forEach(c => flipShapeObj(c, axis, cx, cy));
  } else if (shape.type === "rect" || shape.type === "image" || shape.type === "text") {
    if (axis === "h") shape.x = cx + (cx - (shape.x + shape.w));
    else shape.y = cy + (cy - (shape.y + shape.h));
  } else if (shape.type === "ellipse") {
    if (axis === "h") shape.cx = cx + (cx - shape.cx);
    else shape.cy = cy + (cy - shape.cy);
  } else if (shape.type === "circle") {
    if (axis === "h") shape.x = cx + (cx - shape.x);
    else shape.y = cy + (cy - shape.y);
  } else if (shape.type === "line" || shape.type === "arrow" || shape.type === "wave") {
    if (axis === "h") {
      shape.x1 = cx + (cx - shape.x1);
      shape.x2 = cx + (cx - shape.x2);
    } else {
      shape.y1 = cy + (cy - shape.y1);
      shape.y2 = cy + (cy - shape.y2);
    }
  } else if (shape.type === "curve") {
    if (axis === "h") {
      shape.x1 = cx + (cx - shape.x1);
      shape.cx = cx + (cx - shape.cx);
      shape.x2 = cx + (cx - shape.x2);
    } else {
      shape.y1 = cy + (cy - shape.y1);
      shape.cy = cy + (cy - shape.cy);
      shape.y2 = cy + (cy - shape.y2);
    }
  } else if (shape.points) {
    shape.points = shape.points.map(([x, y]) => [
      axis === "h" ? cx + (cx - x) : x,
      axis === "v" ? cy + (cy - y) : y
    ]);
  }
}

function flipSelected(axis) {
  const items = selectedItems.length > 0 ? selectedItems : (selected ? [selected] : []);
  if (!items.length) return;
  const shapes = items.map(i => layers[i.layer]?.shapes[i.shape]).filter(Boolean);
  if (!shapes.length) return;
  snapshot();
  shapes.forEach(shape => {
    const b = shapeBounds(shape);
    const cx = b.x + b.w / 2;
    const cy = b.y + b.h / 2;
    flipShapeObj(shape, axis, cx, cy);
  });
  render();
  setStatus(axis === "h" ? "Đã lật ngang hình" : "Đã lật dọc hình");
}

const flipHBtn = document.querySelector("#flip-h");
if (flipHBtn) flipHBtn.addEventListener("click", () => flipSelected("h"));

const flipVBtn = document.querySelector("#flip-v");
if (flipVBtn) flipVBtn.addEventListener("click", () => flipSelected("v"));

// ── Xử lý Căn lề (Align) ──────────────────────────────────
function alignSelected(type) {
  const items = selectedItems.length > 0 ? selectedItems : (selected ? [selected] : []);
  if (!items.length) return;
  const shapes = items.map(i => layers[i.layer]?.shapes[i.shape]).filter(Boolean);
  if (!shapes.length) return;
  snapshot();
  let targetBox;
  if (shapes.length > 1) {
    const bboxes = shapes.map(s => shapeBounds(s));
    const minX = Math.min(...bboxes.map(b => b.x));
    const minY = Math.min(...bboxes.map(b => b.y));
    const maxX = Math.max(...bboxes.map(b => b.x + b.w));
    const maxY = Math.max(...bboxes.map(b => b.y + b.h));
    targetBox = { x: minX, y: minY, w: maxX - minX, h: maxY - minY };
  } else {
    targetBox = { x: 0, y: 0, w: CANVAS_WIDTH, h: CANVAS_HEIGHT };
  }
  shapes.forEach(shape => {
    const b = shapeBounds(shape);
    let dx = 0, dy = 0;
    if (type === "left") dx = targetBox.x - b.x;
    else if (type === "center-h") dx = (targetBox.x + targetBox.w / 2) - (b.x + b.w / 2);
    else if (type === "right") dx = (targetBox.x + targetBox.w) - (b.x + b.w);
    else if (type === "top") dy = targetBox.y - b.y;
    else if (type === "center-v") dy = (targetBox.y + targetBox.h / 2) - (b.y + b.h / 2);
    else if (type === "bottom") dy = (targetBox.y + targetBox.h) - (b.y + b.h);
    translateShape(shape, dx, dy);
  });
  render();
  const alignNameMap = {
    left: "căn lề trái",
    "center-h": "căn giữa ngang",
    right: "căn lề phải",
    top: "căn lề trên",
    "center-v": "căn giữa dọc",
    bottom: "căn lề dưới"
  };
  setStatus(`Đã ${alignNameMap[type] || "căn lề"}`);
}

const alignLeftBtn = document.querySelector("#align-left");
if (alignLeftBtn) alignLeftBtn.addEventListener("click", () => alignSelected("left"));
const alignCenterHBtn = document.querySelector("#align-center-h");
if (alignCenterHBtn) alignCenterHBtn.addEventListener("click", () => alignSelected("center-h"));
const alignRightBtn = document.querySelector("#align-right");
if (alignRightBtn) alignRightBtn.addEventListener("click", () => alignSelected("right"));
const alignTopBtn = document.querySelector("#align-top");
if (alignTopBtn) alignTopBtn.addEventListener("click", () => alignSelected("top"));
const alignCenterVBtn = document.querySelector("#align-center-v");
if (alignCenterVBtn) alignCenterVBtn.addEventListener("click", () => alignSelected("center-v"));
const alignBottomBtn = document.querySelector("#align-bottom");
if (alignBottomBtn) alignBottomBtn.addEventListener("click", () => alignSelected("bottom"));

// ── Thước đo CAD (Rulers) Canvas Vẽ ───────────────────────
// ── Thước đo CAD (Rulers) Canvas Vẽ ───────────────────────
function renderRulers(cursorPoint = null) {
  const rulerTopCanvas = document.querySelector("#ruler-top");
  const rulerLeftCanvas = document.querySelector("#ruler-left");
  const rulerCorner = document.querySelector("#ruler-corner");
  const viewport = document.querySelector("#canvas-viewport");

  if (rulerToolBtn) rulerToolBtn.classList.toggle("active", Boolean(showRulers));
  if (viewport) viewport.classList.toggle("no-rulers", !showRulers);
  if (rulerTopCanvas) rulerTopCanvas.hidden = !showRulers;
  if (rulerLeftCanvas) rulerLeftCanvas.hidden = !showRulers;
  if (rulerCorner) rulerCorner.hidden = !showRulers;

  if (!showRulers || !rulerTopCanvas || !rulerLeftCanvas) return;

  const stage = document.querySelector("#canvas-stage");
  if (!stage) return;
  const stageRect = stage.getBoundingClientRect();
  const drawingRect = drawing.getBoundingClientRect();

  const dpr = window.devicePixelRatio || 1;
  const topWidth = Math.max(50, Math.round(rulerTopCanvas.offsetWidth || stageRect.width));
  const leftHeight = Math.max(50, Math.round(rulerLeftCanvas.offsetHeight || stageRect.height));

  if (rulerTopCanvas.width !== Math.round(topWidth * dpr) || rulerTopCanvas.height !== Math.round(26 * dpr)) {
    rulerTopCanvas.width = Math.round(topWidth * dpr);
    rulerTopCanvas.height = Math.round(26 * dpr);
  }
  if (rulerLeftCanvas.width !== Math.round(26 * dpr) || rulerLeftCanvas.height !== Math.round(leftHeight * dpr)) {
    rulerLeftCanvas.width = Math.round(26 * dpr);
    rulerLeftCanvas.height = Math.round(leftHeight * dpr);
  }

  const ctxTop = rulerTopCanvas.getContext("2d");
  const ctxLeft = rulerLeftCanvas.getContext("2d");
  ctxTop.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctxLeft.setTransform(dpr, 0, 0, dpr, 0, 0);

  const isDark = theme === "dark";
  const baseBg = isDark ? "#141a23" : "#f1f5f9";
  const paperBg = isDark ? "#1b2432" : "#ffffff";
  const boundaryColor = isDark ? "#38bdf8" : "#2563eb";
  const tickColor = isDark ? "#4b5b70" : "#cbd5e1";
  const majorTickColor = isDark ? "#7e90a7" : "#64748b";
  const textColor = isDark ? "#cbd5e1" : "#334155";
  const highlightColor = isDark ? "#a3e635" : "#2563eb";

  const originX = drawingRect.left - stageRect.left;
  const scaleX = drawingRect.width / CANVAS_WIDTH;

  const originY = drawingRect.top - stageRect.top;
  const scaleY = drawingRect.height / CANVAS_HEIGHT;

  // Bước chia vạch thích ứng linh hoạt theo tỷ lệ thu phóng zoom
  let majorStep = 100;
  let midStep = 50;
  let minorStep = 10;

  if (scaleX >= 2.8) {
    majorStep = 20;
    midStep = 10;
    minorStep = 2;
  } else if (scaleX >= 1.5) {
    majorStep = 50;
    midStep = 25;
    minorStep = 5;
  } else if (scaleX >= 0.7) {
    majorStep = 100;
    midStep = 50;
    minorStep = 10;
  } else if (scaleX >= 0.35) {
    majorStep = 200;
    midStep = 100;
    minorStep = 20;
  } else {
    majorStep = 400;
    midStep = 200;
    minorStep = 50;
  }

  // ── Vẽ Ruler ngang (Top) ──────────────────────────────────
  ctxTop.fillStyle = baseBg;
  ctxTop.fillRect(0, 0, topWidth, 26);

  // Vùng trang vẽ (Canvas Paper)
  const paperLeft = Math.max(0, originX);
  const paperRight = Math.min(topWidth, originX + CANVAS_WIDTH * scaleX);
  if (paperRight > paperLeft) {
    ctxTop.fillStyle = paperBg;
    ctxTop.fillRect(paperLeft, 0, paperRight - paperLeft, 26);
  }

  ctxTop.font = "9px 'Segoe UI', system-ui, sans-serif";
  ctxTop.textBaseline = "top";
  ctxTop.textAlign = "left";

  for (let px = 0; px <= CANVAS_WIDTH; px += minorStep) {
    const screenX = Math.round(originX + px * scaleX);
    if (screenX < -20 || screenX > topWidth + 20) continue;

    const isMajor = px % majorStep === 0;
    const isMid = px % midStep === 0;

    ctxTop.beginPath();
    ctxTop.strokeStyle = isMajor ? majorTickColor : tickColor;
    ctxTop.lineWidth = isMajor ? 1.2 : 1;

    if (isMajor) {
      ctxTop.moveTo(screenX, 15);
      ctxTop.lineTo(screenX, 26);
      ctxTop.stroke();

      ctxTop.fillStyle = textColor;
      ctxTop.fillText(String(px), screenX + 3, 3);
    } else if (isMid) {
      ctxTop.moveTo(screenX, 18);
      ctxTop.lineTo(screenX, 26);
      ctxTop.stroke();
    } else {
      ctxTop.moveTo(screenX, 22);
      ctxTop.lineTo(screenX, 26);
      ctxTop.stroke();
    }
  }

  // Viền biên trang giấy vẽ trên thước ngang
  if (originX >= 0 && originX <= topWidth) {
    ctxTop.strokeStyle = boundaryColor;
    ctxTop.lineWidth = 1.5;
    ctxTop.beginPath();
    ctxTop.moveTo(originX, 0);
    ctxTop.lineTo(originX, 26);
    ctxTop.stroke();
  }
  const rightBoundary = originX + CANVAS_WIDTH * scaleX;
  if (rightBoundary >= 0 && rightBoundary <= topWidth) {
    ctxTop.strokeStyle = boundaryColor;
    ctxTop.lineWidth = 1.5;
    ctxTop.beginPath();
    ctxTop.moveTo(rightBoundary, 0);
    ctxTop.lineTo(rightBoundary, 26);
    ctxTop.stroke();
  }

  // ── Vẽ Ruler dọc (Left) ───────────────────────────────────
  ctxLeft.fillStyle = baseBg;
  ctxLeft.fillRect(0, 0, 26, leftHeight);

  const paperTop = Math.max(0, originY);
  const paperBottom = Math.min(leftHeight, originY + CANVAS_HEIGHT * scaleY);
  if (paperBottom > paperTop) {
    ctxLeft.fillStyle = paperBg;
    ctxLeft.fillRect(0, paperTop, 26, paperBottom - paperTop);
  }

  ctxLeft.font = "8px 'Segoe UI', system-ui, sans-serif";
  ctxLeft.textBaseline = "middle";
  ctxLeft.textAlign = "right";

  for (let py = 0; py <= CANVAS_HEIGHT; py += minorStep) {
    const screenY = Math.round(originY + py * scaleY);
    if (screenY < -20 || screenY > leftHeight + 20) continue;

    const isMajor = py % majorStep === 0;
    const isMid = py % midStep === 0;

    ctxLeft.beginPath();
    ctxLeft.strokeStyle = isMajor ? majorTickColor : tickColor;
    ctxLeft.lineWidth = isMajor ? 1.2 : 1;

    if (isMajor) {
      ctxLeft.moveTo(18, screenY);
      ctxLeft.lineTo(26, screenY);
      ctxLeft.stroke();

      ctxLeft.fillStyle = textColor;
      // Chữ số hiển thị ngang, căn lề phải ngay ngắn, không bao giờ bị cắt xén
      ctxLeft.fillText(String(py), 16, screenY);
    } else if (isMid) {
      ctxLeft.moveTo(21, screenY);
      ctxLeft.lineTo(26, screenY);
      ctxLeft.stroke();
    } else {
      ctxLeft.moveTo(23, screenY);
      ctxLeft.lineTo(26, screenY);
      ctxLeft.stroke();
    }
  }

  // Viền biên trang giấy vẽ trên thước dọc
  if (originY >= 0 && originY <= leftHeight) {
    ctxLeft.strokeStyle = boundaryColor;
    ctxLeft.lineWidth = 1.5;
    ctxLeft.beginPath();
    ctxLeft.moveTo(0, originY);
    ctxLeft.lineTo(26, originY);
    ctxLeft.stroke();
  }
  const bottomBoundary = originY + CANVAS_HEIGHT * scaleY;
  if (bottomBoundary >= 0 && bottomBoundary <= leftHeight) {
    ctxLeft.strokeStyle = boundaryColor;
    ctxLeft.lineWidth = 1.5;
    ctxLeft.beginPath();
    ctxLeft.moveTo(0, bottomBoundary);
    ctxLeft.lineTo(26, bottomBoundary);
    ctxLeft.stroke();
  }

  // ── Vạch chỉ báo con trỏ chuột trên cả 2 thước ───────────
  if (cursorPoint) {
    const curX = originX + cursorPoint.x * scaleX;
    const curY = originY + cursorPoint.y * scaleY;

    if (curX >= 0 && curX <= topWidth) {
      ctxTop.strokeStyle = highlightColor;
      ctxTop.lineWidth = 1.5;
      ctxTop.beginPath();
      ctxTop.moveTo(curX, 0);
      ctxTop.lineTo(curX, 26);
      ctxTop.stroke();
    }
    if (curY >= 0 && curY <= leftHeight) {
      ctxLeft.strokeStyle = highlightColor;
      ctxLeft.lineWidth = 1.5;
      ctxLeft.beginPath();
      ctxLeft.moveTo(0, curY);
      ctxLeft.lineTo(26, curY);
      ctxLeft.stroke();
    }
  }
}

window.addEventListener("resize", () => {
  renderSelectionToolbar();
  renderRulers();
});
document.querySelector("#canvas-stage").addEventListener("scroll", () => {
  renderSelectionToolbar();
  renderRulers();
});

document.querySelector("#brush-color").addEventListener("input", event => {
  brushColor = event.target.value;
  if (selected && layers[selected.layer].shapes[selected.shape]?.type === "brush") {
    layers[selected.layer].shapes[selected.shape].stroke = brushColor;
    render();
  }
});
document.querySelector("#brush-width").addEventListener("input", event => {
  brushWidth = Number(event.target.value);
  document.querySelector("#brush-width-value").value = brushWidth;
  if (selected && layers[selected.layer].shapes[selected.shape]?.type === "brush") {
    layers[selected.layer].shapes[selected.shape].width = brushWidth;
    render();
  }
});
document.querySelector("#brush-opacity").addEventListener("input", event => {
  brushOpacity = Number(event.target.value) / 100;
  document.querySelector("#brush-opacity-value").value = `${event.target.value}%`;
  if (selected && layers[selected.layer].shapes[selected.shape]?.type === "brush") {
    layers[selected.layer].shapes[selected.shape].opacity = brushOpacity;
    render();
  }
});
document.querySelector("#eraser-width").addEventListener("input", event => {
  eraserWidth = Number(event.target.value);
  document.querySelector("#eraser-width-value").value = eraserWidth;
  if (eraserCursor) eraserCursor.setAttribute("r", eraserWidth / 2);
});

for (const [inputId, property] of [["stroke-color", "stroke"], ["fill-color", "fill"], ["stroke-width", "width"]]) {
  const input = document.querySelector(`#${inputId}`);
  let historyCaptured = false;
  input.addEventListener("input", event => {
    if (!selected) return;
    if (layers[selected.layer].locked) {
      setStatus("Layer đang bị khóa, không thể chỉnh sửa hình.");
      return;
    }
    if (!historyCaptured) {
      snapshot();
      historyCaptured = true;
    }
    if (inputId === "fill-color") {
      const chk = document.querySelector("#fill-transparent-checkbox");
      if (chk) chk.checked = false;
    }
    layers[selected.layer].shapes[selected.shape][property] = property === "width" ? Number(event.target.value) : event.target.value;
    document.querySelector("#width-value").value = event.target.value;
    render();
  });
  input.addEventListener("change", () => { historyCaptured = false; });
}

const fillTransparentCheckbox = document.querySelector("#fill-transparent-checkbox");
if (fillTransparentCheckbox) {
  fillTransparentCheckbox.addEventListener("change", event => {
    if (!selected) return;
    if (layers[selected.layer].locked) {
      setStatus("Layer đang bị khóa, không thể chỉnh sửa hình.");
      return;
    }
    const shape = layers[selected.layer].shapes[selected.shape];
    if (!shape) return;
    snapshot();
    if (event.target.checked) {
      shape.fill = "none";
    } else {
      shape.fill = document.querySelector("#fill-color")?.value || "#ffffff";
    }
    render();
    setStatus(event.target.checked ? "Đã đặt nền trong suốt" : "Đã bật màu nền");
  });
}

function deleteSelectedShape() {
  const items = selectedItems.length > 0 ? selectedItems : (selected ? [selected] : []);
  if (!items.length) return;
  const locked = items.some(i => layers[i.layer]?.locked);
  if (locked) {
    setStatus("Có layer đang bị khóa, không thể xóa hình.");
    return;
  }
  snapshot();
  const sorted = [...items].sort((a, b) => b.shape - a.shape);
  sorted.forEach(item => {
    layers[item.layer]?.shapes.splice(item.shape, 1);
  });
  selected = null;
  selectedItems = [];
  render();
  setStatus(items.length > 1 ? `Đã xóa ${items.length} hình` : "Đã xóa hình");
}

document.querySelector("#delete-shape").addEventListener("click", deleteSelectedShape);

document.querySelector("#copy-shape").addEventListener("click", () => {
  if (!selected) return;
  copiedShape = structuredClone(layers[selected.layer].shapes[selected.shape]);
  renderSelectionToolbar();
  setStatus("Đã sao chép hình — chọn một layer rồi bấm Dán");
});

document.querySelector("#duplicate-shape").addEventListener("click", () => {
  if (!selected) return;
  const layerIndex = selected.layer;
  if (layers[layerIndex].locked) {
    setStatus("Layer đang bị khóa, không thể nhân bản hình.");
    return;
  }
  if (!layers[layerIndex].visible) {
    setStatus("Không thể nhân bản vào layer đang ẩn.");
    return;
  }
  snapshot();
  const duplicate = structuredClone(layers[layerIndex].shapes[selected.shape]);
  translateShape(duplicate, 16, 16);
  layers[layerIndex].shapes.push(duplicate);
  selected = { layer: layerIndex, shape: layers[layerIndex].shapes.length - 1 };
  activeLayer = layerIndex;
  render();
  setStatus("Đã tạo bản sao của hình");
});

document.querySelector("#paste-shape").addEventListener("click", pasteCopiedShape);

function pasteCopiedShape() {
  if (!copiedShape) return;
  if (layers[activeLayer].locked) {
    setStatus("Layer đang bị khóa, không thể dán hình.");
    return;
  }
  if (!layers[activeLayer].visible) {
    setStatus("Không thể dán vào layer đang ẩn.");
    return;
  }
  snapshot();
  const pasted = structuredClone(copiedShape);
  translateShape(pasted, 16, 16);
  layers[activeLayer].shapes.push(pasted);
  selected = { layer: activeLayer, shape: layers[activeLayer].shapes.length - 1 };
  render();
  setStatus("Đã dán hình vào layer hiện tại");
}

document.querySelector("#add-layer").addEventListener("click", () => {
  snapshot();
  const name = `Layer ${layers.length + 1}`;
  layers.push({ name, visible: true, shapes: [] });
  activeLayer = layers.length - 1;
  selected = null;
  if (layerSearchInput && layerSearchQuery) {
    layerSearchInput.value = "";
    layerSearchQuery = "";
    if (layerSearchClear) layerSearchClear.classList.remove("visible");
  }
  render();
  setStatus(`Đã tạo ${name}`);
});

const layerSearchInput = document.querySelector("#layer-search-input");
const layerSearchClear = document.querySelector("#layer-search-clear");

if (layerSearchInput) {
  layerSearchInput.addEventListener("input", e => {
    layerSearchQuery = e.target.value.trim().toLowerCase();
    if (layerSearchClear) {
      layerSearchClear.classList.toggle("visible", Boolean(e.target.value));
    }
    renderLayers();
  });
}

if (layerSearchClear) {
  layerSearchClear.addEventListener("click", () => {
    if (layerSearchInput) {
      layerSearchInput.value = "";
      layerSearchInput.focus();
    }
    layerSearchClear.classList.remove("visible");
    layerSearchQuery = "";
    renderLayers();
  });
}

// ── Chèn ảnh ──────────────────────────────────────────────
const imageFileInput = document.querySelector("#image-file-input");

document.querySelector("#insert-image-btn").addEventListener("click", () => {
  if (!layers[activeLayer].visible) {
    setStatus("Layer đang bị ẩn. Hãy bật hiển thị trước khi chèn ảnh.");
    return;
  }
  if (layers[activeLayer].locked) {
    setStatus("Layer đang bị khóa. Hãy chọn layer khác để chèn ảnh.");
    return;
  }
  imageFileInput.value = "";
  imageFileInput.click();
});

imageFileInput.addEventListener("change", () => {
  const file = imageFileInput.files[0];
  if (!file) return;
  if (!file.type.startsWith("image/")) {
    setStatus("File không hợp lệ. Vui lòng chọn file ảnh (jpg, png, gif, webp…).");
    return;
  }
  const reader = new FileReader();
  reader.onload = (e) => {
    const src = e.target.result;
    // Lấy kích thước thật của ảnh, tự scale vừa canvas 800×600
    const img = new Image();
    img.onload = () => {
      const maxW = 400, maxH = 300;
      let w = img.naturalWidth || 200;
      let h = img.naturalHeight || 200;
      const scale = Math.min(1, maxW / w, maxH / h);
      w = Math.round(w * scale);
      h = Math.round(h * scale);
      const x = Math.round((CANVAS_WIDTH - w) / 2);
      const y = Math.round((CANVAS_HEIGHT - h) / 2);
      const shape = { type: "image", src, x, y, w, h, opacity: 1, rotation: 0 };
      snapshot();
      layers[activeLayer].shapes.push(shape);
      selected = { layer: activeLayer, shape: layers[activeLayer].shapes.length - 1 };
      render();
      setStatus(`Đã chèn ảnh "${file.name}" vào layer`);
    };
    img.src = src;
  };
  reader.readAsDataURL(file);
});

// Slider độ mờ ảnh
let imageOpacityHistoryCaptured = false;
document.querySelector("#image-opacity").addEventListener("input", (e) => {
  if (!selected) return;
  const shape = layers[selected.layer]?.shapes[selected.shape];
  if (!shape || shape.type !== "image") return;
  if (layers[selected.layer].locked) {
    setStatus("Layer đang bị khóa, không thể chỉnh sửa ảnh.");
    return;
  }
  if (!imageOpacityHistoryCaptured) { snapshot(); imageOpacityHistoryCaptured = true; }
  shape.opacity = Number(e.target.value) / 100;
  document.querySelector("#image-opacity-value").value = `${e.target.value}%`;
  render();
});
document.querySelector("#image-opacity").addEventListener("change", () => {
  imageOpacityHistoryCaptured = false;
});

// ── Văn bản (Text) & Hiệu ứng chữ ─────────────────────────
const inlineTextEditor = document.querySelector("#canvas-inline-text-editor");

function openInlineTextEditor(shape, layerIndex, shapeIndex, initialBox = null) {
  if (!inlineTextEditor) return;
  currentEditingTextTarget = { shape, layerIndex, shapeIndex };

  const stage = document.querySelector("#canvas-stage");
  const stageRect = stage.getBoundingClientRect();
  const drawingRect = drawing.getBoundingClientRect();

  const scaleX = drawingRect.width / CANVAS_WIDTH;
  const scaleY = drawingRect.height / CANVAS_HEIGHT;

  const left = (shape.x * scaleX) + (drawingRect.left - stageRect.left);
  const top = (shape.y * scaleY) + (drawingRect.top - stageRect.top);

  inlineTextEditor.style.left = `${Math.max(4, left)}px`;
  inlineTextEditor.style.top = `${Math.max(4, top)}px`;

  if (initialBox) {
    inlineTextEditor.style.width = `${Math.max(80, Math.round(initialBox.w * scaleX))}px`;
    inlineTextEditor.style.height = `${Math.max(38, Math.round(initialBox.h * scaleY))}px`;
  } else {
    const bounds = shapeBounds(shape);
    inlineTextEditor.style.width = `${Math.max(80, Math.round((bounds.w + 16) * scaleX))}px`;
    inlineTextEditor.style.height = `${Math.max(38, Math.round((bounds.h + 8) * scaleY))}px`;
  }

  inlineTextEditor.style.fontSize = `${Math.max(12, Math.round((shape.fontSize || 24) * scaleY))}px`;
  inlineTextEditor.style.fontFamily = shape.fontFamily || "Segoe UI";
  inlineTextEditor.style.fontWeight = shape.bold ? "bold" : "normal";
  inlineTextEditor.style.fontStyle = shape.italic ? "italic" : "normal";
  inlineTextEditor.style.textDecoration = shape.underline ? "underline" : "none";
  inlineTextEditor.style.color = shape.color || "#000000";
  inlineTextEditor.style.backgroundColor = "transparent";

  inlineTextEditor.value = shape.text || "";
  inlineTextEditor.hidden = false;
  render();
  inlineTextEditor.focus();
  inlineTextEditor.select();
}

function closeInlineTextEditor(save = true) {
  if (!inlineTextEditor || inlineTextEditor.hidden) return;
  const target = currentEditingTextTarget;
  currentEditingTextTarget = null;
  inlineTextEditor.hidden = true;
  if (save && target) {
    const { shape } = target;
    const newText = inlineTextEditor.value.trim();
    if (newText !== undefined && newText !== shape.text) {
      snapshot();
      shape.text = newText || "Văn bản mới";
      setStatus("Đã cập nhật văn bản");
    }
  }
  render();
}

if (inlineTextEditor) {
  inlineTextEditor.addEventListener("input", () => {
    if (inlineTextEditor.scrollHeight > inlineTextEditor.clientHeight) {
      inlineTextEditor.style.height = `${inlineTextEditor.scrollHeight + 6}px`;
    }
  });
  inlineTextEditor.addEventListener("blur", () => closeInlineTextEditor(true));
  inlineTextEditor.addEventListener("keydown", event => {
    if (event.key === "Escape") {
      event.preventDefault();
      closeInlineTextEditor(false);
    } else if (event.key === "Enter" && (event.ctrlKey || event.metaKey)) {
      event.preventDefault();
      closeInlineTextEditor(true);
    }
  });
}

// Bảng thuộc tính Text
const textContentInput = document.querySelector("#text-content-input");
const textFontFamily = document.querySelector("#text-font-family");
const textFontSize = document.querySelector("#text-font-size");
const textSizeValue = document.querySelector("#text-size-value");
const textColor = document.querySelector("#text-color");
const textBoldBtn = document.querySelector("#text-bold-btn");
const textItalicBtn = document.querySelector("#text-italic-btn");
const textUnderlineBtn = document.querySelector("#text-underline-btn");

let textContentHistoryCaptured = false;
if (textContentInput) {
  textContentInput.addEventListener("input", e => {
    if (!selected) return;
    const shape = layers[selected.layer]?.shapes[selected.shape];
    if (!shape || shape.type !== "text") return;
    if (!textContentHistoryCaptured) { snapshot(); textContentHistoryCaptured = true; }
    shape.text = e.target.value;
    render();
  });
  textContentInput.addEventListener("change", () => {
    textContentHistoryCaptured = false;
  });
}

if (textFontFamily) {
  textFontFamily.addEventListener("change", e => {
    currentTextFontFamily = e.target.value;
    if (!selected) return;
    const shape = layers[selected.layer]?.shapes[selected.shape];
    if (!shape || shape.type !== "text") return;
    snapshot();
    shape.fontFamily = currentTextFontFamily;
    render();
    setStatus(`Đã đổi phông chữ thành ${shape.fontFamily}`);
  });
}

let textSizeHistoryCaptured = false;
if (textFontSize) {
  textFontSize.addEventListener("input", e => {
    const size = Number(e.target.value);
    currentTextFontSize = size;
    if (textSizeValue) textSizeValue.value = `${size}px`;
    if (!selected) return;
    const shape = layers[selected.layer]?.shapes[selected.shape];
    if (!shape || shape.type !== "text") return;
    if (!textSizeHistoryCaptured) { snapshot(); textSizeHistoryCaptured = true; }
    shape.fontSize = size;
    render();
  });
  textFontSize.addEventListener("change", () => {
    textSizeHistoryCaptured = false;
  });
}

let textColorHistoryCaptured = false;
if (textColor) {
  textColor.addEventListener("input", e => {
    currentTextColor = e.target.value;
    if (!selected) return;
    const shape = layers[selected.layer]?.shapes[selected.shape];
    if (!shape || shape.type !== "text") return;
    if (!textColorHistoryCaptured) { snapshot(); textColorHistoryCaptured = true; }
    shape.color = currentTextColor;
    render();
  });
  textColor.addEventListener("change", () => {
    textColorHistoryCaptured = false;
  });
}

if (textBoldBtn) {
  textBoldBtn.addEventListener("click", () => {
    if (!selected) return;
    const shape = layers[selected.layer]?.shapes[selected.shape];
    if (!shape || shape.type !== "text") return;
    snapshot();
    shape.bold = !shape.bold;
    currentTextBold = shape.bold;
    textBoldBtn.classList.toggle("active", shape.bold);
    render();
    setStatus(shape.bold ? "Đã bật in đậm" : "Đã tắt in đậm");
  });
}

if (textItalicBtn) {
  textItalicBtn.addEventListener("click", () => {
    if (!selected) return;
    const shape = layers[selected.layer]?.shapes[selected.shape];
    if (!shape || shape.type !== "text") return;
    snapshot();
    shape.italic = !shape.italic;
    currentTextItalic = shape.italic;
    textItalicBtn.classList.toggle("active", shape.italic);
    render();
    setStatus(shape.italic ? "Đã bật chữ nghiêng" : "Đã tắt chữ nghiêng");
  });
}

if (textUnderlineBtn) {
  textUnderlineBtn.addEventListener("click", () => {
    if (!selected) return;
    const shape = layers[selected.layer]?.shapes[selected.shape];
    if (!shape || shape.type !== "text") return;
    snapshot();
    shape.underline = !shape.underline;
    currentTextUnderline = shape.underline;
    textUnderlineBtn.classList.toggle("active", shape.underline);
    render();
    setStatus(shape.underline ? "Đã bật gạch chân" : "Đã tắt gạch chân");
  });
}

function updateThemeToggleUI() {
  const button = document.querySelector("#theme-toggle");
  if (!button) return;
  const sunSvg = `<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align: -2px; margin-right: 4px;"><circle cx="12" cy="12" r="5"/><line x1="12" y1="1" x2="12" y2="3"/><line x1="12" y1="21" x2="12" y2="23"/><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"/><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"/><line x1="1" y1="12" x2="3" y2="12"/><line x1="21" y1="12" x2="23" y2="12"/><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"/><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"/></svg>`;
  const moonSvg = `<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align: -2px; margin-right: 4px;"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"/></svg>`;
  button.innerHTML = theme === "dark" ? `${sunSvg}<span>Sáng</span>` : `${moonSvg}<span>Tối</span>`;
  button.title = theme === "dark" ? "Chuyển sang giao diện sáng" : "Chuyển sang giao diện tối";
}

document.querySelector("#theme-toggle").addEventListener("click", () => {
  theme = theme === "dark" ? "light" : "dark";
  localStorage.setItem("mini-vector-cad-theme", theme);
  render();
  updateThemeToggleUI();
  setStatus(`Đã chuyển sang giao diện ${theme === "dark" ? "tối" : "sáng"}`);
});

document.querySelector("#undo").addEventListener("click", () => {
  if (!undoStack.length) return;
  redoStack.push(JSON.stringify({ layers, activeLayer }));
  restore(undoStack.pop());
});
document.querySelector("#redo").addEventListener("click", () => {
  if (!redoStack.length) return;
  undoStack.push(JSON.stringify({ layers, activeLayer }));
  restore(redoStack.pop());
});

function downloadSvg() {
  const svg = document.createElementNS(svgNS, "svg");
  svg.setAttribute("xmlns", svgNS);
  svg.setAttribute("viewBox", `0 0 ${CANVAS_WIDTH} ${CANVAS_HEIGHT}`);
  svg.setAttribute("width", CANVAS_WIDTH.toString());
  svg.setAttribute("height", CANVAS_HEIGHT.toString());
  layers.forEach((layer, index) => {
    if (!layer.visible) return;
    const group = makeSvg("g", { id: `layer_${index}` });
    group.setAttribute("data-name", layer.name);
    layer.shapes.forEach(shape => {
      let element;
      if (shape.type === "image") {
        element = makeSvg("image", {
          x: shape.x, y: shape.y, width: shape.w, height: shape.h,
          href: shape.src,
          opacity: shape.opacity ?? 1,
          preserveAspectRatio: "none"
        });
        if (shape.rotation) {
          const cx = shape.x + shape.w / 2, cy = shape.y + shape.h / 2;
          element.setAttribute("transform", `rotate(${shape.rotation} ${cx} ${cy})`);
        }
      } else if (shape.type === "curve") {
        element = makeSvg("path", {
          d: `M ${shape.x1} ${shape.y1} Q ${shape.cx} ${shape.cy} ${shape.x2} ${shape.y2}`,
          fill: "none",
          stroke: shape.stroke,
          "stroke-width": shape.width,
          "stroke-linecap": "round",
          "stroke-linejoin": "round"
        });
        if (shape.rotation) {
          const b = shapeBounds(shape);
          element.setAttribute("transform", `rotate(${shape.rotation} ${b.x + b.w / 2} ${b.y + b.h / 2})`);
        }
      } else if (shape.type === "wave") {
        element = makeSvg("path", {
          d: wavePathData(shape.x1, shape.y1, shape.x2, shape.y2, shape.amplitude || 15, shape.cycles || 3),
          fill: "none",
          stroke: shape.stroke,
          "stroke-width": shape.width,
          "stroke-linecap": "round",
          "stroke-linejoin": "round"
        });
        if (shape.rotation) {
          const b = shapeBounds(shape);
          element.setAttribute("transform", `rotate(${shape.rotation} ${b.x + b.w / 2} ${b.y + b.h / 2})`);
        }
      } else if (shape.type === "arrow") {
        element = makeSvg("path", {
          d: arrowPathData(shape.x1, shape.y1, shape.x2, shape.y2),
          fill: "none",
          stroke: shape.stroke,
          "stroke-width": shape.width,
          "stroke-linecap": "round",
          "stroke-linejoin": "round"
        });
        if (shape.rotation) {
          const b = shapeBounds(shape);
          element.setAttribute("transform", `rotate(${shape.rotation} ${b.x + b.w / 2} ${b.y + b.h / 2})`);
        }
      } else {
        element = shapeElement(shape, index, -1);
        element.removeAttribute("class");
        element.removeAttribute("data-layer");
        element.removeAttribute("data-shape");
      }
      group.append(element);
    });
    svg.append(group);
  });
  const titleEl = document.createElementNS(svgNS, "title");
  titleEl.textContent = documentTitle;
  svg.append(titleEl);
  const blob = new Blob([new XMLSerializer().serializeToString(svg)], { type: "image/svg+xml;charset=utf-8" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  const safeName = documentTitle.replace(/[\\/:*?"<>|]/g, "_").trim() || "mini-vector-cad";
  link.download = `${safeName}.svg`;
  link.click();
  setTimeout(() => URL.revokeObjectURL(link.href), 0);
  setStatus(`Đã xuất file "${safeName}.svg"`);
}

document.querySelector("#export").addEventListener("click", downloadSvg);
document.querySelector("#zoom-in").addEventListener("click", () => setZoom(zoom + 10));
document.querySelector("#zoom-out").addEventListener("click", () => setZoom(zoom - 10));
document.querySelector("#fit").addEventListener("click", () => {
  const stage = document.querySelector("#canvas-stage");
  if (!stage) { setZoom(100); return; }
  const availW = Math.max(100, stage.clientWidth - 60);
  const availH = Math.max(100, stage.clientHeight - 60);
  const fitZoom = Math.min(availW / CANVAS_WIDTH, availH / CANVAS_HEIGHT) * 100;
  setZoom(Math.max(25, Math.min(400, Math.round(fitZoom / 5) * 5)));
  stage.scrollLeft = Math.max(0, (stage.scrollWidth - stage.clientWidth) / 2);
  stage.scrollTop = Math.max(0, (stage.scrollHeight - stage.clientHeight) / 2);
});

function setZoom(value, anchor = null) {
  const stage = document.querySelector("#canvas-stage");
  const stageRect = stage ? stage.getBoundingClientRect() : null;
  const drawingRect = drawing.getBoundingClientRect();

  // Xác định điểm neo trên màn hình (Anchor Screen Point)
  let cursorX, cursorY;
  if (anchor && typeof anchor.clientX === "number" && typeof anchor.clientY === "number") {
    cursorX = anchor.clientX;
    cursorY = anchor.clientY;
  } else if (stageRect) {
    cursorX = stageRect.left + stageRect.clientWidth / 2;
    cursorY = stageRect.top + stageRect.clientHeight / 2;
  } else {
    cursorX = drawingRect.left + drawingRect.width / 2;
    cursorY = drawingRect.top + drawingRect.height / 2;
  }

  // Tỷ lệ vị trí con trỏ trên tờ giấy vẽ trước khi đổi kích thước
  const ratioX = drawingRect.width > 0 ? (cursorX - drawingRect.left) / drawingRect.width : 0.5;
  const ratioY = drawingRect.height > 0 ? (cursorY - drawingRect.top) / drawingRect.height : 0.5;

  const targetZoom = Math.max(25, Math.min(400, Math.round(value)));
  zoom = targetZoom;

  const w = Math.round(CANVAS_WIDTH * zoom / 100);
  const h = Math.round(CANVAS_HEIGHT * zoom / 100);
  drawing.style.width = `${w}px`;
  drawing.style.height = `${h}px`;
  drawing.style.minWidth = `${w}px`;
  drawing.style.minHeight = `${h}px`;
  document.querySelector("#zoom-label").textContent = `${zoom}%`;

  // Căn chỉnh thanh cuộn để điểm dưới con trỏ chuột giữ nguyên tuyệt đối trên màn hình
  if (stage) {
    const newDrawingRect = drawing.getBoundingClientRect();
    const newPointScreenX = newDrawingRect.left + ratioX * newDrawingRect.width;
    const newPointScreenY = newDrawingRect.top + ratioY * newDrawingRect.height;
    stage.scrollLeft += Math.round(newPointScreenX - cursorX);
    stage.scrollTop += Math.round(newPointScreenY - cursorY);
  }

  const cp = (anchor && typeof anchor.clientX === "number") ? pointFromEvent(anchor) : null;
  renderRulers(cp);
  renderSelectionToolbar();
}

// Zoom bằng cuộn chuột: phóng to/thu nhỏ ngay tại vị trí con trỏ chuột
document.querySelector("#canvas-stage").addEventListener("wheel", (event) => {
  event.preventDefault();
  const delta = event.deltaY < 0 ? 10 : -10;
  setZoom(zoom + delta, event);
}, { passive: false });

document.addEventListener("wheel", (event) => {
  if (!event.ctrlKey) return;
  if (event.target.closest && event.target.closest("#canvas-stage")) return;
  event.preventDefault();
  const delta = event.deltaY < 0 ? 10 : -10;
  setZoom(zoom + delta, event);
}, { passive: false });

document.querySelector("#help").addEventListener("click", () => {
  setStatus("Phím tắt: V (Di chuyển) · G (Quét nhóm) · Ctrl+G (Nhóm) · Ctrl+Shift+G (Rã nhóm) · M (Thước CAD) · Delete (Xóa) · Ctrl+Z/Y/C/V");
});

function colorChannels(color, alpha = 255) {
  const hex = color.slice(1);
  return [0, 2, 4].map(index => Number.parseInt(hex.slice(index, index + 2), 16)).concat(alpha);
}

function serializeNativeShape(shape) {
  if (shape.type === "image" || shape.type === "text") return null; // Ảnh và Text lưu cục bộ trên giao diện Web
  const stroke = colorChannels(shape.stroke, Math.round((shape.opacity ?? 1) * 255));
  const fill = shape.fill && shape.fill !== "none" ? colorChannels(shape.fill) : [255, 255, 255, 0];
  let geometry;
  if (shape.type === "rect") geometry = `chunhat ${shape.x} ${shape.y} ${shape.w} ${shape.h}`;
  else if (shape.type === "circle") geometry = `tron ${shape.x} ${shape.y} ${shape.r}`;
  else if (shape.type === "ellipse") {
    const pts = [];
    for (let i = 0; i < 24; i++) {
      const a = (i * 2 * Math.PI) / 24;
      pts.push(Math.round(shape.cx + shape.rx * Math.cos(a)), Math.round(shape.cy + shape.ry * Math.sin(a)));
    }
    geometry = `dagiac ${pts.join(" ")}`;
  }
  else if (shape.type === "line" || shape.type === "arrow") geometry = `doanthang ${shape.x1} ${shape.y1} ${shape.x2} ${shape.y2}`;
  else if (shape.type === "curve") geometry = `doancong ${shape.x1} ${shape.y1} ${shape.cx} ${shape.cy} ${shape.x2} ${shape.y2}`;
  else if (shape.type === "wave") geometry = `doansong ${shape.x1} ${shape.y1} ${shape.x2} ${shape.y2} ${shape.amplitude || 15} ${shape.cycles || 3}`;
  else if (shape.type === "triangle") geometry = `tamgiac ${shape.points.flat().join(" ")}`;
  else if (shape.type === "polygon" || shape.type === "star" || shape.type === "parallelogram" || shape.type === "trapezoid") geometry = `dagiac ${shape.points.flat().join(" ")}`;
  else if (shape.type === "brush") geometry = `brush ${shape.points.length} ${shape.points.flat().join(" ")}`;
  else throw new Error(`Loại hình "${shape.type}" chưa hỗ trợ lưu sang C++.`);
  return `${geometry} vien ${stroke.join(" ")} nen ${fill.join(" ")} net ${shape.width}`;
}

function serializeNativeProject() {
  const lines = ["MINI_VECTOR_CAD_V1", `LAYERS ${layers.length}`];
  function flattenShapes(shapes) {
    const res = [];
    shapes.forEach(s => {
      if (s.type === "group") res.push(...flattenShapes(s.children || []));
      else res.push(s);
    });
    return res;
  }
  for (const layer of layers) {
    const name = layer.name.replace(/\\/g, "\\\\").replace(/"/g, '\\"').replace(/\n/g, "\\n").replace(/\t/g, "\\t").replace(/\r/g, "\\r");
    const nonNativeShapes = flattenShapes(layer.shapes).filter(s => s.type !== "image" && s.type !== "text");
    lines.push(`LAYER "${name}" ${layer.visible ? 1 : 0} ${layer.locked ? 1 : 0} ${nonNativeShapes.length}`);
    nonNativeShapes.forEach(shape => lines.push(serializeNativeShape(shape)));
  }
  return `${lines.join("\n")}\n`;
}

function parseNativeProject(text) {
  const lines = text.split(/\r?\n/).map(line => line.trim()).filter(Boolean);
  if (lines[0] !== "MINI_VECTOR_CAD_V1") throw new Error("Không đúng định dạng bản vẽ Mini Vector CAD.");
  const layerCountMatch = /^LAYERS\s+(\d+)$/.exec(lines[1] || "");
  if (!layerCountMatch) throw new Error("Thiếu khai báo số layer.");
  const layerCount = Number(layerCountMatch[1]);
  if (layerCount < 1 || layerCount > 1000) throw new Error("Số layer trong file không hợp lệ.");
  let lineIndex = 2;
  let totalShapes = 0;

  function readStyle(tokens, start) {
    const style = { stroke: "#000000", fill: "none", width: 1, opacity: 1 };
    let cursor = start;
    while (cursor < tokens.length) {
      const tag = tokens[cursor++].toLowerCase();
      if (["vien", "stroke", "nen", "fill"].includes(tag)) {
        if (cursor + 2 >= tokens.length) throw new Error("Thiếu thành phần màu trong file.");
        const rgba = tokens.slice(cursor, cursor + 3).map(Number);
        cursor += 3;
        let alpha = 255;
        if (cursor < tokens.length && !["vien", "stroke", "nen", "fill", "net", "width"].includes(tokens[cursor].toLowerCase())) {
          alpha = Number(tokens[cursor++]);
        }
        if (rgba.some(value => !Number.isInteger(value) || value < 0 || value > 255) || !Number.isInteger(alpha) || alpha < 0 || alpha > 255) {
          throw new Error("Giá trị màu trong file không hợp lệ.");
        }
        const color = `#${rgba.map(value => value.toString(16).padStart(2, "0")).join("")}`;
        if (tag === "vien" || tag === "stroke") {
          style.stroke = color;
          style.opacity = alpha / 255;
        } else {
          style.fill = alpha ? color : "none";
        }
      } else if (tag === "net" || tag === "width") {
        style.width = Number(tokens[cursor++]);
        if (!Number.isFinite(style.width) || style.width <= 0) throw new Error("Độ dày nét trong file không hợp lệ.");
      } else {
        throw new Error(`Thuộc tính bản vẽ chưa hỗ trợ: ${tag}`);
      }
    }
    return style;
  }

  function readShape() {
    const tokens = (lines[lineIndex++] || "").split(/\s+/);
    const type = tokens[0]?.toLowerCase();
    if (!type) throw new Error("File có hình vẽ bị thiếu dữ liệu.");
    if (type === "group" || type === "nhom") {
      const count = Number(tokens[1]);
      if (!Number.isInteger(count) || count < 0 || count > 10000) throw new Error("Số hình trong group không hợp lệ.");
      readStyle(tokens, 2);
      const children = [];
      for (let index = 0; index < count; index++) children.push(...readShape());
      return children;
    }
    if (++totalShapes > 100000) throw new Error("File vượt quá giới hạn số hình cho phép.");

    let shape;
    let cursor;
    if (type === "brush") {
      const count = Number(tokens[1]);
      if (!Number.isInteger(count) || count < 1 || count > 100000 || 2 + count * 2 > tokens.length) {
        throw new Error("Dữ liệu nét bút trong file không hợp lệ.");
      }
      const values = tokens.slice(2, 2 + count * 2).map(Number);
      if (values.some(value => !Number.isFinite(value))) throw new Error("Tọa độ nét bút không hợp lệ.");
      shape = { type: "brush", points: Array.from({ length: count }, (_, index) => values.slice(index * 2, index * 2 + 2)) };
      cursor = 2 + count * 2;
    } else {
      const coordinateCount = {
        tron: 3, circle: 3,
        chunhat: 4, rect: 4, rectangle: 4,
        doanthang: 4, line: 4,
        doancong: 6, curve: 6,
        doansong: 6, wave: 6,
        tamgiac: 6, triangle: 6
      }[type];
      if (type === "dagiac" || type === "polygon") {
        cursor = tokens.findIndex((token, index) => index > 0 && ["vien", "stroke", "nen", "fill", "net", "width"].includes(token.toLowerCase()));
        if (cursor < 0) cursor = tokens.length;
        const values = tokens.slice(1, cursor).map(Number);
        if (values.length < 6 || values.length % 2 || values.some(value => !Number.isFinite(value))) throw new Error("Đa giác trong file không hợp lệ.");
        shape = {
          type: values.length === 6 ? "triangle" : "polygon",
          points: Array.from({ length: values.length / 2 }, (_, index) => values.slice(index * 2, index * 2 + 2))
        };
      } else {
        if (!coordinateCount || tokens.length < coordinateCount + 1) throw new Error(`Loại hình chưa hỗ trợ hoặc dữ liệu thiếu: ${type}`);
        const values = tokens.slice(1, coordinateCount + 1).map(Number);
        if (values.some(value => !Number.isFinite(value))) throw new Error("Tọa độ hình vẽ không hợp lệ.");
        if (type === "tron" || type === "circle") shape = { type: "circle", x: values[0], y: values[1], r: values[2] };
        else if (type === "chunhat" || type === "rect" || type === "rectangle") shape = { type: "rect", x: values[0], y: values[1], w: values[2], h: values[3] };
        else if (type === "doanthang" || type === "line") shape = { type: "line", x1: values[0], y1: values[1], x2: values[2], y2: values[3] };
        else if (type === "doancong" || type === "curve") shape = { type: "curve", x1: values[0], y1: values[1], cx: values[2], cy: values[3], x2: values[4], y2: values[5] };
        else if (type === "doansong" || type === "wave") shape = { type: "wave", x1: values[0], y1: values[1], x2: values[2], y2: values[3], amplitude: values[4], cycles: values[5] };
        else shape = { type: "triangle", points: [[values[0], values[1]], [values[2], values[3]], [values[4], values[5]]] };
        cursor = coordinateCount + 1;
      }
      shape.rotation = 0;
    }
    return [{ ...shape, ...readStyle(tokens, cursor) }];
  }

  function unescapeName(name) {
    return name.replace(/\\(["\\ntr])/g, (_, char) => ({ '"': '"', "\\": "\\", n: "\n", t: "\t", r: "\r" })[char]);
  }

  const importedLayers = [];
  for (let index = 0; index < layerCount; index++) {
    const header = /^LAYER\s+"((?:\\.|[^"])*)"\s+([01])\s+([01])\s+(\d+)$/.exec(lines[lineIndex++] || "");
    if (!header) throw new Error("Khai báo layer không hợp lệ.");
    const shapeCount = Number(header[4]);
    if (shapeCount > 100000) throw new Error("Layer vượt quá giới hạn số hình cho phép.");
    const shapes = [];
    for (let shapeIndex = 0; shapeIndex < shapeCount; shapeIndex++) shapes.push(...readShape());
    importedLayers.push({
      name: unescapeName(header[1]),
      visible: header[2] === "1",
      locked: header[3] === "1",
      shapes
    });
  }
  if (lineIndex !== lines.length) throw new Error("File còn dữ liệu thừa sau khi đọc xong bản vẽ.");
  return importedLayers;
}

function updateBackendStatus(connected) {
  backendConnected = connected;
  document.querySelector("#backend-status").textContent = connected ? "C++ đã kết nối · tự đồng bộ" : "C++ đang kết nối lại...";
}

function scheduleCppSync() {
  hasPendingLocalChanges = true;
  projectRevision += 1;
  if (!backendConnected) return;
  clearTimeout(backendSyncTimer);
  backendSyncTimer = setTimeout(() => syncCppProject(false), 250);
}

// Tự động nhận diện URL máy chủ C++:
const API_BASE_URL = (() => {
  if (!window.location.protocol.startsWith("http")) return "http://127.0.0.1:8766";
  // Khi chạy trên Render (HTTPS port chuẩn 443) hoặc trực tiếp server C++ (port 8766): dùng đường dẫn tương đối ""
  if (window.location.port === "" || window.location.port === "8766" || window.location.port === "443" || window.location.port === "80") {
    return "";
  }
  // Khi chạy qua Live Server cục bộ (port 5500, 8765): gọi về backend port 8766
  return `${window.location.protocol}//${window.location.hostname}:8766`;
})();

let currentProjectId = localStorage.getItem("mini-vector-cad-current-project-id") || "proj_default";

function getProjectApiUrl(id = currentProjectId) {
  return `${API_BASE_URL}/api/project?id=` + encodeURIComponent(id || "proj_default");
}

function getAuthHeaders(extra = {}) {
  return {
    ...extra,
    ...(authToken ? { "Authorization": `Bearer ${authToken}` } : {})
  };
}

async function syncCppProject(reportStatus = true) {
  // 1. TÀI KHOẢN KHÁCH: Vẽ riêng cục bộ trên trình duyệt, không gửi lên C++ server
  if (!currentUser || !authToken) {
    try {
      localStorage.setItem("mini-vector-cad-guest-drawing", serializeNativeProject());
      hasPendingLocalChanges = false;
      if (reportStatus) setStatus("🔒 Bản vẽ riêng của Khách đã lưu cục bộ.");
    } catch (_) {}
    return;
  }

  // 2. TÀI KHOẢN ĐÃ ĐĂNG NHẬP: Đồng bộ lên C++ server
  if (!backendConnected) return;
  if (backendSyncInProgress) {
    backendSyncPending = true;
    return;
  }
  backendSyncInProgress = true;
  let succeeded = true;
  do {
    backendSyncPending = false;
    const revision = projectRevision;
    try {
      const response = await fetch(getProjectApiUrl(), {
        method: "POST",
        headers: getAuthHeaders({ "Content-Type": "text/plain;charset=UTF-8" }),
        body: serializeNativeProject()
      });
      if (!response.ok) throw new Error(`C++ backend trả về HTTP ${response.status}.`);
      await response.text();
      if (revision === projectRevision) hasPendingLocalChanges = false;
      else backendSyncPending = true;
    } catch (error) {
      succeeded = false;
      updateBackendStatus(false);
      setStatus(`Mất kết nối C++: ${error.message}`);
      scheduleCppReconnect();
    }
  } while (backendSyncPending && succeeded);
  backendSyncInProgress = false;
  if (reportStatus && succeeded) {
    const curProj = userProjects.find(p => p.id === currentProjectId);
    const isCollab = curProj && curProj.members && curProj.members.length > 0;
    setStatus(isCollab ? "Đã đồng bộ bản vẽ chung với C++" : "Đã lưu bản vẽ riêng lên C++ Cloud");
  }
}

function scheduleCppReconnect() {
  if (backendReconnectTimer || backendConnected) return;
  backendReconnectTimer = setTimeout(() => {
    backendReconnectTimer = 0;
    connectCppBackend();
  }, 3000);
}

async function connectCppBackend() {
  if (!currentUser || !authToken) {
    // Khách: nạp từ localStorage của khách nếu có
    const savedGuest = localStorage.getItem("mini-vector-cad-guest-drawing");
    if (savedGuest) {
      try {
        const importedLayers = parseNativeProject(savedGuest);
        if (importedLayers && importedLayers.length > 0) {
          layers.splice(0, layers.length, ...importedLayers);
          activeLayer = 0;
          selected = null;
          render();
        }
      } catch (_) {}
    }
    backendConnected = true;
    const statusEl = document.querySelector("#backend-status");
    if (statusEl) statusEl.textContent = "Chế độ Khách · Bản vẽ riêng (Cục bộ)";
    setStatus("🔒 Bạn đang ở Chế độ Khách (Bản vẽ riêng, không chia sẻ với ai)");
    return;
  }

  if (backendConnected) return;
  try {
    if (hasPendingLocalChanges) {
      const revision = projectRevision;
      const response = await fetch(getProjectApiUrl(), {
        method: "POST",
        headers: getAuthHeaders({ "Content-Type": "text/plain;charset=UTF-8" }),
        body: serializeNativeProject()
      });
      if (!response.ok) throw new Error(`C++ backend trả về HTTP ${response.status}.`);
      await response.text();
      backendConnected = true;
      document.querySelector("#backend-status").textContent = "C++ đã kết nối · tự đồng bộ";
      if (revision === projectRevision) hasPendingLocalChanges = false;
      else scheduleCppSync();
    } else {
      const revision = projectRevision;
      const response = await fetch(getProjectApiUrl(), {
        headers: getAuthHeaders()
      });
      if (!response.ok) throw new Error(`C++ backend trả về HTTP ${response.status}.`);
      const importedLayers = parseNativeProject(await response.text());
      if (hasPendingLocalChanges || revision !== projectRevision) {
        backendConnected = true;
        document.querySelector("#backend-status").textContent = "C++ đã kết nối · tự đồng bộ";
        await syncCppProject(false);
        return;
      }
      layers.splice(0, layers.length, ...importedLayers);
      activeLayer = 0;
      selected = null;
      hasPendingLocalChanges = false;
      render();
      backendConnected = true;
      document.querySelector("#backend-status").textContent = "C++ đã kết nối · tự đồng bộ";
    }
    setStatus("Đã kết nối C++ — thay đổi sẽ tự động đồng bộ");
  } catch (error) {
    updateBackendStatus(false);
    setStatus(`Không kết nối được C++: ${error.message}`);
    scheduleCppReconnect();
  }
}

window.addEventListener("resize", renderSelectionToolbar);

document.addEventListener("keydown", event => {
  if (event.ctrlKey && event.shiftKey && event.key.toLowerCase() === "g") {
    event.preventDefault();
    ungroupSelectedShape();
  } else if (event.ctrlKey && event.key.toLowerCase() === "g") {
    event.preventDefault();
    groupSelectedShapes();
  } else if (event.ctrlKey && event.key.toLowerCase() === "z") {
    event.preventDefault();
    document.querySelector("#undo").click();
  } else if (event.ctrlKey && event.key.toLowerCase() === "y") {
    event.preventDefault();
    document.querySelector("#redo").click();
  } else if (event.ctrlKey && event.key.toLowerCase() === "c") {
    if (selected) {
      event.preventDefault();
      document.querySelector("#copy-shape").click();
    }
  } else if (event.ctrlKey && event.key.toLowerCase() === "v") {
    if (copiedShape) {
      event.preventDefault();
      pasteCopiedShape();
    }
  } else if (event.key === "Delete") {
    deleteSelectedShape();
  } else if (event.key === "Escape") {
    if (selectFlyoutBoard && !selectFlyoutBoard.hidden) selectFlyoutBoard.hidden = true;
    if (shapesFlyoutBoard && !shapesFlyoutBoard.hidden) shapesFlyoutBoard.hidden = true;
    if (linesFlyoutBoard && !linesFlyoutBoard.hidden) linesFlyoutBoard.hidden = true;
    if (brushFlyoutBoard && !brushFlyoutBoard.hidden) brushFlyoutBoard.hidden = true;
  } else if (event.key.toLowerCase() === "m" && !event.ctrlKey && !["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) {
    if (rulerToolBtn) rulerToolBtn.click();
  } else if (event.key.toLowerCase() === "g" && !event.ctrlKey && !["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) {
    setTool("group_select");
  } else if (event.key.toLowerCase() === "i" && !event.ctrlKey && !["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) {
    document.querySelector("#insert-image-btn").click();
  } else if (["v", "r", "c", "l", "t", "p", "e", "s", "a", "o", "b", "h"].includes(event.key.toLowerCase()) && !["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) {
    const tool = { v: "select", r: "rect", c: "circle", l: "line", t: "text", p: "brush", e: "eraser", s: "star", a: "arrow", o: "ellipse", b: "parallelogram", h: "trapezoid" }[event.key.toLowerCase()];
    if (selectFlyoutBoard) selectFlyoutBoard.hidden = true;
    if (shapesFlyoutBoard) shapesFlyoutBoard.hidden = true;
    if (linesFlyoutBoard) linesFlyoutBoard.hidden = true;
    if (brushFlyoutBoard) brushFlyoutBoard.hidden = true;
    setTool(tool);
  }
});

setZoom(100);
render();
updateThemeToggleUI();
connectCppBackend();

// ── Đổi tên bản vẽ tại Breadcrumb (Bản vẽ / Untitled) ───────
function applyDocumentTitle(newTitle, announce = true) {
  documentTitle = (newTitle || "").trim() || "Untitled";
  localStorage.setItem("mini-vector-cad-title", documentTitle);
  const breadcrumbSpan = document.querySelector("#breadcrumb-title");
  if (breadcrumbSpan) breadcrumbSpan.textContent = documentTitle;
  document.title = `${documentTitle} · Mini Vector CAD`;
  if (announce) setStatus(`Đã đổi tên bản vẽ thành "${documentTitle}"`);
}

function startEditingTitleAt(targetEl, inputClass = "breadcrumb-title-input") {
  if (!targetEl || targetEl.tagName === "INPUT") return;
  const currentTitle = documentTitle;
  const input = document.createElement("input");
  input.type = "text";
  input.className = inputClass;
  input.value = currentTitle;
  input.maxLength = 80;

  let committed = false;
  function commitRename() {
    if (committed) return;
    committed = true;
    const val = input.value.trim();
    applyDocumentTitle(val || currentTitle, Boolean(val && val !== currentTitle));
    input.replaceWith(targetEl);
    targetEl.textContent = documentTitle;
  }

  function cancelRename() {
    if (committed) return;
    committed = true;
    input.replaceWith(targetEl);
    targetEl.textContent = documentTitle;
  }

  input.addEventListener("blur", commitRename);
  input.addEventListener("keydown", event => {
    if (event.key === "Enter") {
      event.preventDefault();
      input.blur();
    } else if (event.key === "Escape") {
      event.preventDefault();
      cancelRename();
    }
  });

  targetEl.replaceWith(input);
  input.focus();
  input.select();
}

const breadcrumbTitleEl = document.querySelector("#breadcrumb-title");
if (breadcrumbTitleEl) {
  breadcrumbTitleEl.addEventListener("click", () => startEditingTitleAt(breadcrumbTitleEl, "breadcrumb-title-input"));
  breadcrumbTitleEl.addEventListener("keydown", event => {
    if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      startEditingTitleAt(breadcrumbTitleEl, "breadcrumb-title-input");
    }
  });
}
const breadcrumbRenameBtn = document.querySelector("#breadcrumb-rename-btn");
if (breadcrumbRenameBtn) {
  breadcrumbRenameBtn.addEventListener("click", () => startEditingTitleAt(breadcrumbTitleEl, "breadcrumb-title-input"));
}

applyDocumentTitle(documentTitle, false);

// ============================================================================
// USER AUTHENTICATION & SESSION MANAGEMENT (C++ BACKEND)
// ============================================================================
let currentUser = null;
let authToken = localStorage.getItem("mini-vector-cad-auth-token") || "";

try {
  const savedUser = localStorage.getItem("mini-vector-cad-user");
  if (savedUser) currentUser = JSON.parse(savedUser);
} catch (_) {}

function showAuthMessage(elementId, message, isSuccess = false) {
  const el = document.getElementById(elementId);
  if (!el) return;
  el.textContent = message;
  el.className = `auth-msg ${isSuccess ? "success" : "error"}`;
  el.hidden = false;
}

function hideAuthMessage(elementId) {
  const el = document.getElementById(elementId);
  if (el) el.hidden = true;
}

function openLoginModal() {
  closeRegisterModal();
  hideAuthMessage("login-msg");
  const modal = document.getElementById("modal-login");
  if (modal) {
    modal.hidden = false;
    setTimeout(() => document.getElementById("login-username")?.focus(), 50);
  }
}

function closeLoginModal() {
  const modal = document.getElementById("modal-login");
  if (modal) modal.hidden = true;
}

function openRegisterModal() {
  closeLoginModal();
  hideAuthMessage("register-msg");
  const modal = document.getElementById("modal-register");
  if (modal) {
    modal.hidden = false;
    setTimeout(() => document.getElementById("register-fullname")?.focus(), 50);
  }
}

function closeRegisterModal() {
  const modal = document.getElementById("modal-register");
  if (modal) modal.hidden = true;
}

function renderUserAuthSection() {
  const container = document.getElementById("user-auth-section");
  if (!container) return;

  if (currentUser) {
    const displayName = currentUser.fullName || currentUser.username;
    const initial = displayName.charAt(0).toUpperCase();
    container.innerHTML = `
      <div class="user-profile-badge" id="btn-user-profile" title="Xem tài khoản: ${displayName}">
        <span class="user-avatar">${initial}</span>
        <span>${displayName}</span>
        <span style="font-size: 8px; opacity: 0.7;">▼</span>
      </div>
      <div class="user-dropdown-board" id="user-dropdown-board" hidden>
        <div class="user-dropdown-header">
          <div class="user-dropdown-name">${displayName}</div>
          <div class="user-dropdown-username">@${currentUser.username}</div>
          <div style="font-size: 10px; color: var(--muted); margin-top: 4px;">Tạo: ${currentUser.createdAt || "Mới"}</div>
        </div>
        <button type="button" class="user-dropdown-logout-btn" id="btn-logout">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"></path>
            <polyline points="16 17 21 12 16 7"></polyline>
            <line x1="21" y1="12" x2="9" y2="12"></line>
          </svg>
          <span>Đăng xuất</span>
        </button>
      </div>
    `;

    const badge = document.getElementById("btn-user-profile");
    const dropdown = document.getElementById("user-dropdown-board");
    const logoutBtn = document.getElementById("btn-logout");

    badge?.addEventListener("click", e => {
      e.stopPropagation();
      dropdown.hidden = !dropdown.hidden;
    });

    if (logoutBtn) {
      logoutBtn.onclick = handleLogout;
    }
  } else {
    container.innerHTML = `
      <button class="button button-quiet" id="btn-login-open" title="Đăng nhập">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align: -2px; margin-right: 4px;">
          <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"></path>
          <circle cx="12" cy="7" r="4"></circle>
        </svg>
        <span>Đăng nhập</span>
      </button>
      <button class="button button-secondary" id="btn-register-open" title="Đăng ký tài khoản">
        <span>Đăng ký</span>
      </button>
    `;

    document.getElementById("btn-login-open")?.addEventListener("click", openLoginModal);
    document.getElementById("btn-register-open")?.addEventListener("click", openRegisterModal);
  }
}

function resetCanvasToBlank() {
  layers.splice(0, layers.length, {
    id: 1,
    name: "Layer 1",
    visible: true,
    locked: false,
    items: []
  });
  activeLayer = 0;
  selected = null;
  selectedItems = [];
  copiedShape = null;
  undoStack.splice(0, undoStack.length);
  redoStack.splice(0, redoStack.length);
  hasPendingLocalChanges = false;
  applyDocumentTitle("Untitled", false);
  render();
  renderLayers();
  renderSelectionToolbar();
}

async function loadAndOpenUserProject() {
  hasPendingLocalChanges = false;
  await loadProjectsList();
  const targetId = (userProjects && userProjects.length > 0) ? userProjects[0].id : "proj_default";
  await switchProject(targetId, true);
  backendConnected = true;
  const statusEl = document.querySelector("#backend-status");
  if (statusEl) statusEl.textContent = "C++ đã kết nối · tự đồng bộ";
}

async function checkAuthSession() {
  if (!authToken) {
    currentUser = null;
    localStorage.removeItem("mini-vector-cad-user");
    renderUserAuthSection();
    return;
  }
  try {
    const res = await fetch(`${API_BASE_URL}/api/me`, {
      headers: { "Authorization": `Bearer ${authToken}` }
    });
    if (res.ok) {
      const data = await res.json();
      if (data.success && data.user) {
        currentUser = data.user;
        localStorage.setItem("mini-vector-cad-user", JSON.stringify(currentUser));
        renderUserAuthSection();
        await loadAndOpenUserProject();
        return;
      }
    } else {
      authToken = "";
      currentUser = null;
      localStorage.removeItem("mini-vector-cad-auth-token");
      localStorage.removeItem("mini-vector-cad-user");
      resetCanvasToBlank();
    }
  } catch (_) {
    // If backend is offline, keep cached user
    if (currentUser) {
      await loadAndOpenUserProject();
    }
  }
  renderUserAuthSection();
}

async function handleLoginSubmit(event) {
  event.preventDefault();
  const username = document.getElementById("login-username")?.value.trim() || "";
  const password = document.getElementById("login-password")?.value || "";
  const submitBtn = document.getElementById("btn-login-submit");

  if (!username || !password) {
    showAuthMessage("login-msg", "Vui lòng nhập đầy đủ tên đăng nhập và mật khẩu.");
    return;
  }

  hideAuthMessage("login-msg");
  if (submitBtn) submitBtn.disabled = true;

  try {
    const res = await fetch(`${API_BASE_URL}/api/login`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password })
    });
    const data = await res.json();
    if (res.ok && data.success) {
      authToken = data.token;
      currentUser = data.user;
      localStorage.setItem("mini-vector-cad-auth-token", authToken);
      localStorage.setItem("mini-vector-cad-user", JSON.stringify(currentUser));
      closeLoginModal();
      renderUserAuthSection();
      await loadAndOpenUserProject();
      setStatus(`Chào mừng bạn quay lại, ${currentUser.fullName || currentUser.username}!`);
    } else {
      showAuthMessage("login-msg", data.message || "Tên đăng nhập hoặc mật khẩu không chính xác.");
    }
  } catch (err) {
    const localUser = JSON.parse(localStorage.getItem("mini-vector-cad-user") || "null");
    if (localUser && localUser.username === username) {
      authToken = "offline-token-" + Date.now();
      currentUser = localUser;
      localStorage.setItem("mini-vector-cad-auth-token", authToken);
      closeLoginModal();
      renderUserAuthSection();
      await loadAndOpenUserProject();
      setStatus(`Đăng nhập thành công (chế độ dự phòng)! Chào mừng ${currentUser.fullName || currentUser.username}.`);
    } else {
      showAuthMessage("login-msg", "Không thể kết nối đến C++ backend. Vui lòng chạy MiniVectorCADServer.exe để đăng nhập.");
    }
  } finally {
    if (submitBtn) submitBtn.disabled = false;
  }
}

async function handleRegisterSubmit(event) {
  event.preventDefault();
  const fullName = document.getElementById("register-fullname")?.value.trim() || "";
  const username = document.getElementById("register-username")?.value.trim() || "";
  const password = document.getElementById("register-password")?.value || "";
  const confirmPassword = document.getElementById("register-confirm-password")?.value || "";
  const submitBtn = document.getElementById("btn-register-submit");

  if (!username || !password) {
    showAuthMessage("register-msg", "Vui lòng nhập tên đăng nhập và mật khẩu.");
    return;
  }
  if (password.length < 6) {
    showAuthMessage("register-msg", "Mật khẩu phải có ít nhất 6 ký tự.");
    return;
  }
  if (password !== confirmPassword) {
    showAuthMessage("register-msg", "Mật khẩu nhập lại không khớp!");
    return;
  }

  hideAuthMessage("register-msg");
  if (submitBtn) submitBtn.disabled = true;

  try {
    const res = await fetch(`${API_BASE_URL}/api/register`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password, fullName })
    });
    const data = await res.json();
    if (res.ok && data.success) {
      authToken = data.token;
      currentUser = data.user;
      localStorage.setItem("mini-vector-cad-auth-token", authToken);
      localStorage.setItem("mini-vector-cad-user", JSON.stringify(currentUser));
      closeRegisterModal();
      renderUserAuthSection();
      await loadAndOpenUserProject();
      setStatus(`Đăng ký thành công! Chào mừng ${currentUser.fullName || currentUser.username}.`);
    } else {
      showAuthMessage("register-msg", data.message || "Đăng ký thất bại. Vui lòng thử lại.");
    }
  } catch (err) {
    // Chế độ dự phòng tự động nếu backend C++ chưa khởi động
    const fallbackUser = {
      username,
      fullName: fullName || username,
      createdAt: new Date().toLocaleString()
    };
    authToken = "offline-token-" + Date.now();
    currentUser = fallbackUser;
    localStorage.setItem("mini-vector-cad-auth-token", authToken);
    localStorage.setItem("mini-vector-cad-user", JSON.stringify(currentUser));
    closeRegisterModal();
    renderUserAuthSection();
    await loadAndOpenUserProject();
    setStatus(`Đăng ký tài khoản thành công! Chào mừng ${currentUser.fullName}.`);
  } finally {
    if (submitBtn) submitBtn.disabled = false;
  }
}

async function handleLogout(e) {
  if (e) {
    if (typeof e.preventDefault === "function") e.preventDefault();
    if (typeof e.stopPropagation === "function") e.stopPropagation();
  }

  // 1. Ẩn dropdown ngay lập tức
  const dropdown = document.getElementById("user-dropdown-board");
  if (dropdown) dropdown.hidden = true;

  // 2. Lưu token cũ để gọi api ngầm
  const oldToken = authToken;

  // 3. Xóa sạch phiên đăng nhập ngay lập tức
  authToken = "";
  currentUser = null;
  localStorage.removeItem("mini-vector-cad-auth-token");
  localStorage.removeItem("mini-vector-cad-user");
  localStorage.removeItem("mini-vector-cad-current-project-id");
  localStorage.removeItem("mini-vector-cad-guest-drawing");
  currentProjectId = "guest_local";

  // 4. Reset bản vẽ lại hoàn toàn trắng tinh
  try {
    resetCanvasToBlank();
  } catch (err) {
    console.warn("resetCanvasToBlank error:", err);
  }

  // 5. Cập nhật giao diện về Khách ngay lập tức
  backendConnected = false;
  userProjects = [{
    id: "guest_local",
    name: "Bản vẽ riêng (Khách)",
    owner: "Khách",
    isOwner: true,
    members: [],
    isGuest: true
  }];
  renderUserAuthSection();
  updateProjectSelector();

  const statusEl = document.querySelector("#backend-status");
  if (statusEl) statusEl.textContent = "Chế độ Khách · Bản vẽ mới";
  setStatus("Đã đăng xuất thành công. Bản vẽ đã được reset lại.");

  // 6. Gửi API logout lên backend trong background (không chặn giao diện)
  if (oldToken) {
    fetch(`${API_BASE_URL}/api/logout`, {
      method: "POST",
      headers: { "Authorization": `Bearer ${oldToken}` }
    }).catch(() => {});
  }
}

// Global modal events
document.getElementById("login-modal-close")?.addEventListener("click", closeLoginModal);
document.getElementById("register-modal-close")?.addEventListener("click", closeRegisterModal);
document.getElementById("link-to-register")?.addEventListener("click", e => { e.preventDefault(); openRegisterModal(); });
document.getElementById("link-to-login")?.addEventListener("click", e => { e.preventDefault(); openLoginModal(); });
document.getElementById("form-login")?.addEventListener("submit", handleLoginSubmit);
document.getElementById("form-register")?.addEventListener("submit", handleRegisterSubmit);

document.addEventListener("click", event => {
  const dropdown = document.getElementById("user-dropdown-board");
  const profileBadge = document.getElementById("btn-user-profile");
  if (dropdown && !dropdown.hidden && !profileBadge?.contains(event.target) && !dropdown.contains(event.target)) {
    dropdown.hidden = true;
  }
  const projMenu = document.getElementById("project-dropdown-menu");
  const projBtn = document.getElementById("project-picker-btn");
  if (projMenu && !projMenu.hidden && !projBtn?.contains(event.target) && !projMenu.contains(event.target)) {
    projMenu.hidden = true;
  }
});

document.addEventListener("keydown", event => {
  if (event.key === "Escape") {
    closeLoginModal();
    closeRegisterModal();
    const dropdown = document.getElementById("user-dropdown-board");
    if (dropdown) dropdown.hidden = true;
    const projMenu = document.getElementById("project-dropdown-menu");
    if (projMenu) projMenu.hidden = true;
  }
});

// Initialize Auth
checkAuthSession();

// ============================================================================
// MULTI-USER COLLABORATION & PROJECT MANAGEMENT
// ============================================================================
let userProjects = [];
let collabPollTimer = null;

function getNextProjectNumber() {
  let maxNum = 0;
  userProjects.forEach(p => {
    const match = p.name.match(/Dự án\s*(\d+)/i);
    if (match) {
      const n = parseInt(match[1], 10);
      if (n > maxNum) maxNum = n;
    }
  });
  if (maxNum === 0) {
    return Math.max(1, userProjects.length + 1);
  }
  return maxNum + 1;
}

async function openInviteModal() {
  if (!currentUser || !authToken) {
    alert("🔒 Bạn đang ở Chế độ Khách (Bản vẽ riêng cục bộ trên máy này).\n\nĐể mời bạn bè vẽ chung hoặc lưu trữ lên Cloud, vui lòng Đăng ký hoặc Đăng nhập!");
    openLoginModal();
    return;
  }

  await loadProjectsList();
  const currentProj = userProjects.find(p => p.id === currentProjectId) || userProjects[0];
  if (!currentProj) return;

  const nameEl = document.getElementById("invite-target-project-name");
  if (nameEl) nameEl.textContent = currentProj.name;

  hideAuthMessage("invite-msg");
  renderMembersList(currentProj);

  const modal = document.getElementById("modal-invite");
  if (modal) {
    modal.hidden = false;
    setTimeout(() => document.getElementById("invite-username")?.focus(), 50);
  }
}

function closeInviteModal() {
  const modal = document.getElementById("modal-invite");
  if (modal) modal.hidden = true;
}

function openCreateProjectModal() {
  if (!currentUser || !authToken) {
    alert("🔒 Bạn cần Đăng ký hoặc Đăng nhập tài khoản để tạo và lưu dự án trên Cloud!");
    openLoginModal();
    return;
  }
  hideAuthMessage("create-project-msg");
  const modal = document.getElementById("modal-create-project");
  if (modal) {
    modal.hidden = false;
    const input = document.getElementById("create-project-name");
    if (input) {
      input.value = `Dự án ${getNextProjectNumber()}`;
      setTimeout(() => {
        input.focus();
        input.select();
      }, 50);
    }
  }
}

function closeCreateProjectModal() {
  const modal = document.getElementById("modal-create-project");
  if (modal) modal.hidden = true;
}

const SVG_ICON_OWNER = `<svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 4l3 12h14l3-12-6 7-4-7-4 7-6-7z"/><circle cx="5" cy="4" r="1.5" fill="currentColor"/><circle cx="12" cy="4" r="1.5" fill="currentColor"/><circle cx="19" cy="4" r="1.5" fill="currentColor"/><line x1="5" y1="20" x2="19" y2="20" stroke-width="2.2"/></svg>`;

const SVG_ICON_COLLAB = `<svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"/><circle cx="11" cy="7" r="4"/><path d="M22 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/></svg>`;

const SVG_ICON_PRIVATE = `<svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>`;

function renderMembersList(project) {
  const container = document.getElementById("project-members-list");
  if (!container) return;

  const rawOwner = project.owner || (currentUser ? currentUser.username : "Bạn");
  const displayOwner = rawOwner;
  const isCurrentOwner = currentUser && (currentUser.username === rawOwner);
  const hasCollab = project.members && project.members.length > 0;

  let html = `
    <div style="font-size: 12px; margin-bottom: 8px; padding: 6px 10px; border-radius: 6px; background: ${hasCollab ? "rgba(16, 185, 129, 0.12)" : "rgba(245, 158, 11, 0.12)"}; border: 1px solid ${hasCollab ? "rgba(16, 185, 129, 0.3)" : "rgba(245, 158, 11, 0.3)"}; color: ${hasCollab ? "#10b981" : "#f59e0b"};">
      ${hasCollab
        ? `<b>👥 Chế độ: Đang vẽ chung</b> (${project.members.length + 1} người đang có quyền vẽ)`
        : `<b>🔒 Chế độ: Bản vẽ riêng</b> (Chỉ một mình bạn có quyền xem & vẽ. Hãy mời bạn bè bên dưới để chuyển sang Vẽ chung!)`
      }
    </div>
    <div class="member-item">
      <div class="member-info">
        <span class="proj-badge-icon owner" title="Chủ dự án">${SVG_ICON_OWNER}</span>
        <b class="member-name">@${displayOwner}</b>
      </div>
      <span class="member-role-badge owner">Chủ dự án</span>
    </div>
  `;

  if (hasCollab) {
    project.members.forEach(member => {
      html += `
        <div class="member-item">
          <div class="member-info">
            <span class="proj-badge-icon collab" title="Thành viên vẽ chung">${SVG_ICON_COLLAB}</span>
            <span class="member-name">@${member}</span>
          </div>
          <div style="display: flex; align-items: center; gap: 8px;">
            <span class="member-role-badge member">Thành viên vẽ chung</span>
            ${isCurrentOwner ? `<button type="button" class="member-remove-btn" data-kick="${member}" title="Xóa thành viên">✕ Xóa</button>` : ""}
          </div>
        </div>
      `;
    });
  } else {
    html += `
      <div style="font-size: 11px; color: var(--muted); padding: 8px 0; font-style: italic;">
        Chưa có thành viên nào khác. Bạn đang ở chế độ Bản vẽ riêng. Hãy nhập username bạn bè ở trên và bấm "Gửi lời mời" để vẽ chung!
      </div>
    `;
  }

  container.innerHTML = html;

  container.querySelectorAll(".member-remove-btn").forEach(btn => {
    btn.addEventListener("click", () => {
      const targetUser = btn.dataset.kick;
      if (confirm(`Bạn có chắc muốn xóa thành viên "${targetUser}" khỏi dự án này?`)) {
        handleRemoveMember(targetUser);
      }
    });
  });
}

async function loadProjectsList() {
  if (!currentUser || !authToken) {
    userProjects = [{
      id: "guest_local",
      name: "Bản vẽ riêng (Khách)",
      owner: "Khách",
      isOwner: true,
      members: [],
      isGuest: true
    }];
    currentProjectId = "guest_local";
    updateProjectSelector();
    return;
  }

  try {
    const res = await fetch(`${API_BASE_URL}/api/projects`, {
      headers: getAuthHeaders()
    });
    if (!res.ok) return;
    const data = await res.json();
    if (data.success && Array.isArray(data.projects)) {
      userProjects = data.projects;
      if (userProjects.length === 0) {
        userProjects = [{
          id: "proj_default",
          name: "Bản vẽ cá nhân 1",
          owner: currentUser.username,
          isOwner: true,
          members: []
        }];
      }
      if (currentProjectId === "guest_local" || !userProjects.some(p => p.id === currentProjectId)) {
        currentProjectId = userProjects[0].id;
        localStorage.setItem("mini-vector-cad-current-project-id", currentProjectId);
      }
      updateProjectSelector();
    }
  } catch (_) {}
}

function updateProjectSelector() {
  const select = document.getElementById("project-select");
  const currentIconEl = document.getElementById("project-current-icon");
  const currentNameEl = document.getElementById("project-current-name");
  const dropdownList = document.getElementById("project-dropdown-list");
  const dropdownCount = document.getElementById("project-dropdown-count");

  if (!userProjects.length) {
    if (currentUser) {
      userProjects = [{ id: "proj_default", name: "Bản vẽ cá nhân 1", owner: currentUser.username, isOwner: true, members: [] }];
    } else {
      userProjects = [{ id: "guest_local", name: "Bản vẽ riêng (Khách)", owner: "Khách", isOwner: true, members: [], isGuest: true }];
    }
  }

  if (!userProjects.some(p => p.id === currentProjectId)) {
    currentProjectId = userProjects[0].id;
    localStorage.setItem("mini-vector-cad-current-project-id", currentProjectId);
  }

  const currentProj = userProjects.find(p => p.id === currentProjectId) || userProjects[0];
  const isGuestMode = !currentUser || Boolean(currentProj.isGuest);
  const isCollabMode = !isGuestMode && Array.isArray(currentProj.members) && currentProj.members.length > 0;

  // Update hidden select for backward-compatibility
  if (select) {
    select.innerHTML = "";
    userProjects.forEach(p => {
      const opt = document.createElement("option");
      opt.value = p.id;
      let rolePrefix = "[Bản vẽ riêng]";
      if (p.isGuest) {
        rolePrefix = "[Khách - Riêng]";
      } else if (p.members && p.members.length > 0) {
        rolePrefix = p.isOwner ? "[Chủ • Vẽ chung]" : "[Thành viên • Vẽ chung]";
      }
      opt.textContent = `${rolePrefix} ${p.name}`;
      select.appendChild(opt);
    });
    select.value = currentProj.id;
  }

  // Update topbar button
  if (currentIconEl) {
    if (isGuestMode) {
      currentIconEl.className = "project-current-icon private guest";
      currentIconEl.innerHTML = SVG_ICON_PRIVATE;
      currentIconEl.title = "Bản vẽ riêng (Khách - Cục bộ trên máy)";
    } else if (isCollabMode) {
      currentIconEl.className = "project-current-icon collab";
      currentIconEl.innerHTML = SVG_ICON_COLLAB;
      currentIconEl.title = "Đang vẽ chung thời gian thực";
    } else {
      currentIconEl.className = "project-current-icon private";
      currentIconEl.innerHTML = SVG_ICON_PRIVATE;
      currentIconEl.title = "Bản vẽ riêng cá nhân (Chỉ mình bạn)";
    }
  }
  if (currentNameEl) {
    currentNameEl.textContent = currentProj.name;
    currentNameEl.title = currentProj.name;
  }

  // Update dropdown list items
  if (dropdownList) {
    dropdownList.innerHTML = "";
    userProjects.forEach(p => {
      const isGuest = Boolean(p.isGuest);
      const isOwner = p.isOwner;
      const hasCollab = Array.isArray(p.members) && p.members.length > 0;

      let iconHtml = SVG_ICON_PRIVATE;
      let badgeClass = "private";
      let roleDesc = "";

      if (isGuest) {
        iconHtml = SVG_ICON_PRIVATE;
        badgeClass = "private guest";
        roleDesc = "🔒 Bản vẽ riêng (Cục bộ trên máy này)";
      } else if (hasCollab) {
        iconHtml = SVG_ICON_COLLAB;
        badgeClass = "collab";
        const memberCount = (p.members ? p.members.length : 0) + 1;
        roleDesc = isOwner
          ? `👥 Đang vẽ chung (${memberCount} người)`
          : `👥 Được mời bởi @${p.owner}`;
      } else {
        iconHtml = SVG_ICON_PRIVATE;
        badgeClass = "private";
        roleDesc = `🔒 Bản vẽ riêng Cloud (Chỉ mình bạn)`;
      }

      const itemBtn = document.createElement("button");
      itemBtn.type = "button";
      itemBtn.className = `project-dropdown-item ${p.id === currentProjectId ? "active" : ""}`;
      itemBtn.innerHTML = `
        <div class="project-dropdown-item-left">
          <span class="proj-badge-icon ${badgeClass}">
            ${iconHtml}
          </span>
          <div class="project-dropdown-item-info">
            <div class="project-dropdown-item-name">${p.name}</div>
            <div class="project-dropdown-item-role">${roleDesc}</div>
          </div>
        </div>
        ${p.id === currentProjectId ? `
          <svg class="project-dropdown-check" width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
            <polyline points="20 6 9 17 4 12"></polyline>
          </svg>
        ` : ""}
      `;
      itemBtn.addEventListener("click", () => {
        const menu = document.getElementById("project-dropdown-menu");
        if (menu) menu.hidden = true;
        switchProject(p.id);
      });
      dropdownList.appendChild(itemBtn);
    });
  }

  if (dropdownCount) {
    dropdownCount.textContent = userProjects.length;
  }

  // Update member count badge
  const badge = document.getElementById("member-count-badge");
  if (badge) {
    if (isCollabMode) {
      const count = currentProj.members.length;
      badge.textContent = `+${count}`;
      badge.hidden = false;
      badge.title = `Dự án vẽ chung với ${count} người khác`;
    } else {
      badge.hidden = true;
    }
  }
}

async function switchProject(projectId, forceReload = false) {
  if (currentProjectId === projectId && !forceReload) return;
  currentProjectId = projectId;
  localStorage.setItem("mini-vector-cad-current-project-id", projectId);

  const proj = userProjects.find(p => p.id === projectId);
  if (proj) {
    applyDocumentTitle(proj.name, false);
  }

  updateProjectSelector();

  if (!currentUser || !authToken || (proj && proj.isGuest)) {
    resetCanvasToBlank();
    setStatus("🔒 Đang ở Chế độ Khách (Bản vẽ mới).");
    return;
  }

  setStatus(`Đang tải dự án: ${proj ? proj.name : projectId}...`);

  try {
    const res = await fetch(getProjectApiUrl(projectId), {
      headers: getAuthHeaders()
    });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const text = await res.text();
    const importedLayers = parseNativeProject(text);
    layers.splice(0, layers.length, ...importedLayers);
    activeLayer = 0;
    selected = null;
    selectedItems = [];
    hasPendingLocalChanges = false;
    render();
    renderLayers();
    renderSelectionToolbar();
    const isCollab = proj && Array.isArray(proj.members) && proj.members.length > 0;
    setStatus(isCollab ? `👥 Đã mở dự án vẽ chung "${proj.name}"!` : `🔒 Đã mở bản vẽ riêng "${proj ? proj.name : projectId}"!`);
  } catch (err) {
    setStatus(`Lỗi khi mở dự án: ${err.message}`);
  }
}

async function handleInviteSubmit(event) {
  event.preventDefault();
  const input = document.getElementById("invite-username");
  const targetUsername = input?.value.trim() || "";
  const submitBtn = document.getElementById("btn-invite-submit");

  if (!targetUsername) {
    showAuthMessage("invite-msg", "Vui lòng nhập tên đăng nhập muốn mời.");
    return;
  }

  hideAuthMessage("invite-msg");
  if (submitBtn) submitBtn.disabled = true;

  try {
    const res = await fetch(`${API_BASE_URL}/api/projects/invite`, {
      method: "POST",
      headers: getAuthHeaders({ "Content-Type": "application/json" }),
      body: JSON.stringify({ projectId: currentProjectId, username: targetUsername })
    });
    const data = await res.json();
    if (res.ok && data.success) {
      showAuthMessage("invite-msg", data.message || `Đã mời "${targetUsername}" thành công!`, true);
      if (input) input.value = "";
      await loadProjectsList();
      const proj = userProjects.find(p => p.id === currentProjectId);
      if (proj) renderMembersList(proj);
      setStatus(`Đã mời @${targetUsername} vào cùng vẽ!`);
    } else {
      showAuthMessage("invite-msg", data.message || "Không thể mời thành viên này.");
    }
  } catch (err) {
    showAuthMessage("invite-msg", "Lỗi kết nối C++ backend: " + err.message);
  } finally {
    if (submitBtn) submitBtn.disabled = false;
  }
}

async function handleRemoveMember(targetUsername) {
  try {
    const res = await fetch(`${API_BASE_URL}/api/projects/remove_member`, {
      method: "POST",
      headers: getAuthHeaders({ "Content-Type": "application/json" }),
      body: JSON.stringify({ projectId: currentProjectId, username: targetUsername })
    });
    const data = await res.json();
    if (res.ok && data.success) {
      await loadProjectsList();
      const proj = userProjects.find(p => p.id === currentProjectId);
      if (proj) renderMembersList(proj);
      setStatus(`Đã xóa @${targetUsername} khỏi dự án.`);
    } else {
      alert(data.message || "Không thể xóa thành viên.");
    }
  } catch (err) {
    alert("Lỗi kết nối C++: " + err.message);
  }
}

async function handleCreateProjectSubmit(event) {
  event.preventDefault();
  const input = document.getElementById("create-project-name");
  let name = input?.value.trim() || "";
  if (!name) {
    name = `Dự án ${getNextProjectNumber()}`;
  }
  const submitBtn = document.getElementById("btn-create-project-submit");

  hideAuthMessage("create-project-msg");
  if (submitBtn) submitBtn.disabled = true;

  try {
    const res = await fetch(`${API_BASE_URL}/api/projects`, {
      method: "POST",
      headers: getAuthHeaders({ "Content-Type": "application/json" }),
      body: JSON.stringify({ name })
    });
    const data = await res.json();
    if (res.ok && data.success && data.project) {
      closeCreateProjectModal();
      await loadProjectsList();
      await switchProject(data.project.id);
      setStatus(`Đã tạo dự án "${name}" thành công! Bây giờ bạn có thể bấm "Mời vẽ chung" để mời bạn bè.`);
    } else {
      showAuthMessage("create-project-msg", data.message || "Tạo dự án thất bại.");
    }
  } catch (err) {
    showAuthMessage("create-project-msg", "Lỗi kết nối C++ backend: " + err.message);
  } finally {
    if (submitBtn) submitBtn.disabled = false;
  }
}

// Real-time collaborative polling (tự động cập nhật nét vẽ của đồng đội)
function startCollaborationPolling() {
  if (collabPollTimer) clearInterval(collabPollTimer);
  let pollCounter = 0;
  collabPollTimer = setInterval(async () => {
    // 1. Khách chưa đăng nhập: Không vẽ chung, không poll server
    if (!currentUser || !authToken) return;

    // 2. Định kỳ mỗi 3 lần poll (~4.5s) tự làm mới danh sách dự án (để cập nhật khi được ai đó mời)
    pollCounter++;
    if (pollCounter % 3 === 0 && !pointerAction) {
      loadProjectsList();
    }

    // 3. Kiểm tra dự án hiện tại: CHỈ VẼ CHUNG KHI CÓ THÀNH VIÊN ĐƯỢC MỜI (members.length > 0)
    // Nếu không phải vẽ chung (bản vẽ riêng cá nhân): KHÔNG poll canvas từ server tránh đè nét!
    const currentProj = userProjects.find(p => p.id === currentProjectId);
    const isCollabMode = currentProj && Array.isArray(currentProj.members) && currentProj.members.length > 0;
    if (!isCollabMode) {
      return;
    }

    // Chỉ đồng bộ khi không đang vẽ kéo chuột và không có thay đổi nội bộ chưa lưu
    if (pointerAction || hasPendingLocalChanges || !backendConnected) return;
    try {
      const res = await fetch(getProjectApiUrl(), { headers: getAuthHeaders() });
      if (!res.ok) return;
      const remoteText = await res.text();
      const localText = serializeNativeProject();
      if (remoteText.trim() !== localText.trim()) {
        const importedLayers = parseNativeProject(remoteText);
        layers.splice(0, layers.length, ...importedLayers);
        if (activeLayer >= layers.length) {
          activeLayer = Math.max(0, layers.length - 1);
        }
        render();
      }
    } catch (_) {}
  }, 1500);
}

// Bind collaboration events
document.getElementById("btn-invite-open")?.addEventListener("click", openInviteModal);
document.getElementById("invite-modal-close")?.addEventListener("click", closeInviteModal);
document.getElementById("form-invite")?.addEventListener("submit", handleInviteSubmit);

document.getElementById("btn-create-project-open")?.addEventListener("click", openCreateProjectModal);
document.getElementById("create-project-modal-close")?.addEventListener("click", closeCreateProjectModal);
document.getElementById("form-create-project")?.addEventListener("submit", handleCreateProjectSubmit);

document.getElementById("project-select")?.addEventListener("change", e => {
  switchProject(e.target.value);
});

document.getElementById("project-picker-btn")?.addEventListener("click", async e => {
  e.stopPropagation();
  const menu = document.getElementById("project-dropdown-menu");
  if (menu) {
    const isOpening = menu.hidden;
    menu.hidden = !menu.hidden;
    if (isOpening) {
      await loadProjectsList();
    }
  }
});

document.getElementById("btn-dropdown-create-project")?.addEventListener("click", () => {
  const menu = document.getElementById("project-dropdown-menu");
  if (menu) menu.hidden = true;
  openCreateProjectModal();
});

// Hook into auth login to reload projects
const oldRenderUserAuthSection = renderUserAuthSection;
renderUserAuthSection = function() {
  oldRenderUserAuthSection();
  loadProjectsList();
};

// Initialize projects & collaborative real-time polling
loadProjectsList();
startCollaborationPolling();





