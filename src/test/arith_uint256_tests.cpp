// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>

#include <arith_uint256.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

/// Convert vector to arith_uint256, via uint256 blob
static inline arith_uint256 arith_uint256V(const std::vector<unsigned char>& vch)
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
static std::string ArrayToString(const unsigned char A[], unsigned int width)
{
    std::stringstream Stream;
    Stream << std::hex;
    for (unsigned int i = 0; i < width; ++i)
    {
        Stream<<std::setw(2)<<std::setfill('0')<<(unsigned int)A[width-i-1];
    }
    return Stream.str();
}

TEST(ArithUint256Tests, Basics) // constructors, equality, inequality
{
    EXPECT_TRUE(1 == 0+1);
    // constructor arith_uint256(vector<char>):
    EXPECT_TRUE(R1L.ToString() == ArrayToString(R1Array,32));
    EXPECT_TRUE(R2L.ToString() == ArrayToString(R2Array,32));
    EXPECT_TRUE(ZeroL.ToString() == ArrayToString(ZeroArray,32));
    EXPECT_TRUE(OneL.ToString() == ArrayToString(OneArray,32));
    EXPECT_TRUE(MaxL.ToString() == ArrayToString(MaxArray,32));
    EXPECT_TRUE(OneL.ToString() != ArrayToString(ZeroArray,32));

    // == and !=
    EXPECT_TRUE(R1L != R2L);
    EXPECT_TRUE(ZeroL != OneL);
    EXPECT_TRUE(OneL != ZeroL);
    EXPECT_TRUE(MaxL != ZeroL);
    EXPECT_TRUE(~MaxL == ZeroL);
    EXPECT_TRUE( ((R1L ^ R2L) ^ R1L) == R2L);

    uint64_t Tmp64 = 0xc4dab720d9c7acaaULL;
    for (unsigned int i = 0; i < 256; ++i)
    {
        EXPECT_TRUE(ZeroL != (OneL << i));
        EXPECT_TRUE((OneL << i) != ZeroL);
        EXPECT_TRUE(R1L != (R1L ^ (OneL << i)));
        EXPECT_TRUE(((arith_uint256(Tmp64) ^ (OneL << i) ) != Tmp64 ));
    }
    EXPECT_TRUE(ZeroL == (OneL << 256));

    // Construct from hex string
    EXPECT_EQ(UintToArith256(uint256::FromHex(R1L.ToString()).value()), R1L);
    EXPECT_EQ(UintToArith256(uint256::FromHex(R2L.ToString()).value()), R2L);
    EXPECT_EQ(UintToArith256(uint256::FromHex(ZeroL.ToString()).value()), ZeroL);
    EXPECT_EQ(UintToArith256(uint256::FromHex(OneL.ToString()).value()), OneL);
    EXPECT_EQ(UintToArith256(uint256::FromHex(MaxL.ToString()).value()), MaxL);
    EXPECT_EQ(UintToArith256(uint256::FromHex(R1ArrayHex).value()), R1L);

    // Copy constructor
    EXPECT_TRUE(arith_uint256(R1L) == R1L);
    EXPECT_TRUE((arith_uint256(R1L^R2L)^R2L) == R1L);
    EXPECT_TRUE(arith_uint256(ZeroL) == ZeroL);
    EXPECT_TRUE(arith_uint256(OneL) == OneL);

    // uint64_t constructor
    EXPECT_EQ(R1L & arith_uint256{0xffffffffffffffff}, arith_uint256{R1LLow64});
    EXPECT_EQ(ZeroL, arith_uint256{0});
    EXPECT_EQ(OneL, arith_uint256{1});
    EXPECT_EQ(arith_uint256{0xffffffffffffffff}, arith_uint256{0xffffffffffffffffULL});

    // Assignment (from base_uint)
    arith_uint256 tmpL = ~ZeroL; EXPECT_TRUE(tmpL == ~ZeroL);
    tmpL = ~OneL; EXPECT_TRUE(tmpL == ~OneL);
    tmpL = ~R1L; EXPECT_TRUE(tmpL == ~R1L);
    tmpL = ~R2L; EXPECT_TRUE(tmpL == ~R2L);
    tmpL = ~MaxL; EXPECT_TRUE(tmpL == ~MaxL);
}

static void shiftArrayRight(unsigned char* to, const unsigned char* from, unsigned int arrayLength, unsigned int bitsToShift)
{
    for (unsigned int T=0; T < arrayLength; ++T)
    {
        unsigned int F = (T+bitsToShift/8);
        if (F < arrayLength)
            to[T] = uint8_t(from[F] >> (bitsToShift % 8));
        else
            to[T] = 0;
        if (F + 1 < arrayLength)
            to[T] |= uint8_t(from[(F + 1)] << (8 - bitsToShift % 8));
    }
}

static void shiftArrayLeft(unsigned char* to, const unsigned char* from, unsigned int arrayLength, unsigned int bitsToShift)
{
    for (unsigned int T=0; T < arrayLength; ++T)
    {
        if (T >= bitsToShift/8)
        {
            unsigned int F = T-bitsToShift/8;
            to[T] = uint8_t(from[F] << (bitsToShift % 8));
            if (T >= bitsToShift/8+1)
                to[T] |= uint8_t(from[F - 1] >> (8 - bitsToShift % 8));
        }
        else {
            to[T] = 0;
        }
    }
}

TEST(ArithUint256Tests, Shifts) // "<<", ">>", "<<=", ">>="
{
    unsigned char TmpArray[32];
    arith_uint256 TmpL;
    for (unsigned int i = 0; i < 256; ++i)
    {
        shiftArrayLeft(TmpArray, OneArray, 32, i);
        EXPECT_TRUE(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (OneL << i));
        TmpL = OneL; TmpL <<= i;
        EXPECT_TRUE(TmpL == (OneL << i));
        EXPECT_TRUE((HalfL >> (255-i)) == (OneL << i));
        TmpL = HalfL; TmpL >>= (255-i);
        EXPECT_TRUE(TmpL == (OneL << i));

        shiftArrayLeft(TmpArray, R1Array, 32, i);
        EXPECT_TRUE(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (R1L << i));
        TmpL = R1L; TmpL <<= i;
        EXPECT_TRUE(TmpL == (R1L << i));

        shiftArrayRight(TmpArray, R1Array, 32, i);
        EXPECT_TRUE(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (R1L >> i));
        TmpL = R1L; TmpL >>= i;
        EXPECT_TRUE(TmpL == (R1L >> i));

        shiftArrayLeft(TmpArray, MaxArray, 32, i);
        EXPECT_TRUE(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (MaxL << i));
        TmpL = MaxL; TmpL <<= i;
        EXPECT_TRUE(TmpL == (MaxL << i));

        shiftArrayRight(TmpArray, MaxArray, 32, i);
        EXPECT_TRUE(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (MaxL >> i));
        TmpL = MaxL; TmpL >>= i;
        EXPECT_TRUE(TmpL == (MaxL >> i));
    }
    arith_uint256 c1L = arith_uint256(0x0123456789abcdefULL);
    arith_uint256 c2L = c1L << 128;
    for (unsigned int i = 0; i < 128; ++i) {
        EXPECT_TRUE((c1L << i) == (c2L >> (128-i)));
    }
    for (unsigned int i = 128; i < 256; ++i) {
        EXPECT_TRUE((c1L << i) == (c2L << (i-128)));
    }
}

TEST(ArithToUint256Tests, UnaryOperators)
{
    EXPECT_TRUE(~ZeroL == MaxL);

    unsigned char TmpArray[32];
    for (unsigned int i = 0; i < 32; ++i) { TmpArray[i] = uint8_t(~R1Array[i]); }
    EXPECT_TRUE(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (~R1L));

    EXPECT_TRUE(-ZeroL == ZeroL);
    EXPECT_TRUE(-R1L == (~R1L)+1);
    for (unsigned int i = 0; i < 256; ++i)
        EXPECT_TRUE(-(OneL<<i) == (MaxL << i));
}


// Check if doing _A_ _OP_ _B_ results in the same as applying _OP_ onto each
// element of Aarray and Barray, and then converting the result into an arith_uint256.
#define CHECKBITWISEOPERATOR(_A_,_B_,_OP_)                              \
    for (unsigned int i = 0; i < 32; ++i) { TmpArray[i] = uint8_t(_A_##Array[i] _OP_ _B_##Array[i]); } \
    EXPECT_TRUE(arith_uint256V(std::vector<unsigned char>(TmpArray,TmpArray+32)) == (_A_##L _OP_ _B_##L));

#define CHECKASSIGNMENTOPERATOR(_A_,_B_,_OP_)                           \
    TmpL = _A_##L; TmpL _OP_##= _B_##L; EXPECT_TRUE(TmpL == (_A_##L _OP_ _B_##L));

TEST(ArithUint256Tests, BitwiseOperators)
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
    TmpL = R1L; TmpL |= Tmp64;  EXPECT_TRUE(TmpL == (R1L | arith_uint256(Tmp64)));
    TmpL = R1L; TmpL |= 0; EXPECT_TRUE(TmpL == R1L);
    TmpL ^= 0; EXPECT_TRUE(TmpL == R1L);
    TmpL ^= Tmp64;  EXPECT_TRUE(TmpL == (R1L ^ arith_uint256(Tmp64)));
}

TEST(ArithUint256Tests, Comparison) // <= >= < >
{
    arith_uint256 TmpL;
    for (unsigned int i = 0; i < 256; ++i) {
        TmpL= OneL<< i;
        EXPECT_TRUE( TmpL >= ZeroL && TmpL > ZeroL && ZeroL < TmpL && ZeroL <= TmpL);
        EXPECT_TRUE( TmpL >= 0 && TmpL > 0 && 0 < TmpL && 0 <= TmpL);
        TmpL |= R1L;
        EXPECT_TRUE( TmpL >= R1L ); EXPECT_TRUE( (TmpL == R1L) != (TmpL > R1L)); EXPECT_TRUE( (TmpL == R1L) || !( TmpL <= R1L));
        EXPECT_TRUE( R1L <= TmpL ); EXPECT_TRUE( (R1L == TmpL) != (R1L < TmpL)); EXPECT_TRUE( (TmpL == R1L) || !( R1L >= TmpL));
        EXPECT_TRUE(! (TmpL < R1L)); EXPECT_TRUE(! (R1L > TmpL));
    }

    EXPECT_LT(ZeroL, OneL);
}

TEST(ArithUint256Tests, PlusMinus)
{
    arith_uint256 TmpL = 0;
    EXPECT_EQ(R1L + R2L, UintToArith256(uint256{"549fb09fea236a1ea3e31d4d58f1b1369288d204211ca751527cfc175767850c"}));
    TmpL += R1L;
    EXPECT_TRUE(TmpL == R1L);
    TmpL += R2L;
    EXPECT_TRUE(TmpL == R1L + R2L);
    EXPECT_TRUE(OneL+MaxL == ZeroL);
    EXPECT_TRUE(MaxL+OneL == ZeroL);
    for (unsigned int i = 1; i < 256; ++i) {
        EXPECT_TRUE( (MaxL >> i) + OneL == (HalfL >> (i-1)) );
        EXPECT_TRUE( OneL + (MaxL >> i) == (HalfL >> (i-1)) );
        TmpL = (MaxL>>i); TmpL += OneL;
        EXPECT_TRUE( TmpL == (HalfL >> (i-1)) );
        TmpL = (MaxL>>i); TmpL += 1;
        EXPECT_TRUE( TmpL == (HalfL >> (i-1)) );
        TmpL = (MaxL>>i);
        EXPECT_TRUE( TmpL++ == (MaxL>>i) );
        EXPECT_TRUE( TmpL == (HalfL >> (i-1)));
    }
    EXPECT_TRUE(arith_uint256(0xbedc77e27940a7ULL) + 0xee8d836fce66fbULL == arith_uint256(0xbedc77e27940a7ULL + 0xee8d836fce66fbULL));
    TmpL = arith_uint256(0xbedc77e27940a7ULL); TmpL += 0xee8d836fce66fbULL;
    EXPECT_TRUE(TmpL == arith_uint256(0xbedc77e27940a7ULL+0xee8d836fce66fbULL));
    TmpL -= 0xee8d836fce66fbULL;  EXPECT_TRUE(TmpL == 0xbedc77e27940a7ULL);
    TmpL = R1L;
    EXPECT_TRUE(++TmpL == R1L+1);

    EXPECT_TRUE(R1L -(-R2L) == R1L+R2L);
    EXPECT_TRUE(R1L -(-OneL) == R1L+OneL);
    EXPECT_TRUE(R1L - OneL == R1L+(-OneL));
    for (unsigned int i = 1; i < 256; ++i) {
        EXPECT_TRUE((MaxL>>i) - (-OneL)  == (HalfL >> (i-1)));
        EXPECT_TRUE((HalfL >> (i-1)) - OneL == (MaxL>>i));
        TmpL = (HalfL >> (i-1));
        EXPECT_TRUE(TmpL-- == (HalfL >> (i-1)));
        EXPECT_TRUE(TmpL == (MaxL >> i));
        TmpL = (HalfL >> (i-1));
        EXPECT_TRUE(--TmpL == (MaxL >> i));
    }
    TmpL = R1L;
    EXPECT_TRUE(--TmpL == R1L-1);
}

TEST(ArithUint256Tests, Multiply)
{
    EXPECT_TRUE((R1L * R1L).ToString() == "62a38c0486f01e45879d7910a7761bf30d5237e9873f9bff3642a732c4d84f10");
    EXPECT_TRUE((R1L * R2L).ToString() == "de37805e9986996cfba76ff6ba51c008df851987d9dd323f0e5de07760529c40");
    EXPECT_TRUE((R1L * ZeroL) == ZeroL);
    EXPECT_TRUE((R1L * OneL) == R1L);
    EXPECT_TRUE((R1L * MaxL) == -R1L);
    EXPECT_TRUE((R2L * R1L) == (R1L * R2L));
    EXPECT_TRUE((R2L * R2L).ToString() == "ac8c010096767d3cae5005dec28bb2b45a1d85ab7996ccd3e102a650f74ff100");
    EXPECT_TRUE((R2L * ZeroL) == ZeroL);
    EXPECT_TRUE((R2L * OneL) == R2L);
    EXPECT_TRUE((R2L * MaxL) == -R2L);

    EXPECT_TRUE(MaxL * MaxL == OneL);

    EXPECT_TRUE((R1L * 0) == 0);
    EXPECT_TRUE((R1L * 1) == R1L);
    EXPECT_TRUE((R1L * 3).ToString() == "7759b1c0ed14047f961ad09b20ff83687876a0181a367b813634046f91def7d4");
    EXPECT_TRUE((R2L * 0x87654321UL).ToString() == "23f7816e30c4ae2017257b7a0fa64d60402f5234d46e746b61c960d09a26d070");
}

TEST(ArithUint256Tests, Divide)
{
    arith_uint256 D1L{UintToArith256(uint256{"00000000000000000000000000000000000000000000000ad7133ac1977fa2b7"})};
    arith_uint256 D2L{UintToArith256(uint256{"0000000000000000000000000000000000000000000000000000000ecd751716"})};
    EXPECT_TRUE((R1L / D1L).ToString() == "00000000000000000b8ac01106981635d9ed112290f8895545a7654dde28fb3a");
    EXPECT_TRUE((R1L / D2L).ToString() == "000000000873ce8efec5b67150bad3aa8c5fcb70e947586153bf2cec7c37c57a");
    EXPECT_TRUE(R1L / OneL == R1L);
    EXPECT_TRUE(R1L / MaxL == ZeroL);
    EXPECT_TRUE(MaxL / R1L == 2);
    EXPECT_THROW(R1L / ZeroL, uint_error);
    EXPECT_TRUE((R2L / D1L).ToString() == "000000000000000013e1665895a1cc981de6d93670105a6b3ec3b73141b3a3c5");
    EXPECT_TRUE((R2L / D2L).ToString() == "000000000e8f0abe753bb0afe2e9437ee85d280be60882cf0bd1aaf7fa3cc2c4");
    EXPECT_TRUE(R2L / OneL == R2L);
    EXPECT_TRUE(R2L / MaxL == ZeroL);
    EXPECT_TRUE(MaxL / R2L == 1);
    EXPECT_THROW(R2L / ZeroL, uint_error);
}

static bool almostEqual(double d1, double d2)
{
    return fabs(d1-d2) <= 4*fabs(d1)*std::numeric_limits<double>::epsilon();
}

TEST(ArithUint256Tests, Methods) // GetHex operator= size() GetLow64 GetSerializeSize, Serialize, Unserialize
{
    EXPECT_TRUE(R1L.GetHex() == R1L.ToString());
    EXPECT_TRUE(R2L.GetHex() == R2L.ToString());
    EXPECT_TRUE(OneL.GetHex() == OneL.ToString());
    EXPECT_TRUE(MaxL.GetHex() == MaxL.ToString());
    arith_uint256 TmpL(R1L);
    EXPECT_TRUE(TmpL == R1L);
    TmpL = R2L;
    EXPECT_TRUE(TmpL == R2L);
    TmpL = ZeroL;
    EXPECT_TRUE(TmpL == 0);
    TmpL = HalfL;
    EXPECT_TRUE(TmpL == HalfL);

    TmpL = R1L;
    EXPECT_TRUE(R1L.size() == 32);
    EXPECT_TRUE(R2L.size() == 32);
    EXPECT_TRUE(ZeroL.size() == 32);
    EXPECT_TRUE(MaxL.size() == 32);
    EXPECT_TRUE(R1L.GetLow64()  == R1LLow64);
    EXPECT_TRUE(HalfL.GetLow64() ==0x0000000000000000ULL);
    EXPECT_TRUE(OneL.GetLow64() ==0x0000000000000001ULL);

    for (unsigned int i = 0; i < 255; ++i)
    {
        EXPECT_TRUE((OneL << i).getdouble() == ldexp(1.0,i));
    }
    EXPECT_TRUE(ZeroL.getdouble() == 0.0);
    for (int i = 256; i > 53; --i)
        EXPECT_TRUE(almostEqual((R1L>>(256-i)).getdouble(), ldexp(R1Ldouble,i)));
    uint64_t R1L64part = (R1L>>192).GetLow64();
    for (int i = 53; i > 0; --i) // doubles can store all integers in {0,...,2^54-1} exactly
    {
        EXPECT_TRUE((R1L>>(256-i)).getdouble() == (double)(R1L64part >> (64-i)));
    }
}

TEST(ArithUint256Tests, BignumSetCompact)
{
    arith_uint256 num;
    bool fNegative;
    bool fOverflow;
    num.SetCompact(0, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x00123456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x01003456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x02000056, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x03000000, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x04000000, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x00923456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x01803456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x02800056, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x03800000, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x04800000, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x01123456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000000012");
    EXPECT_EQ(num.GetCompact(), 0x01120000U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    // Make sure that we don't generate compacts with the 0x00800000 bit set
    num = 0x80;
    EXPECT_EQ(num.GetCompact(), 0x02008000U);

    num.SetCompact(0x01fedcba, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "000000000000000000000000000000000000000000000000000000000000007e");
    EXPECT_EQ(num.GetCompact(true), 0x01fe0000U);
    EXPECT_EQ(fNegative, true);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x02123456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000001234");
    EXPECT_EQ(num.GetCompact(), 0x02123400U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x03123456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000000123456");
    EXPECT_EQ(num.GetCompact(), 0x03123456U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x04123456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000012345600");
    EXPECT_EQ(num.GetCompact(), 0x04123456U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x04923456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000012345600");
    EXPECT_EQ(num.GetCompact(true), 0x04923456U);
    EXPECT_EQ(fNegative, true);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x05009234, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "0000000000000000000000000000000000000000000000000000000092340000");
    EXPECT_EQ(num.GetCompact(), 0x05009234U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0x20123456, &fNegative, &fOverflow);
    EXPECT_EQ(num.GetHex(), "1234560000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(num.GetCompact(), 0x20123456U);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, false);

    num.SetCompact(0xff123456, &fNegative, &fOverflow);
    EXPECT_EQ(fNegative, false);
    EXPECT_EQ(fOverflow, true);
}

TEST(ArithUint256Tests, GetMaxCoverage) // some more tests just to get 100% coverage
{
    // ~R1L give a base_uint<256>
    EXPECT_TRUE((~~R1L >> 10) == (R1L >> 10));
    EXPECT_TRUE((~~R1L << 10) == (R1L << 10));
    EXPECT_TRUE(!(~~R1L < R1L));
    EXPECT_TRUE(~~R1L <= R1L);
    EXPECT_TRUE(!(~~R1L > R1L));
    EXPECT_TRUE(~~R1L >= R1L);
    EXPECT_TRUE(!(R1L < ~~R1L));
    EXPECT_TRUE(R1L <= ~~R1L);
    EXPECT_TRUE(!(R1L > ~~R1L));
    EXPECT_TRUE(R1L >= ~~R1L);

    EXPECT_TRUE(~~R1L + R2L == R1L + ~~R2L);
    EXPECT_TRUE(~~R1L - R2L == R1L - ~~R2L);
    EXPECT_TRUE(~R1L != R1L); EXPECT_TRUE(R1L != ~R1L);
    unsigned char TmpArray[32];
    CHECKBITWISEOPERATOR(~R1,R2,|)
    CHECKBITWISEOPERATOR(~R1,R2,^)
    CHECKBITWISEOPERATOR(~R1,R2,&)
    CHECKBITWISEOPERATOR(R1,~R2,|)
    CHECKBITWISEOPERATOR(R1,~R2,^)
    CHECKBITWISEOPERATOR(R1,~R2,&)
}

TEST(ArithUint256Tests, Conversion)
{
    for (const arith_uint256& arith : {ZeroL, OneL, R1L, R2L}) {
        const auto u256{uint256::FromHex(arith.GetHex()).value()};
        EXPECT_EQ(UintToArith256(ArithToUint256(arith)), arith);
        EXPECT_EQ(UintToArith256(u256), arith);
        EXPECT_EQ(u256, ArithToUint256(arith));
        EXPECT_EQ(ArithToUint256(arith).GetHex(), UintToArith256(u256).GetHex());
    }

    for (uint8_t num : {0, 1, 0xff}) {
        EXPECT_EQ(UintToArith256(uint256{num}), arith_uint256{num});
        EXPECT_EQ(uint256{num}, ArithToUint256(arith_uint256{num}));
        EXPECT_EQ(UintToArith256(uint256{num}), num);
    }
}

TEST(ArithUint256Tests, GeneratorWithSelf)
{
    /* Clang 16 and earlier detects v -= v and v /= v as self-assignments
       to 0 and 1 respectively.
       See: https://github.com/llvm/llvm-project/issues/42469
       and the fix in commit c5302325b2a62d77cf13dd16cd5c19141862fed0 .

       This makes some sense for arithmetic classes, but could be considered a bug
       elsewhere. Disable the warning here so that the code can be tested, but the
       warning should remain on as there will likely always be a better way to
       express this.
    */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
    arith_uint256 v{2};
    v *= v;
    EXPECT_EQ(v, arith_uint256{4});
    v /= v;
    EXPECT_EQ(v, arith_uint256{1});
    v += v;
    EXPECT_EQ(v, arith_uint256{2});
    v -= v;
    EXPECT_EQ(v, arith_uint256{0});
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
}
