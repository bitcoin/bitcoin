package blschia_test

import "crypto/sha256"

// Sha256 is a test helper method for getting the sha256 hash of a byte slice
func Sha256(payload []byte) []byte {
	temp := sha256.Sum256(payload)
	return temp[:]
}
