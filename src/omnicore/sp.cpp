// Smart Properties & Crowd Sales

#include <omnicore/sp.h>

#include <omnicore/log.h>
#include <omnicore/omnicore.h>
#include <omnicore/uint256_extensions.h>

#include <arith_uint256.h>
#include <hash.h>
#include <validation.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/time.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>

#include <map>
#include <string>
#include <vector>
#include <utility>

using namespace mastercore;

CMPCrowd::CMPCrowd()
  : propertyId(0), nValue(0), property_desired(0), deadline(0),
    early_bird(0), percentage(0), u_created(0), i_created(0)
{
}

CMPCrowd::CMPCrowd(uint32_t pid, int64_t nv, uint32_t cd, int64_t dl, uint8_t eb, uint8_t per, int64_t uct, int64_t ict)
  : propertyId(pid), nValue(nv), property_desired(cd), deadline(dl),
    early_bird(eb), percentage(per), u_created(uct), i_created(ict)
{
}

void CMPCrowd::insertDatabase(const uint256& txHash, const std::vector<int64_t>& txData)
{
    txFundraiserData.insert(std::make_pair(txHash, txData));
}

std::string CMPCrowd::toString(const std::string& address) const
{
    return strprintf("%34s : id=%u=%X; prop=%u, value= %li, deadline: %d)", address, propertyId, propertyId,
        property_desired, nValue, deadline);
}

void CMPCrowd::print(const std::string& address, FILE* fp) const
{
    fprintf(fp, "%s\n", toString(address).c_str());
}

void CMPCrowd::saveCrowdSale(std::ofstream& file, const std::string& addr, CHash256& hasher) const
{
    // compose the outputline
    // addr,propertyId,nValue,property_desired,deadline,early_bird,percentage,created,mined
    std::string lineOut = strprintf("%s,%d,%d,%d,%d,%d,%d,%d,%d",
            addr,
            propertyId,
            nValue,
            property_desired,
            deadline,
            early_bird,
            percentage,
            u_created,
            i_created);

    // append N pairs of address=nValue;blockTime for the database
    std::map<uint256, std::vector<int64_t> >::const_iterator iter;
    for (iter = txFundraiserData.begin(); iter != txFundraiserData.end(); ++iter) {
        lineOut.append(strprintf(",%s=", (*iter).first.GetHex()));
        std::vector<int64_t> const &vals = (*iter).second;

        std::vector<int64_t>::const_iterator valIter;
        for (valIter = vals.begin(); valIter != vals.end(); ++valIter) {
            if (valIter != vals.begin()) {
                lineOut.append(";");
            }

            lineOut.append(strprintf("%d", *valIter));
        }
    }

    // add the line to the hash
    hasher.Write((unsigned char*)lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << std::endl;
}

CMPCrowd* mastercore::getCrowd(const std::string& address)
{
    CrowdMap::iterator my_it = my_crowds.find(address);

    if (my_it != my_crowds.end()) return &(my_it->second);

    return static_cast<CMPCrowd*>(nullptr);
}

bool mastercore::IsPropertyIdValid(uint32_t propertyId)
{
    if (propertyId == 0) return false;

    uint32_t nextId = 0;

    if (propertyId < TEST_ECO_PROPERTY_1) {
        nextId = pDbSpInfo->peekNextSPID(1);
    } else {
        nextId = pDbSpInfo->peekNextSPID(2);
    }

    if (propertyId < nextId) {
        return true;
    }

    return false;
}

bool mastercore::isPropertyNonFungible(uint32_t propertyId)
{
    CMPSPInfo::Entry sp;

    if (pDbSpInfo->getSP(propertyId, sp)) return sp.unique;

    return false;
}

bool mastercore::isPropertyDivisible(uint32_t propertyId)
{
    // TODO: is a lock here needed
    CMPSPInfo::Entry sp;

    if (pDbSpInfo->getSP(propertyId, sp)) return sp.isDivisible();

    return true;
}

std::string mastercore::getPropertyName(uint32_t propertyId)
{
    CMPSPInfo::Entry sp;
    if (pDbSpInfo->getSP(propertyId, sp)) return sp.name;
    return "Property Name Not Found";
}

bool mastercore::isCrowdsaleActive(uint32_t propertyId)
{
    for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
        const CMPCrowd& crowd = it->second;
        uint32_t foundPropertyId = crowd.getPropertyId();
        if (foundPropertyId == propertyId) return true;
    }
    return false;
}

/**
 * Calculates missing bonus tokens, which are credited to the crowdsale issuer.
 *
 * Due to rounding effects, a crowdsale issuer may not receive the full
 * bonus immediately. The missing amount is calculated based on the total
 * tokens created and already credited.
 *
 * @param sp        The crowdsale property
 * @param crowdsale The crowdsale
 * @return The number of missed tokens
 */
int64_t mastercore::GetMissedIssuerBonus(const CMPSPInfo::Entry& sp, const CMPCrowd& crowdsale)
{
    // consistency check
    assert(getTotalTokens(crowdsale.getPropertyId())
            == (crowdsale.getIssuerCreated() + crowdsale.getUserCreated()));

    arith_uint256 amountMissing = 0;
    arith_uint256 bonusPercentForIssuer = ConvertTo256(sp.percentage);
    arith_uint256 amountAlreadyCreditedToIssuer = ConvertTo256(crowdsale.getIssuerCreated());
    arith_uint256 amountCreditedToUsers = ConvertTo256(crowdsale.getUserCreated());
    arith_uint256 amountTotal = amountCreditedToUsers + amountAlreadyCreditedToIssuer;

    // calculate theoretical bonus for issuer based on the amount of
    // tokens credited to users
    arith_uint256 exactBonus = amountCreditedToUsers * bonusPercentForIssuer;
    exactBonus /= ConvertTo256(100); // 100 %

    // there shall be no negative missing amount
    if (exactBonus < amountAlreadyCreditedToIssuer) {
        return 0;
    }

    // subtract the amount already credited to the issuer
    if (exactBonus > amountAlreadyCreditedToIssuer) {
        amountMissing = exactBonus - amountAlreadyCreditedToIssuer;
    }

    // calculate theoretical total amount of all tokens
    arith_uint256 newTotal = amountTotal + amountMissing;

    // reduce to max. possible amount
    if (newTotal > uint256_const::max_int64) {
        amountMissing = uint256_const::max_int64 - amountTotal;
    }

    return ConvertTo64(amountMissing);
}

// calculateFundraiser does token calculations per transaction
void mastercore::calculateFundraiser(bool inflateAmount, int64_t amtTransfer, uint8_t bonusPerc,
        int64_t fundraiserSecs, int64_t currentSecs, int64_t numProps, uint8_t issuerPerc, int64_t totalTokens,
        std::pair<int64_t, int64_t>& tokens, bool& close_crowdsale)
{
    // Weeks in seconds
    arith_uint256 weeks_sec_ = ConvertTo256(604800);

    // Precision for all non-bitcoin values (bonus percentages, for example)
    arith_uint256 precision_ = ConvertTo256(1000000000000LL);

    // Precision for all percentages (10/100 = 10%)
    arith_uint256 percentage_precision = ConvertTo256(100);

    // Calculate the bonus seconds
    arith_uint256 bonusSeconds_ = 0;
    if (currentSecs < fundraiserSecs) {
        bonusSeconds_ = ConvertTo256(fundraiserSecs) - ConvertTo256(currentSecs);
    }

    // Calculate the whole number of weeks to apply bonus
    arith_uint256 weeks_ = (bonusSeconds_ / weeks_sec_) * precision_;
    weeks_ += (Modulo256(bonusSeconds_, weeks_sec_) * precision_) / weeks_sec_;

    // Calculate the earlybird percentage to be applied
    arith_uint256 ebPercentage_ = weeks_ * ConvertTo256(bonusPerc);

    // Calculate the bonus percentage to apply up to percentage_precision number of digits
    arith_uint256 bonusPercentage_ = (precision_ * percentage_precision);
    bonusPercentage_ += ebPercentage_;
    bonusPercentage_ /= percentage_precision;

    // Calculate the bonus percentage for the issuer
    arith_uint256 issuerPercentage_ = ConvertTo256(issuerPerc);
    issuerPercentage_ *= precision_;
    issuerPercentage_ /= percentage_precision;

    // Precision for bitcoin amounts (satoshi)
    arith_uint256 satoshi_precision_ = ConvertTo256(100000000L);

    // Total tokens including remainders
    arith_uint256 createdTokens = ConvertTo256(amtTransfer);
    if (inflateAmount) {
        createdTokens *= ConvertTo256(100000000L);
    }
    createdTokens *= ConvertTo256(numProps);
    createdTokens *= bonusPercentage_;

    arith_uint256 issuerTokens = createdTokens / satoshi_precision_;
    issuerTokens /= precision_;
    issuerTokens *= (issuerPercentage_ / 100);
    issuerTokens *= precision_;

    arith_uint256 createdTokens_int = createdTokens / precision_;
    createdTokens_int /= satoshi_precision_;

    arith_uint256 issuerTokens_int = issuerTokens / precision_;
    issuerTokens_int /= satoshi_precision_;
    issuerTokens_int /= 100;

    arith_uint256 newTotalCreated = ConvertTo256(totalTokens) + createdTokens_int + issuerTokens_int;

    if (newTotalCreated > uint256_const::max_int64) {
        arith_uint256 maxCreatable = uint256_const::max_int64 - ConvertTo256(totalTokens);
        arith_uint256 created = createdTokens_int + issuerTokens_int;

        // Calculate the ratio of tokens for what we can create and apply it
        arith_uint256 ratio = created * precision_;
        ratio *= satoshi_precision_;
        ratio /= maxCreatable;

        // The tokens for the issuer
        issuerTokens_int = issuerTokens_int * precision_;
        issuerTokens_int *= satoshi_precision_;
        issuerTokens_int /= ratio;

        assert(issuerTokens_int <= maxCreatable);

        // The tokens for the user
        createdTokens_int = maxCreatable - issuerTokens_int;

        // Close the crowdsale after assigning all tokens
        close_crowdsale = true;
    }

    // The tokens to credit
    tokens = std::make_pair(ConvertTo64(createdTokens_int), ConvertTo64(issuerTokens_int));
}

// go hunting for whether a simple send is a crowdsale purchase
// TODO !!!! horribly inefficient !!!! find a more efficient way to do this
bool mastercore::isCrowdsalePurchase(const uint256& txid, const std::string& address, int64_t* propertyId, int64_t* userTokens, int64_t* issuerTokens)
{
    // 1. loop crowdsales (active/non-active) looking for issuer address
    // 2. loop those crowdsales for that address and check their participant txs in database

    // check for an active crowdsale to this address
    CMPCrowd* pcrowdsale = getCrowd(address);
    if (pcrowdsale) {
        std::map<uint256, std::vector<int64_t> >::const_iterator it;
        const std::map<uint256, std::vector<int64_t> >& database = pcrowdsale->getDatabase();
        for (it = database.begin(); it != database.end(); it++) {
            const uint256& tmpTxid = it->first;
            if (tmpTxid == txid) {
                *propertyId = pcrowdsale->getPropertyId();
                *userTokens = it->second.at(2);
                *issuerTokens = it->second.at(3);
                return true;
            }
        }
    }

    // if we still haven't found txid, check non active crowdsales to this address
    for (uint8_t ecosystem = 1; ecosystem <= 2; ecosystem++) {
        uint32_t startPropertyId = (ecosystem == 1) ? 1 : TEST_ECO_PROPERTY_1;
        for (uint32_t loopPropertyId = startPropertyId; loopPropertyId < pDbSpInfo->peekNextSPID(ecosystem); loopPropertyId++) {
            CMPSPInfo::Entry sp;
            if (!pDbSpInfo->getSP(loopPropertyId, sp)) continue;
            for (std::map<uint256, std::vector<int64_t> >::const_iterator it = sp.historicalData.begin(); it != sp.historicalData.end(); it++) {
                if (it->first == txid) {
                    *propertyId = loopPropertyId;
                    *userTokens = it->second.at(2);
                    *issuerTokens = it->second.at(3);
                    return true;
                }
            }
        }
    }

    // didn't find anything, not a crowdsale purchase
    return false;
}

void mastercore::eraseMaxedCrowdsale(const std::string& address, int64_t blockTime, int block, uint256& blockHash)
{
    CrowdMap::iterator it = my_crowds.find(address);

    if (it != my_crowds.end()) {
        const CMPCrowd& crowdsale = it->second;

        PrintToLog("%s(): ERASING MAXED OUT CROWDSALE from address=%s, at block %d (timestamp: %d), SP: %d (%s)\n",
            __func__, address, block, blockTime, crowdsale.getPropertyId(), strMPProperty(crowdsale.getPropertyId()));

        if (msc_debug_sp) {
            PrintToLog("%s(): %s\n", __func__, FormatISO8601DateTime(blockTime));
            PrintToLog("%s(): %s\n", __func__, crowdsale.toString(address));
        }

        // get sp from data struct
        CMPSPInfo::Entry sp;
        assert(pDbSpInfo->getSP(crowdsale.getPropertyId(), sp));

        // get txdata
        sp.historicalData = crowdsale.getDatabase();
        sp.close_early = true;
        sp.max_tokens = true;
        sp.timeclosed = blockTime;
        sp.update_block = blockHash;

        assert(pDbSpInfo->updateSP(crowdsale.getPropertyId(), sp));

        // no calculate fractional calls here, no more tokens (at MAX)
        my_crowds.erase(it);
    }
}

unsigned int mastercore::eraseExpiredCrowdsale(const CBlockIndex* pBlockIndex)
{
    if (pBlockIndex == nullptr) return 0;

    const int64_t blockTime = pBlockIndex->GetBlockTime();
    const int blockHeight = pBlockIndex->nHeight;
    unsigned int how_many_erased = 0;
    CrowdMap::iterator my_it = my_crowds.begin();

    while (my_crowds.end() != my_it) {
        const std::string& address = my_it->first;
        const CMPCrowd& crowdsale = my_it->second;

        if (blockTime > crowdsale.getDeadline()) {
            PrintToLog("%s(): ERASING EXPIRED CROWDSALE from address=%s, at block %d (timestamp: %d), SP: %d (%s)\n",
                __func__, address, blockHeight, blockTime, crowdsale.getPropertyId(), strMPProperty(crowdsale.getPropertyId()));

            if (msc_debug_sp) {
                PrintToLog("%s(): %s\n", __func__, FormatISO8601DateTime(blockTime));
                PrintToLog("%s(): %s\n", __func__, crowdsale.toString(address));
            }

            // get sp from data struct
            CMPSPInfo::Entry sp;
            assert(pDbSpInfo->getSP(crowdsale.getPropertyId(), sp));

            // find missing tokens
            int64_t missedTokens = GetMissedIssuerBonus(sp, crowdsale);

            // get txdata
            sp.historicalData = crowdsale.getDatabase();
            sp.missedTokens = missedTokens;

            // update SP with this data
            sp.update_block = pBlockIndex->GetBlockHash();
            assert(pDbSpInfo->updateSP(crowdsale.getPropertyId(), sp));

            // update values
            if (missedTokens > 0) {
                assert(update_tally_map(sp.issuer, crowdsale.getPropertyId(), missedTokens, BALANCE));
            }

            my_crowds.erase(my_it++);

            ++how_many_erased;

        } else my_it++;
    }

    return how_many_erased;
}

std::string mastercore::strPropertyType(uint16_t propertyType)
{
    switch (propertyType) {
        case MSC_PROPERTY_TYPE_DIVISIBLE: return "divisible";
        case MSC_PROPERTY_TYPE_INDIVISIBLE: return "indivisible";
        case MSC_PROPERTY_TYPE_NONFUNGIBLE: return "non-fungible";
    }

    return "unknown";
}

std::string mastercore::strEcosystem(uint8_t ecosystem)
{
    switch (ecosystem) {
        case OMNI_PROPERTY_MSC: return "main";
        case OMNI_PROPERTY_TMSC: return "test";
    }

    return "unknown";
}
