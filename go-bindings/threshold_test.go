package blschia

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestThreshold(t *testing.T) {
	testCases := []struct {
		n, m int
		hash [32]byte
	}{
		{
			n:    5,
			m:    3,
			hash: genHash(100),
		},
	}
	for j, tc := range testCases {
		t.Run(fmt.Sprintf("threshold test case #%d", j), func(t *testing.T) {
			n, m := tc.n, tc.m
			var hashes []Hash
			for i := 0; i < n; i++ {
				hashes = append(hashes, genHash(byte(i+1)))
			}
			pks := make([]*G1Element, m)
			sigs := make([]*G2Element, m)
			sks := make([]*PrivateKey, m)
			for i := 0; i < m; i++ {
				sks[i] = mustPrivateKeyFromBytes(genSeed(byte(i+11)), true)
				pks[i] = mustGetG1(sks[i])
				sigs[i] = ThresholdSign(sks[i], tc.hash)
				assert.True(t, ThresholdVerify(pks[i], tc.hash, sigs[i]))
			}
			sig := ThresholdSign(sks[0], tc.hash)
			assert.True(t, ThresholdVerify(pks[0], tc.hash, sig))
			skShares := make([]*PrivateKey, n)
			pkShares := make([]*G1Element, n)
			sigShares := make([]*G2Element, n)
			for i := 0; i < n; i++ {
				skShares[i] = mustThresholdPrivateKeyShare(sks, hashes[i])
				pkShares[i] = mustThresholdPublicKeyShare(pks, hashes[i])
				sigShares[i] = mustThresholdSignatureShare(sigs, hashes[i])
				assert.True(t, mustGetG1(skShares[i]).EqualTo(pkShares[i]))
				sigShare2 := ThresholdSign(skShares[i], tc.hash)
				assert.True(t, sigShares[i].EqualTo(sigShare2))
				assert.True(t, ThresholdVerify(pkShares[i], tc.hash, sigShares[i]))
			}
			recSk := mustThresholdPrivateKeyRecover(skShares[:m-1], hashes[:m-1])
			recPk := mustThresholdPublicKeyRecover(pkShares[:m-1], hashes[:m-1])
			recSig := mustThresholdSignatureRecover(sigShares[:m-1], hashes[:m-1])
			assert.False(t, recSk.EqualTo(sks[0]))
			assert.False(t, recPk.EqualTo(pks[0]))
			assert.False(t, recSig.EqualTo(sig))
			recSk = mustThresholdPrivateKeyRecover(skShares[:m], hashes[:m])
			recPk = mustThresholdPublicKeyRecover(pkShares[:m], hashes[:m])
			recSig = mustThresholdSignatureRecover(sigShares[:m], hashes[:m])
			assert.True(t, recSk.EqualTo(sks[0]))
			assert.True(t, recPk.EqualTo(pks[0]))
			assert.True(t, recSig.EqualTo(sig))
		})
	}
}

func mustGetG1(sk *PrivateKey) *G1Element {
	pk, err := sk.G1Element()
	if err != nil {
		panic(err)
	}
	return pk
}

func mustPrivateKeyFromBytes(data []byte, modOrder bool) *PrivateKey {
	sk, err := PrivateKeyFromBytes(data, modOrder)
	if err != nil {
		panic(err)
	}
	return sk
}

func genHash(v byte) [HashSize]byte {
	var hash Hash
	copy(hash[:], genSeed(v))
	return hash
}

func mustThresholdPrivateKeyShare(sks []*PrivateKey, hash Hash) *PrivateKey {
	share, err := ThresholdPrivateKeyShare(sks, hash)
	if err != nil {
		panic(err)
	}
	return share
}

func mustThresholdPublicKeyShare(pks []*G1Element, hash Hash) *G1Element {
	share, err := ThresholdPublicKeyShare(pks, hash)
	if err != nil {
		panic(err)
	}
	return share
}

func mustThresholdSignatureShare(sigs []*G2Element, hash Hash) *G2Element {
	share, err := ThresholdSignatureShare(sigs, hash)
	if err != nil {
		panic(err)
	}
	return share
}

func mustThresholdPrivateKeyRecover(sks []*PrivateKey, hashes []Hash) *PrivateKey {
	recSk, err := ThresholdPrivateKeyRecover(sks, hashes)
	if err != nil {
		panic(err)
	}
	return recSk
}

func mustThresholdPublicKeyRecover(pks []*G1Element, hashes []Hash) *G1Element {
	recPk, err := ThresholdPublicKeyRecover(pks, hashes)
	if err != nil {
		panic(err)
	}
	return recPk
}

func mustThresholdSignatureRecover(sigs []*G2Element, hashes []Hash) *G2Element {
	recSig, err := ThresholdSignatureRecover(sigs, hashes)
	if err != nil {
		panic(err)
	}
	return recSig
}
