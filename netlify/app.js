let walletState = {
  status: "loading",
  data: null,
  error: null,
};

let networkState = {
  status: "loading",
  data: null,
  error: null,
};

const availableBalance = document.getElementById("balance-available");
const pendingBalance = document.getElementById("balance-pending");
const lastMovement = document.getElementById("last-movement");
const availableHint = document.getElementById("available-hint");
const activeAddressCode = document.getElementById("active-address");
const copyActiveAddressButton = document.getElementById("copy-active-address");
const activeAddressStatus = document.getElementById("active-address-status");
const historyList = document.getElementById("history-list");
const addressList = document.getElementById("address-list");
const qrCode = document.getElementById("qr-code");

const sendForm = document.getElementById("send-form");
const addressInput = document.getElementById("send-address");
const amountInput = document.getElementById("send-amount");
const feeSelect = document.getElementById("send-fee");
const estimateButton = document.getElementById("estimate");
const formStatus = document.getElementById("form-status");
const addressError = document.getElementById("address-error");
const amountError = document.getElementById("amount-error");
const feeError = document.getElementById("fee-error");

const networkStatus = document.getElementById("network-status");
const blockHeight = document.getElementById("block-height");
const syncState = document.getElementById("sync-state");
const syncPercent = document.getElementById("sync-percent");
const syncProgress = document.getElementById("sync-progress");
const syncChip = document.getElementById("sync-chip");
const latency = document.getElementById("latency");
const mempool = document.getElementById("mempool");
const nextBlock = document.getElementById("next-block");
const peersInbound = document.getElementById("peers-inbound");
const peersOutbound = document.getElementById("peers-outbound");

const copyAddressButton = document.getElementById("copy-address");
const newAddressButton = document.getElementById("new-address");
const receiveStatus = document.getElementById("receive-status");

const seedBox = document.getElementById("seed-phrase");
const toggleSeedButton = document.getElementById("toggle-seed");
const backupSeedButton = document.getElementById("backup-seed");
const seedStatus = document.getElementById("seed-status");

let seedVisible = false;

const formatBtc = (value) => `${Number(value || 0).toFixed(4)} BTC`;

const getWalletData = () =>
  walletState.data || {
    available: 0,
    pending: 0,
    lastMovement: 0,
    addresses: [],
    seedWords: [],
    history: [],
  };

const updateBalances = () => {
  if (walletState.status === "loading") {
    availableBalance.textContent = "Cargando...";
    pendingBalance.textContent = "Cargando...";
    lastMovement.textContent = "Cargando...";
    availableHint.textContent = "—";
    if (activeAddressCode) {
      activeAddressCode.textContent = "Cargando...";
    }
    if (copyActiveAddressButton) {
      copyActiveAddressButton.disabled = true;
    }
    if (activeAddressStatus) {
      activeAddressStatus.textContent = "Esperando datos de la wallet.";
    }
    return;
  }

  if (walletState.status === "error") {
    availableBalance.textContent = "—";
    pendingBalance.textContent = "—";
    lastMovement.textContent = "—";
    availableHint.textContent = "—";
    if (activeAddressCode) {
      activeAddressCode.textContent = "Sin datos";
    }
    if (copyActiveAddressButton) {
      copyActiveAddressButton.disabled = true;
    }
    if (activeAddressStatus) {
      activeAddressStatus.textContent = "No se pudo cargar la dirección.";
    }
    return;
  }

  const data = getWalletData();
  availableBalance.textContent = formatBtc(data.available);
  pendingBalance.textContent = formatBtc(data.pending);
  lastMovement.textContent = `${data.lastMovement > 0 ? "+" : ""}${Number(data.lastMovement || 0).toFixed(4)} BTC`;
  availableHint.textContent = Number(data.available || 0).toFixed(4);
  if (activeAddressCode) {
    activeAddressCode.textContent = data.addresses[0] || "Sin dirección disponible";
  }
  if (copyActiveAddressButton) {
    copyActiveAddressButton.disabled = !data.addresses[0];
  }
  if (activeAddressStatus) {
    activeAddressStatus.textContent = data.addresses[0]
      ? "Lista para compartir."
      : "Genera una nueva dirección para recibir fondos.";
  }
};

const renderHistory = () => {
  historyList.innerHTML = "";

  if (walletState.status === "loading") {
    const li = document.createElement("li");
    li.className = "history-item";
    li.textContent = "Cargando movimientos...";
    historyList.appendChild(li);
    return;
  }

  if (walletState.status === "error") {
    const li = document.createElement("li");
    li.className = "history-item";
    li.textContent = "No se pudo cargar el historial. Intenta de nuevo.";
    historyList.appendChild(li);
    return;
  }

  const data = getWalletData();
  if (!data.history.length) {
    const li = document.createElement("li");
    li.className = "history-item";
    li.textContent = "Sin movimientos recientes.";
    historyList.appendChild(li);
    return;
  }

  data.history.forEach((item) => {
    const li = document.createElement("li");
    li.className = "history-item";
    li.innerHTML = `
      <div class="history-meta">
        <span>${item.type} · ${item.id}</span>
        <span class="status-pill tx-status ${item.status === "confirmado" ? "confirmed" : item.status === "pendiente" ? "pending" : "failed"}">
          ${item.status}
        </span>
      </div>
      <strong>${item.amount}</strong>
      <div class="history-meta">
        <span>${item.from}</span>
        <span>${item.time}</span>
      </div>
    `;
    historyList.appendChild(li);
  });
};

const renderAddresses = () => {
  addressList.innerHTML = "";

  if (walletState.status === "loading") {
    const li = document.createElement("li");
    li.className = "address-item";
    li.textContent = "Cargando direcciones...";
    addressList.appendChild(li);
    return;
  }

  if (walletState.status === "error") {
    const li = document.createElement("li");
    li.className = "address-item";
    li.textContent = "No se pudieron cargar las direcciones.";
    addressList.appendChild(li);
    return;
  }

  const data = getWalletData();
  if (!data.addresses.length) {
    const li = document.createElement("li");
    li.className = "address-item";
    li.textContent = "Sin direcciones disponibles.";
    addressList.appendChild(li);
    return;
  }

  data.addresses.slice(0, 3).forEach((address, index) => {
    const li = document.createElement("li");
    li.className = "address-item";
    li.innerHTML = `
      <div>
        <div>Dirección ${index + 1}</div>
        <code>${address}</code>
      </div>
      <span class="helper">${index === 0 ? "Activa" : "Guardada"}</span>
    `;
    addressList.appendChild(li);
  });
};

const renderQr = () => {
  qrCode.innerHTML = "";
  const data = getWalletData();
  const addressSeed = data.addresses[0] ? data.addresses[0].length : 12;
  const totalCells = 144;
  for (let i = 0; i < totalCells; i += 1) {
    const cell = document.createElement("span");
    const seed = (i * 37 + addressSeed) % 11;
    if (seed % 2 === 0) {
      cell.style.opacity = "0.9";
    } else if (seed % 3 === 0) {
      cell.style.opacity = "0.4";
    } else {
      cell.style.opacity = "0.15";
    }
    qrCode.appendChild(cell);
  }
};

const resetErrors = () => {
  addressError.textContent = "";
  amountError.textContent = "";
  feeError.textContent = "";
};

const validateAddress = (value) => {
  const trimmed = value.trim();
  const startsOk = trimmed.startsWith("bc1") || trimmed.startsWith("1") || trimmed.startsWith("3");
  return trimmed.length >= 26 && startsOk;
};

const validateAmount = (value) => {
  const amount = Number(value);
  if (walletState.status !== "ready") {
    return false;
  }
  return !Number.isNaN(amount) && amount > 0 && amount <= getWalletData().available;
};

const validateFee = (value) => {
  const fee = Number(value);
  return !Number.isNaN(fee) && fee >= 1;
};

const simulateEstimate = () => {
  const fee = Number(feeSelect.value);
  if (!validateFee(fee)) {
    feeError.textContent = "Selecciona una tarifa válida.";
    return;
  }
  const eta = fee >= 20 ? "~10 min" : fee >= 12 ? "~25 min" : "~45 min";
  formStatus.textContent = `Tiempo estimado de confirmación: ${eta}.`;
};

const copyActiveAddress = async (statusElement) => {
  const data = getWalletData();
  const address = data.addresses[0];
  if (!address) {
    statusElement.textContent = "No hay dirección disponible.";
    return;
  }
  try {
    await navigator.clipboard.writeText(address);
    statusElement.textContent = "Dirección copiada. Comparte con cuidado.";
  } catch (error) {
    statusElement.textContent = "No se pudo copiar automáticamente.";
  }
};

sendForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  resetErrors();

  if (walletState.status !== "ready") {
    formStatus.textContent = "Espera a que cargue la información del wallet.";
    return;
  }

  const addressValue = addressInput.value.trim();
  const amountValue = amountInput.value;
  const feeValue = feeSelect.value;

  let hasError = false;

  if (!validateAddress(addressValue)) {
    addressError.textContent = "Dirección inválida. Revisa el formato (bc1, 1 o 3).";
    hasError = true;
  }

  if (!validateAmount(amountValue)) {
    amountError.textContent = "Monto inválido o superior al saldo disponible.";
    hasError = true;
  }

  if (!validateFee(feeValue)) {
    feeError.textContent = "Selecciona una tarifa válida.";
    hasError = true;
  }

  if (hasError) {
    formStatus.textContent = "Corrige los campos antes de continuar.";
    return;
  }

  const amountNumber = Number(amountValue);
  const feeNumber = Number(feeValue);
  formStatus.textContent = "Construyendo y firmando la transacción...";

  try {
    const response = await fetch("/api/tx/send", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        address: addressValue,
        amount: amountNumber,
        feeRate: feeNumber,
      }),
    });

    const payload = await response.json();
    if (!response.ok) {
      throw new Error(payload?.error || `Error del backend (${response.status})`);
    }

    if (payload.wallet) {
      walletState = {
        status: "ready",
        data: {
          ...getWalletData(),
          ...payload.wallet,
        },
        error: null,
      };
      updateBalances();
      renderHistory();
      renderAddresses();
      renderQr();
    }

    if (payload.mempool?.bytes) {
      mempool.textContent = `${(payload.mempool.bytes / 1e6).toFixed(1)} MB`;
    }

    formStatus.textContent = `Transacción ${payload.txid || ""} enviada (${payload.status || "pendiente"}).`;
    sendForm.reset();
  } catch (error) {
    formStatus.textContent =
      error instanceof Error ? error.message : "No se pudo enviar la transacción.";
  }
});

estimateButton.addEventListener("click", () => {
  resetErrors();
  simulateEstimate();
});

copyAddressButton.addEventListener("click", async () => {
  await copyActiveAddress(receiveStatus);
});

if (copyActiveAddressButton && activeAddressStatus) {
  copyActiveAddressButton.addEventListener("click", async () => {
    await copyActiveAddress(activeAddressStatus);
  });
}

newAddressButton.addEventListener("click", () => {
  if (walletState.status !== "ready") {
    receiveStatus.textContent = "No es posible generar una dirección aún.";
    return;
  }
  const data = getWalletData();
  const rand = Math.random().toString(36).slice(2, 10);
  const newAddress = `bc1q${rand}9l${rand}x2${rand}`;
  data.addresses.unshift(newAddress);
  data.addresses = data.addresses.slice(0, 5);
  renderAddresses();
  renderQr();
  updateBalances();
  receiveStatus.textContent = "Nueva dirección generada y lista para compartir.";
});

const renderSeed = () => {
  const data = getWalletData();
  if (!data.seedWords.length) {
    seedBox.textContent = "Seed no disponible";
    toggleSeedButton.textContent = "Mostrar seed";
    return;
  }
  seedBox.textContent = seedVisible ? data.seedWords.join(" ") : "•••• •••• •••• •••• •••• ••••";
  toggleSeedButton.textContent = seedVisible ? "Ocultar seed" : "Mostrar seed";
};

toggleSeedButton.addEventListener("click", () => {
  seedVisible = !seedVisible;
  renderSeed();
});

backupSeedButton.addEventListener("click", () => {
  seedStatus.textContent = "Respaldo iniciado. Guarda el archivo cifrado en un lugar seguro.";
});

const formatChainLabel = (chain) => {
  if (chain === "main") {
    return "Red principal";
  }
  if (chain === "test") {
    return "Testnet";
  }
  if (chain === "signet") {
    return "Signet";
  }
  return chain ? chain.toUpperCase() : "Red";
};

const renderNetwork = () => {
  if (networkState.status === "loading") {
    blockHeight.textContent = "Altura: Cargando...";
    syncState.textContent = "Peers: Cargando...";
    syncPercent.textContent = "—";
    latency.textContent = "—";
    mempool.textContent = "—";
    nextBlock.textContent = "—";
    if (peersInbound) {
      peersInbound.textContent = "Entrantes: —";
    }
    if (peersOutbound) {
      peersOutbound.textContent = "Salientes: —";
    }
    networkStatus.textContent = "Red principal · Sincronizando";
    networkStatus.classList.add("warning");
    networkStatus.classList.remove("success");
    if (syncProgress) {
      syncProgress.style.width = "0%";
      syncProgress.classList.add("warning");
      syncProgress.classList.remove("success");
    }
    if (syncChip) {
      syncChip.textContent = "Sincronizando";
      syncChip.classList.add("warning");
      syncChip.classList.remove("success");
    }
    return;
  }

  if (networkState.status === "error") {
    blockHeight.textContent = "Altura: —";
    syncState.textContent = "Peers: —";
    syncPercent.textContent = "—";
    latency.textContent = "—";
    mempool.textContent = "—";
    nextBlock.textContent = "—";
    if (peersInbound) {
      peersInbound.textContent = "Entrantes: —";
    }
    if (peersOutbound) {
      peersOutbound.textContent = "Salientes: —";
    }
    networkStatus.textContent = "Estado de red no disponible";
    networkStatus.classList.add("warning");
    networkStatus.classList.remove("success");
    if (syncProgress) {
      syncProgress.style.width = "0%";
      syncProgress.classList.add("warning");
      syncProgress.classList.remove("success");
    }
    if (syncChip) {
      syncChip.textContent = "Sin conexión";
      syncChip.classList.add("warning");
      syncChip.classList.remove("success");
    }
    return;
  }

  const data = networkState.data || {};
  const syncValue = Number(data.verificationProgress ?? 0);
  const syncPercentValue = Number.isFinite(syncValue)
    ? Math.min(Math.max(syncValue * 100, 0), 100)
    : 0;
  const syncDisplay = Number.isFinite(syncValue) ? syncPercentValue.toFixed(2) : "—";
  const peers = data.peers?.total ?? 0;
  const inboundPeers = Number.isFinite(data.peers?.inbound) ? data.peers.inbound : null;
  const outboundPeers = Number.isFinite(data.peers?.outbound) ? data.peers.outbound : null;
  const height = Number.isFinite(data.blocks) ? data.blocks : null;
  const latencyValue = Number.isFinite(data.avgPingMs) ? Math.round(data.avgPingMs) : null;
  const mempoolValue = Number.isFinite(data.mempoolBytes)
    ? (data.mempoolBytes / (1024 * 1024)).toFixed(1)
    : null;
  const nextBlockMinutes = Number.isFinite(data.nextBlockMinutes) ? Math.round(data.nextBlockMinutes) : 10;
  const chainLabel = formatChainLabel(data.chain);

  blockHeight.textContent = height ? `Altura: ${height.toLocaleString("es-ES")}` : "Altura: —";
  syncState.textContent = `Peers: ${peers} activos`;
  syncPercent.textContent = syncDisplay === "—" ? "—" : `${syncDisplay}%`;
  latency.textContent = latencyValue !== null ? `${latencyValue} ms` : "—";
  mempool.textContent = mempoolValue !== null ? `${mempoolValue} MB` : "—";
  nextBlock.textContent = `~${nextBlockMinutes} min`;
  if (peersInbound) {
    peersInbound.textContent = `Entrantes: ${inboundPeers !== null ? inboundPeers : "—"}`;
  }
  if (peersOutbound) {
    peersOutbound.textContent = `Salientes: ${outboundPeers !== null ? outboundPeers : "—"}`;
  }

  const isSynced = Number.isFinite(syncValue) ? syncValue >= 0.9995 : false;
  networkStatus.textContent = isSynced ? `${chainLabel} · Sincronizada` : `${chainLabel} · Sincronizando`;
  networkStatus.classList.toggle("warning", !isSynced);
  networkStatus.classList.toggle("success", isSynced);
  if (syncProgress) {
    syncProgress.style.width = `${syncPercentValue}%`;
    syncProgress.classList.toggle("warning", !isSynced);
    syncProgress.classList.toggle("success", isSynced);
  }
  if (syncChip) {
    syncChip.textContent = isSynced ? "Completo" : "En progreso";
    syncChip.classList.toggle("warning", !isSynced);
    syncChip.classList.toggle("success", isSynced);
  }
};

const cycleNetwork = async () => {
  networkState = { status: "loading", data: null, error: null };
  renderNetwork();

  try {
    const response = await fetch("/api/network/status");
    if (!response.ok) {
      throw new Error(`Respuesta inválida (${response.status})`);
    }
    const payload = await response.json();
    networkState = { status: "ready", data: payload, error: null };
  } catch (error) {
    networkState = {
      status: "error",
      data: null,
      error: error instanceof Error ? error.message : "Error desconocido",
    };
  }

  renderNetwork();
};

const loadWalletSummary = async () => {
  walletState = { status: "loading", data: null, error: null };
  updateBalances();
  renderHistory();
  renderAddresses();
  renderQr();
  renderSeed();

  try {
    const response = await fetch("/api/wallet/summary");
    if (!response.ok) {
      throw new Error(`Respuesta inválida (${response.status})`);
    }
    const payload = await response.json();
    walletState = { status: "ready", data: payload, error: null };
  } catch (error) {
    walletState = {
      status: "error",
      data: null,
      error: error instanceof Error ? error.message : "Error desconocido",
    };
  }

  updateBalances();
  renderHistory();
  renderAddresses();
  renderQr();
  renderSeed();
};

loadWalletSummary();
cycleNetwork();
setInterval(cycleNetwork, 6000);
