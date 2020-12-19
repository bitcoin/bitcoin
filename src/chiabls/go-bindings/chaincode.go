package blschia

// #cgo LDFLAGS: -lchiabls -lstdc++ -lgmp
// #cgo CXXFLAGS: -std=c++14
// #include <stdbool.h>
// #include <stdlib.h>
// #include "chaincode.h"
// #include "blschia.h"
import "C"
import (
	"errors"
	"runtime"
)

// ChainCode is used in extended keys to derive child keys
type ChainCode struct {
	cc C.CChainCode
}

// ChainCodeFromBytes creates an ChainCode object given a byte slice
func ChainCodeFromBytes(data []byte) (*ChainCode, error) {
	// Get a C pointer to bytes
	cBytesPtr := C.CBytes(data)
	defer C.free(cBytesPtr)

	var cc ChainCode
	var cDidErr C.bool
	cc.cc = C.CChainCodeFromBytes(cBytesPtr, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return nil, err
	}

	runtime.SetFinalizer(&cc, func(p *ChainCode) { p.Free() })

	return &cc, nil
}

// Serialize returns the serialized byte representation of the ChainCode object
func (cc *ChainCode) Serialize() []byte {
	ptr := C.CChainCodeSerialize(cc.cc)
	defer C.free(ptr)
	runtime.KeepAlive(cc)
	return C.GoBytes(ptr, C.CChainCodeSizeBytes())
}

// Free releases memory allocated by the ChainCode object
func (cc *ChainCode) Free() {
	C.CChainCodeFree(cc.cc)
	runtime.KeepAlive(cc)
}

// Equal tests if one ChainCode object is equal to another
func (cc *ChainCode) Equal(other *ChainCode) bool {
	isEqual := bool(C.CChainCodeIsEqual(cc.cc, other.cc))
	runtime.KeepAlive(cc)
	runtime.KeepAlive(other)
	return isEqual
}
