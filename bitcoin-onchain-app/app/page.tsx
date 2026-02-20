"use client";
import Image from "next/image";
import styles from "./page.module.css";
import { Wallet } from "@coinbase/onchainkit/wallet";
import { WETHInteraction } from "./components/WETHInteraction";

export default function Home() {
  return (
    <div className={styles.container}>
      <header className={styles.headerWrapper}>
        <Wallet />
      </header>

      <div className={styles.content}>
        <Image
          priority
          src="/sphere.svg"
          alt="Sphere"
          width={200}
          height={200}
        />
        <h1 className={styles.title}>OnchainKit</h1>

        <WETHInteraction />

        <h2 className={styles.componentsTitle}>Explore Components</h2>

        <ul className={styles.components}>
          {[
            {
              name: "Transaction",
              url: "https://docs.base.org/onchainkit/transaction/transaction",
            },
            {
              name: "Swap",
              url: "https://docs.base.org/onchainkit/swap/swap",
            },
            {
              name: "Checkout",
              url: "https://docs.base.org/onchainkit/checkout/checkout",
            },
            {
              name: "Wallet",
              url: "https://docs.base.org/onchainkit/wallet/wallet",
            },
            {
              name: "Identity",
              url: "https://docs.base.org/onchainkit/identity/identity",
            },
          ].map((component) => (
            <li key={component.name}>
              <a target="_blank" rel="noreferrer" href={component.url}>
                {component.name}
              </a>
            </li>
          ))}
        </ul>
      </div>
    </div>
  );
}
