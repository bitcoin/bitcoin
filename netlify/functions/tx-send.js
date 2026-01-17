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

const parseBody = (body) => {
  if (!body) {
    return null;
  }
  try {
    return JSON.parse(body);
  } catch (error) {
    return null;
  }
};

exports.handler = async (event) => {
  if (event.httpMethod !== "POST") {
    return {
      statusCode: 405,
      body: JSON.stringify({ error: "Método no permitido" }),
    };
  }

  const payload = parseBody(event.body);
  const address = payload?.address?.trim();
  const amount = Number(payload?.amount);
  const feeRate = Number(payload?.feeRate);

  if (!address || !Number.isFinite(amount) || amount <= 0 || !Number.isFinite(feeRate) || feeRate < 1) {
    return {
      statusCode: 400,
      body: JSON.stringify({
        error: "Solicitud inválida. Verifica dirección, monto y tarifa.",
      }),
    };
  }

  const feeRateBtcPerKvB = feeRate * 0.00001;

  try {
    const psbtPayload = await rpcCall("walletcreatefundedpsbt", [
      [],
      [{ [address]: amount }],
      0,
      {
        fee_rate: feeRateBtcPerKvB,
        replaceable: true,
      },
    ]);

    const processed = await rpcCall("walletprocesspsbt", [psbtPayload.psbt]);
    const finalized = await rpcCall("finalizepsbt", [processed.psbt]);

    if (!finalized.hex) {
      throw new Error("No se pudo finalizar la transacción");
    }

    const txid = await rpcCall("sendrawtransaction", [finalized.hex]);

    const [balances, transactions, received, mempoolEntry, mempoolInfo] = await Promise.all([
      rpcCall("getbalances"),
      rpcCall("listtransactions", ["*", 20, 0, true]),
      rpcCall("listreceivedbyaddress", [0, true]),
      rpcCall("getmempoolentry", [txid]),
      rpcCall("getmempoolinfo"),
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
        txid,
        status: "pendiente",
        mempool: {
          vsize: mempoolEntry?.vsize ?? null,
          weight: mempoolEntry?.weight ?? null,
          fee: mempoolEntry?.fees?.base ?? null,
          bytes: mempoolInfo?.bytes ?? null,
          size: mempoolInfo?.size ?? null,
        },
        wallet: {
          available,
          pending,
          lastMovement,
          addresses,
          seedWords,
          history: (transactions || []).map(formatTx),
        },
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
