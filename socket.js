import { io } from "socket.io-client";
import { assignClientNumber } from "./db.js";
import { socketToRNBO } from "./rnbo.js";

export function initSocket() {
    const socket = io("http://localhost:5000", { transports: ["websocket"] });

    socket.on("connect", () => {
        console.log("✅ Connecté au serveur Python");
    });

    socket.on("client_action_trigger", (data) => {
        const { client_id, action, datas } = data;

        const clientNumber = assignClientNumber(client_id);

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
        socketToRNBO(payload);
    });
}

function toolToInt(tool) {
    switch (tool) {
        case "brush": return 1;
        case "glitter": return 2;
        case "eraser": return 3;
        default: return 0;
    }
}
