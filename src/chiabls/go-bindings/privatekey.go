package blschia

// #cgo LDFLAGS: -lchiabls -lstdc++ -lgmp
// #cgo CXXFLAGS: -std=c++14
// #include <stdbool.h>
// #include <stdlib.h>
// #include "privatekey.h"
// #include "blschia.h"
import "C"
import (
	"encoding/hex"
	"errors"
	"runtime"
	"unsafe"
)

// PrivateKey represents a BLS private key
type PrivateKey struct {
	sk C.CPrivateKey
}

// PrivateKeyFromSeed generates a private key from a seed, similar to HD key
// generation (hashes the seed), and reduces it mod the group order
func PrivateKeyFromSeed(seed []byte) (*PrivateKey, error) {
	// Get a C pointer to bytes
	cBytesPtr := C.CBytes(seed)
	defer C.free(cBytesPtr)

	var sk PrivateKey
	var cDidErr C.bool
	sk.sk = C.CPrivateKeyFromSeed(cBytesPtr, C.int(len(seed)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sk, func(p *PrivateKey) { p.Free() })

	return &sk, nil
}

// PrivateKeyFromBytes constructs a new private key from bytes
func PrivateKeyFromBytes(data []byte, modOrder bool) (*PrivateKey, error) {
	// Get a C pointer to bytes
	cBytesPtr := C.CBytes(data)
	defer C.free(cBytesPtr)

	var sk PrivateKey
	var cDidErr C.bool
	sk.sk = C.CPrivateKeyFromBytes(cBytesPtr, C.bool(modOrder), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sk, func(p *PrivateKey) { p.Free() })
	return &sk, nil
}

// PrivateKeyFromString constructs a new private key from hex string
func PrivateKeyFromString(hexString string) (*PrivateKey, error) {
	bytes, err := hex.DecodeString(hexString)
	if err != nil {
		return nil, err
	}
	return PrivateKeyFromBytes(bytes, false)
}

// Free releases memory allocated by the key
func (sk *PrivateKey) Free() {
	C.CPrivateKeyFree(sk.sk)
	runtime.KeepAlive(sk)
}

// Serialize returns the byte representation of the private key
func (sk *PrivateKey) Serialize() []byte {
	ptr := C.CPrivateKeySerialize(sk.sk)
	defer C.SecFree(ptr)
	runtime.KeepAlive(sk)
	return C.GoBytes(ptr, C.CPrivateKeySizeBytes())
}

// PublicKey returns the public key which corresponds to the private key
func (sk *PrivateKey) PublicKey() *PublicKey {
	var pk PublicKey
	pk.pk = C.CPrivateKeyGetPublicKey(sk.sk)

	runtime.SetFinalizer(&pk, func(p *PublicKey) { p.Free() })
	runtime.KeepAlive(sk)

	return &pk
}

// SignInsecure signs a message without setting aggreagation info
func (sk *PrivateKey) SignInsecure(message []byte) *InsecureSignature {
	// Get a C pointer to bytes
	cMessagePtr := C.CBytes(message)
	defer C.free(cMessagePtr)

	var sig InsecureSignature
	sig.sig = C.CPrivateKeySignInsecure(sk.sk, cMessagePtr, C.size_t(len(message)))

	runtime.SetFinalizer(&sig, func(p *InsecureSignature) { p.Free() })
	runtime.KeepAlive(sk)

	return &sig
}

// SignInsecurePrehashed signs a message without setting aggreagation info
func (sk *PrivateKey) SignInsecurePrehashed(hash []byte) *InsecureSignature {
	// Get a C pointer to bytes
	cHashPtr := C.CBytes(hash)
	defer C.free(cHashPtr)

	var sig InsecureSignature
	sig.sig = C.CPrivateKeySignInsecurePrehashed(sk.sk, cHashPtr)

	runtime.KeepAlive(sk)
	runtime.SetFinalizer(&sig, func(p *InsecureSignature) { p.Free() })

	return &sig
}

// Sign securely signs a message, and sets and returns appropriate aggregation
// info
func (sk *PrivateKey) Sign(message []byte) *Signature {
	// Get a C pointer to bytes
	cMessagePtr := C.CBytes(message)
	defer C.free(cMessagePtr)

	var sig Signature
	sig.sig = C.CPrivateKeySign(sk.sk, cMessagePtr, C.size_t(len(message)))

	runtime.SetFinalizer(&sig, func(p *Signature) { p.Free() })
	runtime.KeepAlive(sk)

	return &sig
}

// PrivateKeyAggregateInsecure insecurely aggregates multiple private keys into
// one.
func PrivateKeyAggregateInsecure(privateKeys []*PrivateKey) (*PrivateKey, error) {
	// Get a C pointer to an array of private keys
	cPrivKeyArrPtr := C.AllocPtrArray(C.size_t(len(privateKeys)))
	defer C.FreePtrArray(cPrivKeyArrPtr)
	// Loop thru each key and add the key C ptr to the array of ptrs at index
	for i, privKey := range privateKeys {
		C.SetPtrArray(cPrivKeyArrPtr, unsafe.Pointer(privKey.sk), C.int(i))
	}

	var sk PrivateKey
	var cDidErr C.bool
	sk.sk = C.CPrivateKeyAggregateInsecure(cPrivKeyArrPtr, C.size_t(len(privateKeys)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sk, func(p *PrivateKey) { p.Free() })
	runtime.KeepAlive(privateKeys)

	return &sk, nil
}

// PrivateKeyAggregate securely aggregates multiple private keys into one by
// exponentiating the keys with the pubKey hashes first
func PrivateKeyAggregate(privateKeys []*PrivateKey, publicKeys []*PublicKey) (*PrivateKey, error) {
	// Get a C pointer to an array of private keys
	cPrivKeyArrPtr := C.AllocPtrArray(C.size_t(len(privateKeys)))
	defer C.FreePtrArray(cPrivKeyArrPtr)
	// Loop thru each key and add the key C ptr to the array of ptrs at index
	for i, privKey := range privateKeys {
		C.SetPtrArray(cPrivKeyArrPtr, unsafe.Pointer(privKey.sk), C.int(i))
	}

	// Get a C pointer to an array of public keys
	cPubKeyArrPtr := C.AllocPtrArray(C.size_t(len(publicKeys)))
	defer C.FreePtrArray(cPubKeyArrPtr)
	// Loop thru each key and add the key C ptr to the array of ptrs at index
	for i, pubKey := range publicKeys {
		C.SetPtrArray(cPubKeyArrPtr, unsafe.Pointer(pubKey.pk), C.int(i))
	}

	var cDidErr C.bool
	var sk PrivateKey
	sk.sk = C.CPrivateKeyAggregate(cPrivKeyArrPtr, C.size_t(len(privateKeys)),
		cPubKeyArrPtr, C.size_t(len(publicKeys)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sk, func(p *PrivateKey) { p.Free() })
	runtime.KeepAlive(privateKeys)
	runtime.KeepAlive(publicKeys)

	return &sk, nil
}

// PrivateKeyShare returns the secret key share for the given id which is the evaluation
// of the polynomial defined by privateKeys
func PrivateKeyShare(privateKeys []*PrivateKey, id Hash) (*PrivateKey, error) {
	// Get a C pointer to an array of private keys
	cPrivateKeyArrPtr := C.AllocPtrArray(C.size_t(len(privateKeys)))
	defer C.FreePtrArray(cPrivateKeyArrPtr)
	// Loop thru each key and add the key C ptr to the array of pointers at index i
	for i, privateKey := range privateKeys {
		C.SetPtrArray(cPrivateKeyArrPtr, unsafe.Pointer(privateKey.sk), C.int(i))
	}

	cIDPtr := C.CBytes(id[:])
	defer C.free(cIDPtr)

	var cDidErr C.bool
	var sk PrivateKey
	sk.sk = C.CPrivateKeyShare(cPrivateKeyArrPtr, C.size_t(len(privateKeys)), cIDPtr, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sk, func(p *PrivateKey) { p.Free() })
	runtime.KeepAlive(privateKeys)

	return &sk, nil
}

// PrivateKeyRecover try to recover a PrivateKey
func PrivateKeyRecover(privateKeys []*PrivateKey, ids []Hash) (*PrivateKey, error) {

	if len(privateKeys) != len(ids) {
		return nil, errors.New("number of private keys must match the number of ids")
	}

	cKeysArrPtr := C.AllocPtrArray(C.size_t(len(privateKeys)))
	defer C.FreePtrArray(cKeysArrPtr)
	for i, privateKey := range privateKeys {
		C.SetPtrArray(cKeysArrPtr, unsafe.Pointer(privateKey.sk), C.int(i))
	}

	cIdsArrPtr := C.AllocPtrArray(C.size_t(len(ids)))
	defer C.FreePtrArray(cIdsArrPtr)
	for i, id := range ids {
		cIDPtr := C.CBytes(id[:])
		C.SetPtrArray(cIdsArrPtr, cIDPtr, C.int(i))
	}

	var recoveredPrivateKey PrivateKey
	var cDidErr C.bool
	recoveredPrivateKey.sk = C.CPrivateKeyRecover(cKeysArrPtr, cIdsArrPtr, C.size_t(len(privateKeys)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	for i := range ids {
		C.free(C.GetPtrAtIndex(cIdsArrPtr, C.int(i)))
	}

	runtime.SetFinalizer(&recoveredPrivateKey, func(p *PrivateKey) { p.Free() })

	return &recoveredPrivateKey, nil
}

// Equal tests if one PrivateKey object is equal to another
func (sk *PrivateKey) Equal(other *PrivateKey) bool {
	isEqual := bool(C.CPrivateKeyIsEqual(sk.sk, other.sk))
	runtime.KeepAlive(sk)
	runtime.KeepAlive(other)
	return isEqual
}
