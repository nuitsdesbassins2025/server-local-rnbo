import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import RNBO from "@rnbo/js";
import { AudioContext, AudioBuffer } from "node-web-audio-api";
const { createDevice, MessageEvent, MIDIEvent, TimeNow } = RNBO;
import { io } from "socket.io-client";

// --- ESM : équivalent de __dirname ---
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// --- Init Buffer ---
global.AudioContext = AudioContext;
global.AudioBuffer = AudioBuffer;

async function init_RNBO() {
    // --- Init Patcher ---
    const patchExportPath = path.join(__dirname, "export/dodgeball_server.export.json");
    const patcher = JSON.parse(fs.readFileSync(patchExportPath, "utf8"));

    // --- Init AudioContext ---
    const context = new AudioContext();

    // --- Init Device ---
    const device = await createDevice({ context, patcher });


    // --- Function Init WAV ---
    async function loadWav(filePath, bufferId) {
        try {
            const raw = fs.readFileSync(filePath);
            // Node Buffer → ArrayBuffer
            const arrayBuf = raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength);
            const audioData = await context.decodeAudioData(arrayBuf);
            await device.setDataBuffer(bufferId, audioData);
            console.log(`✅ Buffer chargé : ${bufferId}`);
        } catch (err) {
            console.error(`❌ Erreur buffer ${bufferId} (${filePath}):`, err.message);
        }
    }

    // --- Init Dependencies ---
    const dependenciesPath = path.join(__dirname, "export", "dependencies.json");
    const mediaDir = path.join(__dirname, "export");

    if (fs.existsSync(dependenciesPath)) {
        const rawDeps = fs.readFileSync(dependenciesPath, "utf8");
        const deps = JSON.parse(rawDeps);

        for (const dep of deps) {
            if (!dep.file) continue;
            const filePath = path.join(mediaDir, dep.file);
            if (!fs.existsSync(filePath)) {
                console.warn(`⚠️ Fichier manquant : ${filePath}`);
                continue;
            }
            await loadWav(filePath, dep.id);
        }
    } else {
        console.warn("⚠️ dependencies.json introuvable !");
    }

    device.node.connect(context.destination);


    device.messageEvent.subscribe(ev => {
        console.log(`📤 RNBO outport: ${ev.tag} → ${ev.payload}`);
    });

    // --- Socket.io ---
    const socket = io("http://localhost:5000", {
        transports: ["websocket"],
    });

    socket.on("connect", () => {
        console.log("✅ Connecté au serveur Python");
    });

    const payload = [
        clientNumber,
        action === "dessin_touch" ? 1 : 0,
        datas.x,
        datas.y,
        parseInt(datas.settings.color, 16),
        datas.settings.brushSize,
        toolToInt(datas.settings.tool),
        datas.first ? 1 : 0,
        datas.last ? 1 : 0,
    ];

    console.log("🎛 Payload RNBO:", payload);

    // Envoi à RNBO
    // const timeNow = device.context.currentTime;
    device.scheduleEvent(new MessageEvent(TimeNow, "from_socket", payload));

    socket.on("disconnect", () => {
        console.log("❌ Client déconnecté:", socket.id);
    });
};

console.log("✅ RNBO + Socket.io serveur prêt !");
console.log("🎹 Appuyez 1, 2 ou 3 pour jouer un son");
console.log("   Ctrl+C pour quitter");


init_RNBO().catch(err => console.error("❌ Erreur init RNBO:", err));


function toolToInt(tool) {
    switch (tool) {
        case "brush": return 1;
        case "glitter": return 2;
        case "eraser": return 3;
        default: return 0;
    }
}

function sendInport(tag, values) {
    console.log("✅ Receive from socket : ", $tag, " ", $values);
    device.scheduleEvent(new MessageEvent(TimeNow, $tag, $values));
}




/*
// --- Capture clavier terminal ---
process.stdin.setRawMode(true);
process.stdin.resume();
process.stdin.setEncoding("utf8");

const keyToMidi = {
    a: 60, // C4
    z: 62, // D4
    e: 64, // E4
    r: 65, // F4
    t: 67, // G4
    y: 69, // A4
    u: 71, // B4
    i: 72  // C5
};

// Listener touches
process.stdin.on("data", (key) => {
    if (key === "\u0003") { // Ctrl+C
        console.log("👋 Bye");
        process.exit();
    }

    if (key === "1") {
        console.log(`🛡️ Touche "1" → Message bouclier`);
        device.scheduleEvent(new MessageEvent(TimeNow, "bouclier", [1]));
        return;
    }

    if (key === "2") {
        console.log(`🛡️ Touche "2" → Message mur`);
        device.scheduleEvent(new MessageEvent(TimeNow, "mur", [1]));
        return;
    }

    if (key === "3") {
        console.log(`🛡️ Touche "3" → Message Joueur`);
        device.scheduleEvent(new MessageEvent(TimeNow, "joueur", [1]));
        return;
    }

    if (keyToMidi[key]) {
        const note = keyToMidi[key];
        console.log(`🎹 Touche "${key}" → Note ${note}`);

        // Exemple : envoi d’un NoteOn et NoteOff à RNBO
        device.scheduleEvent(new MIDIEvent(context.currentTime * 1000, 0, [0x90, note, 0])); // NoteOff qui permet de couper un spam de bouton
        device.scheduleEvent(new MIDIEvent(context.currentTime * 1000, 10, [0x90, note, 100])); // NoteOn
        device.scheduleEvent(new MIDIEvent(context.currentTime * 1000 + 250, 0, [0x90, note, 0])); // NoteOff
    }
});
*/