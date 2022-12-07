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

import (
	"encoding/hex"
	"fmt"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestSchemesSignAndVerify(t *testing.T) {
	msg := []byte{100, 2, 254, 88, 90, 45, 23}
	testCases := []struct {
		name   string
		scheme Scheme
	}{
		{
			name:   "test a verification of basic scheme",
			scheme: NewBasicSchemeMPL(),
		},
		{
			name:   "test a verification of augmented scheme",
			scheme: NewAugSchemeMPL(),
		},
		{
			name:   "test a verification of proof of possession scheme",
			scheme: NewPopSchemeMPL(),
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			sk, err := tc.scheme.KeyGen(genSeed(1))
			assert.NoError(t, err)
			pk, err := sk.G1Element()
			assert.NoError(t, err)
			sig := tc.scheme.Sign(sk, msg)
			assert.True(t, tc.scheme.Verify(pk, msg, sig))
		})
	}
}

func TestSchemeAggregate(t *testing.T) {
	seed := []byte{
		0, 50, 6, 244, 24, 199, 1, 25,
		52, 88, 192, 19, 18, 12, 89, 6,
		220, 18, 102, 58, 209, 82, 12, 62,
		89, 110, 182, 9, 44, 20, 254, 22,
	}
	msg1 := []byte{100, 2, 254, 88, 90, 45, 23}
	msg2 := []byte{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
	testCases := []struct {
		name   string
		scheme Scheme
	}{
		{
			name:   "testing of the basic scheme aggregation",
			scheme: NewBasicSchemeMPL(),
		},
		{
			name:   "testing of the augmented scheme aggregation",
			scheme: NewAugSchemeMPL(),
		},
		{
			name:   "testing of the proof of possession scheme aggregation",
			scheme: NewPopSchemeMPL(),
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			seed1 := replLeft(seed, []byte{1})
			seed2 := replLeft(seed, []byte{2})

			sk1, _ := tc.scheme.KeyGen(seed1)
			pk1, _ := sk1.G1Element()

			sk2, _ := tc.scheme.KeyGen(seed2)
			pk2, _ := sk2.G1Element()

			aggPk := pk1.Add(pk2)

			var sig1, sig2 *G2Element
			switch scheme := tc.scheme.(type) {
			case *AugSchemeMPL:
				sig1 = scheme.SignPrepend(sk1, msg1, aggPk)
				sig2 = scheme.SignPrepend(sk2, msg1, aggPk)
			default:
				sig1 = scheme.Sign(sk1, msg1)
				sig2 = scheme.Sign(sk2, msg1)
			}

			aggSig := tc.scheme.AggregateSigs(sig1, sig2)
			assert.True(t, tc.scheme.Verify(aggPk, msg1, aggSig))

			// Aggregate different msg
			sig1 = tc.scheme.Sign(sk1, msg1)
			sig2 = tc.scheme.Sign(sk2, msg2)
			aggSig = tc.scheme.AggregateSigs(sig1, sig2)
			assert.True(t, tc.scheme.AggregateVerify([]*G1Element{pk1, pk2}, [][]byte{msg1, msg2}, aggSig))

			// HD keys
			child := tc.scheme.DeriveChildSk(sk1, 123)
			childU := tc.scheme.DeriveChildSkUnhardened(sk1, 123)
			childUPk := tc.scheme.DeriveChildPkUnhardened(pk1, 123)

			sigChild := tc.scheme.Sign(child, msg1)
			sigChildPk, _ := child.G1Element()
			assert.True(t, tc.scheme.Verify(sigChildPk, msg1, sigChild))

			sigUChild := tc.scheme.Sign(childU, msg1)
			assert.True(t, tc.scheme.Verify(childUPk, msg1, sigUChild))
		})
	}
}

func TestVectorInvalid(t *testing.T) {
	type testCase struct {
		name string
		data []string
	}
	// Invalid inputs from https://github.com/algorand/bls_sigs_ref/blob/master/python-impl/serdesZ.py
	g1TestCases := []testCase{
		{
			name: "infinity points: too short",
			data: []string{
				// temporary disabled
				//"c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			},
		},
		{
			name: "infinity points: not all zeros",
			data: []string{
				"c00000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000",
				"400000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
				"400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000",
			},
		},
		{
			name: "bad tags",
			data: []string{
				"3a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
				"7a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
				"fa0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
			},
		},
		{
			name: "wrong length for compressed point",
			data: []string{
				"9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaa",
				"9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaaaa",
			},
		},
		{
			name: "wrong length for uncompressed point",
			data: []string{
				"1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
				"1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			},
		},
		{
			name: "invalid x-coord",
			data: []string{
				"9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
			},
		},
		{
			name: "invalid elm of Fp --- equal to p (must be strictly less)",
			data: []string{
				"9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab",
			},
		},
		{
			name: "point not on curve",
			data: []string{
				"1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
			},
		},
	}
	g2TestCases := []testCase{
		{
			name: "infinity points: too short",
			data: []string{
				"c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			},
		},
		{
			name: "infinity points: not all zeros",
			data: []string{
				"c00000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
				"c00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000",
			},
		},
		{
			name: "bad tags 1",
			data: []string{
				"3a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
				"7a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
				"fa0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
			},
		},
		{
			name: "invalid x-coord",
			data: []string{
				"9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaa7",
			},
		},
		{
			name: "invalid elm of Fp --- equal to p (must be strictly less)",
			data: []string{
				"9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
				"9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab",
			},
		},
		{
			name: "point not on curve",
			data: []string{
				"1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
			},
		},
	}
	testCases := []struct {
		name  string
		cases []testCase
		fn    func(data []byte) error
	}{
		{
			name:  "G1Element: ",
			cases: g1TestCases,
			fn: func(data []byte) error {
				_, err := G1ElementFromBytes(data)
				return err
			},
		},
		{
			name:  "G2Element: ",
			cases: g2TestCases,
			fn: func(data []byte) error {
				_, err := G2ElementFromBytes(data)
				return err
			},
		},
	}
	for _, tc := range testCases {
		for _, c := range tc.cases {
			t.Run(tc.name+c.name, func(t *testing.T) {
				for _, in := range c.data {
					data, _ := hex.DecodeString(in)
					err := tc.fn(data)
					assert.Error(t, err)
				}
			})
		}
	}
}

func TestVectorValid(t *testing.T) {
	secret1 := make([]byte, 32)
	secret2 := make([]byte, 32)
	for i := 0; i < 32; i++ {
		secret1[i] = 1
		secret2[i] = byte(i * 314159 % 256)
	}
	sk1, err := PrivateKeyFromBytes(secret1, false)
	assert.NoError(t, err)
	sk2, err := PrivateKeyFromBytes(secret2, false)
	assert.NoError(t, err)
	msg := []byte{3, 1, 4, 1, 5, 9}
	testCases := []struct {
		name    string
		scheme  Scheme
		refSig1 string
		refSig2 string
		refSigA string
	}{
		{
			name:   "testing of signature serialization of the basic scheme",
			scheme: NewBasicSchemeMPL(),
			refSig1: "96ba34fac33c7f129d602a0bc8a3d43f9abc014eceaab7359146b4b150e57b80" +
				"8645738f35671e9e10e0d862a30cab70074eb5831d13e6a5b162d01eebe687d0" +
				"164adbd0a864370a7c222a2768d7704da254f1bf1823665bc2361f9dd8c00e99",
			refSig2: "a402790932130f766af11ba716536683d8c4cfa51947e4f9081fedd692d6dc0c" +
				"ac5b904bee5ea6e25569e36d7be4ca59069a96e34b7f700758b716f9494aaa59" +
				"a96e74d14a3b552a9a6bc129e717195b9d6006fd6d5cef4768c022e0f7316abf",
			refSigA: "987cfd3bcd62280287027483f29c55245ed831f51dd6bd999a6ff1a1f1f1f0b6" +
				"47778b0167359c71505558a76e158e66181ee5125905a642246b01e7fa5ee53d" +
				"68a4fe9bfb29a8e26601f0b9ad577ddd18876a73317c216ea61f430414ec51c5",
		},
		{
			name:   "testing of signature serialization of the augmented scheme",
			scheme: NewAugSchemeMPL(),
			refSig1: "8180f02ccb72e922b152fcedbe0e1d195210354f70703658e8e08cbebf11d497" +
				"0eab6ac3ccf715f3fb876df9a9797abd0c1af61aaeadc92c2cfe5c0a56c146cc" +
				"8c3f7151a073cf5f16df38246724c4aed73ff30ef5daa6aacaed1a26ecaa336b",
			refSig2: "99111eeafb412da61e4c37d3e806c6fd6ac9f3870e54da9222ba4e494822c5b7" +
				"656731fa7a645934d04b559e9261b86201bbee57055250a459a2da10e51f9c1a" +
				"6941297ffc5d970a557236d0bdeb7cf8ff18800b08633871a0f0a7ea42f47480",
			refSigA: "8c5d03f9dae77e19a5945a06a214836edb8e03b851525d84b9de6440e68fc0ca" +
				"7303eeed390d863c9b55a8cf6d59140a01b58847881eb5af67734d44b2555646" +
				"c6616c39ab88d253299acc1eb1b19ddb9bfcbe76e28addf671d116c052bb1847",
		},
		{
			name:   "testing of signature serialization of the proof of possession scheme",
			scheme: NewPopSchemeMPL(),
			refSig1: "9550fb4e7f7e8cc4a90be8560ab5a798b0b23000b6a54a2117520210f986f3f2" +
				"81b376f259c0b78062d1eb3192b3d9bb049f59ecc1b03a7049eb665e0df36494" +
				"ae4cb5f1136ccaeefc9958cb30c3333d3d43f07148c386299a7b1bfc0dc5cf7c",
			refSig2: "a69036bc11ae5efcbf6180afe39addde7e27731ec40257bfdc3c37f17b8df683" +
				"06a34ebd10e9e32a35253750df5c87c2142f8207e8d5654712b4e554f585fb68" +
				"46ff3804e429a9f8a1b4c56b75d0869ed67580d789870babe2c7c8a9d51e7b2a",
			refSigA: "a4ea742bcdc1553e9ca4e560be7e5e6c6efa6a64dddf9ca3bb2854233d85a6aa" +
				"c1b76ec7d103db4e33148b82af9923db05934a6ece9a7101cd8a9d47ce279780" +
				"56b0f5900021818c45698afdd6cf8a6b6f7fee1f0b43716f55e413d4b87a6039",
		},
	}
	for i, tc := range testCases {
		t.Run(fmt.Sprintf("test case #%d", i), func(t *testing.T) {
			sig1 := tc.scheme.Sign(sk1, msg)
			sig2 := tc.scheme.Sign(sk2, msg)
			sigA := tc.scheme.AggregateSigs(sig1, sig2)
			assert.Equal(t, sig1.HexString(), tc.refSig1)
			assert.Equal(t, sig2.HexString(), tc.refSig2)
			assert.Equal(t, sigA.HexString(), tc.refSigA)
		})
	}
}

func TestBasicSchemeMPL(t *testing.T) {
	testCases := []struct {
		scheme         Scheme
		msgs           [][]byte
		refSig1        string
		refSk1         string
		refPk1         string
		refSig2        string
		refAggSig1     string
		refAggSig2     string
		pk1FingerPrint int
		pk2FingerPrint int
	}{
		{
			msgs: [][]byte{
				{7, 8, 9},
				{10, 11, 12},
				{1, 2, 3},
				{1, 2, 3, 4},
				{1, 2},
			},
			scheme: NewBasicSchemeMPL(),
			refSig1: "b8faa6d6a3881c9fdbad803b170d70ca5cbf1e6ba5a586262df368c75acd1d1f" +
				"fa3ab6ee21c71f844494659878f5eb230c958dd576b08b8564aad2ee0992e85a" +
				"1e565f299cd53a285de729937f70dc176a1f01432129bb2b94d3d5031f8065a1",
			refSk1:     "4a353be3dac091a0a7e640620372f5e1e2e4401717c1e79cac6ffba8f6905604",
			refPk1:     "85695fcbc06cc4c4c9451f4dce21cbf8de3e5a13bf48f44cdbb18e2038ba7b8bb1632d7911ef1e2e08749bddbf165352",
			refSig2:    "a9c4d3e689b82c7ec7e838dac2380cb014f9a08f6cd6ba044c263746e39a8f7a60ffee4afb78f146c2e421360784d58f0029491e3bd8ab84f0011d258471ba4e87059de295d9aba845c044ee83f6cf2411efd379ef38bf4cf41d5f3c0ae1205d",
			refAggSig1: "aee003c8cdaf3531b6b0ca354031b0819f7586b5846796615aee8108fec75ef838d181f9d244a94d195d7b0231d4afcf06f27f0cc4d3c72162545c240de7d5034a7ef3a2a03c0159de982fbc2e7790aeb455e27beae91d64e077c70b5506dea3",
			refAggSig2: "a0b1378d518bea4d1100adbc7bdbc4ff64f2c219ed6395cd36fe5d2aa44a4b8e710b607afd965e505a5ac3283291b75413d09478ab4b5cfbafbeea366de2d0c0bcf61deddaa521f6020460fd547ab37659ae207968b545727beba0a3c5572b9c",

			pk1FingerPrint: 0xb40dd58a,
			pk2FingerPrint: 0xb839add1,
		},
	}
	for i, tc := range testCases {
		t.Run(fmt.Sprintf("test case #%d", i), func(t *testing.T) {
			scheme := tc.scheme
			sk1, err := scheme.KeyGen(genSeed(0))
			assert.NoError(t, err)
			pk1, err := sk1.G1Element()
			assert.NoError(t, err)
			sig1 := scheme.Sign(sk1, tc.msgs[0])
			assert.NoError(t, err)

			sk2, err := scheme.KeyGen(genSeed(1))
			assert.NoError(t, err)
			pk2, err := sk2.G1Element()
			assert.NoError(t, err)
			sig2 := scheme.Sign(sk2, tc.msgs[1])
			assert.NoError(t, err)

			assert.Equal(t, pk1.Fingerprint(), 0xb40dd58a)
			assert.Equal(t, pk2.Fingerprint(), 0xb839add1)

			assert.Equal(t, sig1.HexString(), tc.refSig1)
			assert.Equal(t, sig2.HexString(), tc.refSig2)
			assert.Equal(t, sk1.HexString(), tc.refSk1)
			assert.Equal(t, pk1.HexString(), tc.refPk1)

			aggSig1 := scheme.AggregateSigs(sig1, sig2)
			assert.NoError(t, err)
			assert.Equal(t, aggSig1.HexString(), tc.refAggSig1)

			pks := []*G1Element{pk1, pk2}
			assert.True(t, scheme.AggregateVerify(pks, [][]byte{tc.msgs[0], tc.msgs[1]}, aggSig1))
			assert.False(t, scheme.AggregateVerify(pks, [][]byte{tc.msgs[0], tc.msgs[1]}, sig1))
			assert.False(t, scheme.Verify(pk1, tc.msgs[0], sig2))
			assert.False(t, scheme.Verify(pk1, tc.msgs[1], sig1))

			sig3 := scheme.Sign(sk1, tc.msgs[2])
			assert.NoError(t, err)
			sig4 := scheme.Sign(sk1, tc.msgs[3])
			assert.NoError(t, err)
			sig5 := scheme.Sign(sk2, tc.msgs[4])
			assert.NoError(t, err)

			aggSig2 := scheme.AggregateSigs(sig3, sig4, sig5)
			assert.NoError(t, err)

			assert.Equal(t, tc.refAggSig2, aggSig2.HexString())

			assert.True(t, scheme.AggregateVerify([]*G1Element{pk1, pk1, pk2}, tc.msgs[2:], aggSig2))
			assert.False(t, scheme.AggregateVerify([]*G1Element{pk1, pk1, pk2}, tc.msgs[2:], aggSig1))
		})
	}
}

func TestNewBasicSchemeMPL_KeyGen(t *testing.T) {
	scheme := NewBasicSchemeMPL()
	seed1 := genByteSlice(8, 31)
	seed2 := genByteSlice(8, 32)
	_, err := scheme.KeyGen(seed1)
	assert.True(t, checkError(err, "Seed size must be at least 32 bytes"))
	sk, err := scheme.KeyGen(seed2)
	assert.NoError(t, err)
	pk, _ := sk.G1Element()
	assert.Equal(t, pk.Fingerprint(), 0x8ee7ba56)
}

func TestAugSchemeMPL(t *testing.T) {
	msg1 := []byte{1, 2, 3, 40}
	msg2 := []byte{5, 6, 70, 201}
	msg3 := []byte{9, 10, 11, 12, 13}
	msg4 := []byte{15, 63, 244, 92, 0, 1}

	scheme := NewAugSchemeMPL()
	sk1, _ := scheme.KeyGen(genSeed(2))
	sk2, _ := scheme.KeyGen(genSeed(3))

	pk1, _ := sk1.G1Element()
	pk2, _ := sk2.G1Element()

	sig1 := scheme.Sign(sk1, msg1)
	sig2 := scheme.Sign(sk2, msg2)
	sig3 := scheme.Sign(sk2, msg1)
	sig4 := scheme.Sign(sk1, msg3)
	sig5 := scheme.Sign(sk1, msg1)
	sig6 := scheme.Sign(sk1, msg4)

	aggSigL := scheme.AggregateSigs(sig1, sig2)
	aggSigR := scheme.AggregateSigs(sig3, sig4, sig5)
	aggSig := scheme.AggregateSigs(aggSigL, aggSigR, sig6)

	pks := []*G1Element{pk1, pk2, pk2, pk1, pk1, pk1}
	msgs := [][]byte{msg1, msg2, msg1, msg3, msg1, msg4}
	assert.True(t, scheme.AggregateVerify(pks, msgs, aggSig))

	aggSigHex := aggSig.HexString()
	assert.Equal(t,
		aggSigHex,
		"a1d5360dcb418d33b29b90b912b4accde535cf0e52caf467a005dc632d9f7af44b6c4e9acd4"+
			"6eac218b28cdb07a3e3bc087df1cd1e3213aa4e11322a3ff3847bbba0b2fd19ddc25ca964871"+
			"997b9bceeab37a4c2565876da19382ea32a962200",
	)
}

func TestPopSchemeMPL(t *testing.T) {
	scheme := NewPopSchemeMPL()
	sk1, _ := scheme.KeyGen(genSeed(4))
	pop := scheme.PopProve(sk1)
	pk1, _ := sk1.G1Element()
	assert.True(t, scheme.PopVerify(pk1, pop))
	hexStrActual := pop.HexString()
	assert.Equal(t,
		hexStrActual,
		"84f709159435f0dc73b3e8bf6c78d85282d19231555a8ee3b6e2573aaf66872d9203fefa1ef"+
			"700e34e7c3f3fb28210100558c6871c53f1ef6055b9f06b0d1abe22ad584ad3b957f3018a8f5"+
			"8227c6c716b1e15791459850f2289168fa0cf9115",
	)
}

func TestAggSks(t *testing.T) {
	msg := []byte{100, 2, 254, 88, 90, 45, 23}
	scheme := NewBasicSchemeMPL()
	sk1, _ := scheme.KeyGen(genSeed(7))
	pk1, _ := sk1.G1Element()
	sk2, _ := scheme.KeyGen(genSeed(8))
	pk2, _ := sk2.G1Element()
	aggSk := PrivateKeyAggregate(sk1, sk2)
	aggSkAlt := PrivateKeyAggregate(sk2, sk1)

	assert.True(t, aggSk.EqualTo(aggSkAlt))

	aggPubKey := pk1.Add(pk2)
	aggPk, _ := aggSk.G1Element()
	assert.True(t, aggPubKey.EqualTo(aggPk))

	sig1 := scheme.Sign(sk1, msg)
	sig2 := scheme.Sign(sk2, msg)
	aggSig2 := scheme.Sign(aggSk, msg)
	aggSig := scheme.AggregateSigs(sig1, sig2)
	assert.True(t, aggSig.EqualTo(aggSig2))

	// Verify as a single G2Element
	assert.True(t, scheme.Verify(aggPubKey, msg, aggSig))
	assert.True(t, scheme.Verify(aggPubKey, msg, aggSig2))

	// Verify aggregate with both keys (Fails since not distinct)
	assert.False(t, scheme.AggregateVerify([]*G1Element{pk1, pk2}, [][]byte{msg, msg}, aggSig))
	assert.False(t, scheme.AggregateVerify([]*G1Element{pk1, pk2}, [][]byte{msg, msg}, aggSig2))

	msg2 := []byte{200, 29, 54, 8, 9, 29, 155, 55}
	sig3 := scheme.Sign(sk2, msg2)
	aggSigFinal := scheme.AggregateSigs(aggSig, sig3)
	aggSigAlt := scheme.AggregateSigs(sig1, sig2, sig3)
	aggSigAlt2 := scheme.AggregateSigs(sig1, sig3, sig2)
	assert.True(t, aggSigFinal.EqualTo(aggSigAlt))
	assert.True(t, aggSigFinal.EqualTo(aggSigAlt2))

	skFinal := PrivateKeyAggregate(aggSk, sk2)
	skFinalAlt := PrivateKeyAggregate(sk2, sk1, sk2)
	assert.True(t, skFinal.EqualTo(skFinalAlt))
	assert.True(t, !skFinal.EqualTo(aggSk))

	pkFinal := aggPubKey.Add(pk2)
	pkFinalAlt := pk2.Add(pk1).Add(pk2)
	assert.True(t, pkFinal.EqualTo(pkFinalAlt))
	assert.True(t, !pkFinal.EqualTo(aggPubKey))

	// Cannot verify with aggPubKey (since we have multiple messages)
	assert.True(t, scheme.AggregateVerify([]*G1Element{aggPubKey, pk2}, [][]byte{msg, msg2}, aggSigFinal))
}

func TestReadme(t *testing.T) {
	seed := []byte{
		0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192, 19, 18, 12, 89, 6,
		220, 18, 102, 58, 209, 82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22,
	}

	var scheme Scheme = NewAugSchemeMPL()
	sk, _ := scheme.KeyGen(seed)
	pk, _ := sk.G1Element()

	msg := []byte{1, 2, 3, 4, 5}
	sig := scheme.Sign(sk, msg)

	assert.True(t, scheme.Verify(pk, msg, sig))

	skRaw := sk.Serialize()   // 32 bytes
	pkRaw := pk.Serialize()   // 48 bytes
	sigRaw := sig.Serialize() // 96 bytes

	sk, _ = PrivateKeyFromBytes(skRaw, false)
	pk, _ = G1ElementFromBytes(pkRaw)
	sig, _ = G2ElementFromBytes(sigRaw)

	sk1, _ := scheme.KeyGen(replLeft(seed, []byte{1}))
	sk2, _ := scheme.KeyGen(replLeft(seed, []byte{2}))

	msg2 := []byte{1, 2, 3, 4, 5, 6, 7}

	pk1, _ := sk1.G1Element()
	sig1 := scheme.Sign(sk1, msg)

	pk2, _ := sk2.G1Element()
	sig2 := scheme.Sign(sk2, msg2)

	aggSig := scheme.AggregateSigs(sig1, sig2)

	assert.True(t, scheme.AggregateVerify([]*G1Element{pk1, pk2}, [][]byte{msg, msg2}, aggSig))

	sk3, _ := scheme.KeyGen(replLeft(seed, []byte{2}))
	pk3, _ := sk3.G1Element()
	msg3 := []byte{100, 2, 254, 88, 90, 45, 23}
	sig3 := scheme.Sign(sk3, msg3)

	aggSigFinal := scheme.AggregateSigs(aggSig, sig3)
	assert.True(t,
		scheme.AggregateVerify(
			[]*G1Element{pk1, pk2, pk3},
			[][]byte{msg, msg2, msg3},
			aggSigFinal,
		),
	)

	popScheme := NewPopSchemeMPL()
	popSig1 := popScheme.Sign(sk1, msg)
	popSig2 := popScheme.Sign(sk2, msg)
	popSig3 := popScheme.Sign(sk3, msg)
	pop1 := popScheme.PopProve(sk1)
	pop2 := popScheme.PopProve(sk2)
	pop3 := popScheme.PopProve(sk3)

	assert.True(t, popScheme.PopVerify(pk1, pop1))
	assert.True(t, popScheme.PopVerify(pk2, pop2))
	assert.True(t, popScheme.PopVerify(pk3, pop3))

	popSigAgg := popScheme.AggregateSigs(popSig1, popSig2, popSig3)

	assert.True(t, popScheme.FastAggregateVerify([]*G1Element{pk1, pk2, pk3}, msg, popSigAgg))

	popAggPk := pk1.Add(pk2).Add(pk3)

	tPopAggPk1 := pk1.Add(pk2.Add(pk3))
	assert.Equal(t, popAggPk.Serialize(), tPopAggPk1.Serialize())

	assert.True(t, popScheme.Verify(popAggPk, msg, popSigAgg))

	popAggSk := PrivateKeyAggregate(sk1, sk2, sk3)
	assert.True(t, popScheme.Sign(popAggSk, msg).EqualTo(popSigAgg))
	assert.Equal(t, popScheme.Sign(popAggSk, msg).Serialize(), popSigAgg.Serialize())

	masterSk, _ := scheme.KeyGen(seed)

	masterPk, _ := masterSk.G1Element()
	childU := scheme.DeriveChildSkUnhardened(masterSk, 22)
	grandChildU := scheme.DeriveChildSkUnhardened(childU, 0)

	childUPk := scheme.DeriveChildPkUnhardened(masterPk, 22)
	grandChildUPk := scheme.DeriveChildPkUnhardened(childUPk, 0)

	grandChildUPkAlt, _ := grandChildU.G1Element()
	assert.Equal(t, grandChildUPk.Serialize(), grandChildUPkAlt.Serialize())
}

func genSeed(v byte) []byte {
	return genByteSlice(v, 32)
}

func genByteSlice(v byte, l int) []byte {
	data := make([]byte, l)
	for i := 0; i < l; i++ {
		data[i] = v
	}
	return data
}

func replLeft(arr []byte, r []byte) []byte {
	return append(r, arr[len(r):]...)
}

func checkError(err error, wantErr string) bool {
	if err == nil && wantErr == "" {
		return true
	}
	if (err == nil && wantErr != "") || wantErr == "" {
		return false
	}
	return strings.Contains(err.Error(), wantErr)
}
