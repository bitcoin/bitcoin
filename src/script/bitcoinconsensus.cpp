// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinconsensus.h"

#include "primitives/transaction.h"
#include "script/interpreter.h"
#include "script/script_error.h"
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

typedef std::vector<unsigned char> valtype;
typedef std::vector<valtype> bitcoinconsensus_stack_t;

class bitcoinconsensus_script_execution_data {
public:
    CScriptExecution *sexec;
    bitcoinconsensus_stack_t stack;
    CScript script;
    ScriptError serror;
    bitcoinconsensus_txTo_sigchecker tsc;

    bitcoinconsensus_script_execution_data() : sexec(NULL) { }

    ~bitcoinconsensus_script_execution_data() {
        delete sexec;
    };

    bool parse_stackPos(bitcoinconsensus_stack_t*& stackp, bitcoinconsensus_script_execution_stack stackid, int& stackPos, bool allowEnd)
    {
        switch (stackid)
        {
            case bitcoinconsensus_SEXEC_STACK:
                stackp = &stack;
                break;
            case bitcoinconsensus_SEXEC_ALTSTACK:
                if (!sexec)
                    return false;
                stackp = &sexec->altstack;
                break;
            default:
                return false;
        }

        if (stackPos < 0)
        {
            stackPos = stackp->size() + stackPos + 1;
            if (stackPos < 0)
                return false;
        }
        if ((unsigned int)stackPos + (allowEnd ? 0 : 1) > stackp->size())
            return false;

        return true;
    }

    void Terminate() {
        delete sexec;
        sexec = NULL;
    };
};


} // anon namespace

bitcoinconsensus_script_execution_t *bitcoinconsensus_script_execution(const unsigned char *script, unsigned int scriptLen, const unsigned char *txTo, unsigned int txToLen, unsigned int nIn, bitcoinconsensus_error* err)
{
    bitcoinconsensus_script_execution_data *o = NULL;
    try {
        o = new bitcoinconsensus_script_execution_data();

        {
            std::vector<unsigned char> vchScript;
            vchScript.assign(script, &script[scriptLen]);
            o->script = CScript(vchScript);
        }

        if (txTo) {
            if (!bitcoinconsensus_parse_txTo(o->tsc, txTo, txToLen, nIn, err))
                return NULL;
        } else {
            o->tsc.checker = new BaseSignatureChecker();
        }

        return (bitcoinconsensus_script_execution_t *)o;
    } catch (...) {
        delete o;
        set_error(err, bitcoinconsensus_ERR_UNKNOWN);
        return NULL;
    }
}

int bitcoinconsensus_script_execution_stack_insert(bitcoinconsensus_script_execution_t *po, bitcoinconsensus_script_execution_stack stackid, int stackPos, const void *datap, size_t dataLen, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_script_execution_data *o = (bitcoinconsensus_script_execution_data *)po;
        const unsigned char *data = (const unsigned char *)datap;
        bitcoinconsensus_stack_t *stack;

        if (!o->parse_stackPos(stack, stackid, stackPos, true))
            return set_error(err, bitcoinconsensus_ERR_BAD_INDEX);

        valtype vchData;
        vchData.assign(data, &data[dataLen]);

        stack->insert(stack->begin() + stackPos, vchData);

        return 1;
    } catch (...) {
        return set_error(err, bitcoinconsensus_ERR_UNKNOWN);
    }
}

int bitcoinconsensus_script_execution_stack_delete(bitcoinconsensus_script_execution_t *po, bitcoinconsensus_script_execution_stack stackid, int stackPos, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_script_execution_data *o = (bitcoinconsensus_script_execution_data *)po;
        bitcoinconsensus_stack_t *stack;

        if (!o->parse_stackPos(stack, stackid, stackPos, false))
            return set_error(err, bitcoinconsensus_ERR_BAD_INDEX);

        stack->erase(stack->begin() + stackPos);

        return 1;
    } catch (...) {
        return set_error(err, bitcoinconsensus_ERR_UNKNOWN);
    }
}

int bitcoinconsensus_script_execution_stack_get(bitcoinconsensus_script_execution_t *po, bitcoinconsensus_script_execution_stack stackid, int stackPos, const void **datap, size_t *dataLen, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_script_execution_data *o = (bitcoinconsensus_script_execution_data *)po;
        bitcoinconsensus_stack_t *stack;

        if (!o->parse_stackPos(stack, stackid, stackPos, false))
            return set_error(err, bitcoinconsensus_ERR_BAD_INDEX);

        valtype& vchStackElem = *(stack->begin() + stackPos);

        *datap = vchStackElem.data();
        *dataLen = vchStackElem.size();

        return 1;
    } catch (...) {
        return set_error(err, bitcoinconsensus_ERR_UNKNOWN);
    }
}

int bitcoinconsensus_script_execution_start(bitcoinconsensus_script_execution_t *po, unsigned int flags, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_script_execution_data *o = (bitcoinconsensus_script_execution_data *)po;

        if (o->sexec)
            return set_error(err, bitcoinconsensus_ERR_UNKNOWN);

        o->sexec = new CScriptExecution(o->stack, o->script, flags, *o->tsc.checker, &o->serror);
        if (!o->sexec->Start())
        {
            o->Terminate();
            return set_error(err, bitcoinconsensus_ERR_SCRIPT_EXECUTION);
        }

        return 1;
    } catch (...) {
        return set_error(err, bitcoinconsensus_ERR_UNKNOWN);
    }
}

int bitcoinconsensus_script_execution_step(bitcoinconsensus_script_execution_t *po, int *fEof, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_script_execution_data *o = (bitcoinconsensus_script_execution_data *)po;

        if (!o->sexec->Step())
            return set_error(err, bitcoinconsensus_ERR_SCRIPT_EXECUTION);

        *fEof = o->sexec->fEof;
        return 1;
    } catch (...) {
        return set_error(err, bitcoinconsensus_ERR_UNKNOWN);
    }
}

int bitcoinconsensus_script_execution_terminate(bitcoinconsensus_script_execution_t *po, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_script_execution_data *o = (bitcoinconsensus_script_execution_data *)po;
        o->Terminate();
        return 1;
    } catch (...) {
        return set_error(err, bitcoinconsensus_ERR_UNKNOWN);
    }
}

int bitcoinconsensus_script_execution_get_pc(bitcoinconsensus_script_execution_t *po, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_script_execution_data *o = (bitcoinconsensus_script_execution_data *)po;
        std::vector<unsigned char>::const_iterator cbegin = o->script.begin();
        return std::distance(cbegin, o->sexec->pc);
    } catch (...) {
        set_error(err, bitcoinconsensus_ERR_UNKNOWN);
        return -1;
    }
}

int bitcoinconsensus_script_execution_get_codehash_pos(bitcoinconsensus_script_execution_t *po, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_script_execution_data *o = (bitcoinconsensus_script_execution_data *)po;
        std::vector<unsigned char>::const_iterator cbegin = o->script.begin();
        return std::distance(cbegin, o->sexec->pbegincodehash);
    } catch (...) {
        set_error(err, bitcoinconsensus_ERR_UNKNOWN);
        return -1;
    }
}

int bitcoinconsensus_script_execution_get_sigop_count(bitcoinconsensus_script_execution_t *po, bitcoinconsensus_error* err)
{
    try {
        bitcoinconsensus_script_execution_data *o = (bitcoinconsensus_script_execution_data *)po;
        return o->sexec->nOpCount;
    } catch (...) {
        set_error(err, bitcoinconsensus_ERR_UNKNOWN);
        return -1;
    }
}

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
