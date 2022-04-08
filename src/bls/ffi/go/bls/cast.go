package bls

import (
	"unsafe"
)

// SecretKey

func CastFromSecretKey(in *SecretKey) *Fr {
	return (*Fr)(unsafe.Pointer(in))
}

func CastToSecretKey(in *Fr) *SecretKey {
	return (*SecretKey)(unsafe.Pointer(in))
}

// PublicKey

func CastFromPublicKey(in *PublicKey) *G2 {
	return (*G2)(unsafe.Pointer(in))
}

func CastToPublicKey(in *G2) *PublicKey {
	return (*PublicKey)(unsafe.Pointer(in))
}

// Sign

func CastFromSign(in *Sign) *G1 {
	return (*G1)(unsafe.Pointer(in))
}

func CastToSign(in *G1) *Sign {
	return (*Sign)(unsafe.Pointer(in))
}
