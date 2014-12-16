// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
#include "blob256.h"
#include "uint256.h"
#include <string>
#include "version.h"
#include <stdio.h>

BOOST_AUTO_TEST_SUITE(blob256_tests)

const unsigned char R1Array[] =
    "\x9c\x52\x4a\xdb\xcf\x56\x11\x12\x2b\x29\x12\x5e\x5d\x35\xd2\xd2"
    "\x22\x81\xaa\xb5\x33\xf0\x08\x32\xd5\x56\xb1\xf9\xea\xe5\x1d\x7d";
const char R1ArrayHex[] = "7D1DE5EAF9B156D53208F033B5AA8122D2d2355d5e12292b121156cfdb4a529c";
const blob256 R1L = blob256(std::vector<unsigned char>(R1Array,R1Array+32));
const blob160 R1S = blob160(std::vector<unsigned char>(R1Array,R1Array+20));

const unsigned char R2Array[] =
    "\x70\x32\x1d\x7c\x47\xa5\x6b\x40\x26\x7e\x0a\xc3\xa6\x9c\xb6\xbf"
    "\x13\x30\x47\xa3\x19\x2d\xda\x71\x49\x13\x72\xf0\xb4\xca\x81\xd7";
const blob256 R2L = blob256(std::vector<unsigned char>(R2Array,R2Array+32));
const blob160 R2S = blob160(std::vector<unsigned char>(R2Array,R2Array+20));

const unsigned char ZeroArray[] =
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
const blob256 ZeroL = blob256(std::vector<unsigned char>(ZeroArray,ZeroArray+32));
const blob160 ZeroS = blob160(std::vector<unsigned char>(ZeroArray,ZeroArray+20));

const unsigned char OneArray[] =
    "\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
const blob256 OneL = blob256(std::vector<unsigned char>(OneArray,OneArray+32));
const blob160 OneS = blob160(std::vector<unsigned char>(OneArray,OneArray+20));

const unsigned char MaxArray[] =
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
const blob256 MaxL = blob256(std::vector<unsigned char>(MaxArray,MaxArray+32));
const blob160 MaxS = blob160(std::vector<unsigned char>(MaxArray,MaxArray+20));

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

inline blob160 blob160S(const char *str)
{
    blob160 rv;
    rv.SetHex(str);
    return rv;
}
inline blob160 blob160S(const std::string& str)
{
    blob160 rv;
    rv.SetHex(str);
    return rv;
}

BOOST_AUTO_TEST_CASE( basics ) // constructors, equality, inequality
{
    BOOST_CHECK(1 == 0+1);
    // constructor blob256(vector<char>):
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

    // String Constructor and Copy Constructor
    BOOST_CHECK(blob256S("0x"+R1L.ToString()) == R1L);
    BOOST_CHECK(blob256S("0x"+R2L.ToString()) == R2L);
    BOOST_CHECK(blob256S("0x"+ZeroL.ToString()) == ZeroL);
    BOOST_CHECK(blob256S("0x"+OneL.ToString()) == OneL);
    BOOST_CHECK(blob256S("0x"+MaxL.ToString()) == MaxL);
    BOOST_CHECK(blob256S(R1L.ToString()) == R1L);
    BOOST_CHECK(blob256S("   0x"+R1L.ToString()+"   ") == R1L);
    BOOST_CHECK(blob256S("") == ZeroL);
    BOOST_CHECK(R1L == blob256S(R1ArrayHex));
    BOOST_CHECK(blob256(R1L) == R1L);
    BOOST_CHECK(blob256(ZeroL) == ZeroL);
    BOOST_CHECK(blob256(OneL) == OneL);

    BOOST_CHECK(blob160S("0x"+R1S.ToString()) == R1S);
    BOOST_CHECK(blob160S("0x"+R2S.ToString()) == R2S);
    BOOST_CHECK(blob160S("0x"+ZeroS.ToString()) == ZeroS);
    BOOST_CHECK(blob160S("0x"+OneS.ToString()) == OneS);
    BOOST_CHECK(blob160S("0x"+MaxS.ToString()) == MaxS);
    BOOST_CHECK(blob160S(R1S.ToString()) == R1S);
    BOOST_CHECK(blob160S("   0x"+R1S.ToString()+"   ") == R1S);
    BOOST_CHECK(blob160S("") == ZeroS);
    BOOST_CHECK(R1S == blob160S(R1ArrayHex));

    BOOST_CHECK(blob160(R1S) == R1S);
    BOOST_CHECK(blob160(ZeroS) == ZeroS);
    BOOST_CHECK(blob160(OneS) == OneS);
}

BOOST_AUTO_TEST_CASE( comparison ) // <= >= < >
{
    blob256 LastL;
    for (int i = 255; i >= 0; --i) {
        blob256 TmpL;
        *(TmpL.begin() + (i>>3)) |= 1<<(7-(i&7));
        BOOST_CHECK( LastL < TmpL );
        LastL = TmpL;
    }

    BOOST_CHECK( ZeroL < R1L );
    BOOST_CHECK( R2L < R1L );
    BOOST_CHECK( ZeroL < OneL );
    BOOST_CHECK( OneL < MaxL );
    BOOST_CHECK( R1L < MaxL );
    BOOST_CHECK( R2L < MaxL );

    blob160 LastS;
    for (int i = 159; i >= 0; --i) {
        blob160 TmpS;
        *(TmpS.begin() + (i>>3)) |= 1<<(7-(i&7));
        BOOST_CHECK( LastS < TmpS );
        LastS = TmpS;
    }
    BOOST_CHECK( ZeroS < R1S );
    BOOST_CHECK( R2S < R1S );
    BOOST_CHECK( ZeroS < OneS );
    BOOST_CHECK( OneS < MaxS );
    BOOST_CHECK( R1S < MaxS );
    BOOST_CHECK( R2S < MaxS );
}

BOOST_AUTO_TEST_CASE( methods ) // GetHex SetHex begin() end() size() GetLow64 GetSerializeSize, Serialize, Unserialize
{
    BOOST_CHECK(R1L.GetHex() == R1L.ToString());
    BOOST_CHECK(R2L.GetHex() == R2L.ToString());
    BOOST_CHECK(OneL.GetHex() == OneL.ToString());
    BOOST_CHECK(MaxL.GetHex() == MaxL.ToString());
    blob256 TmpL(R1L);
    BOOST_CHECK(TmpL == R1L);
    TmpL.SetHex(R2L.ToString());   BOOST_CHECK(TmpL == R2L);
    TmpL.SetHex(ZeroL.ToString()); BOOST_CHECK(TmpL == blob256());

    TmpL.SetHex(R1L.ToString());
    BOOST_CHECK(memcmp(R1L.begin(), R1Array, 32)==0);
    BOOST_CHECK(memcmp(TmpL.begin(), R1Array, 32)==0);
    BOOST_CHECK(memcmp(R2L.begin(), R2Array, 32)==0);
    BOOST_CHECK(memcmp(ZeroL.begin(), ZeroArray, 32)==0);
    BOOST_CHECK(memcmp(OneL.begin(), OneArray, 32)==0);
    BOOST_CHECK(R1L.size() == sizeof(R1L));
    BOOST_CHECK(sizeof(R1L) == 32);
    BOOST_CHECK(R1L.size() == 32);
    BOOST_CHECK(R2L.size() == 32);
    BOOST_CHECK(ZeroL.size() == 32);
    BOOST_CHECK(MaxL.size() == 32);
    BOOST_CHECK(R1L.begin() + 32 == R1L.end());
    BOOST_CHECK(R2L.begin() + 32 == R2L.end());
    BOOST_CHECK(OneL.begin() + 32 == OneL.end());
    BOOST_CHECK(MaxL.begin() + 32 == MaxL.end());
    BOOST_CHECK(TmpL.begin() + 32 == TmpL.end());
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
    blob160 TmpS(R1S);
    BOOST_CHECK(TmpS == R1S);
    TmpS.SetHex(R2S.ToString());   BOOST_CHECK(TmpS == R2S);
    TmpS.SetHex(ZeroS.ToString()); BOOST_CHECK(TmpS == blob160());

    TmpS.SetHex(R1S.ToString());
    BOOST_CHECK(memcmp(R1S.begin(), R1Array, 20)==0);
    BOOST_CHECK(memcmp(TmpS.begin(), R1Array, 20)==0);
    BOOST_CHECK(memcmp(R2S.begin(), R2Array, 20)==0);
    BOOST_CHECK(memcmp(ZeroS.begin(), ZeroArray, 20)==0);
    BOOST_CHECK(memcmp(OneS.begin(), OneArray, 20)==0);
    BOOST_CHECK(R1S.size() == sizeof(R1S));
    BOOST_CHECK(sizeof(R1S) == 20);
    BOOST_CHECK(R1S.size() == 20);
    BOOST_CHECK(R2S.size() == 20);
    BOOST_CHECK(ZeroS.size() == 20);
    BOOST_CHECK(MaxS.size() == 20);
    BOOST_CHECK(R1S.begin() + 20 == R1S.end());
    BOOST_CHECK(R2S.begin() + 20 == R2S.end());
    BOOST_CHECK(OneS.begin() + 20 == OneS.end());
    BOOST_CHECK(MaxS.begin() + 20 == MaxS.end());
    BOOST_CHECK(TmpS.begin() + 20 == TmpS.end());
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
}

BOOST_AUTO_TEST_CASE( conversion )
{
    BOOST_CHECK(UintToBlob256(BlobToUint256(ZeroL)) == ZeroL);
    BOOST_CHECK(UintToBlob256(BlobToUint256(OneL)) == OneL);
    BOOST_CHECK(UintToBlob256(BlobToUint256(R1L)) == R1L);
    BOOST_CHECK(UintToBlob256(BlobToUint256(R2L)) == R2L);
    BOOST_CHECK(BlobToUint256(ZeroL) == 0);
    BOOST_CHECK(BlobToUint256(OneL) == 1);
    BOOST_CHECK(UintToBlob256(0) == ZeroL);
    BOOST_CHECK(UintToBlob256(1) == OneL);
    BOOST_CHECK(uint256(R1L.GetHex()) == BlobToUint256(R1L));
    BOOST_CHECK(uint256(R2L.GetHex()) == BlobToUint256(R2L));
    BOOST_CHECK(R1L.GetHex() == BlobToUint256(R1L).GetHex());
    BOOST_CHECK(R2L.GetHex() == BlobToUint256(R2L).GetHex());
}

BOOST_AUTO_TEST_SUITE_END()

