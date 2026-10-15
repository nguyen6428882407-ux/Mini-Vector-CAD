const svgNS = "http://www.w3.org/2000/svg";
const artwork = document.querySelector("#artwork");
const drawing = document.querySelector("#drawing");
const layers = [
  { name: "Layer 1", visible: true, shapes: [] }
];

let activeLayer = 0;
let activeTool = "select";
let selected = null;
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
let backendConnected = false;
let backendSyncTimer = 0;
let backendSyncInProgress = false;
let backendSyncPending = false;
let backendReconnectTimer = 0;
let hasPendingLocalChanges = false;
let projectRevision = 0;
const undoStack = [];
const redoStack = [];

const toolLabels = {
  select: "Chọn",
  rect: "Hình chữ nhật",
  circle: "Hình tròn",
  line: "Đoạn thẳng",
  triangle: "Tam giác",
  brush: "Nét bút"
};

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
    fill: shape.type === "line" ? "none" : shape.fill,
    transform: `rotate(${shape.rotation || 0} ${centerX} ${centerY})`
  };
  if (shape.type === "rect") {
    element = makeSvg("rect", { ...common, x: shape.x, y: shape.y, width: shape.w, height: shape.h, rx: 2 });
  } else if (shape.type === "circle") {
    element = makeSvg("circle", { ...common, cx: shape.x, cy: shape.y, r: shape.r });
  } else if (shape.type === "line") {
    element = makeSvg("line", { ...common, x1: shape.x1, y1: shape.y1, x2: shape.x2, y2: shape.y2 });
  } else if (shape.type === "brush") {
    element = makeSvg("path", {
      ...common,
      d: pathData(shape.points),
      fill: "none",
      opacity: shape.opacity
    });
  } else {
    element = makeSvg("polygon", { ...common, points: shape.points.map(point => point.join(",")).join(" ") });
  }
  if (selected && selected.layer === layerIndex && selected.shape === shapeIndex) element.classList.add("selected");
  element.addEventListener("pointerdown", event => {
    if (event.button !== 0) return;
    event.stopPropagation();
    event.preventDefault();
    const alreadySelected = selected?.layer === layerIndex && selected?.shape === shapeIndex;
    selected = { layer: layerIndex, shape: shapeIndex };
    activeLayer = layerIndex;
    if (!alreadySelected) {
      render();
      setStatus("Đã chọn hình — kéo hình để di chuyển");
      return;
    }
    if (layers[layerIndex].locked) {
      setStatus("Layer đang bị khóa, không thể di chuyển hình.");
      return;
    }
    pointerAction = {
      mode: "move",
      layer: layerIndex,
      shape: shapeIndex,
      start: pointFromEvent(event),
      original: structuredClone(shape),
      moved: false
    };
    drawing.setPointerCapture(event.pointerId);
    render();
    setStatus("Đang kéo hình — thả chuột để đặt vào vị trí mới");
  });
  return element;
}

function shapeBounds(shape) {
  if (shape.type === "rect") return { x: shape.x, y: shape.y, w: shape.w, h: shape.h };
  if (shape.type === "circle") return { x: shape.x - shape.r, y: shape.y - shape.r, w: shape.r * 2, h: shape.r * 2 };
  const points = shape.type === "line" ? [[shape.x1, shape.y1], [shape.x2, shape.y2]] : shape.points;
  const xs = points.map(point => point[0]);
  const ys = points.map(point => point[1]);
  const minX = Math.min(...xs);
  const minY = Math.min(...ys);
  return { x: minX, y: minY, w: Math.max(...xs) - minX, h: Math.max(...ys) - minY };
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
  if (shape.type === "line") {
    [["start", shape.x1, shape.y1], ["end", shape.x2, shape.y2]].forEach(([handle, x, y]) => {
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
  if (selected && layers[selected.layer]?.visible) {
    const selectedShape = layers[selected.layer].shapes[selected.shape];
    if (selectedShape) selectionHandles(selectedShape);
  }
  renderPreview();
  renderLayers();
  renderProperties();
  renderSelectionToolbar();
  document.querySelector("#canvas-hint").textContent = activeTool === "select"
    ? selected ? "Kéo hình để di chuyển · nút góc để đổi kích thước · nút trên để xoay" : "Chọn công cụ hình để bắt đầu vẽ"
    : activeTool === "eraser" ? "Kéo trên nét vẽ hoặc hình để gôm"
      : activeTool === "brush" ? "Giữ chuột trái và kéo để vẽ nét tự do"
        : `Kéo trên canvas để vẽ ${toolLabels[activeTool].toLowerCase()}`;
  drawing.classList.toggle("draw-mode", activeTool !== "select");
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
  toolbar.hidden = !shape;
  document.querySelector("#paste-shape").disabled = !copiedShape;
  if (!shape) return;

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

  const stage = document.querySelector("#canvas-stage").getBoundingClientRect();
  const leftEdge = Math.min(...corners.map(point => point.x)) - stage.left;
  const rightEdge = Math.max(...corners.map(point => point.x)) - stage.left;
  const topEdge = Math.min(...corners.map(point => point.y)) - stage.top;
  const bottomEdge = Math.max(...corners.map(point => point.y)) - stage.top;
  const toolbarWidth = toolbar.offsetWidth;
  const toolbarHeight = toolbar.offsetHeight;
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
    preview = makeSvg("path", { ...common, d: pathData(shape.points), fill: "none", opacity: shape.opacity });
    preview.removeAttribute("stroke-dasharray");
  } else if (shape.type === "rect") {
    preview = makeSvg("rect", { ...common, x: shape.x, y: shape.y, width: shape.w, height: shape.h });
  } else if (shape.type === "circle") {
    preview = makeSvg("circle", { ...common, cx: shape.x, cy: shape.y, r: shape.r });
  } else if (shape.type === "line") {
    preview = makeSvg("line", { ...common, x1: shape.x1, y1: shape.y1, x2: shape.x2, y2: shape.y2 });
  } else {
    preview = makeSvg("polygon", { ...common, points: shape.points.map(point => point.join(",")).join(" ") });
  }
  preview.classList.add("drawing-preview");
  artwork.append(preview);
}

function renderLayers() {
  const list = document.querySelector("#layer-list");
  list.replaceChildren();
  [...layers].reverse().forEach((layer, reverseIndex) => {
    const index = layers.length - 1 - reverseIndex;
    const row = document.createElement("div");
    row.className = `layer-row${activeLayer === index ? " active" : ""}`;
    row.innerHTML = `<span class="layer-icon">▱</span><span class="layer-name"></span><span class="layer-count">${layer.shapes.length}</span><button class="layer-toggle${layer.visible ? "" : " off"}" title="${layer.visible ? "Ẩn layer" : "Hiện layer"}" aria-label="${layer.visible ? "Ẩn layer" : "Hiện layer"}">${layer.visible ? "◉" : "○"}</button><button class="layer-delete" title="Xóa layer" aria-label="Xóa layer" ${layers.length === 1 ? "disabled" : ""}>×</button>`;
    row.querySelector(".layer-name").textContent = layer.name;
    row.addEventListener("click", event => {
      if (event.target.closest(".layer-toggle")) {
        snapshot();
        layer.visible = !layer.visible;
        render();
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
      activeLayer = index;
      selected = null;
      render();
      setStatus(`Layer hiện tại: ${layer.name}`);
    });
    list.append(row);
  });
}

function renderProperties() {
  const controls = document.querySelector("#property-controls");
  const empty = document.querySelector("#property-empty");
  const brushControls = document.querySelector("#brush-controls");
  const eraserControls = document.querySelector("#eraser-controls");
  const title = document.querySelector("#selection-title");
  const shape = selected ? layers[selected.layer]?.shapes[selected.shape] : null;
  const usingBrush = activeTool === "brush";
  const usingEraser = activeTool === "eraser";
  controls.hidden = !shape || usingBrush || usingEraser;
  brushControls.hidden = !usingBrush;
  eraserControls.hidden = !usingEraser;
  empty.hidden = Boolean(shape) || usingBrush || usingEraser;
  title.textContent = usingBrush ? "Bút vẽ" : usingEraser ? "Gôm" : shape ? toolLabels[shape.type] || "Hình" : "Chưa chọn hình";
  if (usingBrush) {
    document.querySelector("#brush-color").value = brushColor;
    document.querySelector("#brush-width").value = brushWidth;
    document.querySelector("#brush-width-value").value = brushWidth;
    document.querySelector("#brush-opacity").value = Math.round(brushOpacity * 100);
    document.querySelector("#brush-opacity-value").value = `${Math.round(brushOpacity * 100)}%`;
  }
  if (usingEraser) {
    document.querySelector("#eraser-width").value = eraserWidth;
    document.querySelector("#eraser-width-value").value = eraserWidth;
  }
  if (!shape || usingBrush || usingEraser) return;
  document.querySelector("#stroke-color").value = shape.stroke;
  document.querySelector("#fill-color").value = shape.fill;
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
  return { x: Math.max(0, Math.min(800, local.x)), y: Math.max(0, Math.min(600, local.y)) };
}

function pathData(points) {
  if (points.length === 1) return `M ${points[0][0]} ${points[0][1]} L ${points[0][0]} ${points[0][1]}`;
  return points.map(([x, y], index) => `${index ? "L" : "M"} ${x} ${y}`).join(" ");
}

function createShape(type, start, end) {
  const stroke = document.querySelector("#stroke-color").value;
  const fill = document.querySelector("#fill-color").value;
  const width = Number(document.querySelector("#stroke-width").value);
  const x = Math.min(start.x, end.x);
  const y = Math.min(start.y, end.y);
  const w = Math.abs(end.x - start.x);
  const h = Math.abs(end.y - start.y);
  if (type === "rect" && w > 2 && h > 2) return { type, x, y, w, h, fill, stroke, width };
  if (type === "circle") {
    const r = Math.hypot(end.x - start.x, end.y - start.y);
    if (r > 2) return { type, x: start.x, y: start.y, r, fill, stroke, width };
  }
  if (type === "line" && Math.hypot(w, h) > 2) return { type, x1: start.x, y1: start.y, x2: end.x, y2: end.y, fill, stroke, width };
  if (type === "triangle" && w > 2 && h > 2) {
    return { type, points: [[start.x, end.y], [(start.x + end.x) / 2, start.y], [end.x, end.y]], fill, stroke, width };
  }
  return null;
}

function translateShape(shape, dx, dy) {
  if (shape.type === "rect" || shape.type === "circle") {
    shape.x += dx;
    shape.y += dy;
  } else if (shape.type === "line") {
    shape.x1 += dx;
    shape.y1 += dy;
    shape.x2 += dx;
    shape.y2 += dy;
  } else {
    shape.points = shape.points.map(([x, y]) => [x + dx, y + dy]);
  }
}

function resizeShape(original, handle, point) {
  const shape = structuredClone(original);
  if (shape.type === "line") {
    if (handle === "start") {
      shape.x1 = point.x;
      shape.y1 = point.y;
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
  if (shape.type === "rect") {
    shape.x = next.x;
    shape.y = next.y;
    shape.w = next.w;
    shape.h = next.h;
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
  const threshold = radius + shape.width / 2;
  if (shape.type === "circle") return Math.hypot(point.x - shape.x, point.y - shape.y) <= shape.r + threshold;
  if (shape.type === "line") {
    return distanceToSegment(point, { x: shape.x1, y: shape.y1 }, { x: shape.x2, y: shape.y2 }) <= threshold;
  }
  const bounds = shapeBounds(shape);
  if (shape.type === "rect") {
    return point.x >= bounds.x - threshold && point.x <= bounds.x + bounds.w + threshold
      && point.y >= bounds.y - threshold && point.y <= bounds.y + bounds.h + threshold;
  }
  if (shape.type === "triangle" || shape.type === "polygon") {
    const vertices = shape.points.map(([x, y]) => ({ x, y }));
    if (vertices.some((vertex, index) => distanceToSegment(point, vertex, vertices[(index + 1) % vertices.length]) <= threshold)) return true;
    let inside = false;
    for (let i = 0, j = vertices.length - 1; i < vertices.length; j = i++) {
      const a = vertices[i];
      const b = vertices[j];
      if ((a.y > point.y) !== (b.y > point.y)
        && point.x < ((b.x - a.x) * (point.y - a.y)) / (b.y - a.y) + a.x) inside = !inside;
    }
    return inside;
  }
  return false;
}

function eraseAtPoint(point, radius) {
  const shapes = layers[activeLayer].shapes;
  let changed = false;
  const remaining = [];
  for (const shape of shapes) {
    if (shape.type !== "brush") {
      if (pointHitsShape(point, shape, radius)) changed = true;
      else remaining.push(shape);
      continue;
    }
    const keptPaths = [];
    const erasedPoints = shape.points.map(([x, y]) => Math.hypot(point.x - x, point.y - y) <= radius + shape.width / 2);
    for (let index = 0; index < shape.points.length - 1; index++) {
      const [startX, startY] = shape.points[index];
      const [endX, endY] = shape.points[index + 1];
      if (distanceToSegment(point, { x: startX, y: startY }, { x: endX, y: endY }) <= radius + shape.width / 2) {
        erasedPoints[index] = true;
        erasedPoints[index + 1] = true;
      }
    }
    let currentPath = [];
    let shapeChanged = false;
    shape.points.forEach(([x, y], pointIndex) => {
      if (erasedPoints[pointIndex]) {
        changed = true;
        shapeChanged = true;
        if (currentPath.length) keptPaths.push(currentPath);
        currentPath = [];
      } else {
        currentPath.push([x, y]);
      }
    });
    if (currentPath.length) keptPaths.push(currentPath);
    if (!shapeChanged) {
      remaining.push(shape);
    } else {
      keptPaths.forEach(points => remaining.push({ ...shape, points }));
    }
  }
  if (changed) {
    shapes.splice(0, shapes.length, ...remaining);
    selected = null;
  }
  return changed;
}

function eraseAlong(start, end, radius) {
  const distance = Math.hypot(end.x - start.x, end.y - start.y);
  const steps = Math.max(1, Math.ceil(distance / Math.max(1, radius * 0.45)));
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

drawing.addEventListener("pointerdown", event => {
  if (event.button !== 0) return;
  event.preventDefault();
  if (activeTool === "select") {
    if (event.target === drawing || event.target.classList.contains("grid-bg")) {
      selected = null;
      renderProperties();
      render();
    }
    return;
  }
  if (!layers[activeLayer].visible) {
    setStatus("Layer đang bị ẩn. Hãy bật hiển thị trước khi vẽ.");
    return;
  }
  if (layers[activeLayer].locked) {
    setStatus("Layer đang bị khóa. Hãy chọn layer khác để vẽ hoặc gôm.");
    return;
  }
  const start = pointFromEvent(event);
  if (activeTool === "brush") {
    pointerAction = {
      mode: "brush",
      shape: { type: "brush", points: [[start.x, start.y]], stroke: brushColor, width: brushWidth, opacity: brushOpacity, fill: "none" }
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

drawing.addEventListener("pointermove", event => {
  const point = pointFromEvent(event);
  document.querySelector("#cursor-position").textContent = `X: ${Math.round(point.x)}   Y: ${Math.round(point.y)}`;
  if (!pointerAction) return;

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

  const dx = point.x - pointerAction.start.x;
  const dy = point.y - pointerAction.start.y;
  if (Math.hypot(dx, dy) > 0.5) pointerAction.moved = true;
  if (!pointerAction.moved) return;
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
  if (action.mode === "brush") {
    const shape = action.shape;
    if (shape.points.length === 1) shape.points.push([...shape.points[0]]);
    snapshot();
    layers[activeLayer].shapes.push(shape);
    selected = { layer: activeLayer, shape: layers[activeLayer].shapes.length - 1 };
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
      render();
      setStatus("Đã gôm nét vẽ");
    } else {
      render();
    }
    return;
  }
  if (action.mode === "draw") {
    const shape = createShape(activeTool, action.start, pointFromEvent(event));
    previewShape = null;
    if (!shape) {
      renderPreview();
      return;
    }
    snapshot();
    layers[activeLayer].shapes.push(shape);
    selected = { layer: activeLayer, shape: layers[activeLayer].shapes.length - 1 };
    render();
    setStatus(`Đã thêm ${toolLabels[activeTool].toLowerCase()}`);
    return;
  }
  if (action.moved) {
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
  flushRender();
});

drawing.addEventListener("pointercancel", () => {
  if (pointerAction?.mode === "draw" || pointerAction?.mode === "brush") previewShape = null;
  if (pointerAction?.mode === "eraser") {
    const state = JSON.parse(pointerAction.originalState);
    layers.splice(0, layers.length, ...state.layers);
    activeLayer = state.activeLayer;
  }
  if (pointerAction?.mode !== "draw" && pointerAction?.moved) {
    const { layer, shape, original } = pointerAction;
    layers[layer].shapes[shape] = original;
  }
  pointerAction = null;
  render();
});

document.querySelectorAll(".tool[data-tool]").forEach(button => {
  button.addEventListener("click", () => {
    activeTool = button.dataset.tool;
    document.querySelectorAll(".tool[data-tool]").forEach(item => item.classList.toggle("active", item === button));
    render();
    setStatus(activeTool === "select" ? "Chế độ chọn" : `Công cụ: ${toolLabels[activeTool]}`);
  });

  document.querySelector("#brush-color").addEventListener("input", event => {
    brushColor = event.target.value;
  });
  document.querySelector("#brush-width").addEventListener("input", event => {
    brushWidth = Number(event.target.value);
    document.querySelector("#brush-width-value").value = brushWidth;
  });
  document.querySelector("#brush-opacity").addEventListener("input", event => {
    brushOpacity = Number(event.target.value) / 100;
    document.querySelector("#brush-opacity-value").value = `${event.target.value}%`;
  });
  document.querySelector("#eraser-width").addEventListener("input", event => {
    eraserWidth = Number(event.target.value);
    document.querySelector("#eraser-width-value").value = eraserWidth;
  });
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
    layers[selected.layer].shapes[selected.shape][property] = property === "width" ? Number(event.target.value) : event.target.value;
    document.querySelector("#width-value").value = event.target.value;
    render();
  });
  input.addEventListener("change", () => { historyCaptured = false; });
}

function deleteSelectedShape() {
  if (!selected) return;
  if (layers[selected.layer].locked) {
    setStatus("Layer đang bị khóa, không thể xóa hình.");
    return;
  }
  snapshot();
  layers[selected.layer].shapes.splice(selected.shape, 1);
  selected = null;
  render();
  setStatus("Đã xóa hình");
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
  render();
  setStatus(`Đã tạo ${name}`);
});

document.querySelector("#theme-toggle").addEventListener("click", () => {
  theme = theme === "dark" ? "light" : "dark";
  localStorage.setItem("mini-vector-cad-theme", theme);
  render();
  const button = document.querySelector("#theme-toggle");
  button.innerHTML = theme === "dark" ? "☀ <span>Sáng</span>" : "☾ <span>Tối</span>";
  button.title = theme === "dark" ? "Chuyển sang giao diện sáng" : "Chuyển sang giao diện tối";
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
  svg.setAttribute("viewBox", "0 0 800 600");
  svg.setAttribute("width", "800");
  svg.setAttribute("height", "600");
  layers.forEach((layer, index) => {
    if (!layer.visible) return;
    const group = makeSvg("g", { id: `layer_${index}` });
    group.setAttribute("data-name", layer.name);
    layer.shapes.forEach(shape => {
      const element = shapeElement(shape, index, -1);
      element.removeAttribute("class");
      element.removeAttribute("data-layer");
      element.removeAttribute("data-shape");
      group.append(element);
    });
    svg.append(group);
  });
  const blob = new Blob([new XMLSerializer().serializeToString(svg)], { type: "image/svg+xml;charset=utf-8" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = "mini-vector-cad.svg";
  link.click();
  setTimeout(() => URL.revokeObjectURL(link.href), 0);
  setStatus("Đã xuất file SVG");
}

document.querySelector("#export").addEventListener("click", downloadSvg);
document.querySelector("#zoom-in").addEventListener("click", () => setZoom(zoom + 10));
document.querySelector("#zoom-out").addEventListener("click", () => setZoom(zoom - 10));
document.querySelector("#fit").addEventListener("click", () => setZoom(100));
function setZoom(value) {
  zoom = Math.max(50, Math.min(150, value));
  drawing.style.width = `min(${88 * zoom / 100}%, ${940 * zoom / 100}px)`;
  document.querySelector("#zoom-label").textContent = `${zoom}%`;
}

document.querySelector("#help").addEventListener("click", () => {
  setStatus("Chọn hình rồi kéo để di chuyển; dùng nút góc để đổi kích thước, nút trên để xoay. Phím tắt: V/R/C/L/T/P/E, Delete, Ctrl+Z");
});

function colorChannels(color, alpha = 255) {
  const hex = color.slice(1);
  return [0, 2, 4].map(index => Number.parseInt(hex.slice(index, index + 2), 16)).concat(alpha);
}

function serializeNativeShape(shape) {
  const stroke = colorChannels(shape.stroke, Math.round((shape.opacity ?? 1) * 255));
  const fill = shape.fill && shape.fill !== "none" ? colorChannels(shape.fill) : [255, 255, 255, 0];
  let geometry;
  if (shape.type === "rect") geometry = `chunhat ${shape.x} ${shape.y} ${shape.w} ${shape.h}`;
  else if (shape.type === "circle") geometry = `tron ${shape.x} ${shape.y} ${shape.r}`;
  else if (shape.type === "line") geometry = `doanthang ${shape.x1} ${shape.y1} ${shape.x2} ${shape.y2}`;
  else if (shape.type === "triangle") geometry = `tamgiac ${shape.points.flat().join(" ")}`;
  else if (shape.type === "polygon") geometry = `dagiac ${shape.points.flat().join(" ")}`;
  else if (shape.type === "brush") geometry = `brush ${shape.points.length} ${shape.points.flat().join(" ")}`;
  else throw new Error(`Loại hình "${shape.type}" chưa hỗ trợ lưu sang C++.`);
  return `${geometry} vien ${stroke.join(" ")} nen ${fill.join(" ")} net ${shape.width}`;
}

function serializeNativeProject() {
  const lines = ["MINI_VECTOR_CAD_V1", `LAYERS ${layers.length}`];
  for (const layer of layers) {
    const name = layer.name.replace(/\\/g, "\\\\").replace(/"/g, '\\"').replace(/\n/g, "\\n").replace(/\t/g, "\\t").replace(/\r/g, "\\r");
    lines.push(`LAYER "${name}" ${layer.visible ? 1 : 0} ${layer.locked ? 1 : 0} ${layer.shapes.length}`);
    layer.shapes.forEach(shape => lines.push(serializeNativeShape(shape)));
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
      const coordinateCount = { tron: 3, circle: 3, chunhat: 4, rect: 4, rectangle: 4, doanthang: 4, line: 4, tamgiac: 6, triangle: 6 }[type];
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

async function syncCppProject(reportStatus = true) {
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
      const response = await fetch("http://127.0.0.1:8766/api/project", {
        method: "POST",
        headers: { "Content-Type": "text/plain;charset=UTF-8" },
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
  if (reportStatus && succeeded) setStatus("Đã đồng bộ bản vẽ với C++");
}

function scheduleCppReconnect() {
  if (backendReconnectTimer || backendConnected) return;
  backendReconnectTimer = setTimeout(() => {
    backendReconnectTimer = 0;
    connectCppBackend();
  }, 3000);
}

async function connectCppBackend() {
  if (backendConnected) return;
  try {
    if (hasPendingLocalChanges) {
      const revision = projectRevision;
      const response = await fetch("http://127.0.0.1:8766/api/project", {
        method: "POST",
        headers: { "Content-Type": "text/plain;charset=UTF-8" },
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
      const response = await fetch("http://127.0.0.1:8766/api/project");
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
    setStatus(`Không kết nối được C++: ${error.message} (chạy MiniVectorCADServer.exe để tự kết nối)`);
    scheduleCppReconnect();
  }
}

window.addEventListener("resize", renderSelectionToolbar);

document.addEventListener("keydown", event => {
  if (event.ctrlKey && event.key.toLowerCase() === "z") {
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
  } else if (["v", "r", "c", "l", "t", "p", "e"].includes(event.key.toLowerCase()) && !["INPUT", "TEXTAREA"].includes(document.activeElement.tagName)) {
    const tool = { v: "select", r: "rect", c: "circle", l: "line", t: "triangle", p: "brush", e: "eraser" }[event.key.toLowerCase()];
    document.querySelector(`.tool[data-tool="${tool}"]`).click();
  }
});

render();
document.querySelector("#theme-toggle").innerHTML = theme === "dark" ? "☀ <span>Sáng</span>" : "☾ <span>Tối</span>";
document.querySelector("#theme-toggle").title = theme === "dark" ? "Chuyển sang giao diện sáng" : "Chuyển sang giao diện tối";
connectCppBackend();
