// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
#include "uint256.h"
#include "arith_uint256.h"
#include <string>
#include "version.h"

BOOST_AUTO_TEST_SUITE(arith_uint256_tests)

/// Convert vector to arith_uint256, via uint256 blob
inline arith_uint256 arith_uint256V(const std::vector<unsigned char>& vch)
{
    return UintToArith256(uint256(vch));
}

const unsigned char R1Array[] =
    "\x9c\x52\x4a\xdb\xcf\x56\x11\x12\x2b\x29\x12\x5e\x5d\x35\xd2\xd2"
    "\x22\x81\xaa\xb5\x33\xf0\x08\x32\xd5\x56\xb1\xf9\xea\xe5\x1d\x7d";
const char R1ArrayHex[] = "7D1DE5EAF9B156D53208F033B5AA8122D2d2355d5e12292b121156cfdb4a529c";
const double R1Ldouble = 0.4887374590559308955; // R1L equals roughly R1Ldouble * 2^256
const arith_uint256 R1L = arith_uint256V(std::vector<unsigned char>(R1Array,R1Array+32));
const uint64_t R1LLow64 = 0x121156cfdb4a529cULL;

const unsigned char R2Array[] =
    "\x70\x32\x1d\x7c\x47\xa5\x6b\x40\x26\x7e\x0a\xc3\xa6\x9c\xb6\xbf"
    "\x13\x30\x47\xa3\x19\x2d\xda\x71\x49\x13\x72\xf0\xb4\xca\x81\xd7";
const arith_uint256 R2L = arith_uint256V(std::vector<unsigned char>(R2Array,R2Array+32));

const char R1LplusR2L[] = "549FB09FEA236A1EA3E31D4D58F1B1369288D204211CA751527CFC175767850C";

const unsigned char ZeroArray[] =
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
const arith_uint256 ZeroL = arith_uint256V(std::vector<unsigned char>(ZeroArray,ZeroArray+32));

const unsigned char OneArray[] =
    "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
const arith_uint256 OneL = arith_uint256V(std::vector<unsigned char>(OneArray,OneArray+32));

const unsigned char MaxArray[] =
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
const arith_uint256 MaxL = arith_uint256V(std::vector<unsigned char>(MaxArray,MaxArray+32));

const arith_uint256 HalfL = (OneL << 255);
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
    // constructor arith_uint256(vector<char>):
    BOOST_CHECK(R1L.ToString() == ArrayToString(R1Array,32));
    BOOST_CHECK(R2L.ToString() == ArrayToString(R2Array,32));
    BOOST_CHECK(ZeroL.ToString() == ArrayToString(ZeroArray,32));
    BOOST_CHECK(OneL.ToString() == ArrayToString(OneArray,32));
    BOOST_CHECK(MaxL.ToString() == ArrayToString(MaxArray,32));
    BOOST_CHECK(OneL.ToString() != ArrayToString(ZeroArray,32));

    // == and !=
    BOOST_CHECK(R1L != R2L);
    BOOST_CHECK(ZeroL != OneL);
    BOOST_CHECK(OneL != ZeroL);
    BOOST_CHECK(MaxL != ZeroL);
    BOOST_CHECK(~MaxL == ZeroL);
    BOOST_CHECK( ((R1L ^ R2L) ^ R1L) == R2L);

    uint64_t Tmp64 = 0xc4dab720d9c7acaaULL;
    for (unsigned int i = 0; i < 256; ++i)
    {
        BOOST_CHECK(ZeroL != (OneL << i));
        BOOST_CHECK((OneL << i) != ZeroL);
        BOOST_CHECK(R1L != (R1L ^ (OneL << i)));
        BOOST_CHECK(((arith_uint256(Tmp64) ^ (OneL << i) ) != Tmp64 ));
    }
    BOOST_CHECK(ZeroL == (OneL << 256));

    // String Constructor and Copy Constructor
    BOOST_CHECK(arith_uint256("0x"+R1L.ToString()) == R1L);
    BOOST_CHECK(arith_uint256("0x"+R2L.ToString()) == R2L);
    BOOST_CHECK(arith_uint256("0x"+ZeroL.ToString()) == ZeroL);
    BOOST_CHECK(arith_uint256("0x"+OneL.ToString()) == OneL);
    BOOST_CHECK(arith_uint256("0x"+MaxL.ToString()) == MaxL);
    BOOST_CHECK(arith_uint256(R1L.ToString()) == R1L);
    BOOST_CHECK(arith_uint256("   0x"+R1L.ToString()+"   ") == R1L);
    BOOST_CHECK(arith_uint256("") == ZeroL);
    BOOST_CHECK(R1L == arith_uint256(R1ArrayHex));
    BOOST_CHECK(arith_uint256(R1L) == R1L);
    BOOST_CHECK((arith_uint256(R1L^R2L)^R2L) == R1L);
    BOOST_CHECK(arith_uint256(ZeroL) == ZeroL);
    BOOST_CHECK(arith_uint256(OneL) == OneL);

    // uint64_t constructor
    BOOST_CHECK( (R1L & arith_uint256("0xffffffffffffffff")) == arith_uint256(R1LLow64));
    BOOST_CHECK(ZeroL == arith_uint256(0));
    BOOST_CHECK(OneL == arith_uint256(1));
    BOOST_CHECK(arith_uint256("0xffffffffffffffff") = arith_uint256(0xffffffffffffffffULL));

    // Assignment (from base_uint)
    arith_uint256 tmpL = ~ZeroL; BOOST_CHECK(tmpL == ~ZeroL);
    tmpL = ~OneL; BOOST_CHECK(tmpL == ~OneL);
    tmpL = ~R1L; BOOST_CHECK(tmpL == ~R1L);
    tmpL = ~R2L; BOOST_CHECK(tmpL == ~R2L);
    tmpL = ~MaxL; BOOST_CHECK(tmpL == ~MaxL);
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
    arith_uint256 TmpL;
    for (unsigned int i = 0; i < 256; ++i)
    {
        shiftArrayLeft(TmpArray, OneArray, 32, i);
        BOOST_CHECK(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (OneL << i));
        TmpL = OneL; TmpL <<= i;
        BOOST_CHECK(TmpL == (OneL << i));
        BOOST_CHECK((HalfL >> (255-i)) == (OneL << i));
        TmpL = HalfL; TmpL >>= (255-i);
        BOOST_CHECK(TmpL == (OneL << i));

        shiftArrayLeft(TmpArray, R1Array, 32, i);
        BOOST_CHECK(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (R1L << i));
        TmpL = R1L; TmpL <<= i;
        BOOST_CHECK(TmpL == (R1L << i));

        shiftArrayRight(TmpArray, R1Array, 32, i);
        BOOST_CHECK(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (R1L >> i));
        TmpL = R1L; TmpL >>= i;
        BOOST_CHECK(TmpL == (R1L >> i));

        shiftArrayLeft(TmpArray, MaxArray, 32, i);
        BOOST_CHECK(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (MaxL << i));
        TmpL = MaxL; TmpL <<= i;
        BOOST_CHECK(TmpL == (MaxL << i));

        shiftArrayRight(TmpArray, MaxArray, 32, i);
        BOOST_CHECK(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (MaxL >> i));
        TmpL = MaxL; TmpL >>= i;
        BOOST_CHECK(TmpL == (MaxL >> i));
    }
    arith_uint256 c1L = arith_uint256(0x0123456789abcdefULL);
    arith_uint256 c2L = c1L << 128;
    for (unsigned int i = 0; i < 128; ++i) {
        BOOST_CHECK((c1L << i) == (c2L >> (128-i)));
    }
    for (unsigned int i = 128; i < 256; ++i) {
        BOOST_CHECK((c1L << i) == (c2L << (i-128)));
    }
}

BOOST_AUTO_TEST_CASE( unaryOperators ) // !    ~    -
{
    BOOST_CHECK(!ZeroL);
    BOOST_CHECK(!(!OneL));
    for (unsigned int i = 0; i < 256; ++i)
        BOOST_CHECK(!(!(OneL<<i)));
    BOOST_CHECK(!(!R1L));
    BOOST_CHECK(!(!MaxL));

    BOOST_CHECK(~ZeroL == MaxL);

    unsigned char TmpArray[32];
    for (unsigned int i = 0; i < 32; ++i) { TmpArray[i] = ~R1Array[i]; }
    BOOST_CHECK(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (~R1L));

    BOOST_CHECK(-ZeroL == ZeroL);
    BOOST_CHECK(-R1L == (~R1L)+1);
    for (unsigned int i = 0; i < 256; ++i)
        BOOST_CHECK(-(OneL<<i) == (MaxL << i));
}


// Check if doing _A_ _OP_ _B_ results in the same as applying _OP_ onto each
// element of Aarray and Barray, and then converting the result into a arith_uint256.
#define CHECKBITWISEOPERATOR(_A_,_B_,_OP_)                              \
    for (unsigned int i = 0; i < 32; ++i) { TmpArray[i] = _A_##Array[i] _OP_ _B_##Array[i]; } \
    BOOST_CHECK(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (_A_##L _OP_ _B_##L));

#define CHECKASSIGNMENTOPERATOR(_A_,_B_,_OP_)                           \
    TmpL = _A_##L; TmpL _OP_##= _B_##L; BOOST_CHECK(TmpL == (_A_##L _OP_ _B_##L));

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

    arith_uint256 TmpL;
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
    TmpL = R1L; TmpL |= Tmp64;  BOOST_CHECK(TmpL == (R1L | arith_uint256(Tmp64)));
    TmpL = R1L; TmpL |= 0; BOOST_CHECK(TmpL == R1L);
    TmpL ^= 0; BOOST_CHECK(TmpL == R1L);
    TmpL ^= Tmp64;  BOOST_CHECK(TmpL == (R1L ^ arith_uint256(Tmp64)));
}

BOOST_AUTO_TEST_CASE( comparison ) // <= >= < >
{
    arith_uint256 TmpL;
    for (unsigned int i = 0; i < 256; ++i) {
        TmpL= OneL<< i;
        BOOST_CHECK( TmpL >= ZeroL && TmpL > ZeroL && ZeroL < TmpL && ZeroL <= TmpL);
        BOOST_CHECK( TmpL >= 0 && TmpL > 0 && 0 < TmpL && 0 <= TmpL);
        TmpL |= R1L;
        BOOST_CHECK( TmpL >= R1L ); BOOST_CHECK( (TmpL == R1L) != (TmpL > R1L)); BOOST_CHECK( (TmpL == R1L) || !( TmpL <= R1L));
        BOOST_CHECK( R1L <= TmpL ); BOOST_CHECK( (R1L == TmpL) != (R1L < TmpL)); BOOST_CHECK( (TmpL == R1L) || !( R1L >= TmpL));
        BOOST_CHECK(! (TmpL < R1L)); BOOST_CHECK(! (R1L > TmpL));
    }
}

BOOST_AUTO_TEST_CASE( plusMinus )
{
    arith_uint256 TmpL = 0;
    BOOST_CHECK(R1L+R2L == arith_uint256(R1LplusR2L));
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
    BOOST_CHECK(arith_uint256(0xbedc77e27940a7ULL) + 0xee8d836fce66fbULL == arith_uint256(0xbedc77e27940a7ULL + 0xee8d836fce66fbULL));
    TmpL = arith_uint256(0xbedc77e27940a7ULL); TmpL += 0xee8d836fce66fbULL;
    BOOST_CHECK(TmpL == arith_uint256(0xbedc77e27940a7ULL+0xee8d836fce66fbULL));
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
}

BOOST_AUTO_TEST_CASE( multiply )
{
    BOOST_CHECK((R1L * R1L).ToString() == "62a38c0486f01e45879d7910a7761bf30d5237e9873f9bff3642a732c4d84f10");
    BOOST_CHECK((R1L * R2L).ToString() == "de37805e9986996cfba76ff6ba51c008df851987d9dd323f0e5de07760529c40");
    BOOST_CHECK((R1L * ZeroL) == ZeroL);
    BOOST_CHECK((R1L * OneL) == R1L);
    BOOST_CHECK((R1L * MaxL) == -R1L);
    BOOST_CHECK((R2L * R1L) == (R1L * R2L));
    BOOST_CHECK((R2L * R2L).ToString() == "ac8c010096767d3cae5005dec28bb2b45a1d85ab7996ccd3e102a650f74ff100");
    BOOST_CHECK((R2L * ZeroL) == ZeroL);
    BOOST_CHECK((R2L * OneL) == R2L);
    BOOST_CHECK((R2L * MaxL) == -R2L);

    BOOST_CHECK(MaxL * MaxL == OneL);

    BOOST_CHECK((R1L * 0) == 0);
    BOOST_CHECK((R1L * 1) == R1L);
    BOOST_CHECK((R1L * 3).ToString() == "7759b1c0ed14047f961ad09b20ff83687876a0181a367b813634046f91def7d4");
    BOOST_CHECK((R2L * 0x87654321UL).ToString() == "23f7816e30c4ae2017257b7a0fa64d60402f5234d46e746b61c960d09a26d070");
}

BOOST_AUTO_TEST_CASE( divide )
{
    arith_uint256 D1L("AD7133AC1977FA2B7");
    arith_uint256 D2L("ECD751716");
    BOOST_CHECK((R1L / D1L).ToString() == "00000000000000000b8ac01106981635d9ed112290f8895545a7654dde28fb3a");
    BOOST_CHECK((R1L / D2L).ToString() == "000000000873ce8efec5b67150bad3aa8c5fcb70e947586153bf2cec7c37c57a");
    BOOST_CHECK(R1L / OneL == R1L);
    BOOST_CHECK(R1L / MaxL == ZeroL);
    BOOST_CHECK(MaxL / R1L == 2);
    BOOST_CHECK_THROW(R1L / ZeroL, uint_error);
    BOOST_CHECK((R2L / D1L).ToString() == "000000000000000013e1665895a1cc981de6d93670105a6b3ec3b73141b3a3c5");
    BOOST_CHECK((R2L / D2L).ToString() == "000000000e8f0abe753bb0afe2e9437ee85d280be60882cf0bd1aaf7fa3cc2c4");
    BOOST_CHECK(R2L / OneL == R2L);
    BOOST_CHECK(R2L / MaxL == ZeroL);
    BOOST_CHECK(MaxL / R2L == 1);
    BOOST_CHECK_THROW(R2L / ZeroL, uint_error);
}


bool almostEqual(double d1, double d2)
{
    return fabs(d1-d2) <= 4*fabs(d1)*std::numeric_limits<double>::epsilon();
}

BOOST_AUTO_TEST_CASE( methods ) // GetHex SetHex size() GetLow64 GetSerializeSize, Serialize, Unserialize
{
    BOOST_CHECK(R1L.GetHex() == R1L.ToString());
    BOOST_CHECK(R2L.GetHex() == R2L.ToString());
    BOOST_CHECK(OneL.GetHex() == OneL.ToString());
    BOOST_CHECK(MaxL.GetHex() == MaxL.ToString());
    arith_uint256 TmpL(R1L);
    BOOST_CHECK(TmpL == R1L);
    TmpL.SetHex(R2L.ToString());   BOOST_CHECK(TmpL == R2L);
    TmpL.SetHex(ZeroL.ToString()); BOOST_CHECK(TmpL == 0);
    TmpL.SetHex(HalfL.ToString()); BOOST_CHECK(TmpL == HalfL);

    TmpL.SetHex(R1L.ToString());
    BOOST_CHECK(R1L.size() == 32);
    BOOST_CHECK(R2L.size() == 32);
    BOOST_CHECK(ZeroL.size() == 32);
    BOOST_CHECK(MaxL.size() == 32);
    BOOST_CHECK(R1L.GetLow64()  == R1LLow64);
    BOOST_CHECK(HalfL.GetLow64() ==0x0000000000000000ULL);
    BOOST_CHECK(OneL.GetLow64() ==0x0000000000000001ULL);

    for (unsigned int i = 0; i < 255; ++i)
    {
        BOOST_CHECK((OneL << i).getdouble() == ldexp(1.0,i));
    }
    BOOST_CHECK(ZeroL.getdouble() == 0.0);
    for (int i = 256; i > 53; --i)
        BOOST_CHECK(almostEqual((R1L>>(256-i)).getdouble(), ldexp(R1Ldouble,i)));
    uint64_t R1L64part = (R1L>>192).GetLow64();
    for (int i = 53; i > 0; --i) // doubles can store all integers in {0,...,2^54-1} exactly
    {
        BOOST_CHECK((R1L>>(256-i)).getdouble() == (double)(R1L64part >> (64-i)));
    }
}

BOOST_AUTO_TEST_CASE(bignum_SetCompact)
{
    arith_uint256 num;
    bool fNegative;
    bool fOverflow;
    num.SetCompact(0, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x00123456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x01003456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x02000056, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x03000000, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x04000000, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x00923456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x01803456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x02800056, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x03800000, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x04800000, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x01123456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000012");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x01120000U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    // Make sure that we don't generate compacts with the 0x00800000 bit set
    num = 0x80;
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x02008000U);

    num.SetCompact(0x01fedcba, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "000000000000000000000000000000000000000000000000000000000000007e");
    BOOST_CHECK_EQUAL(num.GetCompact(true), 0x01fe0000U);
    BOOST_CHECK_EQUAL(fNegative, true);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x02123456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000001234");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x02123400U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x03123456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000123456");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x03123456U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x04123456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000012345600");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x04123456U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x04923456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000012345600");
    BOOST_CHECK_EQUAL(num.GetCompact(true), 0x04923456U);
    BOOST_CHECK_EQUAL(fNegative, true);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x05009234, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "0000000000000000000000000000000000000000000000000000000092340000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x05009234U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0x20123456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(num.GetHex(), "1234560000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x20123456U);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, false);

    num.SetCompact(0xff123456, &fNegative, &fOverflow);
    BOOST_CHECK_EQUAL(fNegative, false);
    BOOST_CHECK_EQUAL(fOverflow, true);
}


BOOST_AUTO_TEST_CASE( getmaxcoverage ) // some more tests just to get 100% coverage
{
    // ~R1L give a base_uint<256>
    BOOST_CHECK((~~R1L >> 10) == (R1L >> 10));
    BOOST_CHECK((~~R1L << 10) == (R1L << 10));
    BOOST_CHECK(!(~~R1L < R1L));
    BOOST_CHECK(~~R1L <= R1L);
    BOOST_CHECK(!(~~R1L > R1L));
    BOOST_CHECK(~~R1L >= R1L);
    BOOST_CHECK(!(R1L < ~~R1L));
    BOOST_CHECK(R1L <= ~~R1L);
    BOOST_CHECK(!(R1L > ~~R1L));
    BOOST_CHECK(R1L >= ~~R1L);

    BOOST_CHECK(~~R1L + R2L == R1L + ~~R2L);
    BOOST_CHECK(~~R1L - R2L == R1L - ~~R2L);
    BOOST_CHECK(~R1L != R1L); BOOST_CHECK(R1L != ~R1L);
    unsigned char TmpArray[32];
    CHECKBITWISEOPERATOR(~R1,R2,|)
    CHECKBITWISEOPERATOR(~R1,R2,^)
    CHECKBITWISEOPERATOR(~R1,R2,&)
    CHECKBITWISEOPERATOR(R1,~R2,|)
    CHECKBITWISEOPERATOR(R1,~R2,^)
    CHECKBITWISEOPERATOR(R1,~R2,&)
}

BOOST_AUTO_TEST_SUITE_END()
