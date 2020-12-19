package blschia_test

import (
	"bytes"
	"testing"

	bls "github.com/dashpay/bls-signatures/go-bindings"
)

func TestChainCode(t *testing.T) {
	cc1, err := bls.ChainCodeFromBytes(sk1Bytes)
	if err != nil {
		t.Error(err)
	}

	cc1Bytes := cc1.Serialize()
	if !bytes.Equal(cc1Bytes, sk1Bytes) {
		t.Errorf("got %v, expected %v", cc1Bytes, sk1Bytes)
	}

	cc2, err := bls.ChainCodeFromBytes(sk2Bytes)
	if err != nil {
		t.Error(err)
	}

	if cc1.Equal(cc2) {
		t.Error("cc1 should NOT be equal to cc2")
	}

	cc3, err := bls.ChainCodeFromBytes(cc1Bytes)
	if err != nil {
		t.Error(err)
	}

	if !cc1.Equal(cc3) {
		t.Error("cc1 should be equal to cc3")
	}
}
