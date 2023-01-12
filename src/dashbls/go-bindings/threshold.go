// Copyright (c) 2021 The Dash Core developers

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package blschia

// #cgo LDFLAGS: -ldashbls -lrelic_s -lmimalloc-secure -lgmp
// #cgo CXXFLAGS: -std=c++14
// #include <stdlib.h>
// #include "threshold.h"
// #include "blschia.h"
import "C"
import (
	"crypto/sha256"
	"encoding/hex"
	"errors"
	"runtime"
	"unsafe"
)

// HashSize is the allowed size of a hash
const HashSize = sha256.Size

// Hash represents 32 byte of hash data
type Hash [HashSize]byte

var errWrongHexStringValue = errors.New("a hex string must contain 32 bytes")

// HashFromString convert a hex string into a Hash
func HashFromString(hexString string) (Hash, error) {
	var hash Hash
	data, err := hex.DecodeString(hexString)
	if err != nil {
		return hash, err
	}
	if len(data) < HashSize {
		return hash, errWrongHexStringValue
	}
	for i, d := range data[len(data)-HashSize:] {
		hash[HashSize-(i+1)] = d
	}
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

// ThresholdPrivateKeyShare retrieves a shared PrivateKey
// from a set of PrivateKey(s) and a hash
// this function is a binding of bls::Threshold::PrivateKeyShare
func ThresholdPrivateKeyShare(sks []*PrivateKey, hash Hash) (*PrivateKey, error) {
	cHashPtr := C.CBytes(hash[:])
	defer C.free(cHashPtr)
	cArrPtr := cAllocPrivKeys(sks...)
	defer C.FreePtrArray(cArrPtr)
	var cDidErr C.bool
	sk := PrivateKey{
		val: C.CThresholdPrivateKeyShare(cArrPtr, C.size_t(len(sks)), cHashPtr, &cDidErr),
	}
	if cDidErr {
		return nil, errFromC()
	}
	runtime.SetFinalizer(&sk, func(pk *PrivateKey) { sk.free() })
	runtime.KeepAlive(sks)
	return &sk, nil
}

// ThresholdPublicKeyShare retrieves a shared G1Element (public key)
// from a set of G1Element(s) and a hash
// this function is a binding of bls::Threshold::PublicKeyShare
func ThresholdPublicKeyShare(pks []*G1Element, hash Hash) (*G1Element, error) {
	cHashPtr := C.CBytes(hash[:])
	defer C.free(cHashPtr)
	cArrPtr := cAllocPubKeys(pks...)
	defer C.FreePtrArray(cArrPtr)
	var cDidErr C.bool
	pk := G1Element{
		val: C.CThresholdPublicKeyShare(cArrPtr, C.size_t(len(pks)), cHashPtr, &cDidErr),
	}
	if cDidErr {
		return nil, errFromC()
	}
	runtime.SetFinalizer(&pk, func(pk *G1Element) { pk.free() })
	runtime.KeepAlive(pks)
	return &pk, nil
}

// ThresholdSignatureShare retrieves a shared G2Element (signature)
// from a set of G2Element(s) and a hash
// this function is a binding of bls::Threshold::SignatureShare
func ThresholdSignatureShare(sigs []*G2Element, hash Hash) (*G2Element, error) {
	cHashPtr := C.CBytes(hash[:])
	defer C.free(cHashPtr)
	cArrPtr := cAllocSigs(sigs...)
	defer C.FreePtrArray(cArrPtr)
	var cDidErr C.bool
	sig := G2Element{
		val: C.CThresholdSignatureShare(cArrPtr, C.size_t(len(sigs)), cHashPtr, &cDidErr),
	}
	if cDidErr {
		return nil, errFromC()
	}
	runtime.SetFinalizer(&sig, func(pk *G2Element) { sig.free() })
	runtime.KeepAlive(sigs)
	return &sig, nil
}

// ThresholdPrivateKeyRecover recovers PrivateKey
// from the set of shared PrivateKey(s) with a list of the hashes
// this function is a binding of bls::Threshold::PrivateKeyRecover
func ThresholdPrivateKeyRecover(sks []*PrivateKey, hashes []Hash) (*PrivateKey, error) {
	cArrPtr := cAllocPrivKeys(sks...)
	defer C.FreePtrArray(cArrPtr)
	cHashArrPtr := cAllocHashes(hashes)
	defer C.FreePtrArray(cHashArrPtr)
	var cDidErr C.bool
	sk := PrivateKey{
		val: C.CThresholdPrivateKeyRecover(
			cArrPtr,
			C.size_t(len(sks)),
			cHashArrPtr,
			C.size_t(len(hashes)),
			&cDidErr,
		),
	}
	if cDidErr {
		return nil, errFromC()
	}
	runtime.SetFinalizer(&sk, func(sk *PrivateKey) { sk.free() })
	runtime.KeepAlive(sks)
	return &sk, nil
}

// ThresholdPublicKeyRecover recovers G1Element (public key)
// from the set of shared G1Element(s) with a list of the hashes
// this function is a binding of bls::Threshold::PublicKeyRecover
func ThresholdPublicKeyRecover(pks []*G1Element, hashes []Hash) (*G1Element, error) {
	cArrPtr := cAllocPubKeys(pks...)
	defer C.FreePtrArray(cArrPtr)
	cHashArrPtr := cAllocHashes(hashes)
	defer C.FreePtrArray(cHashArrPtr)
	var cDidErr C.bool
	pk := G1Element{
		val: C.CThresholdPublicKeyRecover(
			cArrPtr,
			C.size_t(len(pks)),
			cHashArrPtr,
			C.size_t(len(hashes)),
			&cDidErr,
		),
	}
	if cDidErr {
		return nil, errFromC()
	}
	runtime.SetFinalizer(&pk, func(pk *G1Element) { pk.free() })
	runtime.KeepAlive(pks)
	return &pk, nil
}

// ThresholdSignatureRecover recovers G2Element (signature)
// from the set of shared G2Element(s) with a list of the hashes
// this function is a binding of bls::Threshold::SignatureRecover
func ThresholdSignatureRecover(sigs []*G2Element, hashes []Hash) (*G2Element, error) {
	cArrPtr := cAllocSigs(sigs...)
	defer C.FreePtrArray(cArrPtr)
	cHashArrPtr := cAllocHashes(hashes)
	defer C.FreePtrArray(cHashArrPtr)
	var cDidErr C.bool
	sig := G2Element{
		val: C.CThresholdSignatureRecover(
			cArrPtr,
			C.size_t(len(sigs)),
			cHashArrPtr,
			C.size_t(len(hashes)),
			&cDidErr,
		),
	}
	if cDidErr {
		return nil, errFromC()
	}
	runtime.SetFinalizer(&sig, func(sig *G2Element) { sig.free() })
	runtime.KeepAlive(sigs)
	return &sig, nil
}

// ThresholdSign signs of the hash with PrivateKey
// this function is a binding of bls::Threshold::Sign
func ThresholdSign(sk *PrivateKey, hash Hash) *G2Element {
	cHashPtr := C.CBytes(hash[:])
	defer C.free(cHashPtr)
	sig := G2Element{
		val: C.CThresholdSign(sk.val, cHashPtr),
	}
	runtime.SetFinalizer(&sig, func(sig *G2Element) { sig.free() })
	runtime.KeepAlive(sk)
	return &sig
}

// ThresholdVerify verifies of the hash with G1Element (public key)
// this function is a binding of bls::Threshold::Verify
func ThresholdVerify(pk *G1Element, hash Hash, sig *G2Element) bool {
	cHashPtr := C.CBytes(hash[:])
	defer C.free(cHashPtr)
	val := C.CThresholdVerify(pk.val, cHashPtr, sig.val)
	runtime.KeepAlive(pk)
	runtime.KeepAlive(sig)
	return bool(val)
}

func cAllocHashes(hashes []Hash) *unsafe.Pointer {
	cArrPtr := C.AllocPtrArray(C.size_t(len(hashes)))
	for i, hash := range hashes {
		C.SetPtrArray(cArrPtr, unsafe.Pointer(C.CBytes(hash[:])), C.int(i))
	}
	return cArrPtr
}
