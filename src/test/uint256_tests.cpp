// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
#include "uint256.h"
#include <string>
#include "version.h"

BOOST_AUTO_TEST_SUITE(uint256_tests)
 
const unsigned char R1Array[] = 
    "\x9c\x52\x4a\xdb\xcf\x56\x11\x12\x2b\x29\x12\x5e\x5d\x35\xd2\xd2"
    "\x22\x81\xaa\xb5\x33\xf0\x08\x32\xd5\x56\xb1\xf9\xea\xe5\x1d\x7d";
const char R1ArrayHex[] = "7D1DE5EAF9B156D53208F033B5AA8122D2d2355d5e12292b121156cfdb4a529c";
const double R1Ldouble = 0.4887374590559308955; // R1L equals roughly R1Ldouble * 2^256
const double R1Sdouble = 0.7096329412477836074; 
const uint256 R1L = uint256(std::vector<unsigned char>(R1Array,R1Array+32));
const uint160 R1S = uint160(std::vector<unsigned char>(R1Array,R1Array+20));
const uint64_t R1LLow64 = 0x121156cfdb4a529cULL;

const unsigned char R2Array[] = 
    "\x70\x32\x1d\x7c\x47\xa5\x6b\x40\x26\x7e\x0a\xc3\xa6\x9c\xb6\xbf"
    "\x13\x30\x47\xa3\x19\x2d\xda\x71\x49\x13\x72\xf0\xb4\xca\x81\xd7";
const uint256 R2L = uint256(std::vector<unsigned char>(R2Array,R2Array+32));
const uint160 R2S = uint160(std::vector<unsigned char>(R2Array,R2Array+20));

const char R1LplusR2L[] = "549FB09FEA236A1EA3E31D4D58F1B1369288D204211CA751527CFC175767850C";

const unsigned char ZeroArray[] = 
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
const uint256 ZeroL = uint256(std::vector<unsigned char>(ZeroArray,ZeroArray+32));
const uint160 ZeroS = uint160(std::vector<unsigned char>(ZeroArray,ZeroArray+20));
                             
const unsigned char OneArray[] = 
    "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
const uint256 OneL = uint256(std::vector<unsigned char>(OneArray,OneArray+32));
const uint160 OneS = uint160(std::vector<unsigned char>(OneArray,OneArray+20));

const unsigned char MaxArray[] = 
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
const uint256 MaxL = uint256(std::vector<unsigned char>(MaxArray,MaxArray+32));
const uint160 MaxS = uint160(std::vector<unsigned char>(MaxArray,MaxArray+20));

const uint256 HalfL = (OneL << 255);
const uint160 HalfS = (OneS << 159);
std::string ArrayToString(const unsigned char A[], unsigned int width)
{
    std::stringstream Stream;
    Stream << std::hex;
    for (unsigned int i = 0; i < width; ++i) 
    {
        Stream<<std::setw(2)<<std::setfill('0')<<(unsigned int)A[width-i-1];
    }       
    return Stream.str();
}

BOOST_AUTO_TEST_CASE( basics ) // constructors, equality, inequality
{
    BOOST_CHECK(1 == 0+1);
    // constructor uint256(vector<char>):
    BOOST_CHECK(R1L.ToString() == ArrayToString(R1Array,32));
    BOOST_CHECK(R1S.ToString() == ArrayToString(R1Array,20));
    BOOST_CHECK(R2L.ToString() == ArrayToString(R2Array,32));
    BOOST_CHECK(R2S.ToString() == ArrayToString(R2Array,20));
    BOOST_CHECK(ZeroL.ToString() == ArrayToString(ZeroArray,32));
    BOOST_CHECK(ZeroS.ToString() == ArrayToString(ZeroArray,20));
    BOOST_CHECK(OneL.ToString() == ArrayToString(OneArray,32));
    BOOST_CHECK(OneS.ToString() == ArrayToString(OneArray,20));
    BOOST_CHECK(MaxL.ToString() == ArrayToString(MaxArray,32));
    BOOST_CHECK(MaxS.ToString() == ArrayToString(MaxArray,20));
    BOOST_CHECK(OneL.ToString() != ArrayToString(ZeroArray,32));
    BOOST_CHECK(OneS.ToString() != ArrayToString(ZeroArray,20));

    // == and !=
    BOOST_CHECK(R1L != R2L && R1S != R2S);
    BOOST_CHECK(ZeroL != OneL && ZeroS != OneS);
    BOOST_CHECK(OneL != ZeroL && OneS != ZeroS);
    BOOST_CHECK(MaxL != ZeroL && MaxS != ZeroS);
    BOOST_CHECK(~MaxL == ZeroL && ~MaxS == ZeroS);
    BOOST_CHECK( ((R1L ^ R2L) ^ R1L) == R2L);
    BOOST_CHECK( ((R1S ^ R2S) ^ R1S) == R2S);
    
    uint64_t Tmp64 = 0xc4dab720d9c7acaaULL;
    for (unsigned int i = 0; i < 256; ++i) 
    {
        BOOST_CHECK(ZeroL != (OneL << i)); 
        BOOST_CHECK((OneL << i) != ZeroL); 
        BOOST_CHECK(R1L != (R1L ^ (OneL << i)));
        BOOST_CHECK(((uint256(Tmp64) ^ (OneL << i) ) != Tmp64 ));
    }
    BOOST_CHECK(ZeroL == (OneL << 256)); 

    for (unsigned int i = 0; i < 160; ++i) 
    {
        BOOST_CHECK(ZeroS != (OneS << i)); 
        BOOST_CHECK((OneS << i) != ZeroS); 
        BOOST_CHECK(R1S != (R1S ^ (OneS << i)));
        BOOST_CHECK(((uint160(Tmp64) ^ (OneS << i) ) != Tmp64 ));
    }
    BOOST_CHECK(ZeroS == (OneS << 256)); 

    // String Constructor and Copy Constructor
    BOOST_CHECK(uint256("0x"+R1L.ToString()) == R1L);
    BOOST_CHECK(uint256("0x"+R2L.ToString()) == R2L);
    BOOST_CHECK(uint256("0x"+ZeroL.ToString()) == ZeroL);
    BOOST_CHECK(uint256("0x"+OneL.ToString()) == OneL);
    BOOST_CHECK(uint256("0x"+MaxL.ToString()) == MaxL);
    BOOST_CHECK(uint256(R1L.ToString()) == R1L);
    BOOST_CHECK(uint256("   0x"+R1L.ToString()+"   ") == R1L);
    BOOST_CHECK(uint256("") == ZeroL);
    BOOST_CHECK(R1L == uint256(R1ArrayHex));
    BOOST_CHECK(uint256(R1L) == R1L);
    BOOST_CHECK((uint256(R1L^R2L)^R2L) == R1L);
    BOOST_CHECK(uint256(ZeroL) == ZeroL);
    BOOST_CHECK(uint256(OneL) == OneL);

    BOOST_CHECK(uint160("0x"+R1S.ToString()) == R1S);
    BOOST_CHECK(uint160("0x"+R2S.ToString()) == R2S);
    BOOST_CHECK(uint160("0x"+ZeroS.ToString()) == ZeroS);
    BOOST_CHECK(uint160("0x"+OneS.ToString()) == OneS);
    BOOST_CHECK(uint160("0x"+MaxS.ToString()) == MaxS);
    BOOST_CHECK(uint160(R1S.ToString()) == R1S);
    BOOST_CHECK(uint160("   0x"+R1S.ToString()+"   ") == R1S); 
    BOOST_CHECK(uint160("") == ZeroS);
    BOOST_CHECK(R1S == uint160(R1ArrayHex));

    BOOST_CHECK(uint160(R1S) == R1S);
    BOOST_CHECK((uint160(R1S^R2S)^R2S) == R1S);
    BOOST_CHECK(uint160(ZeroS) == ZeroS);
    BOOST_CHECK(uint160(OneS) == OneS);

    // uint64_t constructor
    BOOST_CHECK( (R1L & uint256("0xffffffffffffffff")) == uint256(R1LLow64));
    BOOST_CHECK(ZeroL == uint256(0));
    BOOST_CHECK(OneL == uint256(1));
    BOOST_CHECK(uint256("0xffffffffffffffff") = uint256(0xffffffffffffffffULL));
    BOOST_CHECK( (R1S & uint160("0xffffffffffffffff")) == uint160(R1LLow64));
    BOOST_CHECK(ZeroS == uint160(0));
    BOOST_CHECK(OneS == uint160(1));
    BOOST_CHECK(uint160("0xffffffffffffffff") = uint160(0xffffffffffffffffULL));

    // Assignment (from base_uint)
    uint256 tmpL = ~ZeroL; BOOST_CHECK(tmpL == ~ZeroL);
    tmpL = ~OneL; BOOST_CHECK(tmpL == ~OneL);
    tmpL = ~R1L; BOOST_CHECK(tmpL == ~R1L);
    tmpL = ~R2L; BOOST_CHECK(tmpL == ~R2L);
    tmpL = ~MaxL; BOOST_CHECK(tmpL == ~MaxL);
    uint160 tmpS = ~ZeroS; BOOST_CHECK(tmpS == ~ZeroS);
    tmpS = ~OneS; BOOST_CHECK(tmpS == ~OneS);
    tmpS = ~R1S; BOOST_CHECK(tmpS == ~R1S);
    tmpS = ~R2S; BOOST_CHECK(tmpS == ~R2S);
    tmpS = ~MaxS; BOOST_CHECK(tmpS == ~MaxS);

    // Wrong length must give 0
    BOOST_CHECK(uint256(std::vector<unsigned char>(OneArray,OneArray+31)) == 0);
    BOOST_CHECK(uint256(std::vector<unsigned char>(OneArray,OneArray+20)) == 0);
    BOOST_CHECK(uint160(std::vector<unsigned char>(OneArray,OneArray+32)) == 0);
    BOOST_CHECK(uint160(std::vector<unsigned char>(OneArray,OneArray+19)) == 0);
}

void shiftArrayRight(unsigned char* to, const unsigned char* from, unsigned int arrayLength, unsigned int bitsToShift) 
{
    for (unsigned int T=0; T < arrayLength; ++T) 
    {
        unsigned int F = (T+bitsToShift/8);
        if (F < arrayLength) 
            to[T]  = from[F] >> (bitsToShift%8);
        else
            to[T] = 0;
        if (F + 1 < arrayLength) 
            to[T] |= from[(F+1)] << (8-bitsToShift%8);
    }
}

void shiftArrayLeft(unsigned char* to, const unsigned char* from, unsigned int arrayLength, unsigned int bitsToShift) 
{
    for (unsigned int T=0; T < arrayLength; ++T) 
    {
        if (T >= bitsToShift/8) 
        {
            unsigned int F = T-bitsToShift/8;
            to[T]  = from[F] << (bitsToShift%8);
            if (T >= bitsToShift/8+1)
                to[T] |= from[F-1] >> (8-bitsToShift%8);
        }
        else {
            to[T] = 0;
        }
    }
}

BOOST_AUTO_TEST_CASE( shifts ) { // "<<"  ">>"  "<<="  ">>="
    unsigned char TmpArray[32];
    uint256 TmpL;
    for (unsigned int i = 0; i < 256; ++i)
    {
        shiftArrayLeft(TmpArray, OneArray, 32, i);
        BOOST_CHECK(uint256(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (OneL << i));
        TmpL = OneL; TmpL <<= i;
        BOOST_CHECK(TmpL == (OneL << i));
        BOOST_CHECK((HalfL >> (255-i)) == (OneL << i));
        TmpL = HalfL; TmpL >>= (255-i);
        BOOST_CHECK(TmpL == (OneL << i));
                    
        shiftArrayLeft(TmpArray, R1Array, 32, i);
        BOOST_CHECK(uint256(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (R1L << i));
        TmpL = R1L; TmpL <<= i;
        BOOST_CHECK(TmpL == (R1L << i));

        shiftArrayRight(TmpArray, R1Array, 32, i);
        BOOST_CHECK(uint256(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (R1L >> i)); 
        TmpL = R1L; TmpL >>= i;
        BOOST_CHECK(TmpL == (R1L >> i));

        shiftArrayLeft(TmpArray, MaxArray, 32, i);
        BOOST_CHECK(uint256(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (MaxL << i));
        TmpL = MaxL; TmpL <<= i;
        BOOST_CHECK(TmpL == (MaxL << i));

        shiftArrayRight(TmpArray, MaxArray, 32, i);
        BOOST_CHECK(uint256(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (MaxL >> i));
        TmpL = MaxL; TmpL >>= i;
        BOOST_CHECK(TmpL == (MaxL >> i));
    }
    uint256 c1L = uint256(0x0123456789abcdefULL);
    uint256 c2L = c1L << 128;
    for (unsigned int i = 0; i < 128; ++i) {
        BOOST_CHECK((c1L << i) == (c2L >> (128-i)));
    }
    for (unsigned int i = 128; i < 256; ++i) {
        BOOST_CHECK((c1L << i) == (c2L << (i-128)));
    }

    uint160 TmpS;
    for (unsigned int i = 0; i < 160; ++i)
    {
        shiftArrayLeft(TmpArray, OneArray, 20, i);
        BOOST_CHECK(uint160(std::vector<unsigned char>(TmpArray,TmpArray+20)) == (OneS << i));
        TmpS = OneS; TmpS <<= i;
        BOOST_CHECK(TmpS == (OneS << i));
        BOOST_CHECK((HalfS >> (159-i)) == (OneS << i));
        TmpS = HalfS; TmpS >>= (159-i);
        BOOST_CHECK(TmpS == (OneS << i));
                    
        shiftArrayLeft(TmpArray, R1Array, 20, i);
        BOOST_CHECK(uint160(std::vector<unsigned char>(TmpArray,TmpArray+20)) == (R1S << i));
        TmpS = R1S; TmpS <<= i;
        BOOST_CHECK(TmpS == (R1S << i));

        shiftArrayRight(TmpArray, R1Array, 20, i);
        BOOST_CHECK(uint160(std::vector<unsigned char>(TmpArray,TmpArray+20)) == (R1S >> i)); 
        TmpS = R1S; TmpS >>= i;
        BOOST_CHECK(TmpS == (R1S >> i));

        shiftArrayLeft(TmpArray, MaxArray, 20, i);
        BOOST_CHECK(uint160(std::vector<unsigned char>(TmpArray,TmpArray+20)) == (MaxS << i));
        TmpS = MaxS; TmpS <<= i;
        BOOST_CHECK(TmpS == (MaxS << i));

        shiftArrayRight(TmpArray, MaxArray, 20, i);
        BOOST_CHECK(uint160(std::vector<unsigned char>(TmpArray,TmpArray+20)) == (MaxS >> i));
        TmpS = MaxS; TmpS >>= i;
        BOOST_CHECK(TmpS == (MaxS >> i));
    }
    uint160 c1S = uint160(0x0123456789abcdefULL);
    uint160 c2S = c1S << 80;
    for (unsigned int i = 0; i < 80; ++i) {
        BOOST_CHECK((c1S << i) == (c2S >> (80-i)));
    }
    for (unsigned int i = 80; i < 160; ++i) {
        BOOST_CHECK((c1S << i) == (c2S << (i-80)));
    }
}

BOOST_AUTO_TEST_CASE( unaryOperators ) // !    ~    -
{
    BOOST_CHECK(!ZeroL);  BOOST_CHECK(!ZeroS);
    BOOST_CHECK(!(!OneL));BOOST_CHECK(!(!OneS));
    for (unsigned int i = 0; i < 256; ++i) 
        BOOST_CHECK(!(!(OneL<<i)));
    for (unsigned int i = 0; i < 160; ++i) 
        BOOST_CHECK(!(!(OneS<<i)));
    BOOST_CHECK(!(!R1L));BOOST_CHECK(!(!R1S));
    BOOST_CHECK(!(!R2S));BOOST_CHECK(!(!R2S)); 
    BOOST_CHECK(!(!MaxL));BOOST_CHECK(!(!MaxS));

    BOOST_CHECK(~ZeroL == MaxL); BOOST_CHECK(~ZeroS == MaxS);

    unsigned char TmpArray[32];
    for (unsigned int i = 0; i < 32; ++i) { TmpArray[i] = ~R1Array[i]; } 
    BOOST_CHECK(uint256(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (~R1L));
    BOOST_CHECK(uint160(std::vector<unsigned char>(TmpArray,TmpArray+20)) == (~R1S));

    BOOST_CHECK(-ZeroL == ZeroL); BOOST_CHECK(-ZeroS == ZeroS);
    BOOST_CHECK(-R1L == (~R1L)+1);
    BOOST_CHECK(-R1S == (~R1S)+1);
    for (unsigned int i = 0; i < 256; ++i) 
        BOOST_CHECK(-(OneL<<i) == (MaxL << i));
    for (unsigned int i = 0; i < 160; ++i) 
        BOOST_CHECK(-(OneS<<i) == (MaxS << i));
}


// Check if doing _A_ _OP_ _B_ results in the same as applying _OP_ onto each
// element of Aarray and Barray, and then converting the result into a uint256.
#define CHECKBITWISEOPERATOR(_A_,_B_,_OP_)                              \
    for (unsigned int i = 0; i < 32; ++i) { TmpArray[i] = _A_##Array[i] _OP_ _B_##Array[i]; } \
    BOOST_CHECK(uint256(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (_A_##L _OP_ _B_##L)); \
    for (unsigned int i = 0; i < 20; ++i) { TmpArray[i] = _A_##Array[i] _OP_ _B_##Array[i]; } \
    BOOST_CHECK(uint160(std::vector<unsigned char>(TmpArray,TmpArray+20)) == (_A_##S _OP_ _B_##S));

#define CHECKASSIGNMENTOPERATOR(_A_,_B_,_OP_)                           \
    TmpL = _A_##L; TmpL _OP_##= _B_##L; BOOST_CHECK(TmpL == (_A_##L _OP_ _B_##L)); \
    TmpS = _A_##S; TmpS _OP_##= _B_##S; BOOST_CHECK(TmpS == (_A_##S _OP_ _B_##S));

BOOST_AUTO_TEST_CASE( bitwiseOperators ) 
{
    unsigned char TmpArray[32];
    
    CHECKBITWISEOPERATOR(R1,R2,|)
    CHECKBITWISEOPERATOR(R1,R2,^)
    CHECKBITWISEOPERATOR(R1,R2,&)
    CHECKBITWISEOPERATOR(R1,Zero,|)
    CHECKBITWISEOPERATOR(R1,Zero,^)
    CHECKBITWISEOPERATOR(R1,Zero,&)
    CHECKBITWISEOPERATOR(R1,Max,|)
    CHECKBITWISEOPERATOR(R1,Max,^)
    CHECKBITWISEOPERATOR(R1,Max,&)
    CHECKBITWISEOPERATOR(Zero,R1,|)
    CHECKBITWISEOPERATOR(Zero,R1,^)
    CHECKBITWISEOPERATOR(Zero,R1,&)
    CHECKBITWISEOPERATOR(Max,R1,|)
    CHECKBITWISEOPERATOR(Max,R1,^)
    CHECKBITWISEOPERATOR(Max,R1,&)

    uint256 TmpL;
    uint160 TmpS;
    CHECKASSIGNMENTOPERATOR(R1,R2,|)
    CHECKASSIGNMENTOPERATOR(R1,R2,^)
    CHECKASSIGNMENTOPERATOR(R1,R2,&)
    CHECKASSIGNMENTOPERATOR(R1,Zero,|)
    CHECKASSIGNMENTOPERATOR(R1,Zero,^)
    CHECKASSIGNMENTOPERATOR(R1,Zero,&)
    CHECKASSIGNMENTOPERATOR(R1,Max,|)
    CHECKASSIGNMENTOPERATOR(R1,Max,^)
    CHECKASSIGNMENTOPERATOR(R1,Max,&)
    CHECKASSIGNMENTOPERATOR(Zero,R1,|)
    CHECKASSIGNMENTOPERATOR(Zero,R1,^)
    CHECKASSIGNMENTOPERATOR(Zero,R1,&)
    CHECKASSIGNMENTOPERATOR(Max,R1,|)
    CHECKASSIGNMENTOPERATOR(Max,R1,^)
    CHECKASSIGNMENTOPERATOR(Max,R1,&)

    uint64_t Tmp64 = 0xe1db685c9a0b47a2ULL; 
    TmpL = R1L; TmpL |= Tmp64;  BOOST_CHECK(TmpL == (R1L | uint256(Tmp64)));
    TmpS = R1S; TmpS |= Tmp64;  BOOST_CHECK(TmpS == (R1S | uint160(Tmp64)));
    TmpL = R1L; TmpL |= 0; BOOST_CHECK(TmpL == R1L);
    TmpS = R1S; TmpS |= 0; BOOST_CHECK(TmpS == R1S);
    TmpL ^= 0; BOOST_CHECK(TmpL == R1L);
    TmpS ^= 0; BOOST_CHECK(TmpS == R1S);
    TmpL ^= Tmp64;  BOOST_CHECK(TmpL == (R1L ^ uint256(Tmp64)));
    TmpS ^= Tmp64;  BOOST_CHECK(TmpS == (R1S ^ uint160(Tmp64)));
}

BOOST_AUTO_TEST_CASE( comparison ) // <= >= < >
{
    uint256 TmpL;
    for (unsigned int i = 0; i < 256; ++i) {
        TmpL= OneL<< i;
        BOOST_CHECK( TmpL >= ZeroL && TmpL > ZeroL && ZeroL < TmpL && ZeroL <= TmpL);
        BOOST_CHECK( TmpL >= 0 && TmpL > 0 && 0 < TmpL && 0 <= TmpL);
        TmpL |= R1L;
        BOOST_CHECK( TmpL >= R1L ); BOOST_CHECK( (TmpL == R1L) != (TmpL > R1L)); BOOST_CHECK( (TmpL == R1L) || !( TmpL <= R1L));
        BOOST_CHECK( R1L <= TmpL ); BOOST_CHECK( (R1L == TmpL) != (R1L < TmpL)); BOOST_CHECK( (TmpL == R1L) || !( R1L >= TmpL));
        BOOST_CHECK(! (TmpL < R1L)); BOOST_CHECK(! (R1L > TmpL));
    }
    uint160 TmpS;
    for (unsigned int i = 0; i < 160; ++i) {
        TmpS= OneS<< i;
        BOOST_CHECK( TmpS >= ZeroS && TmpS > ZeroS && ZeroS < TmpS && ZeroS <= TmpS);
        BOOST_CHECK( TmpS >= 0 && TmpS > 0 && 0 < TmpS && 0 <= TmpS);
        TmpS |= R1S;
        BOOST_CHECK( TmpS >= R1S ); BOOST_CHECK( (TmpS == R1S) != (TmpS > R1S)); BOOST_CHECK( (TmpS == R1S) || !( TmpS <= R1S));
        BOOST_CHECK( R1S <= TmpS ); BOOST_CHECK( (R1S == TmpS) != (R1S < TmpS)); BOOST_CHECK( (TmpS == R1S) || !( R1S >= TmpS));
        BOOST_CHECK(! (TmpS < R1S)); BOOST_CHECK(! (R1S > TmpS));
    }
}

BOOST_AUTO_TEST_CASE( plusMinus ) 
{
    uint256 TmpL = 0;
    BOOST_CHECK(R1L+R2L == uint256(R1LplusR2L));
    TmpL += R1L;
    BOOST_CHECK(TmpL == R1L);
    TmpL += R2L;
    BOOST_CHECK(TmpL == R1L + R2L);
    BOOST_CHECK(OneL+MaxL == ZeroL);
    BOOST_CHECK(MaxL+OneL == ZeroL);
    for (unsigned int i = 1; i < 256; ++i) {
        BOOST_CHECK( (MaxL >> i) + OneL == (HalfL >> (i-1)) );
        BOOST_CHECK( OneL + (MaxL >> i) == (HalfL >> (i-1)) );
        TmpL = (MaxL>>i); TmpL += OneL;
        BOOST_CHECK( TmpL == (HalfL >> (i-1)) );
        TmpL = (MaxL>>i); TmpL += 1;
        BOOST_CHECK( TmpL == (HalfL >> (i-1)) );
        TmpL = (MaxL>>i); 
        BOOST_CHECK( TmpL++ == (MaxL>>i) );
        BOOST_CHECK( TmpL == (HalfL >> (i-1)));
    }
    BOOST_CHECK(uint256(0xbedc77e27940a7ULL) + 0xee8d836fce66fbULL == uint256(0xbedc77e27940a7ULL + 0xee8d836fce66fbULL));
    TmpL = uint256(0xbedc77e27940a7ULL); TmpL += 0xee8d836fce66fbULL;
    BOOST_CHECK(TmpL == uint256(0xbedc77e27940a7ULL+0xee8d836fce66fbULL));
    TmpL -= 0xee8d836fce66fbULL;  BOOST_CHECK(TmpL == 0xbedc77e27940a7ULL);
    TmpL = R1L;
    BOOST_CHECK(++TmpL == R1L+1);

    BOOST_CHECK(R1L -(-R2L) == R1L+R2L);
    BOOST_CHECK(R1L -(-OneL) == R1L+OneL);
    BOOST_CHECK(R1L - OneL == R1L+(-OneL));
    for (unsigned int i = 1; i < 256; ++i) {
        BOOST_CHECK((MaxL>>i) - (-OneL)  == (HalfL >> (i-1)));
        BOOST_CHECK((HalfL >> (i-1)) - OneL == (MaxL>>i));
        TmpL = (HalfL >> (i-1));
        BOOST_CHECK(TmpL-- == (HalfL >> (i-1)));
        BOOST_CHECK(TmpL == (MaxL >> i));
        TmpL = (HalfL >> (i-1));
        BOOST_CHECK(--TmpL == (MaxL >> i));
    }
    TmpL = R1L;
    BOOST_CHECK(--TmpL == R1L-1);

    // 160-bit; copy-pasted
    uint160 TmpS = 0;
    BOOST_CHECK(R1S+R2S == uint160(R1LplusR2L));
    TmpS += R1S;
    BOOST_CHECK(TmpS == R1S);
    TmpS += R2S;
    BOOST_CHECK(TmpS == R1S + R2S);
    BOOST_CHECK(OneS+MaxS == ZeroS);
    BOOST_CHECK(MaxS+OneS == ZeroS);
    for (unsigned int i = 1; i < 160; ++i) {
        BOOST_CHECK( (MaxS >> i) + OneS == (HalfS >> (i-1)) );
        BOOST_CHECK( OneS + (MaxS >> i) == (HalfS >> (i-1)) );
        TmpS = (MaxS>>i); TmpS += OneS;
        BOOST_CHECK( TmpS == (HalfS >> (i-1)) );
        TmpS = (MaxS>>i); TmpS += 1;
        BOOST_CHECK( TmpS == (HalfS >> (i-1)) );
        TmpS = (MaxS>>i); 
        BOOST_CHECK( TmpS++ == (MaxS>>i) );
        BOOST_CHECK( TmpS == (HalfS >> (i-1)));
    }
    BOOST_CHECK(uint160(0xbedc77e27940a7ULL) + 0xee8d836fce66fbULL == uint160(0xbedc77e27940a7ULL + 0xee8d836fce66fbULL));
    TmpS = uint160(0xbedc77e27940a7ULL); TmpS += 0xee8d836fce66fbULL;
    BOOST_CHECK(TmpS == uint160(0xbedc77e27940a7ULL+0xee8d836fce66fbULL));
    TmpS -= 0xee8d836fce66fbULL;  BOOST_CHECK(TmpS == 0xbedc77e27940a7ULL);
    TmpS = R1S;
    BOOST_CHECK(++TmpS == R1S+1);

    BOOST_CHECK(R1S -(-R2S) == R1S+R2S);
    BOOST_CHECK(R1S -(-OneS) == R1S+OneS);
    BOOST_CHECK(R1S - OneS == R1S+(-OneS));
    for (unsigned int i = 1; i < 160; ++i) {
        BOOST_CHECK((MaxS>>i) - (-OneS)  == (HalfS >> (i-1)));
        BOOST_CHECK((HalfS >> (i-1)) - OneS == (MaxS>>i));
        TmpS = (HalfS >> (i-1));
        BOOST_CHECK(TmpS-- == (HalfS >> (i-1)));
        BOOST_CHECK(TmpS == (MaxS >> i));
        TmpS = (HalfS >> (i-1));
        BOOST_CHECK(--TmpS == (MaxS >> i));
    }
    TmpS = R1S;
    BOOST_CHECK(--TmpS == R1S-1);

}

bool almostEqual(double d1, double d2) 
{
    return fabs(d1-d2) <= 4*fabs(d1)*std::numeric_limits<double>::epsilon();
}

BOOST_AUTO_TEST_CASE( methods ) // GetHex SetHex begin() end() size() GetLow64 GetSerializeSize, Serialize, Unserialize
{
    BOOST_CHECK(R1L.GetHex() == R1L.ToString());
    BOOST_CHECK(R2L.GetHex() == R2L.ToString());
    BOOST_CHECK(OneL.GetHex() == OneL.ToString());
    BOOST_CHECK(MaxL.GetHex() == MaxL.ToString());
    uint256 TmpL(R1L);
    BOOST_CHECK(TmpL == R1L);
    TmpL.SetHex(R2L.ToString());   BOOST_CHECK(TmpL == R2L);
    TmpL.SetHex(ZeroL.ToString()); BOOST_CHECK(TmpL == 0);
    TmpL.SetHex(HalfL.ToString()); BOOST_CHECK(TmpL == HalfL);

    TmpL.SetHex(R1L.ToString());
    BOOST_CHECK(memcmp(R1L.begin(), R1Array, 32)==0);
    BOOST_CHECK(memcmp(TmpL.begin(), R1Array, 32)==0);
    BOOST_CHECK(memcmp(R2L.begin(), R2Array, 32)==0);
    BOOST_CHECK(memcmp(ZeroL.begin(), ZeroArray, 32)==0);
    BOOST_CHECK(memcmp(OneL.begin(), OneArray, 32)==0);
    BOOST_CHECK(R1L.size() == 32);
    BOOST_CHECK(R2L.size() == 32);
    BOOST_CHECK(ZeroL.size() == 32);
    BOOST_CHECK(MaxL.size() == 32);
    BOOST_CHECK(R1L.begin() + 32 == R1L.end());
    BOOST_CHECK(R2L.begin() + 32 == R2L.end());
    BOOST_CHECK(OneL.begin() + 32 == OneL.end());
    BOOST_CHECK(MaxL.begin() + 32 == MaxL.end());
    BOOST_CHECK(TmpL.begin() + 32 == TmpL.end());
    BOOST_CHECK(R1L.GetLow64()  == R1LLow64);
    BOOST_CHECK(HalfL.GetLow64() ==0x0000000000000000ULL);
    BOOST_CHECK(OneL.GetLow64() ==0x0000000000000001ULL);
    BOOST_CHECK(R1L.GetSerializeSize(0,PROTOCOL_VERSION) == 32);
    BOOST_CHECK(ZeroL.GetSerializeSize(0,PROTOCOL_VERSION) == 32);

    std::stringstream ss;
    R1L.Serialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(ss.str() == std::string(R1Array,R1Array+32));
    TmpL.Unserialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(R1L == TmpL);
    ss.str("");
    ZeroL.Serialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(ss.str() == std::string(ZeroArray,ZeroArray+32));
    TmpL.Unserialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(ZeroL == TmpL);
    ss.str("");
    MaxL.Serialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(ss.str() == std::string(MaxArray,MaxArray+32));
    TmpL.Unserialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(MaxL == TmpL);
    ss.str("");

    BOOST_CHECK(R1S.GetHex() == R1S.ToString());
    BOOST_CHECK(R2S.GetHex() == R2S.ToString());
    BOOST_CHECK(OneS.GetHex() == OneS.ToString());
    BOOST_CHECK(MaxS.GetHex() == MaxS.ToString());
    uint160 TmpS(R1S);
    BOOST_CHECK(TmpS == R1S);
    TmpS.SetHex(R2S.ToString());   BOOST_CHECK(TmpS == R2S);
    TmpS.SetHex(ZeroS.ToString()); BOOST_CHECK(TmpS == 0);
    TmpS.SetHex(HalfS.ToString()); BOOST_CHECK(TmpS == HalfS);

    TmpS.SetHex(R1S.ToString());
    BOOST_CHECK(memcmp(R1S.begin(), R1Array, 20)==0);
    BOOST_CHECK(memcmp(TmpS.begin(), R1Array, 20)==0);
    BOOST_CHECK(memcmp(R2S.begin(), R2Array, 20)==0);
    BOOST_CHECK(memcmp(ZeroS.begin(), ZeroArray, 20)==0);
    BOOST_CHECK(memcmp(OneS.begin(), OneArray, 20)==0);
    BOOST_CHECK(R1S.size() == 20);
    BOOST_CHECK(R2S.size() == 20);
    BOOST_CHECK(ZeroS.size() == 20);
    BOOST_CHECK(MaxS.size() == 20);
    BOOST_CHECK(R1S.begin() + 20 == R1S.end());
    BOOST_CHECK(R2S.begin() + 20 == R2S.end());
    BOOST_CHECK(OneS.begin() + 20 == OneS.end());
    BOOST_CHECK(MaxS.begin() + 20 == MaxS.end());
    BOOST_CHECK(TmpS.begin() + 20 == TmpS.end());
    BOOST_CHECK(R1S.GetLow64()  == R1LLow64);
    BOOST_CHECK(HalfS.GetLow64() ==0x0000000000000000ULL); 
    BOOST_CHECK(OneS.GetLow64() ==0x0000000000000001ULL);
    BOOST_CHECK(R1S.GetSerializeSize(0,PROTOCOL_VERSION) == 20);
    BOOST_CHECK(ZeroS.GetSerializeSize(0,PROTOCOL_VERSION) == 20);

    R1S.Serialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(ss.str() == std::string(R1Array,R1Array+20));
    TmpS.Unserialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(R1S == TmpS);
    ss.str("");
    ZeroS.Serialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(ss.str() == std::string(ZeroArray,ZeroArray+20));
    TmpS.Unserialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(ZeroS == TmpS);
    ss.str("");
    MaxS.Serialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(ss.str() == std::string(MaxArray,MaxArray+20));
    TmpS.Unserialize(ss,0,PROTOCOL_VERSION);
    BOOST_CHECK(MaxS == TmpS);
    ss.str("");
    
    for (unsigned int i = 0; i < 255; ++i) 
    {
        BOOST_CHECK((OneL << i).getdouble() == ldexp(1.0,i));
        if (i < 160) BOOST_CHECK((OneS << i).getdouble() == ldexp(1.0,i));
    }
    BOOST_CHECK(ZeroL.getdouble() == 0.0);
    BOOST_CHECK(ZeroS.getdouble() == 0.0);
    for (int i = 256; i > 53; --i) 
        BOOST_CHECK(almostEqual((R1L>>(256-i)).getdouble(), ldexp(R1Ldouble,i)));
    for (int i = 160; i > 53; --i) 
        BOOST_CHECK(almostEqual((R1S>>(160-i)).getdouble(), ldexp(R1Sdouble,i)));
    uint64_t R1L64part = (R1L>>192).GetLow64();
    uint64_t R1S64part = (R1S>>96).GetLow64();
    for (int i = 53; i > 0; --i) // doubles can store all integers in {0,...,2^54-1} exactly
    {
        BOOST_CHECK((R1L>>(256-i)).getdouble() == (double)(R1L64part >> (64-i)));
        BOOST_CHECK((R1S>>(160-i)).getdouble() == (double)(R1S64part >> (64-i)));
    }
}

BOOST_AUTO_TEST_CASE( getmaxcoverage ) // some more tests just to get 100% coverage
{
    // ~R1L give a base_uint<256>
    BOOST_CHECK((~~R1L >> 10) == (R1L >> 10)); BOOST_CHECK((~~R1S >> 10) == (R1S >> 10));
    BOOST_CHECK((~~R1L << 10) == (R1L << 10)); BOOST_CHECK((~~R1S << 10) == (R1S << 10));
    BOOST_CHECK(!(~~R1L < R1L)); BOOST_CHECK(!(~~R1S < R1S)); 
    BOOST_CHECK(~~R1L <= R1L); BOOST_CHECK(~~R1S <= R1S); 
    BOOST_CHECK(!(~~R1L > R1L)); BOOST_CHECK(!(~~R1S > R1S)); 
    BOOST_CHECK(~~R1L >= R1L); BOOST_CHECK(~~R1S >= R1S); 
    BOOST_CHECK(!(R1L < ~~R1L)); BOOST_CHECK(!(R1S < ~~R1S)); 
    BOOST_CHECK(R1L <= ~~R1L); BOOST_CHECK(R1S <= ~~R1S); 
    BOOST_CHECK(!(R1L > ~~R1L)); BOOST_CHECK(!(R1S > ~~R1S)); 
    BOOST_CHECK(R1L >= ~~R1L); BOOST_CHECK(R1S >= ~~R1S); 
    
    BOOST_CHECK(~~R1L + R2L == R1L + ~~R2L);
    BOOST_CHECK(~~R1S + R2S == R1S + ~~R2S);
    BOOST_CHECK(~~R1L - R2L == R1L - ~~R2L);
    BOOST_CHECK(~~R1S - R2S == R1S - ~~R2S);
    BOOST_CHECK(~R1L != R1L); BOOST_CHECK(R1L != ~R1L); 
    BOOST_CHECK(~R1S != R1S); BOOST_CHECK(R1S != ~R1S); 
    unsigned char TmpArray[32];
    CHECKBITWISEOPERATOR(~R1,R2,|)
    CHECKBITWISEOPERATOR(~R1,R2,^)
    CHECKBITWISEOPERATOR(~R1,R2,&)
    CHECKBITWISEOPERATOR(R1,~R2,|)
    CHECKBITWISEOPERATOR(R1,~R2,^)
    CHECKBITWISEOPERATOR(R1,~R2,&)
}

BOOST_AUTO_TEST_SUITE_END()

