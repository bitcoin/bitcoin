"use client";

import { useState } from "react";
import { useAccount } from "wagmi";
import styles from "./AgentInterface.module.css";

type Message = {
  role: "user" | "assistant";
  content: string;
};

export function AgentInterface() {
  const { isConnected } = useAccount();
  const [input, setInput] = useState("");
  const [messages, setMessages] = useState<Message[]>([
    {
      role: "assistant",
      content: "👋 Hello! I'm your onchain agent. I can help you interact with the blockchain. What would you like to do today?",
    },
  ]);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!input.trim()) return;

    // Add user message
    const userMessage = { role: "user", content: input };
    setMessages([...messages, userMessage]);
    setInput("");

    // Simulate agent response
    setTimeout(() => {
      const agentMessage = {
        role: "assistant",
        content: "I'm working on processing your request. This is a template - integrate your agent logic here!",
      };
      setMessages((prev) => [...prev, agentMessage]);
    }, 1000);
  };

  if (!isConnected) {
    return (
      <div className={styles.notConnected}>
        <p>👛 Connect your wallet to start using the agent</p>
      </div>
    );
  }

  return (
    <div className={styles.container}>
      <div className={styles.messages}>
        {messages.map((message, index) => (
          <div
            key={index}
            className={`${styles.message} ${
              message.role === "user" ? styles.userMessage : styles.assistantMessage
            }`}
          >
            <div className={styles.messageContent}>{message.content}</div>
          </div>
        ))}
      </div>

      <form onSubmit={handleSubmit} className={styles.inputForm}>
        <input
          type="text"
          value={input}
          onChange={(e) => setInput(e.target.value)}
          placeholder="Ask your agent anything..."
          className={styles.input}
        />
        <button type="submit" className={styles.sendButton}>
          Send
        </button>
      </form>
    </div>
  );
}
