const ATTACK_MS  = 40;
const RELEASE_MS = 120;
const WINDOW_SAMPLES = 6;

let ctx;
let analyser;
let data;
let meter = 0;
let prevTs = 0;
let levelSmooth = 0;
let lastSend = 0;
let lastColSend = 0;
let levelElement, pickerElement;

let ws;
function connectWS() {
    const host = location.hostname || "192.168.4.1";
    const url  = `ws://${host}:81/`;
    ws = new WebSocket(url);
    ws.onopen = () => setStatus("WebSocket: connected");
    ws.onclose = () => { setStatus("WebSocket: disconnected"); setTimeout(connectWS, 700); }
    ws.onerror = () => setStatus("WebSocket: error");
    ws.onmessage = (ev) => console.log("WebSocket", ev.data);
}
connectWS();

let pendingStatus = null;
function setStatus(text) {
    const el = document.getElementById("status");
    if (el) {
        el.textContent = text;
    } else {
        pendingStatus = text;
    }
}

function initWebSocket() {
    if (pendingStatus) {
        console.log(pendingStatus);
        setStatus(pendingStatus);
        pendingStatus = null;
    }

    levelElement = document.getElementById("level");
    pickerElement = document.getElementById("colorPicker");

    pickerElement.addEventListener("change", () => {
        let time = performance.now();
        if (time - lastColSend > 100) {
            const [r, g, b] = hexToRgb(pickerElement.value);
            sendPacket(ws, 0xCA, [r & 0xFF, g & 0xFF, b & 0xFF]);
            lastColSend = time;
        }
    });
}

setInterval(() => {
    if (ws && ws.readyState == 1) ws.send("PLAYER_PING");
}, 1000);

function initAudio() {
    if (ctx) return;

    ctx = new (window.AudioContext || window.webkitAudioContext)();
    analyser = ctx.createAnalyser();
    analyser.fftSize = 2048;
    data = new Float32Array(analyser.fftSize);

    const src = ctx.createMediaElementSource(player);
    src.connect(analyser);
    src.connect(ctx.destination);
}

let win = WINDOW_SAMPLES ? new Float32Array(WINDOW_SAMPLES) : null;
let winSum = 0, winIdx = 0, winCount = 0;
function pushAvg(x) {
    if (!WINDOW_SAMPLES) return x;
    winSum += x - win[winIdx];
    win[winIdx] = x;
    winIdx = (winIdx + 1) % WINDOW_SAMPLES;
    if (winCount < WINDOW_SAMPLES) winCount++;
    return winSum / winCount;
}

function float32ToBytesLE(value) {
    const buf = new ArrayBuffer(4);
    new DataView(buf).setFloat32(0, value, true);
    return new Uint8Array(buf);
}

function hexToRgb(hex){
    hex = hex.replace(/^#/, '');
    const n = parseInt(hex, 16);
    return [(n>>16)&255, (n>>8)&255, n&255];
}

function sendPacket(ws, type, payload = new Uint8Array(0)) {
    if (!ws || ws.readyState !== WebSocket.OPEN) return false;
    const out = new Uint8Array(1 + payload.length);
    out[0] = type & 0xFF;
    out.set(payload, 1);
    ws.send(out);
    return true;
}

function tick(ts) {
  if (analyser) {
    if (!prevTs) prevTs = ts;
    const dt = ts - prevTs;
    prevTs = ts;

    analyser.getFloatTimeDomainData(data);
    let s = 0;
    for (let i = 0; i < data.length; i++) { const v = data[i]; s += v * v; }
    const rms = Math.sqrt(s / data.length);
    const raw = Math.min(1, rms * 3.0);
    const target = pushAvg(raw);
    const tau = (target > meter) ? ATTACK_MS : RELEASE_MS;
    const alpha = 1 - Math.exp(-(dt) / tau);
    meter += (target - meter) * alpha;

    levelElement.textContent = "LEVEL: " + meter.toFixed(2);

    if (ws && ws.readyState === 1 && ts - lastSend > 50) {
      const bytes = float32ToBytesLE(meter);
      sendPacket(ws, 0xA0, bytes);
      lastSend = ts;
    }
  }
  requestAnimationFrame(tick);
}
requestAnimationFrame(tick);