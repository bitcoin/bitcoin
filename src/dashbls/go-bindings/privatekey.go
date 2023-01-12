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
// #include "privatekey.h"
// #include "blschia.h"
import "C"
import (
	"encoding/hex"
	"runtime"
)

// PrivateKey represents a bls::PrivateKey (32 byte integer)
type PrivateKey struct {
	val C.CPrivateKey
}

// PrivateKeyFromBytes returns a new PrivateKey from bytes
// this method allocates the new bls::PrivateKey object and keeps its pointer
func PrivateKeyFromBytes(data []byte, modOrder bool) (*PrivateKey, error) {
	cBytesPtr := cAllocBytes(data)
	defer C.SecFree(cBytesPtr)
	var cDidErr C.bool
	sk := PrivateKey{
		val: C.CPrivateKeyFromBytes(cBytesPtr, C.bool(modOrder), &cDidErr),
	}
	if bool(cDidErr) {
		return nil, errFromC()
	}
	runtime.SetFinalizer(&sk, func(p *PrivateKey) { p.free() })
	return &sk, nil
}

// G1Element returns a G1Element (public key) using a state of the current private key
// this method is a binding of the bls::PrivateKey::G1Element
func (sk *PrivateKey) G1Element() (*G1Element, error) {
	var cDidErr C.bool
	el := G1Element{
		val: C.CPrivateKeyGetG1Element(sk.val, &cDidErr),
	}
	if bool(cDidErr) {
		return nil, errFromC()
	}
	runtime.SetFinalizer(&el, func(el *G1Element) { el.free() })
	runtime.KeepAlive(sk)
	return &el, nil
}

// G2Element returns a G2Element (signature) using a state of the current private key
// this method is a binding of the bls::PrivateKey::G2Element
func (sk *PrivateKey) G2Element() (*G2Element, error) {
	var cDidErr C.bool
	el := G2Element{
		val: C.CPrivateKeyGetG2Element(sk.val, &cDidErr),
	}
	runtime.SetFinalizer(&el, func(el *G2Element) { el.free() })
	if bool(cDidErr) {
		return nil, errFromC()
	}
	runtime.KeepAlive(sk)
	return &el, nil
}

// G2Power returns a power of G2Element (signature)
// this method is a binding of the bls::PrivateKey::G2Power
func (sk *PrivateKey) G2Power(el *G2Element) *G2Element {
	sig := G2Element{
		val: C.CPrivateKeyGetG2Power(sk.val, el.val),
	}
	runtime.SetFinalizer(&sig, func() { sig.free() })
	runtime.KeepAlive(sk)
	runtime.KeepAlive(el)
	return &sig
}

// Serialize returns the byte representation of the private key
// this method is a binding of the bls::PrivateKey::Serialize
func (sk *PrivateKey) Serialize() []byte {
	ptr := C.CPrivateKeySerialize(sk.val)
	defer C.SecFree(ptr)
	bytes := C.GoBytes(ptr, C.int(C.CPrivateKeySizeBytes()))
	runtime.KeepAlive(sk)
	return bytes
}

// PrivateKeyAggregate securely aggregates multiple private keys into one
// this method is a binding of the bls::PrivateKey::Aggregate
func PrivateKeyAggregate(sks ...*PrivateKey) *PrivateKey {
	cPrivKeyArrPtr := cAllocPrivKeys(sks...)
	defer C.FreePtrArray(cPrivKeyArrPtr)
	sk := PrivateKey{
		val: C.CPrivateKeyAggregate(cPrivKeyArrPtr, C.size_t(len(sks))),
	}
	runtime.SetFinalizer(&sk, func(p *PrivateKey) { p.free() })
	runtime.KeepAlive(sks)
	return &sk
}

// EqualTo tests if one PrivateKey is equal to another
// this method is the binding of the equality operation
func (sk *PrivateKey) EqualTo(other *PrivateKey) bool {
	isEqual := bool(C.CPrivateKeyIsEqual(sk.val, other.val))
	runtime.KeepAlive(sk)
	runtime.KeepAlive(other)
	return isEqual
}

// HexString returns a hex string representation of serialized data
func (sk *PrivateKey) HexString() string {
	return hex.EncodeToString(sk.Serialize())
}

// free calls CPrivateKeyFree "C" function to release a memory allocated for bls::PrivateKey
func (sk *PrivateKey) free() {
	C.CPrivateKeyFree(sk.val)
}
