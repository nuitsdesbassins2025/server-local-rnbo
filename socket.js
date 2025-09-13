import { io } from "socket.io-client";
import { assignClientNumber } from "./db.js";
import { socketToRNBO } from "./rnbo.js";
//import { TimeNow } from "@rnbo/js";

export function initSocket() {
    const socket = io("http://localhost:5000", { transports: ["websocket"] });

    socket.on("connect", () => {
        console.log("✅ Connecté au serveur Python");
    });

    socket.on("client_action_trigger", (data) => {
        const { client_id, action, datas } = data;
        const clientNumber = assignClientNumber(client_id);

        let payload = [
            clientNumber,
            action,
            datas.x,
            datas.y,
            parseInt(datas.settings.color, 16),
            datas.settings.brushSize,
            toolToInt(datas.settings.tool),
            datas.first ? 1 : 0,
            datas.last ? 1 : 0,
        ];

        if (action === 'dessin_touch') {
            payload = [
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
        } else {
            if (action === 'something') {
                payload = [
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
            }
        }


        console.log("🎛 Payload RNBO:", payload);
        socketToRNBO(payload);
    });

    socket.on("godot_event_transfer", (data) => {
        const { event_type, event_datas } = data; // event_type = "but", "ball_bounce", "ball_collide", ...

        switch (event_type) {
            case "but": {
                console.log("⚽ But reçu :", event_datas);

                // Extraire les deux valeurs
                const { but_position, but_score } = event_datas;
                const payload = [1, but_position, but_score];
                console.log("⚽ But payload :", payload);

                // Envoi vers RNBO
                socketToRNBO("but", payload);
                break;
            }

            case "ball_collide": {
                //console.log("💥 Collision balle :", event_datas);

                // Extraire uniquement position2.x et position2.y
                const { x, y } = event_datas.position2;
                const payload = [1, x, y];
                console.log('ball_collide_payload :', payload)
                socketToRNBO("ball", payload)
                break;
            }

            case "ball_bounce": {
                //console.log("🏓 Rebond balle :", event_datas);
                const { position, velocity, with: collidedWith } = event_datas;

                // position est une string "(909.9403, 40.0)" → on la parse
                const coords = position
                    .replace(/[()]/g, "") // supprime les parenthèses
                    .split(",")
                    .map(v => parseFloat(v.trim())); // → [909.9403, 40.0]

                if (collidedWith === "wall") {
                    console.log("➡️ Collision avec mur :", [1, ...coords, velocity]);
                    socketToRNBO("wall", [1, ...coords, velocity]);

                } else if (collidedWith === "poteau") {
                    console.log("➡️ Collision avec poteau");
                    socketToRNBO("rebond_poteau", [...coords, velocity]);

                } else if (/^\d{4}$/.test(collidedWith)) {
                    console.log("➡️ Collision avec joueur/ID :", [1, ...coords, velocity, Number(collidedWith)]);
                    socketToRNBO("joueur", [1, ...coords, velocity, Number(collidedWith)]);

                } else if (collidedWith === "shield") {
                    console.log("➡️ Collision avec bouclier/ID :", [1, ...coords, velocity, Number(collidedWith)]);
                    socketToRNBO("shield", [1, ...coords, velocity, Number(collidedWith)]);

                } else {
                    console.warn("⚠️ Type de collision inconnu :", collidedWith);
                }

                break;
            }

            default:
                console.warn("⚠️ Événement inconnu :", event_type, event_datas);
                break;
        }
    });

    function toolToInt(tool) {
        switch (tool) {
            case "brush": return 1;
            case "glitter": return 2;
            case "eraser": return 3;
            default: return 0;
        }
    }
}
