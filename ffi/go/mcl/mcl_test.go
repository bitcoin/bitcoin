package mcl

import "testing"
import "fmt"
import "encoding/hex"

func testBadPointOfG2(t *testing.T) {
	var Q G2
	// this value is not in G2 so should return an error
	err := Q.SetString("1 18d3d8c085a5a5e7553c3a4eb628e88b8465bf4de2612e35a0a4eb018fb0c82e9698896031e62fd7633ffd824a859474 1dc6edfcf33e29575d4791faed8e7203832217423bf7f7fbf1f6b36625b12e7132c15fbc15562ce93362a322fb83dd0d 65836963b1f7b6959030ddfa15ab38ce056097e91dedffd996c1808624fa7e2644a77be606290aa555cda8481cfb3cb 1b77b708d3d4f65aeedf54b58393463a42f0dc5856baadb5ce608036baeca398c5d9e6b169473a8838098fd72fd28b50", 16)
	if err == nil {
		t.Error(err)
	}
}

func testGT(t *testing.T) {
	var x GT
	x.Clear()
	if !x.IsZero() {
		t.Errorf("not zero")
	}
	x.SetInt64(1)
	if !x.IsOne() {
		t.Errorf("not one")
	}
}

func testHash(t *testing.T) {
	var x Fr
	if !x.SetHashOf([]byte("abc")) {
		t.Error("SetHashOf")
	}
	fmt.Printf("x=%s\n", x.GetString(16))
}

func testNegAdd(t *testing.T) {
	var x Fr
	var P1, P2, P3 G1
	var Q1, Q2, Q3 G2
	err := P1.HashAndMapTo([]byte("this"))
	if err != nil {
		t.Error(err)
	}
	err = Q1.HashAndMapTo([]byte("this"))
	if err != nil {
		t.Error(err)
	}
	fmt.Printf("P1=%s\n", P1.GetString(16))
	fmt.Printf("Q1=%s\n", Q1.GetString(16))
	G1Neg(&P2, &P1)
	G2Neg(&Q2, &Q1)
	fmt.Printf("P2=%s\n", P2.GetString(16))
	fmt.Printf("Q2=%s\n", Q2.GetString(16))

	x.SetInt64(-1)
	G1Mul(&P3, &P1, &x)
	G2Mul(&Q3, &Q1, &x)
	if !P2.IsEqual(&P3) {
		t.Errorf("P2 != P3 %s\n", P3.GetString(16))
	}
	if !Q2.IsEqual(&Q3) {
		t.Errorf("Q2 != Q3 %s\n", Q3.GetString(16))
	}

	G1Add(&P2, &P2, &P1)
	G2Add(&Q2, &Q2, &Q1)
	if !P2.IsZero() {
		t.Errorf("P2 is not zero %s\n", P2.GetString(16))
	}
	if !Q2.IsZero() {
		t.Errorf("Q2 is not zero %s\n", Q2.GetString(16))
	}
}

func testVecG1(t *testing.T) {
	N := 50
	xVec := make([]G1, N)
	yVec := make([]Fr, N)
	xVec[0].HashAndMapTo([]byte("aa"))
	var R1, R2 G1
	for i := 0; i < N; i++ {
		if i > 0 {
			G1Dbl(&xVec[i], &xVec[i-1])
		}
		yVec[i].SetByCSPRNG()
		G1Mul(&R1, &xVec[i], &yVec[i])
		G1Add(&R2, &R2, &R1)
	}
	G1MulVec(&R1, xVec, yVec)
	if !R1.IsEqual(&R2) {
		t.Errorf("wrong G1MulVec")
	}
}

func testVecG2(t *testing.T) {
	N := 50
	xVec := make([]G2, N)
	yVec := make([]Fr, N)
	xVec[0].HashAndMapTo([]byte("aa"))
	var R1, R2 G2
	for i := 0; i < N; i++ {
		if i > 0 {
			G2Dbl(&xVec[i], &xVec[i-1])
		}
		yVec[i].SetByCSPRNG()
		G2Mul(&R1, &xVec[i], &yVec[i])
		G2Add(&R2, &R2, &R1)
	}
	G2MulVec(&R1, xVec, yVec)
	if !R1.IsEqual(&R2) {
		t.Errorf("wrong G2MulVec")
	}
}

func testVecPairing(t *testing.T) {
	N := 50
	xVec := make([]G1, N)
	yVec := make([]G2, N)
	var e1, e2 GT
	e1.SetInt64(1)
	for i := 0; i < N; i++ {
		xVec[0].HashAndMapTo([]byte("aa"))
		yVec[0].HashAndMapTo([]byte("aa"))
		Pairing(&e2, &xVec[i], &yVec[i])
		GTMul(&e1, &e1, &e2)
	}
	MillerLoopVec(&e2, xVec, yVec)
	FinalExp(&e2, &e2)
	if !e1.IsEqual(&e2) {
		t.Errorf("wrong MillerLoopVec")
	}
}

func testVec(t *testing.T) {
	testVecG1(t)
	testVecG2(t)
	testVecPairing(t)
}

func testPairing(t *testing.T) {
	var a, b, ab Fr
	err := a.SetString("123", 10)
	if err != nil {
		t.Error(err)
		return
	}
	err = b.SetString("456", 10)
	if err != nil {
		t.Error(err)
		return
	}
	FrMul(&ab, &a, &b)
	var P, aP G1
	var Q, bQ G2
	err = P.HashAndMapTo([]byte("this"))
	if err != nil {
		t.Error(err)
		return
	}
	fmt.Printf("P=%s\n", P.GetString(16))
	G1Mul(&aP, &P, &a)
	fmt.Printf("aP=%s\n", aP.GetString(16))
	err = Q.HashAndMapTo([]byte("that"))
	if err != nil {
		t.Error(err)
		return
	}
	fmt.Printf("Q=%s\n", Q.GetString(16))
	G2Mul(&bQ, &Q, &b)
	fmt.Printf("bQ=%s\n", bQ.GetString(16))
	var e1, e2 GT
	Pairing(&e1, &P, &Q)
	fmt.Printf("e1=%s\n", e1.GetString(16))
	Pairing(&e2, &aP, &bQ)
	fmt.Printf("e2=%s\n", e1.GetString(16))
	GTPow(&e1, &e1, &ab)
	fmt.Printf("e1=%s\n", e1.GetString(16))
	if !e1.IsEqual(&e2) {
		t.Errorf("not equal pairing\n%s\n%s", e1.GetString(16), e2.GetString(16))
	}
	{
		s := P.GetString(IoSerializeHexStr)
		var P1 G1
		P1.SetString(s, IoSerializeHexStr)
		if !P1.IsEqual(&P) {
			t.Error("not equal to P")
			return
		}
		s = Q.GetString(IoSerializeHexStr)
		var Q1 G2
		Q1.SetString(s, IoSerializeHexStr)
		if !Q1.IsEqual(&Q) {
			t.Error("not equal to Q")
			return
		}
	}
}

func testSerialize(t *testing.T) {
	var x, xx Fr
	var y, yy Fp
	var P, PP G1
	var Q, QQ G2
	var e, ee GT
	x.SetByCSPRNG()
	y.SetByCSPRNG()
	P.HashAndMapTo([]byte("abc"))
	G1Dbl(&P, &P)
	Q.HashAndMapTo([]byte("abc"))
	G2Dbl(&Q, &Q)
	Pairing(&e, &P, &Q)
	if xx.Deserialize(x.Serialize()) != nil || !x.IsEqual(&xx) {
		t.Error("Serialize Fr")
	}
	if yy.Deserialize(y.Serialize()) != nil || !y.IsEqual(&yy) {
		t.Error("Serialize Fp")
	}
	if PP.Deserialize(P.Serialize()) != nil || !P.IsEqual(&PP) {
		t.Error("Serialize G1")
	}
	if QQ.Deserialize(Q.Serialize()) != nil || !Q.IsEqual(&QQ) {
		t.Error("Serialize G2")
	}
	if ee.Deserialize(e.Serialize()) != nil || !e.IsEqual(&ee) {
		t.Error("Serialize GT")
	}
	G1Dbl(&PP, &PP)
	if PP.DeserializeUncompressed(P.SerializeUncompressed()) != nil || !P.IsEqual(&PP) {
		t.Error("SerializeUncompressed G1")
	}
	G2Dbl(&QQ, &QQ)
	if QQ.DeserializeUncompressed(Q.SerializeUncompressed()) != nil || !Q.IsEqual(&QQ) {
		t.Error("SerializeUncompressed G2")
	}
}

func testMcl(t *testing.T, c int) {
	err := Init(c)
	if err != nil {
		t.Fatal(err)
	}
	testHash(t)
	testNegAdd(t)
	testPairing(t)
	testVec(t)
	testGT(t)
	testBadPointOfG2(t)
	testSerialize(t)
}

func testETHserialize(t *testing.T) {
	b := make([]byte, 32)
	b[0] = 0x12
	b[1] = 0x34
	var x Fr
	SetETHserialization(false)
	x.Deserialize(b)
	fmt.Printf("AAA x=%s\n", x.GetString(16))

	SetETHserialization(true)
	x.Deserialize(b)
	fmt.Printf("AAA x=%s\n", x.GetString(16))
}

func testMapToG1(t *testing.T) {
	SetMapToMode(IRTF)
	fpHex := "0000000000000000000000000000000014406e5bfb9209256a3820879a29ac2f62d6aca82324bf3ae2aa7d3c54792043bd8c791fccdb080c1a52dc68b8b69350"
	g1Hex := "ad7721bcdb7ce1047557776eb2659a444166dc6dd55c7ca6e240e21ae9aa18f529f04ac31d861b54faf3307692545db7"
	fpBin, _ := hex.DecodeString(fpHex)
	var x Fp
	x.SetBigEndianMod(fpBin)
	var P1 G1
	err := MapToG1(&P1, &x)
	if err != nil {
		t.Fatal("MapToG1")
	}
	g1Str, _ := hex.DecodeString(g1Hex)
	var P2 G1
	err = P2.Deserialize(g1Str)
	if err != nil {
		t.Fatal("G1.Deserialize")
	}
	if !P1.IsEqual(&P2) {
		t.Fatal("bad MapToG1")
	}
}

func TestMclMain(t *testing.T) {
	t.Logf("GetMaxOpUnitSize() = %d\n", GetMaxOpUnitSize())
	t.Log("CurveFp254BNb")
	testMcl(t, CurveFp254BNb)
	if GetMaxOpUnitSize() == 6 {
		if GetFrUnitSize() == 6 {
			t.Log("CurveFp382_1")
			testMcl(t, CurveFp382_1)
		}
		t.Log("BLS12_381")
		testMcl(t, BLS12_381)
		testETHserialize(t)
		testMapToG1(t)
	}
}
