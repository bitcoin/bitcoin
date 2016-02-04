// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "stealth.h"
#include "base58.h"
#include "state.h"


#include <openssl/rand.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>


bool CStealthAddress::SetEncoded(const std::string& encodedAddress)
{
    data_chunk raw;
    
    if (!DecodeBase58(encodedAddress, raw))
    {
        if (fDebug)
            LogPrintf("CStealthAddress::SetEncoded DecodeBase58 falied.\n");
        return false;
    };
    
    if (!VerifyChecksum(raw))
    {
        if (fDebug)
            LogPrintf("CStealthAddress::SetEncoded verify_checksum falied.\n");
        return false;
    };
    
    if (raw.size() < 1 + 1 + 33 + 1 + 33 + 1 + 1 + 4)
    {
        if (fDebug)
            LogPrintf("CStealthAddress::SetEncoded() too few bytes provided.\n");
        return false;
    };
    
    
    uint8_t* p = &raw[0];
    uint8_t version = *p++;
    
    if (version != stealth_version_byte)
    {
        LogPrintf("CStealthAddress::SetEncoded version mismatch 0x%x != 0x%x.\n", version, stealth_version_byte);
        return false;
    };
    
    options = *p++;
    
    scan_pubkey.resize(33);
    memcpy(&scan_pubkey[0], p, 33);
    p += 33;
    //uint8_t spend_pubkeys = *p++;
    p++;
    
    spend_pubkey.resize(33);
    memcpy(&spend_pubkey[0], p, 33);
    
    return true;
};

std::string CStealthAddress::Encoded() const
{
    // https://wiki.unsystem.net/index.php/DarkWallet/Stealth#Address_format
    // [version] [options] [scan_key] [N] ... [Nsigs] [prefix_length] ...
    
    data_chunk raw;
    raw.push_back(stealth_version_byte);
    
    raw.push_back(options);
    
    raw.insert(raw.end(), scan_pubkey.begin(), scan_pubkey.end());
    raw.push_back(1); // number of spend pubkeys
    raw.insert(raw.end(), spend_pubkey.begin(), spend_pubkey.end());
    raw.push_back(0); // number of signatures
    raw.push_back(0); // ?
    
    AppendChecksum(raw);
    
    return EncodeBase58(raw);
};

int CStealthAddress::SetScanPubKey(CPubKey pk)
{
    scan_pubkey.resize(pk.size());
    memcpy(&scan_pubkey[0], pk.begin(), pk.size());
    return 0;
};


uint32_t BitcoinChecksum(uint8_t* p, uint32_t nBytes)
{
    if (!p || nBytes == 0)
        return 0;
    
    uint8_t hash1[32];
    SHA256(p, nBytes, (uint8_t*)hash1);
    uint8_t hash2[32];
    SHA256((uint8_t*)hash1, sizeof(hash1), (uint8_t*)hash2);
    
    // -- checksum is the 1st 4 bytes of the hash
    uint32_t checksum = from_little_endian<uint32_t>(&hash2[0]);
    
    return checksum;
};

void AppendChecksum(data_chunk& data)
{
    uint32_t checksum = BitcoinChecksum(&data[0], data.size());
    
    
    std::vector<uint8_t> tmp(4);
    //memcpy(&tmp[0], &checksum, 4);
    
    // -- to_little_endian
    for (int i = 0; i < 4; ++i)
    {
        tmp[i] = checksum & 0xFF;
        checksum >>= 8;
    };
    
    data.insert(data.end(), tmp.begin(), tmp.end());
};

bool VerifyChecksum(const data_chunk& data)
{
    if (data.size() < 4)
        return false;
    
    uint32_t checksum = from_little_endian<uint32_t>(data.end() - 4);
    
    return BitcoinChecksum((uint8_t*)&data[0], data.size()-4) == checksum;
};


int GenerateRandomSecret(ec_secret& out)
{
    RandAddSeedPerfmon();
    
    static uint256 max("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364140");
    static uint256 min(16000); // increase? min valid key is 1
    
    uint256 test;
    
    int i;
    // -- check max, try max 32 times
    for (i = 0; i < 32; ++i)
    {
        RAND_bytes((unsigned char*) test.begin(), 32);
        if (test > min && test < max)
        {
            memcpy(&out.e[0], test.begin(), 32);
            break;
        };
    };
    
    if (i > 31)
    {
        LogPrintf("Error: GenerateRandomSecret failed to generate a valid key.\n");
        return 1;
    };
    
    return 0;
};

int SecretToPublicKey(const ec_secret& secret, ec_point& out)
{
    // -- public key = private * G
    int rv = 0;
    
    EC_GROUP* ecgrp = EC_GROUP_new_by_curve_name(NID_secp256k1);
    
    if (!ecgrp)
    {
        LogPrintf("SecretToPublicKey(): EC_GROUP_new_by_curve_name failed.\n");
        return 1;
    };

    BIGNUM* bnIn = BN_bin2bn(&secret.e[0], ec_secret_size, BN_new());
    if (!bnIn)
    {
        EC_GROUP_free(ecgrp);
        LogPrintf("SecretToPublicKey(): BN_bin2bn failed\n");
        return 1;
    };
    
    EC_POINT* pub = EC_POINT_new(ecgrp);
    
    
    EC_POINT_mul(ecgrp, pub, bnIn, NULL, NULL, NULL);
    
    BIGNUM* bnOut = EC_POINT_point2bn(ecgrp, pub, POINT_CONVERSION_COMPRESSED, BN_new(), NULL);
    if (!bnOut)
    {
        LogPrintf("SecretToPublicKey(): point2bn failed\n");
        rv = 1;
    } else
    {
        out.resize(ec_compressed_size);
        if (BN_num_bytes(bnOut) != (int) ec_compressed_size
            || BN_bn2bin(bnOut, &out[0]) != (int) ec_compressed_size)
        {
            LogPrintf("SecretToPublicKey(): bnOut incorrect length.\n");
            rv = 1;
        };
        
        BN_free(bnOut);
    };
    
    
    EC_POINT_free(pub);
    BN_free(bnIn);
    EC_GROUP_free(ecgrp);
    
    return rv;
};


int StealthSecret(ec_secret& secret, ec_point& pubkey, const ec_point& pkSpend, ec_secret& sharedSOut, ec_point& pkOut)
{
    /*
    
    send:
        secret = ephem_secret, pubkey = scan_pubkey
    
    receive:
        secret = scan_secret, pubkey = ephem_pubkey
        c = H(dP)
    
    Q = public scan key (EC point, 33 bytes)
    d = private scan key (integer, 32 bytes)
    R = public spend key
    f = private spend key

    Q = dG
    R = fG
    
    Sender (has Q and R, not d or f):
    
    P = eG

    c = H(eQ) = H(dP)
    R' = R + cG
    
    
    Recipient gets R' and P
    
    test 0 and infinity?
    */
    
    int rv = 0;
    std::vector<uint8_t> vchOutQ;
    
    BN_CTX* bnCtx   = NULL;
    BIGNUM* bnEphem = NULL;
    BIGNUM* bnQ     = NULL;
    EC_POINT* Q     = NULL;
    BIGNUM* bnOutQ  = NULL;
    BIGNUM* bnc     = NULL;
    EC_POINT* C     = NULL;
    BIGNUM* bnR     = NULL;
    EC_POINT* R     = NULL;
    EC_POINT* Rout  = NULL;
    BIGNUM* bnOutR  = NULL;
    
    EC_GROUP* ecgrp = EC_GROUP_new_by_curve_name(NID_secp256k1);
    
    if (!ecgrp)
    {
        LogPrintf("StealthSecret(): EC_GROUP_new_by_curve_name failed.\n");
        return 1;
    };
    
    if (!(bnCtx = BN_CTX_new()))
    {
        LogPrintf("StealthSecret(): BN_CTX_new failed.\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnEphem = BN_bin2bn(&secret.e[0], ec_secret_size, BN_new())))
    {
        LogPrintf("StealthSecret(): bnEphem BN_bin2bn failed.\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnQ = BN_bin2bn(&pubkey[0], pubkey.size(), BN_new())))
    {
        LogPrintf("StealthSecret(): bnQ BN_bin2bn failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(Q = EC_POINT_bn2point(ecgrp, bnQ, NULL, bnCtx)))
    {
        LogPrintf("StealthSecret(): Q EC_POINT_bn2point failed\n");
        rv = 1;
        goto End;
    };
    
    // -- eQ
    // EC_POINT_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n, const EC_POINT *q, const BIGNUM *m, BN_CTX *ctx);
    // EC_POINT_mul calculates the value generator * n + q * m and stores the result in r. The value n may be NULL in which case the result is just q * m. 
    if (!EC_POINT_mul(ecgrp, Q, NULL, Q, bnEphem, bnCtx))
    {
        LogPrintf("StealthSecret(): eQ EC_POINT_mul failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnOutQ = EC_POINT_point2bn(ecgrp, Q, POINT_CONVERSION_COMPRESSED, BN_new(), bnCtx)))
    {
        LogPrintf("StealthSecret(): Q EC_POINT_bn2point failed\n");
        rv = 1;
        goto End;
    };
    
    
    vchOutQ.resize(ec_compressed_size);
    if (BN_num_bytes(bnOutQ) != (int) ec_compressed_size
        || BN_bn2bin(bnOutQ, &vchOutQ[0]) != (int) ec_compressed_size)
    {
        LogPrintf("StealthSecret(): bnOutQ incorrect length.\n");
        rv = 1;
        goto End;
    };
    
    SHA256(&vchOutQ[0], vchOutQ.size(), &sharedSOut.e[0]);
    
    if (!(bnc = BN_bin2bn(&sharedSOut.e[0], ec_secret_size, BN_new())))
    {
        LogPrintf("StealthSecret(): BN_bin2bn failed\n");
        rv = 1;
        goto End;
    };
    
    // -- cG
    if (!(C = EC_POINT_new(ecgrp)))
    {
        LogPrintf("StealthSecret(): C EC_POINT_new failed\n");
        rv = 1;
        goto End;
    };
    
    if (!EC_POINT_mul(ecgrp, C, bnc, NULL, NULL, bnCtx))
    {
        LogPrintf("StealthSecret(): C EC_POINT_mul failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnR = BN_bin2bn(&pkSpend[0], pkSpend.size(), BN_new())))
    {
        LogPrintf("StealthSecret(): bnR BN_bin2bn failed\n");
        rv = 1;
        goto End;
    };
    
    
    if (!(R = EC_POINT_bn2point(ecgrp, bnR, NULL, bnCtx)))
    {
        LogPrintf("StealthSecret(): R EC_POINT_bn2point failed\n");
        rv = 1;
        goto End;
    };
    
    if (!EC_POINT_mul(ecgrp, C, bnc, NULL, NULL, bnCtx))
    {
        LogPrintf("StealthSecret(): C EC_POINT_mul failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(Rout = EC_POINT_new(ecgrp)))
    {
        LogPrintf("StealthSecret(): Rout EC_POINT_new failed\n");
        rv = 1;
        goto End;
    };
    
    if (!EC_POINT_add(ecgrp, Rout, R, C, bnCtx))
    {
        LogPrintf("StealthSecret(): Rout EC_POINT_add failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnOutR = EC_POINT_point2bn(ecgrp, Rout, POINT_CONVERSION_COMPRESSED, BN_new(), bnCtx)))
    {
        LogPrintf("StealthSecret(): Rout EC_POINT_bn2point failed\n");
        rv = 1;
        goto End;
    };
    
    
    pkOut.resize(ec_compressed_size);
    if (BN_num_bytes(bnOutR) != (int) ec_compressed_size
        || BN_bn2bin(bnOutR, &pkOut[0]) != (int) ec_compressed_size)
    {
        LogPrintf("StealthSecret(): pkOut incorrect length.\n");
        rv = 1;
        goto End;
    };
    
    End:
    if (bnOutR)     BN_free(bnOutR);
    if (Rout)       EC_POINT_free(Rout);
    if (R)          EC_POINT_free(R);
    if (bnR)        BN_free(bnR);
    if (C)          EC_POINT_free(C);
    if (bnc)        BN_free(bnc);
    if (bnOutQ)     BN_free(bnOutQ);
    if (Q)          EC_POINT_free(Q);
    if (bnQ)        BN_free(bnQ);
    if (bnEphem)    BN_free(bnEphem);
    if (bnCtx)      BN_CTX_free(bnCtx);
    EC_GROUP_free(ecgrp);
    
    return rv;
};


int StealthSecretSpend(ec_secret& scanSecret, ec_point& ephemPubkey, ec_secret& spendSecret, ec_secret& secretOut)
{
    /*
    
    c  = H(dP)
    R' = R + cG     [without decrypting wallet]
       = (f + c)G   [after decryption of wallet]
         Remember: mod curve.order, pad with 0x00s where necessary?
    */
    
    int rv = 0;
    std::vector<uint8_t> vchOutP;
    
    BN_CTX* bnCtx           = NULL;
    BIGNUM* bnScanSecret    = NULL;
    BIGNUM* bnP             = NULL;
    EC_POINT* P             = NULL;
    BIGNUM* bnOutP          = NULL;
    BIGNUM* bnc             = NULL;
    BIGNUM* bnOrder         = NULL;
    BIGNUM* bnSpend         = NULL;
    
    EC_GROUP* ecgrp = EC_GROUP_new_by_curve_name(NID_secp256k1);
    
    if (!ecgrp)
    {
        LogPrintf("StealthSecretSpend(): EC_GROUP_new_by_curve_name failed.\n");
        return 1;
    };
    
    if (!(bnCtx = BN_CTX_new()))
    {
        LogPrintf("StealthSecretSpend(): BN_CTX_new failed.\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnScanSecret = BN_bin2bn(&scanSecret.e[0], ec_secret_size, BN_new())))
    {
        LogPrintf("StealthSecretSpend(): bnScanSecret BN_bin2bn failed.\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnP = BN_bin2bn(&ephemPubkey[0], ephemPubkey.size(), BN_new())))
    {
        LogPrintf("StealthSecretSpend(): bnP BN_bin2bn failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(P = EC_POINT_bn2point(ecgrp, bnP, NULL, bnCtx)))
    {
        LogPrintf("StealthSecretSpend(): P EC_POINT_bn2point failed\n");
        rv = 1;
        goto End;
    };
    
    // -- dP
    if (!EC_POINT_mul(ecgrp, P, NULL, P, bnScanSecret, bnCtx))
    {
        LogPrintf("StealthSecretSpend(): dP EC_POINT_mul failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnOutP = EC_POINT_point2bn(ecgrp, P, POINT_CONVERSION_COMPRESSED, BN_new(), bnCtx)))
    {
        LogPrintf("StealthSecretSpend(): P EC_POINT_bn2point failed\n");
        rv = 1;
        goto End;
    };
    
    
    vchOutP.resize(ec_compressed_size);
    if (BN_num_bytes(bnOutP) != (int) ec_compressed_size
        || BN_bn2bin(bnOutP, &vchOutP[0]) != (int) ec_compressed_size)
    {
        LogPrintf("StealthSecretSpend(): bnOutP incorrect length.\n");
        rv = 1;
        goto End;
    };
    
    uint8_t hash1[32];
    SHA256(&vchOutP[0], vchOutP.size(), (uint8_t*)hash1);
    
    
    if (!(bnc = BN_bin2bn(&hash1[0], 32, BN_new())))
    {
        LogPrintf("StealthSecretSpend(): BN_bin2bn failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnOrder = BN_new())
        || !EC_GROUP_get_order(ecgrp, bnOrder, bnCtx))
    {
        LogPrintf("StealthSecretSpend(): EC_GROUP_get_order failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnSpend = BN_bin2bn(&spendSecret.e[0], ec_secret_size, BN_new())))
    {
        LogPrintf("StealthSecretSpend(): bnSpend BN_bin2bn failed.\n");
        rv = 1;
        goto End;
    };
    
    //if (!BN_add(r, a, b)) return 0;
    //return BN_nnmod(r, r, m, ctx);
    if (!BN_mod_add(bnSpend, bnSpend, bnc, bnOrder, bnCtx))
    {
        LogPrintf("StealthSecretSpend(): bnSpend BN_mod_add failed.\n");
        rv = 1;
        goto End;
    };
    
    if (BN_is_zero(bnSpend)) // possible?
    {
        LogPrintf("StealthSecretSpend(): bnSpend is zero.\n");
        rv = 1;
        goto End;
    };
    
    int nBytes;
    memset(&secretOut.e[0], 0, ec_secret_size);
    if ((nBytes = BN_num_bytes(bnSpend)) > (int)ec_secret_size
        || BN_bn2bin(bnSpend, &secretOut.e[ec_secret_size-nBytes]) != nBytes)
    {
        LogPrintf("StealthSecretSpend(): bnSpend incorrect length.\n");
        rv = 1;
        goto End;
    };
    
    End:
    if (bnSpend)        BN_free(bnSpend);
    if (bnOrder)        BN_free(bnOrder);
    if (bnc)            BN_free(bnc);
    if (bnOutP)         BN_free(bnOutP);
    if (P)              EC_POINT_free(P);
    if (bnP)            BN_free(bnP);
    if (bnScanSecret)   BN_free(bnScanSecret);
    if (bnCtx)          BN_CTX_free(bnCtx);
    EC_GROUP_free(ecgrp);
    
    return rv;
};


int StealthSharedToSecretSpend(ec_secret& sharedS, ec_secret& spendSecret, ec_secret& secretOut)
{
    int rv = 0;
    std::vector<uint8_t> vchOutP;
    
    BN_CTX* bnCtx           = NULL;
    BIGNUM* bnc             = NULL;
    BIGNUM* bnOrder         = NULL;
    BIGNUM* bnSpend         = NULL;
    
    EC_GROUP* ecgrp = EC_GROUP_new_by_curve_name(NID_secp256k1);
    
    if (!ecgrp)
    {
        LogPrintf("StealthSecretSpend(): EC_GROUP_new_by_curve_name failed.\n");
        return 1;
    };
    
    if (!(bnCtx = BN_CTX_new()))
    {
        LogPrintf("StealthSecretSpend(): BN_CTX_new failed.\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnc = BN_bin2bn(&sharedS.e[0], ec_secret_size, BN_new())))
    {
        LogPrintf("StealthSecretSpend(): BN_bin2bn failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnOrder = BN_new())
        || !EC_GROUP_get_order(ecgrp, bnOrder, bnCtx))
    {
        LogPrintf("StealthSecretSpend(): EC_GROUP_get_order failed\n");
        rv = 1;
        goto End;
    };
    
    if (!(bnSpend = BN_bin2bn(&spendSecret.e[0], ec_secret_size, BN_new())))
    {
        LogPrintf("StealthSecretSpend(): bnSpend BN_bin2bn failed.\n");
        rv = 1;
        goto End;
    };
    
    //if (!BN_add(r, a, b)) return 0;
    //return BN_nnmod(r, r, m, ctx);
    if (!BN_mod_add(bnSpend, bnSpend, bnc, bnOrder, bnCtx))
    {
        LogPrintf("StealthSecretSpend(): bnSpend BN_mod_add failed.\n");
        rv = 1;
        goto End;
    };
    
    if (BN_is_zero(bnSpend)) // possible?
    {
        LogPrintf("StealthSecretSpend(): bnSpend is zero.\n");
        rv = 1;
        goto End;
    };
    
    int nBytes;
    memset(&secretOut.e[0], 0, ec_secret_size);
    if ((nBytes = BN_num_bytes(bnSpend)) > (int)ec_secret_size
        || BN_bn2bin(bnSpend, &secretOut.e[ec_secret_size-nBytes]) != nBytes)
    {
        LogPrintf("StealthSecretSpend(): bnSpend incorrect length.\n");
        rv = 1;
        goto End;
    };
    
    End:
    if (bnSpend)        BN_free(bnSpend);
    if (bnOrder)        BN_free(bnOrder);
    if (bnc)            BN_free(bnc);
    if (bnCtx)          BN_CTX_free(bnCtx);
    EC_GROUP_free(ecgrp);
    
    return rv;
};

bool IsStealthAddress(const std::string& encodedAddress)
{
    data_chunk raw;
    
    if (!DecodeBase58(encodedAddress, raw))
    {
        //LogPrintf("IsStealthAddress DecodeBase58 falied.\n");
        return false;
    };
    
    if (!VerifyChecksum(raw))
    {
        //LogPrintf("IsStealthAddress verify_checksum falied.\n");
        return false;
    };
    
    if (raw.size() < 1 + 1 + 33 + 1 + 33 + 1 + 1 + 4)
    {
        //LogPrintf("IsStealthAddress too few bytes provided.\n");
        return false;
    };
    
    
    uint8_t* p = &raw[0];
    uint8_t version = *p++;
    
    if (version != stealth_version_byte)
    {
        //LogPrintf("IsStealthAddress version mismatch 0x%x != 0x%x.\n", version, stealth_version_byte);
        return false;
    };
    
    return true;
};
