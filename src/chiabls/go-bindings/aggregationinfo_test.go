package blschia_test

import (
	"bytes"
	"testing"

	bls "github.com/dashpay/bls-signatures/go-bindings"
)

func TestAggregationInfo(t *testing.T) {
	pk1, _ := bls.PublicKeyFromBytes(pk1Bytes)

	mh := Sha256(payload)
	ai := bls.AggregationInfoFromMsgHash(pk1, mh)
	if ai.Empty() {
		t.Error("expected AI to have entries")
	}

	if !ai.Equal(ai) {
		t.Error("ai should be equal to itself")
	}

	err := ai.RemoveEntries([][]byte{mh}, []*bls.PublicKey{pk1})
	if err != nil {
		t.Errorf("got unexpected error: %v", err.Error())
	}
	if !ai.Empty() {
		t.Error("expected AI to be empty")
	}

	ai1 := bls.AggregationInfoFromMsgHash(pk1, mh)
	pks := ai1.GetPubKeys()
	if len(pks) != 1 {
		t.Error("should have returned 1 public key")
	}
	hashes := ai1.GetMessageHashes()
	if len(hashes) != 1 {
		t.Error("should have returned 1 message hash")
	}
	mhBytes := hashes[0]
	if !bytes.Equal(mhBytes, mh) {
		t.Errorf("got %v, expected %v", mhBytes, mh)
	}

	pkBytes := pks[0].Serialize()
	if !bytes.Equal(pkBytes, pk1Bytes) {
		t.Errorf("got %v, expected %v", pkBytes, pk1Bytes)
	}

	pk2, _ := bls.PublicKeyFromBytes(pk2Bytes)
	ai2 := bls.AggregationInfoFromMsgHash(pk2, mh)
	hashes = ai2.GetMessageHashes()
	if len(hashes) != 1 {
		t.Error("should have returned 1 message hash")
	}
	mhBytes = hashes[0]
	if !bytes.Equal(mhBytes, mh) {
		t.Errorf("got %v, expected %v", mhBytes, mh)
	}
	pks = ai2.GetPubKeys()
	if len(pks) != 1 {
		t.Error("should have returned 1 public key")
	}
	pkBytes = pks[0].Serialize()
	if !bytes.Equal(pkBytes, pk2Bytes) {
		t.Errorf("got %v, expected %v", pkBytes, pk2Bytes)
	}

	if !ai1.Less(ai2) {
		t.Error("ai1 should be less then ai2")
	}

	// test equal values, not just same pointer address
	ai3 := bls.AggregationInfoFromMsgHash(pk1, mh)
	if !ai1.Equal(ai3) {
		t.Error("ai1 should be equal to ai3")
	}
	if ai1.Equal(ai2) {
		t.Error("ai1 should NOT be equal to ai2")
	}
}
