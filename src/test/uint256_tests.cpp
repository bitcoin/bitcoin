#include <boost/test/unit_test.hpp>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
#include "uint256.h"
#include <string>
#include "version.h"

#include <boost/lexical_cast.hpp>

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

template<unsigned int BITS>
void _check_ecc32(
    const std::string& hex,
    const std::string& ecc32,
          int          extra)
{
    std::ostringstream ss;
    ss << "hex=0x" << hex
       << ", ecc32=" << ecc32
       << ", extra=" << extra;
    int nExtra=0;
    base_uint<BITS> a, b;
    a.SetHex(hex);
    b.SetCodedBase32(ecc32, &nExtra);
    BOOST_CHECK_MESSAGE(nExtra == extra,
        ss.str() + " - nExtra=" + boost::lexical_cast<std::string>(nExtra));
    BOOST_CHECK_MESSAGE(a == b,
        ss.str() + " - a=0x" + a.ToString() + ", b=0x" + b.ToString());
    BOOST_CHECK_MESSAGE(a.GetCodedBase32(extra) == ecc32,
        ss.str() + " - a.GetCodedBase32(extra)=" + a.GetCodedBase32(extra));
    BOOST_CHECK_MESSAGE(b.GetHex() == hex,
        ss.str() + " - b.GetHex()=" + b.GetHex());
}

BOOST_AUTO_TEST_CASE( ecc32 )
{
    _check_ecc32<32>("be05fbf1", "8t9cn5h74bkr", 0);
    _check_ecc32<64>("c7b15e22faa0f80a", "ek9noxwe16s8doyuop6", 1);
    _check_ecc32<96>("0f43f55ca630a6ff7dadcc5f", "k931sz597ggnuf37kdbh4156k", 1);
    _check_ecc32<128>("48039c14264a44358674458aa00829c1", "gbfrrkbn1fq1ddktnkrakjay4ee53gy", 0);
    _check_ecc32<160>("f0389ca121dbeb09a47dbe32fc45a238c2f7e305", "1bxd69bdte1f9o3mh9prb8i7se67wnepbuohxyp6x1n", 1);
    _check_ecc32<256>("f0628ce6f9c42e8afbf52f4d1e8f9dde2a725fcb00f72bfa20b6ccea8af28d35", "spcp6kfqiufsrd7nz7ay3pxzrkah77hs6us8thujx6z7awmsr98ueaazo91ktw", 3);
    _check_ecc32<320>("9d7612b648cc4f1c3a4c1bf46384d6effd5e2274f225198c48686470185124637f574098ec68be40", "4jymh48cubyfq95drtetohdrpbufenireagjf6j4nrzz779meea9wdpgdcnq3iw8nx3trmcrusuwgb76y", 18);
    _check_ecc32<512>("4da998e0baa31f9d761fc8dd3c948edc8c99969869815b0c3a159bf6822f4593bbc9b682a07fa65c2920ab4386ac6338dc929e6d508da158ef6f144e165dfcd0", "amexazesjakg954awggiy5c61mecmdwqdoa7co3b4sebjm1u89erns5r57oh4tzr4ff6bxpgai8egfzymjunmjudakyjtght4kd3zqed75j487dzmojtkkpagsrd", 2);
    _check_ecc32<768>("aafe6b89d4da7287a2eac4a84cc9bcfdaa366d4129166b862ca253cf96668413988e736b44fc574ee6a6c745062f07673e46b8e41102ff9cbd5f02bf1971fdeeccec7e176cd79a8d31025810441fb0e3cdc3c85de170763920ad11247128a120", "7rf1bejeqr1bdmjy8f58bak73ds11zobh5a7od7nbysynggg3ii5cn79qyorgo3u8q9iatuxanm66339ann81motbki57t6chd16b1fa6uqcu1z9tngshhqj5ai5uyjae3ws37j4rmrgpcmn1ompg4hn5y8ix5xgjj1wcj4ino73piirjpx9kjyykhwwyywy", 2315);
    _check_ecc32<1024>("b5e9ede02d4375a2b586455b65150c2346e291a0c4cd4a9dadb54ca2e66c2e03d4667b668d98f83ff1b54baf2f1909aaf29bcb186851cf505af9ca1daea27143d24a3e8bed608543cb608dd668a116bd8b6f269bf7ec1dd0ca0a1a6441b6fc3ad89f77f819fa4f2c4ab15406e2cf4e6a3a49f14a97e2efbe58d3c8487f2709d4", "hg7penj8x7rctw4az5z6ff4k6fygwu7ruw41q39tyciftjesr96o39b53qhw7o9sb4915rn3y4bmfpy8xc66p1c5f8wpbhmzwmkn4gstiocsohfcdsasx1kkfugp4jbzdeiqdzfx1s1o37esoggmuxhnase3kwne3f6zwzpxt89hjtdmgxpupgx7oyey3qpuukrufiisqwiuqrwne6rtjqgm5tdboksks4fo444r7kdfzoq54pizbir3", 442);
    _check_ecc32<16384>("a9eccb2857caca5ee114121ba7b809d0d2a4ca7fe6c024e0b2c54846ba0de15f968549bf2c187848a0b7eb63234a5b250035a3867d1e6e9e6472103c07349cba2225abeecf0ece7beaf516a003e7ac91b37212054bfea3ff578edc0c9305caa10322b90b75effcf4167f992b1902ef8c5969dfb77670b7f0dbf448cdf78e7085d6510bfd871132388e6e182443e2e2c1cb9f302c73dd582606ccb598afab504e5aaa04c5637885015b99f0d1bbd480c07dbf158f1e0d7c6d29df899f81888ccc0a755b81c46703f846c31155ddd3823dffaa7abbc41703467256d8168cd7f16d132f49c798172679dff4d6aef164948ebc48e2136f60ebe7b4f04231a819334c51a1328291130fc84f225e59efc4d11d9f7eb98bfe591fe4eda5a47720b3e6ff584060ca2d6e155e2ba82567154cdaf49db6ba050661a81a0f45f2262d908df121f45a4d93de5f6ef6ec788c126aefb4ded91f47c901e7391b183b07114da1fca606d38072e22f490488833c0d18373e25fb550ac327f2bcf7e951c4cdf9c1f8bfcad3caadc1062625496ff52251e7c2336c227cccc680fc24062ed547e4e68e7cc115595ba6d39e11df2cb0bca1468f4c9fcbd076c124b8a817751eecb9db764d9a0fd292feb8400c5b2bea8850df9732b1c84742b3df450d786fa549902817f2b0b6c3d0855575c1383bd6adc6325a6ccbd41774dc999a36dc215e4c535ed18acb382a2ad84a17a9b63e6e326a693928b26c6ead95e72d2e1cb3f80b5f1c8f93defa6c0bde96f3b48a147f223683870bf95ce29fbf963dc3bad3eaed27dfc76d57f82319fbcbdcc6216d359c10867cdd073aa71c21ffa46dafad055656a3ac8dff291918640f805f40b12d5ef6880ee31fc9d882716862ae0bcad78b935c92feea910f79e13a3f286e90e1ae28d1657d45ad34d98267f5163e1ba5e0b5d7d546cee8c065754937bda32ddd6e2212b0c689409acd258001e79a9479438459260879c2da2c0a0ad473f3b5f3d2a2abe8885c0700f9cd9f8ecec6f737fe14c6ec22807ca85d45d812adc480872fb156d4dcb45a7f06a611759a542a12ba70ccab84e8caa2cb51f61275e62e6963110c6e598adc698c92bf979c5b5cf4cc3fa733faeddf818eba0400e6d251b995ff2741eb35079191b8918b0e0b12ac41cc22eef2a9b3ab160cdc644fc0321cf31ea3cc61cf1c96b0a684aed4e08c3749479ff06ec51636b1198e34041b2ef9efb2bce7d8f42bc993ab2b8a64e86259e6ffb9f893b42a74f8a7bd1e3b4667ffb1539829201049ad90003dd3b10063387db58bdab050fa56e6c0b40740d7dffc12a7de227fa35542cc31638e1b29820b62844a7383b0baa6d5bd186de294820ab647206e09a1d3ecef3362768641ee707bfe99571aa021193406b52d2fb297d444a875d055ddeddefc57cdd5852b081371817825d17953b336b804e4a044ea4b2b99f0c48425f09ae996b4a79d250e396d7a7907583cac65ecb2beb7754602952df5f1f6a15a71de065e47b8f7b6b8bef758b57bd444023b9420fa2aa12e753833623f41f149d624a708e6531a492bbd75186a0357a557ba4aa64ee294a3ec9bdf46eac2ba50d7a99be37538eeaa4af8bb9e6264e123d495f2a58dee82cee0d3f0db247768fc91bd6b899dee5f44e284293060d81ee9841b3e0415a45770e57b10402d9f3ca1a7aa512f03607a458a9c8c5c6bfe9ba35efb1218d0d3254f72c411d524f6eb28116772ffe7cd70b83e09a2295d31ddc0a4581cfe729e7f38b229a9557cc164aaad267f2551ea850cbaa3b29017667b3f1280de037f64f1d9c386846b9d7ac6ce60a0744636a09848d535ac5f10c0be595744c70af12a0eb3331351c493d35e16954ed2c4ea17cd8050968c6c354e4566f1b9fe30b86622ae181b590ed87dde77431809a64b73a1518f46e2ca104ed5ef748be1c477587de825bf617b66ae7d3b4ecf56802811622d0125787ae680f3ff5d96e42dd8520c5ac3c8bcce837a4dff99255cdc21f6da04d107dd4ec489619025b777dffba1a48976c55d2115fc41d9e6d83dff2e1d7d588b182fc3800f4e4966503f0951204b7445d1e575f97309cf76a755bb20d70e09e405dea0b91e8a0f7ee1a3f709931315ec8f951fdf22c54933cfc116fba8ff82e06cf01a06314777c48bcef474affbd1b1c7ac260b02abf316bb31dc9ea0a7f7c8e4aabd82c2148d8a3d392d071954467c332dd6f9ed75d8d5f00c78b2a7287111ceafdc38485bd861abb94784e2781aaa168cd52db6efa29c74b91da46af3d6255e3a09c869b76c1fdbd839e05afc1561515318940fb571a31532378d65b5b5acbf382311dfd615891d1780b9eb31fadaa8667cbf5791d024fc8b0261aa8b54f07f10b3e02d0e487bc3deab1005bc22420b3add9897f4c4eec33f2be1a4365acf62f6bdd211d7c1dd5775291d00aad29d33326f8314dc738d635c6101e09e5f9e42180b206a6201cc4eba32aa306f5cb06248fa3da22286024f97b86a56ccafc5a2732164a115e4b2a07486467c291b727d87387c8c0163f9b889bfe12146aaa2a4c949c1b5a6da44607e6eb57cde43cc94702fae81c5dc6da78be52bd82f624933f3194c6dc9986c4a7e0ec8a627b37c646119bce36a4ec259035717afc02ee22c62e3e688e14dff7a1958ffc12790fd2e8f032299b098f0b2b2af99c39691ff84733c17790d1afcc94b820a7d8bb5f39d1fab045b2b8e97b90171959ee5010a48e0849797e24b6f6e242f82c5fe42ebe303c908fe2bcbd9cabf4740d61aa39fb36a6444a0bd839ff687b4c618a0317011e8561b403905bb87497c9d4a4d9e263d22b47b69766914f68a1bee781c5604c19c9135153d0add3c6799e14046b4d98e7e1c7190", "9ayyyyyyyyyyyyyyyyyyyyyyyy9ay9ayyyyyyyyyyyyyyyyyyy6aq18d3t91upyhehx48pupngedo3133h5wfd4fjxq6hji1gqcnbnsdthqhghk6akg17ujhh94xxq4nrxjgu3gwi8mhjgd5sbj3eycemh4pip4ytqya4ygggs1d9p8hdzsork5hxje3dksqx4ggssebd591s33ci9hnn6z4qgjyxt6soz6asbn6jdqp7f6ffhzoepquouoronobhsxjkhebzgmahk45y1b65khit98xussfz4nwnjxr9agopxrmuwr9e8ahhr96e3poh3i63mfhrasnc1ryd6kwuhxf563yj6b97cb16z9jzoat3zdijpa8cesqrmsyihmiqy43aj8gia7hdfj9nrnose9fur6ucodu6jjsjt1mpjoc9n4rwggc4jcez7ok9ftqus5zgfogzn6hg1gzurw3tb7h9fip39gytg4w44hn1qj1kodgw1tkwttbhg9auq83ccyaa9bao7rom8u6zrg3jxtdec7fysm1bmemrrf34k6tyrftpx3tmgiqrzjhbecein8z7roaikfeiiomtzubkt1zj8caymnpeoysgnnuwgqku3x37aybcfqg8dmu5okeg531gqxxehsq7fkoydwwzki67a8mtdwi763tc1jewd6stswuo1sx6d75nxjfha5w7ysowngbxtnzontbk66ap7wodtphn3ty99oau5aik1f4waentx6njwrtk69za3ie5mjnpmr7dd473oymt5nei45xtne3az6smf6tuempmftw5urfpdqg4o9fyakpesnfamtkaxhmmodusg5d7smq4qebr7fhjqstphyy6pike8p3q1qkf57sfzkaafikdjyedsuhqfbn8zgisdsn5jyhp3m6qnfa163yjotj71xygxbiqaqzs9uitpgp6rcigegp3y3yhsu1xcktwkcfyi7im1ct778ue9x4wnxpacp5n534syomr4scxcqt9qc36gezzj7gqtxn8qt3tyapxy58yom94fgc6rt6asa88ug1qfrmxt9fcx7okt8rs1quoaj66t6n9oxtexmueg6yz1y1dszpp4nqrn7imj5s3hrz86mihzewk7nmrb1p59heo1x4oc3rwhdhyaczttn4axwq1eesw179375dup1oxhnwoi5tmstg1kd6mu3mg69q651mrdbt8nr7ze84rnpi7tbd4yu35tqnmgx9ji7e8uih3dbiwdn15yqfmixs6jf17993ab3u4xy1onmmnnrwewr9ecyisxja7z5mugxpo56jxexic8xtgikek8btp4q9ikqndfqrtwxkgozg17gcc6o5brcrg766xzcyhse5dnznr3iagdtjehy9muhmgei8dk5rc13eebuez7mnp68d3eruwin3xp8r6rkrju8caqfmaoitce67g4rk7c57oyom6sdmikeunodctuwz8taiwboc7tu4uiiajbsd58ase9ad54baku1ybrx55cam3bcidzegem41tri916j36cpmpkcuyzaipjfg3do9h6qm9basfacobfradqunzjjweru7qdo3zu96hu8bzym6nrwqz7tr4weheh1xrzj7ygy19pfyxndxk8g96ppqe38rkei7gyy3xkgik6iy93xejhuhswyrd5hiafxjyiyo9bzbrqjs98jd5cgycbjoutrez9qusrszxctttm3w9tw8qjg56dj6buwn74g4mhwi4ozpg9at6n3dnu479o1ik7ah8mah5i8mizuymdbqsn7jdp9g9cwqkqruigjk7fxjnc1nmkzypibo7p7fprtww9gbnu1ji1jn9grf6fyu6atu8b417ejk9eojeqaneuzp6k8k8zpka669mtpzzzbdihbs6qfpk9jix9d7zt6ws3ky1gqs557czccssdash5k9oy8xf7g4qeqrsq4xprs7gpxyjcrjwi5nauaj1k4m7jnkb3yrzy5mgw534rzzmnp1ztymtncrnzbqi3im93zzp5ik79y439y7peeukjxctxfs4ocpy3rgobwibt5ufh3937zb51bo35grc9x7uj4nnmqwk9q9rbd5cnwn1utg4gf74sumicrdqpwp96pfeeaomoewtzdudg8grripdxhtpt9kc47ja19ux7qoy8suyqciz4knapin3ftcj7ixwhggyft4c6obrfpjrenykca8yiarkqa9634g8cxm5j9aqoimjr9az8bzbhs96csmn7b1ewk7m1xr1z7gah66m7cp9tf593fapoeprqdgaucfsfp5aj6tw8cpr4jg6gqbifqo1umbfoh37oh3ea639t6j6cqdfonxcuqyafimsqw9f5tn3t8q6tsy4aromb4f3dqrt1rdum44br993pdhi8mqkt4muyybf4t4y795x4gquu9u4zdt58wmtp33f791kgguzrkmfzyarmdh9z8yprzqc7e163ehzesk7nnkzudoze54pt9jnwir4qwe4cbu9mk4p3inssrzag3wz5xygriwjpotk7ib6eyezcaakxhpoedhj9za58e786p9ryyqzre7ni4fwzuecgqgsz3z8iykbespiou3byufubndxg6i8xfkji3aboy1h5g1yt8dmyrtnp5q1q37rs5e77g7rzk3qy7d8rpiqzszokkg3wizw36n54sxys3g1swk9mf4rwk7aco63xe6pawd6qzbxr83d4z61jqj8n6z3ex3q89f4haueqgbpt1e9hc8et716fsaw4upfyyzhyb71bogjj96g43e4skan45m9ykim5pwu91n8f88ed749rgnnqdk5jbqcj9ja5qcz6a3rxhfq5q85hu654suzmm7gihbu5fi9u9tf36emo6buceu9n1fm6t8mcjhhs5afs36s61q8tazam9n3tamxmms1tph6k445ucsewd14mkgjzd7pijmszsin7fpoktk8dfaiwk6kpgfheqhg481gbipjuzdwn9kcs5n4gmdk5it58dyz4qic6kicf4db5pc81nhwjy1pfp7hy4th5k73q9spbrx1ftgkm76wre7eiisdny6g57uzd9jfwoxujgzps737ox8kf7ezym8qji1cn7so3qxw3d4gwg6mymg9ngxpkq84r8j15mrkhn9rqh51rxijqya1x3yppdkbgg3t6nr5buamuinezip7r1kjogzci49agshiw6kz9hcd6qpate6u77h6e9fmwauhgn1i9c1uhpaabw6e8nyrjrz6zjoa3rhwy4cdkp9fbjweoqqaadch6qyk7hfbqjehx7uziw77ibfdda7u5ghz66obie61pgii7bb6gg3ymjg6jno6giecrgyetkdymqisuz4pwuyich14ok46niznugh1851uyebcx93iurb54jjxphoxiu916hi8wmzf9j68qtauzi1ztnj9ry6rhtzkp6uoe3knwkcgcc4ockn6n4qx45yph11yk1j6r1fht4kgjhpq454p66jgn6ccywjfrq1jxnps9dihcn5cfch1gycmhjq4ijqt54im9u5ysu5iktdo4g9ybsxtrbrhftsmp4oiurctny39nq9ffszade6ths354wk569qyodkmzwxoufpodbmacxnocidutjk14j3ekzmhaszgycj1a5i31acz3n1yr93xy6fa1drocg7dtageeax9emj47ifk8mekhrq69gwt7g56n5zy7iz57rjug3wiud8xyec1zgm9n54x355ibqh1njom6ry7b3en3gdght3m99e96jcntrhbbfufiu1gsqqy7yn546w66qb5867k3f5setrrk7japy88oe8r3r6pax85bidgwwdmybynks4krpt6zp7yjbhbomf9jgn3axdtxcz9bbs7rc1gfsmonjo8gx9fkjw71jzzsobghkqga1nuoi71skkhwcz5fjydbdw", 2064552391);
}

BOOST_AUTO_TEST_SUITE_END()

