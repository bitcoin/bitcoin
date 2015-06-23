// Copyright (c) 2014-2015 The Dash developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef MASTERNODE_LASTPAID_H
#define MASTERNODE_LASTPAID_H

#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include <boost/lexical_cast.hpp>

using namespace std;

class CCoinbasePayee;

extern CCoinbasePayee coinbasePayee;

void DumpCoinbasePayees();

/** Save Budget Manager (coinbase-payee.dat)
 */
class CCoinbasePayeeDB
{
private:
    boost::filesystem::path pathDB;
    std::string strMagicMessage;
public:
    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    CCoinbasePayeeDB();
    bool Write(const CCoinbasePayee &objToSave);
    ReadResult Read(CCoinbasePayee& objToLoad);
};


//
// Coinbase Payee : Keep track of the last time addresses were paid up to a few weeks (used for masternode payments)
//
class CCoinbasePayee
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    uint256 GetScriptHash(CScript& pubkey){
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << pubkey;
        uint256 h1 = ss.GetHash();        
        return h1;
    }


public:
    map<uint256, int64_t> mapPaidTime;
    
    CCoinbasePayee() {
        mapPaidTime.clear();
    }

    void BuildIndex();
    void ProcessBlockCoinbaseTX(CTransaction& txCoinbase, int64_t nTime);
    int64_t GetLastPaid(CScript& pubkey);
    void CleanUp();

    void Clear(){
        LogPrintf("CoinbasePayee object cleared\n");
        mapPaidTime.clear();
    }
    std::string ToString() {
        std::string strMessage = boost::lexical_cast<std::string>((int)mapPaidTime.size()) + " objects";

        return strMessage;
    }

    ADD_SERIALIZE_METHODS;

    //for saving to the serialized db
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(mapPaidTime);
    }
};

#endif
