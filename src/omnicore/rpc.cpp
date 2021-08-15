/**
 * @file rpc.cpp
 *
 * This file contains RPC calls for data retrieval.
 */

#include <omnicore/rpc.h>

#include <omnicore/activation.h>
#include <omnicore/consensushash.h>
#include <omnicore/convert.h>
#include <omnicore/dbfees.h>
#include <omnicore/dbspinfo.h>
#include <omnicore/dbstolist.h>
#include <omnicore/dbtradelist.h>
#include <omnicore/dbtxlist.h>
#include <omnicore/dex.h>
#include <omnicore/errors.h>
#include <omnicore/log.h>
#include <omnicore/mdex.h>
#include <omnicore/notifications.h>
#include <omnicore/omnicore.h>
#include <omnicore/parsing.h>
#include <omnicore/rpcrequirements.h>
#include <omnicore/rpctxobject.h>
#include <omnicore/rpcvalues.h>
#include <omnicore/rules.h>
#include <omnicore/sp.h>
#include <omnicore/sto.h>
#include <omnicore/tally.h>
#include <omnicore/tx.h>
#include <omnicore/utilsbitcoin.h>
#include <omnicore/version.h>
#include <omnicore/walletfetchtxs.h>
#include <omnicore/walletutils.h>

#include <amount.h>
#include <base58.h>
#include <chainparams.h>
#include <init.h>
#include <index/txindex.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <wallet/rpcwallet.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif

#include <univalue.h>

#include <stdint.h>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>

#include <boost/algorithm/string.hpp> // boost::split

using std::runtime_error;
using namespace mastercore;

/**
 * Throws a JSONRPCError, depending on error code.
 */
void PopulateFailure(int error)
{
    switch (error) {
        case MP_TX_NOT_FOUND:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
        case MP_TX_UNCONFIRMED:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unconfirmed transactions are not supported");
        case MP_BLOCK_NOT_IN_CHAIN:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not part of the active chain");
        case MP_CROWDSALE_WITHOUT_PROPERTY:
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Potential database corruption: \
                                                  \"Crowdsale Purchase\" without valid property identifier");
        case MP_INVALID_TX_IN_DB_FOUND:
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Potential database corruption: Invalid transaction found");
        case MP_TX_IS_NOT_OMNI_PROTOCOL:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No Omni Layer Protocol transaction");
        case MP_TXINDEX_STILL_SYNCING:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No such mempool transaction. Blockchain transactions are still in the process of being indexed.");
        case MP_RPC_DECODE_INPUTS_MISSING:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction inputs were not found. Please provide inputs explicitly (see help description) or fully synchronize node.");

    }
    throw JSONRPCError(RPC_INTERNAL_ERROR, "Generic transaction population failure");
}

void PropertyToJSON(const CMPSPInfo::Entry& sProperty, UniValue& property_obj)
{
    property_obj.pushKV("name", sProperty.name);
    property_obj.pushKV("category", sProperty.category);
    property_obj.pushKV("subcategory", sProperty.subcategory);
    property_obj.pushKV("data", sProperty.data);
    property_obj.pushKV("url", sProperty.url);
    property_obj.pushKV("divisible", sProperty.isDivisible());
    property_obj.pushKV("issuer", sProperty.issuer);
    property_obj.pushKV("creationtxid", sProperty.txid.GetHex());
    property_obj.pushKV("fixedissuance", sProperty.fixed);
    property_obj.pushKV("managedissuance", sProperty.manual);
}

void MetaDexObjectToJSON(const CMPMetaDEx& obj, UniValue& metadex_obj)
{
    bool propertyIdForSaleIsDivisible = isPropertyDivisible(obj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(obj.getDesProperty());
    // add data to JSON object
    metadex_obj.pushKV("address", obj.getAddr());
    metadex_obj.pushKV("txid", obj.getHash().GetHex());
    if (obj.getAction() == 4) metadex_obj.pushKV("ecosystem", isTestEcosystemProperty(obj.getProperty()) ? "test" : "main");
    metadex_obj.pushKV("propertyidforsale", (uint64_t) obj.getProperty());
    metadex_obj.pushKV("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible);
    metadex_obj.pushKV("amountforsale", FormatMP(obj.getProperty(), obj.getAmountForSale()));
    metadex_obj.pushKV("amountremaining", FormatMP(obj.getProperty(), obj.getAmountRemaining()));
    metadex_obj.pushKV("propertyiddesired", (uint64_t) obj.getDesProperty());
    metadex_obj.pushKV("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible);
    metadex_obj.pushKV("amountdesired", FormatMP(obj.getDesProperty(), obj.getAmountDesired()));
    metadex_obj.pushKV("amounttofill", FormatMP(obj.getDesProperty(), obj.getAmountToFill()));
    metadex_obj.pushKV("action", (int) obj.getAction());
    metadex_obj.pushKV("block", obj.getBlock());
    metadex_obj.pushKV("blocktime", obj.getBlockTime());
}

void MetaDexObjectsToJSON(std::vector<CMPMetaDEx>& vMetaDexObjs, UniValue& response)
{
    MetaDEx_compare compareByHeight;

    // sorts metadex objects based on block height and position in block
    std::sort (vMetaDexObjs.begin(), vMetaDexObjs.end(), compareByHeight);

    for (std::vector<CMPMetaDEx>::const_iterator it = vMetaDexObjs.begin(); it != vMetaDexObjs.end(); ++it) {
        UniValue metadex_obj(UniValue::VOBJ);
        MetaDexObjectToJSON(*it, metadex_obj);

        response.push_back(metadex_obj);
    }
}

bool BalanceToJSON(const std::string& address, uint32_t property, UniValue& balance_obj, bool divisible)
{
    // confirmed balance minus unconfirmed, spent amounts
    int64_t nAvailable = GetAvailableTokenBalance(address, property);
    int64_t nReserved = GetReservedTokenBalance(address, property);
    int64_t nFrozen = GetFrozenTokenBalance(address, property);

    if (divisible) {
        balance_obj.pushKV("balance", FormatDivisibleMP(nAvailable));
        balance_obj.pushKV("reserved", FormatDivisibleMP(nReserved));
        balance_obj.pushKV("frozen", FormatDivisibleMP(nFrozen));
    } else {
        balance_obj.pushKV("balance", FormatIndivisibleMP(nAvailable));
        balance_obj.pushKV("reserved", FormatIndivisibleMP(nReserved));
        balance_obj.pushKV("frozen", FormatIndivisibleMP(nFrozen));
    }

    return (nAvailable || nReserved || nFrozen);
}

// Obtains details of a fee distribution
static UniValue omni_getfeedistribution(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_getfeedistribution",
               "\nGet the details for a fee distribution.\n",
               {
                   {"distributionid", RPCArg::Type::NUM, RPCArg::Optional::NO, "(number, required) the distribution to obtain details for\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"distributionid\" : n,          (number) the distribution id\n"
                   "  \"propertyid\" : n,              (number) the property id of the distributed tokens\n"
                   "  \"block\" : n,                   (number) the block the distribution occurred\n"
                   "  \"amount\" : \"n.nnnnnnnn\",     (string) the amount that was distributed\n"
                   "  \"recipients\": [                (array of JSON objects) a list of recipients\n"
                   "    {\n"
                   "      \"address\" : \"address\",          (string) the address of the recipient\n"
                   "      \"amount\" : \"n.nnnnnnnn\"         (string) the amount of fees received by the recipient\n"
                   "    },\n"
                   "    ...\n"
                   "  ]\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getfeedistribution", "1")
                   + HelpExampleRpc("omni_getfeedistribution", "1")
               }
            }.ToString());

    int id = request.params[0].get_int();

    int block = 0;
    uint32_t propertyId = 0;
    int64_t total = 0;

    UniValue response(UniValue::VOBJ);

    bool found = pDbFeeHistory->GetDistributionData(id, &propertyId, &block, &total);
    if (!found) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Fee distribution ID does not exist");
    }
    response.pushKV("distributionid", id);
    response.pushKV("propertyid", (uint64_t)propertyId);
    response.pushKV("block", block);
    response.pushKV("amount", FormatMP(propertyId, total));
    UniValue recipients(UniValue::VARR);
    std::set<std::pair<std::string,int64_t> > sRecipients = pDbFeeHistory->GetFeeDistribution(id);
    bool divisible = isPropertyDivisible(propertyId);
    if (!sRecipients.empty()) {
        for (std::set<std::pair<std::string,int64_t> >::iterator it = sRecipients.begin(); it != sRecipients.end(); it++) {
            std::string address = (*it).first;
            int64_t amount = (*it).second;
            UniValue recipient(UniValue::VOBJ);
            recipient.pushKV("address", address);
            if (divisible) {
                recipient.pushKV("amount", FormatDivisibleMP(amount));
            } else {
                recipient.pushKV("amount", FormatIndivisibleMP(amount));
            }
            recipients.push_back(recipient);
        }
    }
    response.pushKV("recipients", recipients);
    return response;
}

// Obtains all fee distributions for a property
// TODO : Split off code to populate a fee distribution object into a separate function
static UniValue omni_getfeedistributions(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_getfeedistributions",
               "\nGet the details of all fee distributions for a property.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the property id to retrieve distributions for\n"},
               },
               RPCResult{
                   "[                       (array of JSON objects)\n"
                   "  {\n"
                   "    \"distributionid\" : n,          (number) the distribution id\n"
                   "    \"propertyid\" : n,              (number) the property id of the distributed tokens\n"
                   "    \"block\" : n,                   (number) the block the distribution occurred\n"
                   "    \"amount\" : \"n.nnnnnnnn\",     (string) the amount that was distributed\n"
                   "    \"recipients\": [                (array of JSON objects) a list of recipients\n"
                   "      {\n"
                   "        \"address\" : \"address\",          (string) the address of the recipient\n"
                   "        \"amount\" : \"n.nnnnnnnn\"         (string) the amount of fees received by the recipient\n"
                   "      },\n"
                   "      ...\n"
                   "  }\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getfeedistributions", "1")
                   + HelpExampleRpc("omni_getfeedistributions", "1")
               }
            }.ToString());

    uint32_t prop = ParsePropertyId(request.params[0]);
    RequireExistingProperty(prop);

    UniValue response(UniValue::VARR);

    std::set<int> sDistributions = pDbFeeHistory->GetDistributionsForProperty(prop);
    if (!sDistributions.empty()) {
        for (std::set<int>::iterator it = sDistributions.begin(); it != sDistributions.end(); it++) {
            int id = *it;
            int block = 0;
            uint32_t propertyId = 0;
            int64_t total = 0;
            UniValue responseObj(UniValue::VOBJ);
            bool found = pDbFeeHistory->GetDistributionData(id, &propertyId, &block, &total);
            if (!found) {
                PrintToLog("Fee History Error - Distribution data not found for distribution ID %d but it was included in GetDistributionsForProperty(prop %d)\n", id, prop);
                continue;
            }
            responseObj.pushKV("distributionid", id);
            responseObj.pushKV("propertyid", (uint64_t)propertyId);
            responseObj.pushKV("block", block);
            responseObj.pushKV("amount", FormatMP(propertyId, total));
            UniValue recipients(UniValue::VARR);
            std::set<std::pair<std::string,int64_t> > sRecipients = pDbFeeHistory->GetFeeDistribution(id);
            bool divisible = isPropertyDivisible(propertyId);
            if (!sRecipients.empty()) {
                for (std::set<std::pair<std::string,int64_t> >::iterator it = sRecipients.begin(); it != sRecipients.end(); it++) {
                    std::string address = (*it).first;
                    int64_t amount = (*it).second;
                    UniValue recipient(UniValue::VOBJ);
                    recipient.pushKV("address", address);
                    if (divisible) {
                        recipient.pushKV("amount", FormatDivisibleMP(amount));
                    } else {
                        recipient.pushKV("amount", FormatIndivisibleMP(amount));
                    }
                    recipients.push_back(recipient);
                }
            }
            responseObj.pushKV("recipients", recipients);
            response.push_back(responseObj);
        }
    }
    return response;
}

// Obtains the trigger value for fee distribution for a/all properties
static UniValue omni_getfeetrigger(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw runtime_error(
            RPCHelpMan{"omni_getfeetrigger",
               "\nReturns the amount of fees required in the cache to trigger distribution.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "filter the results on this property id\n"},
               },
               RPCResult{
                   "[                       (array of JSON objects)\n"
                   "  {\n"
                   "    \"propertyid\" : nnnnnnn,          (number) the property id\n"
                   "    \"feetrigger\" : \"n.nnnnnnnn\",   (string) the amount of fees required to trigger distribution\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getfeetrigger", "3")
                   + HelpExampleRpc("omni_getfeetrigger", "3")
               }
            }.ToString());

    uint32_t propertyId = 0;
    if (0 < request.params.size()) {
        propertyId = ParsePropertyId(request.params[0]);
    }

    if (propertyId > 0) {
        RequireExistingProperty(propertyId);
    }

    UniValue response(UniValue::VARR);

    for (uint8_t ecosystem = 1; ecosystem <= 2; ecosystem++) {
        uint32_t startPropertyId = (ecosystem == 1) ? 1 : TEST_ECO_PROPERTY_1;
        for (uint32_t itPropertyId = startPropertyId; itPropertyId < pDbSpInfo->peekNextSPID(ecosystem); itPropertyId++) {
            if (propertyId == 0 || propertyId == itPropertyId) {
                int64_t feeTrigger = pDbFeeCache->GetDistributionThreshold(itPropertyId);
                std::string strFeeTrigger = FormatMP(itPropertyId, feeTrigger);
                UniValue cacheObj(UniValue::VOBJ);
                cacheObj.pushKV("propertyid", (uint64_t)itPropertyId);
                cacheObj.pushKV("feetrigger", strFeeTrigger);
                response.push_back(cacheObj);
            }
        }
    }

    return response;
}

// Provides the fee share the wallet (or specific address) will receive from fee distributions
#ifdef ENABLE_WALLET
static UniValue omni_getfeeshare(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pWallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() > 2)
        throw runtime_error(
            RPCHelpMan{"omni_getfeeshare",
               "\nReturns the percentage share of fees distribution applied to the wallet (default) or address (if supplied).\n",
               {
                   {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "retrieve the fee share for the supplied address\n"},
                   {"ecosystem", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "the ecosystem to check the fee share (1 for main ecosystem, 2 for test ecosystem)\n"},
               },
               RPCResult{
                   "[                       (array of JSON objects)\n"
                   "  {\n"
                   "    \"address\" : nnnnnnn,          (number) the property id\n"
                   "    \"feeshare\" : \"n.nnnnnnnn\",   (string) the percentage of fees this address will receive based on current state\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getfeeshare", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" 1")
                   + HelpExampleRpc("omni_getfeeshare", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", 1")
               }
            }.ToString());

    std::string address;
    uint8_t ecosystem = 1;
    if (0 < request.params.size()) {
        if ("*" != request.params[0].get_str()) { //ParseAddressOrEmpty doesn't take wildcards
            address = ParseAddressOrEmpty(request.params[0]);
        } else {
            address = "*";
        }
    }
    if (1 < request.params.size()) {
        ecosystem = ParseEcosystem(request.params[1]);
    }

    UniValue response(UniValue::VARR);
    bool addObj = false;

    OwnerAddrType receiversSet;
    if (ecosystem == 1) {
        receiversSet = STO_GetReceivers("FEEDISTRIBUTION", OMNI_PROPERTY_MSC, COIN);
    } else {
        receiversSet = STO_GetReceivers("FEEDISTRIBUTION", OMNI_PROPERTY_TMSC, COIN);
    }

    for (OwnerAddrType::reverse_iterator it = receiversSet.rbegin(); it != receiversSet.rend(); ++it) {
        addObj = false;
        if (address.empty()) {
            if (IsMyAddress(it->second, pWallet.get())) {
                addObj = true;
            }
        } else if (address == it->second || address == "*") {
            addObj = true;
        }
        if (addObj) {
            UniValue feeShareObj(UniValue::VOBJ);
            // NOTE: using float here as this is a display value only which isn't an exact percentage and
            //       changes block to block (due to dev Omni) so high precision not required(?)
            double feeShare = (double(it->first) / double(COIN)) * (double)100;
            std::string strFeeShare = strprintf("%.4f", feeShare);
            strFeeShare += "%";
            feeShareObj.pushKV("address", it->second);
            feeShareObj.pushKV("feeshare", strFeeShare);
            response.push_back(feeShareObj);
        }
    }

    return response;
}
#endif

// Provides the current values of the fee cache
static UniValue omni_getfeecache(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw runtime_error(
            RPCHelpMan{"omni_getfeecache",
               "\nReturns the amount of fees cached for distribution.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "filter the results on this property id\n"},
               },
               RPCResult{
                   "[                       (array of JSON objects)\n"
                   "  {\n"
                   "    \"propertyid\" : nnnnnnn,          (number) the property id\n"
                   "    \"cachedfees\" : \"n.nnnnnnnn\",   (string) the amount of fees cached for this property\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getfeecache", "31")
                   + HelpExampleRpc("omni_getfeecache", "31")
               }
            }.ToString());

    uint32_t propertyId = 0;
    if (0 < request.params.size()) {
        propertyId = ParsePropertyId(request.params[0]);
    }

    if (propertyId > 0) {
        RequireExistingProperty(propertyId);
    }

    UniValue response(UniValue::VARR);

    for (uint8_t ecosystem = 1; ecosystem <= 2; ecosystem++) {
        uint32_t startPropertyId = (ecosystem == 1) ? 1 : TEST_ECO_PROPERTY_1;
        for (uint32_t itPropertyId = startPropertyId; itPropertyId < pDbSpInfo->peekNextSPID(ecosystem); itPropertyId++) {
            if (propertyId == 0 || propertyId == itPropertyId) {
                int64_t cachedFee = pDbFeeCache->GetCachedAmount(itPropertyId);
                if (cachedFee == 0) {
                    // filter empty results unless the call specifically requested this property
                    if (propertyId != itPropertyId) continue;
                }
                std::string strFee = FormatMP(itPropertyId, cachedFee);
                UniValue cacheObj(UniValue::VOBJ);
                cacheObj.pushKV("propertyid", (uint64_t)itPropertyId);
                cacheObj.pushKV("cachedfees", strFee);
                response.push_back(cacheObj);
            }
        }
    }

    return response;
}

// generate a list of seed blocks based on the data in LevelDB
static UniValue omni_getseedblocks(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            RPCHelpMan{"omni_getseedblocks",
               "\nReturns a list of blocks containing Omni transactions for use in seed block filtering.\n",
               {
                   {"startblock", RPCArg::Type::NUM, RPCArg::Optional::NO, "the first block to look for Omni transactions (inclusive)\n"},
                   {"endblock", RPCArg::Type::NUM, RPCArg::Optional::NO, "the last block to look for Omni transactions (inclusive)\n"},
               },
               RPCResult{
                   "[                     (array of numbers) a list of seed blocks\n"
                   "   nnnnnn,              (number) the block height of the seed block\n"
                   "   ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getseedblocks", "290000 300000")
                   + HelpExampleRpc("omni_getseedblocks", "290000, 300000")
               }
            }.ToString());

    int startHeight = request.params[0].get_int();
    int endHeight = request.params[1].get_int();

    RequireHeightInChain(startHeight);
    RequireHeightInChain(endHeight);

    UniValue response(UniValue::VARR);

    {
        LOCK(cs_tally);
        std::set<int> setSeedBlocks = pDbTransactionList->GetSeedBlocks(startHeight, endHeight);
        for (std::set<int>::const_iterator it = setSeedBlocks.begin(); it != setSeedBlocks.end(); ++it) {
            response.push_back(*it);
        }
    }

    return response;
}

// obtain the payload for a transaction
static UniValue omni_getpayload(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_getpayload",
               "\nGet the payload for an Omni transaction.\n",
               {
                   {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "the hash of the transaction to retrieve payload\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"payload\" : \"payloadmessage\",       (string) the decoded Omni payload message\n"
                   "  \"payloadsize\" : n                     (number) the size of the payload\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getpayload", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
                   + HelpExampleRpc("omni_getpayload", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
               }
            }.ToString());

    uint256 txid = ParseHashV(request.params[0], "txid");

    bool f_txindex_ready = false;
    if (g_txindex) {
        f_txindex_ready = g_txindex->BlockUntilSyncedToCurrentChain();
    }

    CTransactionRef tx;
    uint256 blockHash;
    if (!GetTransaction(txid, tx, Params().GetConsensus(), blockHash)) {
        if (!f_txindex_ready) {
            PopulateFailure(MP_TXINDEX_STILL_SYNCING);
        } else {
            PopulateFailure(MP_TX_NOT_FOUND);
        }
    }

    int blockTime = 0;
    int blockHeight = GetHeight();
    if (!blockHash.IsNull()) {
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (nullptr != pBlockIndex) {
            blockTime = pBlockIndex->nTime;
            blockHeight = pBlockIndex->nHeight;
        }
    }

    CMPTransaction mp_obj;
    int parseRC = ParseTransaction(*tx, blockHeight, 0, mp_obj, blockTime);
    if (parseRC < 0) PopulateFailure(MP_TX_IS_NOT_OMNI_PROTOCOL);

    UniValue payloadObj(UniValue::VOBJ);
    payloadObj.pushKV("payload", mp_obj.getPayload());
    payloadObj.pushKV("payloadsize", mp_obj.getPayloadSize());
    return payloadObj;
}

// determine whether to automatically commit transactions
#ifdef ENABLE_WALLET
static UniValue omni_setautocommit(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_setautocommit",
               "\nSets the global flag that determines whether transactions are automatically committed and broadcast.\n",
               {
                   {"flag", RPCArg::Type::BOOL, RPCArg::Optional::NO, "the flag\n"},
               },
               RPCResult{
                   "true|false              (boolean) the updated flag status\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_setautocommit", "false")
                   + HelpExampleRpc("omni_setautocommit", "false")
               }
            }.ToString());

    LOCK(cs_tally);

    autoCommit = request.params[0].get_bool();
    return autoCommit;
}
#endif

// display the tally map & the offer/accept list(s)
static UniValue mscrpc(const JSONRPCRequest& request)
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
#endif

    int extra = 0;
    int extra2 = 0, extra3 = 0;

    if (request.fHelp || request.params.size() > 3)
        throw runtime_error(
                RPCHelpMan{"mscrpc",
                   "\nReturns the number of blocks in the longest block chain.\n",
                   {},
                   RPCResult{
                       "n    (number) the current block count\n"
                   },
                   RPCExamples{
                       HelpExampleCli("mscrpc", "")
                       + HelpExampleRpc("mscrpc", "")
                   }
                }.ToString());

    if (0 < request.params.size()) extra = atoi(request.params[0].get_str());
    if (1 < request.params.size()) extra2 = atoi(request.params[1].get_str());
    if (2 < request.params.size()) extra3 = atoi(request.params[2].get_str());

    PrintToConsole("%s(extra=%d,extra2=%d,extra3=%d)\n", __FUNCTION__, extra, extra2, extra3);

    bool bDivisible = isPropertyDivisible(extra2);

    // various extra tests
    switch (extra) {
        case 0:
        {
            LOCK(cs_tally);
            int64_t total = 0;
            // display all balances
            for (std::unordered_map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
                PrintToConsole("%34s => ", my_it->first);
                total += (my_it->second).print(extra2, bDivisible);
            }
            PrintToConsole("total for property %d  = %X is %s\n", extra2, extra2, FormatDivisibleMP(total));
            break;
        }
        case 1:
        {
            LOCK(cs_tally);
            // display the whole CMPTxList (leveldb)
            pDbTransactionList->printAll();
            pDbTransactionList->printStats();
            break;
        }
        case 2:
        {
            LOCK(cs_tally);
            // display smart properties
            pDbSpInfo->printAll();
            break;
        }
        case 3:
        {
            LOCK(cs_tally);
            uint32_t id = 0;
            // for each address display all currencies it holds
            for (std::unordered_map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
                PrintToConsole("%34s => ", my_it->first);
                (my_it->second).print(extra2);
                (my_it->second).init();
                while (0 != (id = (my_it->second).next())) {
                    PrintToConsole("Id: %u=0x%X ", id, id);
                }
                PrintToConsole("\n");
            }
            break;
        }
        case 4:
        {
            LOCK(cs_tally);
            for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
                (it->second).print(it->first);
            }
            break;
        }
        case 5:
        {
            LOCK(cs_tally);
            PrintToConsole("isMPinBlockRange(%d,%d)=%s\n", extra2, extra3, pDbTransactionList->isMPinBlockRange(extra2, extra3, false) ? "YES" : "NO");
            break;
        }
        case 6:
        {
            LOCK(cs_tally);
            MetaDEx_debug_print(true, true);
            break;
        }
        case 7:
        {
            LOCK(cs_tally);
            // display the whole CMPTradeList (leveldb)
            pDbTradeList->printAll();
            pDbTradeList->printStats();
            break;
        }
        case 8:
        {
            LOCK(cs_tally);
            // display the STO receive list
            pDbStoList->printAll();
            pDbStoList->printStats();
            break;
        }
        case 9:
        {
            PrintToConsole("Locking cs_tally for %d milliseconds..\n", extra2);
            LOCK(cs_tally);
            MilliSleep(extra2);
            PrintToConsole("Unlocking cs_tally now\n");
            break;
        }
        case 10:
        {
            PrintToConsole("Locking cs_main for %d milliseconds..\n", extra2);
            LOCK(cs_main);
            MilliSleep(extra2);
            PrintToConsole("Unlocking cs_main now\n");
            break;
        }
#ifdef ENABLE_WALLET
        case 11:
        {
            PrintToConsole("Locking pwallet->cs_wallet for %d milliseconds..\n", extra2);
            LOCK(pwallet->cs_wallet);
            MilliSleep(extra2);
            PrintToConsole("Unlocking pwallet->cs_wallet now\n");
            break;
        }
#endif
        case 14:
        {
            LOCK(cs_tally);
            pDbFeeCache->printAll();
            pDbFeeCache->printStats();

            int64_t a = 1000;
            int64_t b = 1999;
            int64_t c = 2001;
            int64_t d = 20001;
            int64_t e = 19999;

            int64_t z = 2000;

            int64_t aa = a/z;
            int64_t bb = b/z;
            int64_t cc = c/z;
            int64_t dd = d/z;
            int64_t ee = e/z;

            PrintToConsole("%d / %d = %d\n",a,z,aa);
            PrintToConsole("%d / %d = %d\n",b,z,bb);
            PrintToConsole("%d / %d = %d\n",c,z,cc);
            PrintToConsole("%d / %d = %d\n",d,z,dd);
            PrintToConsole("%d / %d = %d\n",e,z,ee);

            break;
        }
        default:
            break;
    }

    return GetHeight();
}

// display an MP balance via RPC
static UniValue omni_getbalance(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            RPCHelpMan{"omni_getbalance",
               "\nReturns the token balance for a given address and property.\n",
               {
                   {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "the address\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the property identifier\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"balance\" : \"n.nnnnnnnn\",   (string) the available balance of the address\n"
                   "  \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
                   "  \"frozen\" : \"n.nnnnnnnn\"     (string) the amount frozen by the issuer (applies to managed properties only)\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getbalance", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" 1")
                   + HelpExampleRpc("omni_getbalance", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", 1")
               }
            }.ToString());

    std::string address = ParseAddress(request.params[0]);
    uint32_t propertyId = ParsePropertyId(request.params[1]);

    RequireExistingProperty(propertyId);

    UniValue balanceObj(UniValue::VOBJ);
    BalanceToJSON(address, propertyId, balanceObj, isPropertyDivisible(propertyId));

    return balanceObj;
}

static UniValue omni_getallbalancesforid(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_getallbalancesforid",
               "\nReturns a list of token balances for a given currency or property identifier.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the property identifier\n"},
               },
               RPCResult{
                   "[                           (array of JSON objects)\n"
                   "  {\n"
                   "    \"address\" : \"address\",      (string) the address\n"
                   "    \"balance\" : \"n.nnnnnnnn\",   (string) the available balance of the address\n"
                   "    \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
                   "    \"frozen\" : \"n.nnnnnnnn\"     (string) the amount frozen by the issuer (applies to managed properties only)\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getallbalancesforid", "1")
                   + HelpExampleRpc("omni_getallbalancesforid", "1")
               }
            }.ToString());

    uint32_t propertyId = ParsePropertyId(request.params[0]);

    RequireExistingProperty(propertyId);

    UniValue response(UniValue::VARR);
    bool isDivisible = isPropertyDivisible(propertyId); // we want to check this BEFORE the loop

    LOCK(cs_tally);

    for (std::unordered_map<std::string, CMPTally>::iterator it = mp_tally_map.begin(); it != mp_tally_map.end(); ++it) {
        uint32_t id = 0;
        bool includeAddress = false;
        std::string address = it->first;
        (it->second).init();
        while (0 != (id = (it->second).next())) {
            if (id == propertyId) {
                includeAddress = true;
                break;
            }
        }
        if (!includeAddress) {
            continue; // ignore this address, has never transacted in this propertyId
        }
        UniValue balanceObj(UniValue::VOBJ);
        balanceObj.pushKV("address", address);
        bool nonEmptyBalance = BalanceToJSON(address, propertyId, balanceObj, isDivisible);

        if (nonEmptyBalance) {
            response.push_back(balanceObj);
        }
    }

    return response;
}

static UniValue omni_getallbalancesforaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_getallbalancesforaddress",
               "\nReturns a list of all token balances for a given address.\n",
               {
                   {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "the address\n"},
               },
               RPCResult{
                   "[                           (array of JSON objects)\n"
                   "  {\n"
                   "    \"propertyid\" : n,           (number) the property identifier\n"
                   "    \"name\" : \"name\",            (string) the name of the property\n"
                   "    \"balance\" : \"n.nnnnnnnn\",   (string) the available balance of the address\n"
                   "    \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
                   "    \"frozen\" : \"n.nnnnnnnn\"     (string) the amount frozen by the issuer (applies to managed properties only)\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getallbalancesforaddress", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\"")
                   + HelpExampleRpc("omni_getallbalancesforaddress", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\"")
               }
            }.ToString());

    std::string address = ParseAddress(request.params[0]);

    UniValue response(UniValue::VARR);

    LOCK(cs_tally);

    CMPTally* addressTally = getTally(address);

    if (nullptr == addressTally) { // addressTally object does not exist
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Address not found");
    }

    addressTally->init();

    uint32_t propertyId = 0;
    while (0 != (propertyId = addressTally->next())) {
        CMPSPInfo::Entry property;
        if (!pDbSpInfo->getSP(propertyId, property)) {
            continue;
        }

        UniValue balanceObj(UniValue::VOBJ);
        balanceObj.pushKV("propertyid", (uint64_t) propertyId);
        balanceObj.pushKV("name", property.name);

        bool nonEmptyBalance = BalanceToJSON(address, propertyId, balanceObj, property.isDivisible());

        if (nonEmptyBalance) {
            response.push_back(balanceObj);
        }
    }

    return response;
}

/** Returns all addresses that may be mine. */
static std::set<std::string> getWalletAddresses(const JSONRPCRequest& request, bool fIncludeWatchOnly)
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
#endif

    std::set<std::string> result;

#ifdef ENABLE_WALLET
    LOCK(pwallet->cs_wallet);

    for(const auto& item : pwallet->mapAddressBook) {
        const CTxDestination& address = item.first;
        isminetype iIsMine = IsMine(*pwallet, address);

        if (iIsMine == ISMINE_SPENDABLE || (fIncludeWatchOnly && iIsMine != ISMINE_NO)) {
            result.insert(EncodeDestination(address));
        }
    }
#endif

    return result;
}

#ifdef ENABLE_WALLET
static UniValue omni_getwalletbalances(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();

    if (request.fHelp || request.params.size() > 1)
        throw runtime_error(
            RPCHelpMan{"omni_getwalletbalances",
               "\nReturns a list of the total token balances of the whole wallet.\n",
               {
                   {"includewatchonly", RPCArg::Type::BOOL, /* default */ "false", "include balances of watchonly addresses\n"},
               },
               RPCResult{
                   "[                           (array of JSON objects)\n"
                   "  {\n"
                   "    \"propertyid\" : n,         (number) the property identifier\n"
                   "    \"name\" : \"name\",            (string) the name of the token\n"
                   "    \"balance\" : \"n.nnnnnnnn\",   (string) the total available balance for the token\n"
                   "    \"reserved\" : \"n.nnnnnnnn\"   (string) the total amount reserved by sell offers and accepts\n"
                   "    \"frozen\" : \"n.nnnnnnnn\"     (string) the total amount frozen by the issuer (applies to managed properties only)\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getwalletbalances", "")
                   + HelpExampleRpc("omni_getwalletbalances", "")
               }
            }.ToString());

    bool fIncludeWatchOnly = false;
    if (request.params.size() > 0) {
        fIncludeWatchOnly = request.params[0].get_bool();
    }

    UniValue response(UniValue::VARR);

    if (!pwallet) {
        return response;
    }

    std::set<std::string> addresses = getWalletAddresses(request, fIncludeWatchOnly);
    std::map<uint32_t, std::tuple<int64_t, int64_t, int64_t>> balances;

    LOCK(cs_tally);
    for(const std::string& address : addresses) {
        CMPTally* addressTally = getTally(address);
        if (nullptr == addressTally) {
            continue; // address doesn't have tokens
        }

        uint32_t propertyId = 0;
        addressTally->init();

        while (0 != (propertyId = addressTally->next())) {
            int64_t nAvailable = GetAvailableTokenBalance(address, propertyId);
            int64_t nReserved = GetReservedTokenBalance(address, propertyId);
            int64_t nFrozen = GetFrozenTokenBalance(address, propertyId);

            if (!nAvailable && !nReserved && !nFrozen) {
                continue;
            }

            std::tuple<int64_t, int64_t, int64_t> currentBalance{0, 0, 0};
            if (balances.find(propertyId) != balances.end()) {
                currentBalance = balances[propertyId];
            }

            currentBalance = std::make_tuple(
                    std::get<0>(currentBalance) + nAvailable,
                    std::get<1>(currentBalance) + nReserved,
                    std::get<2>(currentBalance) + nFrozen
            );

            balances[propertyId] = currentBalance;
        }
    }

    for (auto const& item : balances) {
        uint32_t propertyId = item.first;
        std::tuple<int64_t, int64_t, int64_t> balance = item.second;

        CMPSPInfo::Entry property;
        if (!pDbSpInfo->getSP(propertyId, property)) {
            continue; // token wasn't found in the DB
        }

        int64_t nAvailable = std::get<0>(balance);
        int64_t nReserved = std::get<1>(balance);
        int64_t nFrozen = std::get<2>(balance);

        UniValue objBalance(UniValue::VOBJ);
        objBalance.pushKV("propertyid", (uint64_t) propertyId);
        objBalance.pushKV("name", property.name);

        if (property.isDivisible()) {
            objBalance.pushKV("balance", FormatDivisibleMP(nAvailable));
            objBalance.pushKV("reserved", FormatDivisibleMP(nReserved));
            objBalance.pushKV("frozen", FormatDivisibleMP(nFrozen));
        } else {
            objBalance.pushKV("balance", FormatIndivisibleMP(nAvailable));
            objBalance.pushKV("reserved", FormatIndivisibleMP(nReserved));
            objBalance.pushKV("frozen", FormatIndivisibleMP(nFrozen));
        }

        response.push_back(objBalance);
    }

    return response;
}

static UniValue omni_getwalletaddressbalances(const JSONRPCRequest& request)
{

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();

    if (request.fHelp || request.params.size() > 1)
        throw runtime_error(
            RPCHelpMan{"omni_getwalletaddressbalances",
               "\nReturns a list of all token balances for every wallet address.\n",
               {
                   {"includewatchonly", RPCArg::Type::BOOL, /*default */ "false", "include balances of watchonly addresses\n"},
               },
               RPCResult{
                   "[                           (array of JSON objects)\n"
                   "  {\n"
                   "    \"address\" : \"address\",      (string) the address linked to the following balances\n"
                   "    \"balances\" :\n"
                   "    [\n"
                   "      {\n"
                   "        \"propertyid\" : n,         (number) the property identifier\n"
                   "        \"name\" : \"name\",            (string) the name of the token\n"
                   "        \"balance\" : \"n.nnnnnnnn\",   (string) the available balance for the token\n"
                   "        \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
                   "        \"frozen\" : \"n.nnnnnnnn\"     (string) the amount frozen by the issuer (applies to managed properties only)\n"
                   "      },\n"
                   "      ...\n"
                   "    ]\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getwalletaddressbalances", "")
                   + HelpExampleRpc("omni_getwalletaddressbalances", "")
               }
            }.ToString());

    bool fIncludeWatchOnly = false;
    if (request.params.size() > 0) {
        fIncludeWatchOnly = request.params[0].get_bool();
    }

    UniValue response(UniValue::VARR);

    if (!pwallet) {
        return response;
    }

    std::set<std::string> addresses = getWalletAddresses(request, fIncludeWatchOnly);

    LOCK(cs_tally);
    for(const std::string& address : addresses) {
        CMPTally* addressTally = getTally(address);
        if (nullptr == addressTally) {
            continue; // address doesn't have tokens
        }

        UniValue arrBalances(UniValue::VARR);
        uint32_t propertyId = 0;
        addressTally->init();

        while (0 != (propertyId = addressTally->next())) {
            CMPSPInfo::Entry property;
            if (!pDbSpInfo->getSP(propertyId, property)) {
                continue; // token wasn't found in the DB
            }

            UniValue objBalance(UniValue::VOBJ);
            objBalance.pushKV("propertyid", (uint64_t) propertyId);
            objBalance.pushKV("name", property.name);

            bool nonEmptyBalance = BalanceToJSON(address, propertyId, objBalance, property.isDivisible());

            if (nonEmptyBalance) {
                arrBalances.push_back(objBalance);
            }
        }

        if (arrBalances.size() > 0) {
            UniValue objEntry(UniValue::VOBJ);
            objEntry.pushKV("address", address);
            objEntry.pushKV("balances", arrBalances);
            response.push_back(objEntry);
        }
    }

    return response;
}
#endif

static UniValue omni_getproperty(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_getproperty",
               "\nReturns details for about the tokens or smart property to lookup.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens or property\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"propertyid\" : n,                (number) the identifier\n"
                   "  \"name\" : \"name\",                 (string) the name of the tokens\n"
                   "  \"category\" : \"category\",         (string) the category used for the tokens\n"
                   "  \"subcategory\" : \"subcategory\",   (string) the subcategory used for the tokens\n"
                   "  \"data\" : \"information\",          (string) additional information or a description\n"
                   "  \"url\" : \"uri\",                   (string) an URI, for example pointing to a website\n"
                   "  \"divisible\" : true|false,        (boolean) whether the tokens are divisible\n"
                   "  \"issuer\" : \"address\",            (string) the Qitcoin address of the issuer on record\n"
                   "  \"creationtxid\" : \"hash\",         (string) the hex-encoded creation transaction hash\n"
                   "  \"fixedissuance\" : true|false,    (boolean) whether the token supply is fixed\n"
                   "  \"managedissuance\" : true|false,    (boolean) whether the token supply is managed\n"
                   "  \"freezingenabled\" : true|false,    (boolean) whether freezing is enabled for the property (managed properties only)\n"
                   "  \"totaltokens\" : \"n.nnnnnnnn\"     (string) the total number of tokens in existence\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getproperty", "3")
                   + HelpExampleRpc("omni_getproperty", "3")
               }
            }.ToString());

    uint32_t propertyId = ParsePropertyId(request.params[0]);

    RequireExistingProperty(propertyId);

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (!pDbSpInfo->getSP(propertyId, sp)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
        }
    }
    int64_t nTotalTokens = getTotalTokens(propertyId);
    std::string strCreationHash = sp.txid.GetHex();
    std::string strTotalTokens = FormatMP(propertyId, nTotalTokens);

    UniValue response(UniValue::VOBJ);
    response.pushKV("propertyid", (uint64_t) propertyId);
    PropertyToJSON(sp, response); // name, category, subcategory, ...
    if (sp.manual) {
        int currentBlock = GetHeight();
        LOCK(cs_tally);
        response.pushKV("freezingenabled", isFreezingEnabled(propertyId, currentBlock));
    }
    response.pushKV("totaltokens", strTotalTokens);

    return response;
}

static UniValue omni_listproperties(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw runtime_error(
            RPCHelpMan{"omni_listproperties",
               "\nLists all tokens or smart properties. To get the total number of tokens, please use omni_getproperty.\n",
               {},
               RPCResult{
                   "[                                (array of JSON objects)\n"
                   "  {\n"
                   "    \"propertyid\" : n,                (number) the identifier of the tokens\n"
                   "    \"name\" : \"name\",                 (string) the name of the tokens\n"
                   "    \"category\" : \"category\",         (string) the category used for the tokens\n"
                   "    \"subcategory\" : \"subcategory\",   (string) the subcategory used for the tokens\n"
                   "    \"data\" : \"information\",          (string) additional information or a description\n"
                   "    \"url\" : \"uri\",                   (string) an URI, for example pointing to a website\n"
                   "    \"divisible\" : true|false,        (boolean) whether the tokens are divisible\n"
                   "    \"issuer\" : \"address\",            (string) the Bitcoin address of the issuer on record\n"
                   "    \"creationtxid\" : \"hash\",         (string) the hex-encoded creation transaction hash\n"
                   "    \"fixedissuance\" : true|false,    (boolean) whether the token supply is fixed\n"
                   "    \"managedissuance\" : true|false   (boolean) whether the token supply is managed\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_listproperties", "")
                   + HelpExampleRpc("omni_listproperties", "")
               }
            }.ToString());

    UniValue response(UniValue::VARR);

    LOCK(cs_tally);

    uint32_t nextSPID = pDbSpInfo->peekNextSPID(1);
    for (uint32_t propertyId = 1; propertyId < nextSPID; propertyId++) {
        CMPSPInfo::Entry sp;
        if (pDbSpInfo->getSP(propertyId, sp)) {
            UniValue propertyObj(UniValue::VOBJ);
            propertyObj.pushKV("propertyid", (uint64_t) propertyId);
            PropertyToJSON(sp, propertyObj); // name, category, subcategory, ...

            response.push_back(propertyObj);
        }
    }

    uint32_t nextTestSPID = pDbSpInfo->peekNextSPID(2);
    for (uint32_t propertyId = TEST_ECO_PROPERTY_1; propertyId < nextTestSPID; propertyId++) {
        CMPSPInfo::Entry sp;
        if (pDbSpInfo->getSP(propertyId, sp)) {
            UniValue propertyObj(UniValue::VOBJ);
            propertyObj.pushKV("propertyid", (uint64_t) propertyId);
            PropertyToJSON(sp, propertyObj); // name, category, subcategory, ...

            response.push_back(propertyObj);
        }
    }

    return response;
}

static UniValue omni_getcrowdsale(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            RPCHelpMan{"omni_getcrowdsale",
               "\nReturns information about a crowdsale.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the crowdsale\n"},
                   {"verbose", RPCArg::Type::BOOL, /* default */ "false", "list crowdsale participants\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"propertyid\" : n,                     (number) the identifier of the crowdsale\n"
                   "  \"name\" : \"name\",                      (string) the name of the tokens issued via the crowdsale\n"
                   "  \"active\" : true|false,                (boolean) whether the crowdsale is still active\n"
                   "  \"issuer\" : \"address\",                 (string) the Qitcoin address of the issuer on record\n"
                   "  \"propertyiddesired\" : n,              (number) the identifier of the tokens eligible to participate in the crowdsale\n"
                   "  \"tokensperunit\" : \"n.nnnnnnnn\",       (string) the amount of tokens granted per unit invested in the crowdsale\n"
                   "  \"earlybonus\" : n,                     (number) an early bird bonus for participants in percent per week\n"
                   "  \"percenttoissuer\" : n,                (number) a percentage of tokens that will be granted to the issuer\n"
                   "  \"starttime\" : nnnnnnnnnn,             (number) the start time of the of the crowdsale as Unix timestamp\n"
                   "  \"deadline\" : nnnnnnnnnn,              (number) the deadline of the crowdsale as Unix timestamp\n"
                   "  \"amountraised\" : \"n.nnnnnnnn\",        (string) the amount of tokens invested by participants\n"
                   "  \"tokensissued\" : \"n.nnnnnnnn\",        (string) the total number of tokens issued via the crowdsale\n"
                   "  \"issuerbonustokens\" : \"n.nnnnnnnn\",   (string) the amount of tokens granted to the issuer as bonus\n"
                   "  \"addedissuertokens\" : \"n.nnnnnnnn\",   (string) the amount of issuer bonus tokens not yet emitted\n"
                   "  \"closedearly\" : true|false,           (boolean) whether the crowdsale ended early (if not active)\n"
                   "  \"maxtokens\" : true|false,             (boolean) whether the crowdsale ended early due to reaching the limit of max. issuable tokens (if not active)\n"
                   "  \"endedtime\" : nnnnnnnnnn,             (number) the time when the crowdsale ended (if closed early)\n"
                   "  \"closetx\" : \"hash\",                   (string) the hex-encoded hash of the transaction that closed the crowdsale (if closed manually)\n"
                   "  \"participanttransactions\": [          (array of JSON objects) a list of crowdsale participations (if verbose=true)\n"
                   "    {\n"
                   "      \"txid\" : \"hash\",                      (string) the hex-encoded hash of participation transaction\n"
                   "      \"amountsent\" : \"n.nnnnnnnn\",          (string) the amount of tokens invested by the participant\n"
                   "      \"participanttokens\" : \"n.nnnnnnnn\",   (string) the tokens granted to the participant\n"
                   "      \"issuertokens\" : \"n.nnnnnnnn\"         (string) the tokens granted to the issuer as bonus\n"
                   "    },\n"
                   "    ...\n"
                   "  ]\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getcrowdsale", "3 true")
                   + HelpExampleRpc("omni_getcrowdsale", "3, true")
               }
            }.ToString());

    uint32_t propertyId = ParsePropertyId(request.params[0]);
    bool showVerbose = (request.params.size() > 1) ? request.params[1].get_bool() : false;

    RequireExistingProperty(propertyId);
    RequireCrowdsale(propertyId);

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (!pDbSpInfo->getSP(propertyId, sp)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
        }
    }

    const uint256& creationHash = sp.txid;

    bool f_txindex_ready = false;
    if (g_txindex) {
        f_txindex_ready = g_txindex->BlockUntilSyncedToCurrentChain();
    }

    CTransactionRef tx;
    uint256 hashBlock;
    if (!GetTransaction(creationHash, tx, Params().GetConsensus(), hashBlock)) {
        if (!f_txindex_ready) {
            PopulateFailure(MP_TXINDEX_STILL_SYNCING);
        } else {
            PopulateFailure(MP_TX_NOT_FOUND);
        }
    }

    UniValue response(UniValue::VOBJ);
    bool active = isCrowdsaleActive(propertyId);
    std::map<uint256, std::vector<int64_t> > database;

    if (active) {
        bool crowdFound = false;

        LOCK(cs_tally);

        for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
            const CMPCrowd& crowd = it->second;
            if (propertyId == crowd.getPropertyId()) {
                crowdFound = true;
                database = crowd.getDatabase();
            }
        }
        if (!crowdFound) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Crowdsale is flagged active but cannot be retrieved");
        }
    } else {
        database = sp.historicalData;
    }

    int64_t tokensIssued = getTotalTokens(propertyId);
    const std::string& txidClosed = sp.txid_close.GetHex();

    int64_t startTime = -1;
    if (!hashBlock.IsNull() && GetBlockIndex(hashBlock)) {
        startTime = GetBlockIndex(hashBlock)->nTime;
    }

    // note the database is already deserialized here and there is minimal performance penalty to iterate recipients to calculate amountRaised
    int64_t amountRaised = 0;
    int64_t amountIssuerTokens = 0;
    uint16_t propertyIdType = isPropertyDivisible(propertyId) ? MSC_PROPERTY_TYPE_DIVISIBLE : MSC_PROPERTY_TYPE_INDIVISIBLE;
    uint16_t desiredIdType = isPropertyDivisible(sp.property_desired) ? MSC_PROPERTY_TYPE_DIVISIBLE : MSC_PROPERTY_TYPE_INDIVISIBLE;
    std::map<std::string, UniValue> sortMap;
    for (std::map<uint256, std::vector<int64_t> >::const_iterator it = database.begin(); it != database.end(); it++) {
        UniValue participanttx(UniValue::VOBJ);
        std::string txid = it->first.GetHex();
        amountRaised += it->second.at(0);
        amountIssuerTokens += it->second.at(3);
        participanttx.pushKV("txid", txid);
        participanttx.pushKV("amountsent", FormatByType(it->second.at(0), desiredIdType));
        participanttx.pushKV("participanttokens", FormatByType(it->second.at(2), propertyIdType));
        participanttx.pushKV("issuertokens", FormatByType(it->second.at(3), propertyIdType));
        std::string sortKey = strprintf("%d-%s", it->second.at(1), txid);
        sortMap.insert(std::make_pair(sortKey, participanttx));
    }

    response.pushKV("propertyid", (uint64_t) propertyId);
    response.pushKV("name", sp.name);
    response.pushKV("active", active);
    response.pushKV("issuer", sp.issuer);
    response.pushKV("propertyiddesired", (uint64_t) sp.property_desired);
    response.pushKV("tokensperunit", FormatMP(propertyId, sp.num_tokens));
    response.pushKV("earlybonus", sp.early_bird);
    response.pushKV("percenttoissuer", sp.percentage);
    response.pushKV("starttime", startTime);
    response.pushKV("deadline", sp.deadline);
    response.pushKV("amountraised", FormatMP(sp.property_desired, amountRaised));
    response.pushKV("tokensissued", FormatMP(propertyId, tokensIssued));
    response.pushKV("issuerbonustokens", FormatMP(propertyId, amountIssuerTokens));
    response.pushKV("addedissuertokens", FormatMP(propertyId, sp.missedTokens));

    // TODO: return fields every time?
    if (!active) response.pushKV("closedearly", sp.close_early);
    if (!active) response.pushKV("maxtokens", sp.max_tokens);
    if (sp.close_early) response.pushKV("endedtime", sp.timeclosed);
    if (sp.close_early && !sp.max_tokens) response.pushKV("closetx", txidClosed);

    if (showVerbose) {
        UniValue participanttxs(UniValue::VARR);
        for (std::map<std::string, UniValue>::iterator it = sortMap.begin(); it != sortMap.end(); ++it) {
            participanttxs.push_back(it->second);
        }
        response.pushKV("participanttransactions", participanttxs);
    }

    return response;
}

static UniValue omni_getactivecrowdsales(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw runtime_error(
            RPCHelpMan{"omni_getactivecrowdsales",
               "\nLists currently active crowdsales.\n",
               {},
               RPCResult{
                   "[                                 (array of JSON objects)\n"
                   "  {\n"
                   "    \"propertyid\" : n,                 (number) the identifier of the crowdsale\n"
                   "    \"name\" : \"name\",                  (string) the name of the tokens issued via the crowdsale\n"
                   "    \"issuer\" : \"address\",             (string) the Qitcoin address of the issuer on record\n"
                   "    \"propertyiddesired\" : n,          (number) the identifier of the tokens eligible to participate in the crowdsale\n"
                   "    \"tokensperunit\" : \"n.nnnnnnnn\",   (string) the amount of tokens granted per unit invested in the crowdsale\n"
                   "    \"earlybonus\" : n,                 (number) an early bird bonus for participants in percent per week\n"
                   "    \"percenttoissuer\" : n,            (number) a percentage of tokens that will be granted to the issuer\n"
                   "    \"starttime\" : nnnnnnnnnn,         (number) the start time of the of the crowdsale as Unix timestamp\n"
                   "    \"deadline\" : nnnnnnnnnn           (number) the deadline of the crowdsale as Unix timestamp\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getactivecrowdsales", "")
                   + HelpExampleRpc("omni_getactivecrowdsales", "")
               }
            }.ToString());

    UniValue response(UniValue::VARR);

    LOCK2(cs_main, cs_tally);

    for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
        const CMPCrowd& crowd = it->second;
        uint32_t propertyId = crowd.getPropertyId();

        CMPSPInfo::Entry sp;
        if (!pDbSpInfo->getSP(propertyId, sp)) {
            continue;
        }

        const uint256& creationHash = sp.txid;

        bool f_txindex_ready = false;
        if (g_txindex) {
            f_txindex_ready = g_txindex->BlockUntilSyncedToCurrentChain();
        }

        CTransactionRef tx;
        uint256 hashBlock;
        if (!GetTransaction(creationHash, tx, Params().GetConsensus(), hashBlock)) {
            if (!f_txindex_ready) {
                PopulateFailure(MP_TXINDEX_STILL_SYNCING);
            } else {
                PopulateFailure(MP_TX_NOT_FOUND);
            }
        }

        int64_t startTime = -1;
        if (!hashBlock.IsNull() && GetBlockIndex(hashBlock)) {
            startTime = GetBlockIndex(hashBlock)->nTime;
        }

        UniValue responseObj(UniValue::VOBJ);
        responseObj.pushKV("propertyid", (uint64_t) propertyId);
        responseObj.pushKV("name", sp.name);
        responseObj.pushKV("issuer", sp.issuer);
        responseObj.pushKV("propertyiddesired", (uint64_t) sp.property_desired);
        responseObj.pushKV("tokensperunit", FormatMP(propertyId, sp.num_tokens));
        responseObj.pushKV("earlybonus", sp.early_bird);
        responseObj.pushKV("percenttoissuer", sp.percentage);
        responseObj.pushKV("starttime", startTime);
        responseObj.pushKV("deadline", sp.deadline);
        response.push_back(responseObj);
    }

    return response;
}

static UniValue omni_getgrants(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_getgrants",
               "\nReturns information about granted and revoked units of managed tokens.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the managed tokens to lookup\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"propertyid\" : n,               (number) the identifier of the managed tokens\n"
                   "  \"name\" : \"name\",                (string) the name of the tokens\n"
                   "  \"issuer\" : \"address\",           (string) the Qitcoin address of the issuer on record\n"
                   "  \"creationtxid\" : \"hash\",        (string) the hex-encoded creation transaction hash\n"
                   "  \"totaltokens\" : \"n.nnnnnnnn\",   (string) the total number of tokens in existence\n"
                   "  \"issuances\": [                  (array of JSON objects) a list of the granted and revoked tokens\n"
                   "    {\n"
                   "      \"txid\" : \"hash\",                (string) the hash of the transaction that granted tokens\n"
                   "      \"grant\" : \"n.nnnnnnnn\"          (string) the number of tokens granted by this transaction\n"
                   "    },\n"
                   "    {\n"
                   "      \"txid\" : \"hash\",                (string) the hash of the transaction that revoked tokens\n"
                   "      \"grant\" : \"n.nnnnnnnn\"          (string) the number of tokens revoked by this transaction\n"
                   "    },\n"
                   "    ...\n"
                   "  ]\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getgrants", "31")
                   + HelpExampleRpc("omni_getgrants", "31")
               }
            }.ToString());

    uint32_t propertyId = ParsePropertyId(request.params[0]);

    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (false == pDbSpInfo->getSP(propertyId, sp)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
        }
    }
    UniValue response(UniValue::VOBJ);
    const uint256& creationHash = sp.txid;
    int64_t totalTokens = getTotalTokens(propertyId);

    // TODO: sort by height?

    UniValue issuancetxs(UniValue::VARR);
    std::map<uint256, std::vector<int64_t> >::const_iterator it;
    for (it = sp.historicalData.begin(); it != sp.historicalData.end(); it++) {
        const std::string& txid = it->first.GetHex();
        int64_t grantedTokens = it->second.at(0);
        int64_t revokedTokens = it->second.at(1);

        if (grantedTokens > 0) {
            UniValue granttx(UniValue::VOBJ);
            granttx.pushKV("txid", txid);
            granttx.pushKV("grant", FormatMP(propertyId, grantedTokens));
            issuancetxs.push_back(granttx);
        }

        if (revokedTokens > 0) {
            UniValue revoketx(UniValue::VOBJ);
            revoketx.pushKV("txid", txid);
            revoketx.pushKV("revoke", FormatMP(propertyId, revokedTokens));
            issuancetxs.push_back(revoketx);
        }
    }

    response.pushKV("propertyid", (uint64_t) propertyId);
    response.pushKV("name", sp.name);
    response.pushKV("issuer", sp.issuer);
    response.pushKV("creationtxid", creationHash.GetHex());
    response.pushKV("totaltokens", FormatMP(propertyId, totalTokens));
    response.pushKV("issuances", issuancetxs);

    return response;
}

static UniValue omni_getorderbook(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            RPCHelpMan{"omni_getorderbook",
               "\nList active offers on the distributed token exchange.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "filter orders by property identifier for sale\n"},
                   {"propertyiddesired", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "filter orders by property identifier desired\n"},
               },
               RPCResult{
                   "[                                              (array of JSON objects)\n"
                   "  {\n"
                   "    \"address\" : \"address\",                         (string) the Qitcoin address of the trader\n"
                   "    \"txid\" : \"hash\",                               (string) the hex-encoded hash of the transaction of the order\n"
                   "    \"ecosystem\" : \"main\"|\"test\",                   (string) the ecosytem in which the order was made (if \"cancel-ecosystem\")\n"
                   "    \"propertyidforsale\" : n,                       (number) the identifier of the tokens put up for sale\n"
                   "    \"propertyidforsaleisdivisible\" : true|false,   (boolean) whether the tokens for sale are divisible\n"
                   "    \"amountforsale\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially offered\n"
                   "    \"amountremaining\" : \"n.nnnnnnnn\",              (string) the amount of tokens still up for sale\n"
                   "    \"propertyiddesired\" : n,                       (number) the identifier of the tokens desired in exchange\n"
                   "    \"propertyiddesiredisdivisible\" : true|false,   (boolean) whether the desired tokens are divisible\n"
                   "    \"amountdesired\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially desired\n"
                   "    \"amounttofill\" : \"n.nnnnnnnn\",                 (string) the amount of tokens still needed to fill the offer completely\n"
                   "    \"action\" : n,                                  (number) the action of the transaction: (1) \"trade\", (2) \"cancel-price\", (3) \"cancel-pair\", (4) \"cancel-ecosystem\"\n"
                   "    \"block\" : nnnnnn,                              (number) the index of the block that contains the transaction\n"
                   "    \"blocktime\" : nnnnnnnnnn                       (number) the timestamp of the block that contains the transaction\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getorderbook", "2")
                   + HelpExampleRpc("omni_getorderbook", "2")
               }
            }.ToString());

    bool filterDesired = (request.params.size() > 1);
    uint32_t propertyIdForSale = ParsePropertyId(request.params[0]);
    uint32_t propertyIdDesired = 0;

    RequireExistingProperty(propertyIdForSale);

    if (filterDesired) {
        propertyIdDesired = ParsePropertyId(request.params[1]);

        RequireExistingProperty(propertyIdDesired);
        RequireSameEcosystem(propertyIdForSale, propertyIdDesired);
        RequireDifferentIds(propertyIdForSale, propertyIdDesired);
    }

    std::vector<CMPMetaDEx> vecMetaDexObjects;
    {
        LOCK(cs_tally);
        for (md_PropertiesMap::const_iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
            const md_PricesMap& prices = my_it->second;
            for (md_PricesMap::const_iterator it = prices.begin(); it != prices.end(); ++it) {
                const md_Set& indexes = it->second;
                for (md_Set::const_iterator it = indexes.begin(); it != indexes.end(); ++it) {
                    const CMPMetaDEx& obj = *it;
                    if (obj.getProperty() != propertyIdForSale) continue;
                    if (!filterDesired || obj.getDesProperty() == propertyIdDesired) vecMetaDexObjects.push_back(obj);
                }
            }
        }
    }

    UniValue response(UniValue::VARR);
    MetaDexObjectsToJSON(vecMetaDexObjects, response);
    return response;
}

static UniValue omni_gettradehistoryforaddress(const JSONRPCRequest& request)
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pWallet = interfaces::MakeWallet(wallet);
#else
    std::unique_ptr<interfaces::Wallet> pWallet;
#endif

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 3)
        throw runtime_error(
            RPCHelpMan{"omni_gettradehistoryforaddress",
               "\nRetrieves the history of orders on the distributed exchange for the supplied address.\n",
               {
                   {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "address to retrieve history for\n"},
                   {"count", RPCArg::Type::NUM, /* default */ "10", "number of orders to retrieve\n"},
                   {"propertyid", RPCArg::Type::NUM, /* default */ "no filter", "filter by property identifier transacted\n"},
               },
               RPCResult{
                   "[                                              (array of JSON objects)\n"
                   "  {\n"
                   "    \"txid\" : \"hash\",                               (string) the hex-encoded hash of the transaction of the order\n"
                   "    \"sendingaddress\" : \"address\",                  (string) the Qitcoin address of the trader\n"
                   "    \"ismine\" : true|false,                         (boolean) whether the order involes an address in the wallet\n"
                   "    \"confirmations\" : nnnnnnnnnn,                  (number) the number of transaction confirmations\n"
                   "    \"fee\" : \"n.nnnnnnnn\",                          (string) the transaction fee in bitcoins\n"
                   "    \"blocktime\" : nnnnnnnnnn,                      (number) the timestamp of the block that contains the transaction\n"
                   "    \"valid\" : true|false,                          (boolean) whether the transaction is valid\n"
                   "    \"version\" : n,                                 (number) the transaction version\n"
                   "    \"type_int\" : n,                                (number) the transaction type as number\n"
                   "    \"type\" : \"type\",                               (string) the transaction type as string\n"
                   "    \"propertyidforsale\" : n,                       (number) the identifier of the tokens put up for sale\n"
                   "    \"propertyidforsaleisdivisible\" : true|false,   (boolean) whether the tokens for sale are divisible\n"
                   "    \"amountforsale\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially offered\n"
                   "    \"propertyiddesired\" : n,                       (number) the identifier of the tokens desired in exchange\n"
                   "    \"propertyiddesiredisdivisible\" : true|false,   (boolean) whether the desired tokens are divisible\n"
                   "    \"amountdesired\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially desired\n"
                   "    \"unitprice\" : \"n.nnnnnnnnnnn...\"               (string) the unit price (shown in the property desired)\n"
                   "    \"status\" : \"status\"                            (string) the status of the order (\"open\", \"cancelled\", \"filled\", ...)\n"
                   "    \"canceltxid\" : \"hash\",                         (string) the hash of the transaction that cancelled the order (if cancelled)\n"
                   "    \"matches\": [                                   (array of JSON objects) a list of matched orders and executed trades\n"
                   "      {\n"
                   "        \"txid\" : \"hash\",                               (string) the hash of the transaction that was matched against\n"
                   "        \"block\" : nnnnnn,                              (number) the index of the block that contains this transaction\n"
                   "        \"address\" : \"address\",                         (string) the Qitcoin address of the other trader\n"
                   "        \"amountsold\" : \"n.nnnnnnnn\",                   (string) the number of tokens sold in this trade\n"
                   "        \"amountreceived\" : \"n.nnnnnnnn\"                (string) the number of tokens traded in exchange\n"
                   "      },\n"
                   "      ...\n"
                   "    ]\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
                   "\nNote:\n"
                   "The documentation only covers the output for a trade, but there are also cancel transactions with different properties.\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_gettradehistoryforaddress", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\"")
                   + HelpExampleRpc("omni_gettradehistoryforaddress", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\"")
               }
            }.ToString());

    std::string address = ParseAddress(request.params[0]);
    uint64_t count = (request.params.size() > 1) ? request.params[1].get_int64() : 10;
    uint32_t propertyId = 0;

    if (request.params.size() > 2) {
        propertyId = ParsePropertyId(request.params[2]);
        RequireExistingProperty(propertyId);
    }

    // Obtain a sorted vector of txids for the address trade history
    std::vector<uint256> vecTransactions;
    {
        LOCK(cs_tally);
        pDbTradeList->getTradesForAddress(address, vecTransactions, propertyId);
    }

    // Populate the address trade history into JSON objects until we have processed count transactions
    UniValue response(UniValue::VARR);
    uint32_t processed = 0;
    for(std::vector<uint256>::reverse_iterator it = vecTransactions.rbegin(); it != vecTransactions.rend(); ++it) {
        UniValue txobj(UniValue::VOBJ);
        int populateResult = populateRPCTransactionObject(*it, txobj, "", true, "", pWallet.get());
        if (0 == populateResult) {
            response.push_back(txobj);
            processed++;
            if (processed >= count) break;
        }
    }

    return response;
}

static UniValue omni_gettradehistoryforpair(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3)
        throw runtime_error(
            RPCHelpMan{"omni_gettradehistoryforpair",
               "\nRetrieves the history of trades on the distributed token exchange for the specified market.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the first side of the traded pair\n"},
                   {"propertyidsecond", RPCArg::Type::NUM, RPCArg::Optional::NO, "the second side of the traded pair\n"},
                   {"count", RPCArg::Type::NUM, /* default */ "10", "number of trades to retrieve\n"},
               },
               RPCResult{
                   "[                                      (array of JSON objects)\n"
                   "  {\n"
                   "    \"block\" : nnnnnn,                      (number) the index of the block that contains the trade match\n"
                   "    \"unitprice\" : \"n.nnnnnnnnnnn...\" ,     (string) the unit price used to execute this trade (received/sold)\n"
                   "    \"inverseprice\" : \"n.nnnnnnnnnnn...\",   (string) the inverse unit price (sold/received)\n"
                   "    \"sellertxid\" : \"hash\",                 (string) the hash of the transaction of the seller\n"
                   "    \"address\" : \"address\",                 (string) the Qitcoin address of the seller\n"
                   "    \"amountsold\" : \"n.nnnnnnnn\",           (string) the number of tokens sold in this trade\n"
                   "    \"amountreceived\" : \"n.nnnnnnnn\",       (string) the number of tokens traded in exchange\n"
                   "    \"matchingtxid\" : \"hash\",               (string) the hash of the transaction that was matched against\n"
                   "    \"matchingaddress\" : \"address\"          (string) the Qitcoin address of the other party of this trade\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_gettradehistoryforpair", "1 12 500")
                   + HelpExampleRpc("omni_gettradehistoryforpair", "1, 12, 500")
               }
            }.ToString());

    // obtain property identifiers for pair & check valid parameters
    uint32_t propertyIdSideA = ParsePropertyId(request.params[0]);
    uint32_t propertyIdSideB = ParsePropertyId(request.params[1]);
    uint64_t count = (request.params.size() > 2) ? request.params[2].get_int64() : 10;

    RequireExistingProperty(propertyIdSideA);
    RequireExistingProperty(propertyIdSideB);
    RequireSameEcosystem(propertyIdSideA, propertyIdSideB);
    RequireDifferentIds(propertyIdSideA, propertyIdSideB);

    // request pair trade history from trade db
    UniValue response(UniValue::VARR);
    LOCK(cs_tally);
    pDbTradeList->getTradesForPair(propertyIdSideA, propertyIdSideB, response, count);
    return response;
}

static UniValue omni_getactivedexsells(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw runtime_error(
            RPCHelpMan{"omni_getactivedexsells",
               "\nReturns currently active offers on the distributed exchange.\n",
               {
                   {"address", RPCArg::Type::STR, /* default */ "include any", "address filter\n"},
               },
               RPCResult{
                   "[                                   (array of JSON objects)\n"
                   "  {\n"
                   "    \"txid\" : \"hash\",                    (string) the hash of the transaction of this offer\n"
                   "    \"propertyid\" : n,                   (number) the identifier of the tokens for sale\n"
                   "    \"seller\" : \"address\",               (string) the Qitcoin address of the seller\n"
                   "    \"amountavailable\" : \"n.nnnnnnnn\",   (string) the number of tokens still listed for sale and currently available\n"
                   "    \"bitcoindesired\" : \"n.nnnnnnnn\",    (string) the number of bitcoins desired in exchange\n"
                   "    \"unitprice\" : \"n.nnnnnnnn\" ,        (string) the unit price (QTC/token)\n"
                   "    \"timelimit\" : nn,                   (number) the time limit in blocks a buyer has to pay following a successful accept\n"
                   "    \"minimumfee\" : \"n.nnnnnnnn\",        (string) the minimum mining fee a buyer has to pay to accept this offer\n"
                   "    \"amountaccepted\" : \"n.nnnnnnnn\",    (string) the number of tokens currently reserved for pending \"accept\" orders\n"
                   "    \"accepts\": [                        (array of JSON objects) a list of pending \"accept\" orders\n"
                   "      {\n"
                   "        \"buyer\" : \"address\",                (string) the Qitcoin address of the buyer\n"
                   "        \"block\" : nnnnnn,                   (number) the index of the block that contains the \"accept\" order\n"
                   "        \"blocksleft\" : nn,                  (number) the number of blocks left to pay\n"
                   "        \"amount\" : \"n.nnnnnnnn\"             (string) the amount of tokens accepted and reserved\n"
                   "        \"amounttopay\" : \"n.nnnnnnnn\"        (string) the amount in bitcoins needed finalize the trade\n"
                   "      },\n"
                   "      ...\n"
                   "    ]\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getactivedexsells", "")
                   + HelpExampleRpc("omni_getactivedexsells", "")
               }
            }.ToString());

    std::string addressFilter;

    if (request.params.size() > 0) {
        addressFilter = ParseAddressOrEmpty(request.params[0]);
    }

    UniValue response(UniValue::VARR);

    int curBlock = GetHeight();

    LOCK(cs_tally);

    for (OfferMap::iterator it = my_offers.begin(); it != my_offers.end(); ++it) {
        const CMPOffer& selloffer = it->second;
        std::vector<std::string> vstr;
        boost::split(vstr, it->first, boost::is_any_of("-"), boost::token_compress_on);
        std::string seller = vstr[0];

        // filtering
        if (!addressFilter.empty() && seller != addressFilter) continue;

        std::string txid = selloffer.getHash().GetHex();
        uint32_t propertyId = selloffer.getProperty();
        int64_t minFee = selloffer.getMinFee();
        uint8_t timeLimit = selloffer.getBlockTimeLimit();
        int64_t sellOfferAmount = selloffer.getOfferAmountOriginal(); //badly named - "Original" implies off the wire, but is amended amount
        int64_t sellBitcoinDesired = selloffer.getBTCDesiredOriginal(); //badly named - "Original" implies off the wire, but is amended amount
        int64_t amountAvailable = GetTokenBalance(seller, propertyId, SELLOFFER_RESERVE);
        int64_t amountAccepted = GetTokenBalance(seller, propertyId, ACCEPT_RESERVE);

        // TODO: no math, and especially no rounding here (!)
        // TODO: no math, and especially no rounding here (!)
        // TODO: no math, and especially no rounding here (!)

        // calculate unit price and updated amount of bitcoin desired
        double unitPriceFloat = 0.0;
        if ((sellOfferAmount > 0) && (sellBitcoinDesired > 0)) {
            unitPriceFloat = (double) sellBitcoinDesired / (double) sellOfferAmount; // divide by zero protection
            if (!isPropertyDivisible(propertyId)) {
                unitPriceFloat /= 100000000.0;
            }
        }
        int64_t unitPrice = rounduint64(unitPriceFloat * COIN);
        int64_t bitcoinDesired = calculateDesiredBTC(sellOfferAmount, sellBitcoinDesired, amountAvailable);

        UniValue responseObj(UniValue::VOBJ);
        responseObj.pushKV("txid", txid);
        responseObj.pushKV("propertyid", (uint64_t) propertyId);
        responseObj.pushKV("seller", seller);
        responseObj.pushKV("amountavailable", FormatMP(propertyId, amountAvailable));
        responseObj.pushKV("bitcoindesired", FormatDivisibleMP(bitcoinDesired));
        responseObj.pushKV("unitprice", FormatDivisibleMP(unitPrice));
        responseObj.pushKV("timelimit", timeLimit);
        responseObj.pushKV("minimumfee", FormatDivisibleMP(minFee));

        // display info about accepts related to sell
        responseObj.pushKV("amountaccepted", FormatMP(propertyId, amountAccepted));
        UniValue acceptsMatched(UniValue::VARR);
        for (AcceptMap::const_iterator ait = my_accepts.begin(); ait != my_accepts.end(); ++ait) {
            UniValue matchedAccept(UniValue::VOBJ);
            const CMPAccept& accept = ait->second;
            const std::string& acceptCombo = ait->first;

            // does this accept match the sell?
            if (accept.getHash() == selloffer.getHash()) {
                // split acceptCombo out to get the buyer address
                std::string buyer = acceptCombo.substr((acceptCombo.find("+") + 1), (acceptCombo.size()-(acceptCombo.find("+") + 1)));
                int blockOfAccept = accept.getAcceptBlock();
                int blocksLeftToPay = (blockOfAccept + selloffer.getBlockTimeLimit()) - curBlock;
                int64_t amountAccepted = accept.getAcceptAmountRemaining();
                // TODO: don't recalculate!
                int64_t amountToPayInBTC = calculateDesiredBTC(accept.getOfferAmountOriginal(), accept.getBTCDesiredOriginal(), amountAccepted);
                matchedAccept.pushKV("buyer", buyer);
                matchedAccept.pushKV("block", blockOfAccept);
                matchedAccept.pushKV("blocksleft", blocksLeftToPay);
                matchedAccept.pushKV("amount", FormatMP(propertyId, amountAccepted));
                matchedAccept.pushKV("amounttopay", FormatDivisibleMP(amountToPayInBTC));
                acceptsMatched.push_back(matchedAccept);
            }
        }
        responseObj.pushKV("accepts", acceptsMatched);

        // add sell object into response array
        response.push_back(responseObj);
    }

    return response;
}

static UniValue omni_listblocktransactions(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_listblocktransactions",
               "\nLists all Omni transactions in a block.\n",
               {
                   {"index", RPCArg::Type::NUM, RPCArg::Optional::NO, "the block height or block index\n"},
               },
               RPCResult{
                   "[                       (array of string)\n"
                   "  \"hash\",                 (string) the hash of the transaction\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                HelpExampleCli("omni_listblocktransactions", "279007")
                + HelpExampleRpc("omni_listblocktransactions", "279007")
               }
            }.ToString());

    int blockHeight = request.params[0].get_int();

    RequireHeightInChain(blockHeight);

    // next let's obtain the block for this height
    CBlock block;
    {
        LOCK(cs_main);
        CBlockIndex* pBlockIndex = ::ChainActive()[blockHeight];

        if (!ReadBlockFromDisk(block, pBlockIndex, Params().GetConsensus())) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to read block from disk");
        }
    }

    UniValue response(UniValue::VARR);

    // now we want to loop through each of the transactions in the block and run against CMPTxList::exists
    // those that return positive add to our response array

    LOCK(cs_tally);

    for(const auto tx : block.vtx) {
        if (pDbTransactionList->exists(tx->GetHash())) {
            // later we can add a verbose flag to decode here, but for now callers can send returned txids into gettransaction_MP
            // add the txid into the response as it's an MP transaction
            response.push_back(tx->GetHash().GetHex());
        }
    }

    return response;
}

static UniValue omni_listblockstransactions(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            RPCHelpMan{"omni_listblockstransactions",
               "\nLists all Omni transactions in a given range of blocks.\n",
               {
                   {"firstblock", RPCArg::Type::NUM, RPCArg::Optional::NO, "the index of the first block to consider\n"},
                   {"lastblock", RPCArg::Type::NUM, RPCArg::Optional::NO, "the index of the last block to consider\n"},
               },
               RPCResult{
                   "[                       (array of string)\n"
                   "  \"hash\",                 (string) the hash of the transaction\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_listblockstransactions", "279007 300000")
                   + HelpExampleRpc("omni_listblockstransactions", "279007, 300000")
               }
            }.ToString());

    int blockFirst = request.params[0].get_int();
    int blockLast = request.params[1].get_int();

    std::set<uint256> txs;
    UniValue response(UniValue::VARR);

    LOCK(cs_tally);
    {
        pDbTransactionList->GetOmniTxsInBlockRange(blockFirst, blockLast, txs);
    }

    for(const uint256& tx : txs) {
        response.push_back(tx.GetHex());
    }

    return response;
}

static UniValue omni_gettransaction(const JSONRPCRequest& request)
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pWallet = interfaces::MakeWallet(wallet);
#else
    std::unique_ptr<interfaces::Wallet> pWallet;
#endif

    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_gettransaction",
               "\nGet detailed information about an Omni transaction.\n",
               {
                   {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "the hash of the transaction to lookup\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
                   "  \"sendingaddress\" : \"address\",     (string) the Qitcoin address of the sender\n"
                   "  \"referenceaddress\" : \"address\",   (string) a Qitcoin address used as reference (if any)\n"
                   "  \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
                   "  \"confirmations\" : nnnnnnnnnn,     (number) the number of transaction confirmations\n"
                   "  \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in bitcoins\n"
                   "  \"blocktime\" : nnnnnnnnnn,         (number) the timestamp of the block that contains the transaction\n"
                   "  \"valid\" : true|false,             (boolean) whether the transaction is valid\n"
                   "  \"invalidreason\" : \"reason\",     (string) if a transaction is invalid, the reason \n"
                   "  \"version\" : n,                    (number) the transaction version\n"
                   "  \"type_int\" : n,                   (number) the transaction type as number\n"
                   "  \"type\" : \"type\",                  (string) the transaction type as string\n"
                   "  [...]                             (mixed) other transaction type specific properties\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
                   + HelpExampleRpc("omni_gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
               }
            }.ToString());

    uint256 hash = ParseHashV(request.params[0], "txid");

    UniValue txobj(UniValue::VOBJ);
    int populateResult = populateRPCTransactionObject(hash, txobj, "", false, "", pWallet.get());
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

#ifdef ENABLE_WALLET
static UniValue omni_listtransactions(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pWallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() > 5)
        throw runtime_error(
            RPCHelpMan{"omni_listtransactions",
               "\nList wallet transactions, optionally filtered by an address and block boundaries.\n",
               {
                   {"address", RPCArg::Type::STR, /* default */ "\"*\"", "address filter\n"},
                   {"count", RPCArg::Type::NUM, /* default */ "10", "show at most n transactions\n"},
                   {"skip", RPCArg::Type::NUM, /* default */ "0", "skip the first n transactions\n"},
                   {"startblock", RPCArg::Type::NUM, /* default */ "0", "first block to begin the search\n"},
                   {"endblock", RPCArg::Type::NUM, /* default */ "999999999", "last block to include in the search\n"},
               },
               RPCResult{
                   "[                                 (array of JSON objects)\n"
                   "  {\n"
                   "    \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
                   "    \"sendingaddress\" : \"address\",     (string) the Qitcoin address of the sender\n"
                   "    \"referenceaddress\" : \"address\",   (string) a Qitcoin address used as reference (if any)\n"
                   "    \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
                   "    \"confirmations\" : nnnnnnnnnn,     (number) the number of transaction confirmations\n"
                   "    \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in bitcoins\n"
                   "    \"blocktime\" : nnnnnnnnnn,         (number) the timestamp of the block that contains the transaction\n"
                   "    \"valid\" : true|false,             (boolean) whether the transaction is valid\n"
                   "    \"version\" : n,                    (number) the transaction version\n"
                   "    \"type_int\" : n,                   (number) the transaction type as number\n"
                   "    \"type\" : \"type\",                  (string) the transaction type as string\n"
                   "    [...]                             (mixed) other transaction type specific properties\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_listtransactions", "")
                   + HelpExampleRpc("omni_listtransactions", "")
               }
            }.ToString());

    // obtains parameters - default all wallet addresses & last 10 transactions
    std::string addressParam;
    if (request.params.size() > 0) {
        if (("*" != request.params[0].get_str()) && ("" != request.params[0].get_str())) addressParam = request.params[0].get_str();
    }
    int64_t nCount = 10;
    if (request.params.size() > 1) nCount = request.params[1].get_int64();
    if (nCount < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    int64_t nFrom = 0;
    if (request.params.size() > 2) nFrom = request.params[2].get_int64();
    if (nFrom < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative from");
    int64_t nStartBlock = 0;
    if (request.params.size() > 3) nStartBlock = request.params[3].get_int64();
    if (nStartBlock < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative start block");
    int64_t nEndBlock = 999999999;
    if (request.params.size() > 4) nEndBlock = request.params[4].get_int64();
    if (nEndBlock < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative end block");

    // obtain a sorted list of Omni layer wallet transactions (including STO receipts and pending)
    std::map<std::string,uint256> walletTransactions = FetchWalletOmniTransactions(*pWallet, nFrom+nCount, nStartBlock, nEndBlock);

    // reverse iterate over (now ordered) transactions and populate RPC objects for each one
    UniValue response(UniValue::VARR);
    for (std::map<std::string,uint256>::reverse_iterator it = walletTransactions.rbegin(); it != walletTransactions.rend(); it++) {
        if (nFrom <= 0 && nCount > 0) {
            uint256 txHash = it->second;
            UniValue txobj(UniValue::VOBJ);
            int populateResult = populateRPCTransactionObject(txHash, txobj, addressParam, false, "", pWallet.get());
            if (0 == populateResult) {
                response.push_back(txobj);
                nCount--;
            }
        }
        nFrom--;
    }

    return response;
}
#endif

static UniValue omni_listpendingtransactions(const JSONRPCRequest& request)
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pWallet = interfaces::MakeWallet(wallet);
#else
    std::unique_ptr<interfaces::Wallet> pWallet;
#endif

    if (request.fHelp || request.params.size() > 1)
        throw runtime_error(
            RPCHelpMan{"omni_listpendingtransactions",
               "\nReturns a list of unconfirmed Omni transactions, pending in the memory pool.\n"
               "\nAn optional filter can be provided to only include transactions which involve the given address.\n"
               "\nNote: the validity of pending transactions is uncertain, and the state of the memory pool may "
               "change at any moment. It is recommended to check transactions after confirmation, and pending "
               "transactions should be considered as invalid.\n",
               {
                   {"address", RPCArg::Type::STR, /* default */ "\"\" for no filter", "address filter\n"},
               },
               RPCResult{
                   "[                                 (array of JSON objects)\n"
                   "  {\n"
                   "    \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
                   "    \"sendingaddress\" : \"address\",     (string) the Qitcoin address of the sender\n"
                   "    \"referenceaddress\" : \"address\",   (string) a Qitcoin address used as reference (if any)\n"
                   "    \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
                   "    \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in bitcoins\n"
                   "    \"version\" : n,                    (number) the transaction version\n"
                   "    \"type_int\" : n,                   (number) the transaction type as number\n"
                   "    \"type\" : \"type\",                  (string) the transaction type as string\n"
                   "    [...]                             (mixed) other transaction type specific properties\n"
                   "  },\n"
                   "  ...\n"
                   "]\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_listpendingtransactions", "")
                   + HelpExampleRpc("omni_listpendingtransactions", "")
               }
            }.ToString());

    std::string filterAddress;
    if (request.params.size() > 0) {
        filterAddress = ParseAddressOrEmpty(request.params[0]);
    }

    std::vector<uint256> vTxid;
    mempool.queryHashes(vTxid);

    UniValue result(UniValue::VARR);
    for(const uint256& hash : vTxid) {
        if (!IsInMarkerCache(hash)) {
            continue;
        }

        UniValue txObj(UniValue::VOBJ);
        if (populateRPCTransactionObject(hash, txObj, filterAddress, false, "", pWallet.get()) == 0) {
            result.push_back(txObj);
        }
    }

    return result;
}

static UniValue omni_getinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            RPCHelpMan{"omni_getinfo",
               "Returns various state information of the client and protocol.\n",
               {},
               RPCResult{
                   "{\n"
                   "  \"omnicoreversion_int\" : xxxxxxx,       (number) client version as integer\n"
                   "  \"omnicoreversion\" : \"x.x.x.x-xxx\",     (string) client version\n"
                   "  \"mastercoreversion\" : \"x.x.x.x-xxx\",   (string) client version (DEPRECATED)\n"
                   "  \"bitcoincoreversion\" : \"x.x.x\",        (string) Qitcoin Core version\n"
                   "  \"block\" : nnnnnn,                      (number) index of the last processed block\n"
                   "  \"blocktime\" : nnnnnnnnnn,              (number) timestamp of the last processed block\n"
                   "  \"blocktransactions\" : nnnn,            (number) Omni transactions found in the last processed block\n"
                   "  \"totaltransactions\" : nnnnnnnn,        (number) Omni transactions processed in total\n"
                   "  \"alerts\" : [                           (array of JSON objects) active protocol alert (if any)\n"
                   "    {\n"
                   "      \"alerttypeint\" : n,                    (number) alert type as integer\n"
                   "      \"alerttype\" : \"xxx\",                   (string) alert type\n"
                   "      \"alertexpiry\" : \"nnnnnnnnnn\",          (string) expiration criteria\n"
                   "      \"alertmessage\" : \"xxx\"                 (string) information about the alert\n"
                   "    },\n"
                   "    ...\n"
                   "  ]\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getinfo", "")
                   + HelpExampleRpc("omni_getinfo", "")
               }
            }.ToString());

    UniValue infoResponse(UniValue::VOBJ);

    // provide the mastercore and bitcoin version
    infoResponse.pushKV("omnicoreversion_int", OMNICORE_VERSION);
    infoResponse.pushKV("omnicoreversion", OmniCoreVersion());
    infoResponse.pushKV("mastercoreversion", OmniCoreVersion());
    infoResponse.pushKV("bitcoincoreversion", BitcoinCoreVersion());

    // provide the current block details
    int block = GetHeight();
    int64_t blockTime = GetLatestBlockTime();

    LOCK(cs_tally);

    int blockMPTransactions = pDbTransactionList->getMPTransactionCountBlock(block);
    int totalMPTransactions = pDbTransactionList->getMPTransactionCountTotal();
    int totalMPTrades = pDbTradeList->getMPTradeCountTotal();
    infoResponse.pushKV("block", block);
    infoResponse.pushKV("blocktime", blockTime);
    infoResponse.pushKV("blocktransactions", blockMPTransactions);

    // provide the number of trades completed
    infoResponse.pushKV("totaltrades", totalMPTrades);
    // provide the number of transactions parsed
    infoResponse.pushKV("totaltransactions", totalMPTransactions);

    // handle alerts
    UniValue alerts(UniValue::VARR);
    std::vector<AlertData> omniAlerts = GetOmniCoreAlerts();
    for (std::vector<AlertData>::iterator it = omniAlerts.begin(); it != omniAlerts.end(); it++) {
        AlertData alert = *it;
        UniValue alertResponse(UniValue::VOBJ);
        std::string alertTypeStr;
        switch (alert.alert_type) {
            case 1: alertTypeStr = "alertexpiringbyblock";
            break;
            case 2: alertTypeStr = "alertexpiringbyblocktime";
            break;
            case 3: alertTypeStr = "alertexpiringbyclientversion";
            break;
            default: alertTypeStr = "error";
        }
        alertResponse.pushKV("alerttypeint", alert.alert_type);
        alertResponse.pushKV("alerttype", alertTypeStr);
        alertResponse.pushKV("alertexpiry", FormatIndivisibleMP(alert.alert_expiry));
        alertResponse.pushKV("alertmessage", alert.alert_message);
        alerts.push_back(alertResponse);
    }
    infoResponse.pushKV("alerts", alerts);

    return infoResponse;
}

static UniValue omni_getactivations(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            RPCHelpMan{"omni_getactivations",
               "Returns pending and completed feature activations.\n",
               {},
               RPCResult{
                   "{\n"
                   "  \"pendingactivations\": [       (array of JSON objects) a list of pending feature activations\n"
                   "    {\n"
                   "      \"featureid\" : n,              (number) the id of the feature\n"
                   "      \"featurename\" : \"xxxxxxxx\",   (string) the name of the feature\n"
                   "      \"activationblock\" : n,        (number) the block the feature will be activated\n"
                   "      \"minimumversion\" : n          (number) the minimum client version needed to support this feature\n"
                   "    },\n"
                   "    ...\n"
                   "  ]\n"
                   "  \"completedactivations\": [     (array of JSON objects) a list of completed feature activations\n"
                   "    {\n"
                   "      \"featureid\" : n,              (number) the id of the feature\n"
                   "      \"featurename\" : \"xxxxxxxx\",   (string) the name of the feature\n"
                   "      \"activationblock\" : n,        (number) the block the feature will be activated\n"
                   "      \"minimumversion\" : n          (number) the minimum client version needed to support this feature\n"
                   "    },\n"
                   "    ...\n"
                   "  ]\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getactivations", "")
                   + HelpExampleRpc("omni_getactivations", "")
               }
            }.ToString());

    UniValue response(UniValue::VOBJ);

    UniValue arrayPendingActivations(UniValue::VARR);
    std::vector<FeatureActivation> vecPendingActivations = GetPendingActivations();
    for (std::vector<FeatureActivation>::iterator it = vecPendingActivations.begin(); it != vecPendingActivations.end(); ++it) {
        UniValue actObj(UniValue::VOBJ);
        FeatureActivation pendingAct = *it;
        actObj.pushKV("featureid", pendingAct.featureId);
        actObj.pushKV("featurename", pendingAct.featureName);
        actObj.pushKV("activationblock", pendingAct.activationBlock);
        actObj.pushKV("minimumversion", (uint64_t)pendingAct.minClientVersion);
        arrayPendingActivations.push_back(actObj);
    }

    UniValue arrayCompletedActivations(UniValue::VARR);
    std::vector<FeatureActivation> vecCompletedActivations = GetCompletedActivations();
    for (std::vector<FeatureActivation>::iterator it = vecCompletedActivations.begin(); it != vecCompletedActivations.end(); ++it) {
        UniValue actObj(UniValue::VOBJ);
        FeatureActivation completedAct = *it;
        actObj.pushKV("featureid", completedAct.featureId);
        actObj.pushKV("featurename", completedAct.featureName);
        actObj.pushKV("activationblock", completedAct.activationBlock);
        actObj.pushKV("minimumversion", (uint64_t)completedAct.minClientVersion);
        arrayCompletedActivations.push_back(actObj);
    }

    response.pushKV("pendingactivations", arrayPendingActivations);
    response.pushKV("completedactivations", arrayCompletedActivations);

    return response;
}

static UniValue omni_getsto(const JSONRPCRequest& request)
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pWallet = interfaces::MakeWallet(wallet);
#else
    std::unique_ptr<interfaces::Wallet> pWallet;
#endif

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw runtime_error(
            RPCHelpMan{"omni_getsto",
               "\nGet information and recipients of a send-to-owners transaction.\n",
               {
                   {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "the hash of the transaction to lookup\n"},
                   {"recipientfilter", RPCArg::Type::STR, /* default */ "\"*\" for all", "a filter for recipients\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"txid\" : \"hash\",                (string) the hex-encoded hash of the transaction\n"
                   "  \"sendingaddress\" : \"address\",   (string) the Qitcoin address of the sender\n"
                   "  \"ismine\" : true|false,          (boolean) whether the transaction involes an address in the wallet\n"
                   "  \"confirmations\" : nnnnnnnnnn,   (number) the number of transaction confirmations\n"
                   "  \"fee\" : \"n.nnnnnnnn\",           (string) the transaction fee in bitcoins\n"
                   "  \"blocktime\" : nnnnnnnnnn,       (number) the timestamp of the block that contains the transaction\n"
                   "  \"valid\" : true|false,           (boolean) whether the transaction is valid\n"
                   "  \"version\" : n,                  (number) the transaction version\n"
                   "  \"type_int\" : n,                 (number) the transaction type as number\n"
                   "  \"type\" : \"type\",                (string) the transaction type as string\n"
                   "  \"propertyid\" : n,               (number) the identifier of sent tokens\n"
                   "  \"divisible\" : true|false,       (boolean) whether the sent tokens are divisible\n"
                   "  \"amount\" : \"n.nnnnnnnn\",        (string) the number of tokens sent to owners\n"
                   "  \"totalstofee\" : \"n.nnnnnnnn\",   (string) the fee paid by the sender, nominated in OMN or TOMN\n"
                   "  \"recipients\": [                 (array of JSON objects) a list of recipients\n"
                   "    {\n"
                   "      \"address\" : \"address\",          (string) the Qitcoin address of the recipient\n"
                   "      \"amount\" : \"n.nnnnnnnn\"         (string) the number of tokens sent to this recipient\n"
                   "    },\n"
                   "    ...\n"
                   "  ]\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getsto", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\" \"*\"")
                   + HelpExampleRpc("omni_getsto", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"*\"")
               }
            }.ToString());

    uint256 hash = ParseHashV(request.params[0], "txid");
    std::string filterAddress;
    if (request.params.size() > 1) filterAddress = ParseAddressOrWildcard(request.params[1]);

    UniValue txobj(UniValue::VOBJ);
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true, filterAddress, pWallet.get());
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

static UniValue omni_gettrade(const JSONRPCRequest& request)
{
#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pWallet = interfaces::MakeWallet(wallet);
#else
    std::unique_ptr<interfaces::Wallet> pWallet;
#endif

    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_gettrade",
               "\nGet detailed information and trade matches for orders on the distributed token exchange.\n",
               {
                   {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "the hash of the order to lookup\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"txid\" : \"hash\",                               (string) the hex-encoded hash of the transaction of the order\n"
                   "  \"sendingaddress\" : \"address\",                  (string) the Qitcoin address of the trader\n"
                   "  \"ismine\" : true|false,                         (boolean) whether the order involes an address in the wallet\n"
                   "  \"confirmations\" : nnnnnnnnnn,                  (number) the number of transaction confirmations\n"
                   "  \"fee\" : \"n.nnnnnnnn\",                          (string) the transaction fee in bitcoins\n"
                   "  \"blocktime\" : nnnnnnnnnn,                      (number) the timestamp of the block that contains the transaction\n"
                   "  \"valid\" : true|false,                          (boolean) whether the transaction is valid\n"
                   "  \"version\" : n,                                 (number) the transaction version\n"
                   "  \"type_int\" : n,                                (number) the transaction type as number\n"
                   "  \"type\" : \"type\",                               (string) the transaction type as string\n"
                   "  \"propertyidforsale\" : n,                       (number) the identifier of the tokens put up for sale\n"
                   "  \"propertyidforsaleisdivisible\" : true|false,   (boolean) whether the tokens for sale are divisible\n"
                   "  \"amountforsale\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially offered\n"
                   "  \"propertyiddesired\" : n,                       (number) the identifier of the tokens desired in exchange\n"
                   "  \"propertyiddesiredisdivisible\" : true|false,   (boolean) whether the desired tokens are divisible\n"
                   "  \"amountdesired\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially desired\n"
                   "  \"unitprice\" : \"n.nnnnnnnnnnn...\"               (string) the unit price (shown in the property desired)\n"
                   "  \"status\" : \"status\"                            (string) the status of the order (\"open\", \"cancelled\", \"filled\", ...)\n"
                   "  \"canceltxid\" : \"hash\",                         (string) the hash of the transaction that cancelled the order (if cancelled)\n"
                   "  \"matches\": [                                   (array of JSON objects) a list of matched orders and executed trades\n"
                   "    {\n"
                   "      \"txid\" : \"hash\",                               (string) the hash of the transaction that was matched against\n"
                   "      \"block\" : nnnnnn,                              (number) the index of the block that contains this transaction\n"
                   "      \"address\" : \"address\",                         (string) the Qitcoin address of the other trader\n"
                   "      \"amountsold\" : \"n.nnnnnnnn\",                   (string) the number of tokens sold in this trade\n"
                   "      \"amountreceived\" : \"n.nnnnnnnn\"                (string) the number of tokens traded in exchange\n"
                   "    },\n"
                   "    ...\n"
                   "  ]\n"
                   "}\n"
                   "\nNote:\n"
                   "The documentation only covers the output for a trade, but there are also cancel transactions with different properties.\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_gettrade", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
                   + HelpExampleRpc("omni_gettrade", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
               }
            }.ToString());

    uint256 hash = ParseHashV(request.params[0], "txid");

    UniValue txobj(UniValue::VOBJ);
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true, "", pWallet.get());
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

static UniValue omni_getcurrentconsensushash(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            RPCHelpMan{"omni_getcurrentconsensushash",
               "\nReturns the consensus hash for all balances for the current block.\n",
               {},
               RPCResult{
                   "{\n"
                   "  \"block\" : nnnnnn,          (number) the index of the block this consensus hash applies to\n"
                   "  \"blockhash\" : \"hash\",      (string) the hash of the corresponding block\n"
                   "  \"consensushash\" : \"hash\"   (string) the consensus hash for the block\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getcurrentconsensushash", "")
                   + HelpExampleRpc("omni_getcurrentconsensushash", "")
               }
            }.ToString());

    LOCK(cs_main); // TODO - will this ensure we don't take in a new block in the couple of ms it takes to calculate the consensus hash?

    int block = GetHeight();

    CBlockIndex* pblockindex = ::ChainActive()[block];
    uint256 blockHash = pblockindex->GetBlockHash();

    uint256 consensusHash = GetConsensusHash();

    UniValue response(UniValue::VOBJ);
    response.pushKV("block", block);
    response.pushKV("blockhash", blockHash.GetHex());
    response.pushKV("consensushash", consensusHash.GetHex());

    return response;
}

static UniValue omni_getmetadexhash(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw runtime_error(
            RPCHelpMan{"omni_getmetadexhash",
               "\nReturns a hash of the current state of the MetaDEx (default) or orderbook.\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "hash orderbook (only trades selling propertyid)\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"block\" : nnnnnn,          (number) the index of the block this hash applies to\n"
                   "  \"blockhash\" : \"hash\",    (string) the hash of the corresponding block\n"
                   "  \"propertyid\" : nnnnnn,     (number) the market this hash applies to (or 0 for all markets)\n"
                   "  \"metadexhash\" : \"hash\"   (string) the hash for the state of the MetaDEx/orderbook\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getmetadexhash", "3")
                   + HelpExampleRpc("omni_getmetadexhash", "3")
               }
            }.ToString());

    LOCK(cs_main);

    uint32_t propertyId = 0;
    if (request.params.size() > 0) {
        propertyId = ParsePropertyId(request.params[0]);
        RequireExistingProperty(propertyId);
    }

    int block = GetHeight();
    CBlockIndex* pblockindex = ::ChainActive()[block];
    uint256 blockHash = pblockindex->GetBlockHash();

    uint256 metadexHash = GetMetaDExHash(propertyId);

    UniValue response(UniValue::VOBJ);
    response.pushKV("block", block);
    response.pushKV("blockhash", blockHash.GetHex());
    response.pushKV("propertyid", (uint64_t)propertyId);
    response.pushKV("metadexhash", metadexHash.GetHex());

    return response;
}

static UniValue omni_getbalanceshash(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            RPCHelpMan{"omni_getbalanceshash",
               "omni_getbalanceshash propertyid\n",
               {
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the property to hash balances for\n"},
               },
               RPCResult{
                   "{\n"
                   "  \"block\" : nnnnnn,          (number) the index of the block this hash applies to\n"
                   "  \"blockhash\" : \"hash\",    (string) the hash of the corresponding block\n"
                   "  \"propertyid\" : nnnnnn,     (number) the property id of the hashed balances\n"
                   "  \"balanceshash\" : \"hash\"  (string) the hash for the balances\n"
                   "}\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_getbalanceshash", "31")
                   + HelpExampleRpc("omni_getbalanceshash", "31")
               }
            }.ToString());

    LOCK(cs_main);

    uint32_t propertyId = ParsePropertyId(request.params[0]);
    RequireExistingProperty(propertyId);

    int block = GetHeight();
    CBlockIndex* pblockindex = ::ChainActive()[block];
    uint256 blockHash = pblockindex->GetBlockHash();

    uint256 balancesHash = GetBalancesHash(propertyId);

    UniValue response(UniValue::VOBJ);
    response.pushKV("block", block);
    response.pushKV("blockhash", blockHash.GetHex());
    response.pushKV("propertyid", (uint64_t)propertyId);
    response.pushKV("balanceshash", balancesHash.GetHex());

    return response;
}

static const CRPCCommand commands[] =
{ //  category                             name                            actor (function)               argNames
  //  ------------------------------------ ------------------------------- ------------------------------ ----------
    { "omni layer (data retrieval)", "omni_getinfo",                   &omni_getinfo,                    {} },
    { "omni layer (data retrieval)", "omni_getactivations",            &omni_getactivations,             {} },
    { "omni layer (data retrieval)", "omni_getallbalancesforid",       &omni_getallbalancesforid,        {"propertyid"} },
    { "omni layer (data retrieval)", "omni_getbalance",                &omni_getbalance,                 {"address", "propertyid"} },
    { "omni layer (data retrieval)", "omni_gettransaction",            &omni_gettransaction,             {"txid"} },
    { "omni layer (data retrieval)", "omni_getproperty",               &omni_getproperty,                {"propertyid"} },
    { "omni layer (data retrieval)", "omni_listproperties",            &omni_listproperties,             {} },
    { "omni layer (data retrieval)", "omni_getcrowdsale",              &omni_getcrowdsale,               {"propertyid", "verbose"} },
    { "omni layer (data retrieval)", "omni_getgrants",                 &omni_getgrants,                  {"propertyid"} },
    { "omni layer (data retrieval)", "omni_getactivedexsells",         &omni_getactivedexsells,          {"address"} },
    { "omni layer (data retrieval)", "omni_getactivecrowdsales",       &omni_getactivecrowdsales,        {} },
    { "omni layer (data retrieval)", "omni_getorderbook",              &omni_getorderbook,               {"propertyid", "propertyid"} },
    { "omni layer (data retrieval)", "omni_gettrade",                  &omni_gettrade,                   {"txid"} },
    { "omni layer (data retrieval)", "omni_getsto",                    &omni_getsto,                     {"txid", "recipientfilter"} },
    { "omni layer (data retrieval)", "omni_listblocktransactions",     &omni_listblocktransactions,      {"index"} },
    { "omni layer (data retrieval)", "omni_listblockstransactions",    &omni_listblockstransactions,     {"firstblock", "lastblock"} },
    { "omni layer (data retrieval)", "omni_listpendingtransactions",   &omni_listpendingtransactions,    {"address"} },
    { "omni layer (data retrieval)", "omni_getallbalancesforaddress",  &omni_getallbalancesforaddress,   {"address"} },
    { "omni layer (data retrieval)", "omni_gettradehistoryforaddress", &omni_gettradehistoryforaddress,  {"address", "count", "propertyid"} },
    { "omni layer (data retrieval)", "omni_gettradehistoryforpair",    &omni_gettradehistoryforpair,     {"propertyid", "propertyidsecond", "count"} },
    { "omni layer (data retrieval)", "omni_getcurrentconsensushash",   &omni_getcurrentconsensushash,    {} },
    { "omni layer (data retrieval)", "omni_getpayload",                &omni_getpayload,                 {"txid"} },
    { "omni layer (data retrieval)", "omni_getseedblocks",             &omni_getseedblocks,              {"startblock", "endblock"} },
    { "omni layer (data retrieval)", "omni_getmetadexhash",            &omni_getmetadexhash,             {"propertyid"} },
    { "omni layer (data retrieval)", "omni_getfeecache",               &omni_getfeecache,                {"propertyid"} },
    { "omni layer (data retrieval)", "omni_getfeetrigger",             &omni_getfeetrigger,              {"propertyid"} },
    { "omni layer (data retrieval)", "omni_getfeedistribution",        &omni_getfeedistribution,         {"distributionid"} },
    { "omni layer (data retrieval)", "omni_getfeedistributions",       &omni_getfeedistributions,        {"propertyid"} },
    { "omni layer (data retrieval)", "omni_getbalanceshash",           &omni_getbalanceshash,            {"propertyid"} },
#ifdef ENABLE_WALLET
    { "omni layer (data retrieval)", "omni_listtransactions",          &omni_listtransactions,           {"address", "count", "skip", "startblock", "endblock"} },
    { "omni layer (data retrieval)", "omni_getfeeshare",               &omni_getfeeshare,                {"address", "ecosystem"} },
    { "omni layer (configuration)",  "omni_setautocommit",             &omni_setautocommit,              {"flag"}  },
    { "omni layer (data retrieval)", "omni_getwalletbalances",         &omni_getwalletbalances,          {"includewatchonly"} },
    { "omni layer (data retrieval)", "omni_getwalletaddressbalances",  &omni_getwalletaddressbalances,   {"includewatchonly"} },
#endif
    { "hidden",                      "mscrpc",                         &mscrpc,                          {"extra", "extra2", "extra3"}  },

    /* deprecated: */
    { "hidden",                      "getinfo_MP",                     &omni_getinfo,                    {}  },
    { "hidden",                      "getbalance_MP",                  &omni_getbalance,                 {"address", "propertyid"} },
    { "hidden",                      "getallbalancesforaddress_MP",    &omni_getallbalancesforaddress,   {"address"} },
    { "hidden",                      "getallbalancesforid_MP",         &omni_getallbalancesforid,        {"propertyid"} },
    { "hidden",                      "getproperty_MP",                 &omni_getproperty,                {"propertyid"} },
    { "hidden",                      "listproperties_MP",              &omni_listproperties,             {} },
    { "hidden",                      "getcrowdsale_MP",                &omni_getcrowdsale,               {"propertyid", "verbose"} },
    { "hidden",                      "getgrants_MP",                   &omni_getgrants,                  {"propertyid"} },
    { "hidden",                      "getactivedexsells_MP",           &omni_getactivedexsells,          {"address"} },
    { "hidden",                      "getactivecrowdsales_MP",         &omni_getactivecrowdsales,        {} },
    { "hidden",                      "getsto_MP",                      &omni_getsto,                     {"txid", "recipientfilter"} },
    { "hidden",                      "getorderbook_MP",                &omni_getorderbook,               {"propertyid", "propertyiddesired"} },
    { "hidden",                      "gettrade_MP",                    &omni_gettrade,                   {"txid"} },
    { "hidden",                      "gettransaction_MP",              &omni_gettransaction,             {"txid"} },
    { "hidden",                      "listblocktransactions_MP",       &omni_listblocktransactions,      {"index"} },
#ifdef ENABLE_WALLET
    { "hidden",                      "listtransactions_MP",            &omni_listtransactions,           {"address", "count", "skip", "startblock", "endblock"} },
#endif
};

void RegisterOmniDataRetrievalRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
