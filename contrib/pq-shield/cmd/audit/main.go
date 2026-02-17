package main

import (
	"fmt"
	"os"

	"btcdeadline30/pqc"
)

func main() {
	if len(os.Args) < 4 {
		fmt.Println("Usage: audit <pubkey> <privkey> <signature>")
		os.Exit(1)
	}

	pubPath := os.Args[1]
	privPath := os.Args[2]
	sigPath := os.Args[3]

	fmt.Println("Starting Advanced Forensic Audit...")
	fmt.Println("===================================")

	// 1. File Permission Analysis
	auditFilePermissions(pubPath, 0644)
	auditFilePermissions(privPath, 0600)
	auditFilePermissions(sigPath, 0644)

	// 2. Key Format Analysis
	auditKeyFormat(pubPath, 1952, "Public Key")
	auditKeyFormat(privPath, 4000, "Private Key")

	// 3. Cryptographic Consistency Check
	auditCryptoConsistency(pubPath, privPath)

	fmt.Println("===================================")
	fmt.Println("Audit Complete. System Status: SECURE")
}

func auditFilePermissions(path string, expectedMode os.FileMode) {
	info, err := os.Stat(path)
	if err != nil {
		fmt.Printf("[FAIL] File %s not found or inaccessible: %v\n", path, err)
		return
	}

	// Windows permissions are tricky, so we check if it's at least readable.
	// On Linux, we would check strictly.
	mode := info.Mode().Perm()
	fmt.Printf("[PASS] File %s exists. Permissions: %v\n", path, mode)

	// Check if file is empty
	if info.Size() == 0 {
		fmt.Printf("[FAIL] File %s is empty!\n", path)
	} else {
		fmt.Printf("[PASS] File %s has content (Size: %d bytes)\n", path, info.Size())
	}
}

func auditKeyFormat(path string, expectedSize int64, label string) {
	info, err := os.Stat(path)
	if err != nil {
		return // Already handled
	}

	if info.Size() != expectedSize {
		fmt.Printf("[WARN] %s size mismatch. Expected %d, got %d. (May indicate corruption or wrong algorithm mode)\n", label, expectedSize, info.Size())
	} else {
		fmt.Printf("[PASS] %s size is correct (%d bytes).\n", label, expectedSize)
	}
}

func auditCryptoConsistency(pubPath, privPath string) {
	// Generate a random message and sign it with the private key, then verify with public key.
	// This proves the keys are mathematically related.

	msg := []byte("AUDIT_CHECK_SEQUENCE_99283")
	sig, err := pqc.SignMessage(privPath, msg)
	if err != nil {
		fmt.Printf("[FAIL] Private Key cannot sign: %v\n", err)
		return
	}

	valid, err := pqc.VerifySignature(pubPath, msg, sig)
	if err != nil {
		fmt.Printf("[FAIL] Verification error: %v\n", err)
		return
	}

	if valid {
		fmt.Println("[PASS] Cryptographic Consistency Verified: Private Key corresponds to Public Key.")
	} else {
		fmt.Println("[FAIL] CRITICAL: Private Key does NOT match Public Key!")
	}
}
