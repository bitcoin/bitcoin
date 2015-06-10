// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"

#include "util.h"

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}

void COutPoint::print() const
{
    LogPrintf("%s\n", ToString());
}

Credits_CTxIn::Credits_CTxIn(COutPoint prevoutIn, CScript scriptSigIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
}

Credits_CTxIn::Credits_CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
}

std::string Credits_CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    str += prevout.ToString();
    if (prevout.IsNull())
        str += strprintf(", coinbase %s", HexStr(scriptSig));
    else
        str += strprintf(", scriptSig=%s", scriptSig.ToString().substr(0,24));
    str += ")";
    return str;
}

void Credits_CTxIn::print() const
{
    LogPrintf("%s\n", ToString());
}

CTxOut::CTxOut(int64_t nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
}

std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, scriptPubKey.ToString().substr(0,60));
}

void CTxOut::print() const
{
    LogPrintf("%s\n", ToString());
}

Bitcoin_CTxOut::Bitcoin_CTxOut(int64_t nValueOriginalIn, int64_t nValueClaimableIn, CScript scriptPubKeyIn, int nValueOriginalHasBeenSpentIn)
{
	assert_with_stacktrace(Bitcoin_MoneyRange(nValueOriginalIn), strprintf("Bitcoin_CTxOut() : valueOriginal out of range: %d", nValueOriginalIn));
	assert_with_stacktrace(Bitcoin_MoneyRange(nValueClaimableIn), strprintf("Bitcoin_CTxOut() : valueClaimable out of range: %d", nValueClaimableIn));
	assert_with_stacktrace(nValueOriginalIn >= nValueClaimableIn, strprintf("Bitcoin_CTxOut() : valueOriginal less than valueClaimable: %d:%d", nValueOriginalIn, nValueClaimableIn));

	nValueOriginal = nValueOriginalIn;
	nValueClaimable = nValueClaimableIn;
    scriptPubKey = scriptPubKeyIn;
    nValueOriginalHasBeenSpent = nValueOriginalHasBeenSpentIn;
}

std::string Bitcoin_CTxOut::ToString() const
{
    return strprintf("Bitcoin_CTxOut(nValueOriginal=%d.%08d, nValueClaimable=%d.%08d, scriptPubKey=%s(%s), nValueOriginalHasBeenSpent=%d)", nValueOriginal / COIN, nValueOriginal % COIN, nValueClaimable / COIN, nValueClaimable % COIN, scriptPubKey.ToString().substr(0,30), HexStr(scriptPubKey.begin(), scriptPubKey.end(), false), nValueOriginalHasBeenSpent);
}

void Bitcoin_CTxOut::print() const
{
    LogPrintf("%s\n", ToString());
}

uint256 Credits_CTransaction::GetHash() const
{
    return SerializeHash(*this, SER_GETHASH, CREDITS_PROTOCOL_VERSION);
}

int64_t Credits_CTransaction::GetValueOut() const
{
    int64_t nValueOut = 0;
    BOOST_FOREACH(const CTxOut& txout, vout)
    {
        nValueOut += txout.nValue;
        if (!Credits_MoneyRange(txout.nValue) || !Credits_MoneyRange(nValueOut))
            throw std::runtime_error("Credits_CTransaction::GetValueOut() : value out of range");
    }
    return nValueOut;
}
int64_t Credits_CTransaction::GetDepositValueOut() const
{
    int64_t nValueOut = vout[0].nValue;
	if (!Credits_MoneyRange(nValueOut))
		throw std::runtime_error("Credits_CTransaction::GetDepositValueOut() : value out of range");
    return nValueOut;
}

double Credits_CTransaction::ComputePriority(double dPriorityInputs, unsigned int nTxSize) const
{
    // In order to avoid disincentivizing cleaning up the UTXO set we don't count
    // the constant overhead for each txin and up to 110 bytes of scriptSig (which
    // is enough to cover a compressed pubkey p2sh redemption) for priority.
    // Providing any more cleanup incentive than making additional inputs free would
    // risk encouraging people to create junk outputs to redeem later.
    if (nTxSize == 0)
        nTxSize = ::GetSerializeSize(*this, SER_NETWORK, CREDITS_PROTOCOL_VERSION);
    BOOST_FOREACH(const Credits_CTxIn& txin, vin)
    {
        unsigned int offset = 41U + std::min(110U, (unsigned int)txin.scriptSig.size());
        if (nTxSize > offset)
            nTxSize -= offset;
    }
    if (nTxSize == 0) return 0.0;
    return dPriorityInputs / nTxSize;
}

std::string Credits_CTransaction::ToString() const
{
    std::string str;
    str += strprintf("Credits_CTransaction(hash=%s, ver=%d, type=%d, vin.size=%u, vout.size=%u, signingKeyId=%s, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        nTxType,
        vin.size(),
        vout.size(),
        signingKeyId.GetHex(),
        nLockTime);
    for (unsigned int i = 0; i < vin.size(); i++)
        str += "    " + vin[i].ToString() + "\n";
    for (unsigned int i = 0; i < vout.size(); i++)
        str += "    " + vout[i].ToString() + "\n";
    return str;
}

void Credits_CTransaction::print() const
{
    LogPrintf("%s", ToString());
}

// Amount compression:
// * If the amount is 0, output 0
// * first, divide the amount (in base units) by the largest power of 10 possible; call the exponent e (e is max 9)
// * if e<9, the last digit of the resulting number cannot be 0; store it as d, and drop it (divide by 10)
//   * call the result n
//   * output 1 + 10*(9*n + d - 1) + e
// * if e==9, we only know the resulting number is not zero, so output 1 + 10*(n - 1) + 9
// (this is decodable, as d is in [1-9] and e is in [0-9])

uint64_t CTxOutCompressor::CompressAmount(uint64_t n)
{
    if (n == 0)
        return 0;
    int e = 0;
    while (((n % 10) == 0) && e < 9) {
        n /= 10;
        e++;
    }
    if (e < 9) {
        int d = (n % 10);
        assert(d >= 1 && d <= 9);
        n /= 10;
        return 1 + (n*9 + d - 1)*10 + e;
    } else {
        return 1 + (n - 1)*10 + 9;
    }
}

uint64_t CTxOutCompressor::DecompressAmount(uint64_t x)
{
    // x = 0  OR  x = 1+10*(9*n + d - 1) + e  OR  x = 1+10*(n - 1) + 9
    if (x == 0)
        return 0;
    x--;
    // x = 10*(9*n + d - 1) + e
    int e = x % 10;
    x /= 10;
    uint64_t n = 0;
    if (e < 9) {
        // x = 9*n + d - 1
        int d = (x % 9) + 1;
        x /= 9;
        // x = n
        n = x*10 + d;
    } else {
        n = x+1;
    }
    while (e) {
        n *= 10;
        e--;
    }
    return n;
}

uint64_t Bitcoin_CTxOutCompressor::CompressAmount(uint64_t n)
{
    if (n == 0)
        return 0;
    int e = 0;
    while (((n % 10) == 0) && e < 9) {
        n /= 10;
        e++;
    }
    if (e < 9) {
        int d = (n % 10);
        assert(d >= 1 && d <= 9);
        n /= 10;
        return 1 + (n*9 + d - 1)*10 + e;
    } else {
        return 1 + (n - 1)*10 + 9;
    }
}

uint64_t Bitcoin_CTxOutCompressor::DecompressAmount(uint64_t x)
{
    // x = 0  OR  x = 1+10*(9*n + d - 1) + e  OR  x = 1+10*(n - 1) + 9
    if (x == 0)
        return 0;
    x--;
    // x = 10*(9*n + d - 1) + e
    int e = x % 10;
    x /= 10;
    uint64_t n = 0;
    if (e < 9) {
        // x = 9*n + d - 1
        int d = (x % 9) + 1;
        x /= 9;
        // x = n
        n = x*10 + d;
    } else {
        n = x+1;
    }
    while (e) {
        n *= 10;
        e--;
    }
    return n;
}

/**
 * This is only a helper object used to make a temporary copy of the relevant data from
 * CBlockHeader into a separate memory space which is used to create a BlockHeader hash.
 */
class Credits_CLockHashInput{
public:
    int nVersion;
    uint256 hashPrevBlock;
    uint256 hashLinkedBitcoinBlock;
    unsigned int nBits;
    uint64_t nTotalMonetaryBase;
    uint64_t nTotalDepositBase;
    uint64_t nDepositAmount;

    Credits_CLockHashInput(){
        SetNull();
    }

    void SetNull(){
        nVersion = Credits_CBlockHeader::CURRENT_VERSION;
        hashPrevBlock = 0;
        hashLinkedBitcoinBlock = 0;
        nBits = 0;
        nTotalMonetaryBase = 0;
        nTotalDepositBase = 0;
        nDepositAmount = 0;
    }
};

uint256 Credits_CBlockHeader::GetLockHash() const
{
	Credits_CLockHashInput input;
	input.nVersion = nVersion;
	input.hashPrevBlock = hashPrevBlock;
	input.hashLinkedBitcoinBlock = hashLinkedBitcoinBlock;
	input.nBits = nBits;
	input.nTotalMonetaryBase = nTotalMonetaryBase;
	input.nTotalDepositBase = nTotalDepositBase;
	input.nDepositAmount= nDepositAmount;

    return Hash(BEGIN(input.nVersion), END(input.nDepositAmount));
}

uint256 Credits_CBlockHeader::GetHash() const
{
    return Hash(BEGIN(nVersion), END(nDepositAmount));
}

uint256 Credits_CBlock::BuildSigMerkleTree() const
{
    vSigMerkleTree.clear();
    BOOST_FOREACH(const CCompactSignature& sig, vsig)
    	vSigMerkleTree.push_back(sig.GetHash());
    int j = 0;
    for (int nSize = vsig.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (int i = 0; i < nSize; i += 2)
        {
            int i2 = std::min(i+1, nSize-1);
            vSigMerkleTree.push_back(Hash(BEGIN(vSigMerkleTree[j+i]),  END(vSigMerkleTree[j+i]),
                                       BEGIN(vSigMerkleTree[j+i2]), END(vSigMerkleTree[j+i2])));
        }
        j += nSize;
    }
    return (vSigMerkleTree.empty() ? 0 : vSigMerkleTree.back());
}

uint256 Credits_CBlock::BuildMerkleTree() const
{
    vMerkleTree.clear();
    BOOST_FOREACH(const Credits_CTransaction& tx, vtx)
        vMerkleTree.push_back(tx.GetHash());
    int j = 0;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (int i = 0; i < nSize; i += 2)
        {
            int i2 = std::min(i+1, nSize-1);
            vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
                                       BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
        }
        j += nSize;
    }
    return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
}

void Credits_CBlock::RecalcLockHashAndMerkleRoot()
{
	vtx[0].vin[0].prevout.hash = GetLockHash();
	hashMerkleRoot = BuildMerkleTree();
}
bool Credits_CBlock::UpdateSignatures(const CKeyStore &deposit_keyStore)
{
	vsig.clear();
    BOOST_FOREACH(const Credits_CTransaction& tx, vtx) {
    	if(tx.IsDeposit()) {
			const CKeyID keyID = tx.signingKeyId;
			CKey signingKey;
			if (!deposit_keyStore.GetKey(keyID, signingKey)) {
				return false;
			}

			std::vector<unsigned char> vchSig;
			if (!signingKey.SignCompact(hashMerkleRoot, vchSig)) {
				return false;
			}

			vsig.push_back(CCompactSignature(vchSig));
    	}
    }
    hashSigMerkleRoot = BuildSigMerkleTree();

    return true;
}

std::vector<uint256> Credits_CBlock::GetMerkleBranch(int nIndex) const
{
    if (vMerkleTree.empty())
        BuildMerkleTree();
    std::vector<uint256> vMerkleBranch;
    int j = 0;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        int i = std::min(nIndex^1, nSize-1);
        vMerkleBranch.push_back(vMerkleTree[j+i]);
        nIndex >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}

uint256 Credits_CBlock::CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex)
{
    if (nIndex == -1)
        return 0;
    BOOST_FOREACH(const uint256& otherside, vMerkleBranch)
    {
        if (nIndex & 1)
            hash = Hash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
        nIndex >>= 1;
    }
    return hash;
}

void Credits_CBlock::print() const
{
    LogPrintf("\nCBlock(\nhash=%s, \nver=%d, \nhashPrevBlock=%s, \nhashMerkleRoot=%s, \nnTime=%u, \nnBits=%08x, \nnNonce=%u, \nnTotalMonetaryBase=%u, \nnTotalDepositBase=%u, \nnDepositAmount=%u, \nhashLinkedBitcoinBlock=%u, \nhashSigMerkleRoot=%u, \nvtx=%u)\n\n",
    	GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        nTotalMonetaryBase,
		nTotalDepositBase,
        nDepositAmount,
        hashLinkedBitcoinBlock.ToString(),
        hashSigMerkleRoot.ToString(),
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        LogPrintf("  ");
        vtx[i].print();
    }
    LogPrintf("  vSigMerkleTree: ");
    for (unsigned int i = 0; i < vSigMerkleTree.size(); i++)
    	LogPrintf("%s ", vSigMerkleTree[i].ToString());
    LogPrintf("\n");
    LogPrintf("  vMerkleTree: ");
    for (unsigned int i = 0; i < vMerkleTree.size(); i++)
        LogPrintf("%s ", vMerkleTree[i].ToString());
    LogPrintf("\n");
}
