import fs from "fs";
import Speaker from "speaker";
import { createDevice, MessageEvent, TimeNow } from "rnbo/js";

const patchExportURL = "export/NuitsBassins_dodgeweb.export.json";

async function init() {

    //0. Chemin
    const patchExportURL = "export/NuitsBassins_dodgeweb.export.json";
    const dependenciesURL = "export/dependencies.json"

    //1.1. RNBO - Fetch Patch
    let response, patcher;
    response = await fetch(patchExportURL);
    patcher = await response.json();

    //1.2. RNBO - Fetch the dependencies
    let dependencies = [];
    try {
        const dependenciesResponse = await fetch(dependenciesURL);
        dependencies = await dependenciesResponse.json();

        // Prepend "export" to any file dependenciies
        dependencies = dependencies.map(d => d.file ? Object.assign({}, d, { file: "export/" + d.file }) : d);
    } catch (e) { }

    //1.3. RNBO - Create the device
    let device;
    try {
        device = await createDevice({ context, patcher });
    } catch (err) {
        if (typeof guardrails === "function") {
            guardrails({ error: err });
        } else {
            throw err;
        }
        return;
    }

    // 2.1. SPEAKER - Créer la sortie Speaker
    const speaker = new Speaker({
        channels: device.numOutputChannels,  // stéréo ou mono selon RNBO
        bitDepth: 16,
        sampleRate: device.sampleRate        // doit matcher celui du patch
    });

    // 2.2. SPEAKER - Fonction de rendu en boucle
    const FRAMES = 128; // taille de buffer RNBO
    function process() {
        const out = device.getOutputData(FRAMES); // Float32Array
        const pcm = Buffer.alloc(out.length * 2); // 16-bit PCM

        for (let i = 0; i < out.length; i++) {
            let s = Math.max(-1, Math.min(1, out[i])); // clamp
            pcm.writeInt16LE(s * 32767, i * 2);
        }

        speaker.write(pcm);
        setImmediate(process); // relance boucle
    }

    process();

    // 4. Trigger
    const playParam = device.parametersById.get("play");
    if (playParam) {
        playParam.value = 1;
    }

    const inport = device.inport.get("")

    console.log("RNBO + Speaker prêt ✅");

    //5. Stream

}

init();


function triggerEvent(type, x = 0.5, device) {
    if (!["mur", "bouclier"].includes(type)) {
        return console.warn("Type non reconnu:", type);
    }
    const now = RNBO.TimeNow;
    const pan = Math.max(0, Math.min(1, x)) * 2 - 1;
    device.scheduleEvent(new RNBO.MessageEvent(now, `pan_${type}`, [pan]));
    device.scheduleEvent(new RNBO.MessageEvent(now, type, [1]));
    console.log("🎯 Event envoyé :", `${type}`, `${x}`);



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