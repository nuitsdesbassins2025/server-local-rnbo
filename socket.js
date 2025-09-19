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
        "#FF0000": 1, // rouge
        "#00FF00": 2, // vert
        "#0000FF": 3, // bleu
        "#FFFF00": 4, // jaune
        "#FF00FF": 5, // magenta
        "#00FFFF": 6, // cyan
        "#FF8000": 7, // orange
        "#8000FF": 8, // violet
        "#0080FF": 9, // bleu marine
        "#FF0080": 10, // rose
        "#008080": 11, // bleu canard
        "#FFFFFF": 12, // blanc
    };

    // --- Mapping des outils
    const toolMap = {
        pencil: 1,
        neon: 2,
        ball: 3,
        eraser: 4
    };

    socket.on("client_action_trigger", (data) => {
        const { client_id, action, datas } = data;
        const clientNumber = assignClientNumber(client_id);

        // sécurité : settings peut être undefined
        const colorHex = datas.settings?.color || "#000000";
        const toolName = datas.settings?.tool || "unknown";

        const colorCode = colorMap[colorHex] || 0;
        const toolCode = toolToInt(toolName);

        // coords
        let x = datas.x;
        let y = datas.y;

        // si c'est un touch_end → on envoie (0,0)
        if (action === "touch_end") {
            x = 0;
            y = 0;
        }

        const payload = [colorCode, toolCode, x, y];
        console.log(`🎨 Payload joueur ${clientNumber} :`, payload);

        const inportName = `player${clientNumber}`;
        socketToRNBO(inportName, payload);
    });

    // --- Dodgeball Game ---
    socket.on("godot_event_transfer", (data) => {
        const { event_type, event_datas } = data;

        switch (event_type) {

            case "lost_control_endgame": {
                console.log("ENDGAME");
                socketToRNBO("endgame", 1);
                break;
            }
            case "but": {
                console.log("⚽ But reçu :", event_datas);

                const { but_position, but_score } = event_datas;
                const payload = [1, but_position, but_score];
                console.log("⚽ But payload :", payload);

                socketToRNBO("goal", payload);
                break;
            }

            case "black_grow": {
                const payload = [1, event_datas.objects_absorbed];
                console.log("BLACK HOLE GROW +1")
                socketToRNBO("blackgrow", payload);
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

                if (["wall", "back_wall"].includes(collidedWith)) {
                    console.log("➡️ Collision avec mur :", [1, ...coords, velocity]);
                    socketToRNBO("wall", [1, ...coords, velocity]);

                } else if (collidedWith === "poteaux") {
                    console.log("➡️ Collision avec poteaux");
                    socketToRNBO("poteaux", [1, ...coords, velocity]);

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
            /*
                        default:
                            console.warn("⚠️ Événement inconnu :", event_type, event_datas);
                            break;
                        */
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
