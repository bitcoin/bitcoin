"use client";

import { useAccount, useConnect, useDisconnect } from "wagmi";
import styles from "./ConnectButton.module.css";

export function ConnectButton() {
  const { address, isConnected } = useAccount();
  const { connect, connectors } = useConnect();
  const { disconnect } = useDisconnect();

  if (isConnected) {
    return (
      <div className={styles.container}>
        <div className={styles.address}>
          {address?.slice(0, 6)}...{address?.slice(-4)}
        </div>
        <button onClick={() => disconnect()} className={styles.button}>
          Disconnect
        </button>
      </div>
    );
  }

  return (
    <div className={styles.container}>
      {connectors.map((connector) => (
        <button
          key={connector.id}
          onClick={() => connect({ connector })}
          className={styles.button}
        >
          Connect Wallet
        </button>
      ))}
    </div>
  );
}
