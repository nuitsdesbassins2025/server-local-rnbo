import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import RNBO from "@rnbo/js";
import Speaker from "speaker";
import { AudioContext, OscillatorNode, GainNode } from "node-web-audio-api";


const { createDevice, MessageEvent, TimeNow } = RNBO;

// --- ESM : équivalent de __dirname ---
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function init() {
    // --- Chemins ---
    const patchExportPath = path.join(__dirname, "export/NuitsBassins_dodgeweb.export.json");
    const dependenciesPath = path.join(__dirname, "export/dependencies.json");

    // --- Créer le contexte audio Node ---
    const context = new AudioContext();
    const env = new GainNode(context, { gain: 1 });
    env.connect(context.destination);

    // --- Charger le patch RNBO ---
    if (!fs.existsSync(patchExportPath)) {
        console.error("❌ Patch RNBO introuvable :", patchExportPath);
        return;
    }
    const patcher = JSON.parse(fs.readFileSync(patchExportPath, "utf8"));
    const device = await createDevice({ context, patcher });

    // --- Charger les dépendances (buffers) via HTTP ---
    let dependencies = [];
    if (fs.existsSync(dependenciesPath)) {
        const rawDeps = fs.readFileSync(dependenciesPath, "utf8");
        const deps = JSON.parse(rawDeps);

        dependencies = deps
            .map(d => {
                if (!d.file) return null;
                const filePath = path.join(mediaDir, d.file);
                if (!fs.existsSync(filePath)) {
                    console.warn(`⚠️ Fichier manquant : ${filePath}`);
                    return null;
                }
                // URL HTTP vers le serveur local
                return { id: d.id, file: `http://localhost:${PORT}/media/${d.file}` };
            })
            .filter(Boolean);
    } else {
        console.warn("⚠️ dependencies.json introuvable !");
    }

    // Charger les buffers dans RNBO
    if (dependencies.length > 0) {
        const results = await device.loadDataBufferDependencies(dependencies);
        results.forEach(r =>
            console.log(r.type === "success"
                ? `✅ Buffer chargé: ${r.id}`
                : `❌ Échec buffer: ${r.id} → ${r.error}`)
        );
    }

    // --- Démarrer l’audio ---
    await context.resume();
    console.log("🎵 AudioContext démarré, RNBO prêt.");

    // --- Exemple : envoyer un message à un inport ---
    device.scheduleEvent(new MessageEvent(TimeNow, "play", [1]));

    // --- Ecouter les outports RNBO ---
    device.messageEvent.subscribe(ev => {
        console.log(`📤 RNBO outport: ${ev.tag} → ${ev.payload}`);
    });
}

init().catch(err => console.error("❌ Erreur init RNBO:", err));



/*
// 4. Créer un serveur Socket.IO (port 3000)
const io = new Server(3000, {
    cors: { origin: "*" } // si tu veux connecter un client web
});

io.on("connection", socket => {
    console.log("Client connecté :", socket.id);

    // Exemple : réception d'événements "coords" { x:0.5, y:0.8 }
    socket.on("coords", data => {
        if (typeof data.x === "number") {
            device.scheduleEvent(new MessageEvent(TimeNow, "xCoord", [data.x]));
        }
        if (typeof data.y === "number") {
            device.scheduleEvent(new MessageEvent(TimeNow, "yCoord", [data.y]));
        }
        console.log("Coords reçues et envoyées à RNBO:", data);
    });

    // Exemple : déclencher la lecture
    socket.on("play", () => {
        device.scheduleEvent(new MessageEvent(TimeNow, "play"));
        console.log("Play déclenché via Socket.IO");
    });
});

console.log("Serveur Socket.IO en écoute sur ws://localhost:3000");
*/

/*
let device, context, x;


const RNBO = require("@rnbo/js");
import Interface from './Interface.svelte';
import Button from './Button.svelte';
import { loadSamples, createDeviceInstance } from '@jamesb93/rnbo-svelte';


async function initRNBO() {

    const patchExportURL = "export/NuitsBassins_dodgeweb.export.json";

    // Create Audio Context
    const context = new WA.AudioContext();

    // Create gain node and connect it to audio output
    const outputNode = context.createGain();
    outputNode.connect(context.destination);

    // Create outStream
    const mSD = context.mediaStreamDestination();
    outputNode.connect(mSD);



    // Fetch the exported patcher
    let response, patcher;
    try {
        response = await fetch(patchExportURL);
        patcher = await response.json();

        if (!window.RNBO) {
            // Load RNBO script dynamically
            // Note that you can skip this by knowing the RNBO version of your patch
            // beforehand and just include it using a <script> tag
            await loadRNBOScript(patcher.desc.meta.rnboversion);
        }
    } catch (err) {
        const errorContext = {
            error: err,
        };
        if (response && (response.status >= 300 || response.status < 200)) {
            (errorContext.header = `Couldn't load patcher export bundle`),
                (errorContext.description =
                    `Check app.js to see what file it's trying to load. Currently it's` +
                    ` trying to load "${patchExportURL}". If that doesn't` +
                    ` match the name of the file you exported from RNBO, modify` +
                    ` patchExportURL in app.js.`);
        }
        if (typeof guardrails === "function") {
            guardrails(errorContext);
        } else {
            throw err;
        }
        return;
    }

    // Fetch the dependencies
    let dependencies = [];
    try {
        const dependenciesResponse = await fetch("export/dependencies.json");
        dependencies = await dependenciesResponse.json();

        // Prepend "export" to any file dependenciies
        dependencies = dependencies.map(d => d.file ? Object.assign({}, d, { file: "export/" + d.file }) : d);
    } catch (e) { }

    // Create the device
    let device;
    try {
        device = await RNBO.createDevice({ context, patcher });
    } catch (err) {
        if (typeof guardrails === "function") {
            guardrails({ error: err });
        } else {
            throw err;
        }
        return;
    }


    // Load the samples
    const results = await device.loadDataBufferDependencies(dependencies);
    results.forEach(result => {
        if (result.type === "success") {
            console.log(`Successfully loaded buffer with id ${result.id}`);
        } else {
            console.log(`Failed to load buffer with id ${result.id}, ${result.error}`);
        }
    });

    device.node.connect(outputNode);
    //    attachOutports(rnboDevice);

    // 4) (Optionnel) charger les buffers référencés
    try {
        const depsRes = await fetch("/export/dependencies.json");
        let deps = await depsRes.json();
        // préfixer les chemins si nécessaire
        deps = deps.map(d => d.file ? { ...d, file: "/export/" + d.file } : d);

        const results = await device.loadDataBufferDependencies(deps);
        results.forEach(r =>
            console.log((r.type === "success")
                ? `✅ Buffer chargé: ${r.id}`
                : `❌ Échec buffer: ${r.id} → ${r.error}`
            )
        );
    } catch (_) {
        // pas de dependencies.json, ce n’est pas bloquant
    }

    // Receive Outport Message for Inport Feedback
    device.messageEvent.subscribe((ev) => {
        console.log(`Receive message ${ev.tag}: ${ev.payload}`);

        if (ev.tag === "5") console.log("from the bouclier inport");
    });
}

function loadRNBOScript(version) {
    return new Promise((resolve, reject) => {
        if (/^\d+\.\d+\.\d+-dev$/.test(version)) {
            throw new Error("Patcher exported with a Debug Version!\nPlease specify the correct RNBO version to use in the code.");
        }
        const el = document.createElement("script");
        el.src = "https://c74-public.nyc3.digitaloceanspaces.com/rnbo/" + encodeURIComponent(version) + "/rnbo.min.js";
        el.onload = resolve;
        el.onerror = function (err) {
            console.log(err);
            reject(new Error("Failed to load rnbo.js v" + version));
        };
        document.body.append(el);
    });
}

function triggerEvent(type, x = 0.5, device) {
    if (!["mur", "bouclier"].includes(type)) {
        return console.warn("Type non reconnu:", type);
    }
    const now = RNBO.TimeNow;
    const pan = Math.max(0, Math.min(1, x)) * 2 - 1;
    device.scheduleEvent(new RNBO.MessageEvent(now, `pan_${type}`, [pan]));
    device.scheduleEvent(new RNBO.MessageEvent(now, type, [1]));
    console.log("🎯 Event envoyé :", `${type}`, `${x}`);
}


// Au chargement
initRNBO();
*/