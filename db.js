import Database from "better-sqlite3";

const db = new Database("mapping.db");

// Création table
db.prepare(`
    CREATE TABLE IF NOT EXISTS client_mapping (
    client_id TEXT PRIMARY KEY,
    numero INTEGER
    )
`).run();

let nextNumber = 1;
const MAX_CLIENTS = 20;

export function assignClientNumber(client_id) {
    const existing = db.prepare("SELECT numero FROM client_mapping WHERE client_id = ?").get(client_id);
    if (existing) return existing.numero;

    const numero = nextNumber;
    db.prepare("INSERT INTO client_mapping (client_id, numero) VALUES (?, ?)").run(client_id, numero);

    nextNumber++;
    if (nextNumber > MAX_CLIENTS) nextNumber = 1;

    return numero;
}