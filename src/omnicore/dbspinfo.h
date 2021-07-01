#ifndef BITCOIN_OMNICORE_DBSPINFO_H
#define BITCOIN_OMNICORE_DBSPINFO_H

#include <omnicore/dbbase.h>
#include <omnicore/log.h>
#include <omnicore/omnicore.h>

#include <fs.h>
#include <serialize.h>
#include <uint256.h>

#include <stdint.h>

#include <map>
#include <string>

/** LevelDB based storage for currencies, smart properties and tokens.
 *
 * DB Schema:
 *
 *  Key:
 *      char 'B'
 *  Value:
 *      uint256 hashBlock
 *
 *  Key:
 *      char 's'
 *      uint32_t propertyId
 *  Value:
 *      CMPSPInfo::Entry info
 *
 *  Key:
 *      char 't'
 *      uint256 hashTxid
 *  Value:
 *      uint32_t propertyId
 *
 *  Key:
 *      char 'b'
 *      uint256 hashBlock
 *      uint32_t propertyId
 *  Value:
 *      CMPSPInfo::Entry info
 */
class CMPSPInfo : public CDBBase
{
public:
    struct Entry {
        // common SP data
        std::string issuer;
        std::string delegate;
        uint16_t prop_type;
        uint32_t prev_prop_id;
        std::string category;
        std::string subcategory;
        std::string name;
        std::string url;
        std::string data;
        int64_t num_tokens;

        // crowdsale generated SP
        uint32_t property_desired;
        int64_t deadline;
        uint8_t early_bird;
        uint8_t percentage;

        // closedearly states, if the SP was a crowdsale and closed due to MAXTOKENS or CLOSE command
        bool close_early;
        bool max_tokens;
        int64_t missedTokens;
        int64_t timeclosed;
        uint256 txid_close;

        // other information
        uint256 txid;
        uint256 creation_block;
        uint256 update_block;
        bool fixed;
        bool manual;
        bool unique;

        // For crowdsale properties:
        //   txid -> amount invested, crowdsale deadline, user issued tokens, issuer issued tokens
        // For managed properties:
        //   txid -> granted amount, revoked amount
        std::map<uint256, std::vector<int64_t> > historicalData;

        // Historical issuers:
        //   (block, idx) -> issuer
        std::map<std::pair<int, int>, std::string > historicalIssuers;

        // Historical delegates:
        //   (block, idx) -> delegate
        std::map<std::pair<int, int>, std::string > historicalDelegates;

        Entry();

        ADD_SERIALIZE_METHODS;

        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action) {
            READWRITE(issuer);
            READWRITE(prop_type);
            READWRITE(prev_prop_id);
            READWRITE(category);
            READWRITE(subcategory);
            READWRITE(name);
            READWRITE(url);
            READWRITE(data);
            READWRITE(num_tokens);
            READWRITE(property_desired);
            READWRITE(deadline);
            READWRITE(early_bird);
            READWRITE(percentage);
            READWRITE(close_early);
            READWRITE(max_tokens);
            READWRITE(missedTokens);
            READWRITE(timeclosed);
            READWRITE(txid_close);
            READWRITE(txid);
            READWRITE(creation_block);
            READWRITE(update_block);
            READWRITE(fixed);
            READWRITE(manual);
            READWRITE(historicalData);
            READWRITE(historicalIssuers);
        }

        bool isDivisible() const;
        void print() const;

        /** Stores a new issuer in the DB. */
        void updateIssuer(int block, int idx, const std::string& newIssuer);

        /** Returns the issuer for the given block. */
        std::string getIssuer(int block) const;

        /** Stores a new delegate in the DB. */
        void addDelegate(int block, int idx, const std::string& newDelegate);

        /** Clears the delegate in the DB. */
        void removeDelegate(int block, int idx);

        /** Returns the delegate for the given block, if there is one. */
        std::string getDelegate(int block) const;
    };

private:
    // implied version of OMN and TOMN so they don't hit the leveldb
    Entry implied_omni;
    Entry implied_tomni;

    uint32_t next_spid;
    uint32_t next_test_spid;

public:
    CMPSPInfo(const fs::path& path, bool fWipe);
    virtual ~CMPSPInfo();

    /** Extends clearing of CDBBase. */
    void Clear();

    void init(uint32_t nextSPID = 0x3UL, uint32_t nextTestSPID = TEST_ECO_PROPERTY_1);

    uint32_t peekNextSPID(uint8_t ecosystem) const;
    bool updateSP(uint32_t propertyId, const Entry& info);
    uint32_t putSP(uint8_t ecosystem, const Entry& info);
    bool getSP(uint32_t propertyId, Entry& info) const;
    bool hasSP(uint32_t propertyId) const;
    uint32_t findSPByTX(const uint256& txid) const;

    int64_t popBlock(const uint256& block_hash);

    void setWatermark(const uint256& watermark);
    bool getWatermark(uint256& watermark) const;

    void printAll() const;
};


#endif // BITCOIN_OMNICORE_DBSPINFO_H
