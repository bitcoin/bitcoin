#ifndef BITCOIN_OMNICORE_TX_H
#define BITCOIN_OMNICORE_TX_H

class CMPMetaDEx;
class CMPOffer;
class CTransaction;

#include <omnicore/omnicore.h>
#include <omnicore/parsing.h>

#include <uint256.h>
#include <util/strencodings.h>

#include <stdint.h>
#include <string.h>

#include <string>

using mastercore::strTransactionType;

/** The class is responsible for transaction interpreting/parsing.
 *
 * It invokes other classes and methods: offers, accepts, tallies (balances).
 */
class CMPTransaction
{
    friend class CMPMetaDEx;
    friend class CMPOffer;

private:
    uint256 txid;
    int block;
    int64_t blockTime;  // internally nTime is still an "unsigned int"
    unsigned int tx_idx;  // tx # within the block, 0-based
    uint64_t tx_fee_paid;

    int pkt_size;
    unsigned char pkt[1 + MAX_PACKETS * PACKET_SIZE];
    int encodingClass;  // No Marker = 0, Class A = 1, Class B = 2, Class C = 3

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

    // SendToOwners v1
    unsigned int distribution_property;

    // CreatePropertyFixed, CreatePropertyVariable, CreatePropertyMananged, MetaDEx, SendAll
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
    unsigned char action; // deprecated

    // TradeOffer
    uint64_t amount_desired;
    unsigned char blocktimelimit;
    uint64_t min_fee;
    unsigned char subaction;

    // Unique Send
    uint64_t nonfungible_token_start;
    uint64_t nonfungible_token_end;
    uint8_t nonfungible_data_type; // Issuer (1) or holder (0)
    char nonfungible_data[SP_STRING_FIELD_LEN]; // GrantData, IssuerData or HolderData

    // Alert
    uint16_t alert_type;
    uint32_t alert_expiry;
    char alert_text[SP_STRING_FIELD_LEN];

    // Activation
    uint16_t feature_id;
    uint32_t activation_block;
    uint32_t min_client_version;

    // Indicates whether the transaction can be used to execute logic
    bool rpcOnly;

    /** Checks whether a pointer to the payload is past it's last position. */
    bool isOverrun(const char* p);

    /**
     * Payload parsing
     */
    bool interpret_TransactionType();
    bool interpret_SimpleSend();
    bool interpret_SendToOwners();
    bool interpret_SendAll();
    bool interpret_SendNonFungible();
    bool interpret_TradeOffer();
    bool interpret_MetaDExTrade();
    bool interpret_MetaDExCancelPrice();
    bool interpret_MetaDExCancelPair();
    bool interpret_MetaDExCancelEcosystem();
    bool interpret_AcceptOfferBTC();
    bool interpret_CreatePropertyFixed();
    bool interpret_CreatePropertyVariable();
    bool interpret_CloseCrowdsale();
    bool interpret_CreatePropertyManaged();
    bool interpret_GrantTokens();
    bool interpret_RevokeTokens();
    bool interpret_ChangeIssuer();
    bool interpret_EnableFreezing();
    bool interpret_DisableFreezing();
    bool interpret_FreezeTokens();
    bool interpret_UnfreezeTokens();
    bool interpret_AddDelegate();
    bool interpret_RemoveDelegate();
    bool interpret_AnyData();
    bool interpret_NonFungibleData();
    bool interpret_Activation();
    bool interpret_Deactivation();
    bool interpret_Alert();

    /**
     * Logic and "effects"
     */
    int logicMath_SimpleSend(uint256& blockHash);
    int logicMath_SendToOwners();
    int logicMath_SendAll();
    int logicMath_SendNonFungible();
    int logicMath_TradeOffer();
    int logicMath_AcceptOffer_BTC();
    int logicMath_MetaDExTrade();
    int logicMath_MetaDExCancelPrice();
    int logicMath_MetaDExCancelPair();
    int logicMath_MetaDExCancelEcosystem();
    int logicMath_CreatePropertyFixed(CBlockIndex *pindex);
    int logicMath_CreatePropertyVariable(CBlockIndex *pindex);
    int logicMath_CloseCrowdsale(CBlockIndex *pindex);
    int logicMath_CreatePropertyManaged(CBlockIndex* pindex);
    int logicMath_GrantTokens(CBlockIndex* pindex, uint256 &blockHash);
    int logicMath_RevokeTokens(CBlockIndex *pindex);
    int logicMath_ChangeIssuer(CBlockIndex *pindex);
    int logicMath_EnableFreezing(CBlockIndex *pindex);
    int logicMath_DisableFreezing(CBlockIndex *pindex);
    int logicMath_FreezeTokens(CBlockIndex *pindex);
    int logicMath_UnfreezeTokens(CBlockIndex *pindex);
    int logicMath_AddDelegate(CBlockIndex *pindex);
    int logicMath_RemoveDelegate(CBlockIndex *pindex);
    int logicMath_AnyData();
    int logicMath_NonFungibleData();
    int logicMath_Activation();
    int logicMath_Deactivation();
    int logicMath_Alert();

    /**
     * Logic helpers
     */
    int logicHelper_CrowdsaleParticipation(uint256& blockHash);

public:
    //! DEx and MetaDEx action values
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

    uint256 getHash() const { return txid; }
    unsigned int getType() const { return type; }
    std::string getTypeString() const { return strTransactionType(getType()); }
    unsigned int getProperty() const { return property; }
    unsigned short getVersion() const { return version; }
    unsigned short getPropertyType() const { return prop_type; }
    uint64_t getFeePaid() const { return tx_fee_paid; }
    std::string getSender() const { return sender; }
    std::string getReceiver() const { return receiver; }
    std::string getPayload() const { return HexStr(pkt, pkt + pkt_size); }
    std::string getPayloadData() const { return HexStr(pkt + 4 /* skip version and type */, pkt + pkt_size); }
    uint64_t getAmount() const { return nValue; }
    uint64_t getNewAmount() const { return nNewValue; }
    uint8_t getEcosystem() const { return ecosystem; }
    uint32_t getPreviousId() const { return prev_prop_id; }
    std::string getSPCategory() const { return category; }
    std::string getSPSubCategory() const { return subcategory; }
    std::string getSPName() const { return name; }
    std::string getSPUrl() const { return url; }
    std::string getSPData() const { return data; }
    int64_t getDeadline() const { return deadline; }
    uint8_t getEarlyBirdBonus() const { return early_bird; }
    uint8_t getIssuerBonus() const { return percentage; }
    bool isRpcOnly() const { return rpcOnly; }
    int getEncodingClass() const { return encodingClass; }
    uint16_t getAlertType() const { return alert_type; }
    uint32_t getAlertExpiry() const { return alert_expiry; }
    std::string getAlertMessage() const { return alert_text; }
    int getPayloadSize() const { return pkt_size; }
    uint16_t getFeatureId() const { return feature_id; }
    uint32_t getActivationBlock() const { return activation_block; }
    uint32_t getMinClientVersion() const { return min_client_version; }
    unsigned int getIndexInBlock() const { return tx_idx; }
    uint32_t getDistributionProperty() const { return distribution_property; }
    uint64_t getNonFungibleTokenStart() const { return nonfungible_token_start; }
    uint64_t getNonFungibleTokenEnd() const { return nonfungible_token_end; }
    uint64_t getNonFungibleDataType() const { return nonfungible_data_type; }
    std::string getNonFungibleData() const { return nonfungible_data; }

    /** Creates a new CMPTransaction object. */
    CMPTransaction()
    {
        SetNull();
    }

    /** Resets and clears all values. */
    void SetNull()
    {
        txid.SetNull();
        block = -1;
        blockTime = 0;
        tx_idx = 0;
        tx_fee_paid = 0;
        pkt_size = 0;
        memset(&pkt, 0, sizeof(pkt));
        encodingClass = 0;
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
        alert_type = 0;
        alert_expiry = 0;
        memset(&alert_text, 0, sizeof(alert_text));
        rpcOnly = true;
        feature_id = 0;
        activation_block = 0;
        min_client_version = 0;
        distribution_property = 0;
        nonfungible_token_start = 0;
        nonfungible_token_end = 0;
        memset(&nonfungible_data, 0, sizeof(nonfungible_data));
    }

    /** Sets the given values. */
    void Set(const uint256& t, int b, unsigned int idx, int64_t bt)
    {
        txid = t;
        block = b;
        tx_idx = idx;
        blockTime = bt;
    }

    /** Sets the given values. */
    void Set(const std::string& s, const std::string& r, uint64_t n, const uint256& t,
        int b, unsigned int idx, unsigned char *p, unsigned int size, int encodingClassIn, uint64_t txf)
    {
        sender = s;
        receiver = r;
        txid = t;
        block = b;
        tx_idx = idx;
        pkt_size = size < sizeof (pkt) ? size : sizeof (pkt);
        nValue = n;
        nNewValue = n;
        encodingClass = encodingClassIn;
        tx_fee_paid = txf;
        memcpy(&pkt, p, pkt_size);
    }

    /** Parses the packet or payload. */
    bool interpret_Transaction();

    /** Interprets the payload and executes the logic. */
    int interpretPacket();

    /** Enables access of interpretPacket. */
    void unlockLogic() { rpcOnly = false; };

    /** Compares transaction objects based on block height and position within the block. */
    bool operator<(const CMPTransaction& other) const
    {
        if (block != other.block) return block > other.block;
        return tx_idx > other.tx_idx;
    }
};


#endif // BITCOIN_OMNICORE_TX_H
