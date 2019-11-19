#include <chainparams.h>
#include <net.h>
#include <net_processing.h>
#include <validation.h>
#include <shutdown.h>
#include <serialize.h>
#include <consensus/validation.h>
#include <logging.h>
#include <script/standard.h>
#include <wallet/wallet.h>
#include <wallet/coincontrol.h>
#include <core_io.h>
#include <node/transaction.h>
#include <key_io.h>

/** A class that deserializes a single thing one time. */
class InputStream
{
public:
    InputStream(int nTypeIn, int nVersionIn, const unsigned char *data, size_t datalen) :
    m_type(nTypeIn),
    m_version(nVersionIn),
    m_data(data),
    m_remaining(datalen)
    {}

    void read(char* pch, size_t nSize)
    {
        if (nSize > m_remaining)
            throw std::ios_base::failure(std::string(__func__) + ": end of data");

        if (pch == nullptr)
            throw std::ios_base::failure(std::string(__func__) + ": bad destination buffer");

        if (m_data == nullptr)
            throw std::ios_base::failure(std::string(__func__) + ": bad source buffer");

        memcpy(pch, m_data, nSize);
        m_remaining -= nSize;
        m_data += nSize;
    }

    template<typename T>
    InputStream& operator>>(T&& obj)
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

extern "C" {

bool rusty_IsInitialBlockDownload() {
    return ::ChainstateActive().IsInitialBlockDownload();
}

bool rusty_ShutdownRequested() {
    return ShutdownRequested();
}

void rusty_ProcessNewBlock(const uint8_t* blockdata, size_t blocklen, const void* pindexvoid) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    CBlock block;
    try {
        InputStream(SER_NETWORK, PROTOCOL_VERSION, blockdata, blocklen) >> block;
    } catch (...) {}
    if (pindex && block.GetHash() == pindex->GetBlockHash()) {
        ProcessNewBlock(::Params(), std::make_shared<const CBlock>(block), true, nullptr);
    } else {
        ProcessNewBlock(::Params(), std::make_shared<const CBlock>(block), false, nullptr);
    }
}

const void* rusty_ConnectHeaders(const uint8_t* headers_data, size_t stride, size_t count) {
    std::vector<CBlockHeader> headers;
    for(size_t i = 0; i < count; i++) {
        CBlockHeader header;
        try {
            InputStream(SER_NETWORK, PROTOCOL_VERSION, headers_data + (stride * i), 80) >> header;
        } catch (...) {}
        headers.push_back(header);
    }
    BlockValidationState state_dummy;
    const CBlockIndex* last_index = nullptr;
    ProcessNewBlockHeaders(headers, state_dummy, ::Params(), &last_index);
    return last_index;
}

const void* rusty_GetChainTip() {
    LOCK(cs_main);
    const CBlockIndex* tip = ::ChainActive().Tip();
    assert(tip != nullptr);
    return tip;
}

const void* rusty_GetBestHeader() {
    LOCK(cs_main);
    assert(pindexBestHeader != nullptr);
    return pindexBestHeader;
}

const void* rusty_GetGenesisIndex() {
    LOCK(cs_main);
    const CBlockIndex* genesis = ::ChainActive().Genesis();
    assert(genesis != nullptr);
    return genesis;
}

const void* rusty_HashToIndex(const uint8_t* hash_ptr) {
    uint256 hash;
    memcpy(hash.begin(), hash_ptr, 32);
    LOCK(cs_main);
    return LookupBlockIndex(hash);
}

int32_t rusty_IndexToHeight(const void* pindexvoid) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    assert(pindex != nullptr);
    return pindex->nHeight;
}

const uint8_t* rusty_IndexToHash(const void* pindexvoid) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    assert(pindex != nullptr);
    return pindex->phashBlock->begin();
}

const void* rusty_IndexToPrev(const void* pindexvoid) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    return pindex->pprev;
}

const void* rusty_IndexToAncestor(const void* pindexvoid, int32_t height) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    if (height < 0 || height > pindex->nHeight) {
        return nullptr;
    }
    if (height == pindex->nHeight) { return pindex; }
    return pindex->GetAncestor(height);
}

// Create a dummy enum to return a 32 byte array, ensuring changes in uint256 don't break things.
// The compiler should optimize this away for us.
struct ThirtyTwoBytes {
    uint8_t val[32];
};
ThirtyTwoBytes rusty_IndexToWork(const void* pindexvoid) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    ThirtyTwoBytes ret;
    uint256 work = ArithToUint256(pindex->nChainWork);
    memcpy(ret.val, work.begin(), 32);
    return ret;
}

ThirtyTwoBytes rusty_MinimumChainWork() {
    ThirtyTwoBytes ret;
    uint256 work = ArithToUint256(nMinimumChainWork);
    memcpy(ret.val, work.begin(), 32);
    return ret;
}

bool rusty_IndexToHaveData(const void* pindexvoid) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    LOCK(cs_main);
    return (pindex->nStatus & BLOCK_HAVE_DATA) != 0;
}

bool rusty_IndexToNotInvalid(const void* pindexvoid, bool invalid_parent_ok) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    LOCK(cs_main);
    if (pindex->nStatus & BLOCK_FAILED_MASK) { return false; }
    if (pindex->IsValid(BLOCK_VALID_SCRIPTS)) { return true; }
    if (invalid_parent_ok) { return true; }
    while (pindex->pprev && !pindex->pprev->IsValid(BLOCK_VALID_SCRIPTS)) {
        pindex = pindex->pprev;
        if (pindex->nStatus & BLOCK_FAILED_MASK) { return false; }
    }
    return true;
}

void rusty_SerializeIndex(const void* pindexvoid, unsigned char* eighty_bytes_dest) {
    //TODO: Could optimize this a bit
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    std::vector<unsigned char> ser;
    ser.reserve(80);
    CVectorWriter(SER_NETWORK, PROTOCOL_VERSION, ser, 0) << pindex->GetBlockHeader();
    assert(ser.size() == 80);
    memcpy(eighty_bytes_dest, ser.data(), 80);
}

const char* rusty_GetNetworkIDString() {
    auto network = Params().NetworkIDString().c_str();
    return network;
}

void* rusty_GetBlockData(const void* pindexvoid, const unsigned char **data, uint64_t *datalen) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    std::vector<uint8_t> *res = new std::vector<uint8_t>();
    if (!ReadRawBlockFromDisk(*res, pindex, Params().MessageStart())) {
        delete res;
        return nullptr;
    }
    *data = res->data();
    *datalen = res->size();
    return (void*)res;
}

void rusty_FreeBlockData(const void* vectorvoid) {
    std::vector<uint8_t> *vec = (std::vector<uint8_t>*) vectorvoid;
    delete vec;
}

bool rusty_BlockRequestAllowed(const void* pindexvoid) {
    LOCK(cs_main);
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    return BlockRequestAllowed(pindex, Params().GetConsensus());
}

void* rusty_ProviderStateInit(const void* pindexvoid) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    BlockProviderState* state = new BlockProviderState;
    state->m_best_known_block = pindex;
    return state;
}

void rusty_ProviderStateFree(void* providerindexvoid) {
    BlockProviderState* state = (BlockProviderState*) providerindexvoid;
    delete state;
}

void rusty_ProviderStateSetBest(void* providerindexvoid, const void* pindexvoid) {
    BlockProviderState* state = (BlockProviderState*) providerindexvoid;
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    state->m_best_known_block = pindex;
}

const void* rusty_ProviderStateGetNextDownloads(void* providerindexvoid, bool has_witness) {
    BlockProviderState* state = (BlockProviderState*) providerindexvoid;
    std::vector<const CBlockIndex*> blocks;
    LOCK(cs_main);
    state->FindNextBlocksToDownload(has_witness, 1, blocks, ::Params().GetConsensus(), [] (const uint256& block_hash) { return false; });
    return blocks.empty() ? nullptr : blocks[0];
}

void* rusty_InitRandContext() {
    return (void*) new FastRandomContext();
}

void rusty_FreeRandContext(void* contextvoid) {
    FastRandomContext* ctx = (FastRandomContext*) contextvoid;
    delete ctx;
}

uint64_t rusty_GetRandU64(void* contextvoid) {
    FastRandomContext* ctx = (FastRandomContext*) contextvoid;
    return ctx->rand64();
}

uint64_t rusty_GetRandRange(void* contextvoid, uint64_t range) {
    FastRandomContext* ctx = (FastRandomContext*) contextvoid;
    return ctx->randrange(range);
}

ThirtyTwoBytes rusty_GetRandThirtyTwoBytes(void* contextvoid) {
    FastRandomContext* ctx = (FastRandomContext*) contextvoid;
    ThirtyTwoBytes ret;
    uint256 work = ctx->rand256();
    memcpy(ret.val, work.begin(), 32);
    return ret;
}

const uint64_t rusty_GetBlockCount() {
    LOCK(cs_main);
    return ::ChainActive().Height();
};

void* rusty_GetSignedTx(unsigned char* script, unsigned int scriptlen, uint64_t amount, const unsigned char **data, uint64_t *datalen) {
    CMutableTransaction mtx = CMutableTransaction();

    std::vector<unsigned char> scriptvec(script, script + scriptlen);
    std::string s(scriptvec.begin(), scriptvec.end());

    CScript scriptPubKey = GetScriptForDestination(DecodeDestination(s));

    CTxOut out(amount, scriptPubKey);

    mtx.vout.push_back(out);

    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();
    std::shared_ptr<CWallet> pwallet = wallets[0];

    CAmount fee;
    int change_position = 0;

    std::string strFailReason;
    bool lockUnspents = false;
    std::set<int> setSubtractFeeFromOutputs;
    CCoinControl coinControl;

    bool success = pwallet->FundTransaction(mtx, fee, change_position, strFailReason, lockUnspents, setSubtractFeeFromOutputs, coinControl);

    if (!success) {
        return nullptr;
    }

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    pwallet->SignTransaction(mtx);

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << mtx;

    std::string result_str = HexStr(ssTx.str());

    std::vector<uint8_t> *res = new std::vector<uint8_t>(ssTx.begin(), ssTx.end());

    *data = res->data();
    *datalen = res->size();
    return (void*)res;
}

bool rusty_BroadcastTx(const unsigned char* tx, unsigned int txlen) {
    CMutableTransaction mtx;

    std::vector<unsigned char> tx_data(tx, tx + txlen);
    CDataStream ssData(tx_data, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssData >> mtx;
    } catch (const std::exception&) {
        // Fall through.
    }

    std::string result_str = HexStr(ssData.str());

    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();
    std::shared_ptr<CWallet> pwallet = wallets[0];
    std::string strFailReason;

    std::shared_ptr<const CTransaction> tx_ptr = std::make_shared<const CTransaction>(CTransaction(mtx));

    bool ret = pwallet->chain().broadcastTransaction(tx_ptr, strFailReason, pwallet->m_default_max_tx_fee, true);
    return ret;
}

size_t rusty_EstimateFee(unsigned int conftarget, bool conservative) {
    FeeCalculation feeCalc;
    CFeeRate feeRate = ::feeEstimator.estimateSmartFee(conftarget, &feeCalc, conservative);
    return feeRate.GetFeePerK();
}

bool rusty_ImportKey(const unsigned char* key) {
    std::vector<unsigned char> keyvec(key, key + 32);

    CKey privkey = CKey();
    privkey.Set(keyvec.begin(), keyvec.end(), true);

    CPubKey pubkey = privkey.GetPubKey();
    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();
    std::shared_ptr<CWallet> pwallet = wallets[0];

    LOCK(pwallet->cs_wallet);
    return pwallet->ImportPrivKeys({{pubkey.GetID(), privkey}}, 1);
}

void rusty_LogLine(const unsigned char* str, bool debug) {
    if (debug) {
        LogPrint(BCLog::RUST, "%s\n", str);
    } else {
        LogPrintf("%s\n", str);
    }
}

void rusty_AddOutboundP2PNonce(void* pconnmanvoid, uint64_t nonce) {
    CConnman* g_rusty_connman = (CConnman*) pconnmanvoid;
    if (g_rusty_connman) {
        g_rusty_connman->AddOutboundNonce(nonce);
    }
}

void rusty_DropOutboundP2PNonce(void* pconnmanvoid, uint64_t nonce) {
    CConnman* g_rusty_connman = (CConnman*) pconnmanvoid;
    if (g_rusty_connman) {
        g_rusty_connman->DropOutboundNonce(nonce);
    }
}

bool rusty_CheckInboundP2PNonce(void* pconnmanvoid, uint64_t nonce) {
    CConnman* g_rusty_connman = (CConnman*) pconnmanvoid;
    if (g_rusty_connman) {
        return g_rusty_connman->CheckIncomingNonce(nonce);
    }
    return false;
}

}
