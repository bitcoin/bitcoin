"use client";
import { useState, useEffect } from "react";
import { useAccount, useReadContract, useWriteContract, useWaitForTransactionReceipt, useEnsAddress } from "wagmi";
import { parseEther, formatEther } from "viem";
import { WETH_ADDRESS, WETH_ABI } from "../contracts/weth";
import styles from "./WETHInteraction.module.css";

const YAKETH_ENS = "yaketh.eth";

export function WETHInteraction() {
  const [depositAmount, setDepositAmount] = useState("");
  const [withdrawAmount, setWithdrawAmount] = useState("");
  const [transferAmount, setTransferAmount] = useState("");
  const { address, isConnected, chain } = useAccount();
  const { writeContract, data: hash, isPending: isWritePending } = useWriteContract();

  const { data: yakethAddress } = useEnsAddress({
    name: YAKETH_ENS,
    chainId: 1,
  });

  const { isLoading: isConfirming, isSuccess: isConfirmed } =
    useWaitForTransactionReceipt({
      hash,
    });

  const { data: balance, refetch: refetchBalance } = useReadContract({
    address: WETH_ADDRESS,
    abi: WETH_ABI,
    functionName: "balanceOf",
    args: address ? [address] : undefined,
    query: {
      enabled: !!address,
    },
  });

  useEffect(() => {
    if (isConfirmed) {
      refetchBalance();
      setDepositAmount("");
      setWithdrawAmount("");
      setTransferAmount("");
    }
  }, [isConfirmed, refetchBalance]);

  const handleDeposit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!depositAmount || parseFloat(depositAmount) <= 0) return;

    try {
      writeContract({
        address: WETH_ADDRESS,
        abi: WETH_ABI,
        functionName: "deposit",
        value: parseEther(depositAmount),
      });
    } catch (error) {
      console.error("Deposit error:", error);
    }
  };

  const handleWithdraw = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!withdrawAmount || parseFloat(withdrawAmount) <= 0) return;

    try {
      writeContract({
        address: WETH_ADDRESS,
        abi: WETH_ABI,
        functionName: "withdraw",
        args: [parseEther(withdrawAmount)],
      });
    } catch (error) {
      console.error("Withdraw error:", error);
    }
  };

  const handleTransferToYaketh = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!transferAmount || parseFloat(transferAmount) <= 0 || !yakethAddress) return;

    try {
      writeContract({
        address: WETH_ADDRESS,
        abi: WETH_ABI,
        functionName: "transfer",
        args: [yakethAddress, parseEther(transferAmount)],
      });
    } catch (error) {
      console.error("Transfer error:", error);
    }
  };

  if (!isConnected) {
    return (
      <div className={styles.container}>
        <h2 className={styles.title}>WETH Contract Interaction</h2>
        <p className={styles.message}>Please connect your wallet to interact with WETH</p>
      </div>
    );
  }

  if (chain?.id !== 1) {
    return (
      <div className={styles.container}>
        <h2 className={styles.title}>WETH Contract Interaction</h2>
        <p className={styles.warning}>
          Please switch to Ethereum Mainnet to interact with WETH
        </p>
        <p className={styles.info}>
          Current chain: {chain?.name || "Unknown"}
        </p>
      </div>
    );
  }

  return (
    <div className={styles.container}>
      <h2 className={styles.title}>WETH Contract Interaction</h2>
      <p className={styles.contractInfo}>
        Contract: <code>{WETH_ADDRESS}</code>
      </p>

      {yakethAddress && (
        <p className={styles.ensInfo}>
          {YAKETH_ENS} → <code>{yakethAddress}</code>
        </p>
      )}

      <div className={styles.balanceSection}>
        <h3>Your WETH Balance</h3>
        <p className={styles.balance}>
          {balance ? formatEther(balance as bigint) : "0"} WETH
        </p>
      </div>

      <div className={styles.actionsContainer}>
        <div className={styles.actionCard}>
          <h3>Deposit ETH → WETH</h3>
          <form onSubmit={handleDeposit}>
            <input
              type="number"
              step="0.001"
              min="0"
              placeholder="Amount in ETH"
              value={depositAmount}
              onChange={(e) => setDepositAmount(e.target.value)}
              className={styles.input}
              disabled={isWritePending || isConfirming}
            />
            <button
              type="submit"
              className={styles.button}
              disabled={!depositAmount || isWritePending || isConfirming}
            >
              {isWritePending || isConfirming ? "Processing..." : "Deposit"}
            </button>
          </form>
        </div>

        <div className={styles.actionCard}>
          <h3>Withdraw WETH → ETH</h3>
          <form onSubmit={handleWithdraw}>
            <input
              type="number"
              step="0.001"
              min="0"
              placeholder="Amount in WETH"
              value={withdrawAmount}
              onChange={(e) => setWithdrawAmount(e.target.value)}
              className={styles.input}
              disabled={isWritePending || isConfirming}
            />
            <button
              type="submit"
              className={styles.button}
              disabled={!withdrawAmount || isWritePending || isConfirming}
            >
              {isWritePending || isConfirming ? "Processing..." : "Withdraw"}
            </button>
          </form>
        </div>

        <div className={styles.actionCard}>
          <h3>Transfer WETH to yaketh.eth</h3>
          <form onSubmit={handleTransferToYaketh}>
            <input
              type="number"
              step="0.001"
              min="0"
              placeholder="Amount in WETH"
              value={transferAmount}
              onChange={(e) => setTransferAmount(e.target.value)}
              className={styles.input}
              disabled={isWritePending || isConfirming || !yakethAddress}
            />
            <button
              type="submit"
              className={styles.button}
              disabled={!transferAmount || isWritePending || isConfirming || !yakethAddress}
            >
              {isWritePending || isConfirming ? "Processing..." : "Transfer to yaketh.eth"}
            </button>
          </form>
          {!yakethAddress && (
            <p className={styles.ensResolving}>Resolving ENS address...</p>
          )}
        </div>
      </div>

      {hash && (
        <div className={styles.transactionStatus}>
          <p>Transaction Hash: <code>{hash}</code></p>
          {isConfirming && <p>Waiting for confirmation...</p>}
          {isConfirmed && <p className={styles.success}>Transaction confirmed! ✓</p>}
        </div>
      )}
    </div>
  );
}
