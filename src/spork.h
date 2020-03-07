
// Copyright (c) 2014-2020 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SPORK_H
#define SPORK_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "main.h"

#include "protocol.h"
#include "legacysigner.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

/*
    Don't ever reuse these IDs for other sporks
    - This would result in old clients getting confused about which spork is for what
*/
#define SPORK_START                                           10001
#define SPORK_END                                             10016

#define SPORK_2_INSTANTX                                      10001
#define SPORK_3_INSTANTX_BLOCK_FILTERING                      10002
#define SPORK_4_ENABLE_MASTERNODE_PAYMENTS                    10003
#define SPORK_5_MAX_VALUE                                     10004
#define SPORK_7_MASTERNODE_SCANNING                           10006
#define SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT                10007
#define SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT                 10008
#define SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES                10009
#define SPORK_11_RESET_BUDGET                                 10010
#define SPORK_12_RECONSIDER_BLOCKS                            10011
#define SPORK_13_ENABLE_SUPERBLOCKS                           10012
#define SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT               10013
#define SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES                10014
#define SPORK_16_DISCONNECT_OLD_NODES                         10015
#define SPORK_17_NFT_TX                                       10016

#define SPORK_2_INSTANTX_DEFAULT                              978307200     //2001-1-1
#define SPORK_3_INSTANTX_BLOCK_FILTERING_DEFAULT              1424217600    //2015-2-18
#define SPORK_4_ENABLE_MASTERNODE_PAYMENTS_DEFAULT            4070908800
#define SPORK_5_MAX_VALUE_DEFAULT                             10000         //10000 CRW
#define SPORK_7_MASTERNODE_SCANNING_DEFAULT                   978307200     //2001-1-1
#define SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT        4070908800   //OFF
#define SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT         4070908800   //OFF
#define SPORK_10_MASTERNODE_DONT_PAY_OLD_NODES_DEFAULT        4070908800   //OFF
#define SPORK_11_RESET_BUDGET_DEFAULT                         0
#define SPORK_12_RECONSIDER_BLOCKS_DEFAULT                    0
#define SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT                   4070908800   //OFF
#define SPORK_14_SYSTEMNODE_PAYMENT_ENFORCEMENT_DEFAULT       4070908800   //OFF
#define SPORK_15_SYSTEMNODE_DONT_PAY_OLD_NODES_DEFAULT        4070908800   //OFF
#define SPORK_16_DISCONNECT_OLD_NODES_DEFAULT                 4070908800   //OFF
#define SPORK_17_NFT_TX_DEFAULT                               1585044000   //2020-3-24 10:00 AM UTC
    
class CSporkMessage;
class CSporkManager;

extern std::map<uint256, CSporkMessage> mapSporks;
extern std::map<int, CSporkMessage> mapSporksActive;
extern CSporkManager sporkManager;

void ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
int64_t GetSporkValue(int nSporkID);
bool IsSporkActive(int nSporkID);
void ExecuteSpork(int nSporkID, int nValue);
void ReprocessBlocks(int nBlocks);

//
// Spork Class
// Keeps track of all of the network spork settings
//

class CSporkMessage
{
public:
    std::vector<unsigned char> vchSig;
    int nSporkID;
    int64_t nValue;
    int64_t nTimeSigned;

    uint256 GetHash(){
        uint256 n = Hash(BEGIN(nSporkID), END(nTimeSigned));
        return n;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nSporkID);
        READWRITE(nValue);
        READWRITE(nTimeSigned);
        READWRITE(vchSig);
    }
};


class CSporkManager
{
private:
    std::vector<unsigned char> vchSig;
    std::string strMasterPrivKey;

public:

    CSporkManager() {
    }

    std::string GetSporkNameByID(int id);
    int GetSporkIDByName(std::string strName);
    bool UpdateSpork(int nSporkID, int64_t nValue);
    bool SetPrivKey(std::string strPrivKey);
    bool CheckSignature(CSporkMessage& spork);
    bool Sign(CSporkMessage& spork);
    void Relay(CSporkMessage& msg);

};

#endif
