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
// #include "blschia.h"
// #include <string.h>
import "C"
import (
	"unsafe"
)

func cAllocBytes(data []byte) unsafe.Pointer {
	l := C.size_t(len(data))
	ptr := unsafe.Pointer(C.SecAllocBytes(l))
	C.memcpy(ptr, unsafe.Pointer(&data[0]), l)
	return ptr
}

func cAllocSigs(sigs ...*G2Element) *unsafe.Pointer {
	arr := C.AllocPtrArray(C.size_t(len(sigs)))
	for i, pk := range sigs {
		C.SetPtrArray(arr, unsafe.Pointer(pk.val), C.int(i))
	}
	return arr
}

func cAllocPrivKeys(sks ...*PrivateKey) *unsafe.Pointer {
	arr := C.AllocPtrArray(C.size_t(len(sks)))
	for i, sk := range sks {
		C.SetPtrArray(arr, unsafe.Pointer(sk.val), C.int(i))
	}
	return arr
}

func cAllocMsgs(msgs [][]byte) (*unsafe.Pointer, []int) {
	msgLens := make([]int, len(msgs))
	cMsgArrPtr := C.AllocPtrArray(C.size_t(len(msgs)))
	for i, msg := range msgs {
		cMsgPtr := C.CBytes(msg)
		C.SetPtrArray(cMsgArrPtr, unsafe.Pointer(cMsgPtr), C.int(i))
		msgLens[i] = len(msg)
	}
	return cMsgArrPtr, msgLens
}
