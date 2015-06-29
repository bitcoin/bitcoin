#ifndef OMNICORE_TX_H
#define OMNICORE_TX_H

class CMPOffer;
class CMPMetaDEx;

#include "omnicore/log.h"
#include "omnicore/omnicore.h"

class CTransaction;

#include "amount.h"
#include "uint256.h"
#include "utilstrencodings.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <utility>

using mastercore::c_strMasterProtocolTXType;

/** The class is responsible for transaction interpreting/parsing.
 *
 * It invokes other classes and methods: offers, accepts, tallies (balances).
 */
class CMPTransaction
{
private:
    uint256 txid;
    int block;
    int64_t blockTime;  // internally nTime is still an "unsigned int"
    unsigned int tx_idx;  // tx # within the block, 0-based
    uint64_t tx_fee_paid;

    int pkt_size;
    unsigned char pkt[1 + MAX_PACKETS * PACKET_SIZE];
    int multi;  // Class A = 0, Class B = 1, Class C = 2

    std::string sender;
    std::string receiver;

    unsigned int type;
    unsigned short version; // = MP_TX_PKT_V0;

    // SimpleSend, SendToOwners, TradeOffer, MetaDEx, AcceptOfferBTC,
    // CreatePropertyFixed, CreatePropertyVariable, GrantTokens, RevokeTokens
    uint64_t nValue;
    uint64_t nNewValue;

    // SimpleSend, SendToOwners, TradeOffer, MetaDEx, AcceptOfferBTC,
    // CreatePropertyFixed, CreatePropertyVariable, CloseCrowdsale,
    // CreatePropertyMananged, GrantTokens, RevokeTokens, ChangeIssuer
    unsigned int property;

    // CreatePropertyFixed, CreatePropertyVariable, CreatePropertyMananged, MetaDEx
    unsigned char ecosystem;

    // CreatePropertyFixed, CreatePropertyVariable, CreatePropertyMananged
    unsigned short prop_type;
    unsigned int prev_prop_id;
    char category[SP_STRING_FIELD_LEN];
    char subcategory[SP_STRING_FIELD_LEN];
    char name[SP_STRING_FIELD_LEN];
    char url[SP_STRING_FIELD_LEN];
    char data[SP_STRING_FIELD_LEN];
    uint64_t deadline;
    unsigned char early_bird;
    unsigned char percentage;

    // MetaDEx
    unsigned int desired_property;
    uint64_t desired_value;
    unsigned char action; // depreciated

    // TradeOffer
    uint64_t amount_desired;
    unsigned char blocktimelimit;
    uint64_t min_fee;
    unsigned char subaction;

    // Alert
    char alertString[SP_STRING_FIELD_LEN];

    struct SendToOwners_compare
    {
        bool operator()(std::pair<int64_t, std::string> p1, std::pair<int64_t, std::string> p2) const
        {
            if (p1.first == p2.first) return p1.second > p2.second; // reverse check
            else return p1.first < p2.first;
        }
    };

public:
    enum ActionTypes
    {
        INVALID = 0,

        // DEx
        NEW = 1,
        UPDATE = 2,
        CANCEL = 3,

        // MetaDEx
        ADD                 = 1,
        CANCEL_AT_PRICE     = 2,
        CANCEL_ALL_FOR_PAIR = 3,
        CANCEL_EVERYTHING   = 4,
    };

    unsigned int getType() const { return type; }
    std::string getTypeString() const { return c_strMasterProtocolTXType(getType()); }
    unsigned int getProperty() const { return property; }
    unsigned short getVersion() const { return version; }
    unsigned short getPropertyType() const { return prop_type; }
    uint64_t getFeePaid() const { return tx_fee_paid; }

    std::string getSender() const { return sender; }
    std::string getReceiver() const { return receiver; }

    std::string getPayload() const { return HexStr(pkt, pkt + pkt_size); }

    uint64_t getAmount() const { return nValue; }
    uint64_t getNewAmount() const { return nNewValue; }

    std::string getSPName() const { return name; }

    void printInfo(FILE *fp);

    void SetNull()
    {
        txid = 0;
        block = -1;
        blockTime = 0;
        tx_idx = 0;
        tx_fee_paid = 0;
        pkt_size = 0;
        memset(&pkt, 0, sizeof(pkt));
        multi = 0;
        sender.clear();
        receiver.clear();
        type = 0;
        version = 0;
        nValue = 0;
        nNewValue = 0;
        property = 0;
        ecosystem = 0;
        prop_type = 0;
        prev_prop_id = 0;
        memset(&category, 0, sizeof(category));
        memset(&subcategory, 0, sizeof(subcategory));
        memset(&name, 0, sizeof(name));
        memset(&url, 0, sizeof(url));
        memset(&data, 0, sizeof(data));
        deadline = 0;
        early_bird = 0;
        percentage = 0;
        desired_property = 0;
        desired_value = 0;
        action = 0;
        amount_desired = 0;
        blocktimelimit = 0;
        min_fee = 0;
        subaction = 0;
        memset(&alertString, 0, sizeof(alertString));
    }

    CMPTransaction()
    {
        SetNull();
    }

    /**
     * Payload parsing
     */
    bool interpret_Transaction();
    bool interpret_TransactionType();
    bool interpret_SimpleSend();
    bool interpret_SendToOwners();
    bool interpret_TradeOffer();
    bool interpret_MetaDExTrade();
    bool interpret_MetaDExCancelPrice();
    bool interpret_MetaDExCancelPair();
    bool interpret_MetaDExCancelEcosystem();
    bool interpret_AcceptOfferBTC();
    bool interpret_CreatePropertyFixed();
    bool interpret_CreatePropertyVariable();
    bool interpret_CloseCrowdsale();
    bool interpret_CreatePropertyMananged();
    bool interpret_GrantTokens();
    bool interpret_RevokeTokens();
    bool interpret_ChangeIssuer();
    bool interpret_Alert();

    /**
     * Logic and "effects"
     */
    int logicMath_SimpleSend();
    int logicMath_SendToOwners();
    int logicMath_TradeOffer(CMPOffer*);
    int logicMath_MetaDEx(CMPMetaDEx*);
    int logicMath_AcceptOffer_BTC();
    int logicMath_MetaDExTrade();
    int logicMath_MetaDExCancelPrice();
    int logicMath_MetaDExCancelPair();
    int logicMath_MetaDExCancelEcosystem();
    int logicMath_CreatePropertyFixed();
    int logicMath_CreatePropertyVariable();
    int logicMath_CloseCrowdsale();
    int logicMath_CreatePropertyMananged();
    int logicMath_GrantTokens();
    int logicMath_RevokeTokens();
    int logicMath_ChangeIssuer();
    int logicMath_Alert();
    int logicMath_SavingsMark();
    int logicMath_SavingsCompromised();

    int interpretPacket(CMPOffer* obj_o = NULL, CMPMetaDEx* mdex_o = NULL);
    bool isOverrun(const char* p, unsigned int line);

    // Deprecated
    int step1();
    int step2_Alert(std::string* new_global_alert_message);
    int step2_Value();
    const char* step2_SmartProperty(int& error_code);
    int step3_sp_fixed(const char* p);
    int step3_sp_variable(const char* p);

    void Set(const uint256& t, int b, unsigned int idx, int64_t bt)
    {
        txid = t;
        block = b;
        tx_idx = idx;
        blockTime = bt;
    }

    void Set(const std::string& s, const std::string& r, uint64_t n, const uint256& t,
        int b, unsigned int idx, unsigned char *p, unsigned int size, int fMultisig, uint64_t txf)
    {
        sender = s;
        receiver = r;
        txid = t;
        block = b;
        tx_idx = idx;
        pkt_size = size < sizeof (pkt) ? size : sizeof (pkt);
        nValue = n;
        nNewValue = n;
        multi = fMultisig;
        tx_fee_paid = txf;
        memcpy(&pkt, p, pkt_size);
    }

    bool operator<(const CMPTransaction& other) const
    {
        // sort by block # & additionally the tx index within the block
        if (block != other.block) return block > other.block;
        return tx_idx > other.tx_idx;
    }

    void print()
    {
        PrintToLog("BLOCK: %d =txid: %s =fee: %s\n", block, txid.GetHex(), FormatDivisibleMP(tx_fee_paid));
        PrintToLog("sender: %s ; receiver: %s\n", sender, receiver);

        if (0 < pkt_size) {
            PrintToLog("pkt: %s\n", HexStr(pkt, pkt_size + pkt, false));
        } else {
            // error ?
        }
    }
};

int ParseTransaction(const CTransaction& tx, int nBlock, unsigned int idx, CMPTransaction& mptx, unsigned int nTime=0);


#endif // OMNICORE_TX_H
