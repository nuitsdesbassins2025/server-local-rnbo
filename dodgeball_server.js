import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import RNBO from "@rnbo/js";
import { AudioContext, AudioBuffer } from "node-web-audio-api";
const { createDevice, MessageEvent, MIDIEvent, TimeNow } = RNBO;

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

        if (keyToMidi[key]) {
            const note = keyToMidi[key];
            console.log(`🎹 Touche "${key}" → Note ${note}`);

            // Exemple : envoi d’un NoteOn et NoteOff à RNBO
            device.scheduleEvent(new MIDIEvent(TimeNow, 0, [0x90, note, 100])); // NoteOn
        }
    });

    device.messageEvent.subscribe(ev => {
        console.log(`📤 RNBO outport: ${ev.tag} → ${ev.payload}`);
    });


    /*
        // --- Exemple : envoyer un message à un inport ---
        device.scheduleEvent(new MessageEvent(TimeNow, "bouclier", [1]));

    // --- Ecouter les outports RNBO ---



    // --- Socket.io ---
    const io = new Server(3000, { cors: { origin: "*" } });
    io.on("connection", socket => {
        console.log("⚡ Client connecté:", socket.id);

        socket.on("inport", ({ tag, values }) => {
            sendInport(tag, values);
        });

        socket.on("disconnect", () => {
            console.log("❌ Client déconnecté:", socket.id);
        });
    });

    console.log("✅ RNBO + Socket.io serveur prêt !");
 */

    console.log("🎹 Appuyez 1 pour jouer un son");
    console.log("   Ctrl+C pour quitter");
}

init().catch(err => console.error("❌ Erreur init RNBO:", err));

