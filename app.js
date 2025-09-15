import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import RNBO from "@rnbo/js";
import { AudioContext, AudioBuffer } from "node-web-audio-api";
const { createDevice, TimeNow, MessageEvent } = RNBO;
import { initRNBO } from "./rnbo.js";
import { initSocket } from "./socket.js";

// --- ESM : équivalent de __dirname ---
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// --- Init Buffer ---
global.AudioContext = AudioContext;
global.AudioBuffer = AudioBuffer;

async function loadRNBODevice() {

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

    // --- Capture clavier terminal ---
    process.stdin.setRawMode(true);
    process.stdin.resume();
    process.stdin.setEncoding("utf8");

    process.stdin.on("data", (key) => {
        if (key === "\u0003") { // Ctrl+C
            console.log("👋 Bye");
            process.exit();
        }

        if (key === "0") {
            console.log(`🛡️ Touche "0" → Bang Stop All`);
            device.scheduleEvent(new MessageEvent(TimeNow, "stop", ["bang"]));
            return;
        }
    });

    device.messageEvent.subscribe(ev => {
        console.log(`📤 RNBO outport: ${ev.tag} → ${ev.payload}`);
    });

    return device

}
// --- Initialisation RNBO ---
async function main() {
    // Ici tu charges ton RNBO device
    const device = await loadRNBODevice();
    initRNBO(device);

    // --- Socket.io ---
    initSocket();

    console.log("🚀 Application démarrée");
}

main();

