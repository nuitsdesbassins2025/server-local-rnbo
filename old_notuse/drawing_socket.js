import { MessageEvent, TimeNow } from "@rnbo/js";
import { io } from "socket.io-client";
import Database from "better-sqlite3";

// --- Connexion SQLite ---
const db = new Database("mapping.db");

// Créer la table si elle n’existe pas
db.prepare(`
    CREATE TABLE IF NOT EXISTS client_mapping (
    client_id TEXT PRIMARY KEY,
    numero INTEGER
    )
`).run();

// --- Gestion du compteur cyclique ---
let nextNumber = 1;
const MAX_CLIENTS = 10;

// Charger le dernier numéro attribué au démarrage
const row = db.prepare("SELECT MAX(numero) as maxNum FROM client_mapping").get();
if (row && row.maxNum) {
    nextNumber = (row.maxNum % MAX_CLIENTS) + 1;
}

// --- Fonction attribution cyclique persistante ---
function assignClientNumber(client_id) {
    // Vérifie si déjà en DB
    const existing = db.prepare("SELECT numero FROM client_mapping WHERE client_id = ?").get(client_id);
    if (existing) {
        return existing.numero;
    }

    // Sinon, on attribue le prochain numéro cyclique
    const numero = nextNumber;
    db.prepare("INSERT INTO client_mapping (client_id, numero) VALUES (?, ?)").run(client_id, numero);

    console.log(`🔢 Nouveau client ${client_id} → numéro ${numero}`);

    // Préparer le prochain numéro
    nextNumber++;
    if (nextNumber > MAX_CLIENTS) nextNumber = 1;

    return numero;
}

// --- Connexion Socket.IO ---
const socket = io("http://localhost:5000", { transports: ["websocket"] });

socket.on("connect", () => {
    console.log("✅ Connecté au serveur Python");
});

// --- Réception des événements ---
socket.on("client_action_trigger", (data) => {
    const { client_id, action, datas } = data;

    const clientNumber = assignClientNumber(client_id);

    // Construire le payload RNBO
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
    const timeNow = device.context.currentTime;
    device.scheduleEvent(new MessageEvent(timeNow, "from_socket", payload));
});

// --- Helper conversion tool string → entier
function toolToInt(tool) {
    switch (tool) {
        case "brush": return 1;
        case "glitter": return 2;
        case "eraser": return 3;
        default: return 0;
    }
}

