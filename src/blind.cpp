// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "blind.h"

#include <assert.h>
#include <secp256k1.h>
#include <secp256k1_rangeproof.h>

#include "support/allocators/secure.h"
#include "random.h"
#include "util.h"


secp256k1_context *secp256k1_ctx_blind = nullptr;

static int CountLeadingZeros(uint64_t nValueIn)
{
    int nZeros = 0;

    for (size_t i = 0; i < 64; ++i, nValueIn >>= 1)
    {
        if ((nValueIn & 1))
            break;
        nZeros++;
    };

    return nZeros;
};

static int CountTrailingZeros(uint64_t nValueIn)
{
    int nZeros = 0;

    uint64_t mask = ((uint64_t)1) << 63;
    for (size_t i = 0; i < 64; ++i, nValueIn <<= 1)
    {
        if ((nValueIn & mask))
            break;
        nZeros++;
    };

    return nZeros;
};

static int64_t ipow(int64_t base, int exp)
{
    int64_t result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    };
    return result;
};


int SelectRangeProofParameters(uint64_t nValueIn, uint64_t &minValue, int &exponent, int &nBits)
{

    int nLeadingZeros = CountLeadingZeros(nValueIn);
    int nTrailingZeros = CountTrailingZeros(nValueIn);

    size_t nBitsReq = 64 - nLeadingZeros - nTrailingZeros;


    nBits = 32;

    // TODO: output rangeproof parameters should depend on the parameters of the inputs
    // TODO: drop low value bits to fee

    if (nValueIn == 0)
    {
        exponent = GetRandInt(5);
        if (GetRandInt(10) == 0) // sometimes raise the exponent
            nBits += GetRandInt(5);
        return 0;
    };


    uint64_t nTest = nValueIn;
    size_t nDiv10; // max exponent
    for (nDiv10 = 0; nTest % 10 == 0; nDiv10++, nTest /= 10) ;


    // TODO: how to pick best?

    int eMin = nDiv10 / 2;
    exponent = eMin + GetRandInt(nDiv10-eMin);


    nTest = nValueIn / ipow(10, exponent);

    nLeadingZeros = CountLeadingZeros(nTest);
    nTrailingZeros = CountTrailingZeros(nTest);

    nBitsReq = 64 - nTrailingZeros;




    if (nBitsReq > 32)
    {
        nBits = nBitsReq;
    };

    // make multiple of 4
    while (nBits < 63 && nBits % 4 != 0)
        nBits++;


    return 0;
};

int GetRangeProofInfo(const std::vector<uint8_t> &vRangeproof, int &rexp, int &rmantissa, CAmount &min_value, CAmount &max_value)
{
    return (!(secp256k1_rangeproof_info(secp256k1_ctx_blind,
        &rexp, &rmantissa, (uint64_t*) &min_value, (uint64_t*) &max_value,
        &vRangeproof[0], vRangeproof.size()) == 1));
};

void ECC_Start_Blinding()
{
    assert(secp256k1_ctx_blind == nullptr);

    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    assert(ctx != nullptr);

    {
        // Pass in a random blinding seed to the secp256k1 context.
        std::vector<unsigned char, secure_allocator<unsigned char>> vseed(32);
        GetRandBytes(vseed.data(), 32);
        bool ret = secp256k1_context_randomize(ctx, vseed.data());
        assert(ret);
    }

    secp256k1_ctx_blind = ctx;
};

void ECC_Stop_Blinding()
{
    secp256k1_context *ctx = secp256k1_ctx_blind;
    secp256k1_ctx_blind = nullptr;

    if (ctx)
    {
        secp256k1_context_destroy(ctx);
    };
};
