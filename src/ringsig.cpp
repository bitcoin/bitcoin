// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "ringsig.h"
#include "base58.h"

#include <openssl/rand.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>


static EC_GROUP* ecGrp         = NULL;
static BN_CTX*   bnCtx         = NULL;
static BIGNUM*   bnP           = NULL;
static BIGNUM*   bnN           = NULL;

int initialiseRingSigs()
{
    if (fDebugRingSig)
        LogPrintf("initialiseRingSigs()\n");
    
    if (!ecGrp && !(ecGrp = EC_GROUP_new_by_curve_name(NID_secp256k1)))
    {
        LogPrintf("initialiseRingSigs(): EC_GROUP_new_by_curve_name failed.\n");
        return 1;
    };
    
    if (!bnCtx && !(bnCtx = BN_CTX_new()))
    {
        LogPrintf("initialiseRingSigs(): BN_CTX_new failed.\n");
        return 1;
    };
    
    bnN = BN_new();
    EC_GROUP_get_order(ecGrp, bnN, bnCtx);
    
    bnP = BN_new();
    EC_GROUP_get_curve_GFp(ecGrp, bnP, NULL, NULL, bnCtx);
    
    return 0;
};

int finaliseRingSigs()
{
    if (fDebugRingSig)
        LogPrintf("finaliseRingSigs()\n");
    
    if (bnN)
        BN_free(bnN);
    if (bnP)
        BN_free(bnP);
    if (bnCtx)
        BN_CTX_free(bnCtx);
    if (ecGrp)
        EC_GROUP_free(ecGrp);
    
    return 0;
};

int splitAmount(int64_t nValue, std::vector<int64_t>& vOut)
{
    int64_t nTest = 1;
    int i;
    
    // split amounts into 1, 3, 4, 5
     
    while (nValue >= nTest)
    {
        i = (nValue / nTest) % 10;
        switch (i)
        {
            case 0:
                break;
            case 2:
                vOut.push_back(1*nTest);
                vOut.push_back(1*nTest);
                break;
            case 6:
                vOut.push_back(5*nTest);
                vOut.push_back(1*nTest);
                break;
            case 7:
                vOut.push_back(3*nTest);
                vOut.push_back(4*nTest);
                break;
            case 8:
                vOut.push_back(5*nTest);
                vOut.push_back(3*nTest);
                break;
            case 9:
                vOut.push_back(5*nTest);
                vOut.push_back(4*nTest);
                break;
            default:
                vOut.push_back(i*nTest);
        };
        nTest *= 10;
    };
    
    return 0;
};

static int hashToEC(const uint8_t* p, uint32_t len, BIGNUM* bnTmp, EC_POINT* ptRet)
{
    // bn(hash(data)) * G
    uint256 pkHash = Hash(p, p + len);
    
    if (!bnTmp || !(BN_bin2bn(pkHash.begin(), 32, bnTmp)))
    {
        LogPrintf("hashToEC(): BN_bin2bn failed.\n");
        return 1;
    };
    
    if (!ptRet
        || !EC_POINT_mul(ecGrp, ptRet, bnTmp, NULL, NULL, bnCtx))
    {
        LogPrintf("hashToEC() EC_POINT_mul failed.\n");
        return 1;
    };
    
    return 0;
};

int generateKeyImage(ec_point &publicKey, ec_secret secret, ec_point &keyImage)
{
    // -- keyImage = secret * hash(publicKey) * G
    
    if (publicKey.size() != ec_compressed_size)
    {
        LogPrintf("generateKeyImage(): invalid publicKey.\n");
        return 1;
    };
    
    int rv = 0;
    BN_CTX_start(bnCtx);
    BIGNUM*   bnTmp     = BN_CTX_get(bnCtx);
    BIGNUM*   bnSec     = BN_CTX_get(bnCtx);
    EC_POINT* hG        = NULL;
    
    if (!(hG = EC_POINT_new(ecGrp)))
    {
        LogPrintf("generateKeyImage() EC_POINT_new failed.\n");
        rv = 1; goto End;
    };
    
    if (hashToEC(&publicKey[0], publicKey.size(), bnTmp, hG) != 0)
    {
        LogPrintf("generateKeyImage(): hashToEC failed.\n");
        rv = 1; goto End;
    };
    
    if (!bnSec || !(BN_bin2bn(&secret.e[0], 32, bnSec)))
    {
        LogPrintf("generateKeyImage(): BN_bin2bn failed.\n");
        rv = 1; goto End;
    };
    
    if (!EC_POINT_mul(ecGrp, hG, NULL, hG, bnSec, bnCtx))
    {
        LogPrintf("kimg EC_POINT_mul failed.\n");
        rv = 1; goto End;
    };
    
    try { keyImage.resize(ec_compressed_size); } catch (std::exception& e)
    {
        LogPrintf("keyImage.resize threw: %s.\n", e.what());
        rv = 1; goto End;
    };
    
    if (!(EC_POINT_point2bn(ecGrp, hG, POINT_CONVERSION_COMPRESSED, bnTmp, bnCtx))
        || BN_num_bytes(bnTmp) != (int) ec_compressed_size
        || BN_bn2bin(bnTmp, &keyImage[0]) != (int) ec_compressed_size)
    {
        LogPrintf("point -> keyImage failed.\n");
        rv = 1; goto End;
    };
    
    if (fDebugRingSig)
        LogPrintf("keyImage %s\n", HexStr(keyImage).c_str());
    
    End:
    if (hG)
        EC_POINT_free(hG);
    BN_CTX_end(bnCtx);
    
    return rv;
};


int generateRingSignature(std::vector<uint8_t>& keyImage, uint256& txnHash, int nRingSize, int nSecretOffset, ec_secret secret, const uint8_t *pPubkeys, uint8_t *pSigc, uint8_t *pSigr)
{
    if (fDebugRingSig)
        LogPrintf("generateRingSignature() size %d\n", nRingSize);
    
    int rv = 0;
    int nBytes;
    
    BN_CTX_start(bnCtx);
    
    BIGNUM*   bnKS     = BN_CTX_get(bnCtx);
    BIGNUM*   bnK1     = BN_CTX_get(bnCtx);
    BIGNUM*   bnK2     = BN_CTX_get(bnCtx);
    BIGNUM*   bnT      = BN_CTX_get(bnCtx);
    BIGNUM*   bnH      = BN_CTX_get(bnCtx);
    BIGNUM*   bnSum    = BN_CTX_get(bnCtx);
    EC_POINT* ptT1     = NULL;
    EC_POINT* ptT2     = NULL;
    EC_POINT* ptT3     = NULL;
    EC_POINT* ptPk     = NULL;
    EC_POINT* ptKi     = NULL;
    EC_POINT* ptL      = NULL;
    EC_POINT* ptR      = NULL;
    
    uint8_t tempData[66]; // hold raw point data to hash
    uint256 commitHash;
    ec_secret scData1, scData2;
    
    CHashWriter ssCommitHash(SER_GETHASH, PROTOCOL_VERSION);
    
    ssCommitHash << txnHash;
    
    
    // zero signature
    memset(pSigc, 0, ec_secret_size * nRingSize);
    memset(pSigr, 0, ec_secret_size * nRingSize);
    
    
    // ks = random 256 bit int mod P
    if (GenerateRandomSecret(scData1) != 0)
    {
        LogPrintf("generateRingSignature(): GenerateRandomSecret failed.\n");
        rv = 1; goto End;
    };
    
    if (!bnKS || !(BN_bin2bn(&scData1.e[0], 32, bnKS)))
    {
        LogPrintf("generateRingSignature(): BN_bin2bn failed.\n");
        rv = 1; goto End;
    };
    
    // zero sum
    if (!bnSum || !(BN_zero(bnSum)))
    {
        LogPrintf("generateRingSignature(): BN_zero failed.\n");
        rv = 1; goto End;
    };
    
    if (   !(ptT1 = EC_POINT_new(ecGrp))
        || !(ptT2 = EC_POINT_new(ecGrp))
        || !(ptT3 = EC_POINT_new(ecGrp))
        || !(ptPk = EC_POINT_new(ecGrp))
        || !(ptKi = EC_POINT_new(ecGrp))
        || !(ptL  = EC_POINT_new(ecGrp))
        || !(ptR  = EC_POINT_new(ecGrp)))
    {
        LogPrintf("generateRingSignature(): EC_POINT_new failed.\n");
        rv = 1; goto End;
    };
    
    // get keyimage as point
    if (!(bnT = BN_bin2bn(&keyImage[0], ec_compressed_size, bnT))
        || !(ptKi) || !(ptKi = EC_POINT_bn2point(ecGrp, bnT, ptKi, bnCtx)))
    {
        LogPrintf("generateRingSignature(): extract ptKi failed\n");
        rv = 1; goto End;
    };
    
    for (int i = 0; i < nRingSize; ++i)
    {
        if (i == nSecretOffset)
        {
            // k = random 256 bit int mod P
            // L = k * G
            // R = k * HashToEC(PKi)
            
            if (!EC_POINT_mul(ecGrp, ptL, bnKS, NULL, NULL, bnCtx))
            {
                LogPrintf("generateRingSignature(): EC_POINT_mul failed.\n");
                rv = 1; goto End;
            };
            
            if (hashToEC(&pPubkeys[i * ec_compressed_size], ec_compressed_size, bnT, ptT1) != 0)
            {
                LogPrintf("generateRingSignature(): hashToEC failed.\n");
                rv = 1; goto End;
            };
            
            if (!EC_POINT_mul(ecGrp, ptR, NULL, ptT1, bnKS, bnCtx))
            {
                LogPrintf("generateRingSignature(): EC_POINT_mul failed.\n");
                rv = 1; goto End;
            };
            
        } else
        {
            // k1 = random 256 bit int mod P
            // k2 = random 256 bit int mod P
            // Li = k1 * Pi + k2 * G
            // Ri = k1 * I + k2 * Hp(Pi)
            // ci = k1
            // ri = k2
            
            if (GenerateRandomSecret(scData1) != 0
                || !bnK1 || !(BN_bin2bn(&scData1.e[0], 32, bnK1))
                || GenerateRandomSecret(scData2) != 0
                || !bnK2 || !(BN_bin2bn(&scData2.e[0], 32, bnK2)))
            {
                LogPrintf("generateRingSignature(): k1 and k2 failed.\n");
                rv = 1; goto End;
            };
            
            // get Pk i as point
            if (!(bnT = BN_bin2bn(&pPubkeys[i * ec_compressed_size], ec_compressed_size, bnT))
                || !(ptPk) || !(ptPk = EC_POINT_bn2point(ecGrp, bnT, ptPk, bnCtx)))
            {
                LogPrintf("generateRingSignature(): extract ptPk failed\n");
                rv = 1; goto End;
            };
            
            // ptT1 = k1 * Pi
            if (!EC_POINT_mul(ecGrp, ptT1, NULL, ptPk, bnK1, bnCtx))
            {
                LogPrintf("generateRingSignature(): EC_POINT_mul failed.\n");
                rv = 1; goto End;
            };
            
            // ptT2 = k2 * G
            if (!EC_POINT_mul(ecGrp, ptT2, bnK2, NULL, NULL, bnCtx))
            {
                LogPrintf("generateRingSignature(): EC_POINT_mul failed.\n");
                rv = 1; goto End;
            };
            
            // ptL = ptT1 + ptT2
            if (!EC_POINT_add(ecGrp, ptL, ptT1, ptT2, bnCtx))
            {
                LogPrintf("generateRingSignature(): EC_POINT_add failed.\n");
                rv = 1; goto End;
            };
            
            // ptT3 = Hp(Pi)
            if (hashToEC(&pPubkeys[i * ec_compressed_size], ec_compressed_size, bnT, ptT3) != 0)
            {
                LogPrintf("generateRingSignature(): hashToEC failed.\n");
                rv = 1; goto End;
            };
            
            // ptT1 = k1 * I
            if (!EC_POINT_mul(ecGrp, ptT1, NULL, ptKi, bnK1, bnCtx))
            {
                LogPrintf("generateRingSignature(): EC_POINT_mul failed.\n");
                rv = 1; goto End;
            };
            
            // ptT2 = k2 * ptT3
            if (!EC_POINT_mul(ecGrp, ptT2, NULL, ptT3, bnK2, bnCtx))
            {
                LogPrintf("generateRingSignature(): EC_POINT_mul failed.\n");
                rv = 1; goto End;
            };
            
            // ptR = ptT1 + ptT2
            if (!EC_POINT_add(ecGrp, ptR, ptT1, ptT2, bnCtx))
            {
                LogPrintf("generateRingSignature(): EC_POINT_add failed.\n");
                rv = 1; goto End;
            };
            
            memcpy(&pSigc[i * ec_secret_size], &scData1.e[0], ec_secret_size);
            memcpy(&pSigr[i * ec_secret_size], &scData2.e[0], ec_secret_size);
            
            // sum = (sum + sigc) % N , sigc == bnK1
            if (!BN_mod_add(bnSum, bnSum, bnK1, bnN, bnCtx))
            {
                LogPrintf("generateRingSignature(): BN_mod_add failed.\n");
                rv = 1; goto End;
            };
        };
        
        // -- add ptL and ptR to hash
        if (!(EC_POINT_point2bn(ecGrp, ptL, POINT_CONVERSION_COMPRESSED, bnT, bnCtx))
            || BN_num_bytes(bnT) != (int) ec_compressed_size
            || BN_bn2bin(bnT, &tempData[0]) != (int) ec_compressed_size
            || !(EC_POINT_point2bn(ecGrp, ptR, POINT_CONVERSION_COMPRESSED, bnT, bnCtx))
            || BN_num_bytes(bnT) != (int) ec_compressed_size
            || BN_bn2bin(bnT, &tempData[33]) != (int) ec_compressed_size)
        {
            LogPrintf("extract ptL and ptR failed.\n");
            rv = 1; goto End;
        };
        
        ssCommitHash.write((const char*)&tempData[0], 66);
    };
    
    commitHash = ssCommitHash.GetHash();
    
    if (!(bnH) || !(bnH = BN_bin2bn(commitHash.begin(), ec_secret_size, bnH)))
    {
        LogPrintf("generateRingSignature(): commitHash -> bnH failed\n");
        rv = 1; goto End;
    };
    
    
    if (!BN_mod(bnH, bnH, bnN, bnCtx)) // this is necessary
    {
        LogPrintf("generateRingSignature(): BN_mod failed.\n");
        rv = 1; goto End;
    };
    
    // sigc[nSecretOffset] = (bnH - bnSum) % N
    if (!BN_mod_sub(bnT, bnH, bnSum, bnN, bnCtx))
    {
        LogPrintf("generateRingSignature(): BN_mod_sub failed.\n");
        rv = 1; goto End;
    };
    
    if ((nBytes = BN_num_bytes(bnT)) > (int)ec_secret_size
        || BN_bn2bin(bnT, &pSigc[nSecretOffset * ec_secret_size + (32-nBytes)]) != nBytes)
    {
        LogPrintf("bnT -> pSigc failed.\n");
        rv = 1; goto End;
    };
    
    // sigr[nSecretOffset] = (bnKS - sigc[nSecretOffset] * bnSecret) % N
    // reuse bnH for bnSecret
    if (!bnH || !(BN_bin2bn(&secret.e[0], 32, bnH)))
    {
        LogPrintf("generateRingSignature(): BN_bin2bn failed.\n");
        rv = 1; goto End;
    };
    
    // bnT = sigc[nSecretOffset] * bnSecret , TODO: mod N ?
    if (!BN_mul(bnT, bnT, bnH, bnCtx))
    {
        LogPrintf("generateRingSignature(): BN_mul failed.\n");
        rv = 1; goto End;
    };
    
    if (!BN_mod_sub(bnT, bnKS, bnT, bnN, bnCtx))
    {
        LogPrintf("generateRingSignature(): BN_mod_sub failed.\n");
        rv = 1; goto End;
    };
    
    if ((nBytes = BN_num_bytes(bnT)) > (int)ec_secret_size
        || BN_bn2bin(bnT, &pSigr[nSecretOffset * ec_secret_size + (32-nBytes)]) != nBytes)
    {
        LogPrintf("bnT -> pSigr failed.\n");
        rv = 1; goto End;
    };
    
    End:
    if (ptT1)
        EC_POINT_free(ptT1);
    if (ptT2)
        EC_POINT_free(ptT2);
    if (ptT3)
        EC_POINT_free(ptT3);
    if (ptPk)
        EC_POINT_free(ptPk);
    if (ptKi)
        EC_POINT_free(ptKi);
    if (ptL)
        EC_POINT_free(ptL);
    if (ptR)
        EC_POINT_free(ptR);
    
    BN_CTX_end(bnCtx);
    
    return rv;
};

int verifyRingSignature(std::vector<uint8_t>& keyImage, uint256& txnHash, int nRingSize, const uint8_t *pPubkeys, const uint8_t *pSigc, const uint8_t *pSigr)
{
    if (fDebugRingSig)
    {
        // LogPrintf("verifyRingSignature() size %d\n", nRingSize); // happens often
    };
    
    int rv = 0;
    
    BN_CTX_start(bnCtx);
    
    BIGNUM*   bnT      = BN_CTX_get(bnCtx);
    BIGNUM*   bnH      = BN_CTX_get(bnCtx);
    BIGNUM*   bnC      = BN_CTX_get(bnCtx);
    BIGNUM*   bnR      = BN_CTX_get(bnCtx);
    BIGNUM*   bnSum    = BN_CTX_get(bnCtx);
    EC_POINT* ptT1     = NULL;
    EC_POINT* ptT2     = NULL;
    EC_POINT* ptT3     = NULL;
    EC_POINT* ptPk     = NULL;
    EC_POINT* ptKi     = NULL;
    EC_POINT* ptL      = NULL;
    EC_POINT* ptR      = NULL;
    
    uint8_t tempData[66]; // hold raw point data to hash
    uint256 commitHash;
    CHashWriter ssCommitHash(SER_GETHASH, PROTOCOL_VERSION);
    
    ssCommitHash << txnHash;
    
    // zero sum
    if (!bnSum || !(BN_zero(bnSum)))
    {
        LogPrintf("verifyRingSignature(): BN_zero failed.\n");
        rv = 1; goto End;
    };
    
    if (   !(ptT1 = EC_POINT_new(ecGrp))
        || !(ptT2 = EC_POINT_new(ecGrp))
        || !(ptT3 = EC_POINT_new(ecGrp))
        || !(ptPk = EC_POINT_new(ecGrp))
        || !(ptKi = EC_POINT_new(ecGrp))
        || !(ptL  = EC_POINT_new(ecGrp))
        || !(ptR  = EC_POINT_new(ecGrp)))
    {
        LogPrintf("verifyRingSignature(): EC_POINT_new failed.\n");
        rv = 1; goto End;
    };
    
    // get keyimage as point
    if (!(bnT = BN_bin2bn(&keyImage[0], ec_compressed_size, bnT))
        || !(ptKi) || !(ptKi = EC_POINT_bn2point(ecGrp, bnT, ptKi, bnCtx)))
    {
        LogPrintf("verifyRingSignature(): extract ptKi failed\n");
        rv = 1; goto End;
    };
    
    for (int i = 0; i < nRingSize; ++i)
    {
        // Li = ci * Pi + ri * G
        // Ri = ci * I + ri * Hp(Pi)
        
        if (!bnC || !(bnC = BN_bin2bn(&pSigc[i * ec_secret_size], ec_secret_size, bnC))
            || !bnR || !(bnR = BN_bin2bn(&pSigr[i * ec_secret_size], ec_secret_size, bnR)))
        {
            LogPrintf("verifyRingSignature(): extract bnC and bnR failed\n");
            rv = 1; goto End;
        };
        
        // get Pk i as point
        if (!(bnT = BN_bin2bn(&pPubkeys[i * ec_compressed_size], ec_compressed_size, bnT))
            || !(ptPk) || !(ptPk = EC_POINT_bn2point(ecGrp, bnT, ptPk, bnCtx)))
        {
            LogPrintf("verifyRingSignature(): extract ptPk failed\n");
            rv = 1; goto End;
        };
        
        // ptT1 = ci * Pi
        if (!EC_POINT_mul(ecGrp, ptT1, NULL, ptPk, bnC, bnCtx))
        {
            LogPrintf("verifyRingSignature(): EC_POINT_mul failed.\n");
            rv = 1; goto End;
        };
        
        // ptT2 = ri * G
        if (!EC_POINT_mul(ecGrp, ptT2, bnR, NULL, NULL, bnCtx))
        {
            LogPrintf("verifyRingSignature(): EC_POINT_mul failed.\n");
            rv = 1; goto End;
        };
        
        // ptL = ptT1 + ptT2
        if (!EC_POINT_add(ecGrp, ptL, ptT1, ptT2, bnCtx))
        {
            LogPrintf("verifyRingSignature(): EC_POINT_add failed.\n");
            rv = 1; goto End;
        };
        
        // ptT3 = Hp(Pi)
        if (hashToEC(&pPubkeys[i * ec_compressed_size], ec_compressed_size, bnT, ptT3) != 0)
        {
            LogPrintf("verifyRingSignature(): hashToEC failed.\n");
            rv = 1; goto End;
        };
        
        // ptT1 = k1 * I
        if (!EC_POINT_mul(ecGrp, ptT1, NULL, ptKi, bnC, bnCtx))
        {
            LogPrintf("verifyRingSignature(): EC_POINT_mul failed.\n");
            rv = 1; goto End;
        };
        
        // ptT2 = k2 * ptT3
        if (!EC_POINT_mul(ecGrp, ptT2, NULL, ptT3, bnR, bnCtx))
        {
            LogPrintf("verifyRingSignature(): EC_POINT_mul failed.\n");
            rv = 1; goto End;
        };
        
        // ptR = ptT1 + ptT2
        if (!EC_POINT_add(ecGrp, ptR, ptT1, ptT2, bnCtx))
        {
            LogPrintf("verifyRingSignature(): EC_POINT_add failed.\n");
            rv = 1; goto End;
        };
        
        // sum = (sum + ci) % N
        if (!BN_mod_add(bnSum, bnSum, bnC, bnN, bnCtx))
        {
            LogPrintf("verifyRingSignature(): BN_mod_add failed.\n");
            rv = 1; goto End;
        };
        
        // -- add ptL and ptR to hash
        if (!(EC_POINT_point2bn(ecGrp, ptL, POINT_CONVERSION_COMPRESSED, bnT, bnCtx))
            || BN_num_bytes(bnT) != (int) ec_compressed_size
            || BN_bn2bin(bnT, &tempData[0]) != (int) ec_compressed_size
            || !(EC_POINT_point2bn(ecGrp, ptR, POINT_CONVERSION_COMPRESSED, bnT, bnCtx))
            || BN_num_bytes(bnT) != (int) ec_compressed_size
            || BN_bn2bin(bnT, &tempData[33]) != (int) ec_compressed_size)
        {
            LogPrintf("extract ptL and ptR failed.\n");
            rv = 1; goto End;
        };
        
        ssCommitHash.write((const char*)&tempData[0], 66);
    };
    
    commitHash = ssCommitHash.GetHash();
    
    if (!(bnH) || !(bnH = BN_bin2bn(commitHash.begin(), ec_secret_size, bnH)))
    {
        LogPrintf("verifyRingSignature(): commitHash -> bnH failed\n");
        rv = 1; goto End;
    };
    
    if (!BN_mod(bnH, bnH, bnN, bnCtx))
    {
        LogPrintf("verifyRingSignature(): BN_mod failed.\n");
        rv = 1; goto End;
    };
    
    // bnT = (bnH - bnSum) % N
    if (!BN_mod_sub(bnT, bnH, bnSum, bnN, bnCtx))
    {
        LogPrintf("verifyRingSignature(): BN_mod_sub failed.\n");
        rv = 1; goto End;
    };
    
    // test bnT == 0  (bnSum == bnH)
    if (!BN_is_zero(bnT))
    {
        LogPrintf("verifyRingSignature(): signature does not verify.\n");
        rv = 2;
    };
    
    End:
    
    if (ptT1)
        EC_POINT_free(ptT1);
    if (ptT2)
        EC_POINT_free(ptT2);
    if (ptT3)
        EC_POINT_free(ptT3);
    if (ptPk)
        EC_POINT_free(ptPk);
    if (ptKi)
        EC_POINT_free(ptKi);
    if (ptL)
        EC_POINT_free(ptL);
    if (ptR)
        EC_POINT_free(ptR);
    
    BN_CTX_end(bnCtx);
    
    return rv;
};

