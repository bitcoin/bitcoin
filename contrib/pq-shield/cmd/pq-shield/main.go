package main

import (
	"flag"
	"fmt"
	"os"

	"btcdeadline30/pqc"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: pq-shield <command> [arguments]")
		fmt.Println("Commands: keygen, sign, verify")
		os.Exit(1)
	}

	command := os.Args[1]

	switch command {
	case "keygen":
		keygenCmd := flag.NewFlagSet("keygen", flag.ExitOnError)
		pubKeyPath := keygenCmd.String("pub", "public.key", "Path to save public key")
		privKeyPath := keygenCmd.String("priv", "private.key", "Path to save private key")
		keygenCmd.Parse(os.Args[2:])

		fmt.Printf("Generating Dilithium (Mode3) Keypair...\n")
		if err := pqc.GenerateKeyPair(*pubKeyPath, *privKeyPath); err != nil {
			fmt.Fprintf(os.Stderr, "Error generating keys: %v\n", err)
			os.Exit(1)
		}
		fmt.Println("Keys generated successfully.")
		// Verify file existence (Kernel Executor style)
		if _, err := os.Stat(*pubKeyPath); err == nil {
			fmt.Printf("Verified: %s exists\n", *pubKeyPath)
		}
		if _, err := os.Stat(*privKeyPath); err == nil {
			fmt.Printf("Verified: %s exists\n", *privKeyPath)
		}

	case "sign":
		signCmd := flag.NewFlagSet("sign", flag.ExitOnError)
		privKeyPath := signCmd.String("priv", "private.key", "Path to private key")
		msgPath := signCmd.String("msg", "message.txt", "Path to message file")
		sigPath := signCmd.String("out", "message.sig", "Path to save signature")
		signCmd.Parse(os.Args[2:])

		msg, err := os.ReadFile(*msgPath)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error reading message: %v\n", err)
			os.Exit(1)
		}

		sig, err := pqc.SignMessage(*privKeyPath, msg)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error signing message: %v\n", err)
			os.Exit(1)
		}

		if err := os.WriteFile(*sigPath, sig, 0644); err != nil {
			fmt.Fprintf(os.Stderr, "Error saving signature: %v\n", err)
			os.Exit(1)
		}
		fmt.Printf("Signature saved to %s\n", *sigPath)

	case "verify":
		verifyCmd := flag.NewFlagSet("verify", flag.ExitOnError)
		pubKeyPath := verifyCmd.String("pub", "public.key", "Path to public key")
		msgPath := verifyCmd.String("msg", "message.txt", "Path to message file")
		sigPath := verifyCmd.String("sig", "message.sig", "Path to signature file")
		verifyCmd.Parse(os.Args[2:])

		msg, err := os.ReadFile(*msgPath)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error reading message: %v\n", err)
			os.Exit(1)
		}

		sig, err := os.ReadFile(*sigPath)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error reading signature: %v\n", err)
			os.Exit(1)
		}

		valid, err := pqc.VerifySignature(*pubKeyPath, msg, sig)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error during verification: %v\n", err)
			os.Exit(1)
		}

		if valid {
			fmt.Println("VERIFIED: Signature is valid.")
			os.Exit(0)
		} else {
			fmt.Println("INVALID: Signature is invalid.")
			os.Exit(1)
		}

	default:
		fmt.Printf("Unknown command: %s\n", command)
		os.Exit(1)
	}
}
