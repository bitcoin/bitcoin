/**
 * QuantumCast Signal Server
 * Lightweight signaling bridge for SDP/ICE exchange between peers.
 * Designed for decentralized or edge deployments (Netlify, Deno, Vercel, Fly.io).
 *
 * Author: LeonidCypher
 */

import express from "express";
import { v4 as uuidv4 } from "uuid";
import cors from "cors";

const app = express();
app.use(express.json());
app.use(cors());

/**
 * In-memory room storage.
 * In production, you can replace this with Redis, SQLite, or libp2p PubSub.
 */
const rooms: Record<string, unknown[]> = {};

app.post("/signal/:room", (req, res) => {
  const { room } = req.params;
  const payload = req.body;

  if (!rooms[room]) rooms[room] = [];
  rooms[room].push(payload);

  res.json({ status: "ok" });
});

app.get("/signal/:room", (req, res) => {
  const { room } = req.params;
  res.json(rooms[room] || []);
});

app.post("/create-room", (_req, res) => {
  const roomId = uuidv4().slice(0, 8);
  rooms[roomId] = [];
  res.json({ roomId });
});

app.get("/", (_req, res) => {
  res.json({
    name: "QuantumCast Signal Server",
    version: "0.1.0",
    message: "Lightweight signaling API active.",
    routes: ["/create-room", "/signal/:room (POST/GET)"],
  });
});

const PORT = process.env.PORT || 4000;
app.listen(PORT, () => {
  console.log(`🚀 QuantumCast Signal Server running on port ${PORT}`);
});
