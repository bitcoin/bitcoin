// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/bitcoinconsensus.h>

#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>

namespace {

/** A class that deserializes a single CTransaction one time. */
class TxInputStream
{
public:
    TxInputStream(const unsigned char *txTo, size_t txToLen) :
    m_data(txTo),
    m_remaining(txToLen)
    {}

    void read(Span<std::byte> dst)
    {
        if (dst.size() > m_remaining) {
            throw std::ios_base::failure(std::string(__func__) + ": end of data");
        }

        if (dst.data() == nullptr) {
            throw std::ios_base::failure(std::string(__func__) + ": bad destination buffer");
        }

        if (m_data == nullptr) {
            throw std::ios_base::failure(std::string(__func__) + ": bad source buffer");
        }

        memcpy(dst.data(), m_data, dst.size());
        m_remaining -= dst.size();
        m_data += dst.size();
    }

    template<typename T>
    TxInputStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }

private:
    const unsigned char* m_data;
    size_t m_remaining;
};

inline int set_error(bitcoinconsensus_error* ret, bitcoinconsensus_error serror)
{
    if (ret)
        *ret = serror;
    return 0;
}

} // namespace

/** Check that all specified flags are part of the libconsensus interface. */
static bool verify_flags(unsigned int flags)
{
    return (flags & ~(bitcoinconsensus_SCRIPT_FLAGS_VERIFY_ALL)) == 0;
}

static int verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, CAmount amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    const UTXO *spentOutputs, unsigned int spentOutputsLen,
                                    unsigned int nIn, unsigned int flags, bitcoinconsensus_error* err)
{
    if (!verify_flags(flags)) {
        return set_error(err, bitcoinconsensus_ERR_INVALID_FLAGS);
    }

    if (flags & bitcoinconsensus_SCRIPT_FLAGS_VERIFY_TAPROOT && spentOutputs == nullptr) {
        return set_error(err, bitcoinconsensus_ERR_SPENT_OUTPUTS_REQUIRED);
    }

    try {
        TxInputStream stream(txTo, txToLen);
        CTransaction tx(deserialize, TX_WITH_WITNESS, stream);

        std::vector<CTxOut> spent_outputs;
        if (spentOutputs != nullptr) {
            if (spentOutputsLen != tx.vin.size()) {
                return set_error(err, bitcoinconsensus_ERR_SPENT_OUTPUTS_MISMATCH);
            }
            for (size_t i = 0; i < spentOutputsLen; i++) {
                CScript spk = CScript(spentOutputs[i].scriptPubKey, spentOutputs[i].scriptPubKey + spentOutputs[i].scriptPubKeySize);
                const CAmount& value = spentOutputs[i].value;
                CTxOut tx_out = CTxOut(value, spk);
                spent_outputs.push_back(tx_out);
            }
        }

        if (nIn >= tx.vin.size())
            return set_error(err, bitcoinconsensus_ERR_TX_INDEX);
        if (GetSerializeSize(TX_WITH_WITNESS(tx)) != txToLen)
            return set_error(err, bitcoinconsensus_ERR_TX_SIZE_MISMATCH);

        // Regardless of the verification result, the tx did not error.
        set_error(err, bitcoinconsensus_ERR_OK);

        PrecomputedTransactionData txdata(tx);

        if (spentOutputs != nullptr && flags & bitcoinconsensus_SCRIPT_FLAGS_VERIFY_TAPROOT) {
            txdata.Init(tx, std::move(spent_outputs));
        }

        return VerifyScript(tx.vin[nIn].scriptSig, CScript(scriptPubKey, scriptPubKey + scriptPubKeyLen), &tx.vin[nIn].scriptWitness, flags, TransactionSignatureChecker(&tx, nIn, amount, txdata, MissingDataBehavior::FAIL), nullptr);
    } catch (const std::exception&) {
        return set_error(err, bitcoinconsensus_ERR_TX_DESERIALIZE); // Error deserializing
    }
}

int bitcoinconsensus_verify_script_with_spent_outputs(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    const UTXO *spentOutputs, unsigned int spentOutputsLen,
                                    unsigned int nIn, unsigned int flags, bitcoinconsensus_error* err)
{
    CAmount am(amount);
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, txTo, txToLen, spentOutputs, spentOutputsLen, nIn, flags, err);
}

int bitcoinconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, bitcoinconsensus_error* err)
{
    CAmount am(amount);
    UTXO *spentOutputs = nullptr;
    unsigned int spentOutputsLen = 0;
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, txTo, txToLen, spentOutputs, spentOutputsLen, nIn, flags, err);
}


int bitcoinconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                   const unsigned char *txTo        , unsigned int txToLen,
                                   unsigned int nIn, unsigned int flags, bitcoinconsensus_error* err)
{
    if (flags & bitcoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS) {
        return set_error(err, bitcoinconsensus_ERR_AMOUNT_REQUIRED);
    }

    CAmount am(0);
    UTXO *spentOutputs = nullptr;
    unsigned int spentOutputsLen = 0;
    return ::verify_script(scriptPubKey, scriptPubKeyLen, am, txTo, txToLen, spentOutputs, spentOutputsLen, nIn, flags, err);
}

unsigned int bitcoinconsensus_version()
{
    // Just use the API version for now
    return BITCOINCONSENSUS_API_VER;
}
