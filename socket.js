import { io } from "socket.io-client";
import { assignClientNumber } from "./db.js";
import { socketToRNBO } from "./rnbo.js";
//import { TimeNow } from "@rnbo/js";

export function initSocket() {
    const socket = io("http://localhost:5000", { transports: ["websocket"] });

    socket.on("connect", () => {
        console.log("✅ Connecté au serveur Python");
    });



    // --- Drawing Game ---

    // --- Mapping des couleurs
    const colorMap = {
        0xFF0000: "a", // rouge
        0x00FF00: "b", // vert
        0x0000FF: "c", // bleu
        // ...
        0x27F5D3: "d"  // exemple cyan
    };

    // --- Mapping des outils
    const toolMap = {
        brush: "a",
        glitter: "b",
        eraser: "c"
    };

    socket.on("client_action_trigger", (data) => {
        const { client_id, action, datas } = data;
        const clientNumber = assignClientNumber(client_id);

        const color = parseInt(datas.settings.color, 16); // hex string => int
        const tool = datas.settings.tool;

        let payload = [datas.x, datas.y];

        if (action === "dessin_touch") {
            const colorLetter = colorMap[color];
            const toolLetter = toolMap[tool];

            if (colorLetter && toolLetter) {
                const type = colorLetter + toolLetter;
                socketToRNBO(type, payload);
            } else {
                console.warn("⚠️ Mapping inconnu pour :", datas.settings.color, tool);
            }
        }
    });

    // --- Dodgeball Game ---
    socket.on("godot_event_transfer", (data) => {
        const { event_type, event_datas } = data;

        switch (event_type) {
            case "but": {
                console.log("⚽ But reçu :", event_datas);

                const { but_position, but_score } = event_datas;
                const payload = [1, but_position, but_score];
                console.log("⚽ But payload :", payload);

                socketToRNBO("but", payload);
                break;
            }

            case "ball_collide": {
                const { x, y } = event_datas.position2;
                const payload = [1, x, y];
                console.log("💥 Collision balle payload :", payload);
                socketToRNBO("ball", payload);
                break;
            }

            case "ball_bounce": {
                const { position, velocity, with: collidedWith } = event_datas;

                const coords = position
                    .replace(/[()]/g, "")
                    .split(",")
                    .map(v => parseFloat(v.trim()));

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
                    console.log("➡️ Collision avec bouclier :", [1, ...coords, velocity]);
                    socketToRNBO("shield", [1, ...coords, velocity]);

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
