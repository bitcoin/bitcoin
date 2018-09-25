/**
 * @file rpctx.cpp
 *
 * This file contains RPC calls for creating and sending Omni transactions.
 */

#include "omnicore/rpctx.h"

#include "omnicore/createpayload.h"
#include "omnicore/dex.h"
#include "omnicore/errors.h"
#include "omnicore/omnicore.h"
#include "omnicore/pending.h"
#include "omnicore/rpcrequirements.h"
#include "omnicore/rpcvalues.h"
#include "omnicore/sp.h"
#include "omnicore/tx.h"
#include "omnicore/wallettxbuilder.h"

#include "init.h"
#include "main.h"
#include "rpc/server.h"
#include "sync.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <univalue.h>

#include <stdint.h>
#include <stdexcept>
#include <string>

using std::runtime_error;
using namespace mastercore;


UniValue omni_funded_send(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 5)
        throw runtime_error(
            "omni_funded_send \"fromaddress\" \"toaddress\" propertyid \"amount\" \"feeaddress\"\n"

            "\nCreates and sends a funded simple send transaction.\n"

            "\nAll bitcoins from the sender are consumed and if there are bitcoins missing, they are taken from the specified fee source. Change is sent to the fee source!\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send the tokens from\n"
            "2. toaddress            (string, required) the address of the receiver\n"
            "3. propertyid           (number, required) the identifier of the tokens to send\n"
            "4. amount               (string, required) the amount to send\n"
            "5. feeaddress           (string, required) the address that is used for change and to pay for fees, if needed\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_funded_send", "\"1DFa5bT6KMEr6ta29QJouainsjaNBsJQhH\" \"15cWrfuvMxyxGst2FisrQcvcpF48x6sXoH\" 1 \"100.0\" \"15Jhzz4omEXEyFKbdcccJwuVPea5LqsKM1\"")
            + HelpExampleRpc("omni_funded_send", "\"1DFa5bT6KMEr6ta29QJouainsjaNBsJQhH\", \"15cWrfuvMxyxGst2FisrQcvcpF48x6sXoH\", 1, \"100.0\", \"15Jhzz4omEXEyFKbdcccJwuVPea5LqsKM1\"")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseAddress(params[1]);
    uint32_t propertyId = ParsePropertyId(params[2]);
    int64_t amount = ParseAmount(params[3], isPropertyDivisible(propertyId));
    std::string feeAddress = ParseAddress(params[4]);

    // perform checks
    RequireExistingProperty(propertyId);
    RequireBalance(fromAddress, propertyId, amount);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SimpleSend(propertyId, amount);

    // create the raw transaction
    uint256 retTxid;
    int result = CreateFundedTransaction(fromAddress, toAddress, feeAddress, payload, retTxid);
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    }

    return retTxid.ToString();
}

UniValue omni_funded_sendall(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error(
            "omni_funded_sendall \"fromaddress\" \"toaddress\" ecosystem \"feeaddress\"\n"

            "\nCreates and sends a transaction that transfers all available tokens in the given ecosystem to the recipient.\n"

            "\nAll bitcoins from the sender are consumed and if there are bitcoins missing, they are taken from the specified fee source. Change is sent to the fee source!\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to the tokens send from\n"
            "2. toaddress            (string, required) the address of the receiver\n"
            "3. ecosystem            (number, required) the ecosystem of the tokens to send (1 for main ecosystem, 2 for test ecosystem)\n"
            "4. feeaddress           (string, required) the address that is used for change and to pay for fees, if needed\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_funded_sendall", "\"1DFa5bT6KMEr6ta29QJouainsjaNBsJQhH\" \"15cWrfuvMxyxGst2FisrQcvcpF48x6sXoH\" 1 \"15Jhzz4omEXEyFKbdcccJwuVPea5LqsKM1\"")
            + HelpExampleRpc("omni_funded_sendall", "\"1DFa5bT6KMEr6ta29QJouainsjaNBsJQhH\", \"15cWrfuvMxyxGst2FisrQcvcpF48x6sXoH\", 1, \"15Jhzz4omEXEyFKbdcccJwuVPea5LqsKM1\"")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseAddress(params[1]);
    uint8_t ecosystem = ParseEcosystem(params[2]);
    std::string feeAddress = ParseAddress(params[3]);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SendAll(ecosystem);

    // create the raw transaction
    uint256 retTxid;
    int result = CreateFundedTransaction(fromAddress, toAddress, feeAddress, payload, retTxid);
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    }

    return retTxid.ToString();
}

UniValue omni_sendrawtx(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw runtime_error(
            "omni_sendrawtx \"fromaddress\" \"rawtransaction\" ( \"referenceaddress\" \"redeemaddress\" \"referenceamount\" )\n"
            "\nBroadcasts a raw Omni Layer transaction.\n"
            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. rawtransaction       (string, required) the hex-encoded raw transaction\n"
            "3. referenceaddress     (string, optional) a reference address (none by default)\n"
            "4. redeemaddress        (string, optional) an address that can spent the transaction dust (sender by default)\n"
            "5. referenceamount      (string, optional) a bitcoin amount that is sent to the receiver (minimal by default)\n"
            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_sendrawtx", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\" \"000000000000000100000000017d7840\" \"1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV\"")
            + HelpExampleRpc("omni_sendrawtx", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\", \"000000000000000100000000017d7840\", \"1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV\"")
        );

    std::string fromAddress = ParseAddress(params[0]);
    std::vector<unsigned char> data = ParseHexV(params[1], "raw transaction");
    std::string toAddress = (params.size() > 2) ? ParseAddressOrEmpty(params[2]): "";
    std::string redeemAddress = (params.size() > 3) ? ParseAddressOrEmpty(params[3]): "";
    int64_t referenceAmount = (params.size() > 4) ? ParseAmount(params[4], true): 0;

    //some sanity checking of the data supplied?
    uint256 newTX;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, redeemAddress, referenceAmount, data, newTX, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return newTX.GetHex();
        }
    }
}

UniValue omni_send(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 6)
        throw runtime_error(
            "omni_send \"fromaddress\" \"toaddress\" propertyid \"amount\" ( \"redeemaddress\" \"referenceamount\" )\n"

            "\nCreate and broadcast a simple send transaction.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. toaddress            (string, required) the address of the receiver\n"
            "3. propertyid           (number, required) the identifier of the tokens to send\n"
            "4. amount               (string, required) the amount to send\n"
            "5. redeemaddress        (string, optional) an address that can spend the transaction dust (sender by default)\n"
            "6. referenceamount      (string, optional) a bitcoin amount that is sent to the receiver (minimal by default)\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_send", "\"3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY\" \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1 \"100.0\"")
            + HelpExampleRpc("omni_send", "\"3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY\", \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1, \"100.0\"")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseAddress(params[1]);
    uint32_t propertyId = ParsePropertyId(params[2]);
    int64_t amount = ParseAmount(params[3], isPropertyDivisible(propertyId));
    std::string redeemAddress = (params.size() > 4 && !ParseText(params[4]).empty()) ? ParseAddress(params[4]): "";
    int64_t referenceAmount = (params.size() > 5) ? ParseAmount(params[5], true): 0;

    // perform checks
    RequireExistingProperty(propertyId);
    RequireBalance(fromAddress, propertyId, amount);
    RequireSaneReferenceAmount(referenceAmount);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SimpleSend(propertyId, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, redeemAddress, referenceAmount, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            PendingAdd(txid, fromAddress, MSC_TYPE_SIMPLE_SEND, propertyId, amount);
            return txid.GetHex();
        }
    }
}

UniValue omni_sendall(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "omni_sendall \"fromaddress\" \"toaddress\" ecosystem ( \"redeemaddress\" \"referenceamount\" )\n"

            "\nTransfers all available tokens in the given ecosystem to the recipient.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. toaddress            (string, required) the address of the receiver\n"
            "3. ecosystem            (number, required) the ecosystem of the tokens to send (1 for main ecosystem, 2 for test ecosystem)\n"
            "4. redeemaddress        (string, optional) an address that can spend the transaction dust (sender by default)\n"
            "5. referenceamount      (string, optional) a bitcoin amount that is sent to the receiver (minimal by default)\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendall", "\"3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY\" \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 2")
            + HelpExampleRpc("omni_sendall", "\"3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY\", \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 2")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseAddress(params[1]);
    uint8_t ecosystem = ParseEcosystem(params[2]);
    std::string redeemAddress = (params.size() > 3 && !ParseText(params[3]).empty()) ? ParseAddress(params[3]): "";
    int64_t referenceAmount = (params.size() > 4) ? ParseAmount(params[4], true): 0;

    // perform checks
    RequireSaneReferenceAmount(referenceAmount);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SendAll(ecosystem);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, redeemAddress, referenceAmount, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            // TODO: pending
            return txid.GetHex();
        }
    }
}

UniValue omni_senddexsell(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 7)
        throw runtime_error(
            "omni_senddexsell \"fromaddress\" propertyidforsale \"amountforsale\" \"amountdesired\" paymentwindow minacceptfee action\n"

            "\nPlace, update or cancel a sell offer on the traditional distributed OMNI/BTC exchange.\n"

            "\nArguments:\n"

            "1. fromaddress          (string, required) the address to send from\n"
            "2. propertyidforsale    (number, required) the identifier of the tokens to list for sale (must be 1 for OMNI or 2 for TOMNI)\n"
            "3. amountforsale        (string, required) the amount of tokens to list for sale\n"
            "4. amountdesired        (string, required) the amount of bitcoins desired\n"
            "5. paymentwindow        (number, required) a time limit in blocks a buyer has to pay following a successful accepting order\n"
            "6. minacceptfee         (string, required) a minimum mining fee a buyer has to pay to accept the offer\n"
            "7. action               (number, required) the action to take (1 for new offers, 2 to update\", 3 to cancel)\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_senddexsell", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1 \"1.5\" \"0.75\" 25 \"0.0005\" 1")
            + HelpExampleRpc("omni_senddexsell", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1, \"1.5\", \"0.75\", 25, \"0.0005\", 1")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(params[1]);
    int64_t amountForSale = 0; // depending on action
    int64_t amountDesired = 0; // depending on action
    uint8_t paymentWindow = 0; // depending on action
    int64_t minAcceptFee = 0;  // depending on action
    uint8_t action = ParseDExAction(params[6]);

    // perform conversions
    if (action <= CMPTransaction::UPDATE) { // actions 3 permit zero values, skip check
        amountForSale = ParseAmount(params[2], true); // TMSC/MSC is divisible
        amountDesired = ParseAmount(params[3], true); // BTC is divisible
        paymentWindow = ParseDExPaymentWindow(params[4]);
        minAcceptFee = ParseDExFee(params[5]);
    }

    // perform checks
    switch (action) {
        case CMPTransaction::NEW:
        {
            RequirePrimaryToken(propertyIdForSale);
            RequireBalance(fromAddress, propertyIdForSale, amountForSale);
            RequireNoOtherDExOffer(fromAddress, propertyIdForSale);
            break;
        }
        case CMPTransaction::UPDATE:
        {
            RequirePrimaryToken(propertyIdForSale);
            RequireBalance(fromAddress, propertyIdForSale, amountForSale);
            RequireMatchingDExOffer(fromAddress, propertyIdForSale);
            break;
        }
        case CMPTransaction::CANCEL:
        {
            RequirePrimaryToken(propertyIdForSale);
            RequireMatchingDExOffer(fromAddress, propertyIdForSale);
            break;
        }
    }

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExSell(propertyIdForSale, amountForSale, amountDesired, paymentWindow, minAcceptFee, action);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            bool fSubtract = (action <= CMPTransaction::UPDATE); // no pending balances for cancels
            PendingAdd(txid, fromAddress, MSC_TYPE_TRADE_OFFER, propertyIdForSale, amountForSale, fSubtract);
            return txid.GetHex();
        }
    }
}

UniValue omni_senddexaccept(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 5)
        throw runtime_error(
            "omni_senddexaccept \"fromaddress\" \"toaddress\" propertyid \"amount\" ( override )\n"

            "\nCreate and broadcast an accept offer for the specified token and amount.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. toaddress            (string, required) the address of the seller\n"
            "3. propertyid           (number, required) the identifier of the token to purchase\n"
            "4. amount               (string, required) the amount to accept\n"
            "5. override             (boolean, optional) override minimum accept fee and payment window checks (use with caution!)\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_senddexaccept", "\"35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8\" \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1 \"15.0\"")
            + HelpExampleRpc("omni_senddexaccept", "\"35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8\", \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1, \"15.0\"")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseAddress(params[1]);
    uint32_t propertyId = ParsePropertyId(params[2]);
    int64_t amount = ParseAmount(params[3], true); // MSC/TMSC is divisible
    bool override = (params.size() > 4) ? params[4].get_bool(): false;

    // perform checks
    RequirePrimaryToken(propertyId);
    RequireMatchingDExOffer(toAddress, propertyId);

    if (!override) { // reject unsafe accepts - note client maximum tx fee will always be respected regardless of override here
        RequireSaneDExFee(toAddress, propertyId);
        RequireSaneDExPaymentWindow(toAddress, propertyId);
    }

#ifdef ENABLE_WALLET
    // use new 0.10 custom fee to set the accept minimum fee appropriately
    int64_t nMinimumAcceptFee = 0;
    {
        LOCK(cs_tally);
        const CMPOffer* sellOffer = DEx_getOffer(toAddress, propertyId);
        if (sellOffer == NULL) throw JSONRPCError(RPC_TYPE_ERROR, "Unable to load sell offer from the distributed exchange");
        nMinimumAcceptFee = sellOffer->getMinFee();
    }

    LOCK2(cs_main, pwalletMain->cs_wallet);

    // temporarily update the global transaction fee to pay enough for the accept fee
    CFeeRate payTxFeeOriginal = payTxFee;
    payTxFee = CFeeRate(nMinimumAcceptFee, 225); // TODO: refine!
    // fPayAtLeastCustomFee = true;
#endif

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExAccept(propertyId, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, "", 0, payload, txid, rawHex, autoCommit);

#ifdef ENABLE_WALLET
    // set the custom fee back to original
    payTxFee = payTxFeeOriginal;
#endif

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

UniValue omni_sendissuancecrowdsale(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 14)
        throw runtime_error(
            "omni_sendissuancecrowdsale \"fromaddress\" ecosystem type previousid \"category\" \"subcategory\" \"name\" \"url\" \"data\" propertyiddesired tokensperunit deadline ( earlybonus issuerpercentage )\n"

            "Create new tokens as crowdsale."

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. ecosystem            (string, required) the ecosystem to create the tokens in (1 for main ecosystem, 2 for test ecosystem)\n"
            "3. type                 (number, required) the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"
            "4. previousid           (number, required) an identifier of a predecessor token (0 for new crowdsales)\n"
            "5. category             (string, required) a category for the new tokens (can be \"\")\n"
            "6. subcategory          (string, required) a subcategory for the new tokens  (can be \"\")\n"
            "7. name                 (string, required) the name of the new tokens to create\n"
            "8. url                  (string, required) an URL for further information about the new tokens (can be \"\")\n"
            "9. data                 (string, required) a description for the new tokens (can be \"\")\n"
            "10. propertyiddesired   (number, required) the identifier of a token eligible to participate in the crowdsale\n"
            "11. tokensperunit       (string, required) the amount of tokens granted per unit invested in the crowdsale\n"
            "12. deadline            (number, required) the deadline of the crowdsale as Unix timestamp\n"
            "13. earlybonus          (number, required) an early bird bonus for participants in percent per week\n"
            "14. issuerpercentage    (number, required) a percentage of tokens that will be granted to the issuer\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendissuancecrowdsale", "\"3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo\" 2 1 0 \"Companies\" \"Bitcoin Mining\" \"Quantum Miner\" \"\" \"\" 2 \"100\" 1483228800 30 2")
            + HelpExampleRpc("omni_sendissuancecrowdsale", "\"3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo\", 2, 1, 0, \"Companies\", \"Bitcoin Mining\", \"Quantum Miner\", \"\", \"\", 2, \"100\", 1483228800, 30, 2")
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
    RequirePropertyName(name);
    RequireExistingProperty(propertyIdDesired);
    RequireSameEcosystem(ecosystem, propertyIdDesired);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_IssuanceVariable(ecosystem, type, previousId, category, subcategory, name, url, data, propertyIdDesired, numTokens, deadline, earlyBonus, issuerPercentage);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendissuancefixed(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 10)
        throw runtime_error(
            "omni_sendissuancefixed \"fromaddress\" ecosystem type previousid \"category\" \"subcategory\" \"name\" \"url\" \"data\" \"amount\"\n"

            "\nCreate new tokens with fixed supply.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. ecosystem            (string, required) the ecosystem to create the tokens in (1 for main ecosystem, 2 for test ecosystem)\n"
            "3. type                 (number, required) the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"
            "4. previousid           (number, required) an identifier of a predecessor token (use 0 for new tokens)\n"
            "5. category             (string, required) a category for the new tokens (can be \"\")\n"
            "6. subcategory          (string, required) a subcategory for the new tokens  (can be \"\")\n"
            "7. name                 (string, required) the name of the new tokens to create\n"
            "8. url                  (string, required) an URL for further information about the new tokens (can be \"\")\n"
            "9. data                 (string, required) a description for the new tokens (can be \"\")\n"
            "10. amount              (string, required) the number of tokens to create\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendissuancefixed", "\"3Ck2kEGLJtZw9ENj2tameMCtS3HB7uRar3\" 2 1 0 \"Companies\" \"Bitcoin Mining\" \"Quantum Miner\" \"\" \"\" \"1000000\"")
            + HelpExampleRpc("omni_sendissuancefixed", "\"3Ck2kEGLJtZw9ENj2tameMCtS3HB7uRar3\", 2, 1, 0, \"Companies\", \"Bitcoin Mining\", \"Quantum Miner\", \"\", \"\", \"1000000\"")
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
    RequirePropertyName(name);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_IssuanceFixed(ecosystem, type, previousId, category, subcategory, name, url, data, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendissuancemanaged(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 9)
        throw runtime_error(
            "omni_sendissuancemanaged \"fromaddress\" ecosystem type previousid \"category\" \"subcategory\" \"name\" \"url\" \"data\"\n"

            "\nCreate new tokens with manageable supply.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. ecosystem            (string, required) the ecosystem to create the tokens in (1 for main ecosystem, 2 for test ecosystem)\n"
            "3. type                 (number, required) the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"
            "4. previousid           (number, required) an identifier of a predecessor token (use 0 for new tokens)\n"
            "5. category             (string, required) a category for the new tokens (can be \"\")\n"
            "6. subcategory          (string, required) a subcategory for the new tokens  (can be \"\")\n"
            "7. name                 (string, required) the name of the new tokens to create\n"
            "8. url                  (string, required) an URL for further information about the new tokens (can be \"\")\n"
            "9. data                 (string, required) a description for the new tokens (can be \"\")\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendissuancemanaged", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\" 2 1 0 \"Companies\" \"Bitcoin Mining\" \"Quantum Miner\" \"\" \"\"")
            + HelpExampleRpc("omni_sendissuancemanaged", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\", 2, 1, 0, \"Companies\", \"Bitcoin Mining\", \"Quantum Miner\", \"\", \"\"")
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
    RequirePropertyName(name);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_IssuanceManaged(ecosystem, type, previousId, category, subcategory, name, url, data);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendsto(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "omni_sendsto \"fromaddress\" propertyid \"amount\" ( \"redeemaddress\" distributionproperty )\n"

            "\nCreate and broadcast a send-to-owners transaction.\n"

            "\nArguments:\n"
            "1. fromaddress            (string, required) the address to send from\n"
            "2. propertyid             (number, required) the identifier of the tokens to distribute\n"
            "3. amount                 (string, required) the amount to distribute\n"
            "4. redeemaddress          (string, optional) an address that can spend the transaction dust (sender by default)\n"
            "5. distributionproperty   (number, optional) the identifier of the property holders to distribute to\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendsto", "\"32Z3tJccZuqQZ4PhJR2hxHC3tjgjA8cbqz\" \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 3 \"5000\"")
            + HelpExampleRpc("omni_sendsto", "\"32Z3tJccZuqQZ4PhJR2hxHC3tjgjA8cbqz\", \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 3, \"5000\"")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);
    int64_t amount = ParseAmount(params[2], isPropertyDivisible(propertyId));
    std::string redeemAddress = (params.size() > 3 && !ParseText(params[3]).empty()) ? ParseAddress(params[3]): "";
    uint32_t distributionPropertyId = (params.size() > 4) ? ParsePropertyId(params[4]) : propertyId;

    // perform checks
    RequireBalance(fromAddress, propertyId, amount);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SendToOwners(propertyId, amount, distributionPropertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", redeemAddress, 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            PendingAdd(txid, fromAddress, MSC_TYPE_SEND_TO_OWNERS, propertyId, amount);
            return txid.GetHex();
        }
    }
}

UniValue omni_sendgrant(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 5)
        throw runtime_error(
            "omni_sendgrant \"fromaddress\" \"toaddress\" propertyid \"amount\" ( \"memo\" )\n"

            "\nIssue or grant new units of managed tokens.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. toaddress            (string, required) the receiver of the tokens (sender by default, can be \"\")\n"
            "3. propertyid           (number, required) the identifier of the tokens to grant\n"
            "4. amount               (string, required) the amount of tokens to create\n"
            "5. memo                 (string, optional) a text note attached to this transaction (none by default)\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendgrant", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\" \"\" 51 \"7000\"")
            + HelpExampleRpc("omni_sendgrant", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\", \"\", 51, \"7000\"")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = !ParseText(params[1]).empty() ? ParseAddress(params[1]): "";
    uint32_t propertyId = ParsePropertyId(params[2]);
    int64_t amount = ParseAmount(params[3], isPropertyDivisible(propertyId));
    std::string memo = (params.size() > 4) ? ParseText(params[4]): "";

    // perform checks
    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_Grant(propertyId, amount, memo);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendrevoke(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 4)
        throw runtime_error(
            "omni_sendrevoke \"fromaddress\" propertyid \"amount\" ( \"memo\" )\n"

            "\nRevoke units of managed tokens.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to revoke the tokens from\n"
            "2. propertyid           (number, required) the identifier of the tokens to revoke\n"
            "3. amount               (string, required) the amount of tokens to revoke\n"
            "4. memo                 (string, optional) a text note attached to this transaction (none by default)\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendrevoke", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\" \"\" 51 \"100\"")
            + HelpExampleRpc("omni_sendrevoke", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\", \"\", 51, \"100\"")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);
    int64_t amount = ParseAmount(params[2], isPropertyDivisible(propertyId));
    std::string memo = (params.size() > 3) ? ParseText(params[3]): "";

    // perform checks
    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);
    RequireBalance(fromAddress, propertyId, amount);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_Revoke(propertyId, amount, memo);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendclosecrowdsale(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "omni_sendclosecrowdsale \"fromaddress\" propertyid\n"

            "\nManually close a crowdsale.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address associated with the crowdsale to close\n"
            "2. propertyid           (number, required) the identifier of the crowdsale to close\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendclosecrowdsale", "\"3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo\" 70")
            + HelpExampleRpc("omni_sendclosecrowdsale", "\"3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo\", 70")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);

    // perform checks
    RequireExistingProperty(propertyId);
    RequireCrowdsale(propertyId);
    RequireActiveCrowdsale(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_CloseCrowdsale(propertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue trade_MP(const UniValue& params, bool fHelp)
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

    UniValue values(UniValue::VARR);
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
            return omni_sendtrade(values, fHelp);
        }
        case CMPTransaction::CANCEL_AT_PRICE:
        {
            values.push_back(params[0]); // fromAddress
            values.push_back(params[1]); // propertyIdForSale
            values.push_back(params[2]); // amountForSale
            values.push_back(params[3]); // propertyIdDesired
            values.push_back(params[4]); // amountDesired
            return omni_sendcanceltradesbyprice(values, fHelp);
        }
        case CMPTransaction::CANCEL_ALL_FOR_PAIR:
        {
            values.push_back(params[0]); // fromAddress
            values.push_back(params[1]); // propertyIdForSale
            values.push_back(params[3]); // propertyIdDesired
            return omni_sendcanceltradesbypair(values, fHelp);
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
            return omni_sendcancelalltrades(values, fHelp);
        }
    }

    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid action (1,2,3,4 only)");
}

UniValue omni_sendtrade(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 5)
        throw runtime_error(
            "omni_sendtrade \"fromaddress\" propertyidforsale \"amountforsale\" propertiddesired \"amountdesired\"\n"

            "\nPlace a trade offer on the distributed token exchange.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to trade with\n"
            "2. propertyidforsale    (number, required) the identifier of the tokens to list for sale\n"
            "3. amountforsale        (string, required) the amount of tokens to list for sale\n"
            "4. propertiddesired     (number, required) the identifier of the tokens desired in exchange\n"
            "5. amountdesired        (string, required) the amount of tokens desired in exchange\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendtrade", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\" 31 \"250.0\" 1 \"10.0\"")
            + HelpExampleRpc("omni_sendtrade", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\", 31, \"250.0\", 1, \"10.0\"")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(params[1]);
    int64_t amountForSale = ParseAmount(params[2], isPropertyDivisible(propertyIdForSale));
    uint32_t propertyIdDesired = ParsePropertyId(params[3]);
    int64_t amountDesired = ParseAmount(params[4], isPropertyDivisible(propertyIdDesired));

    // perform checks
    RequireExistingProperty(propertyIdForSale);
    RequireExistingProperty(propertyIdDesired);
    RequireBalance(fromAddress, propertyIdForSale, amountForSale);
    RequireSameEcosystem(propertyIdForSale, propertyIdDesired);
    RequireDifferentIds(propertyIdForSale, propertyIdDesired);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExTrade(propertyIdForSale, amountForSale, propertyIdDesired, amountDesired);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            PendingAdd(txid, fromAddress, MSC_TYPE_METADEX_TRADE, propertyIdForSale, amountForSale);
            return txid.GetHex();
        }
    }
}

UniValue omni_sendcanceltradesbyprice(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 5)
        throw runtime_error(
            "omni_sendcanceltradesbyprice \"fromaddress\" propertyidforsale \"amountforsale\" propertiddesired \"amountdesired\"\n"

            "\nCancel offers on the distributed token exchange with the specified price.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to trade with\n"
            "2. propertyidforsale    (number, required) the identifier of the tokens listed for sale\n"
            "3. amountforsale        (string, required) the amount of tokens to listed for sale\n"
            "4. propertiddesired     (number, required) the identifier of the tokens desired in exchange\n"
            "5. amountdesired        (string, required) the amount of tokens desired in exchange\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendcanceltradesbyprice", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\" 31 \"100.0\" 1 \"5.0\"")
            + HelpExampleRpc("omni_sendcanceltradesbyprice", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\", 31, \"100.0\", 1, \"5.0\"")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(params[1]);
    int64_t amountForSale = ParseAmount(params[2], isPropertyDivisible(propertyIdForSale));
    uint32_t propertyIdDesired = ParsePropertyId(params[3]);
    int64_t amountDesired = ParseAmount(params[4], isPropertyDivisible(propertyIdDesired));

    // perform checks
    RequireExistingProperty(propertyIdForSale);
    RequireExistingProperty(propertyIdDesired);
    RequireSameEcosystem(propertyIdForSale, propertyIdDesired);
    RequireDifferentIds(propertyIdForSale, propertyIdDesired);
    // TODO: check, if there are matching offers to cancel

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelPrice(propertyIdForSale, amountForSale, propertyIdDesired, amountDesired);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            PendingAdd(txid, fromAddress, MSC_TYPE_METADEX_CANCEL_PRICE, propertyIdForSale, amountForSale, false);
            return txid.GetHex();
        }
    }
}

UniValue omni_sendcanceltradesbypair(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "omni_sendcanceltradesbypair \"fromaddress\" propertyidforsale propertiddesired\n"

            "\nCancel all offers on the distributed token exchange with the given currency pair.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to trade with\n"
            "2. propertyidforsale    (number, required) the identifier of the tokens listed for sale\n"
            "3. propertiddesired     (number, required) the identifier of the tokens desired in exchange\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendcanceltradesbypair", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\" 1 31")
            + HelpExampleRpc("omni_sendcanceltradesbypair", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\", 1, 31")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(params[1]);
    uint32_t propertyIdDesired = ParsePropertyId(params[2]);

    // perform checks
    RequireExistingProperty(propertyIdForSale);
    RequireExistingProperty(propertyIdDesired);
    RequireSameEcosystem(propertyIdForSale, propertyIdDesired);
    RequireDifferentIds(propertyIdForSale, propertyIdDesired);
    // TODO: check, if there are matching offers to cancel

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelPair(propertyIdForSale, propertyIdDesired);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            PendingAdd(txid, fromAddress, MSC_TYPE_METADEX_CANCEL_PAIR, propertyIdForSale, 0, false);
            return txid.GetHex();
        }
    }
}

UniValue omni_sendcancelalltrades(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "omni_sendcancelalltrades \"fromaddress\" ecosystem\n"

            "\nCancel all offers on the distributed token exchange.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to trade with\n"
            "2. ecosystem            (number, required) the ecosystem of the offers to cancel (1 for main ecosystem, 2 for test ecosystem)\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendcancelalltrades", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\" 1")
            + HelpExampleRpc("omni_sendcancelalltrades", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\", 1")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint8_t ecosystem = ParseEcosystem(params[1]);

    // perform checks
    // TODO: check, if there are matching offers to cancel

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelEcosystem(ecosystem);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            PendingAdd(txid, fromAddress, MSC_TYPE_METADEX_CANCEL_ECOSYSTEM, ecosystem, 0, false);
            return txid.GetHex();
        }
    }
}

UniValue omni_sendchangeissuer(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "omni_sendchangeissuer \"fromaddress\" \"toaddress\" propertyid\n"

            "\nChange the issuer on record of the given tokens.\n"

            "\nArguments:\n"
            "1. fromaddress          (string, required) the address associated with the tokens\n"
            "2. toaddress            (string, required) the address to transfer administrative control to\n"
            "3. propertyid           (number, required) the identifier of the tokens\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendchangeissuer", "\"1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu\" \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 3")
            + HelpExampleRpc("omni_sendchangeissuer", "\"1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu\", \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 3")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string toAddress = ParseAddress(params[1]);
    uint32_t propertyId = ParsePropertyId(params[2]);

    // perform checks
    RequireExistingProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_ChangeIssuer(propertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendenablefreezing(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "omni_sendenablefreezing \"fromaddress\" propertyid\n"

            "\nEnables address freezing for a centrally managed property.\n"

            "\nArguments:\n"
            "1. fromaddress          (string,  required) the issuer of the tokens\n"
            "2. propertyid           (number,  required) the identifier of the tokens\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_sendenablefreezing", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 3")
            + HelpExampleRpc("omni_sendenablefreezing", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 3")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);

    // perform checks
    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_EnableFreezing(propertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_senddisablefreezing(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "omni_senddisablefreezing \"fromaddress\" propertyid\n"

            "\nDisables address freezing for a centrally managed property.\n"
            "\nIMPORTANT NOTE:  Disabling freezing for a property will UNFREEZE all frozen addresses for that property!"

            "\nArguments:\n"
            "1. fromaddress          (string,  required) the issuer of the tokens\n"
            "2. propertyid           (number,  required) the identifier of the tokens\n"

            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_senddisablefreezing", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 3")
            + HelpExampleRpc("omni_senddisablefreezing", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 3")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);

    // perform checks
    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DisableFreezing(propertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendfreeze(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error(
            "omni_sendfreeze \"fromaddress\" \"toaddress\" propertyid amount \n"
            "\nFreeze an address for a centrally managed token.\n"
            "\nNote: Only the issuer may freeze tokens, and only if the token is of the managed type with the freezing option enabled.\n"
            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from (must be the issuer of the property)\n"
            "2. toaddress            (string, required) the address to freeze tokens for\n"
            "3. propertyid           (number, required) the property to freeze tokens for (must be managed type and have freezing option enabled)\n"
            "4. amount               (number, required) the amount of tokens to freeze (note: this is unused - once frozen an address cannot send any transactions for the property)\n"
            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_sendfreeze", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 1 0")
            + HelpExampleRpc("omni_sendfreeze", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 1, 0")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string refAddress = ParseAddress(params[1]);
    uint32_t propertyId = ParsePropertyId(params[2]);
    int64_t amount = ParseAmount(params[3], isPropertyDivisible(propertyId));

    // perform checks
    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_FreezeTokens(propertyId, amount, refAddress);

    // request the wallet build the transaction (and if needed commit it)
    // Note: no ref address is sent to WalletTxBuilder as the ref address is contained within the payload
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendunfreeze(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error(
            "omni_sendunfreeze \"fromaddress\" \"toaddress\" propertyid amount \n"
            "\nUnfreezes an address for a centrally managed token.\n"
            "\nNote: Only the issuer may unfreeze tokens.\n"
            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from (must be the issuer of the property)\n"
            "2. toaddress            (string, required) the address to unfreeze tokens for\n"
            "3. propertyid           (number, required) the property to unfreeze tokens for (must be managed type and have freezing option enabled)\n"
            "4. amount               (number, required) the amount of tokens to unfreeze (note: this is unused - once frozen an address cannot send any transactions for the property)\n"
            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_sendunfreeze", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 1 0")
            + HelpExampleRpc("omni_sendunfreeze", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 1, 0")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    std::string refAddress = ParseAddress(params[1]);
    uint32_t propertyId = ParsePropertyId(params[2]);
    int64_t amount = ParseAmount(params[3], isPropertyDivisible(propertyId));

    // perform checks
    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_UnfreezeTokens(propertyId, amount, refAddress);

    // request the wallet build the transaction (and if needed commit it)
    // Note: no ref address is sent to WalletTxBuilder as the ref address is contained within the payload
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendactivation(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error(
            "omni_sendactivation \"fromaddress\" featureid block minclientversion\n"
            "\nActivate a protocol feature.\n"
            "\nNote: Omni Core ignores activations from unauthorized sources.\n"
            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. featureid            (number, required) the identifier of the feature to activate\n"
            "3. block                (number, required) the activation block\n"
            "4. minclientversion     (number, required) the minimum supported client version\n"
            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_sendactivation", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" 1 370000 999")
            + HelpExampleRpc("omni_sendactivation", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", 1, 370000, 999")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint16_t featureId = params[1].get_int();
    uint32_t activationBlock = params[2].get_int();
    uint32_t minClientVersion = params[3].get_int();

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_ActivateFeature(featureId, activationBlock, minClientVersion);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_senddeactivation(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "omni_senddeactivation \"fromaddress\" featureid\n"
            "\nDeactivate a protocol feature.  For Emergency Use Only.\n"
            "\nNote: Omni Core ignores deactivations from unauthorized sources.\n"
            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. featureid            (number, required) the identifier of the feature to activate\n"
            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_senddeactivation", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" 1")
            + HelpExampleRpc("omni_senddeactivation", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", 1")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    uint16_t featureId = params[1].get_int64();

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DeactivateFeature(featureId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

UniValue omni_sendalert(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error(
            "omni_sendalert \"fromaddress\" alerttype expiryvalue typecheck versioncheck \"message\"\n"
            "\nCreates and broadcasts an Omni Core alert.\n"
            "\nNote: Omni Core ignores alerts from unauthorized sources.\n"
            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. alerttype            (number, required) the alert type\n"
            "3. expiryvalue          (number, required) the value when the alert expires (depends on alert type)\n"
            "4. message              (string, required) the user-faced alert message\n"
            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_sendalert", "")
            + HelpExampleRpc("omni_sendalert", "")
        );

    // obtain parameters & info
    std::string fromAddress = ParseAddress(params[0]);
    int64_t tempAlertType = params[1].get_int64();
    if (tempAlertType < 1 || 65535 < tempAlertType) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Alert type is out of range");
    }
    uint16_t alertType = static_cast<uint16_t>(tempAlertType);
    int64_t tempExpiryValue = params[2].get_int64();
    if (tempExpiryValue < 1 || 4294967295LL < tempExpiryValue) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Expiry value is out of range");
    }
    uint32_t expiryValue = static_cast<uint32_t>(tempExpiryValue);
    std::string alertMessage = ParseText(params[3]);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_OmniCoreAlert(alertType, expiryValue, alertMessage);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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

static const CRPCCommand commands[] =
{ //  category                             name                            actor (function)               okSafeMode
  //  ------------------------------------ ------------------------------- ------------------------------ ----------
#ifdef ENABLE_WALLET
    { "omni layer (transaction creation)", "omni_sendrawtx",               &omni_sendrawtx,               false },
    { "omni layer (transaction creation)", "omni_send",                    &omni_send,                    false },
    { "omni layer (transaction creation)", "omni_senddexsell",             &omni_senddexsell,             false },
    { "omni layer (transaction creation)", "omni_senddexaccept",           &omni_senddexaccept,           false },
    { "omni layer (transaction creation)", "omni_sendissuancecrowdsale",   &omni_sendissuancecrowdsale,   false },
    { "omni layer (transaction creation)", "omni_sendissuancefixed",       &omni_sendissuancefixed,       false },
    { "omni layer (transaction creation)", "omni_sendissuancemanaged",     &omni_sendissuancemanaged,     false },
    { "omni layer (transaction creation)", "omni_sendtrade",               &omni_sendtrade,               false },
    { "omni layer (transaction creation)", "omni_sendcanceltradesbyprice", &omni_sendcanceltradesbyprice, false },
    { "omni layer (transaction creation)", "omni_sendcanceltradesbypair",  &omni_sendcanceltradesbypair,  false },
    { "omni layer (transaction creation)", "omni_sendcancelalltrades",     &omni_sendcancelalltrades,     false },
    { "omni layer (transaction creation)", "omni_sendsto",                 &omni_sendsto,                 false },
    { "omni layer (transaction creation)", "omni_sendgrant",               &omni_sendgrant,               false },
    { "omni layer (transaction creation)", "omni_sendrevoke",              &omni_sendrevoke,              false },
    { "omni layer (transaction creation)", "omni_sendclosecrowdsale",      &omni_sendclosecrowdsale,      false },
    { "omni layer (transaction creation)", "omni_sendchangeissuer",        &omni_sendchangeissuer,        false },
    { "omni layer (transaction creation)", "omni_sendall",                 &omni_sendall,                 false },
    { "omni layer (transaction creation)", "omni_sendenablefreezing",      &omni_sendenablefreezing,      false },
    { "omni layer (transaction creation)", "omni_senddisablefreezing",     &omni_senddisablefreezing,     false },
    { "omni layer (transaction creation)", "omni_sendfreeze",              &omni_sendfreeze,              false },
    { "omni layer (transaction creation)", "omni_sendunfreeze",            &omni_sendunfreeze,            false },
    { "hidden",                            "omni_senddeactivation",        &omni_senddeactivation,        true  },
    { "hidden",                            "omni_sendactivation",          &omni_sendactivation,          false },
    { "hidden",                            "omni_sendalert",               &omni_sendalert,               true  },
    { "omni layer (transaction creation)", "omni_funded_send",             &omni_funded_send,             false },
    { "omni layer (transaction creation)", "omni_funded_sendall",          &omni_funded_sendall,          false },

    /* depreciated: */
    { "hidden",                            "sendrawtx_MP",                 &omni_sendrawtx,               false },
    { "hidden",                            "send_MP",                      &omni_send,                    false },
    { "hidden",                            "sendtoowners_MP",              &omni_sendsto,                 false },
    { "hidden",                            "trade_MP",                     &trade_MP,                     false },
#endif
};

void RegisterOmniTransactionCreationRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
