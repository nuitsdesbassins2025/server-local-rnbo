// Exemple wrapper RNBO
// Ici on suppose que `device` est créé ailleurs et importé

import RNBO from "@rnbo/js";
const { TimeNow, MessageEvent } = RNBO;

let device = null;

export function initRNBO(rnboDevice) {
    device = rnboDevice;
}

export function socketToRNBO(type, payload) {
    if (!device) throw new Error("RNBO device non initialisé");
    device.scheduleEvent(new MessageEvent(TimeNow, type, payload));
    console.log("MessageEvent :", type, payload);
}
