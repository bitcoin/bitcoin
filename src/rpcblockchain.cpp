// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2013 Primecoin developers
// Distributed under conditional MIT/X11 software license,
// see the accompanying file COPYING

#include "main.h"
#include "bitcoinrpc.h"
#include "prime.h"
#include "wallet.h"
#include "init.h"

using namespace json_spirit;
using namespace std;

void ScriptPubKeyToJSON(const CScript& scriptPubKey, Object& out);

// Primecoin: get prime difficulty value (chain length)
double GetDifficulty(const CBlockIndex* blockindex)
{
    // Floating point number that is approximate log scale of prime target,
    // minimum difficulty = 256, maximum difficulty = 2039
    if (blockindex == NULL)
    {
        if (pindexBest == NULL)
            return 256.0;
        else
            blockindex = pindexBest;
    }

    double dDiff = GetPrimeDifficulty(blockindex->nBits);
    return dDiff;
}

Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    CMerkleTx txGen(block.vtx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("headerhash", block.GetHeaderHash().GetHex()));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    Array txs;
    BOOST_FOREACH(const CTransaction&tx, block.vtx)
        txs.push_back(tx.GetHash().GetHex());
    result.push_back(Pair("tx", txs));
    result.push_back(Pair("time", (boost::int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (boost::uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", GetPrimeDifficulty(block.nBits)));
    result.push_back(Pair("transition", GetPrimeDifficulty(blockindex->nWorkTransition)));
    CBigNum bnPrimeChainOrigin = CBigNum(block.GetHeaderHash()) * block.bnPrimeChainMultiplier;
    result.push_back(Pair("primechain", GetPrimeChainName(blockindex->nPrimeChainType, blockindex->nPrimeChainLength).c_str()));
    result.push_back(Pair("primeorigin", bnPrimeChainOrigin.ToString().c_str()));

    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));
    return result;
}


Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "Returns the number of blocks in the longest block chain.");

    return nBestHeight;
}


Value getdifficulty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getdifficulty\n"
            "Returns the proof-of-work difficulty in prime chain length.");

    return GetDifficulty();
}


Value settxfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1 || AmountFromValue(params[0]) < MIN_TX_FEE)
        throw runtime_error(
            "settxfee <amount>\n"
            "<amount> is a real and is rounded to 0.01 (cent)\n"
            "Minimum and default transaction fee per KB is 1 cent");

    nTransactionFee = (AmountFromValue(params[0]) / CENT) * CENT; // round to cent
    return true;
}

Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getrawmempool\n"
            "Returns all transaction ids in memory pool.");

    vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    Array a;
    BOOST_FOREACH(const uint256& hash, vtxid)
        a.push_back(hash.ToString());

    return a;
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash <index>\n"
            "Returns hash of block in best-block-chain at <index>.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = FindBlockByHeight(nHeight);
    return pblockindex->phashBlock->GetHex();
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblock <hash>\n"
            "Returns details of a block with given block-hash.");

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex);

    return blockToJSON(block, pblockindex);
}

Value gettxoutsetinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "gettxoutsetinfo\n"
            "Returns statistics about the unspent transaction output set.");

    Object ret;

    CCoinsStats stats;
    if (pcoinsTip->GetStats(stats)) {
        ret.push_back(Pair("height", (boost::int64_t)stats.nHeight));
        ret.push_back(Pair("bestblock", stats.hashBlock.GetHex()));
        ret.push_back(Pair("transactions", (boost::int64_t)stats.nTransactions));
        ret.push_back(Pair("txouts", (boost::int64_t)stats.nTransactionOutputs));
        ret.push_back(Pair("bytes_serialized", (boost::int64_t)stats.nSerializedSize));
        ret.push_back(Pair("hash_serialized", stats.hashSerialized.GetHex()));
        ret.push_back(Pair("total_amount", ValueFromAmount(stats.nTotalAmount)));
    }
    return ret;
}

Value gettxout(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "gettxout <txid> <n> [includemempool=true]\n"
            "Returns details about an unspent transaction output.");

    Object ret;

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);
    int n = params[1].get_int();
    bool fMempool = true;
    if (params.size() > 2)
        fMempool = params[2].get_bool();

    CCoins coins;
    if (fMempool) {
        LOCK(mempool.cs);
        CCoinsViewMemPool view(*pcoinsTip, mempool);
        if (!view.GetCoins(hash, coins))
            return Value::null;
        mempool.pruneSpent(hash, coins); // TODO: this should be done by the CCoinsViewMemPool
    } else {
        if (!pcoinsTip->GetCoins(hash, coins))
            return Value::null;
    }
    if (n<0 || (unsigned int)n>=coins.vout.size() || coins.vout[n].IsNull())
        return Value::null;

    ret.push_back(Pair("bestblock", pcoinsTip->GetBestBlock()->GetBlockHash().GetHex()));
    if ((unsigned int)coins.nHeight == MEMPOOL_HEIGHT)
        ret.push_back(Pair("confirmations", 0));
    else
        ret.push_back(Pair("confirmations", pcoinsTip->GetBestBlock()->nHeight - coins.nHeight + 1));
    ret.push_back(Pair("value", ValueFromAmount(coins.vout[n].nValue)));
    Object o;
    ScriptPubKeyToJSON(coins.vout[n].scriptPubKey, o);
    ret.push_back(Pair("scriptPubKey", o));
    ret.push_back(Pair("version", coins.nVersion));
    ret.push_back(Pair("coinbase", coins.fCoinBase));

    return ret;
}

// Primecoin: list prime chain records within primecoin network
Value listprimerecords(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "listprimerecords <primechain length> [primechain type]\n"
            "Returns the list of record prime chains in primecoin network.\n"
            "<primechain length> is integer like 10, 11, 12 etc.\n"
            "[primechain type] is optional type, among 1CC, 2CC and TWN");

    int nPrimeChainLength = params[0].get_int();
    unsigned int nPrimeChainType = 0;
    if (params.size() > 1)
    {
        std::string strPrimeChainType = params[1].get_str();
        if (strPrimeChainType.compare("1CC") == 0)
            nPrimeChainType = PRIME_CHAIN_CUNNINGHAM1;
        else if (strPrimeChainType.compare("2CC") == 0)
            nPrimeChainType = PRIME_CHAIN_CUNNINGHAM2;
        else if (strPrimeChainType.compare("TWN") == 0)
            nPrimeChainType = PRIME_CHAIN_BI_TWIN;
        else
            throw runtime_error("Prime chain type must be 1CC, 2CC or TWN.");
    }

    Array ret;

    CBigNum bnPrimeRecord = 0;

    for (CBlockIndex* pindex = pindexGenesisBlock; pindex; pindex = pindex->pnext)
    {
        if (nPrimeChainLength != (int) TargetGetLength(pindex->nPrimeChainLength))
            continue; // length not matching, next block
        if (nPrimeChainType && nPrimeChainType != pindex->nPrimeChainType)
            continue; // type not matching, next block

        CBlock block;
        block.ReadFromDisk(pindex); // read block
        CBigNum bnPrimeChainOrigin = CBigNum(block.GetHeaderHash()) * block.bnPrimeChainMultiplier; // compute prime chain origin

        if (bnPrimeChainOrigin > bnPrimeRecord)
        {
            bnPrimeRecord = bnPrimeChainOrigin; // new record in primecoin
            Object entry;
            entry.push_back(Pair("time", DateTimeStrFormat("%Y-%m-%d %H:%M:%S UTC", pindex->GetBlockTime()).c_str()));
            entry.push_back(Pair("epoch", (boost::int64_t) pindex->GetBlockTime()));
            entry.push_back(Pair("height", pindex->nHeight));
            entry.push_back(Pair("ismine", pwalletMain->IsMine(block.vtx[0])));
            CTxDestination address;
            entry.push_back(Pair("mineraddress", (block.vtx[0].vout.size() > 1)? "multiple" : ExtractDestination(block.vtx[0].vout[0].scriptPubKey, address)? CBitcoinAddress(address).ToString().c_str() : "invalid"));
            entry.push_back(Pair("primedigit", (int) bnPrimeChainOrigin.ToString().length()));
            entry.push_back(Pair("primechain", GetPrimeChainName(pindex->nPrimeChainType, pindex->nPrimeChainLength).c_str()));
            entry.push_back(Pair("primeorigin", bnPrimeChainOrigin.ToString().c_str()));
            entry.push_back(Pair("primorialform", GetPrimeOriginPrimorialForm(bnPrimeChainOrigin).c_str()));
            ret.push_back(entry);
        }
    }

    return ret;
}

// Primecoin: list top prime chain within primecoin network
Value listtopprimes(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "listtopprimes <primechain length> [primechain type]\n"
            "Returns the list of top prime chains in primecoin network.\n"
            "<primechain length> is integer like 10, 11, 12 etc.\n"
            "[primechain type] is optional type, among 1CC, 2CC and TWN");

    int nPrimeChainLength = params[0].get_int();
    unsigned int nPrimeChainType = 0;
    if (params.size() > 1)
    {
        std::string strPrimeChainType = params[1].get_str();
        if (strPrimeChainType.compare("1CC") == 0)
            nPrimeChainType = PRIME_CHAIN_CUNNINGHAM1;
        else if (strPrimeChainType.compare("2CC") == 0)
            nPrimeChainType = PRIME_CHAIN_CUNNINGHAM2;
        else if (strPrimeChainType.compare("TWN") == 0)
            nPrimeChainType = PRIME_CHAIN_BI_TWIN;
        else
            throw runtime_error("Prime chain type must be 1CC, 2CC or TWN.");
    }

    // Search for top prime chains
    unsigned int nRankingSize = 10; // ranking list size
    unsigned int nSortVectorSize = 64; // vector size for sort operation
    CBigNum bnPrimeQualify = 0; // minimum qualify value for ranking list
    vector<pair<CBigNum, uint256> > vSortedByOrigin;
    for (CBlockIndex* pindex = pindexGenesisBlock; pindex; pindex = pindex->pnext)
    {
        if (nPrimeChainLength != (int) TargetGetLength(pindex->nPrimeChainLength))
            continue; // length not matching, next block
        if (nPrimeChainType && nPrimeChainType != pindex->nPrimeChainType)
            continue; // type not matching, next block

        CBlock block;
        block.ReadFromDisk(pindex); // read block
        CBigNum bnPrimeChainOrigin = CBigNum(block.GetHeaderHash()) * block.bnPrimeChainMultiplier; // compute prime chain origin

        if (bnPrimeChainOrigin > bnPrimeQualify)
            vSortedByOrigin.push_back(make_pair(bnPrimeChainOrigin, block.GetHash()));

        if (vSortedByOrigin.size() >= nSortVectorSize)
        {
            // Sort prime chain candidates
            sort(vSortedByOrigin.begin(), vSortedByOrigin.end());
            reverse(vSortedByOrigin.begin(), vSortedByOrigin.end());
            // Truncate candidate list
            while (vSortedByOrigin.size() > nRankingSize)
                vSortedByOrigin.pop_back();
            // Update minimum qualify value for top prime chains
            bnPrimeQualify = vSortedByOrigin.back().first;
        }
    }

    // Final sort of prime chain candidates
    sort(vSortedByOrigin.begin(), vSortedByOrigin.end());
    reverse(vSortedByOrigin.begin(), vSortedByOrigin.end());
    // Truncate candidate list
    while (vSortedByOrigin.size() > nRankingSize)
        vSortedByOrigin.pop_back();

    // Output top prime chains
    Array ret;
    BOOST_FOREACH(const PAIRTYPE(CBigNum, uint256)& item, vSortedByOrigin)
    {
        CBigNum bnPrimeChainOrigin = item.first;
        CBlockIndex* pindex = mapBlockIndex[item.second];
        CBlock block;
        block.ReadFromDisk(pindex); // read block
        Object entry;
        entry.push_back(Pair("time", DateTimeStrFormat("%Y-%m-%d %H:%M:%S UTC", pindex->GetBlockTime()).c_str()));
        entry.push_back(Pair("epoch", (boost::int64_t) pindex->GetBlockTime()));
        entry.push_back(Pair("height", pindex->nHeight));
        entry.push_back(Pair("ismine", pwalletMain->IsMine(block.vtx[0])));
        CTxDestination address;
        entry.push_back(Pair("mineraddress", (block.vtx[0].vout.size() > 1)? "multiple" : ExtractDestination(block.vtx[0].vout[0].scriptPubKey, address)? CBitcoinAddress(address).ToString().c_str() : "invalid"));
        entry.push_back(Pair("primedigit", (int) bnPrimeChainOrigin.ToString().length()));
        entry.push_back(Pair("primechain", GetPrimeChainName(pindex->nPrimeChainType, pindex->nPrimeChainLength).c_str()));
        entry.push_back(Pair("primeorigin", bnPrimeChainOrigin.ToString().c_str()));
        entry.push_back(Pair("primorialform", GetPrimeOriginPrimorialForm(bnPrimeChainOrigin).c_str()));
        ret.push_back(entry);
    }

    return ret;
}
