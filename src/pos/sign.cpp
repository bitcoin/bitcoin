// Copyright (c) 2012-2019 The Peercoin developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos/sign.h>
#include <chainparams.h>
#include <key.h>
#include <pubkey.h>
#include <primitives/block.h>
#include <script/standard.h>
#include <wallet/wallet.h>

typedef std::vector<unsigned char> valtype;

bool CBlockSigner::SignBlock()
{
    std::vector<valtype> vSolutions;
    txnouttype whichType;

    // We dont need to sign PoW blocks.
    if (block.IsProofOfWork())
        return false;

    const CTxOut& txout = block.vtx[1]->vout[1];

    whichType = Solver(txout.scriptPubKey, vSolutions);

    CKeyID keyID;

    if (whichType == TX_PUBKEYHASH)
        keyID = CKeyID(uint160(vSolutions[0]));
    else if (whichType == TX_PUBKEY)
        keyID = CPubKey(vSolutions[0]).GetID();

    CKey key;
    if (!wallet->GetKey(keyID, key)) return false;
    if (!key.Sign(block.GetHash(), block.vchBlockSig)) return false;
    return true;
}

bool CBlockSigner::CheckBlockSignature()
{
    if (block.IsProofOfWork() ||
        block.GetHash() == Params().GetConsensus().hashGenesisBlock)
        return block.vchBlockSig.empty();

    std::vector<valtype> vSolutions;
    txnouttype whichType;
    const CTxOut& txout = block.vtx[1]->vout[1];

    whichType = Solver(txout.scriptPubKey, vSolutions);
    valtype& vchPubKey = vSolutions[0];
    if (whichType == TX_PUBKEY)
    {
        CPubKey key(vchPubKey);
        if (block.vchBlockSig.empty()) return false;
        return key.Verify(block.GetHash(), block.vchBlockSig);
    }
    else if (whichType == TX_PUBKEYHASH)
    {
        CKeyID keyID;
        keyID = CKeyID(uint160(vchPubKey));
        CPubKey pubkey(vchPubKey);

        if (!pubkey.IsValid()) return false;
        if (block.vchBlockSig.empty()) return false;
        return pubkey.Verify(block.GetHash(), block.vchBlockSig);
    }

    return false;
}