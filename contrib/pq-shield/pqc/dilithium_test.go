package pqc

import (
	"crypto/rand"
	"os"
	"testing"
)

// QA-001: Verify Key Generation Completeness and File Integrity
func TestGenerateKeyPair(t *testing.T) {
	pubFile := "test_pub.key"
	privFile := "test_priv.key"
	defer os.Remove(pubFile)
	defer os.Remove(privFile)

	err := GenerateKeyPair(pubFile, privFile)
	if err != nil {
		t.Fatalf("Failed to generate keys: %v", err)
	}

	// Forensic Check: Verify file existence
	if _, err := os.Stat(pubFile); os.IsNotExist(err) {
		t.Errorf("Public key file not created")
	}
	if _, err := os.Stat(privFile); os.IsNotExist(err) {
		t.Errorf("Private key file not created")
	}

	// Forensic Check: Verify file sizes (Dilithium3 specific)
	// Mode3 Public Key: 1952 bytes
	// Mode3 Private Key: 4000 bytes
	pubBytes, _ := os.ReadFile(pubFile)
	if len(pubBytes) != 1952 {
		t.Errorf("Public key size mismatch. Expected 1952, got %d", len(pubBytes))
	}

	privBytes, _ := os.ReadFile(privFile)
	if len(privBytes) != 4000 {
		t.Errorf("Private key size mismatch. Expected 4000, got %d", len(privBytes))
	}
}

// QA-002: Verify Signature Round-Trip (Sign -> Verify)
func TestSignVerify(t *testing.T) {
	pubFile := "test_pub_sv.key"
	privFile := "test_priv_sv.key"
	defer os.Remove(pubFile)
	defer os.Remove(privFile)

	GenerateKeyPair(pubFile, privFile)

	message := []byte("Quantum supremacy is near.")

	// Execute Signing
	sig, err := SignMessage(privFile, message)
	if err != nil {
		t.Fatalf("Signing failed: %v", err)
	}

	// Verify Signature Length (Dilithium3 signature size is 3293 bytes)
	if len(sig) != 3293 {
		t.Errorf("Signature size mismatch. Expected 3293, got %d", len(sig))
	}

	// Execute Verification
	valid, err := VerifySignature(pubFile, message, sig)
	if err != nil {
		t.Fatalf("Verification failed with error: %v", err)
	}
	if !valid {
		t.Fatal("Valid signature was rejected")
	}
}

// QA-003: Pentest - Tamper Resistance
func TestTamperResistance(t *testing.T) {
	pubFile := "test_pub_tr.key"
	privFile := "test_priv_tr.key"
	defer os.Remove(pubFile)
	defer os.Remove(privFile)

	GenerateKeyPair(pubFile, privFile)

	message := []byte("Original Message")
	sig, _ := SignMessage(privFile, message)

	// Attack Vector 1: Tampered Message
	tamperedMessage := []byte("Tampered Message")
	valid, _ := VerifySignature(pubFile, tamperedMessage, sig)
	if valid {
		t.Fatal("CRITICAL: Signature valid for tampered message!")
	}

	// Attack Vector 2: Tampered Signature
	sig[0] ^= 0xFF // Flip first byte
	valid, _ = VerifySignature(pubFile, message, sig)
	if valid {
		t.Fatal("CRITICAL: Signature valid after bit flip!")
	}
}

// QA-004: Fuzzing - Random Input Robustness
func FuzzVerifySignature(f *testing.F) {
	// Setup initial corpus
	pubFile := "fuzz_pub.key"
	privFile := "fuzz_priv.key"
	defer os.Remove(pubFile)
	defer os.Remove(privFile)
	GenerateKeyPair(pubFile, privFile)

	msg := []byte("fuzz_test")
	sig, _ := SignMessage(privFile, msg)

	f.Add(msg, sig)

	f.Fuzz(func(t *testing.T, message []byte, signature []byte) {
		// We expect this to NOT panic, and generally return false (unless we randomly hit a valid sig, which is impossible)
		// We are testing for crashes here.
		VerifySignature(pubFile, message, signature)
	})
}

// Benchmark-001: Performance Analysis
func BenchmarkGenerateKeyPair(b *testing.B) {
	pubFile := "bench_pub.key"
	privFile := "bench_priv.key"
	defer os.Remove(pubFile)
	defer os.Remove(privFile)

	for i := 0; i < b.N; i++ {
		GenerateKeyPair(pubFile, privFile)
	}
}

func BenchmarkSign(b *testing.B) {
	pubFile := "bench_pub_s.key"
	privFile := "bench_priv_s.key"
	defer os.Remove(pubFile)
	defer os.Remove(privFile)
	GenerateKeyPair(pubFile, privFile)

	message := make([]byte, 1024)
	rand.Read(message)

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		SignMessage(privFile, message)
	}
}

func BenchmarkVerify(b *testing.B) {
	pubFile := "bench_pub_v.key"
	privFile := "bench_priv_v.key"
	defer os.Remove(pubFile)
	defer os.Remove(privFile)
	GenerateKeyPair(pubFile, privFile)

	message := make([]byte, 1024)
	rand.Read(message)
	sig, _ := SignMessage(privFile, message)

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		VerifySignature(pubFile, message, sig)
	}
}
