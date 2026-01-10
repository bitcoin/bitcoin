/**
 * QuantumCast — Signal Client Example (browser-first)
 * Connects two peers via the minimal Signal Server and establishes a WebRTC DataChannel.
 * Modes:
 *  - screen: creates room, sends offer, waits for answer
 *  - remote: joins room, receives offer, sends answer
 *
 * Usage (serve this as a simple TS/JS app or paste into a bundler project):
 *  - Open: https://your-app/#role=screen
 *    -> console will print a roomId and a join link for the remote
 *  - Open: https://your-app/#role=remote&room=XXXX
 *    -> it will auto-join and establish a P2P channel
 *
 * Requires:
 *  - Signal server running (see apps/signal-server/index.ts)
 *  - @quantumcore/quantumcast-core built and resolvable
 *
 * Author: LeonidCypher
 */

import {
  createPeer,
  createOffer,
  acceptOffer,
  finalizeAnswer,
  sendMessage,
  onMessage,
  newSessionId,
} from "@quantumcore/quantumcast-core";

// ================== CONFIG ==================
const SIGNAL_BASE =
  (window as { QUANTUMCAST_SIGNAL_BASE?: string }).QUANTUMCAST_SIGNAL_BASE ||
  "http://localhost:4000";
const POLL_INTERVAL_MS = 1000;

// ================== HELPERS ==================
function parseHash(): Record<string, string> {
  const h = window.location.hash.replace(/^#/, "");
  const out: Record<string, string> = {};
  for (const kv of h.split("&")) {
    const [k, v] = kv.split("=");
    if (k) out[decodeURIComponent(k)] = decodeURIComponent(v || "");
  }
  return out;
}

async function createRoom(): Promise<string> {
  const res = await fetch(`${SIGNAL_BASE}/create-room`, { method: "POST" });
  const data = await res.json();
  return data.roomId as string;
}

async function postSignal(room: string, payload: unknown): Promise<void> {
  await fetch(`${SIGNAL_BASE}/signal/${room}`, {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify(payload),
  });
}

async function getSignals(room: string): Promise<unknown[]> {
  const res = await fetch(`${SIGNAL_BASE}/signal/${room}`);
  return res.json();
}

// Long-poll style reader (naive): keeps track of how many messages were seen
class SignalCursor {
  private seen = 0;
  constructor(private room: string) {}
  async nextBatch(): Promise<unknown[]> {
    const all = await getSignals(this.room);
    if (all.length <= this.seen) return [];
    const slice = all.slice(this.seen);
    this.seen = all.length;
    return slice;
  }
}

// ================== ICE exchange helpers ==================
function wireIceExchange(
  pc: RTCPeerConnection,
  room: string,
  role: "screen" | "remote"
) {
  pc.onicecandidate = (e) => {
    if (e.candidate) {
      void postSignal(room, { type: "ice", from: role, candidate: e.candidate });
    }
  };
}

async function consumeIce(
  pc: RTCPeerConnection,
  msgs: Array<{ type?: string; from?: string; candidate?: RTCIceCandidateInit }>,
  fromRole: "screen" | "remote"
) {
  for (const m of msgs) {
    if (m.type === "ice" && m.from === fromRole && m.candidate) {
      try {
        await pc.addIceCandidate(m.candidate);
      } catch (err) {
        console.warn("ICE addCandidate error:", err);
      }
    }
  }
}

// ================== MAIN FLOWS ==================
async function runScreen() {
  console.log("🎬 Role: screen");
  const roomId = await createRoom();
  console.log(`🆔 roomId: ${roomId}`);
  console.log(
    `🔗 Remote join link: ${window.location.origin}${window.location.pathname}#role=remote&room=${roomId}`
  );

  const { pc, channel } = await createPeer(true);
  if (!channel) throw new Error("DataChannel not created (screen)");
  const sessionId = newSessionId();

  // Wire message receive
  onMessage(channel, (msg) => {
    console.log("📥 [screen] received:", msg);
  });

  wireIceExchange(pc, roomId, "screen");

  // Create and send offer
  const offerSdp = await createOffer(pc);
  await postSignal(roomId, { type: "offer", sdp: offerSdp });

  // Poll for answer + ICE
  const cursor = new SignalCursor(roomId);
  const poll = window.setInterval(async () => {
    const batch = await cursor.nextBatch();
    if (!batch.length) return;

    // Handle answer
    const answerMsg = batch.find(
      (m) => typeof m === "object" && m && (m as { type?: string }).type === "answer"
    ) as { sdp?: string } | undefined;
    if (answerMsg?.sdp) {
      await finalizeAnswer(pc, answerMsg.sdp);
      console.log("✅ Screen: Answer applied");
    }

    // Handle remote ICE
    await consumeIce(
      pc,
      batch as Array<{ type?: string; from?: string; candidate?: RTCIceCandidateInit }>,
      "remote"
    );

    // Once connected, demonstrate messaging
    if (channel.readyState === "open") {
      console.log("🟢 DataChannel open — sending demo messages...");
      sendMessage(channel, {
        t: "LOAD",
        sid: sessionId,
        url: "https://example.com/intro.mp4",
      });
      sendMessage(channel, { t: "PLAY", sid: sessionId });
      clearInterval(poll);
    } else {
      channel.onopen = () => {
        console.log("🟢 DataChannel open (late) — sending demo messages...");
        sendMessage(channel, {
          t: "LOAD",
          sid: sessionId,
          url: "https://example.com/intro.mp4",
        });
        sendMessage(channel, { t: "PLAY", sid: sessionId });
      };
    }
  }, POLL_INTERVAL_MS);
}

async function runRemote(roomId: string) {
  console.log("📱 Role: remote");
  console.log("Joining room:", roomId);

  const { pc, channel } = await createPeer(false);
  wireIceExchange(pc, roomId, "remote");

  const cursor = new SignalCursor(roomId);

  // Wait for offer, then send answer
  const waitOffer = window.setInterval(async () => {
    const batch = await cursor.nextBatch();
    if (!batch.length) return;

    const offerMsg = batch.find(
      (m) => typeof m === "object" && m && (m as { type?: string }).type === "offer"
    ) as { sdp?: string } | undefined;
    if (offerMsg?.sdp) {
      const answerSdp = await acceptOffer(pc, offerMsg.sdp);
      await postSignal(roomId, { type: "answer", sdp: answerSdp });
      console.log("✅ Remote: Answer posted");
      clearInterval(waitOffer);
    }

    // consume screen ICE immediately too
    await consumeIce(
      pc,
      batch as Array<{ type?: string; from?: string; candidate?: RTCIceCandidateInit }>,
      "screen"
    );
  }, POLL_INTERVAL_MS);

  // Messaging
  const onOpen = () => {
    console.log("🟢 DataChannel open (remote)");
    if (!channel) return;
    onMessage(channel, (msg) => {
      console.log("📥 [remote] received:", msg);
      // You can react to LOAD/PLAY/PAUSE here (control UI / player)
    });
  };

  if (channel) {
    channel.onopen = onOpen;
  } else {
    pc.ondatachannel = (e: RTCDataChannelEvent) => {
      const ch = e.channel;
      ch.onopen = onOpen;
      onMessage(ch, (msg) => console.log("📥 [remote] received:", msg));
    };
  }
}

// ================== BOOT ==================
void (async () => {
  const params = parseHash();
  const role = (params.role || "screen") as "screen" | "remote";
  if (role === "screen") {
    await runScreen();
  } else {
    const room = params.room;
    if (!room) {
      alert("Missing room param. Use #role=remote&room=XXXX");
      return;
    }
    await runRemote(room);
  }
})();
