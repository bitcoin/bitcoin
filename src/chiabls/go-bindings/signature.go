package blschia

// #cgo LDFLAGS: -lchiabls -lstdc++ -lgmp
// #cgo CXXFLAGS: -std=c++14
// #include <stdbool.h>
// #include <stdlib.h>
// #include "signature.h"
// #include "blschia.h"
import "C"
import (
	"encoding/hex"
	"errors"
	"runtime"
	"unsafe"
)

// InsecureSignature represents an insecure BLS signature.
type InsecureSignature struct {
	sig C.CInsecureSignature
}

// Signature represents a BLS signature with aggregation info.
type Signature struct {
	sig C.CSignature
}

// InsecureSignatureFromBytes constructs a new insecure signature from bytes
func InsecureSignatureFromBytes(data []byte) (*InsecureSignature, error) {
	// Get a C pointer to bytes
	cBytesPtr := C.CBytes(data)
	defer C.free(cBytesPtr)

	var sig InsecureSignature
	var cDidErr C.bool
	sig.sig = C.CInsecureSignatureFromBytes(cBytesPtr, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sig, func(p *InsecureSignature) { p.Free() })
	return &sig, nil
}

// InsecureSignatureFromString constructs a new insecure signature from hex string
func InsecureSignatureFromString(hexString string) (*InsecureSignature, error) {
	bytes, err := hex.DecodeString(hexString)
	if err != nil {
		return nil, err
	}
	return InsecureSignatureFromBytes(bytes)
}

// Serialize returns the byte representation of the signature
func (sig *InsecureSignature) Serialize() []byte {
	ptr := C.CInsecureSignatureSerialize(sig.sig)
	defer C.free(ptr)
	runtime.KeepAlive(sig)
	return C.GoBytes(ptr, C.CInsecureSignatureSizeBytes())
}

// Free releases memory allocated by the signature
func (sig *InsecureSignature) Free() {
	C.CInsecureSignatureFree(sig.sig)
	runtime.KeepAlive(sig)
}

// SignatureFromBytes creates a new Signature object from the raw bytes
func SignatureFromBytes(data []byte) (*Signature, error) {
	// Get a C pointer to bytes
	cBytesPtr := C.CBytes(data)
	defer C.free(cBytesPtr)

	var sig Signature
	var cDidErr C.bool
	sig.sig = C.CSignatureFromBytes(cBytesPtr, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sig, func(p *Signature) { p.Free() })
	return &sig, nil
}

// Serialize returns the byte representation of the signature
func (sig *Signature) Serialize() []byte {
	ptr := C.CSignatureSerialize(sig.sig)
	defer C.free(ptr)
	runtime.KeepAlive(sig)
	return C.GoBytes(ptr, C.CSignatureSizeBytes())
}

// Free releases memory allocated by the signature
func (sig *Signature) Free() {
	C.CSignatureFree(sig.sig)
	runtime.KeepAlive(sig)
}

// Verify a single or aggregate signature
func (sig *Signature) Verify() bool {
	verifyResult := bool(C.CSignatureVerify(sig.sig))
	runtime.KeepAlive(sig)
	return verifyResult
}

// SetAggregationInfo sets the aggregation information on this signature, which
// describes how this signature was generated, and how it should be verified.
func (sig *Signature) SetAggregationInfo(ai *AggregationInfo) {
	C.CSignatureSetAggregationInfo(sig.sig, ai.ai)
	runtime.KeepAlive(sig)
	runtime.KeepAlive(ai)
}

// GetAggregationInfo returns the aggregation info on this signature.
func (sig *Signature) GetAggregationInfo() *AggregationInfo {
	var ai AggregationInfo
	ai.ai = C.CSignatureGetAggregationInfo(sig.sig)

	runtime.SetFinalizer(&ai, func(info *AggregationInfo) { info.Free() })
	runtime.KeepAlive(sig)

	return &ai
}

// SignatureAggregate aggregates many signatures using the secure aggregation
// method.
func SignatureAggregate(signatures []*Signature) (*Signature, error) {
	// Get a C pointer to an array of signatures
	cSigArrPtr := C.AllocPtrArray(C.size_t(len(signatures)))
	defer C.FreePtrArray(cSigArrPtr)

	// Loop thru each sig and add the pointer to it, to the C pointer array at
	// the given index.
	for i, sig := range signatures {
		C.SetPtrArray(cSigArrPtr, unsafe.Pointer(sig.sig), C.int(i))
	}

	var sig Signature
	var cDidErr C.bool
	sig.sig = C.CSignatureAggregate(cSigArrPtr, C.size_t(len(signatures)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sig, func(p *Signature) { p.Free() })
	runtime.KeepAlive(signatures)

	return &sig, nil
}

// DivideBy divides the aggregate signature (this) by a list of signatures.
//
// These divisors can be single or aggregate signatures, but all msg/pk pairs
// in these signatures must be distinct and unique.
func (sig *Signature) DivideBy(signatures []*Signature) (*Signature, error) {
	if len(signatures) == 0 {
		return sig, nil
	}

	// Get a C pointer to an array of signatures
	cSigArrPtr := C.AllocPtrArray(C.size_t(len(signatures)))
	defer C.FreePtrArray(cSigArrPtr)
	// Loop thru each sig and add the pointer to it, to the C pointer array at
	// the given index.
	for i, signature := range signatures {
		C.SetPtrArray(cSigArrPtr, unsafe.Pointer(signature.sig), C.int(i))
	}

	var quo Signature
	var cDidErr C.bool
	quo.sig = C.CSignatureDivideBy(sig.sig, cSigArrPtr, C.size_t(len(signatures)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&quo, func(p *Signature) { p.Free() })
	runtime.KeepAlive(sig)
	runtime.KeepAlive(signatures)

	return &quo, nil
}

// DivideBy insecurely divides signatures
func (sig *InsecureSignature) DivideBy(signatures []*InsecureSignature) (*InsecureSignature, error) {
	if len(signatures) == 0 {
		return sig, nil
	}

	// Get a C pointer to an array of signatures
	cSigArrPtr := C.AllocPtrArray(C.size_t(len(signatures)))
	defer C.FreePtrArray(cSigArrPtr)
	// Loop thru each sig and add the pointer to it, to the C pointer array at
	// the given index.
	for i, sig := range signatures {
		C.SetPtrArray(cSigArrPtr, unsafe.Pointer(sig.sig), C.int(i))
	}

	var quo InsecureSignature
	var cDidErr C.bool
	quo.sig = C.CInsecureSignatureDivideBy(sig.sig, cSigArrPtr, C.size_t(len(signatures)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&quo, func(p *InsecureSignature) { p.Free() })
	runtime.KeepAlive(sig)
	runtime.KeepAlive(signatures)

	return &quo, nil
}

// Verify a single or aggregate signature
//
// This verification method is insecure in regard to the rogue public key
// attack
func (sig *InsecureSignature) Verify(hashes [][]byte, publicKeys []*PublicKey) bool {
	if (len(hashes) != len(publicKeys)) || len(hashes) == 0 {
		// panic("hashes and pubKeys vectors must be of same size and non-empty")
		return false
	}

	// Get a C pointer to an array of message hashes
	cNumHashes := C.size_t(len(hashes))
	cHashesPtr := C.AllocPtrArray(cNumHashes)
	defer C.FreePtrArray(cHashesPtr)
	// Loop thru each message and add the key C ptr to the array of ptrs at index
	for i, hash := range hashes {
		cBytesPtr := C.CBytes(hash)
		C.SetPtrArray(cHashesPtr, cBytesPtr, C.int(i))
	}

	// Get a C pointer to an array of public keys
	cNumPublicKeys := C.size_t(len(publicKeys))
	cPublicKeysPtr := C.AllocPtrArray(cNumPublicKeys)
	defer C.FreePtrArray(cPublicKeysPtr)
	// Loop thru each key and add the key C ptr to the array of ptrs at index
	for i, key := range publicKeys {
		C.SetPtrArray(cPublicKeysPtr, unsafe.Pointer(key.pk), C.int(i))
	}

	verifyResult := bool(C.CInsecureSignatureVerify(sig.sig, cHashesPtr, cNumHashes, cPublicKeysPtr, cNumPublicKeys))

	for i := range hashes {
		C.free(C.GetPtrAtIndex(cHashesPtr, C.int(i)))
	}

	runtime.KeepAlive(sig)
	runtime.KeepAlive(publicKeys)

	return verifyResult
}

// InsecureSignatureAggregate insecurely aggregates signatures
func InsecureSignatureAggregate(signatures []*InsecureSignature) (*InsecureSignature, error) {
	// Get a C pointer to an array of signatures
	cSigArrPtr := C.AllocPtrArray(C.size_t(len(signatures)))
	defer C.FreePtrArray(cSigArrPtr)
	// Loop thru each sig and add the pointer to it, to the C pointer array at
	// the given index.
	for i, sig := range signatures {
		C.SetPtrArray(cSigArrPtr, unsafe.Pointer(sig.sig), C.int(i))
	}

	var sig InsecureSignature
	var cDidErr C.bool
	sig.sig = C.CInsecureSignatureAggregate(cSigArrPtr, C.size_t(len(signatures)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sig, func(p *InsecureSignature) { p.Free() })
	runtime.KeepAlive(signatures)

	return &sig, nil
}

// InsecureSignatureShare returns the signature share for the given id which is the evaluation
// of the polynomial defined by publicKeys
func InsecureSignatureShare(signatures []*InsecureSignature, id Hash) (*InsecureSignature, error) {
	// Get a C pointer to an array of private keys
	cSignatureArrPtr := C.AllocPtrArray(C.size_t(len(signatures)))
	defer C.FreePtrArray(cSignatureArrPtr)
	// Loop thru each key and add the key C ptr to the array of pointers at index i
	for i, signature := range signatures {
		C.SetPtrArray(cSignatureArrPtr, unsafe.Pointer(signature.sig), C.int(i))
	}

	cIDPtr := C.CBytes(id[:])
	defer C.free(cIDPtr)

	var cDidErr C.bool
	var sig InsecureSignature
	sig.sig = C.CInsecureSignatureShare(cSignatureArrPtr, C.size_t(len(signatures)), cIDPtr, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sig, func(p *InsecureSignature) { p.Free() })
	runtime.KeepAlive(signatures)

	return &sig, nil
}

// InsecureSignatureRecover try to recover a threshold signature
func InsecureSignatureRecover(signatures []*InsecureSignature, ids []Hash) (*InsecureSignature, error) {

	if len(signatures) != len(ids) {
		return nil, errors.New("number of signatures must match the number of ids")
	}

	cSigArrPtr := C.AllocPtrArray(C.size_t(len(signatures)))
	defer C.FreePtrArray(cSigArrPtr)
	for i, sig := range signatures {
		C.SetPtrArray(cSigArrPtr, unsafe.Pointer(sig.sig), C.int(i))
	}

	cIdsArrPtr := C.AllocPtrArray(C.size_t(len(ids)))
	defer C.FreePtrArray(cIdsArrPtr)
	for i, id := range ids {
		cIDPtr := C.CBytes(id[:])
		C.SetPtrArray(cIdsArrPtr, cIDPtr, C.int(i))
	}

	var recoveredSig InsecureSignature
	var cDidErr C.bool
	recoveredSig.sig = C.CInsecureSignatureRecover(cSigArrPtr, cIdsArrPtr, C.size_t(len(signatures)), &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	for i := range ids {
		C.free(C.GetPtrAtIndex(cIdsArrPtr, C.int(i)))
	}

	runtime.SetFinalizer(&recoveredSig, func(p *InsecureSignature) { p.Free() })

	return &recoveredSig, nil
}

// Equal tests if one InsecureSignature object is equal to another
func (sig *InsecureSignature) Equal(other *InsecureSignature) bool {
	isEqual := bool(C.CInsecureSignatureIsEqual(sig.sig, other.sig))
	runtime.KeepAlive(sig)
	runtime.KeepAlive(other)
	return isEqual
}

// Equal tests if one Signature object is equal to another
func (sig *Signature) Equal(other *Signature) bool {
	isEqual := bool(C.CSignatureIsEqual(sig.sig, other.sig))
	runtime.KeepAlive(sig)
	runtime.KeepAlive(other)
	return isEqual
}

// SignatureFromBytesWithAggregationInfo creates a new Signature object from
// the raw bytes and aggregation info
func SignatureFromBytesWithAggregationInfo(data []byte, ai *AggregationInfo) (*Signature, error) {
	// Get a C pointer to bytes
	cBytesPtr := C.CBytes(data)
	defer C.free(cBytesPtr)

	var sig Signature
	var cDidErr C.bool
	sig.sig = C.CSignatureFromBytesWithAggregationInfo(cBytesPtr, ai.ai, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&sig, func(p *Signature) { p.Free() })
	runtime.KeepAlive(ai)

	return &sig, nil
}

// SignatureFromInsecureSig constructs a signature from an insecure signature
// (but has no aggregation info)
func SignatureFromInsecureSig(isig *InsecureSignature) *Signature {
	var sig Signature
	sig.sig = C.CSignatureFromInsecureSig(isig.sig)

	runtime.SetFinalizer(&sig, func(p *Signature) { p.Free() })
	runtime.KeepAlive(isig)

	return &sig
}

// SignatureFromInsecureSigWithAggregationInfo constructs a secure signature
// from an insecure signature and aggregation info
func SignatureFromInsecureSigWithAggregationInfo(isig *InsecureSignature, ai *AggregationInfo) *Signature {
	var sig Signature
	sig.sig = C.CSignatureFromInsecureSigWithAggregationInfo(isig.sig, ai.ai)

	runtime.SetFinalizer(&sig, func(p *Signature) { p.Free() })
	runtime.KeepAlive(isig)
	runtime.KeepAlive(ai)

	return &sig
}

// GetInsecureSig returns an insecure signature from the secure variant
func (sig *Signature) GetInsecureSig() *InsecureSignature {
	var isig InsecureSignature
	isig.sig = C.CSignatureGetInsecureSig(sig.sig)

	runtime.SetFinalizer(&isig, func(p *InsecureSignature) { p.Free() })
	runtime.KeepAlive(sig)

	return &isig
}
