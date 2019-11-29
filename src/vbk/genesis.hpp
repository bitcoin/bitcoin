#ifndef BITCOIN_SRC_VBK_GENESIS_HPP
#define BITCOIN_SRC_VBK_GENESIS_HPP

#include "vbk/merkle.hpp"
#include <chainparams.h>
#include <pow.h>
#include <primitives/block.h>
#include <streams.h>
#include <util/strencodings.h>

namespace VeriBlock {

inline CScript ScriptWithPrefix(uint32_t nBits)
{
    CScript script;
    if (nBits <= 0xff)
        script << nBits << CScriptNum(1);
    else if (nBits <= 0xffff)
        script << nBits << CScriptNum(2);
    else if (nBits <= 0xffffff)
        script << nBits << CScriptNum(3);
    else
        script << nBits << CScriptNum(4);

    return script;
}

inline CBlock CreateGenesisBlock(
    std::string pszTimestamp,
    const CScript& genesisOutputScript,
    uint32_t nTime,
    uint32_t nNonce,
    uint32_t nBits,
    int32_t nVersion,
    const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = VeriBlock::ScriptWithPrefix(nBits) << std::vector<uint8_t>{pszTimestamp.begin(), pszTimestamp.end()};
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = VeriBlock::TopLevelMerkleRoot(nullptr, genesis);
    return genesis;
}

inline CBlock CreateGenesisBlock(
    uint32_t nTime,
    uint32_t nNonce,
    uint32_t nBits,
    int32_t nVersion,
    const CAmount& genesisReward,
    const std::string& initialPubkeyHex,
    const std::string& pszTimestamp)
{
    const CScript genesisOutputScript = CScript() << ParseHex(initialPubkeyHex) << OP_CHECKSIG;
    return VeriBlock::CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}


inline CBlock MineGenesisBlock(
    uint32_t nTime,
    const std::string& pszTimestamp,
    const std::string& initialPubkeyHex,
    uint32_t nBits,
    uint32_t nVersion = 1,
    uint32_t nNonce = 0, // starting nonce
    uint64_t genesisReward = 50 * COIN)
{
    CBlock genesis = CreateGenesisBlock(nTime, nNonce, nBits, nVersion, genesisReward, initialPubkeyHex, pszTimestamp);

    printf("started genesis block mining...\n");
    while (!CheckProofOfWork(genesis.GetHash(), genesis.nBits, Params().GetConsensus())) {
        ++genesis.nNonce;
        if (genesis.nNonce > 4294967294LL) {
            ++genesis.nTime;
            genesis.nNonce = 0;
        }
    }

    assert(CheckProofOfWork(genesis.GetHash(), genesis.nBits, Params().GetConsensus()));

    return genesis;
}

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_GENESIS_HPP
