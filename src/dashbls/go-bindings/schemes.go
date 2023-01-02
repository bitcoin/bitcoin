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
// #include <stdbool.h>
// #include <stdlib.h>
// #include "schemes.h"
// #include "privatekey.h"
// #include "elements.h"
// #include "blschia.h"
import "C"
import (
	"runtime"
	"unsafe"
)

// Signer is a signer interface
type Signer interface {
	Sign(sk *PrivateKey, msg []byte) *G2Element
}

// Generator is the interface that wraps the method needed to generate a private key
type Generator interface {
	KeyGen(seed []byte) (*PrivateKey, error)
}

// Verifier is the interface that wraps the methods needed to verifications
// the interface contains the method for direct verification
// and aggregation with verification
type Verifier interface {
	Verify(pk *G1Element, msg []byte, sig *G2Element) bool
	AggregateVerify(pks []*G1Element, msgs [][]byte, sig *G2Element) bool
}

// Aggregator is the interface that's described methods for aggregation public (G1) and private (g2) keys
type Aggregator interface {
	AggregatePubKeys(pks ...*G1Element) *G1Element
	AggregateSigs(sigs ...*G2Element) *G2Element
}

// Deriver is an interface for describing some methods for the derivation of children
type Deriver interface {
	DeriveChildSk(sk *PrivateKey, index int) *PrivateKey
	DeriveChildSkUnhardened(sk *PrivateKey, index int) *PrivateKey
	DeriveChildPkUnhardened(el *G1Element, index int) *G1Element
}

// Scheme is a schema interface
type Scheme interface {
	Signer
	Generator
	Verifier
	Aggregator
	Deriver
}

type coreMPL struct {
	val C.CCoreMPL
}

// KeyGen returns a new generated PrivateKey using passed a genSeed data
// this method is a binding of bls::CoreMPL::KeyGen
func (s *coreMPL) KeyGen(seed []byte) (*PrivateKey, error) {
	cSeedPtr := C.CBytes(seed)
	defer C.free(cSeedPtr)
	var cDidErr C.bool
	sk := PrivateKey{
		val: C.CCoreMPLKeyGen(s.val, cSeedPtr, C.size_t(len(seed)), &cDidErr),
	}
	if cDidErr {
		return nil, errFromC()
	}
	runtime.SetFinalizer(&sk, func(sk *PrivateKey) { sk.free() })
	runtime.KeepAlive(s)
	return &sk, nil
}

// SkToG1 converts PrivateKey into G1Element (public key)
// this method is a binding of bls::CoreMPL::SkToG1
func (s *coreMPL) SkToG1(sk *PrivateKey) *G1Element {
	el := &G1Element{
		val: C.CCoreMPSkToG1(s.val, sk.val),
	}
	runtime.KeepAlive(s)
	runtime.KeepAlive(sk)
	return el
}

// Sign signs a message using a PrivateKey and returns the G2Element as a signature
// this method is a binding of bls::CoreMPL::Sign
func (s *coreMPL) Sign(sk *PrivateKey, msg []byte) *G2Element {
	cMsgPtr := C.CBytes(msg)
	defer C.free(cMsgPtr)
	sig := G2Element{
		val: C.CCoreMPLSign(s.val, sk.val, cMsgPtr, C.size_t(len(msg))),
	}
	runtime.SetFinalizer(&sig, func(sig *G2Element) { sig.free() })
	runtime.KeepAlive(s)
	runtime.KeepAlive(sk)
	return &sig
}

// Verify verifies a signature for a message with a G1Element as a public key
// this method is a binding of bls::CoreMPL::Verify
func (s *coreMPL) Verify(pk *G1Element, msg []byte, sig *G2Element) bool {
	cMsgPtr := C.CBytes(msg)
	defer C.free(cMsgPtr)
	isVerified := bool(C.CCoreMPLVerify(s.val, pk.val, cMsgPtr, C.size_t(len(msg)), sig.val))
	runtime.KeepAlive(s)
	runtime.KeepAlive(pk)
	runtime.KeepAlive(sig)
	return isVerified
}

// AggregatePubKeys returns a new G1Element (public key) as an aggregated public keys
// this method is a binding of bls::CoreMPL::AggregatePubKeys
func (s *coreMPL) AggregatePubKeys(pks ...*G1Element) *G1Element {
	cPkArrPtr := cAllocPubKeys(pks...)
	defer C.FreePtrArray(cPkArrPtr)
	aggSig := G1Element{
		val: C.CCoreMPLAggregatePubKeys(s.val, cPkArrPtr, C.size_t(len(pks))),
	}
	runtime.SetFinalizer(&aggSig, func(aggSig *G1Element) { aggSig.free() })
	runtime.KeepAlive(s)
	runtime.KeepAlive(pks)
	return &aggSig
}

// AggregateSigs returns a new G1Element (aggregated public keys)
// as a result of the aggregation of the passed public keys
// this method is a binding of bls::CoreMPL::AggregateSigs
func (s *coreMPL) AggregateSigs(sigs ...*G2Element) *G2Element {
	cSigArrPtr := cAllocSigs(sigs...)
	defer C.FreePtrArray(cSigArrPtr)
	aggSig := G2Element{
		val: C.CCoreMPLAggregateSigs(s.val, cSigArrPtr, C.size_t(len(sigs))),
	}
	runtime.SetFinalizer(&aggSig, func(aggSig *G2Element) { aggSig.free() })
	runtime.KeepAlive(s)
	runtime.KeepAlive(sigs)
	return &aggSig
}

// DeriveChildSk returns a child PrivateKey using the passed as a master key
// this method is a binding of bls::CoreMPL::DeriveChildSk
func (s *coreMPL) DeriveChildSk(sk *PrivateKey, index int) *PrivateKey {
	res := PrivateKey{
		val: C.CCoreMPLDeriveChildSk(s.val, sk.val, C.uint32_t(index)),
	}
	runtime.SetFinalizer(&res, func(res *PrivateKey) { res.free() })
	runtime.KeepAlive(s)
	runtime.KeepAlive(sk)
	return &res
}

// DeriveChildSkUnhardened returns a child PrivateKey of the passed master PrivateKey
// this method is a binding of bls::CoreMPL::DeriveChildSkUnhardened
func (s *coreMPL) DeriveChildSkUnhardened(sk *PrivateKey, index int) *PrivateKey {
	res := PrivateKey{
		val: C.CCoreMPLDeriveChildSkUnhardened(s.val, sk.val, C.uint32_t(index)),
	}
	runtime.SetFinalizer(&res, func(res *PrivateKey) { res.free() })
	runtime.KeepAlive(s)
	runtime.KeepAlive(sk)
	return &res
}

// DeriveChildPkUnhardened returns a child G1Element of the passed master G1Element
// this method is a binding of bls::CoreMPL::DeriveChildPkUnhardened
func (s *coreMPL) DeriveChildPkUnhardened(el *G1Element, index int) *G1Element {
	res := G1Element{
		val: C.CCoreMPLDeriveChildPkUnhardened(s.val, el.val, C.uint32_t(index)),
	}
	runtime.SetFinalizer(&res, func(res *G1Element) { res.free() })
	runtime.KeepAlive(s)
	runtime.KeepAlive(el)
	return &res
}

// AggregateVerify verifies the aggregated signature for a list of messages with public keys
// returns true if the signature is a valid otherwise returns false
// this method is a binding of bls::CoreMPL::AggregateVerify
func (s *coreMPL) AggregateVerify(pks []*G1Element, msgs [][]byte, sig *G2Element) bool {
	cPkArrPtr := cAllocPubKeys(pks...)
	defer C.FreePtrArray(cPkArrPtr)
	cMsgArrPtr, msgLens := cAllocMsgs(msgs)
	defer C.FreePtrArray(cMsgArrPtr)
	val := C.CCoreMPLAggregateVerify(
		s.val,
		cPkArrPtr,
		C.size_t(len(pks)),
		cMsgArrPtr,
		unsafe.Pointer(&msgLens[0]),
		C.size_t(len(msgs)),
		sig.val,
	)
	runtime.KeepAlive(s)
	runtime.KeepAlive(pks)
	runtime.KeepAlive(sig)
	return bool(val)
}

// BasicSchemeMPL represents bls::BasicSchemeMPL (basic scheme using minimum public key sizes)
type BasicSchemeMPL struct {
	coreMPL
}

// NewBasicSchemeMPL returns a new BasicSchemeMPL
// this method allocates the bls::BasicSchemeMPL object and keeps its pointer
func NewBasicSchemeMPL() *BasicSchemeMPL {
	scheme := BasicSchemeMPL{
		coreMPL{
			val: C.NewCBasicSchemeMPL(),
		},
	}
	runtime.SetFinalizer(&scheme, func(scheme *BasicSchemeMPL) { scheme.free() })
	return &scheme
}

// AggregateVerify verifies the aggregated signature for a list of messages with public keys
// this method is a binding of bls::BasicSchemeMPL::AggregateVerify
func (s *BasicSchemeMPL) AggregateVerify(pks []*G1Element, msgs [][]byte, sig *G2Element) bool {
	cPkArrPtr := cAllocPubKeys(pks...)
	defer C.FreePtrArray(cPkArrPtr)
	cMsgArrPtr, msgLens := cAllocMsgs(msgs)
	defer C.FreePtrArray(cMsgArrPtr)
	val := C.CBasicSchemeMPLAggregateVerify(
		s.val,
		cPkArrPtr,
		C.size_t(len(pks)),
		cMsgArrPtr,
		unsafe.Pointer(&msgLens[0]),
		C.size_t(len(msgs)),
		sig.val,
	)
	runtime.KeepAlive(s)
	runtime.KeepAlive(pks)
	runtime.KeepAlive(sig)
	return bool(val)
}

// free calls CPopSchemeMPLFree "C" function to release a memory allocated for bls::BasicSchemeMPL
func (s *BasicSchemeMPL) free() {
	C.CBasicSchemeMPLFree(s.val)
}

// AugSchemeMPL represents bls::AugSchemeMPL
// augmented should be enough for most use cases
type AugSchemeMPL struct {
	coreMPL
}

// NewAugSchemeMPL returns a new AugSchemeMPL
// this method allocates the bls::AugSchemeMPL object and keeps its pointer
func NewAugSchemeMPL() *AugSchemeMPL {
	scheme := AugSchemeMPL{
		coreMPL: coreMPL{
			val: C.NewCAugSchemeMPL(),
		},
	}
	runtime.SetFinalizer(&scheme, func(scheme *AugSchemeMPL) { scheme.free() })
	return &scheme
}

// Sign signs a message with a PrivateKey
// this method is a binding of bls::AugSchemeMPL::Sign
func (s *AugSchemeMPL) Sign(sk *PrivateKey, msg []byte) *G2Element {
	cMsgPtr := C.CBytes(msg)
	defer C.free(cMsgPtr)
	sig := G2Element{
		val: C.CAugSchemeMPLSign(s.val, sk.val, cMsgPtr, C.size_t(len(msg))),
	}
	runtime.SetFinalizer(&sig, func(sig *G2Element) { sig.free() })
	runtime.KeepAlive(s)
	runtime.KeepAlive(sk)
	return &sig
}

// SignPrepend is used for prepending different message
// this method is a binding of bls::AugSchemeMPL::Sign
func (s *AugSchemeMPL) SignPrepend(sk *PrivateKey, msg []byte, prepPk *G1Element) *G2Element {
	sig := G2Element{
		val: C.CAugSchemeMPLSignPrepend(s.val, sk.val, C.CBytes(msg), C.size_t(len(msg)), prepPk.val),
	}
	runtime.SetFinalizer(&sig, func(sig *G2Element) { sig.free() })
	runtime.KeepAlive(s)
	runtime.KeepAlive(sk)
	runtime.KeepAlive(prepPk)
	return &sig
}

// Verify verifies a G2Element (signature) for a message with a G1Element (public key)
// this method is a binding of bls::AugSchemeMPL::Verify
func (s *AugSchemeMPL) Verify(pk *G1Element, msg []byte, sig *G2Element) bool {
	cMsgPtr := C.CBytes(msg)
	defer C.free(cMsgPtr)
	val := C.CAugSchemeMPLVerify(s.val, pk.val, cMsgPtr, C.size_t(len(msg)), sig.val)
	runtime.KeepAlive(s)
	runtime.KeepAlive(pk)
	runtime.KeepAlive(sig)
	return bool(val)
}

// AggregateVerify verifies the aggregated signature for a list of messages with public keys
// this method is a binding of bls::AugSchemeMPL::AggregateVerify
func (s *AugSchemeMPL) AggregateVerify(pks []*G1Element, msgs [][]byte, sig *G2Element) bool {
	cPkArrPtr := cAllocPubKeys(pks...)
	defer C.FreePtrArray(cPkArrPtr)
	cMsgArrPtr, msgLens := cAllocMsgs(msgs)
	defer C.FreePtrArray(cMsgArrPtr)
	val := C.CAugSchemeMPLAggregateVerify(
		s.val,
		cPkArrPtr,
		C.size_t(len(pks)),
		cMsgArrPtr,
		unsafe.Pointer(&msgLens[0]),
		C.size_t(len(msgs)),
		sig.val,
	)
	runtime.KeepAlive(s)
	runtime.KeepAlive(pks)
	runtime.KeepAlive(sig)
	return bool(val)
}

// free calls CAugSchemeMPLFree "C" function to release a memory allocated for bls::AugSchemeMPL
func (s *AugSchemeMPL) free() {
	C.CAugSchemeMPLFree(s.val)
}

// PopSchemeMPL represents bls::PopSchemeMPL (proof of possession scheme)
// proof of possession can be used where verification must be fast
type PopSchemeMPL struct {
	coreMPL
}

// NewPopSchemeMPL returns a new bls::PopSchemeMPL
// this method allocates the new bls::PopSchemeMPL object and keeps its pointer
func NewPopSchemeMPL() *PopSchemeMPL {
	scheme := PopSchemeMPL{
		coreMPL: coreMPL{
			val: C.NewCPopSchemeMPL(),
		},
	}
	runtime.SetFinalizer(&scheme, func(scheme *PopSchemeMPL) { scheme.free() })
	return &scheme
}

// PopProve proves using the PrivateKey
// this method is a binding of bls::PopSchemeMPL::PopProve
func (s *PopSchemeMPL) PopProve(sk *PrivateKey) *G2Element {
	sig := G2Element{
		val: C.CPopSchemeMPLPopProve(s.val, sk.val),
	}
	runtime.SetFinalizer(&sig, func(sig *G2Element) { sig.free() })
	runtime.KeepAlive(s)
	runtime.KeepAlive(sk)
	return &sig
}

// PopVerify verifies of a signature using proof of possession
// this method is a binding of bls::PopSchemeMPL::PopVerify
func (s *PopSchemeMPL) PopVerify(pk *G1Element, sig *G2Element) bool {
	isVerified := bool(C.CPopSchemeMPLPopVerify(s.val, pk.val, sig.val))
	runtime.KeepAlive(s)
	runtime.KeepAlive(pk)
	runtime.KeepAlive(sig)
	return isVerified
}

// FastAggregateVerify uses for a fast verification
// this method is a binding of bls::PopSchemeMPL::FastAggregateVerify
func (s *PopSchemeMPL) FastAggregateVerify(pks []*G1Element, msg []byte, sig *G2Element) bool {
	msgPtr := C.CBytes(msg)
	cPkArrPtr := cAllocPubKeys(pks...)
	defer C.FreePtrArray(cPkArrPtr)
	isVerified := C.CPopSchemeMPLFastAggregateVerify(
		s.val,
		cPkArrPtr,
		C.size_t(len(pks)),
		msgPtr,
		C.size_t(len(msg)),
		sig.val,
	)
	runtime.KeepAlive(s)
	runtime.KeepAlive(pks)
	runtime.KeepAlive(sig)
	return bool(isVerified)
}

// free calls CPopSchemeMPLFree "C" function to release a memory allocated for bls::PopSchemeMPL
func (s *PopSchemeMPL) free() {
	C.CPopSchemeMPLFree(s.val)
}

func cAllocPubKeys(pks ...*G1Element) *unsafe.Pointer {
	arr := C.AllocPtrArray(C.size_t(len(pks)))
	for i, pk := range pks {
		C.SetPtrArray(arr, unsafe.Pointer(pk.val), C.int(i))
	}
	return arr
}
