package blschia

import (
	"crypto/sha256"
	"encoding/hex"
)

// Hash represents 32 byte of hash data
type Hash [32]byte

// HashFromString convert a hex string into a Hash
func HashFromString(hexString string) (Hash, error) {
	var hash Hash
	bytes, err := hex.DecodeString(hexString)
	if err != nil {
		return Hash{}, err
	}

	var reversed []byte
	for i := range bytes {
		n := bytes[len(bytes)-1-i]
		reversed = append(reversed, n)
	}
	copy(hash[:], reversed)

	return hash, nil
}

// BuildSignHash creates the required signHash for LLMQ threshold signing process
func BuildSignHash(llmqType uint8, quorumHash Hash, signID Hash, msgHash Hash) Hash {
	hasher := sha256.New()
	hasher.Write([]byte{llmqType})
	hasher.Write(quorumHash[:])
	hasher.Write(signID[:])
	hasher.Write(msgHash[:])
	return sha256.Sum256(hasher.Sum(nil))
}
