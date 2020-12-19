package blschia

// #cgo LDFLAGS: -lchiabls -lstdc++ -lgmp
// #cgo CXXFLAGS: -std=c++14
// #include <stdbool.h>
// #include <stdlib.h>
// #include "extendedprivatekey.h"
// #include "extendedpublickey.h"
// #include "publickey.h"
// #include "blschia.h"
import "C"
import (
	"encoding/hex"
	"errors"
	"runtime"
)

// ExtendedPrivateKey represents a BIP-32 style extended key, which is composed
// of a private key and a chain code.
type ExtendedPrivateKey struct {
	key C.CExtendedPrivateKey
}

// ExtendedPrivateKeyFromSeed generates a master private key and chain code
// from a seed
func ExtendedPrivateKeyFromSeed(seed []byte) (*ExtendedPrivateKey, error) {
	// Get a C pointer to bytes
	cBytesPtr := C.CBytes(seed)
	defer C.free(cBytesPtr)

	var key ExtendedPrivateKey
	var cDidErr C.bool
	key.key = C.CExtendedPrivateKeyFromSeed(cBytesPtr, C.size_t(len(seed)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&key, func(p *ExtendedPrivateKey) { p.Free() })

	return &key, nil
}

// ExtendedPrivateKeyFromBytes parses a private key and chain code from bytes
func ExtendedPrivateKeyFromBytes(data []byte) (*ExtendedPrivateKey, error) {
	// Get a C pointer to bytes
	cBytesPtr := C.CBytes(data)
	defer C.free(cBytesPtr)

	var key ExtendedPrivateKey
	var cDidErr C.bool
	key.key = C.CExtendedPrivateKeyFromBytes(cBytesPtr, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&key, func(p *ExtendedPrivateKey) { p.Free() })

	return &key, nil
}

// ExtendedPrivateKeyFromString constructs a new extended private key from hex string
func ExtendedPrivateKeyFromString(hexString string) (*ExtendedPrivateKey, error) {
	bytes, err := hex.DecodeString(hexString)
	if err != nil {
		return nil, err
	}
	return ExtendedPrivateKeyFromBytes(bytes)
}

// Free releases memory allocated by the key
func (key *ExtendedPrivateKey) Free() {
	C.CExtendedPrivateKeyFree(key.key)
	runtime.KeepAlive(key)
}

// Serialize returns the serialized byte representation of the
// ExtendedPrivateKey object
func (key *ExtendedPrivateKey) Serialize() []byte {
	ptr := C.CExtendedPrivateKeySerialize(key.key)
	defer C.SecFree(ptr)
	runtime.KeepAlive(key)
	return C.GoBytes(ptr, C.CExtendedPrivateKeySizeBytes())
}

// GetPublicKey returns the PublicKey which corresponds to the PrivateKey for
// the given node
func (key *ExtendedPrivateKey) GetPublicKey() *PublicKey {
	var pk PublicKey
	pk.pk = C.CExtendedPrivateKeyGetPublicKey(key.key)

	runtime.SetFinalizer(&pk, func(p *PublicKey) { p.Free() })
	runtime.KeepAlive(key)

	return &pk
}

// GetChainCode returns the ChainCode for the given node
func (key *ExtendedPrivateKey) GetChainCode() *ChainCode {
	var cc ChainCode
	cc.cc = C.CExtendedPrivateKeyGetChainCode(key.key)

	runtime.SetFinalizer(&cc, func(p *ChainCode) { p.Free() })
	runtime.KeepAlive(key)

	return &cc
}

// PrivateChild derives a child ExtendedPrivateKey
func (key *ExtendedPrivateKey) PrivateChild(i uint32) (*ExtendedPrivateKey, error) {

	var child ExtendedPrivateKey
	var cDidErr C.bool
	child.key = C.CExtendedPrivateKeyPrivateChild(key.key, C.uint(i), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&child, func(p *ExtendedPrivateKey) { p.Free() })
	runtime.KeepAlive(key)

	return &child, nil
}

// PublicChild derives a child ExtendedPublicKey
func (key *ExtendedPrivateKey) PublicChild(i uint32) (*ExtendedPublicKey, error) {

	var child ExtendedPublicKey
	var cDidErr C.bool
	child.key = C.CExtendedPrivateKeyPublicChild(key.key, C.uint(i), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&child, func(p *ExtendedPublicKey) { p.Free() })
	runtime.KeepAlive(key)

	return &child, nil
}

// GetExtendedPublicKey returns the extended public key which corresponds to
// the extended private key for the given node
func (key *ExtendedPrivateKey) GetExtendedPublicKey() *ExtendedPublicKey {
	var xpub ExtendedPublicKey
	xpub.key = C.CExtendedPrivateKeyGetExtendedPublicKey(key.key)

	runtime.SetFinalizer(&xpub, func(p *ExtendedPublicKey) { p.Free() })
	runtime.KeepAlive(key)

	return &xpub
}

// GetVersion returns the version bytes
func (key *ExtendedPrivateKey) GetVersion() uint32 {
	version := uint32(C.CExtendedPrivateKeyGetVersion(key.key))
	runtime.KeepAlive(key)
	return version
}

// GetDepth returns the depth byte
func (key *ExtendedPrivateKey) GetDepth() uint8 {
	depth := uint8(C.CExtendedPrivateKeyGetDepth(key.key))
	runtime.KeepAlive(key)
	return depth
}

// GetParentFingerprint returns the parent fingerprint
func (key *ExtendedPrivateKey) GetParentFingerprint() uint32 {
	parentFingerprint := uint32(C.CExtendedPrivateKeyGetParentFingerprint(key.key))
	runtime.KeepAlive(key)
	return parentFingerprint
}

// GetChildNumber returns the child number
func (key *ExtendedPrivateKey) GetChildNumber() uint32 {
	childNumber := uint32(C.CExtendedPrivateKeyGetChildNumber(key.key))
	runtime.KeepAlive(key)
	return childNumber
}

// GetPrivateKey returns the private key at the given node
func (key *ExtendedPrivateKey) GetPrivateKey() *PrivateKey {
	var sk PrivateKey
	sk.sk = C.CExtendedPrivateKeyGetPrivateKey(key.key)

	runtime.SetFinalizer(&sk, func(p *PrivateKey) { p.Free() })
	runtime.KeepAlive(key)

	return &sk
}

// Equal tests if one ExtendedPrivateKey object is equal to another
//
// Only the privatekey and chaincode material is tested
func (key *ExtendedPrivateKey) Equal(other *ExtendedPrivateKey) bool {
	isEqual := bool(C.CExtendedPrivateKeyIsEqual(key.key, other.key))
	runtime.KeepAlive(key)
	runtime.KeepAlive(other)
	return isEqual
}
