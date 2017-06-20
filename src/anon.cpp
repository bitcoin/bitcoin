// Copyright (c) 2017 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "anon.h"

#include <assert.h>
#include <secp256k1.h>
#include <secp256k1_rangeproof.h>
#include <secp256k1_mlsag.h>

#include "blind.h"
#include "rctindex.h"
#include "txdb.h"
#include "txmempool.h"
#include "util.h"
#include "validation.h"


bool VerifyMLSAG(const CTransaction &tx, unsigned int i, CValidationState &state)
{
    
    if (i >= tx.vin.size())
        return state.DoS(100, false, REJECT_MALFORMED, "bad-input");
    const auto &txin = tx.vin[i];
    
    if (!txin.IsAnonInput())
        return state.DoS(100, false, REJECT_MALFORMED, "bad-anon-input");
    
    size_t nStandard = 0, nCt = 0, nRingCT = 0;
    CAmount nPlainValueOut = tx.GetPlainValueOut(nStandard, nCt, nRingCT);
    CAmount nTxFee = 0;
    if (!tx.GetCTFee(nTxFee))
        return state.DoS(100, error("%s: bad-fee-output", __func__), REJECT_INVALID, "bad-fee-output");
    
    nPlainValueOut += nTxFee;
    
    uint32_t nInputs, nRingSize;
    txin.GetAnonInfo(nInputs, nRingSize);
    
    if (nInputs < 1 || nInputs > MAX_ANON_INPUTS) // TODO: Select max inputs size
        return state.DoS(100, false, REJECT_INVALID, "bad-anon-num-inputs");
    
    if (nRingSize < MIN_RINGSIZE || nRingSize > MAX_RINGSIZE)
        return state.DoS(100, false, REJECT_INVALID, "bad-anon-ringsize");
    
    
    uint256 txhash = tx.GetHash();
    
    size_t nCols = nRingSize;
    size_t nRows = nInputs + 1;
    
    if (txin.scriptData.stack.size() != 1)
        return state.DoS(100, false, REJECT_MALFORMED, "bad-anonin-dstack-size");
    if (txin.scriptWitness.stack.size() != 2)
        return state.DoS(100, false, REJECT_MALFORMED, "bad-anonin-wstack-size");
    
    
    const std::vector<uint8_t> &vKeyImages = txin.scriptData.stack[0];
    const std::vector<uint8_t> &vMI = txin.scriptWitness.stack[0];
    const std::vector<uint8_t> &vDL = txin.scriptWitness.stack[1];
    
    if (vKeyImages.size() != nInputs * 33)
        return state.DoS(100, false, REJECT_MALFORMED, "bad-anonin-keyimages-size");
    
    if (vDL.size() != (1 + (nInputs+1) * nRingSize) * 32)
        return state.DoS(100, false, REJECT_MALFORMED, "bad-anonin-sig-size");
    
    std::set<int64_t> setHaveI;
    std::vector<uint8_t> vM(nCols * nRows * 33);
    
    std::vector<secp256k1_pedersen_commitment> vCommitments;
    vCommitments.reserve(nCols * nInputs);
    std::vector<const uint8_t*> vpOutCommits;
    std::vector<const uint8_t*> vpInCommits(nCols * nInputs);
    
    
    
    uint8_t zeroBlind[32];
    memset(zeroBlind, 0, 32);
    secp256k1_pedersen_commitment plainCommitment;
    
    if (nPlainValueOut > 0)
    {
        if (!secp256k1_pedersen_commit(secp256k1_ctx_blind, 
            &plainCommitment, zeroBlind, (uint64_t) nPlainValueOut, secp256k1_generator_h))
            return state.DoS(100, false, REJECT_INVALID, "bad-plain-commitment");
        
        vpOutCommits.push_back(plainCommitment.data);
    };
    
    secp256k1_pedersen_commitment *pc;
    for (auto &txout : tx.vpout)
    {
        if ((pc = txout->GetPCommitment()))
            vpOutCommits.push_back(pc->data);
    };
    
    
    size_t ofs = 0, nB = 0;
    for (size_t k = 0; k < nInputs; ++k)
    for (size_t i = 0; i < nCols; ++i)
    {
        int64_t nIndex;
        
        if (0 != GetVarInt(vMI, ofs, (uint64_t&)nIndex, nB))
            return state.DoS(100, false, REJECT_MALFORMED, "bad-anonin-extract-i");
        ofs += nB;
        
        if (!setHaveI.insert(nIndex).second)
            return state.DoS(100, false, REJECT_MALFORMED, "bad-anonin-dup-i");
        
        CAnonOutput ao;
        if (!pblocktree->ReadRCTOutput(nIndex, ao))
            return state.DoS(100, false, REJECT_MALFORMED, "bad-anonin-unknown-i");
        
        memcpy(&vM[(i+k*nCols)*33], ao.pubkey.begin(), 33);
        vCommitments.push_back(ao.commitment);
        vpInCommits[i+k*nCols] = vCommitments.back().data;
    };
    
    uint256 txhashKI;
    for (size_t k = 0; k < nInputs; ++k)
    {
        const CCmpPubKey &ki = *((CCmpPubKey*)&vKeyImages[k*33]);
        
        
        if (mempool.HaveKeyImage(ki, txhashKI))
        {
            if (fDebug)
                LogPrint("rct", "%s: Duplicate keyimage detected in mempool %s, used in %s.\n", __func__,
                    HexStr(ki.begin(), ki.end()), txhashKI.ToString());
            return state.DoS(100, false, REJECT_INVALID, "bad-anonin-dup-ki");
        };
        
        if (pblocktree->ReadRCTKeyImage(ki, txhashKI))
        {
            if (fDebug)
                LogPrint("rct", "%s: Duplicate keyimage detected %s, used in %s.\n", __func__,
                    HexStr(ki.begin(), ki.end()), txhashKI.ToString());
            return state.DoS(100, false, REJECT_INVALID, "bad-anonin-dup-ki");
        };
    };
    
    int rv;
    if (0 != (rv = prepareLastRowMLSAG(vpOutCommits.size(), vpOutCommits.size(), nCols, nRows,
        &vpInCommits[0], &vpOutCommits[0], NULL, &vM[0], NULL)))
        return state.DoS(100, error("%s: prepare-mlsag-failed %d", __func__, rv), REJECT_INVALID, "prepare-mlsag-failed");
    
    if (0 != (rv = verifyMLSAG(secp256k1_ctx_blind,
        txhash.begin(), nCols, nRows, 
        &vM[0], &vKeyImages[0], &vDL[0], &vDL[32])))
        return state.DoS(100, error("%s: verify-mlsag-failed %d", __func__, rv), REJECT_INVALID, "verify-mlsag-failed");
    
    return true;
};


bool AddKeyImagesToMempool(const CTransaction &tx, CTxMemPool &pool)
{
    for (const CTxIn &txin : tx.vin)
    {
        if (!txin.IsAnonInput())
            continue;
        uint256 txhash = tx.GetHash();
        LOCK(pool.cs);
        uint32_t nInputs, nRingSize;
        txin.GetAnonInfo(nInputs, nRingSize);
        
        const std::vector<uint8_t> &vKeyImages = txin.scriptData.stack[0];
        
        if (vKeyImages.size() != nInputs * 33)
            return false;
        
        for (size_t k = 0; k < nInputs; ++k)
        {
            const CCmpPubKey &ki = *((CCmpPubKey*)&vKeyImages[k*33]);
            pool.mapKeyImages[ki] = txhash;
        };
    };
    
    return true;
};

bool RemoveKeyImagesFromMempool(const uint256 &hash, const CTxIn &txin, CTxMemPool &pool)
{
    if (!txin.IsAnonInput())
        return false;
    LOCK(pool.cs);
    uint32_t nInputs, nRingSize;
    txin.GetAnonInfo(nInputs, nRingSize);
    
    const std::vector<uint8_t> &vKeyImages = txin.scriptData.stack[0];
    
    if (vKeyImages.size() != nInputs * 33)
        return false;
    
    for (size_t k = 0; k < nInputs; ++k)
    {
        const CCmpPubKey &ki = *((CCmpPubKey*)&vKeyImages[k*33]);
        pool.mapKeyImages.erase(ki);
    };
    
    return true;
};

bool RemoveAnonDBRecords()
{
    LogPrint("rct", "%s", __func__);
    
    boost::scoped_ptr<CDBIterator> pcursor(pblocktree->NewIterator());

    CDBBatch batch(*pblocktree);
    CCmpPubKey kiFirst;
    int64_t nZero = 0;
    memset(kiFirst.ncbegin(), 0, 33);
    
    pcursor->Seek(std::make_pair(DB_RCTOUTPUT, nZero));
    while (pcursor->Valid())
    {
        std::pair<char,int64_t> key;
        if (!pcursor->GetKey(key) || key.first != DB_RCTOUTPUT)
            break;
        batch.Erase(key);
        pcursor->Next();
    };
    
    if (!pblocktree->WriteBatch(batch))
        LogPrintf("Error: %s - WriteBatch failed.\n");
    
    
    pcursor->Seek(std::make_pair(DB_RCTOUTPUT_LINK, kiFirst));
    while (pcursor->Valid())
    {
        std::pair<char,CCmpPubKey> key;
        if (!pcursor->GetKey(key) || key.first != DB_RCTOUTPUT_LINK)
            break;
        batch.Erase(key);
        pcursor->Next();
    };
    
    if (!pblocktree->WriteBatch(batch))
        LogPrintf("Error: %s - WriteBatch failed.\n");
    
    
    pcursor->Seek(std::make_pair(DB_RCTKEYIMAGE, kiFirst));
    while (pcursor->Valid())
    {
        std::pair<char,CCmpPubKey> key;
        if (!pcursor->GetKey(key) || key.first != DB_RCTKEYIMAGE)
            break;
        batch.Erase(key);
        pcursor->Next();
    };
    
    if (!pblocktree->WriteBatch(batch))
        LogPrintf("Error: %s - WriteBatch failed.\n");
    
    return true;
};
