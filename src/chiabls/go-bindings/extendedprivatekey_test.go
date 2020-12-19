package blschia_test

import (
	"bytes"
	"fmt"
	"testing"

	bls "github.com/dashpay/bls-signatures/go-bindings"
)

func TestExtendedPrivateKey(t *testing.T) {
	xprv1, err := bls.ExtendedPrivateKeyFromSeed(xprvSeed)
	if err != nil {
		t.Error(err)
	}

	xprv1Bytes := xprv1.Serialize()
	xprv1Expected := []byte{
		0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0xd8, 0xb1, 0x25,
		0x55, 0xb4, 0xcc, 0x55, 0x78, 0x95, 0x1e, 0x4a,
		0x7c, 0x80, 0x03, 0x1e, 0x22, 0x01, 0x9c, 0xc0,
		0xdc, 0xe1, 0x68, 0xb3, 0xed, 0x88, 0x11, 0x53,
		0x11, 0xb8, 0xfe, 0xb1, 0xe3, 0x3e, 0x9f, 0x7b,
		0x38, 0x46, 0xc1, 0x80, 0x37, 0x03, 0xf9, 0x4c,
		0x76, 0x4b, 0x51, 0xf5, 0xac, 0xe5, 0x13, 0xb2,
		0xf0, 0x2c, 0x4d, 0x6b, 0x2c, 0x45, 0x2d, 0x8c,
		0xe6, 0x6e, 0x59, 0x75, 0xbd,
	}
	if !bytes.Equal(xprv1Bytes, xprv1Expected) {
		t.Errorf("got %v, expected %v", xprv1Bytes, xprv1Expected)
	}

	xprv2, err := bls.ExtendedPrivateKeyFromBytes(xprv1Bytes)
	if err != nil {
		t.Error(err)
	}

	if !xprv2.Equal(xprv1) {
		t.Error("xprv2 should be equal to xprv1")
	}

	expectedVersion := uint32(1)
	actualVersion := xprv2.GetVersion()
	if actualVersion != expectedVersion {
		t.Errorf("got version %v, expected %v", actualVersion, expectedVersion)
	}

	expectedDepth := uint8(0)
	actualDepth := xprv2.GetDepth()
	if actualDepth != expectedDepth {
		t.Errorf("got depth %v, expected %v", actualDepth, expectedDepth)
	}

	expectedParentFingerprint := uint32(0)
	actualParentFingerprint := xprv2.GetParentFingerprint()
	if actualParentFingerprint != expectedParentFingerprint {
		t.Errorf("got parent fingerprint %v, expected %v", actualParentFingerprint, expectedParentFingerprint)
	}

	expectedChildNumber := uint32(0)
	actualChildNumber := xprv2.GetChildNumber()
	if actualChildNumber != expectedChildNumber {
		t.Errorf("got child number %v, expected %v", actualChildNumber, expectedChildNumber)
	}

	expectedChainCode := []byte{
		0xd8, 0xb1, 0x25, 0x55, 0xb4, 0xcc, 0x55, 0x78,
		0x95, 0x1e, 0x4a, 0x7c, 0x80, 0x03, 0x1e, 0x22,
		0x01, 0x9c, 0xc0, 0xdc, 0xe1, 0x68, 0xb3, 0xed,
		0x88, 0x11, 0x53, 0x11, 0xb8, 0xfe, 0xb1, 0xe3,
	}
	actualChainCode := xprv2.GetChainCode().Serialize()
	if !bytes.Equal(expectedChainCode, actualChainCode) {
		t.Errorf("got %v, expected %v", expectedChainCode, actualChainCode)
	}

	expectedPrivKey := []byte{
		0x3e, 0x9f, 0x7b, 0x38, 0x46, 0xc1, 0x80, 0x37,
		0x03, 0xf9, 0x4c, 0x76, 0x4b, 0x51, 0xf5, 0xac,
		0xe5, 0x13, 0xb2, 0xf0, 0x2c, 0x4d, 0x6b, 0x2c,
		0x45, 0x2d, 0x8c, 0xe6, 0x6e, 0x59, 0x75, 0xbd,
	}
	actualPrivKey := xprv2.GetPrivateKey().Serialize()
	if !bytes.Equal(actualPrivKey, expectedPrivKey) {
		t.Errorf("got key bytes %v, expected %v", actualPrivKey, expectedPrivKey)
	}

	xprv3, err := bls.ExtendedPrivateKeyFromString(fmt.Sprintf("%x", xprv2.Serialize()))
	if err != nil {
		t.Error(err)
	}

	if !xprv3.Equal(xprv2) {
		t.Error("xprv3 should be equal to xprv2")
	}
}

var xprvSeed = []byte{0x01, 0x32, 0x06, 0xf4, 0x18, 0xc7, 0x01, 0x19}
