const buildRpcUrl = () => {
  if (process.env.BITCOIN_RPC_URL) {
    return process.env.BITCOIN_RPC_URL;
  }
  const host = process.env.BITCOIN_RPC_HOST || "http://127.0.0.1";
  const port = process.env.BITCOIN_RPC_PORT || "8332";
  const wallet = process.env.BITCOIN_RPC_WALLET;
  const base = `${host.replace(/\/$/, "")}:${port}`;
  return wallet ? `${base}/wallet/${encodeURIComponent(wallet)}` : base;
};

const rpcCall = async (method, params = []) => {
  const rpcUser = process.env.BITCOIN_RPC_USER;
  const rpcPass = process.env.BITCOIN_RPC_PASS;
  const rpcUrl = buildRpcUrl();

  if (!rpcUser || !rpcPass) {
    throw new Error("RPC credentials missing");
  }

  const response = await fetch(rpcUrl, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Authorization: `Basic ${Buffer.from(`${rpcUser}:${rpcPass}`).toString("base64")}`,
    },
    body: JSON.stringify({
      jsonrpc: "2.0",
      id: method,
      method,
      params,
    }),
  });

  const payload = await response.json();

  if (!response.ok || payload.error) {
    const message = payload.error?.message || response.statusText;
    throw new Error(message);
  }

  return payload.result;
};

const averagePingMs = (peerInfo) => {
  const pingTimes = (peerInfo || [])
    .map((peer) => (Number.isFinite(peer.pingtime) ? peer.pingtime * 1000 : null))
    .filter((value) => Number.isFinite(value));
  if (!pingTimes.length) {
    return null;
  }
  const total = pingTimes.reduce((sum, value) => sum + value, 0);
  return total / pingTimes.length;
};

exports.handler = async () => {
  try {
    const [blockchainInfo, peerInfo, mempoolInfo] = await Promise.all([
      rpcCall("getblockchaininfo"),
      rpcCall("getpeerinfo"),
      rpcCall("getmempoolinfo"),
    ]);

    const peers = peerInfo?.length ?? 0;
    const inboundPeers = (peerInfo || []).filter((peer) => peer.inbound).length;

    return {
      statusCode: 200,
      body: JSON.stringify({
        chain: blockchainInfo?.chain ?? "main",
        blocks: blockchainInfo?.blocks ?? null,
        headers: blockchainInfo?.headers ?? null,
        verificationProgress: blockchainInfo?.verificationprogress ?? null,
        peers: {
          total: peers,
          inbound: inboundPeers,
          outbound: peers - inboundPeers,
        },
        avgPingMs: averagePingMs(peerInfo),
        mempoolBytes: mempoolInfo?.bytes ?? null,
        mempoolSize: mempoolInfo?.size ?? null,
        nextBlockMinutes: 10,
      }),
    };
  } catch (error) {
    return {
      statusCode: 500,
      body: JSON.stringify({
        error: error instanceof Error ? error.message : "RPC error",
      }),
    };
  }
};
