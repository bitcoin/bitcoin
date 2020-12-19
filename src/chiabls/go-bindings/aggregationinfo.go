package blschia

// #cgo LDFLAGS: -lchiabls -lstdc++ -lgmp
// #cgo CXXFLAGS: -std=c++14
// #include <stdbool.h>
// #include <stdlib.h>
// #include "blschia.h"
import "C"
import (
	"errors"
	"runtime"
	"unsafe"
)

// AggregationInfo represents information about how aggregation was performed,
// or how a signature was generated (pks, messageHashes, etc).
type AggregationInfo struct {
	ai C.CAggregationInfo
}

// AggregationInfoFromMsg creates an AggregationInfo object given a PublicKey
// and a message payload.
func AggregationInfoFromMsg(pk *PublicKey, message []byte) *AggregationInfo {
	// Get a C pointer to bytes
	cMessagePtr := C.CBytes(message)
	defer C.free(cMessagePtr)

	var ai AggregationInfo
	ai.ai = C.CAggregationInfoFromMsg(pk.pk, cMessagePtr, C.size_t(len(message)))

	runtime.SetFinalizer(&ai, func(p *AggregationInfo) { p.Free() })
	runtime.KeepAlive(pk)

	return &ai
}

// AggregationInfoFromMsgHash creates an AggregationInfo object given a
// PublicKey and a pre-hashed message payload.
func AggregationInfoFromMsgHash(pk *PublicKey, hash []byte) *AggregationInfo {
	// Get a C pointer to bytes
	cMessagePtr := C.CBytes(hash)
	defer C.free(cMessagePtr)

	var ai AggregationInfo
	ai.ai = C.CAggregationInfoFromMsgHash(pk.pk, cMessagePtr)

	runtime.SetFinalizer(&ai, func(p *AggregationInfo) { p.Free() })
	runtime.KeepAlive(pk)

	return &ai
}

// Free releases memory allocated by the AggregationInfo object
func (ai *AggregationInfo) Free() {
	C.CAggregationInfoFree(ai.ai)
	runtime.KeepAlive(ai)
}

// MergeAggregationInfos merges multiple AggregationInfo objects into one.
func MergeAggregationInfos(AIs []*AggregationInfo) *AggregationInfo {
	// Get a C pointer to an array of aggregation info objects
	cAIsPtr := C.AllocPtrArray(C.size_t(len(AIs)))
	defer C.FreePtrArray(cAIsPtr)

	// Loop thru each AggInfo and add the key C ptr to the array of ptrs at index
	for i, aggInfo := range AIs {
		C.SetPtrArray(cAIsPtr, unsafe.Pointer(aggInfo.ai), C.int(i))
	}

	var ai AggregationInfo
	ai.ai = C.MergeAggregationInfos(cAIsPtr, C.size_t(len(AIs)))

	runtime.KeepAlive(AIs)
	runtime.SetFinalizer(&ai, func(p *AggregationInfo) { p.Free() })

	return &ai
}

// RemoveEntries removes the messages and pubkeys from the tree
func (ai *AggregationInfo) RemoveEntries(messages [][]byte, publicKeys []*PublicKey) error {
	// Get a C pointer to an array of messages
	cNumMessages := C.size_t(len(messages))
	cMessageArrayPtr := C.AllocPtrArray(cNumMessages)
	defer C.FreePtrArray(cMessageArrayPtr)

	// Loop thru each message and add the key C ptr to the array of ptrs at
	// index
	for i, msg := range messages {
		// Get a C pointer to bytes
		cMessagePtr := C.CBytes(msg)
		C.SetPtrArray(cMessageArrayPtr, cMessagePtr, C.int(i))
	}

	// Get a C pointer to an array of public keys
	cNumPublicKeys := C.size_t(len(publicKeys))
	cPublicKeysPtr := C.AllocPtrArray(cNumPublicKeys)
	defer C.FreePtrArray(cPublicKeysPtr)
	// Loop thru each key and add the key C ptr to the array of ptrs at index
	for i, key := range publicKeys {
		C.SetPtrArray(cPublicKeysPtr, unsafe.Pointer(key.pk), C.int(i))
	}

	var cDidErr C.bool
	C.CAggregationInfoRemoveEntries(ai.ai, cMessageArrayPtr, cNumMessages,
		cPublicKeysPtr, cNumPublicKeys, &cDidErr)
	if bool(cDidErr) {
		cErrMsg := C.GetLastErrorMsg()
		err := errors.New(C.GoString(cErrMsg))
		return err
	}

	for i := range messages {
		C.free(C.GetPtrAtIndex(cMessageArrayPtr, C.int(i)))
	}

	runtime.KeepAlive(ai)
	runtime.KeepAlive(publicKeys)

	return nil
}

// Equal tests if two AggregationInfo objects are equal
func (ai *AggregationInfo) Equal(other *AggregationInfo) bool {
	isEqual := bool(C.CAggregationInfoIsEqual(ai.ai, other.ai))
	runtime.KeepAlive(ai)
	runtime.KeepAlive(other)
	return isEqual
}

// Less tests if one AggregationInfo object is less than the other
func (ai *AggregationInfo) Less(other *AggregationInfo) bool {
	isLess := bool(C.CAggregationInfoIsLess(ai.ai, other.ai))
	runtime.KeepAlive(ai)
	runtime.KeepAlive(other)
	return isLess
}

// Empty tests whether an AggregationInfo object is empty
func (ai *AggregationInfo) Empty() bool {
	isEmpty := bool(C.CAggregationInfoEmpty(ai.ai))
	runtime.KeepAlive(ai)
	return isEmpty
}

// GetPubKeys returns the PublicKeys referenced by the AggregationInfo object
func (ai *AggregationInfo) GetPubKeys() []*PublicKey {
	// Get a C pointer to an array of bytes
	var cNumKeys C.size_t
	cPubKeysPtr := C.CAggregationInfoGetPubKeys(ai.ai, &cNumKeys)
	defer C.free(unsafe.Pointer(cPubKeysPtr))

	numKeys := int(cNumKeys)
	keys := make([]*PublicKey, numKeys)
	for i := 0; i < numKeys; i++ {
		keyPtr := C.GetAddressAtIndex(cPubKeysPtr, C.int(i))
		pkBytes := C.GoBytes(unsafe.Pointer(keyPtr), C.CPublicKeySizeBytes())
		keys[i], _ = PublicKeyFromBytes(pkBytes)
	}

	runtime.KeepAlive(ai)

	return keys
}

// GetMessageHashes returns the message hashes referenced by the
// AggregationInfo object
func (ai *AggregationInfo) GetMessageHashes() [][]byte {
	// Get a C pointer to an array of message hashes
	var cNumHashes C.size_t
	hashPtr := C.CAggregationInfoGetMessageHashes(ai.ai, &cNumHashes)
	defer C.free(unsafe.Pointer(hashPtr))

	numHashes := int(cNumHashes)
	hashes := make([][]byte, numHashes)
	for i := 0; i < numHashes; i++ {
		// get the singular pointer at index
		hashPtr := C.GetAddressAtIndex(hashPtr, C.int(i))
		hashes[i] = C.GoBytes(hashPtr, C.CBLSMessageHashLen())
	}

	runtime.KeepAlive(ai)

	return hashes
}
