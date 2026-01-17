const walletState = {
  available: 1.2485,
  pending: 0.0421,
  lastMovement: -0.015,
  addresses: [
    "bc1q5u3a9k2l8ysq0w4d5m5kz9rq5sxfg7l9k3p7d",
    "bc1q9uvj3pj7d6z3l8xj0f2m0k4e7npxv8p4c3z1s",
    "bc1qk6a9s4p2h5y8r7w3d9t0m2z5x4c8n1b6r9j2",
  ],
  seedWords: [
    "luz",
    "trazo",
    "pino",
    "luna",
    "brisa",
    "faro",
    "nube",
    "roble",
    "cobre",
    "cauce",
    "delta",
    "norte",
  ],
  history: [
    {
      id: "TX-3911",
      type: "Recibido",
      amount: "+0.034 BTC",
      status: "confirmado",
      from: "Alice",
      time: "hace 2 h",
    },
    {
      id: "TX-3895",
      type: "Enviado",
      amount: "-0.015 BTC",
      status: "confirmado",
      from: "Servicios Cloud",
      time: "hace 12 min",
    },
    {
      id: "TX-3880",
      type: "Enviado",
      amount: "-0.006 BTC",
      status: "pendiente",
      from: "Reserva",
      time: "hace 1 h",
    },
  ],
};

const availableBalance = document.getElementById("balance-available");
const pendingBalance = document.getElementById("balance-pending");
const lastMovement = document.getElementById("last-movement");
const availableHint = document.getElementById("available-hint");
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
const latency = document.getElementById("latency");
const mempool = document.getElementById("mempool");
const nextBlock = document.getElementById("next-block");

const copyAddressButton = document.getElementById("copy-address");
const newAddressButton = document.getElementById("new-address");
const receiveStatus = document.getElementById("receive-status");

const seedBox = document.getElementById("seed-phrase");
const toggleSeedButton = document.getElementById("toggle-seed");
const backupSeedButton = document.getElementById("backup-seed");
const seedStatus = document.getElementById("seed-status");

let seedVisible = false;

const formatBtc = (value) => `${value.toFixed(4)} BTC`;

const updateBalances = () => {
  availableBalance.textContent = formatBtc(walletState.available);
  pendingBalance.textContent = formatBtc(walletState.pending);
  lastMovement.textContent = `${walletState.lastMovement > 0 ? "+" : ""}${walletState.lastMovement} BTC`;
  availableHint.textContent = walletState.available.toFixed(4);
};

const renderHistory = () => {
  historyList.innerHTML = "";
  walletState.history.forEach((item) => {
    const li = document.createElement("li");
    li.className = "history-item";
    li.innerHTML = `
      <div class="history-meta">
        <span>${item.type} · ${item.id}</span>
        <span class="status-pill ${item.status === "confirmado" ? "confirmed" : item.status === "pendiente" ? "pending" : "failed"}">
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
  walletState.addresses.slice(0, 3).forEach((address, index) => {
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
  const totalCells = 144;
  for (let i = 0; i < totalCells; i += 1) {
    const cell = document.createElement("span");
    const seed = (i * 37 + walletState.addresses[0].length) % 11;
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
  return !Number.isNaN(amount) && amount > 0 && amount <= walletState.available;
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

sendForm.addEventListener("submit", (event) => {
  event.preventDefault();
  resetErrors();

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
  const feeNumber = Number(feeValue) / 10000;
  walletState.available = Math.max(0, walletState.available - amountNumber - feeNumber);
  walletState.pending += amountNumber;
  walletState.lastMovement = -amountNumber;

  walletState.history.unshift({
    id: `TX-${Math.floor(Math.random() * 9000 + 1000)}`,
    type: "Enviado",
    amount: `-${amountNumber.toFixed(3)} BTC`,
    status: "pendiente",
    from: addressValue.slice(0, 10) + "...",
    time: "justo ahora",
  });

  updateBalances();
  renderHistory();
  formStatus.textContent = "Transacción creada y enviada a la mempool.";
  sendForm.reset();
});

estimateButton.addEventListener("click", () => {
  resetErrors();
  simulateEstimate();
});

copyAddressButton.addEventListener("click", async () => {
  const address = walletState.addresses[0];
  try {
    await navigator.clipboard.writeText(address);
    receiveStatus.textContent = "Dirección copiada. Comparte con cuidado.";
  } catch (error) {
    receiveStatus.textContent = "No se pudo copiar automáticamente.";
  }
});

newAddressButton.addEventListener("click", () => {
  const rand = Math.random().toString(36).slice(2, 10);
  const newAddress = `bc1q${rand}9l${rand}x2${rand}`;
  walletState.addresses.unshift(newAddress);
  walletState.addresses = walletState.addresses.slice(0, 5);
  renderAddresses();
  renderQr();
  receiveStatus.textContent = "Nueva dirección generada y lista para compartir.";
});

toggleSeedButton.addEventListener("click", () => {
  seedVisible = !seedVisible;
  seedBox.textContent = seedVisible ? walletState.seedWords.join(" ") : "•••• •••• •••• •••• •••• ••••";
  toggleSeedButton.textContent = seedVisible ? "Ocultar seed" : "Mostrar seed";
});

backupSeedButton.addEventListener("click", () => {
  seedStatus.textContent = "Respaldo iniciado. Guarda el archivo cifrado en un lugar seguro.";
});

const cycleNetwork = () => {
  const height = 821442 + Math.floor(Math.random() * 6);
  const peers = 12 + Math.floor(Math.random() * 6);
  const sync = (99.92 + Math.random() * 0.07).toFixed(2);
  const latencyValue = 160 + Math.floor(Math.random() * 60);
  const mempoolValue = 110 + Math.floor(Math.random() * 60);
  const nextBlockMinutes = 6 + Math.floor(Math.random() * 6);

  blockHeight.textContent = `Altura: ${height.toLocaleString("es-ES")}`;
  syncState.textContent = `Peers: ${peers} activos`;
  syncPercent.textContent = `${sync}%`;
  latency.textContent = `${latencyValue} ms`;
  mempool.textContent = `${mempoolValue} MB`;
  nextBlock.textContent = `~${nextBlockMinutes} min`;

  networkStatus.textContent = sync >= 99.95 ? "Red principal · Sincronizada" : "Red principal · Sincronizando";
  networkStatus.classList.toggle("warning", sync < 99.95);
};

updateBalances();
renderHistory();
renderAddresses();
renderQr();
cycleNetwork();
setInterval(cycleNetwork, 6000);
