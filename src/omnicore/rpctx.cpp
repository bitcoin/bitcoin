// RPC calls for creating and sending Omni transactions

#include "omnicore/rpctx.h"

#include "omnicore/convert.h"
#include "omnicore/createpayload.h"
#include "omnicore/dex.h"
#include "omnicore/errors.h"
#include "omnicore/omnicore.h"
#include "omnicore/parse_string.h"
#include "omnicore/pending.h"
#include "omnicore/rpcvalues.h"
#include "omnicore/sp.h"
#include "omnicore/tx.h"

#include "rpcserver.h"
#include "wallet.h"

#include <boost/algorithm/string.hpp>
#include <boost/exception/to_string.hpp>
#include <boost/lexical_cast.hpp>

#include "json/json_spirit_value.h"

#include <stdint.h>

#include <map>
#include <stdexcept>
#include <string>

using boost::algorithm::token_compress_on;
using boost::to_string;

using std::map;
using std::runtime_error;
using std::string;
using std::vector;

using namespace json_spirit;
using namespace mastercore;

// send_OMNI - simple send
Value send_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 6)
        throw runtime_error(
            "send_OMNI \"fromaddress\" \"toaddress\" propertyid \"amount\" ( \"redeemaddress\" \"referenceamount\" )\n"
            "\nCreates and broadcasts a simple send for a given amount and currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send from\n"
            "ToAddress     : the address to send to\n"
            "PropertyID    : the id of the smart property to send\n"
            "Amount        : the amount to send\n"
            "RedeemAddress : (optional) the address that can redeem class B data outputs. Defaults to FromAddress\n"
            "ReferenceAmount:(optional) the number of satoshis to send to the recipient in the reference output\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored send_OMNI 1FromAddress 1ToAddress PropertyID Amount\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseAddress(params[1]);
    uint32_t propertyId = ParsePropertyId(params[2]);
    int64_t amount = ParseAmount(params[3], isPropertyDivisible(propertyId));
    std::string redeemAddress = (params.size() > 4) ? ParseAddress(params[4]): "";
    int64_t referenceAmount = (params.size() > 5) ? ParseAmount(params[5], true): 0;

    const int64_t senderBalance = getMPbalance(fromAddress, propertyId, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyId);

    // perform conversions
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");

    // perform checks
    if ((0.01 * COIN) < referenceAmount) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid reference amount");
    if (senderBalance < amount) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance");
    if (senderAvailableBalance < amount) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance (due to pending transactions)");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SimpleSend(propertyId, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, toAddress, redeemAddress, referenceAmount, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            PendingAdd(txid, fromAddress, toAddress, MSC_TYPE_SIMPLE_SEND, propertyId, amount);
            return txid.GetHex();
        }
    }
}

// senddexsell_OMNI - DEx sell offer
Value senddexsell_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 7)
        throw runtime_error(
            "senddexsell_OMNI \"fromaddress\" propertyidforsale \"amountforsale\" \"amountdesired\" paymentwindow minacceptfee action\n"
            "\nPlace or cancel a sell offer on the BTC/MSC layer of the distributed exchange.\n"
            "\nParameters:\n"
            "FromAddress         : the address to send this transaction from\n"
            "PropertyIDForSale   : the property to list for sale (must be MSC or TMSC)\n"
            "AmountForSale       : the amount to list for sale\n"
            "AmountDesired       : the amount of BTC desired\n"
            "PaymentWindow       : the time limit a buyer has to pay following a successful accept\n"
            "MinAcceptFee        : the mining fee a buyer has to pay to accept\n"
            "Action              : the action to take: (1) new, (2) update, (3) cancel \n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored senddexsell_OMNI \"1FromAddress\" PropertyIDForSale \"AmountForSale\" \"AmountDesired\" PaymentWindow MinAcceptFee Action\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(params[1]);
    int64_t amountForSale = 0; // depending on action
    int64_t amountDesired = 0; // depending on action
    uint8_t paymentWindow = 0; // depending on action
    int64_t minAcceptFee = ParseDExFee(params[5]);
    uint8_t action = ParseDExAction(params[6]);

    const int64_t senderBalance = getMPbalance(fromAddress, propertyIdForSale, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyIdForSale);

    // perform conversions
    if ((propertyIdForSale > 2 || propertyIdForSale <=0)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid propertyID for sale - only 1 and 2 are permitted");

    // perform checks
    if (action <= 2) { // actions 3 permit zero values, skip check
        amountForSale = ParseAmount(params[2], true); // TMSC/MSC is divisible
        amountDesired = ParseAmount(params[3], true); // BTC is divisible
        paymentWindow = ParseDExPaymentWindow(params[4]);
    }
    if (action != 3) { // only check for sufficient balance for new/update sell offers
        if (senderBalance < amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance");
        if (senderAvailableBalance < amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance (due to pending transactions)");
    }
    if ((action == 1) && (DEx_offerExists(fromAddress, propertyIdForSale))) throw JSONRPCError(RPC_TYPE_ERROR, "There is already a sell offer from this address on the distributed exchange, use update instead");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExSell(propertyIdForSale, amountForSale, amountDesired, paymentWindow, minAcceptFee, action);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            PendingAdd(txid, fromAddress, "", MSC_TYPE_TRADE_OFFER, propertyIdForSale, amountForSale, 0, amountDesired, action);
            return txid.GetHex();
        }
    }
}

// senddexaccept_OMNI - DEx accept offer
Value senddexaccept_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 5)
        throw runtime_error(
            "senddexaccept_OMNI \"fromaddress\" \"toaddress\" propertyid \"amount\"\n"
            "\nCreates and broadcasts an accept offer for a given amount and currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send from\n"
            "ToAddress     : the address to send the accept to\n"
            "PropertyID    : the id of the property to accept\n"
            "Amount        : the amount to accept\n"
            "Override      : override minimum accept fee and payment window checks (use with caution!)\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored senddexaccept_OMNI 1FromAddress 1ToAddress PropertyID Amount\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseAddress(params[1]);
    uint32_t propertyId = ParsePropertyId(params[2]);
    int64_t amount = ParseAmount(params[3], true); // MSC/TMSC is divisible
    bool override = (params.size() > 4) ? params[4].get_bool(): false;

    // perform checks
    if ((propertyId > 2 || propertyId <=0)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid propertyID - only 1 and 2 are permitted");
    if (!DEx_offerExists(toAddress, propertyId)) throw JSONRPCError(RPC_TYPE_ERROR, "There is no matching sell offer on the distributed exchange");

    // retrieve the sell we're accepting and obtain the required minimum fee and payment window
    CMPOffer *sellOffer = DEx_getOffer(toAddress, propertyId);
    if (sellOffer == NULL) throw JSONRPCError(RPC_TYPE_ERROR, "Unable to load sell offer from the distributed exchange");
    int64_t nMinimumAcceptFee = sellOffer->getMinFee();
    unsigned char nBlockTimeLimit = sellOffer->getBlockTimeLimit();

    if (!override) { // reject unsafe accepts - note client maximum tx fee will always be respected regardless of override here
        if (nMinimumAcceptFee > 1000000) throw JSONRPCError(RPC_TYPE_ERROR, "Unsafe trade protection - minimum accept fee is above 0.01 BTC");
        if (nBlockTimeLimit < 10) throw JSONRPCError(RPC_TYPE_ERROR, "Unsafe trade protection - payment time limit is less than 10 blocks");
    }

    // use new 0.10 custom fee to set the accept minimum fee appropriately
    CFeeRate payTxFeeOriginal = payTxFee;
    bool fPayAtLeastCustomFeeOriginal = fPayAtLeastCustomFee;
    payTxFee = CFeeRate(nMinimumAcceptFee, 1000);
    fPayAtLeastCustomFee = true;

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExAccept(propertyId, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, toAddress, "", 0, payload, txid, rawHex, autoCommit);

    // set the custom fee back to original
    payTxFee = payTxFeeOriginal;
    fPayAtLeastCustomFee = fPayAtLeastCustomFeeOriginal;

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return txid.GetHex();
        }
    }
}

// sendissuancecrowdsale_OMNI - Issue new property with crowdsale
Value sendissuancecrowdsale_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 14)
        throw runtime_error(
            "sendissuancecrowdsale_OMNI \"fromaddress\" ecosystem type previousid \"category\" \"subcategory\" \"name\" \"url\" \"data\" propertyiddesired tokensperunit deadline earlybonus issuerpercentage\n"
            "\nCreates and broadcasts a property creation transaction (crowdsale issuance) with the supplied details.\n"
            "\nParameters:\n"
            "FromAddress        : the address to send from\n"
            "Ecosystem          : the ecosystem to create the property - (1) main, (2) test\n"
            "Type               : the type of tokens - (1) indivisible, (2) divisible\n"
            "PreviousID         : the previous property id (0 for a new property)\n"
            "Category           : The category for the new property (max 255 chars)\n"
            "Subcategory        : the subcategory for the new property (max 255 chars)\n"
            "Name               : the name of the new property (max 255 chars)\n"
            "URL                : the URL for the new property (max 255 chars)\n"
            "Data               : additional data for the new property (max 255 chars)\n"
            "PropertyIDDesired  : the property that will be used to purchase from the crowdsale\n"
            "TokensPerUnit      : the amount of tokens per unit crowdfunded\n"
            "Deadline           : the deadline for the crowdsale\n"
            "EarlyBonus         : the early bonus %/week\n"
            "IssuerPercentage   : the percentage of crowdfunded tokens that will be additionally created for the issuer\n"
            "\nResult:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendissuancecrowdsale_OMNI \"1FromAddress\" Ecosystem Type PreviousID \"Category\" \"Subcategory\" \"Name\" \"URL\" \"Data\" PropertyIDDesired TokensPerUnit Deadline EarlyBonus IssuerPercentage\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint8_t ecosystem = ParseEcosystem(params[1]);
    uint16_t type = ParsePropertyType(params[2]);
    uint32_t previousId = ParsePreviousPropertyId(params[3]);
    std::string category = ParseText(params[4]);
    std::string subcategory = ParseText(params[5]);
    std::string name = ParseText(params[6]);
    std::string url = ParseText(params[7]);
    std::string data = ParseText(params[8]);
    uint32_t propertyIdDesired = ParsePropertyId(params[9]);
    int64_t numTokens = ParseAmount(params[10], type);
    int64_t deadline = ParseDeadline(params[11]);
    uint8_t earlyBonus = ParseEarlyBirdBonus(params[12]);
    uint8_t issuerPercentage = ParseIssuerBonus(params[13]);

    // perform checks
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyIdDesired, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property desired does not exist");
    if (name.empty()) throw JSONRPCError(RPC_TYPE_ERROR, "Property name cannot be empty");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_IssuanceVariable(ecosystem, type, previousId, category, subcategory, name, url, data, propertyIdDesired, numTokens, deadline, earlyBonus, issuerPercentage);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return txid.GetHex();
        }
    }
}

// sendissuancecfixed_OMNI - Issue new property with fixed amount
Value sendissuancefixed_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 10)
        throw runtime_error(
            "sendissuancefixed_OMNI \"fromaddress\" ecosystem type previousid \"category\" \"subcategory\" \"name\" \"url\" \"data\" \"amount\"\n"
            "\nCreates and broadcasts a property creation transaction (fixed issuance) with the supplied details.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send from\n"
            "Ecosystem     : the ecosystem to create the property - (1) main, (2) test\n"
            "Type          : the type of tokens - (1) indivisible, (2) divisible\n"
            "PreviousID    : the previous property id (0 for a new property)\n"
            "Category      : The category for the new property (max 255 chars)\n"
            "Subcategory   : the subcategory for the new property (max 255 chars)\n"
            "Name          : the name of the new property (max 255 chars)\n"
            "URL           : the URL for the new property (max 255 chars)\n"
            "Data          : additional data for the new property (max 255 chars)\n"
            "Amount        : the number of tokens to create\n"
            "\nResult:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendissuancefixed_OMNI \"1FromAddress\" Ecosystem Type PreviousID \"Category\" \"Subcategory\" \"Name\" \"URL\" \"Data\" \"Amount\"\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint8_t ecosystem = ParseEcosystem(params[1]);
    uint16_t type = ParsePropertyType(params[2]);
    uint32_t previousId = ParsePreviousPropertyId(params[3]);
    std::string category = ParseText(params[4]);
    std::string subcategory = ParseText(params[5]);
    std::string name = ParseText(params[6]);
    std::string url = ParseText(params[7]);
    std::string data = ParseText(params[8]);
    int64_t amount = ParseAmount(params[9], type);

    // perform checks
    if (name.empty()) throw JSONRPCError(RPC_TYPE_ERROR, "Property name cannot be empty");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_IssuanceFixed(ecosystem, type, previousId, category, subcategory, name, url, data, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return txid.GetHex();
        }
    }
}

// sendissuancemanual_OMNI - Issue new property with manual issuance (grant/revoke)
Value sendissuancemanaged_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 9)
        throw runtime_error(
            "sendissuancemanual_OMNI \"fromaddress\" ecosystem type previousid \"category\" \"subcategory\" \"name\" \"url\" \"data\"\n"
            "\nCreates and broadcasts a property creation transaction (managed issuance) with the supplied details.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send from\n"
            "Ecosystem     : the ecosystem to create the property - (1) main, (2) test\n"
            "Type          : the type of tokens - (1) indivisible, (2) divisible\n"
            "PreviousID    : the previous property id (0 for a new property)\n"
            "Category      : The category for the new property (max 255 chars)\n"
            "Subcategory   : the subcategory for the new property (max 255 chars)\n"
            "Name          : the name of the new property (max 255 chars)\n"
            "URL           : the URL for the new property (max 255 chars)\n"
            "Data          : additional data for the new property (max 255 chars)\n"
            "\nResult:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendissuancemanual_OMNI \"1FromAddress\" Ecosystem Type PreviousID \"Category\" \"Subcategory\" \"Name\" \"URL\" \"Data\"\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint8_t ecosystem = ParseEcosystem(params[1]);
    uint16_t type = ParsePropertyType(params[2]);
    uint32_t previousId = ParsePreviousPropertyId(params[3]);
    std::string category = ParseText(params[4]);
    std::string subcategory = ParseText(params[5]);
    std::string name = ParseText(params[6]);
    std::string url = ParseText(params[7]);
    std::string data = ParseText(params[8]);

    // perform checks
    if (name.empty()) throw JSONRPCError(RPC_TYPE_ERROR, "Property name cannot be empty");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_IssuanceManaged(ecosystem, type, previousId, category, subcategory, name, url, data);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return txid.GetHex();
        }
    }
}

// sendsto_OMNI - Send to owners
Value sendsto_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 4)
        throw runtime_error(
            "sendsto_OMNI \"fromaddress\" propertyid \"amount\" ( \"redeemaddress\" )\n"
            "\nCreates and broadcasts a send-to-owners transaction for a given amount and currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send from\n"
            "PropertyID    : the id of the smart property to send\n"
            "Amount (string): the amount to send\n"
            "RedeemAddress : (optional) the address that can redeem class B data outputs. Defaults to FromAddress\n"
            "\nResult:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendsto_OMNI 1FromAddress PropertyID Amount\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);
    int64_t amount = ParseAmount(params[2], isPropertyDivisible(propertyId));
    std::string redeemAddress = (params.size() > 3) ? ParseAddress(params[3]): "";

    const int64_t senderBalance = getMPbalance(fromAddress, propertyId, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyId);

    // perform checks
    if (senderBalance < amount) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance");
    if (senderAvailableBalance < amount) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance (due to pending transactions)");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SendToOwners(propertyId, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", redeemAddress, 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            PendingAdd(txid, fromAddress, "", MSC_TYPE_SEND_TO_OWNERS, propertyId, amount);
            return txid.GetHex();
        }
    }
}

// sendgrant_OMNI - Grant tokens
Value sendgrant_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 5)
        throw runtime_error(
            "sendgrant_OMNI \"fromaddress\" \"toaddress\" propertyid \"amount\" ( \"memo\" )\n"
            "\nCreates and broadcasts a token grant for a given amount and currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send this transaction from\n"
            "ToAddress     : the address to send the granted tokens to (defaults to FromAddress)\n"
            "PropertyID    : the id of the smart property to grant\n"
            "Amount        : the amount to grant\n"
            "Memo          : (optional) attach a text note to this transaction (max 255 chars)\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendgrant_OMNI \"1FromAddress\" \"1ToAddress\" PropertyID Amount\n"
            ">omnicored sendgrant_OMNI \"1FromAddress\" \"\" PropertyID Amount \"Grant tokens to the sending address and attach this note\"\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseText(params[1]).size() ? ParseAddress(params[1]): "";
    uint32_t propertyId = ParsePropertyId(params[2]);
    int64_t amount = ParseAmount(params[3], isPropertyDivisible(propertyId));
    std::string memo = (params.size() > 4) ? ParseText(params[4]): "";

    // perform conversions
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");

    // perform checks
    if (fromAddress != sp.issuer) throw JSONRPCError(RPC_TYPE_ERROR, "Sender is not authorized to grant tokens for this property");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_Grant(propertyId, amount, memo);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, toAddress, "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return txid.GetHex();
        }
    }
}

// sendrevoke_OMNI - Revoke tokens
Value sendrevoke_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 4)
        throw runtime_error(
            "sendrevoke_OMNI \"fromaddress\" propertyid \"amount\" ( \"memo\" )\n"
            "\nCreates and broadcasts a token revoke for a given amount and currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send the transaction from\n"
            "PropertyID    : the id of the smart property to revoke\n"
            "Amount        : the amount to revoke\n"
            "Memo          : (optional) attach a text note to this transaction (max 255 chars)\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendrevoke_OMNI \"1FromAddress\" PropertyID Amount\n"
            ">omnicored sendrevoke_OMNI \"1FromAddress\" PropertyID Amount \"Revoke tokens from the sending address and attach this note\"\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);
    int64_t amount = ParseAmount(params[2], isPropertyDivisible(propertyId));
    std::string memo = (params.size() > 3) ? ParseText(params[3]): "";

    const int64_t senderBalance = getMPbalance(fromAddress, propertyId, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyId);

    // perform conversions
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");

    // perform checks
    if (fromAddress != sp.issuer) throw JSONRPCError(RPC_TYPE_ERROR, "Sender is not authorized to revoke tokens for this property");
    if (senderBalance < amount) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance");
    if (senderAvailableBalance < amount) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance (due to pending transactions)");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_Revoke(propertyId, amount, memo);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return txid.GetHex();
        }
    }
}

// sendclosecrowdsale_OMNI - Close an active crowdsale
Value sendclosecrowdsale_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "sendclosecrowdsale_OMNI \"fromaddress\" propertyid\n"
            "\nCreates and broadcasts a close crowdsale message for a given currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send this transaction from\n"
            "PropertyID    : the id of the smart property to close the crowdsale\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendclosecrowdsale_OMNI \"1FromAddress\" PropertyID\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);

    // perform conversions
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");

    // perform checks
    if (!isCrowdsaleActive(propertyId)) throw JSONRPCError(RPC_TYPE_ERROR, "The specified property does not have a crowdsale active");
    if (fromAddress != sp.issuer) throw JSONRPCError(RPC_TYPE_ERROR, "Sender is not authorized to close the crowdsale for this property");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_CloseCrowdsale(propertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return txid.GetHex();
        }
    }
}

// trade_MP - MetaDEx trade
Value trade_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error(
            "trade_MP \"fromaddress\" propertyidforsale \"amountforsale\" propertiddesired \"amountdesired\" action\n"
            "\nNote: this command is depreciated, and was replaced by:\n"
            " - sendtrade_OMNI\n"
            " - sendcanceltradebyprice_OMNI\n"
            " - sendcanceltradebypair_OMNI\n"
            " - sendcanceltradebypair_OMNI\n"
        );

    Array values;
    uint8_t action = ParseMetaDExAction(params[5]);

    // Forward to the new commands, based on action value
    switch (action) {
        case CMPTransaction::ADD:
        {
            values.push_back(params[0]); // fromAddress
            values.push_back(params[1]); // propertyIdForSale
            values.push_back(params[2]); // amountForSale
            values.push_back(params[3]); // propertyIdDesired
            values.push_back(params[4]); // amountDesired
            return sendtrade_OMNI(values, fHelp);
        }
        case CMPTransaction::CANCEL_AT_PRICE:
        {
            values.push_back(params[0]); // fromAddress
            values.push_back(params[1]); // propertyIdForSale
            values.push_back(params[2]); // amountForSale
            values.push_back(params[3]); // propertyIdDesired
            values.push_back(params[4]); // amountDesired
            return sendcanceltradesbyprice_OMNI(values, fHelp);
        }
        case CMPTransaction::CANCEL_ALL_FOR_PAIR:
        {
            values.push_back(params[0]); // fromAddress
            values.push_back(params[1]); // propertyIdForSale
            values.push_back(params[3]); // propertyIdDesired
            return sendcanceltradesbypair_OMNI(values, fHelp);
        }
        case CMPTransaction::CANCEL_EVERYTHING:
        {
            uint8_t ecosystem = 0;
            if (isMainEcosystemProperty(params[1].get_int64())
                    && isMainEcosystemProperty(params[3].get_int64())) {
                ecosystem = OMNI_PROPERTY_MSC;
            }
            if (isTestEcosystemProperty(params[1].get_int64())
                    && isTestEcosystemProperty(params[3].get_int64())) {
                ecosystem = OMNI_PROPERTY_TMSC;
            }
            values.push_back(params[0]); // fromAddress
            values.push_back(ecosystem);
            return sendcancelalltrades_OMNI(values, fHelp);
        }
    }

    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid action (1,2,3,4 only)");
}

// Send a new MetaDEx trade
Value sendtrade_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 5)
        throw runtime_error(
            "sendtrade_OMNI \"fromaddress\" propertyidforsale \"amountforsale\" propertiddesired \"amountdesired\"\n"
            "\nPlace a trade offer on the distributed token exchange.\n"
            "\nParameters:\n"
            "FromAddress         : the address to send this transaction from\n"
            "PropertyIDForSale   : the property to list for sale\n"
            "AmountForSale       : the amount to list for sale\n"
            "PropertyIDDesired   : the property desired\n"
            "AmountDesired       : the amount desired\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendtrade_OMNI \"1FromAddress\" PropertyIDForSale \"AmountForSale\" PropertyIDDesired \"AmountDesired\"\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(params[1]);
    int64_t amountForSale = ParseAmount(params[2], isPropertyDivisible(propertyIdForSale));
    uint32_t propertyIdDesired = ParsePropertyId(params[3]);
    int64_t amountDesired = ParseAmount(params[4], isPropertyDivisible(propertyIdDesired));

    CMPSPInfo::Entry spForSale;
    CMPSPInfo::Entry spDesired;
    if (false == _my_sps->getSP(propertyIdForSale, spForSale)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale does not exist");
    if (false == _my_sps->getSP(propertyIdDesired, spDesired)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property desired does not exist");
    if (isTestEcosystemProperty(propertyIdForSale) != isTestEcosystemProperty(propertyIdDesired)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale and property desired must be in the same ecosystem");
    if (propertyIdForSale == propertyIdDesired) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale and property desired must be different");

    const int64_t senderBalance = getMPbalance(fromAddress, propertyIdForSale, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyIdForSale);
    if (senderBalance < amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance");
    if (senderAvailableBalance < amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance (due to pending transactions)");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExTrade(propertyIdForSale, amountForSale, propertyIdDesired, amountDesired);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            uint8_t action = CMPTransaction::ADD; // TODO: move into pending creation
            PendingAdd(txid, fromAddress, "", MSC_TYPE_METADEX, propertyIdForSale, amountForSale, propertyIdDesired, amountDesired, action);
            return txid.GetHex();
        }
    }
}

// Cancel MetaDEx by price
Value sendcanceltradesbyprice_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 5)
        throw runtime_error(
            "sendcanceltradesbyprice_OMNI \"fromaddress\" propertyidforsale \"amountforsale\" propertiddesired \"amountdesired\"\n"
            "\nCancel offers on the distributed token exchange with the given price.\n"
            "\nParameters:\n"
            "FromAddress         : the address to send this transaction from\n"
            "PropertyIDForSale   : the property listed for sale\n"
            "AmountForSale       : the amount listed for sale\n"
            "PropertyIDDesired   : the property desired\n"
            "AmountDesired       : the desired amount\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendcanceltradesbyprice_OMNI \"1FromAddress\" PropertyIDForSale \"AmountForSale\" PropertyIDDesired \"AmountDesired\"\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(params[1]);
    int64_t amountForSale = ParseAmount(params[2], isPropertyDivisible(propertyIdForSale));
    uint32_t propertyIdDesired = ParsePropertyId(params[3]);
    int64_t amountDesired = ParseAmount(params[4], isPropertyDivisible(propertyIdDesired));

    CMPSPInfo::Entry spForSale;
    CMPSPInfo::Entry spDesired;
    if (false == _my_sps->getSP(propertyIdForSale, spForSale)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale does not exist");
    if (false == _my_sps->getSP(propertyIdDesired, spDesired)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property desired does not exist");
    if (isTestEcosystemProperty(propertyIdForSale) != isTestEcosystemProperty(propertyIdDesired)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale and property desired must be in the same ecosystem");
    if (propertyIdForSale == propertyIdDesired) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale and property desired must be different");

    // TODO: check, if there are matching offers to cancel

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelPrice(propertyIdForSale, amountForSale, propertyIdDesired, amountDesired);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            uint8_t action = CMPTransaction::CANCEL_AT_PRICE; // TODO: move into pending creation
            PendingAdd(txid, fromAddress, "", MSC_TYPE_METADEX, propertyIdForSale, amountForSale, propertyIdDesired, amountDesired, action);
            return txid.GetHex();
        }
    }
}

// Cancel MetaDEx orders by currency pair
Value sendcanceltradesbypair_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "sendcanceltradesbypair_OMNI \"fromaddress\" propertyidforsale propertiddesired\n"
            "\nCancel offers on the distributed token exchange with the given currency pair.\n"
            "\nParameters:\n"
            "FromAddress         : the address to send this transaction from\n"
            "PropertyIDForSale   : the property listed for sale\n"
            "PropertyIDDesired   : the property desired\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendcanceltradesbypair_OMNI \"1FromAddress\" PropertyIDForSale \"AmountForSale\" PropertyIDDesired \"AmountDesired\"\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(params[1]);
    uint32_t propertyIdDesired = ParsePropertyId(params[2]);

    CMPSPInfo::Entry spForSale;
    CMPSPInfo::Entry spDesired;
    if (false == _my_sps->getSP(propertyIdForSale, spForSale)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale does not exist");
    if (false == _my_sps->getSP(propertyIdDesired, spDesired)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property desired does not exist");
    if (isTestEcosystemProperty(propertyIdForSale) != isTestEcosystemProperty(propertyIdDesired)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale and property desired must be in the same ecosystem");
    if (propertyIdForSale == propertyIdDesired) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale and property desired must be different");

    // TODO: check, if there are matching offers to cancel

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelPair(propertyIdForSale, propertyIdDesired);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            uint8_t action = CMPTransaction::CANCEL_ALL_FOR_PAIR; // TODO: move into pending creation
            int64_t amountForSale = 0;
            int64_t amountDesired = 0;
            PendingAdd(txid, fromAddress, "", MSC_TYPE_METADEX, propertyIdForSale, amountForSale, propertyIdDesired, amountDesired, action);
            return txid.GetHex();
        }
    }
}

// Cancel MetaDEx orders by ecosystem
Value sendcancelalltrades_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "sendcancelalltrades_OMNI \"fromaddress\" ecosystem\n"
            "\nCancel all offers on the distributed token exchange in the given ecosystem.\n"
            "\nParameters:\n"
            "FromAddress         : the address to send this transaction from\n"
            "Ecosystem           : the ecosystem of the offers to cancel - (0) both, (1) main, (2) test\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendcancelalltrades_OMNI \"1FromAddress\" Ecosystem\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint8_t ecosystem = ParseEcosystem(params[1]);

    // TODO: check, if there are matching offers to cancel

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelEcosystem(ecosystem);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            uint8_t action = CMPTransaction::CANCEL_EVERYTHING; // TODO: move into pending creation
            int64_t amountForSale = 0;
            int64_t amountDesired = 0;
            uint32_t propertyIdForSale = ecosystem;
            uint32_t propertyIdDesired = ecosystem;
            PendingAdd(txid, fromAddress, "", MSC_TYPE_METADEX, propertyIdForSale, amountForSale, propertyIdDesired, amountDesired, action);
            return txid.GetHex();
        }
    }
}

// sendchangeissuer_OMNI - Change issuer for a property
Value sendchangeissuer_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "sendchangeissuer_OMNI \"fromaddress\" \"toaddress\" propertyid\n"
            "\nCreates and broadcasts a change issuer message for a given currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send this transaction from\n"
            "ToAddress     : the address to transfer administrative control for this property to\n"
            "PropertyID    : the id of the smart property to change issuer\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendchangeissuer_OMNI \"1FromAddress\" \"1ToAddress\" PropertyID\n"
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseAddress(params[1]);
    uint32_t propertyId = ParsePropertyId(params[2]);

    // perform conversions
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");

    // perform checks
    if (fromAddress != sp.issuer) throw JSONRPCError(RPC_TYPE_ERROR, "Sender is not authorized to transfer admnistration of this property");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_ChangeIssuer(propertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, toAddress, "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return txid.GetHex();
        }
    }
}


