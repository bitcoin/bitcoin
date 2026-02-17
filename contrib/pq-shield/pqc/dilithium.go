package pqc

import (
	"crypto"
	"crypto/rand"
	"fmt"
	"os"

	"github.com/cloudflare/circl/sign/dilithium/mode3"
)

// GenerateKeyPair creates a new Dilithium Mode3 keypair.
// It persists the keys to the specified paths.
// Returns error if key generation or file writing fails.
func GenerateKeyPair(pubPath, privPath string) error {
	// Generate keypair using system randomness
	// Effect: Consumes entropy from /dev/urandom
	pk, sk, err := mode3.GenerateKey(rand.Reader)
	if err != nil {
		return fmt.Errorf("failed to generate key: %w", err)
	}

	// Persist Public Key
	// Effect: Creates file at pubPath with 0644 permissions
	// pk.Bytes() returns the serialized public key
	if err := os.WriteFile(pubPath, pk.Bytes(), 0644); err != nil {
		return fmt.Errorf("failed to write public key: %w", err)
	}

	// Persist Private Key
	// Effect: Creates file at privPath with 0600 permissions (secure)
	// sk.Bytes() returns the serialized private key
	if err := os.WriteFile(privPath, sk.Bytes(), 0600); err != nil {
		// Cleanup public key on failure
		os.Remove(pubPath)
		return fmt.Errorf("failed to write private key: %w", err)
	}

	return nil
}

// SignMessage signs a message using the private key at privPath.
// Returns the signature.
func SignMessage(privPath string, message []byte) ([]byte, error) {
	// Read Private Key
	skBytes, err := os.ReadFile(privPath)
	if err != nil {
		return nil, fmt.Errorf("failed to read private key: %w", err)
	}

	var sk mode3.PrivateKey
	if err := sk.UnmarshalBinary(skBytes); err != nil {
		return nil, fmt.Errorf("failed to unmarshal private key: %w", err)
	}

	// Sign
	// Effect: Deterministic signature generation (if randomness is not used by scheme, but Dilithium uses randomness for side-channel protection sometimes, though strictly it's deterministic. mode3.Sign uses randomness if provided via SignerOpts or rand reader)
	// mode3.PrivateKey implements crypto.Signer
	signature, err := sk.Sign(rand.Reader, message, crypto.Hash(0))
	if err != nil {
		return nil, fmt.Errorf("failed to sign message: %w", err)
	}

	return signature, nil
}

// VerifySignature verifies a signature using the public key at pubPath.
func VerifySignature(pubPath string, message, signature []byte) (bool, error) {
	// Read Public Key
	pkBytes, err := os.ReadFile(pubPath)
	if err != nil {
		return false, fmt.Errorf("failed to read public key: %w", err)
	}

	var pk mode3.PublicKey
	if err := pk.UnmarshalBinary(pkBytes); err != nil {
		return false, fmt.Errorf("failed to unmarshal public key: %w", err)
	}

	// Verify
	return mode3.Verify(&pk, message, signature), nil
}
