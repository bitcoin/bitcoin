/*
	cd bls
	make minimised_static
*/
package main

import (
	"crypto/rand"
	"fmt"
	"github.com/herumi/bls/ffi/go/bls"
)

type SeqRead struct {
}

func (self *SeqRead) Read(buf []byte) (int, error) {
	n := len(buf)
	for i := 0; i < n; i++ {
		buf[i] = byte(i)
	}
	return n, nil
}

func testReadRand() {
	s1 := new(SeqRead)
	bls.SetRandFunc(s1)
	var sec bls.SecretKey
	sec.SetByCSPRNG()
	buf := sec.GetLittleEndian()
	fmt.Printf("1. buf=%x\n", buf)
	for i := 0; i < len(buf); i++ {
		if buf[i] != byte(i) {
			fmt.Printf("err %d\n", i)
		}
	}
	bls.SetRandFunc(rand.Reader)
	sec.SetByCSPRNG()
	buf = sec.GetLittleEndian()
	fmt.Printf("2. (cr.Read) buf=%x\n", buf)
	bls.SetRandFunc(nil)
	sec.SetByCSPRNG()
	buf = sec.GetLittleEndian()
	fmt.Printf("3. (cr.Read) buf=%x\n", buf)
}

func main() {
	bls.Init(bls.BLS12_381)
	var sec bls.SecretKey
	sec.SetByCSPRNG()
	fmt.Printf("sec:%s\n", sec.SerializeToHexStr())
	pub := sec.GetPublicKey()
	fmt.Printf("pub:%s\n", pub.SerializeToHexStr())
	msgTbl := []string{"abc", "def", "123"}
	n := len(msgTbl)
	sigVec := make([]*bls.Sign, n)
	for i := 0; i < n; i++ {
		m := msgTbl[i]
		sigVec[i] = sec.Sign(m)
		fmt.Printf("%d. sign(%s)=%s\n", i, m, sigVec[i].SerializeToHexStr())
	}
	agg := sigVec[0]
	for i := 1; i < n; i++ {
		agg.Add(sigVec[i])
	}
	hashPt := bls.HashAndMapToSignature([]byte(msgTbl[0]))
	for i := 1; i < n; i++ {
		hashPt.Add(bls.HashAndMapToSignature([]byte(msgTbl[i])))
	}
	fmt.Printf("verify %t\n", bls.VerifyPairing(agg, hashPt, pub))
	testReadRand()
}
