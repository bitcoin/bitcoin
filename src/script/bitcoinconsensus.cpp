// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinconsensus.h"

#include "primitives/transaction.h"
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

    TxInputStream& read(char* pch, size_t nSize)
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
        return *this;
    }

    template<typename T>
    TxInputStream& operator>>(T& obj)
    {
        ::Unserialize(*this, obj, m_type, m_version);
        return *this;
    }

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


class bitcoinconsensus_txTo_sigchecker {
public:
    CTransaction *tx;
    BaseSignatureChecker *checker;

    bitcoinconsensus_txTo_sigchecker() : tx(NULL), checker(NULL) { }

    ~bitcoinconsensus_txTo_sigchecker() {
        delete tx;
        delete checker;
    };
};

bool bitcoinconsensus_parse_txTo(bitcoinconsensus_txTo_sigchecker& o, const unsigned char *txTo, unsigned int txToLen, unsigned int nIn, bitcoinconsensus_error* err)
{
    CTransaction *ptx = new CTransaction();
    try {
        CTransaction& tx = *ptx;
        try {
            TxInputStream stream(SER_NETWORK, PROTOCOL_VERSION, txTo, txToLen);
            stream >> tx;
        } catch (const std::exception&) {
            delete ptx;
            return set_error(err, bitcoinconsensus_ERR_TX_DESERIALIZE); // Error deserializing
        }

        if (nIn >= tx.vin.size())
            return set_error(err, bitcoinconsensus_ERR_TX_INDEX);
        if (tx.GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION) != txToLen)
            return set_error(err, bitcoinconsensus_ERR_TX_SIZE_MISMATCH);

        o.checker = new TransactionSignatureChecker(&tx, nIn);
        o.tx = ptx;
    } catch (...) {
        delete ptx;
        throw;
    }

    return true;
}

} // anon namespace

int bitcoinconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_txTo_sigchecker o;
        if (!bitcoinconsensus_parse_txTo(o, txTo, txToLen, nIn, err))
            return false;

         // Regardless of the verification result, the tx did not error.
         set_error(err, bitcoinconsensus_ERR_OK);

        return VerifyScript(o.tx->vin[nIn].scriptSig, CScript(scriptPubKey, scriptPubKey + scriptPubKeyLen), flags, *o.checker, NULL);
    } catch (const std::exception&) {
        return set_error(err, bitcoinconsensus_ERR_UNKNOWN);
    }
}

unsigned int bitcoinconsensus_version()
{
    // Just use the API version for now
    return BITCOINCONSENSUS_API_VER;
}
