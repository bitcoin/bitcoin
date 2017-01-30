// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinconsensus.h"

#include "consensus/header_verify.h"
#include "consensus/interfaces.h"
#include "consensus/params.h"
#include "consensus/validation.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "pubkey.h"
#include "script/interpreter.h"
#include "version.h"

namespace {

/** A class that deserializes a single CTransaction one time. */
class TxInputStream
{
public:
    TxInputStream(int nTypeIn, int nVersionIn, const unsigned char *txTo, size_t txToLen) :
    m_type(nTypeIn),
    m_version(nVersionIn),
    m_data(txTo),
    m_remaining(txToLen)
    {}

    void read(char* pch, size_t nSize)
    {
        if (nSize > m_remaining)
            throw std::ios_base::failure(std::string(__func__) + ": end of data");

        if (pch == NULL)
            throw std::ios_base::failure(std::string(__func__) + ": bad destination buffer");

        if (m_data == NULL)
            throw std::ios_base::failure(std::string(__func__) + ": bad source buffer");

        memcpy(pch, m_data, nSize);
        m_remaining -= nSize;
        m_data += nSize;
    }

    template<typename T>
    TxInputStream& operator>>(T& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }

    int GetVersion() const { return m_version; }
    int GetType() const { return m_type; }
private:
    const int m_type;
    const int m_version;
    const unsigned char* m_data;
    size_t m_remaining;
};

inline int set_error(bitcoinconsensus_error* ret, bitcoinconsensus_error serror)
{
    if (ret)
        *ret = serror;
    return 0;
}

struct ECCryptoClosure
{
    ECCVerifyHandle handle;
};

ECCryptoClosure instance_of_eccryptoclosure;
}

/** Check that all specified flags are part of the libconsensus interface. */
static bool verify_flags(unsigned int flags)
{
    return (flags & ~(bitcoinconsensus_SCRIPT_FLAGS_VERIFY_ALL)) == 0;
}

static int verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, CAmount amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, bitcoinconsensus_error* err)
{
    if (!verify_flags(flags)) {
        return bitcoinconsensus_ERR_INVALID_FLAGS;
    }
    try {
        TxInputStream stream(SER_NETWORK, PROTOCOL_VERSION, txTo, txToLen);
        CTransaction tx(deserialize, stream);
        if (nIn >= tx.vin.size())
            return set_error(err, bitcoinconsensus_ERR_TX_INDEX);
        if (GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION) != txToLen)
            return set_error(err, bitcoinconsensus_ERR_TX_SIZE_MISMATCH);

        // Regardless of the verification result, the tx did not error.
        set_error(err, bitcoinconsensus_ERR_OK);

        PrecomputedTransactionData txdata(tx);
        return VerifyScript(tx.vin[nIn].scriptSig, CScript(scriptPubKey, scriptPubKey + scriptPubKeyLen), &tx.vin[nIn].scriptWitness, flags, TransactionSignatureChecker(&tx, nIn, amount, txdata), NULL);
    } catch (const std::exception&) {
        return set_error(err, bitcoinconsensus_ERR_TX_DESERIALIZE); // Error deserializing
    }
}

int bitcoinconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, bitcoinconsensus_error* err)
{
    CAmount am(amount);
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, txTo, txToLen, nIn, flags, err);
}


int bitcoinconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                   const unsigned char *txTo        , unsigned int txToLen,
                                   unsigned int nIn, unsigned int flags, bitcoinconsensus_error* err)
{
    if (flags & bitcoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS) {
        return set_error(err, bitcoinconsensus_ERR_AMOUNT_REQUIRED);
    }

    CAmount am(0);
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, txTo, txToLen, nIn, flags, err);
}

void* bitcoinconsensus_create_consensus_parameters(unsigned char* pHashGenesisBlock, int nSubsidyHalvingInterval, int BIP34Height, unsigned char* pBIP34Hash, int BIP65Height, int BIP66Height, uint32_t nRuleChangeActivationThreshold, uint32_t nMinerConfirmationWindow, int bitDeploymentCsv, int64_t nStartTimeDeploymentCsv, int64_t nTimeoutDeploymentCsv, int bitDeploymentSegwit, int64_t nStartTimeDeploymentSegwit, int64_t nTimeoutDeploymentSegwit, unsigned char* pPowLimit, bool fPowAllowMinDifficultyBlocks, bool fPowNoRetargeting, int64_t nPowTargetSpacing, int64_t nPowTargetTimespan)
{
    Consensus::Params* consensusParams = new Consensus::Params();

    consensusParams->vDeployments[Consensus::DEPLOYMENT_CSV].bit = bitDeploymentCsv;
    consensusParams->vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = nStartTimeDeploymentCsv;
    consensusParams->vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = nTimeoutDeploymentCsv;
    consensusParams->vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = bitDeploymentSegwit;
    consensusParams->vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = nStartTimeDeploymentSegwit;
    consensusParams->vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = nTimeoutDeploymentSegwit;

    consensusParams->nSubsidyHalvingInterval = nSubsidyHalvingInterval;
    consensusParams->BIP34Height = BIP34Height;
    consensusParams->BIP65Height = BIP65Height;
    consensusParams->BIP66Height = BIP66Height;
    consensusParams->nRuleChangeActivationThreshold = nRuleChangeActivationThreshold;
    consensusParams->nMinerConfirmationWindow = nMinerConfirmationWindow;
    consensusParams->fPowAllowMinDifficultyBlocks = fPowAllowMinDifficultyBlocks;
    consensusParams->fPowNoRetargeting = fPowNoRetargeting;
    consensusParams->nPowTargetSpacing = nPowTargetSpacing;
    consensusParams->nPowTargetTimespan = nPowTargetTimespan;

    consensusParams->hashGenesisBlock = uint256(pHashGenesisBlock);
    consensusParams->BIP34Hash = uint256(pBIP34Hash);
    consensusParams->powLimit = uint256(pPowLimit);

    return consensusParams;
}

void bitcoinconsensus_destroy_consensus_parameters(void* consensusParams)
{
    delete ((Consensus::Params*)consensusParams);
}

void* bitcoinconsensus_create_blockindex_interface(GetAncestorFn _GetAncestorFn, GetHashFn _GetHashFn, GetHeightFn _GetHeightFn, GetVersionFn _GetVersionFn, GetTimeFn _GetTimeFn, GetBitsFn _GetBitsFn, GetPrevFn _GetPrevFn, GetMedianTimeFn _GetMedianTimeFn, IndexDeallocatorFn _IndexDeallocatorFn)
{
    BlockIndexInterface* iBlockIndex = new BlockIndexInterface();

    iBlockIndex->GetAncestor = _GetAncestorFn;
    iBlockIndex->GetHash = _GetHashFn;
    iBlockIndex->GetHeight = _GetHeightFn;
    iBlockIndex->GetVersion = _GetVersionFn;
    iBlockIndex->GetTime = _GetTimeFn;
    iBlockIndex->GetBits = _GetBitsFn;
    iBlockIndex->GetPrev = _GetPrevFn;
    iBlockIndex->GetMedianTime = _GetMedianTimeFn;
    iBlockIndex->DeleteIndex = _IndexDeallocatorFn;

    return iBlockIndex;
}

void bitcoinconsensus_destroy_blockindex_interface(void* iBlockIndex)
{
    delete ((BlockIndexInterface*)iBlockIndex);
}

int bitcoinconsensus_verify_header(const unsigned char* header, unsigned int headerLen, const void* consensusParams, const void* indexObject, const void* iBlockIndex, int64_t nAdjustedTime, bitcoinconsensus_error* err)
{
    try {
        TxInputStream stream(SER_NETWORK, PROTOCOL_VERSION, header, headerLen);
        CBlockHeader blockHeader;
        stream >> blockHeader;
        CValidationState state;
        return VerifyHeader(blockHeader, state, *(const Consensus::Params*)consensusParams, indexObject, *(const BlockIndexInterface*)iBlockIndex, nAdjustedTime);
    } catch (const std::exception&) {
        return set_error(err, bitcoinconsensus_ERR_TX_DESERIALIZE); // Error deserializing
    }
}

unsigned int bitcoinconsensus_version()
{
    // Just use the API version for now
    return BITCOINCONSENSUS_API_VER;
}
