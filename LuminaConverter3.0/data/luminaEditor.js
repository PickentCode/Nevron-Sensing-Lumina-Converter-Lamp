const Nevrons = {
    NEV_OFF: 0,
    NEV_STALACT: 1,
    NEV_PETANK: 2,
    NEV_GROSSE_TETE: 3,
    NEV_NOIR: 4,
    NEV_ROCHER: 5
};
let pickedNevron = Nevrons.NEV_STALACT;

let colorPickerCount = 1;
let maxColorPickerAmount = 4;

const nevronImages = [
    "",
    "img/stalact.jpg",
    "img/petank.jpg",
    "img/grosse_tete.jpg",
    "img/noir.jpg",
    "img/rocher.jpg"
];

let ws;
function connectWS() {
    const host = location.hostname || "192.168.4.1";
    const url  = `ws://${host}:81/`;
    ws = new WebSocket(url);
    ws.onopen    = () => setStatus("WebSocket: connected");
    ws.onclose   = () => { setStatus("WebSocket: disconnected"); setTimeout(connectWS, 700); }
    ws.onerror   = () => setStatus("WebSocket: error");
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

window.addEventListener('DOMContentLoaded', () => {
    initNevronSelection();
    initControls();

    if (pendingStatus) {
        setStatus(pendingStatus);
        pendingStatus = null;
    }
});

function initControls() {
    const pickersContainer = document.getElementById("pickers");
    const colorPicker = document.getElementById("colorPicker-1");
    const addBtn = document.getElementById("addBtn");
    const removeBtn = document.getElementById("removeBtn");
    const uploadBtn = document.getElementById("uploadBtn");
    const effectSelect = document.getElementById("effectSelect");

    uploadBtn.addEventListener("click", () => {
        console.log("Uploading data...");
        let colors = [];
        
        for (let i = 1; i <= colorPickerCount; i++) {
            const picker = document.getElementById(`colorPicker-${i}`);
            if (picker) {
                const [r, g, b] = hexToRgb(picker.value);
                colors.push(r, g, b);
            }
        }
        console.log("Colors:", colors);
        
        let success = sendPacket(ws, 0xC0, [pickedNevron, ...colors]) &&
        sendPacket(ws, 0xF0, [pickedNevron, parseInt(effectSelect.value, 10)]);
        
        if (success) showNotification("success"); else showNotification("error");
    });
    
    addBtn.addEventListener("click", () => {
        if (colorPickerCount < maxColorPickerAmount) {
            colorPickerCount++;
            const input = document.createElement("input");
            input.type = "color";
            input.value = "#e0ba74";
            input.id = "colorPicker-" + colorPickerCount;
            pickersContainer.appendChild(input);
        }
    });

    removeBtn.addEventListener("click", () => {
        if (colorPickerCount > 1) {
            const lastPicker = document.getElementById("colorPicker-" + colorPickerCount);
            if (lastPicker) {
                pickersContainer.removeChild(lastPicker);
                colorPickerCount--;
            }
        }
    });
}

function initNevronSelection() {
    const container = document.querySelector(".aside");
    container.addEventListener("click", (e) => {
        const target = e.target.closest(".nevron");
        if (!target) return;

        pickedNevron = Number(target.dataset.id);
        document.getElementById("nevImage").src = nevronImages[pickedNevron];

        container.querySelectorAll(".nevron").forEach(div =>
            div.classList.remove("selected")
        );
        target.classList.add("selected");
    });
}

function showNotification(type) {
      const note = document.getElementById(type + "Note");
      note.classList.add("show");

      setTimeout(() => {
        note.classList.remove("show");
      }, 3000);
}

function hexToRgb(hex){
    hex = hex.replace(/^#/, '');
    const n = parseInt(hex, 16);
    return [(n>>16)&255, (n>>8)&255, n&255];
}

function sendPacket(ws, type, payload = []) {
    if (!ws || ws.readyState !== WebSocket.OPEN) return false;
    const buf = new Uint8Array(1 + payload.length);
    buf[0] = type & 0xFF;
    for (let i = 0; i < payload.length; i++) buf[1 + i] = payload[i] & 0xFF;
    ws.send(buf);
    return true;
}