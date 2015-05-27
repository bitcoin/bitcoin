#ifndef OMNICORE_SP_H
#define OMNICORE_SP_H

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/persistence.h"

class CBlockIndex;
class uint256;

#include <boost/filesystem.hpp>

#include <openssl/sha.h>

#include "json/json_spirit_value.h"

#include <stdint.h>
#include <stdio.h>

#include <fstream>
#include <map>
#include <string>
#include <vector>

/** LevelDB based storage for currencies, smart properties and tokens.
 */
class CMPSPInfo : public CDBBase
{
public:
    struct Entry {
        // common SP data
        std::string issuer;
        unsigned short prop_type;
        unsigned int prev_prop_id;
        std::string category;
        std::string subcategory;
        std::string name;
        std::string url;
        std::string data;
        uint64_t num_tokens;

        // crowdsale generated SP
        unsigned int property_desired;
        uint64_t deadline;
        unsigned char early_bird;
        unsigned char percentage;

        // closedearly states, if the SP was a crowdsale and closed due to MAXTOKENS or CLOSE command
        bool close_early;
        bool max_tokens;
        uint64_t missedTokens;
        uint64_t timeclosed;
        uint256 txid_close;

        // other information
        uint256 txid;
        uint256 creation_block;
        uint256 update_block;
        bool fixed;
        bool manual;

        // for crowdsale properties, schema is 'txid:amtSent:deadlineUnix:userIssuedTokens:IssuerIssuedTokens;'
        // for manual properties, schema is 'txid:grantAmount:revokeAmount;'
        std::map<std::string, std::vector<uint64_t> > historicalData;

        Entry();

        json_spirit::Object toJSON() const;
        void fromJSON(const json_spirit::Object& json);
        bool isDivisible() const;
        void print() const;
    };

private:
    // implied version of msc and tmsc so they don't hit the leveldb
    Entry implied_msc;
    Entry implied_tmsc;

    unsigned int next_spid;
    unsigned int next_test_spid;

public:
    CMPSPInfo(const boost::filesystem::path& path, bool fWipe);
    virtual ~CMPSPInfo();

    void init(unsigned int nextSPID = 0x3UL, unsigned int nextTestSPID = TEST_ECO_PROPERTY_1);

    unsigned int peekNextSPID(unsigned char ecosystem) const;
    unsigned int updateSP(unsigned int propertyID, const Entry& info);
    unsigned int putSP(unsigned char ecosystem, const Entry& info);
    bool getSP(unsigned int spid, Entry& info) const;
    bool hasSP(unsigned int spid) const;
    unsigned int findSPByTX(const uint256& txid) const;

    int popBlock(const uint256& block_hash);

    static std::string const watermarkKey;
    void setWatermark(const uint256& watermark);
    int getWatermark(uint256& watermark) const;

    void printAll() const;
};

/** A live crowdsale.
 */
class CMPCrowd
{
private:
    unsigned int propertyId;

    uint64_t nValue;

    unsigned int property_desired;
    uint64_t deadline;
    unsigned char early_bird;
    unsigned char percentage;

    uint64_t u_created;
    uint64_t i_created;

    uint256 txid; // NOTE: not persisted as it doesnt seem used

    std::map<std::string, std::vector<uint64_t> > txFundraiserData; // schema is 'txid:amtSent:deadlineUnix:userIssuedTokens:IssuerIssuedTokens;'

public:
    CMPCrowd();
    CMPCrowd(unsigned int pid, uint64_t nv, unsigned int cd, uint64_t dl, unsigned char eb, unsigned char per, uint64_t uct, uint64_t ict);

    unsigned int getPropertyId() const { return propertyId; }

    uint64_t getDeadline() const { return deadline; }
    uint64_t getCurrDes() const { return property_desired; }

    void incTokensUserCreated(uint64_t amount) { u_created += amount; }
    void incTokensIssuerCreated(uint64_t amount) { i_created += amount; }

    uint64_t getUserCreated() const { return u_created; }
    uint64_t getIssuerCreated() const { return i_created; }

    void insertDatabase(const std::string& txhash, const std::vector<uint64_t>& txdata);
    std::map<std::string, std::vector<uint64_t> > getDatabase() const { return txFundraiserData; }

    void print(const std::string& address, FILE* fp = stdout) const;
    void saveCrowdSale(std::ofstream& file, SHA256_CTX* shaCtx, const std::string& addr) const;
};

namespace mastercore
{
typedef std::map<std::string, CMPCrowd> CrowdMap;

extern CMPSPInfo* _my_sps;
extern CrowdMap my_crowds;

const char* c_strPropertyType(int propertyType);

std::string getPropertyName(unsigned int propertyId);
bool isPropertyDivisible(unsigned int propertyId);

CMPCrowd* getCrowd(const std::string& address);

bool isCrowdsaleActive(unsigned int propertyId);
bool isCrowdsalePurchase(const uint256& txid, const std::string& address, int64_t* propertyId, int64_t* userTokens, int64_t* issuerTokens);

// TODO: check, if this could be combined with the other calculate* functions
int calculateFractional(unsigned short int propType, unsigned char bonusPerc, uint64_t fundraiserSecs, 
  uint64_t numProps, unsigned char issuerPerc, const std::map<std::string, std::vector<uint64_t> >& txFundraiserData, 
  const uint64_t amountPremined);

void eraseMaxedCrowdsale(const std::string& address, uint64_t blockTime, int block);

unsigned int eraseExpiredCrowdsale(const CBlockIndex* pBlockIndex);

// TODO: depreciate
void dumpCrowdsaleInfo(const std::string& address, const CMPCrowd& crowd, bool bExpired = false);
}


#endif // OMNICORE_SP_H
