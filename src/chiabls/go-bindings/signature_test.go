package blschia_test

import (
	"bytes"
	"testing"

	bls "github.com/dashpay/bls-signatures/go-bindings"
)

func TestSignature(t *testing.T) {
	sk1, _ := bls.PrivateKeyFromBytes(sk1Bytes, true)
	sk2, _ := bls.PrivateKeyFromBytes(sk2Bytes, true)

	sig1 := sk1.Sign(payload)
	if !bytes.Equal(sig1.Serialize(), sig1Bytes) {
		t.Error("sig1.Serialize() should be equal to sig1bytes")
	}
	sig2 := sk2.Sign(payload)
	if !bytes.Equal(sig2.Serialize(), sig2Bytes) {
		t.Error("sig2.Serialize() should be equal to sig2bytes")
	}
	if sig1.Equal(sig2) {
		t.Error("sig1 should NOT be equal to sig2")
	}

	insecureSig1, _ := bls.InsecureSignatureFromBytes(sig1Bytes)
	insecureSig2, _ := bls.InsecureSignatureFromBytes(sig2Bytes)
	if insecureSig1.Equal(insecureSig2) {
		t.Error("insecureSig1 should NOT be equal to insecureSig2")
	}
	if !sig1.Verify() {
		t.Error("sig1 should verify")
	}
	if !sig2.Verify() {
		t.Error("sig2 should verify")
	}
	insecureSig3, _ := bls.InsecureSignatureFromBytes(insecureSig1.Serialize())
	if !bytes.Equal(insecureSig3.Serialize(), sig1Bytes) {
		t.Errorf("got %v, expected %v", insecureSig3.Serialize(), sig1Bytes)
	}

	mh := Sha256(payload)
	if !insecureSig1.Verify([][]byte{mh}, []*bls.PublicKey{sk1.PublicKey()}) {
		t.Error("insecureSig1 should verify")
	}
	if !insecureSig2.Verify([][]byte{mh}, []*bls.PublicKey{sk2.PublicKey()}) {
		t.Error("insecureSig2 should verify")
	}

	ai1 := sig1.GetAggregationInfo()
	sig3, _ := bls.SignatureFromBytes(sig1Bytes)
	sig3.SetAggregationInfo(ai1)
	if !sig1.Equal(sig3) {
		t.Error("sig1 should be equal to sig3")
	}
	ai3 := sig3.GetAggregationInfo()
	if !ai1.Equal(ai3) {
		t.Error("ai1 should be equal to ai3")
	}

	sig4, _ := bls.SignatureFromBytes(sig2.Serialize())
	if !bytes.Equal(sig4.Serialize(), sig2Bytes) {
		t.Error("sig4.Serialize() should be equal to sig2bytes")
	}

	aggSig, _ := bls.SignatureAggregate([]*bls.Signature{sig1, sig2})
	aggSigBytes := aggSig.Serialize()
	aggSigExpectedBytes := []byte{
		0x0a, 0x63, 0x84, 0x95, 0xc1, 0x40, 0x3b, 0x25,
		0xbe, 0x39, 0x1e, 0xd4, 0x4c, 0x0a, 0xb0, 0x13,
		0x39, 0x00, 0x26, 0xb5, 0x89, 0x2c, 0x79, 0x6a,
		0x85, 0xed, 0xe4, 0x63, 0x10, 0xff, 0x7d, 0x0e,
		0x06, 0x71, 0xf8, 0x6e, 0xbe, 0x0e, 0x8f, 0x56,
		0xbe, 0xe8, 0x0f, 0x28, 0xeb, 0x6d, 0x99, 0x9c,
		0x0a, 0x41, 0x8c, 0x5f, 0xc5, 0x2d, 0xeb, 0xac,
		0x8f, 0xc3, 0x38, 0x78, 0x4c, 0xd3, 0x2b, 0x76,
		0x33, 0x8d, 0x62, 0x9d, 0xc2, 0xb4, 0x04, 0x5a,
		0x58, 0x33, 0xa3, 0x57, 0x80, 0x97, 0x95, 0xef,
		0x55, 0xee, 0x3e, 0x9b, 0xee, 0x53, 0x2e, 0xdf,
		0xc1, 0xd9, 0xc4, 0x43, 0xbf, 0x5b, 0xc6, 0x58,
	}
	if !bytes.Equal(aggSigBytes, aggSigExpectedBytes) {
		t.Errorf("got %v, expected %v", aggSigBytes, aggSigExpectedBytes)
	}

	aggSigIns, _ := bls.InsecureSignatureAggregate([]*bls.InsecureSignature{insecureSig1, insecureSig2})
	aggSigInsBytes := aggSigIns.Serialize()
	aggSigInsExpectedBytes := []byte{
		0x09, 0x55, 0x87, 0x5b, 0x67, 0xf2, 0x17, 0x94,
		0x9e, 0x37, 0xb7, 0xed, 0x63, 0xc2, 0xec, 0xfd,
		0xdb, 0xd7, 0x49, 0xc2, 0xdf, 0x69, 0x7a, 0x2a,
		0xea, 0xfc, 0x80, 0x67, 0x86, 0x45, 0x56, 0x55,
		0x10, 0xb7, 0x48, 0xde, 0xd3, 0xb9, 0x48, 0xba,
		0xe8, 0xad, 0xe5, 0x5f, 0xa7, 0x17, 0xae, 0x54,
		0x02, 0xc1, 0x50, 0x16, 0x77, 0xa7, 0xf4, 0xfc,
		0xa3, 0x7a, 0x2a, 0x4a, 0xe2, 0x55, 0x24, 0xa1,
		0x9d, 0xec, 0x50, 0xb0, 0xa5, 0xbc, 0x53, 0xf4,
		0x64, 0xd7, 0x65, 0xfd, 0x2e, 0x2f, 0x55, 0xed,
		0x93, 0xfb, 0xac, 0x05, 0xc5, 0xa7, 0x23, 0xcd,
		0xbf, 0xef, 0xe2, 0xd5, 0x20, 0x5d, 0xf4, 0xe4,
	}
	if !bytes.Equal(aggSigInsBytes, aggSigInsExpectedBytes) {
		t.Errorf("got %v, expected %v", aggSigInsBytes, aggSigInsExpectedBytes)
	}

	sig5, _ := bls.SignatureFromBytesWithAggregationInfo(sig1Bytes, ai1)
	if !sig5.Equal(sig1) {
		t.Error("sig5 should be equal to sig1")
	}
	if !sig5.GetAggregationInfo().Equal(ai1) {
		t.Error("sig5 AggInfo should be equal to sig1 AggInfo")
	}

	ai2 := sig2.GetAggregationInfo()
	sig6 := bls.SignatureFromInsecureSig(insecureSig2)
	sig7 := bls.SignatureFromInsecureSigWithAggregationInfo(insecureSig2, ai2)
	if !sig6.Equal(sig7) {
		t.Error("sig6 should be equal to sig7")
	}
	if !sig7.GetAggregationInfo().Equal(ai2) {
		t.Error("sig7 AggInfo should be equal to sig2 AggInfo")
	}

	insecureSig4 := sig2.GetInsecureSig()
	if !insecureSig4.Equal(insecureSig2) {
		t.Error("insecureSig4 should be equal to insecureSig2")
	}

	insecureSignatures := []*bls.InsecureSignature{insecureSig1, insecureSig2, insecureSig3}
	ids := []bls.Hash{{1}, {2}, {3}}

	share1, err := bls.InsecureSignatureShare(insecureSignatures, ids[0])
	if err != nil {
		t.Errorf("InsecureSignatureShare failed with error: %v", err.Error())
	}

	share2, err := bls.InsecureSignatureShare(insecureSignatures, ids[1])
	if err != nil {
		t.Errorf("InsecureSignatureShare failed with error: %v", err.Error())
	}

	share3, err := bls.InsecureSignatureShare(insecureSignatures, ids[2])
	if err != nil {
		t.Errorf("InsecureSignatureShare failed with error: %v", err.Error())
	}

	recovered, err := bls.InsecureSignatureRecover([]*bls.InsecureSignature{share1, share2, share3}, ids)
	if err != nil {
		t.Errorf("InsecureSignatureRecover failed with error: %v", err.Error())
	}

	if !recovered.Equal(insecureSignatures[0]) {
		t.Errorf("got %v, expected %v", recovered.Serialize(), insecureSignatures[0].Serialize())
	}
}

func TestSignatureDivision(t *testing.T) {
	m1 := []byte{1, 2, 3, 40}
	m2 := []byte{5, 6, 70, 201}

	sk1, _ := bls.PrivateKeyFromBytes(sk1Bytes, true)
	sk2, _ := bls.PrivateKeyFromBytes(sk2Bytes, true)

	sig1 := sk1.Sign(m1)
	sig2 := sk2.Sign(m2)

	aggSig, _ := bls.SignatureAggregate([]*bls.Signature{sig1, sig2})
	quot, _ := aggSig.DivideBy([]*bls.Signature{sig1})
	quotBytes := quot.Serialize()
	sig2Bytes := sig2.Serialize()
	if !bytes.Equal(quotBytes, sig2Bytes) {
		t.Errorf("got %v, expected %v", quotBytes, sig2Bytes)
	}

	insecureSig1, _ := bls.InsecureSignatureFromBytes(sig1Bytes)
	insecureSig2, _ := bls.InsecureSignatureFromBytes(sig2Bytes)

	aggSigIns, _ := bls.InsecureSignatureAggregate([]*bls.InsecureSignature{insecureSig1, insecureSig2})
	quotIns, _ := aggSigIns.DivideBy([]*bls.InsecureSignature{insecureSig1})
	quotInsBytes := quotIns.Serialize()
	insecureSig2Bytes := insecureSig2.Serialize()
	if !bytes.Equal(quotInsBytes, insecureSig2Bytes) {
		t.Errorf("got %v, expected %v", quotInsBytes, insecureSig2Bytes)
	}
}
