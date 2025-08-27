// Exemple wrapper RNBO
// Ici on suppose que `device` est créé ailleurs et importé

import RNBO from "@rnbo/js";
const { TimeNow } = RNBO;

let device = null;

export function initRNBO(rnboDevice) {
    device = rnboDevice;
}

export function socketToRNBO(payload) {
    if (!device) throw new Error("RNBO device non initialisé");
    device.scheduleEvent(new MessageEvent(TimeNow, "from_socket", payload));
}
