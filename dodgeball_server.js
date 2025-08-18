import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import RNBO from "@rnbo/js";
import { AudioContext, AudioBuffer } from "node-web-audio-api";
const { createDevice, MessageEvent, TimeNow } = RNBO;

// --- ESM : équivalent de __dirname ---
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// --- Init Buffer ---
global.AudioContext = AudioContext;
global.AudioBuffer = AudioBuffer;

async function init() {
    // --- Init Patcher ---
    const patchExportPath = path.join(__dirname, "export/NuitsBassins_dodgeweb.export.json");
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

    // --- Exemple : envoyer un message à un inport ---
    device.scheduleEvent(new MessageEvent(TimeNow, "bouclier", [1]));

    // --- Ecouter les outports RNBO ---
    device.messageEvent.subscribe(ev => {
        console.log(`📤 RNBO outport: ${ev.tag} → ${ev.payload}`);
    });
}

init().catch(err => console.error("❌ Erreur init RNBO:", err));

