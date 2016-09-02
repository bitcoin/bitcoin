// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "arith_uint256.h"
#include "uint256.h"
#include "version.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
#include <string>
#include <stdio.h>

BOOST_FIXTURE_TEST_SUITE(uint256_tests, BasicTestingSetup)

const unsigned char R1Array[] =
    "\x9c\x52\x4a\xdb\xcf\x56\x11\x12\x2b\x29\x12\x5e\x5d\x35\xd2\xd2"
    "\x22\x81\xaa\xb5\x33\xf0\x08\x32\xd5\x56\xb1\xf9\xea\xe5\x1d\x7d";
const char R1ArrayHex[] = "7D1DE5EAF9B156D53208F033B5AA8122D2d2355d5e12292b121156cfdb4a529c";
const uint256 R1L = uint256(std::vector<unsigned char>(R1Array,R1Array+32));
const uint160 R1S = uint160(std::vector<unsigned char>(R1Array,R1Array+20));

const unsigned char R2Array[] =
    "\x70\x32\x1d\x7c\x47\xa5\x6b\x40\x26\x7e\x0a\xc3\xa6\x9c\xb6\xbf"
    "\x13\x30\x47\xa3\x19\x2d\xda\x71\x49\x13\x72\xf0\xb4\xca\x81\xd7";
const uint256 R2L = uint256(std::vector<unsigned char>(R2Array,R2Array+32));
const uint160 R2S = uint160(std::vector<unsigned char>(R2Array,R2Array+20));

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

inline uint160 uint160S(const char *str)
{
    uint160 rv;
    rv.SetHex(str);
    return rv;
}
inline uint160 uint160S(const std::string& str)
{
    uint160 rv;
    rv.SetHex(str);
    return rv;
}

BOOST_AUTO_TEST_CASE( basics ) // constructors, equality, inequality
{
    FAST_CHECK(1 == 0+1);
    // constructor uint256(vector<char>):
    FAST_CHECK(R1L.ToString() == ArrayToString(R1Array,32));
    FAST_CHECK(R1S.ToString() == ArrayToString(R1Array,20));
    FAST_CHECK(R2L.ToString() == ArrayToString(R2Array,32));
    FAST_CHECK(R2S.ToString() == ArrayToString(R2Array,20));
    FAST_CHECK(ZeroL.ToString() == ArrayToString(ZeroArray,32));
    FAST_CHECK(ZeroS.ToString() == ArrayToString(ZeroArray,20));
    FAST_CHECK(OneL.ToString() == ArrayToString(OneArray,32));
    FAST_CHECK(OneS.ToString() == ArrayToString(OneArray,20));
    FAST_CHECK(MaxL.ToString() == ArrayToString(MaxArray,32));
    FAST_CHECK(MaxS.ToString() == ArrayToString(MaxArray,20));
    FAST_CHECK(OneL.ToString() != ArrayToString(ZeroArray,32));
    FAST_CHECK(OneS.ToString() != ArrayToString(ZeroArray,20));

    // == and !=
    FAST_CHECK(R1L != R2L && R1S != R2S);
    FAST_CHECK(ZeroL != OneL && ZeroS != OneS);
    FAST_CHECK(OneL != ZeroL && OneS != ZeroS);
    FAST_CHECK(MaxL != ZeroL && MaxS != ZeroS);

    // String Constructor and Copy Constructor
    FAST_CHECK(uint256S("0x"+R1L.ToString()) == R1L);
    FAST_CHECK(uint256S("0x"+R2L.ToString()) == R2L);
    FAST_CHECK(uint256S("0x"+ZeroL.ToString()) == ZeroL);
    FAST_CHECK(uint256S("0x"+OneL.ToString()) == OneL);
    FAST_CHECK(uint256S("0x"+MaxL.ToString()) == MaxL);
    FAST_CHECK(uint256S(R1L.ToString()) == R1L);
    FAST_CHECK(uint256S("   0x"+R1L.ToString()+"   ") == R1L);
    FAST_CHECK(uint256S("") == ZeroL);
    FAST_CHECK(R1L == uint256S(R1ArrayHex));
    FAST_CHECK(uint256(R1L) == R1L);
    FAST_CHECK(uint256(ZeroL) == ZeroL);
    FAST_CHECK(uint256(OneL) == OneL);

    FAST_CHECK(uint160S("0x"+R1S.ToString()) == R1S);
    FAST_CHECK(uint160S("0x"+R2S.ToString()) == R2S);
    FAST_CHECK(uint160S("0x"+ZeroS.ToString()) == ZeroS);
    FAST_CHECK(uint160S("0x"+OneS.ToString()) == OneS);
    FAST_CHECK(uint160S("0x"+MaxS.ToString()) == MaxS);
    FAST_CHECK(uint160S(R1S.ToString()) == R1S);
    FAST_CHECK(uint160S("   0x"+R1S.ToString()+"   ") == R1S);
    FAST_CHECK(uint160S("") == ZeroS);
    FAST_CHECK(R1S == uint160S(R1ArrayHex));

    FAST_CHECK(uint160(R1S) == R1S);
    FAST_CHECK(uint160(ZeroS) == ZeroS);
    FAST_CHECK(uint160(OneS) == OneS);
}

BOOST_AUTO_TEST_CASE( comparison ) // <= >= < >
{
    uint256 LastL;
    for (int i = 255; i >= 0; --i) {
        uint256 TmpL;
        *(TmpL.begin() + (i>>3)) |= 1<<(7-(i&7));
        FAST_CHECK( LastL < TmpL );
        LastL = TmpL;
    }

    FAST_CHECK( ZeroL < R1L );
    FAST_CHECK( R2L < R1L );
    FAST_CHECK( ZeroL < OneL );
    FAST_CHECK( OneL < MaxL );
    FAST_CHECK( R1L < MaxL );
    FAST_CHECK( R2L < MaxL );

    uint160 LastS;
    for (int i = 159; i >= 0; --i) {
        uint160 TmpS;
        *(TmpS.begin() + (i>>3)) |= 1<<(7-(i&7));
        FAST_CHECK( LastS < TmpS );
        LastS = TmpS;
    }
    FAST_CHECK( ZeroS < R1S );
    FAST_CHECK( R2S < R1S );
    FAST_CHECK( ZeroS < OneS );
    FAST_CHECK( OneS < MaxS );
    FAST_CHECK( R1S < MaxS );
    FAST_CHECK( R2S < MaxS );
}

BOOST_AUTO_TEST_CASE( methods ) // GetHex SetHex begin() end() size() GetLow64 GetSerializeSize, Serialize, Unserialize
{
    FAST_CHECK(R1L.GetHex() == R1L.ToString());
    FAST_CHECK(R2L.GetHex() == R2L.ToString());
    FAST_CHECK(OneL.GetHex() == OneL.ToString());
    FAST_CHECK(MaxL.GetHex() == MaxL.ToString());
    uint256 TmpL(R1L);
    FAST_CHECK(TmpL == R1L);
    TmpL.SetHex(R2L.ToString());   FAST_CHECK(TmpL == R2L);
    TmpL.SetHex(ZeroL.ToString()); FAST_CHECK(TmpL == uint256());

    TmpL.SetHex(R1L.ToString());
    FAST_CHECK(memcmp(R1L.begin(), R1Array, 32)==0);
    FAST_CHECK(memcmp(TmpL.begin(), R1Array, 32)==0);
    FAST_CHECK(memcmp(R2L.begin(), R2Array, 32)==0);
    FAST_CHECK(memcmp(ZeroL.begin(), ZeroArray, 32)==0);
    FAST_CHECK(memcmp(OneL.begin(), OneArray, 32)==0);
    FAST_CHECK(R1L.size() == sizeof(R1L));
    FAST_CHECK(sizeof(R1L) == 32);
    FAST_CHECK(R1L.size() == 32);
    FAST_CHECK(R2L.size() == 32);
    FAST_CHECK(ZeroL.size() == 32);
    FAST_CHECK(MaxL.size() == 32);
    FAST_CHECK(R1L.begin() + 32 == R1L.end());
    FAST_CHECK(R2L.begin() + 32 == R2L.end());
    FAST_CHECK(OneL.begin() + 32 == OneL.end());
    FAST_CHECK(MaxL.begin() + 32 == MaxL.end());
    FAST_CHECK(TmpL.begin() + 32 == TmpL.end());
    FAST_CHECK(R1L.GetSerializeSize(0,PROTOCOL_VERSION) == 32);
    FAST_CHECK(ZeroL.GetSerializeSize(0,PROTOCOL_VERSION) == 32);

    std::stringstream ss;
    R1L.Serialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(ss.str() == std::string(R1Array,R1Array+32));
    TmpL.Unserialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(R1L == TmpL);
    ss.str("");
    ZeroL.Serialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(ss.str() == std::string(ZeroArray,ZeroArray+32));
    TmpL.Unserialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(ZeroL == TmpL);
    ss.str("");
    MaxL.Serialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(ss.str() == std::string(MaxArray,MaxArray+32));
    TmpL.Unserialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(MaxL == TmpL);
    ss.str("");

    FAST_CHECK(R1S.GetHex() == R1S.ToString());
    FAST_CHECK(R2S.GetHex() == R2S.ToString());
    FAST_CHECK(OneS.GetHex() == OneS.ToString());
    FAST_CHECK(MaxS.GetHex() == MaxS.ToString());
    uint160 TmpS(R1S);
    FAST_CHECK(TmpS == R1S);
    TmpS.SetHex(R2S.ToString());   FAST_CHECK(TmpS == R2S);
    TmpS.SetHex(ZeroS.ToString()); FAST_CHECK(TmpS == uint160());

    TmpS.SetHex(R1S.ToString());
    FAST_CHECK(memcmp(R1S.begin(), R1Array, 20)==0);
    FAST_CHECK(memcmp(TmpS.begin(), R1Array, 20)==0);
    FAST_CHECK(memcmp(R2S.begin(), R2Array, 20)==0);
    FAST_CHECK(memcmp(ZeroS.begin(), ZeroArray, 20)==0);
    FAST_CHECK(memcmp(OneS.begin(), OneArray, 20)==0);
    FAST_CHECK(R1S.size() == sizeof(R1S));
    FAST_CHECK(sizeof(R1S) == 20);
    FAST_CHECK(R1S.size() == 20);
    FAST_CHECK(R2S.size() == 20);
    FAST_CHECK(ZeroS.size() == 20);
    FAST_CHECK(MaxS.size() == 20);
    FAST_CHECK(R1S.begin() + 20 == R1S.end());
    FAST_CHECK(R2S.begin() + 20 == R2S.end());
    FAST_CHECK(OneS.begin() + 20 == OneS.end());
    FAST_CHECK(MaxS.begin() + 20 == MaxS.end());
    FAST_CHECK(TmpS.begin() + 20 == TmpS.end());
    FAST_CHECK(R1S.GetSerializeSize(0,PROTOCOL_VERSION) == 20);
    FAST_CHECK(ZeroS.GetSerializeSize(0,PROTOCOL_VERSION) == 20);

    R1S.Serialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(ss.str() == std::string(R1Array,R1Array+20));
    TmpS.Unserialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(R1S == TmpS);
    ss.str("");
    ZeroS.Serialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(ss.str() == std::string(ZeroArray,ZeroArray+20));
    TmpS.Unserialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(ZeroS == TmpS);
    ss.str("");
    MaxS.Serialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(ss.str() == std::string(MaxArray,MaxArray+20));
    TmpS.Unserialize(ss,0,PROTOCOL_VERSION);
    FAST_CHECK(MaxS == TmpS);
    ss.str("");
}

BOOST_AUTO_TEST_CASE( conversion )
{
    FAST_CHECK(ArithToUint256(UintToArith256(ZeroL)) == ZeroL);
    FAST_CHECK(ArithToUint256(UintToArith256(OneL)) == OneL);
    FAST_CHECK(ArithToUint256(UintToArith256(R1L)) == R1L);
    FAST_CHECK(ArithToUint256(UintToArith256(R2L)) == R2L);
    FAST_CHECK(UintToArith256(ZeroL) == 0);
    FAST_CHECK(UintToArith256(OneL) == 1);
    FAST_CHECK(ArithToUint256(0) == ZeroL);
    FAST_CHECK(ArithToUint256(1) == OneL);
    FAST_CHECK(arith_uint256(R1L.GetHex()) == UintToArith256(R1L));
    FAST_CHECK(arith_uint256(R2L.GetHex()) == UintToArith256(R2L));
    FAST_CHECK(R1L.GetHex() == UintToArith256(R1L).GetHex());
    FAST_CHECK(R2L.GetHex() == UintToArith256(R2L).GetHex());
}

BOOST_AUTO_TEST_SUITE_END()
