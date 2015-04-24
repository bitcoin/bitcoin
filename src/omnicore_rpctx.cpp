// RPC calls for creating and sending Omni transactions

#include "omnicore_rpctx.h"

#include "mastercore.h"
#include "mastercore_convert.h"
#include "mastercore_errors.h"
#include "mastercore_parse_string.h"
#include "mastercore_sp.h"
#include "mastercore_dex.h"
#include "omnicore_createpayload.h"
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

void CreatePendingObject(uint256 txid, const string& sendingAddress, const string& referenceAddress, uint16_t type, uint32_t propertyId, int64_t amount, uint32_t propertyIdDesired = 0, int64_t amountDesired = 0, int64_t action = 0)
{
    Object txobj;
    string amountStr, amountDStr;
    bool divisible;
    bool divisibleDesired;
    txobj.push_back(Pair("txid", txid.GetHex()));
    txobj.push_back(Pair("sendingaddress", sendingAddress));
    if (!referenceAddress.empty()) txobj.push_back(Pair("referenceaddress", referenceAddress));
    txobj.push_back(Pair("confirmations", 0));
    txobj.push_back(Pair("type", c_strMasterProtocolTXType(type)));
    switch (type) {
        case MSC_TYPE_SIMPLE_SEND:
            txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
            divisible = isPropertyDivisible(propertyId);
            txobj.push_back(Pair("divisible", divisible));
            if (divisible) { amountStr = FormatDivisibleMP(amount); } else { amountStr = FormatIndivisibleMP(amount); }
            txobj.push_back(Pair("amount", amountStr));
        break;
        case MSC_TYPE_SEND_TO_OWNERS:
            txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
            divisible = isPropertyDivisible(propertyId);
            txobj.push_back(Pair("divisible", divisible));
            if (divisible) { amountStr = FormatDivisibleMP(amount); } else { amountStr = FormatIndivisibleMP(amount); }
            txobj.push_back(Pair("amount", amountStr));
        break;
        case MSC_TYPE_TRADE_OFFER:
            amountStr = FormatDivisibleMP(amount); // both amount for sale (either TMSC or MSC) and BTC desired are always divisible
            amountDStr = FormatDivisibleMP(amountDesired);
            txobj.push_back(Pair("amountoffered", amountStr));
            txobj.push_back(Pair("propertyidoffered", (uint64_t)propertyId));
            txobj.push_back(Pair("btcamountdesired", amountDStr));
            txobj.push_back(Pair("action", action));
        break;
        case MSC_TYPE_METADEX:
            divisible = isPropertyDivisible(propertyId);
            divisibleDesired = isPropertyDivisible(propertyIdDesired);
            if (divisible) { amountStr = FormatDivisibleMP(amount); } else { amountStr = FormatIndivisibleMP(amount); }
            if (divisibleDesired) { amountDStr = FormatDivisibleMP(amountDesired); } else { amountDStr = FormatIndivisibleMP(amountDesired); }
            txobj.push_back(Pair("amountoffered", amountStr));
            txobj.push_back(Pair("propertyidoffered", (uint64_t)propertyId));
            txobj.push_back(Pair("propertyidofferedisdivisible", divisible));
            txobj.push_back(Pair("amountdesired", amountDStr));
            txobj.push_back(Pair("propertyiddesired", (uint64_t)propertyIdDesired));
            txobj.push_back(Pair("propertyiddesiredisdivisible", divisibleDesired));
            txobj.push_back(Pair("action", action));
        break;
    }
    string txDesc = write_string(Value(txobj), false);
    (void) pendingAdd(txid, sendingAddress, propertyId, amount, type, txDesc);
}

uint32_t int64Touint32Safe(int64_t sourceValue)
{
    if ((0 > sourceValue) || (4294967295 < sourceValue)) return 0; // not safe to do conversion
    unsigned int destValue = int(sourceValue);
    return destValue;
}

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
    std::string fromAddress = (params[0].get_str());
    std::string toAddress = (params[1].get_str());
    unsigned int propertyId = int64Touint32Safe(params[2].get_int64());
    string strAmount = params[3].get_str();
    std::string redeemAddress = (params.size() > 4) ? (params[4].get_str()): "";
    std::string strReferenceAmount = (params.size() > 5) ? (params[5].get_str()): "0";
    const int64_t senderBalance = getMPbalance(fromAddress, propertyId, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyId);

    // perform conversions
    int64_t amount = 0, referenceAmount = 0;
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    amount = StrToInt64(strAmount, sp.isDivisible());
    referenceAmount = StrToInt64(strReferenceAmount, true);

    // perform checks
    if (0 >= amount) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if ((0.01 * COIN) < referenceAmount) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid reference amount");
    if (!isRangeOK(amount)) throw JSONRPCError(RPC_TYPE_ERROR, "Input not in range");
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
            CreatePendingObject(txid, fromAddress, toAddress, MSC_TYPE_SIMPLE_SEND, propertyId, amount);
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
    std::string fromAddress = params[0].get_str();
    unsigned int propertyIdForSale = int64Touint32Safe(params[1].get_int64());
    string strAmountForSale = params[2].get_str();
    string strAmountDesired = params[3].get_str();
    int64_t paymentWindow = params[4].get_int64();
    int64_t minAcceptFee = StrToInt64(params[5].get_str(), true); // BTC so always divisible
    int64_t action = params[6].get_int64();
    const int64_t senderBalance = getMPbalance(fromAddress, propertyIdForSale, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyIdForSale);

    // perform conversions
    int64_t amountForSale = 0, amountDesired = 0;
    if ((propertyIdForSale > 2 || propertyIdForSale <=0)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid propertyID for sale - only 1 and 2 are permitted");
    amountForSale = StrToInt64(strAmountForSale, true); // TMSC/MSC always divisible
    amountDesired = StrToInt64(strAmountDesired, true); // BTC so always divisible

    // perform checks
    if (action <= 0 || 3 < action) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid action (1,2,3 only)");
    if (action <= 2) { // actions 3 permit zero values, skip check
        if (0 >= amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for sale");
        if (!isRangeOK(amountForSale)) throw JSONRPCError(RPC_TYPE_ERROR, "Amount for sale not in range");
        if (0 >= amountDesired) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount desired");
        if (!isRangeOK(amountDesired)) throw JSONRPCError(RPC_TYPE_ERROR, "Amount desired not in range");
    }
    if (action != 3) { // only check for sufficient balance for new/update sell offers
        if (senderBalance < amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance");
        if (senderAvailableBalance < amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance (due to pending transactions)");
    }
    if (minAcceptFee <= 0) throw JSONRPCError(RPC_TYPE_ERROR, "Mininmum accept mining fee invalid");
    if ((paymentWindow <= 0) || (paymentWindow > 255)) throw JSONRPCError(RPC_TYPE_ERROR, "Payment window invalid");
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
            CreatePendingObject(txid, fromAddress, "", MSC_TYPE_TRADE_OFFER, propertyIdForSale, amountForSale, 0, amountDesired, action);
            return txid.GetHex();
        }
    }
}

// senddexaccept_OMNI - DEx accept offer
Value senddexaccept_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4 )
        throw runtime_error(
            "senddexaccept_OMNI \"fromaddress\" \"toaddress\" propertyid \"amount\"\n"
            "\nCreates and broadcasts an accept offer for a given amount and currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send from\n"
            "ToAddress     : the address to send the accept to\n"
            "PropertyID    : the id of the property to accept\n"
            "Amount        : the amount to accept\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored senddexaccept_OMNI 1FromAddress 1ToAddress PropertyID Amount\n"
        );

    // obtain parameters & info
    std::string fromAddress = (params[0].get_str());
    std::string toAddress = (params[1].get_str());
    unsigned int propertyId = int64Touint32Safe(params[2].get_int64());
    string strAmount = params[3].get_str();

    // perform conversions
    int64_t amount = 0;
    if ((propertyId > 2 || propertyId <=0)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid propertyID - only 1 and 2 are permitted");
    amount = StrToInt64(strAmount, true); // MSC/TMSC always divisible

    // perform checks
    if (0 >= amount) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if (!isRangeOK(amount)) throw JSONRPCError(RPC_TYPE_ERROR, "Input not in range");
    if (!DEx_offerExists(toAddress, propertyId)) throw JSONRPCError(RPC_TYPE_ERROR, "There is no matching sell offer on the distributed exchange");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExAccept(propertyId, amount);

    // retrieve the sell we're accepting and use new 0.10 custom fee to set the accept minimum fee appropriately
    // WARNING: exploitable?  Is automatic fee retrieval and application appropriate or should we force the user
    //          to specify explicitly what fee to use?  For now automatic fee includes hard limitation of 0.001 BTC.
    CMPOffer *sellOffer = DEx_getOffer(toAddress, propertyId);
    if (sellOffer == NULL) throw JSONRPCError(RPC_TYPE_ERROR, "Unable to load sell offer from the distributed exchange");
    int64_t nMinimumAcceptFee = sellOffer->getMinFee();
    if (nMinimumAcceptFee > 100000) throw JSONRPCError(RPC_TYPE_ERROR, "Minimum accept fee is above threshold");
    CFeeRate payTxFeeOriginal = payTxFee;
    bool fPayAtLeastCustomFeeOriginal = fPayAtLeastCustomFee;
    payTxFee = CFeeRate(nMinimumAcceptFee, 1000);
    fPayAtLeastCustomFee = true;

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
    std::string fromAddress = params[0].get_str();
    int64_t ecosystem = params[1].get_int64();
    int64_t type = params[2].get_int64();
    unsigned int previousId = int64Touint32Safe(params[3].get_int64());
    std::string category = params[4].get_str();
    std::string subcategory = params[5].get_str();
    std::string name = params[6].get_str();
    std::string url = params[7].get_str();
    std::string data = params[8].get_str();
    unsigned int propertyIdDesired = int64Touint32Safe(params[9].get_int64());
    std::string numTokensStr = params[10].get_str();
    int64_t deadline = params[11].get_int64();
    int64_t earlyBonus = params[12].get_int64();
    int64_t issuerPercentage = params[13].get_int64();

    // perform conversions
    int64_t numTokens = 0;
    if (type == 1) {
        numTokens = StrToInt64(numTokensStr, false);
    } else { // only type 1 and 2 supported currently
        numTokens = StrToInt64(numTokensStr, true);
    }

    // perform checks
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyIdDesired, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property desired does not exist");
    if ((type > 2) || (type <= 0)) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid type");
    if ((ecosystem > 2) || (ecosystem <= 0)) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid ecosystem");
    if (previousId != 0) throw JSONRPCError(RPC_TYPE_ERROR, "Property appends/replaces are not yet supported");
    if (name.empty()) throw JSONRPCError(RPC_TYPE_ERROR, "Property name cannot be empty");
    if ((earlyBonus <=0) || (earlyBonus > 255)) throw JSONRPCError(RPC_TYPE_ERROR, "Early bonus must be in the range 1-255");
    if ((issuerPercentage <=0) || (issuerPercentage > 255)) throw JSONRPCError(RPC_TYPE_ERROR, "Issuer percentage must be in the range 1-255");
    if (0 >= numTokens) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid number of tokens per unit");
    if (!isRangeOK(numTokens)) throw JSONRPCError(RPC_TYPE_ERROR, "Input not in range");

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
    std::string fromAddress = params[0].get_str();
    int64_t ecosystem = params[1].get_int64();
    int64_t type = params[2].get_int64();
    unsigned int previousId = int64Touint32Safe(params[3].get_int64());
    std::string category = params[4].get_str();
    std::string subcategory = params[5].get_str();
    std::string name = params[6].get_str();
    std::string url = params[7].get_str();
    std::string data = params[8].get_str();
    string strAmount = params[9].get_str();

    // perform conversions
    int64_t amount = 0;
    if (type == 1) {
        amount = StrToInt64(strAmount, false);
    } else { // only type 1 and 2 supported currently
        amount = StrToInt64(strAmount, true);
    }

    // perform checks
    if ((type > 2) || (type <= 0)) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid type");
    if ((ecosystem > 2) || (ecosystem <= 0)) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid ecosystem");
    if (previousId != 0) throw JSONRPCError(RPC_TYPE_ERROR, "Property appends/replaces are not yet supported");
    if (name.empty()) throw JSONRPCError(RPC_TYPE_ERROR, "Property name cannot be empty");
    if (0 >= amount) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if (!isRangeOK(amount)) throw JSONRPCError(RPC_TYPE_ERROR, "Input not in range");

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
    std::string fromAddress = params[0].get_str();
    int64_t ecosystem = params[1].get_int64();
    int64_t type = params[2].get_int64();
    unsigned int previousId = int64Touint32Safe(params[3].get_int64());
    std::string category = params[4].get_str();
    std::string subcategory = params[5].get_str();
    std::string name = params[6].get_str();
    std::string url = params[7].get_str();
    std::string data = params[8].get_str();

    // perform checks
    if ((type > 2) || (type <= 0)) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid type");
    if ((ecosystem > 2) || (ecosystem <= 0)) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid ecosystem");
    if (previousId != 0) throw JSONRPCError(RPC_TYPE_ERROR, "Property appends/replaces are not yet supported");
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
    std::string fromAddress = (params[0].get_str());
    unsigned int propertyId = int64Touint32Safe(params[1].get_int64());
    string strAmount = params[2].get_str();
    std::string redeemAddress = (params.size() > 3) ? (params[3].get_str()): "";
    const int64_t senderBalance = getMPbalance(fromAddress, propertyId, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyId);

    // perform conversions
    int64_t amount = 0;
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    amount = StrToInt64(strAmount, sp.isDivisible());

    // perform checks
    if (0 >= amount) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if (!isRangeOK(amount)) throw JSONRPCError(RPC_TYPE_ERROR, "Input not in range");
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
            CreatePendingObject(txid, fromAddress, "", MSC_TYPE_SEND_TO_OWNERS, propertyId, amount);
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
    std::string fromAddress = (params[0].get_str());
    std::string toAddress = (params[1].get_str());
    unsigned int propertyId = int64Touint32Safe(params[2].get_int64());
    string strAmount = params[3].get_str();
    std::string memo = (params.size() > 4) ? (params[4].get_str()): "";

    // perform conversions
    int64_t amount = 0;
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    amount = StrToInt64(strAmount, sp.isDivisible());

    // perform checks
    if (0 >= amount) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if (!isRangeOK(amount)) throw JSONRPCError(RPC_TYPE_ERROR, "Input not in range");
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
    std::string fromAddress = (params[0].get_str());
    unsigned int propertyId = int64Touint32Safe(params[1].get_int64());
    string strAmount = params[2].get_str();
    std::string memo = (params.size() > 3) ? (params[3].get_str()): "";
    const int64_t senderBalance = getMPbalance(fromAddress, propertyId, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyId);

    // perform conversions
    int64_t amount = 0;
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    amount = StrToInt64(strAmount, sp.isDivisible());

    // perform checks
    if (0 >= amount) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if (!isRangeOK(amount)) throw JSONRPCError(RPC_TYPE_ERROR, "Input not in range");
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
    std::string fromAddress = (params[0].get_str());
    unsigned int propertyId = int64Touint32Safe(params[1].get_int64());

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

// sendtrade_OMNI - MetaDEx trade
Value sendtrade_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error(
            "sendtrade_OMNI \"fromaddress\" propertyidforsale \"amountforsale\" propertiddesired \"amountdesired\" action\n"
            "\nPlace or cancel a trade offer on the distributed token exchange.\n"
            "\nParameters:\n"
            "FromAddress         : the address to send this transaction from\n"
            "PropertyIDForSale   : the property to list for sale\n"
            "AmountForSale       : the amount to list for sale\n"
            "PropertyIDDesired   : the property desired\n"
            "AmountDesired       : the amount desired\n"
            "Action              : the action to take: (1) new, (2) cancel by price, (3) cancel by pair, (4) cancel all\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">omnicored sendtrade_OMNI \"1FromAddress\" PropertyIDForSale \"AmountForSale\" PropertyIDDesired \"AmountDesired\" Action\n"
        );

    // obtain parameters & info
    std::string fromAddress = params[0].get_str();
    unsigned int propertyIdForSale = int64Touint32Safe(params[1].get_int64());
    string strAmountForSale = params[2].get_str();
    unsigned int propertyIdDesired = int64Touint32Safe(params[3].get_int64());
    string strAmountDesired = params[4].get_str();
    int64_t action = params[5].get_int64();
    const int64_t senderBalance = getMPbalance(fromAddress, propertyIdForSale, BALANCE);
    const int64_t senderAvailableBalance = getUserAvailableMPbalance(fromAddress, propertyIdForSale);


    // perform conversions
    int64_t amountForSale = 0, amountDesired = 0;
    CMPSPInfo::Entry spForSale;
    if (false == _my_sps->getSP(propertyIdForSale, spForSale)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale does not exist");
    CMPSPInfo::Entry spDesired;
    if (false == _my_sps->getSP(propertyIdDesired, spDesired)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property desired does not exist");
    amountForSale = StrToInt64(strAmountForSale, spForSale.isDivisible());
    amountDesired = StrToInt64(strAmountDesired, spDesired.isDivisible());

    // perform checks
    if (action <= 0 || 4 < action) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid action (1,2,3,4 only)");
    if (action <= 2) { // actions 3 and 4 permit zero values, skip check
        if (0 >= amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for sale");
        if (!isRangeOK(amountForSale)) throw JSONRPCError(RPC_TYPE_ERROR, "Amount for sale not in range");
        if (0 >= amountDesired) throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount desired");
        if (!isRangeOK(amountDesired)) throw JSONRPCError(RPC_TYPE_ERROR, "Amount desired not in range");
    }
    if (action == 1) { // only check for sufficient balance for new trades
        if (senderBalance < amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance");
        if (senderAvailableBalance < amountForSale) throw JSONRPCError(RPC_TYPE_ERROR, "Sender has insufficient balance (due to pending transactions)");
    }
    if (isTestEcosystemProperty(propertyIdForSale) != isTestEcosystemProperty(propertyIdDesired)) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property fpr sale and property desired must be in the same ecosystem");
    if (propertyIdForSale == propertyIdDesired) throw JSONRPCError(RPC_INVALID_PARAMETER, "Property for sale and property desired must be different");

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExTrade(propertyIdForSale, amountForSale, propertyIdDesired, amountDesired, action);

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
            CreatePendingObject(txid, fromAddress, "", MSC_TYPE_METADEX, propertyIdForSale, amountForSale, propertyIdDesired, amountDesired, action);
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
    std::string fromAddress = (params[0].get_str());
    std::string toAddress = (params[1].get_str());
    unsigned int propertyId = int64Touint32Safe(params[2].get_int64());

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


