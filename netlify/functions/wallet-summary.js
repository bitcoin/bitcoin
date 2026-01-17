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

const formatTx = (tx) => {
  const amount = Number(tx.amount || 0);
  const formattedAmount = `${amount > 0 ? "+" : ""}${amount.toFixed(3)} BTC`;
  const status = tx.confirmations > 0 ? "confirmado" : "pendiente";
  const timeLabel = tx.time
    ? new Date(tx.time * 1000).toLocaleString("es-ES", {
        hour: "2-digit",
        minute: "2-digit",
        day: "2-digit",
        month: "short",
      })
    : "";

  return {
    id: tx.txid ? `TX-${tx.txid.slice(0, 6)}` : "TX-0000",
    type: amount >= 0 ? "Recibido" : "Enviado",
    amount: formattedAmount,
    status,
    from: tx.label || tx.address || "Wallet",
    time: timeLabel || "reciente",
  };
};

exports.handler = async () => {
  try {
    const [balances, utxos, transactions, received] = await Promise.all([
      rpcCall("getbalances"),
      rpcCall("listunspent", [0, 9999999]),
      rpcCall("listtransactions", ["*", 20, 0, true]),
      rpcCall("listreceivedbyaddress", [0, true]),
    ]);

    const available = balances?.mine?.trusted ?? 0;
    const pending = balances?.mine?.untrusted_pending ?? 0;
    const lastMovement = transactions?.length ? Number(transactions[0].amount || 0) : 0;

    const addresses = (received || [])
      .map((entry) => entry.address)
      .filter(Boolean);

    const seedWords = process.env.WALLET_SEED_WORDS
      ? process.env.WALLET_SEED_WORDS.split(/\s+/).filter(Boolean)
      : [];

    return {
      statusCode: 200,
      body: JSON.stringify({
        available,
        pending,
        lastMovement,
        addresses,
        seedWords,
        history: (transactions || []).map(formatTx),
        utxos,
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
