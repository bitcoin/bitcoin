package blschia

// #cgo LDFLAGS: -lchiabls -lstdc++ -lgmp
// #cgo CXXFLAGS: -std=c++14
// #include <stdbool.h>
// #include <stdlib.h>
// #include "publickey.h"
// #include "blschia.h"
import "C"
import (
	"encoding/hex"
	"errors"
	"runtime"
	"unsafe"
)

// PublicKey represents a BLS public key
type PublicKey struct {
	pk C.CPublicKey
}

// PublicKeyFromBytes constructs a new public key from bytes
func PublicKeyFromBytes(data []byte) (*PublicKey, error) {
	// Get a C pointer to bytes
	cBytesPtr := C.CBytes(data)
	defer C.free(cBytesPtr)

	var pk PublicKey
	var cDidErr C.bool
	pk.pk = C.CPublicKeyFromBytes(cBytesPtr, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&pk, func(p *PublicKey) { p.Free() })
	return &pk, nil
}

// PublicKeyFromString constructs a new public key from hex string
func PublicKeyFromString(hexString string) (*PublicKey, error) {
	bytes, err := hex.DecodeString(hexString)
	if err != nil {
		return nil, err
	}
	return PublicKeyFromBytes(bytes)
}

// Free releases memory allocated by the key
func (pk *PublicKey) Free() {
	C.CPublicKeyFree(pk.pk)
	runtime.KeepAlive(pk)
}

// Serialize returns the byte representation of the public key
func (pk *PublicKey) Serialize() []byte {
	ptr := C.CPublicKeySerialize(pk.pk)
	defer C.free(ptr)
	runtime.KeepAlive(pk)
	return C.GoBytes(ptr, C.CPublicKeySizeBytes())
}

// Fingerprint returns the first 4 bytes of the serialized key
func (pk *PublicKey) Fingerprint() uint32 {
	fingerprint := uint32(C.CPublicKeyGetFingerprint(pk.pk))
	runtime.KeepAlive(pk)
	return fingerprint
}

// PublicKeyAggregate securely aggregates multiple public keys into one by
// exponentiating the keys with the pubKey hashes first
func PublicKeyAggregate(keys []*PublicKey) (*PublicKey, error) {
	// Get a C pointer to an array of public keys
	cPublicKeyArrayPtr := C.AllocPtrArray(C.size_t(len(keys)))
	defer C.FreePtrArray(cPublicKeyArrayPtr)

	// Loop thru each key and add the key C ptr to the array of ptrs at index
	for i, k := range keys {
		C.SetPtrArray(cPublicKeyArrayPtr, unsafe.Pointer(k.pk), C.int(i))
	}

	var key PublicKey
	var cDidErr C.bool
	key.pk = C.CPublicKeyAggregate(cPublicKeyArrayPtr, C.size_t(len(keys)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&key, func(p *PublicKey) { p.Free() })
	runtime.KeepAlive(keys)

	return &key, nil
}

// PublicKeyAggregateInsecure insecurely aggregates multiple public keys into
// one
func PublicKeyAggregateInsecure(keys []*PublicKey) (*PublicKey, error) {
	// Get a C pointer to an array of public keys
	cPublicKeyArrayPtr := C.AllocPtrArray(C.size_t(len(keys)))
	defer C.FreePtrArray(cPublicKeyArrayPtr)

	// Loop thru each key and add the key C ptr to the array of ptrs at index
	for i, k := range keys {
		C.SetPtrArray(cPublicKeyArrayPtr, unsafe.Pointer(k.pk), C.int(i))
	}

	var key PublicKey
	var cDidErr C.bool
	key.pk = C.CPublicKeyAggregateInsecure(cPublicKeyArrayPtr, C.size_t(len(keys)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&key, func(p *PublicKey) { p.Free() })
	runtime.KeepAlive(keys)

	return &key, nil
}

// PublicKeyShare returns the public key share for the given id which is the evaluation
// of the polynomial defined by publicKeys
func PublicKeyShare(publicKeys []*PublicKey, id Hash) (*PublicKey, error) {
	// Get a C pointer to an array of private keys
	cPublicKeyArrPtr := C.AllocPtrArray(C.size_t(len(publicKeys)))
	defer C.FreePtrArray(cPublicKeyArrPtr)
	// Loop thru each key and add the key C ptr to the array of pointers at index i
	for i, publicKey := range publicKeys {
		C.SetPtrArray(cPublicKeyArrPtr, unsafe.Pointer(publicKey.pk), C.int(i))
	}

	cIDPtr := C.CBytes(id[:])
	defer C.free(cIDPtr)

	var cDidErr C.bool
	var pk PublicKey
	pk.pk = C.CPublicKeyShare(cPublicKeyArrPtr, C.size_t(len(publicKeys)), cIDPtr, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&pk, func(p *PublicKey) { p.Free() })
	runtime.KeepAlive(publicKeys)

	return &pk, nil
}

// PublicKeyRecover try to recover a PublicKey
func PublicKeyRecover(publicKeys []*PublicKey, ids []Hash) (*PublicKey, error) {

	if len(publicKeys) != len(ids) {
		return nil, errors.New("number of public keys must match the number of ids")
	}

	cKeyArrPtr := C.AllocPtrArray(C.size_t(len(publicKeys)))
	defer C.FreePtrArray(cKeyArrPtr)
	for i, publicKey := range publicKeys {
		C.SetPtrArray(cKeyArrPtr, unsafe.Pointer(publicKey.pk), C.int(i))
	}

	cIdsArrPtr := C.AllocPtrArray(C.size_t(len(ids)))
	defer C.FreePtrArray(cIdsArrPtr)
	for i, id := range ids {
		cIDPtr := C.CBytes(id[:])
		C.SetPtrArray(cIdsArrPtr, cIDPtr, C.int(i))
	}

	var recoveredPublicKey PublicKey
	var cDidErr C.bool
	recoveredPublicKey.pk = C.CPublicKeyRecover(cKeyArrPtr, cIdsArrPtr, C.size_t(len(publicKeys)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	for i := range ids {
		C.free(C.GetPtrAtIndex(cIdsArrPtr, C.int(i)))
	}

	runtime.SetFinalizer(&recoveredPublicKey, func(p *PublicKey) { p.Free() })

	return &recoveredPublicKey, nil
}

// Equal tests if one PublicKey object is equal to another
func (pk *PublicKey) Equal(other *PublicKey) bool {
	isEqual := bool(C.CPublicKeyIsEqual(pk.pk, other.pk))
	runtime.KeepAlive(pk)
	runtime.KeepAlive(other)
	return isEqual
}
