import styles from "./page.module.css";
import { ConnectButton } from "./components/ConnectButton";
import { AgentInterface } from "./components/AgentInterface";

export default function Home() {
  return (
    <div className={styles.page}>
      <main className={styles.main}>
        <h1 className={styles.title}>🤖 Onchain Agent</h1>
        <p className={styles.description}>
          Your AI-powered blockchain assistant
        </p>
        
        <div className={styles.connectSection}>
          <ConnectButton />
        </div>

        <div className={styles.agentSection}>
          <AgentInterface />
        </div>
      </main>
      
      <footer className={styles.footer}>
        <p>
          Powered by{" "}
          <a
            href="https://docs.base.org/onchainkit"
            target="_blank"
            rel="noopener noreferrer"
          >
            OnchainKit
          </a>
        </p>
      </footer>
    </div>
  );
}
