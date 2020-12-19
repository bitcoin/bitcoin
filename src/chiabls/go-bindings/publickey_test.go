package blschia_test

import (
	"bytes"
	"testing"

	bls "github.com/dashpay/bls-signatures/go-bindings"
)

func TestPublicKey(t *testing.T) {
	pk1, _ := bls.PublicKeyFromBytes(pk1Bytes)
	pk2, _ := bls.PublicKeyFromBytes(pk2Bytes)
	if pk1.Equal(pk2) {
		t.Error("pk1 should NOT be equal to pk2")
	}

	pk1SerBytes := pk1.Serialize()
	if !bytes.Equal(pk1SerBytes, pk1Bytes) {
		t.Errorf("got %v, expected %v", pk1SerBytes, pk1Bytes)
	}

	pk3, _ := bls.PublicKeyFromBytes(pk1SerBytes)
	if !pk1.Equal(pk3) {
		t.Error("pk1 should be equal to pk3")
	}

	pk1fp := pk1.Fingerprint()
	pk1ExpectedFp := uint32(0x26d53247)
	if pk1fp != pk1ExpectedFp {
		t.Errorf("pk1 fingerprint got %v, expected %v", pk1fp, pk1ExpectedFp)
	}

	aggPk, err := bls.PublicKeyAggregate([]*bls.PublicKey{pk1, pk2})
	if err != nil {
		t.Errorf("got unexpected error: %v", err.Error())
	}
	aggPkBytes := aggPk.Serialize()
	aggPkExpectedBytes := []byte{
		0x13, 0xff, 0x74, 0xea, 0x55, 0x95, 0x29, 0x24,
		0xe8, 0x24, 0xc5, 0xa0, 0x88, 0x25, 0xe3, 0xc3,
		0x6d, 0x92, 0x8d, 0xf7, 0xfb, 0xa1, 0x5b, 0xf4,
		0x92, 0xd0, 0x0a, 0x6a, 0x11, 0x28, 0x68, 0x62,
		0x5f, 0x77, 0x2c, 0x91, 0x02, 0xf2, 0xd9, 0xe2,
		0x1b, 0x99, 0xbf, 0x99, 0xfd, 0xc6, 0x27, 0xb6,
	}
	if !bytes.Equal(aggPkBytes, aggPkExpectedBytes) {
		t.Errorf("got %v, expected %v", aggPkBytes, aggPkExpectedBytes)
	}

	aggPkIns, err := bls.PublicKeyAggregateInsecure([]*bls.PublicKey{pk1, pk2})
	if err != nil {
		t.Errorf("got unexpected error: %v", err.Error())
	}
	aggPkInsBytes := aggPkIns.Serialize()
	aggPkInsExpectedBytes := []byte{
		0x08, 0x69, 0x30, 0xec, 0x41, 0x51, 0x7e, 0x3e,
		0xdc, 0x34, 0xc0, 0xf5, 0x70, 0x47, 0x58, 0x8f,
		0xa3, 0xce, 0xec, 0x96, 0x38, 0x9b, 0x6d, 0xf3,
		0xec, 0xde, 0x66, 0x41, 0xdc, 0x0f, 0x70, 0xda,
		0xe1, 0xdb, 0x6a, 0x6c, 0x12, 0x86, 0x93, 0xd0,
		0xbb, 0xec, 0x3b, 0x20, 0xd8, 0x70, 0x43, 0xf0,
	}
	if !bytes.Equal(aggPkInsBytes, aggPkInsExpectedBytes) {
		t.Errorf("got %v, expected %v", aggPkInsBytes, aggPkInsExpectedBytes)
	}

	publicKeys := []*bls.PublicKey{pk1, pk2, pk3}
	ids := []bls.Hash{{1}, {2}, {3}}

	share1, err := bls.PublicKeyShare(publicKeys, ids[0])
	if err != nil {
		t.Errorf("PublicKeyShare failed with error: %v", err.Error())
	}

	share2, err := bls.PublicKeyShare(publicKeys, ids[1])
	if err != nil {
		t.Errorf("PublicKeyShare failed with error: %v", err.Error())
	}

	share3, err := bls.PublicKeyShare(publicKeys, ids[2])
	if err != nil {
		t.Errorf("PublicKeyShare failed with error: %v", err.Error())
	}

	recovered, err := bls.PublicKeyRecover([]*bls.PublicKey{share1, share2, share3}, ids)
	if err != nil {
		t.Errorf("PublicKeyRecover failed with error: %v", err.Error())
	}

	if !recovered.Equal(publicKeys[0]) {
		t.Errorf("got %v, expected %v", recovered.Serialize(), publicKeys[0].Serialize())
	}
}
