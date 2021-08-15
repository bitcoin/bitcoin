/**
 * @file rpctx.cpp
 *
 * This file contains RPC calls for creating and sending Omni transactions.
 */

#include <omnicore/createpayload.h>
#include <omnicore/dex.h>
#include <omnicore/errors.h>
#include <omnicore/omnicore.h>
#include <omnicore/pending.h>
#include <omnicore/rpcrequirements.h>
#include <omnicore/rpcvalues.h>
#include <omnicore/rules.h>
#include <omnicore/sp.h>
#include <omnicore/tx.h>
#include <omnicore/utilsbitcoin.h>
#include <omnicore/wallettxbuilder.h>

#include <interfaces/wallet.h>
#include <init.h>
#include <key_io.h>
#include <validation.h>
#include <wallet/rpcwallet.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <sync.h>
#include <util/moneystr.h>
#include <wallet/wallet.h>

#include <univalue.h>

#include <stdint.h>
#include <stdexcept>
#include <string>

using std::runtime_error;
using namespace mastercore;


static UniValue omni_funded_send(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 5)
        throw runtime_error(
            RPCHelpMan{"omni_funded_send",
               "\nCreates and sends a funded simple send transaction.\n"
               "\nAll bitcoins from the sender are consumed and if there are bitcoins missing, they are taken from the specified fee source. Change is sent to the fee source!\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send the tokens from\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address of the receiver\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to send\n"},
                   {"amount", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount to send\n"},
                   {"feeaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address that is used for change and to pay for fees, if needed\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_funded_send", "\"1DFa5bT6KMEr6ta29QJouainsjaNBsJQhH\" \"15cWrfuvMxyxGst2FisrQcvcpF48x6sXoH\" 1 \"100.0\" \"15Jhzz4omEXEyFKbdcccJwuVPea5LqsKM1\"")
                   + HelpExampleRpc("omni_funded_send", "\"1DFa5bT6KMEr6ta29QJouainsjaNBsJQhH\", \"15cWrfuvMxyxGst2FisrQcvcpF48x6sXoH\", 1, \"100.0\", \"15Jhzz4omEXEyFKbdcccJwuVPea5LqsKM1\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    std::string toAddress = ParseAddress(request.params[1]);
    uint32_t propertyId = ParsePropertyId(request.params[2]);
    int64_t amount = ParseAmount(request.params[3], isPropertyDivisible(propertyId));
    std::string feeAddress = ParseAddress(request.params[4]);

    // perform checks
    RequireExistingProperty(propertyId);
    RequireBalance(fromAddress, propertyId, amount);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SimpleSend(propertyId, amount);

    // create the raw transaction
    uint256 retTxid;
    int result = CreateFundedTransaction(fromAddress, toAddress, feeAddress, payload, retTxid, pwallet.get());
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    }

    return retTxid.ToString();
}

static UniValue omni_funded_sendall(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 4)
        throw runtime_error(
            RPCHelpMan{"omni_funded_sendall",
               "\nCreates and sends a transaction that transfers all available tokens in the given ecosystem to the recipient.\n"
               "\nAll bitcoins from the sender are consumed and if there are bitcoins missing, they are taken from the specified fee source. Change is sent to the fee source!\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to the tokens send from\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address of the receiver\n"},
                   {"ecosystem", RPCArg::Type::NUM, RPCArg::Optional::NO, "the ecosystem of the tokens to send (1 for main ecosystem, 2 for test ecosystem)\n"},
                   {"feeaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address that is used for change and to pay for fees, if needed\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_funded_sendall", "\"1DFa5bT6KMEr6ta29QJouainsjaNBsJQhH\" \"15cWrfuvMxyxGst2FisrQcvcpF48x6sXoH\" 1 \"15Jhzz4omEXEyFKbdcccJwuVPea5LqsKM1\"")
                   + HelpExampleRpc("omni_funded_sendall", "\"1DFa5bT6KMEr6ta29QJouainsjaNBsJQhH\", \"15cWrfuvMxyxGst2FisrQcvcpF48x6sXoH\", 1, \"15Jhzz4omEXEyFKbdcccJwuVPea5LqsKM1\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    std::string toAddress = ParseAddress(request.params[1]);
    uint8_t ecosystem = ParseEcosystem(request.params[2]);
    std::string feeAddress = ParseAddress(request.params[3]);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SendAll(ecosystem);

    // create the raw transaction
    uint256 retTxid;
    int result = CreateFundedTransaction(fromAddress, toAddress, feeAddress, payload, retTxid, pwallet.get());
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    }

    return retTxid.ToString();
}

static UniValue omni_sendrawtx(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 5)
        throw runtime_error(
            RPCHelpMan{"omni_sendrawtx",
               "\nBroadcasts a raw Omni Layer transaction.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"rawtransaction", RPCArg::Type::STR, RPCArg::Optional::NO, "the hex-encoded raw transaction"},
                   {"referenceaddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "a reference address (none by default)\n"},
                   {"redeemaddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "an address that can spent the transaction dust (sender by default)\n"},
                   {"referenceamount", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "a bitcoin amount that is sent to the receiver (minimal by default)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendrawtx", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\" \"000000000000000100000000017d7840\" \"1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV\"")
                   + HelpExampleRpc("omni_sendrawtx", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\", \"000000000000000100000000017d7840\", \"1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV\"")
               }
            }.ToString());

    std::string fromAddress = ParseAddress(request.params[0]);
    std::vector<unsigned char> data = ParseHexV(request.params[1], "raw transaction");
    std::string toAddress = (request.params.size() > 2) ? ParseAddressOrEmpty(request.params[2]): "";
    std::string redeemAddress = (request.params.size() > 3) ? ParseAddressOrEmpty(request.params[3]): "";
    int64_t referenceAmount = (request.params.size() > 4) ? ParseAmount(request.params[4], true): 0;

    //some sanity checking of the data supplied?
    uint256 newTX;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, redeemAddress, referenceAmount, data, newTX, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_send(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() < 4 || request.params.size() > 6)
        throw runtime_error(
            RPCHelpMan{"omni_send",
               "\nCreate and broadcast a simple send transaction.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address of the receiver\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to send\n"},
                   {"amount", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount to send\n"},
                   {"redeemaddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "an address that can spend the transaction dust (sender by default)\n"},
                   {"referenceamount", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "a bitcoin amount that is sent to the receiver (minimal by default)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_send", "\"3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY\" \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1 \"100.0\"")
                   + HelpExampleRpc("omni_send", "\"3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY\", \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1, \"100.0\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    std::string toAddress = ParseAddress(request.params[1]);
    uint32_t propertyId = ParsePropertyId(request.params[2]);
    int64_t amount = ParseAmount(request.params[3], isPropertyDivisible(propertyId));
    std::string redeemAddress = (request.params.size() > 4 && !ParseText(request.params[4]).empty()) ? ParseAddress(request.params[4]): "";
    int64_t referenceAmount = (request.params.size() > 5) ? ParseAmount(request.params[5], true): 0;

    // perform checks
    RequireExistingProperty(propertyId);
    RequireBalance(fromAddress, propertyId, amount);
    RequireSaneReferenceAmount(referenceAmount);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SimpleSend(propertyId, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, redeemAddress, referenceAmount, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendall(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() < 3 || request.params.size() > 5)
        throw runtime_error(
            RPCHelpMan{"omni_sendall",
               "\nTransfers all available tokens in the given ecosystem to the recipient.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address of the receiver\n"},
                   {"ecosystem", RPCArg::Type::NUM, RPCArg::Optional::NO, "the ecosystem of the tokens to send (1 for main ecosystem, 2 for test ecosystem)\n"},
                   {"redeemaddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "an address that can spend the transaction dust (sender by default)\n"},
                   {"referenceamount", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "a bitcoin amount that is sent to the receiver (minimal by default)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendall", "\"3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY\" \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 2")
                   + HelpExampleRpc("omni_sendall", "\"3M9qvHKtgARhqcMtM5cRT9VaiDJ5PSfQGY\", \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 2")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    std::string toAddress = ParseAddress(request.params[1]);
    uint8_t ecosystem = ParseEcosystem(request.params[2]);
    std::string redeemAddress = (request.params.size() > 3 && !ParseText(request.params[3]).empty()) ? ParseAddress(request.params[3]): "";
    int64_t referenceAmount = (request.params.size() > 4) ? ParseAmount(request.params[4], true): 0;

    // perform checks
    RequireSaneReferenceAmount(referenceAmount);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SendAll(ecosystem);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, redeemAddress, referenceAmount, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_senddexsell(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 7)
        throw runtime_error(
            RPCHelpMan{"omni_senddexsell",
               "\nPlace, update or cancel a sell offer on the distributed token/QTC exchange.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"propertyidforsale", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to list for sale\n"},
                   {"amountforsale", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens to list for sale\n"},
                   {"amountdesired", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of bitcoins desired\n"},
                   {"paymentwindow", RPCArg::Type::NUM, RPCArg::Optional::NO, "a time limit in blocks a buyer has to pay following a successful accepting order\n"},
                   {"minacceptfee", RPCArg::Type::STR, RPCArg::Optional::NO, "a minimum mining fee a buyer has to pay to accept the offer\n"},
                   {"action", RPCArg::Type::NUM, RPCArg::Optional::NO, "the action to take (1 for new offers, 2 to update\", 3 to cancel)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_senddexsell", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1 \"1.5\" \"0.75\" 25 \"0.0001\" 1")
                   + HelpExampleRpc("omni_senddexsell", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1, \"1.5\", \"0.75\", 25, \"0.0001\", 1")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(request.params[1]);
    int64_t amountForSale = 0; // depending on action
    int64_t amountDesired = 0; // depending on action
    uint8_t paymentWindow = 0; // depending on action
    int64_t minAcceptFee = 0;  // depending on action
    uint8_t action = ParseDExAction(request.params[6]);

    // perform conversions
    if (action <= CMPTransaction::UPDATE) { // actions 3 permit zero values, skip check
        amountForSale = ParseAmount(request.params[2], isPropertyDivisible(propertyIdForSale));
        amountDesired = ParseAmount(request.params[3], true); // BTC is divisible
        paymentWindow = ParseDExPaymentWindow(request.params[4]);
        minAcceptFee = ParseDExFee(request.params[5]);
    }

    // perform checks
    if (!IsFeatureActivated(FEATURE_FREEDEX, GetHeight())) {
        RequirePrimaryToken(propertyIdForSale);
    }

    switch (action) {
        case CMPTransaction::NEW:
        {
            RequireBalance(fromAddress, propertyIdForSale, amountForSale);
            RequireNoOtherDExOffer(fromAddress);
            break;
        }
        case CMPTransaction::UPDATE:
        {
            RequireBalance(fromAddress, propertyIdForSale, amountForSale);
            RequireMatchingDExOffer(fromAddress, propertyIdForSale);
            break;
        }
        case CMPTransaction::CANCEL:
        {
            RequireMatchingDExOffer(fromAddress, propertyIdForSale);
            break;
        }
    }

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExSell(propertyIdForSale, amountForSale, amountDesired, paymentWindow, minAcceptFee, action);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_senddexaccept(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() < 4 || request.params.size() > 5)
        throw runtime_error(
            RPCHelpMan{"omni_senddexaccept",
               "\nCreate and broadcast an accept offer for the specified token and amount.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address of the seller\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the token to purchase\n"},
                   {"amount", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount to accept\n"},
                   {"override", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "override minimum accept fee and payment window checks (use with caution!)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_senddexaccept", "\"35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8\" \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1 \"15.0\"")
                   + HelpExampleRpc("omni_senddexaccept", "\"35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8\", \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1, \"15.0\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    std::string toAddress = ParseAddress(request.params[1]);
    uint32_t propertyId = ParsePropertyId(request.params[2]);
    int64_t amount = ParseAmount(request.params[3], isPropertyDivisible(propertyId));
    bool override = (request.params.size() > 4) ? request.params[4].get_bool(): false;

    // perform checks
    if (!IsFeatureActivated(FEATURE_FREEDEX, GetHeight())) {
        RequirePrimaryToken(propertyId);
    }
    RequireMatchingDExOffer(toAddress, propertyId);

    if (!override) { // reject unsafe accepts - note client maximum tx fee will always be respected regardless of override here
        RequireSaneDExFee(toAddress, propertyId);
        RequireSaneDExPaymentWindow(toAddress, propertyId);
    }

    int64_t nMinimumAcceptFee = 0;
    // use new 0.10 custom fee to set the accept minimum fee appropriately
    {
        LOCK(cs_tally);
        const CMPOffer* sellOffer = DEx_getOffer(toAddress, propertyId);
        if (sellOffer == nullptr) throw JSONRPCError(RPC_TYPE_ERROR, "Unable to load sell offer from the distributed exchange");
        nMinimumAcceptFee = sellOffer->getMinFee();
    }

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExAccept(propertyId, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, "", 0, payload, txid, rawHex, autoCommit, pwallet.get(), nMinimumAcceptFee);

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

static UniValue omni_sendnewdexorder(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 6)
        throw runtime_error(
            RPCHelpMan{"omni_sendnewdexorder",
               "\nCreates a new sell offer on the distributed token/QTC exchange.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"propertyidforsale", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to list for sale\n"},
                   {"amountforsale", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens to list for sale\n"},
                   {"amountdesired", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of bitcoins desired\n"},
                   {"paymentwindow", RPCArg::Type::NUM, RPCArg::Optional::NO, "a time limit in blocks a buyer has to pay following a successful accepting order\n"},
                   {"minacceptfee", RPCArg::Type::STR, RPCArg::Optional::NO, "a minimum mining fee a buyer has to pay to accept the offer\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendnewdexorder", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1 \"1.5\" \"0.75\" 50 \"0.0001\"")
                   + HelpExampleRpc("omni_sendnewdexorder", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1, \"1.5\", \"0.75\", 50, \"0.0001\"")
               }
            }.ToString());

    // parse parameters
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(request.params[1]);
    int64_t amountForSale = ParseAmount(request.params[2], isPropertyDivisible(propertyIdForSale));
    int64_t amountDesired = ParseAmount(request.params[3], true); // BTC is divisible
    uint8_t paymentWindow = ParseDExPaymentWindow(request.params[4]);
    int64_t minAcceptFee = ParseDExFee(request.params[5]);
    uint8_t action = CMPTransaction::NEW;

    // perform checks
    if (!IsFeatureActivated(FEATURE_FREEDEX, GetHeight())) {
        RequirePrimaryToken(propertyIdForSale);
    }
    RequireBalance(fromAddress, propertyIdForSale, amountForSale);
    RequireNoOtherDExOffer(fromAddress);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExSell(
        propertyIdForSale,
        amountForSale,
        amountDesired,
        paymentWindow,
        minAcceptFee,
        action);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    }
    if (!autoCommit) {
        return rawHex;
    }

    PendingAdd(txid, fromAddress, MSC_TYPE_TRADE_OFFER, propertyIdForSale, amountForSale, true);
    return txid.GetHex();
}

static UniValue omni_sendupdatedexorder(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 6)
        throw runtime_error(
            RPCHelpMan{"omni_sendupdatedexorder",
               "\nUpdates an existing sell offer on the distributed token/QTC exchange.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"propertyidforsale", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to update\n"},
                   {"amountforsale", RPCArg::Type::STR, RPCArg::Optional::NO, "the new amount of tokens to list for sale\n"},
                   {"amountdesired", RPCArg::Type::STR, RPCArg::Optional::NO, "the new amount of bitcoins desired\n"},
                   {"paymentwindow", RPCArg::Type::NUM, RPCArg::Optional::NO, "a new time limit in blocks a buyer has to pay following a successful accepting order\n"},
                   {"minacceptfee", RPCArg::Type::STR, RPCArg::Optional::NO, "a new minimum mining fee a buyer has to pay to accept the offer\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendupdatedexorder", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1 \"1.0\" \"1.75\" 50 \"0.0001\"")
                   + HelpExampleRpc("omni_sendupdatedexorder", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1, \"1.0\", \"1.75\", 50, \"0.0001\"")
               }
            }.ToString());

    // parse parameters
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(request.params[1]);
    int64_t amountForSale = ParseAmount(request.params[2], isPropertyDivisible(propertyIdForSale));
    int64_t amountDesired = ParseAmount(request.params[3], true); // BTC is divisible
    uint8_t paymentWindow = ParseDExPaymentWindow(request.params[4]);
    int64_t minAcceptFee = ParseDExFee(request.params[5]);
    uint8_t action = CMPTransaction::UPDATE;

    // perform checks
    if (!IsFeatureActivated(FEATURE_FREEDEX, GetHeight())) {
        RequirePrimaryToken(propertyIdForSale);
    }
    RequireBalance(fromAddress, propertyIdForSale, amountForSale);
    RequireMatchingDExOffer(fromAddress, propertyIdForSale);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExSell(
        propertyIdForSale,
        amountForSale,
        amountDesired,
        paymentWindow,
        minAcceptFee,
        action);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    }
    if (!autoCommit) {
        return rawHex;
    }

    PendingAdd(txid, fromAddress, MSC_TYPE_TRADE_OFFER, propertyIdForSale, amountForSale, true);
    return txid.GetHex();
}

static UniValue omni_sendcanceldexorder(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            RPCHelpMan{"omni_sendcanceldexorder",
               "\nCancels existing sell offer on the distributed token/QTC exchange.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"propertyidforsale", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to cancel\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendcanceldexorder", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1")
                   + HelpExampleRpc("omni_sendcanceldexorder", "\"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1")
               }
            }.ToString());

    // parse parameters
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(request.params[1]);
    int64_t amountForSale = 0;
    int64_t amountDesired = 0;
    uint8_t paymentWindow = 0;
    int64_t minAcceptFee = 0;
    uint8_t action = CMPTransaction::CANCEL;

    // perform checks
    if (!IsFeatureActivated(FEATURE_FREEDEX, GetHeight())) {
        RequirePrimaryToken(propertyIdForSale);
    }
    RequireMatchingDExOffer(fromAddress, propertyIdForSale);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DExSell(
        propertyIdForSale,
        amountForSale,
        amountDesired,
        paymentWindow,
        minAcceptFee,
        action);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    }
    if (!autoCommit) {
        return rawHex;
    }

    PendingAdd(txid, fromAddress, MSC_TYPE_TRADE_OFFER, propertyIdForSale, amountForSale, false);
    return txid.GetHex();
}

static UniValue omni_senddexpay(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 4)
        throw runtime_error(
            RPCHelpMan{"omni_senddexpay",
               "\nCreate and broadcast payment for an accept offer.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address of the seller\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the token to purchase\n"},
                   {"amount", RPCArg::Type::STR, RPCArg::Optional::NO, "the Bitcoin amount to send\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_senddexaccept", "\"35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8\" \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 1 \"15.0\"")
                   + HelpExampleRpc("omni_senddexaccept", "\"35URq1NN3xL6GeRKUP6vzaQVcxoJiiJKd8\", \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 1, \"15.0\"")
               }
            }.ToString());

    // Parameters
    std::string buyerAddress = ParseText(request.params[0]);
    std::string sellerAddress = ParseText(request.params[1]);
    uint32_t propertyId = ParsePropertyId(request.params[2]);
    CAmount nAmount = ParseAmount(request.params[3], true);

    // Check parameters are valid
    if (nAmount <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount for send");

    CTxDestination buyerDest = DecodeDestination(buyerAddress);
    if (!IsValidDestination(buyerDest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid buyer address");
    }

    CTxDestination sellerDest = DecodeDestination(sellerAddress);
    if (!IsValidDestination(sellerDest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid seller address");
    }

    if (!IsFeatureActivated(FEATURE_FREEDEX, GetHeight())) {
        RequirePrimaryToken(propertyId);
    }
    RequireMatchingDExAccept(sellerAddress, propertyId, buyerAddress);

    // Get accept offer and make sure buyer is not trying to overpay
    {
        LOCK(cs_tally);
        const CMPAccept* acceptOffer = DEx_getAccept(sellerAddress, propertyId, buyerAddress);
        if (acceptOffer == nullptr)
            throw JSONRPCError(RPC_MISC_ERROR, "Unable to load accept offer from the distributed exchange");

        const CAmount amountAccepted = acceptOffer->getAcceptAmountRemaining();
        const CAmount amountToPayInBTC = calculateDesiredBTC(acceptOffer->getOfferAmountOriginal(), acceptOffer->getBTCDesiredOriginal(), amountAccepted);

        if (nAmount > amountToPayInBTC) {
            throw JSONRPCError(RPC_MISC_ERROR, strprintf("Paying more than required: %lld QTC to pay for %lld tokens", FormatMoney(amountToPayInBTC), FormatMP(propertyId, amountAccepted)));
        }

        if (!isPropertyDivisible(propertyId) && nAmount < amountToPayInBTC) {
            throw JSONRPCError(RPC_MISC_ERROR, strprintf("Paying less than required: %lld QTC to pay for %lld tokens", FormatMoney(amountToPayInBTC), FormatMP(propertyId, amountAccepted)));
        }
    }

    uint256 txid;
    int result = CreateDExTransaction(pwallet.get(), buyerAddress, sellerAddress, nAmount, txid);

    // Check error and return the txid
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        return txid.GetHex();
    }
}

static UniValue omni_sendissuancecrowdsale(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 14)
        throw runtime_error(
            RPCHelpMan{"omni_sendissuancecrowdsale",
               "Create new tokens as crowdsale.",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"ecosystem", RPCArg::Type::NUM, RPCArg::Optional::NO, "the ecosystem to create the tokens in (1 for main ecosystem, 2 for test ecosystem)\n"},
                   {"type", RPCArg::Type::NUM, RPCArg::Optional::NO, "the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"},
                   {"previousid", RPCArg::Type::NUM, RPCArg::Optional::NO, "an identifier of a predecessor token (0 for new crowdsales)\n"},
                   {"category", RPCArg::Type::STR, RPCArg::Optional::NO, "a category for the new tokens (can be \"\")\n"},
                   {"subcategory", RPCArg::Type::STR, RPCArg::Optional::NO, "a subcategory for the new tokens  (can be \"\")\n"},
                   {"name", RPCArg::Type::STR, RPCArg::Optional::NO, "the name of the new tokens to create\n"},
                   {"url", RPCArg::Type::STR, RPCArg::Optional::NO, "a URL for further information about the new tokens (can be \"\")\n"},
                   {"data", RPCArg::Type::STR, RPCArg::Optional::NO, "a description for the new tokens (can be \"\")\n"},
                   {"propertyiddesired", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of a token eligible to participate in the crowdsale\n"},
                   {"tokensperunit", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens granted per unit invested in the crowdsale\n"},
                   {"deadline", RPCArg::Type::NUM, RPCArg::Optional::NO, "the deadline of the crowdsale as Unix timestamp\n"},
                   {"earlybonus", RPCArg::Type::NUM, RPCArg::Optional::NO, "an early bird bonus for participants in percent per week\n"},
                   {"issuerpercentage", RPCArg::Type::NUM, RPCArg::Optional::NO, "a percentage of tokens that will be granted to the issuer\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendissuancecrowdsale", "\"3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo\" 2 1 0 \"Companies\" \"Bitcoin Mining\" \"Quantum Miner\" \"\" \"\" 2 \"100\" 1483228800 30 2")
                   + HelpExampleRpc("omni_sendissuancecrowdsale", "\"3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo\", 2, 1, 0, \"Companies\", \"Bitcoin Mining\", \"Quantum Miner\", \"\", \"\", 2, \"100\", 1483228800, 30, 2")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint8_t ecosystem = ParseEcosystem(request.params[1]);
    uint16_t type = ParsePropertyType(request.params[2]);
    uint32_t previousId = ParsePreviousPropertyId(request.params[3]);
    std::string category = ParseText(request.params[4]);
    std::string subcategory = ParseText(request.params[5]);
    std::string name = ParseText(request.params[6]);
    std::string url = ParseText(request.params[7]);
    std::string data = ParseText(request.params[8]);
    uint32_t propertyIdDesired = ParsePropertyId(request.params[9]);
    int64_t numTokens = ParseAmount(request.params[10], type);
    int64_t deadline = ParseDeadline(request.params[11]);
    uint8_t earlyBonus = ParseEarlyBirdBonus(request.params[12]);
    uint8_t issuerPercentage = ParseIssuerBonus(request.params[13]);

    // perform checks
    RequirePropertyName(name);
    RequireExistingProperty(propertyIdDesired);
    RequireSameEcosystem(ecosystem, propertyIdDesired);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_IssuanceVariable(ecosystem, type, previousId, category, subcategory, name, url, data, propertyIdDesired, numTokens, deadline, earlyBonus, issuerPercentage);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendissuancefixed(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 10)
        throw runtime_error(
            RPCHelpMan{"omni_sendissuancefixed",
               "\nCreate new tokens with fixed supply.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"ecosystem", RPCArg::Type::NUM, RPCArg::Optional::NO, "the ecosystem to create the tokens in (1 for main ecosystem, 2 for test ecosystem)\n"},
                   {"type", RPCArg::Type::NUM, RPCArg::Optional::NO, "the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"},
                   {"previousid", RPCArg::Type::NUM, RPCArg::Optional::NO, "an identifier of a predecessor token (use 0 for new tokens)\n"},
                   {"category", RPCArg::Type::STR, RPCArg::Optional::NO, "a category for the new tokens (can be \"\")\n"},
                   {"subcategory", RPCArg::Type::STR, RPCArg::Optional::NO, "a subcategory for the new tokens  (can be \"\")\n"},
                   {"name", RPCArg::Type::STR, RPCArg::Optional::NO, "the name of the new tokens to create\n"},
                   {"url", RPCArg::Type::STR, RPCArg::Optional::NO, "a URL for further information about the new tokens (can be \"\")\n"},
                   {"data", RPCArg::Type::STR, RPCArg::Optional::NO, "a description for the new tokens (can be \"\")\n"},
                   {"amount", RPCArg::Type::STR, RPCArg::Optional::NO, "the number of tokens to create\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendissuancefixed", "\"3Ck2kEGLJtZw9ENj2tameMCtS3HB7uRar3\" 2 1 0 \"Companies\" \"Bitcoin Mining\" \"Quantum Miner\" \"\" \"\" \"1000000\"")
                   + HelpExampleRpc("omni_sendissuancefixed", "\"3Ck2kEGLJtZw9ENj2tameMCtS3HB7uRar3\", 2, 1, 0, \"Companies\", \"Bitcoin Mining\", \"Quantum Miner\", \"\", \"\", \"1000000\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint8_t ecosystem = ParseEcosystem(request.params[1]);
    uint16_t type = ParsePropertyType(request.params[2]);
    uint32_t previousId = ParsePreviousPropertyId(request.params[3]);
    std::string category = ParseText(request.params[4]);
    std::string subcategory = ParseText(request.params[5]);
    std::string name = ParseText(request.params[6]);
    std::string url = ParseText(request.params[7]);
    std::string data = ParseText(request.params[8]);
    int64_t amount = ParseAmount(request.params[9], type);

    // perform checks
    RequirePropertyName(name);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_IssuanceFixed(ecosystem, type, previousId, category, subcategory, name, url, data, amount);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendissuancemanaged(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 9)
        throw runtime_error(
            RPCHelpMan{"omni_sendissuancemanaged",
               "\nCreate new tokens with manageable supply.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"ecosystem", RPCArg::Type::NUM, RPCArg::Optional::NO, "the ecosystem to create the tokens in (1 for main ecosystem, 2 for test ecosystem)\n"},
                   {"type", RPCArg::Type::NUM, RPCArg::Optional::NO, "the type of the tokens to create: (1 for indivisible tokens, 2 for divisible tokens)\n"},
                   {"previousid", RPCArg::Type::NUM, RPCArg::Optional::NO, "an identifier of a predecessor token (use 0 for new tokens)\n"},
                   {"category", RPCArg::Type::STR, RPCArg::Optional::NO, "a category for the new tokens (can be \"\")\n"},
                   {"subcategory", RPCArg::Type::STR, RPCArg::Optional::NO, "a subcategory for the new tokens  (can be \"\")\n"},
                   {"name", RPCArg::Type::STR, RPCArg::Optional::NO, "the name of the new tokens to create\n"},
                   {"url", RPCArg::Type::STR, RPCArg::Optional::NO, "a URL for further information about the new tokens (can be \"\")\n"},
                   {"data", RPCArg::Type::STR, RPCArg::Optional::NO, "a description for the new tokens (can be \"\")\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendissuancemanaged", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\" 2 1 0 \"Companies\" \"Bitcoin Mining\" \"Quantum Miner\" \"\" \"\"")
                   + HelpExampleRpc("omni_sendissuancemanaged", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\", 2, 1, 0, \"Companies\", \"Bitcoin Mining\", \"Quantum Miner\", \"\", \"\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint8_t ecosystem = ParseEcosystem(request.params[1]);
    uint16_t type = ParsePropertyType(request.params[2]);
    uint32_t previousId = ParsePreviousPropertyId(request.params[3]);
    std::string category = ParseText(request.params[4]);
    std::string subcategory = ParseText(request.params[5]);
    std::string name = ParseText(request.params[6]);
    std::string url = ParseText(request.params[7]);
    std::string data = ParseText(request.params[8]);

    // perform checks
    RequirePropertyName(name);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_IssuanceManaged(ecosystem, type, previousId, category, subcategory, name, url, data);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendsto(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() < 3 || request.params.size() > 5)
        throw runtime_error(
            RPCHelpMan{"omni_sendsto",
               "\nCreate and broadcast a send-to-owners transaction.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to distribute\n"},
                   {"amount", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount to distribute\n"},
                   {"redeemaddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "an address that can spend the transaction dust (sender by default)\n"},
                   {"distributionproperty", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "the identifier of the property holders to distribute to\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendsto", "\"32Z3tJccZuqQZ4PhJR2hxHC3tjgjA8cbqz\" \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\" 3 \"5000\"")
                   + HelpExampleRpc("omni_sendsto", "\"32Z3tJccZuqQZ4PhJR2hxHC3tjgjA8cbqz\", \"37FaKponF7zqoMLUjEiko25pDiuVH5YLEa\", 3, \"5000\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyId = ParsePropertyId(request.params[1]);
    int64_t amount = ParseAmount(request.params[2], isPropertyDivisible(propertyId));
    std::string redeemAddress = (request.params.size() > 3 && !ParseText(request.params[3]).empty()) ? ParseAddress(request.params[3]): "";
    uint32_t distributionPropertyId = (request.params.size() > 4) ? ParsePropertyId(request.params[4]) : propertyId;

    // perform checks
    RequireBalance(fromAddress, propertyId, amount);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_SendToOwners(propertyId, amount, distributionPropertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", redeemAddress, 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendgrant(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() < 4 || request.params.size() > 5)
        throw runtime_error(
            RPCHelpMan{"omni_sendgrant",
               "\nIssue or grant new units of managed tokens.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the receiver of the tokens (sender by default, can be \"\")\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to grant\n"},
                   {"amount", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens to create\n"},
                   {"memo", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "a text note attached to this transaction (none by default)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendgrant", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\" \"\" 51 \"7000\"")
                   + HelpExampleRpc("omni_sendgrant", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\", \"\", 51, \"7000\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    std::string toAddress = !ParseText(request.params[1]).empty() ? ParseAddress(request.params[1]): "";
    uint32_t propertyId = ParsePropertyId(request.params[2]);
    int64_t amount = ParseAmount(request.params[3], isPropertyDivisible(propertyId));
    std::string memo = (request.params.size() > 4) ? ParseText(request.params[4]): "";

    // perform checks
    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_Grant(propertyId, amount, memo);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendrevoke(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() < 3 || request.params.size() > 4)
        throw runtime_error(
            RPCHelpMan{"omni_sendrevoke",
               "\nRevoke units of managed tokens.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to revoke the tokens from\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to revoke\n"},
                   {"amount", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens to revoke\n"},
                   {"memo", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "a text note attached to this transaction (none by default)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendrevoke", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\" \"\" 51 \"100\"")
                   + HelpExampleRpc("omni_sendrevoke", "\"3HsJvhr9qzgRe3ss97b1QHs38rmaLExLcH\", \"\", 51, \"100\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyId = ParsePropertyId(request.params[1]);
    int64_t amount = ParseAmount(request.params[2], isPropertyDivisible(propertyId));
    std::string memo = (request.params.size() > 3) ? ParseText(request.params[3]): "";

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
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendclosecrowdsale(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            RPCHelpMan{"omni_sendclosecrowdsale",
               "\nManually close a crowdsale.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address associated with the crowdsale to close\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the crowdsale to close\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendclosecrowdsale", "\"3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo\" 70")
                   + HelpExampleRpc("omni_sendclosecrowdsale", "\"3JYd75REX3HXn1vAU83YuGfmiPXW7BpYXo\", 70")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyId = ParsePropertyId(request.params[1]);

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
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendtrade(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 5)
        throw runtime_error(
            RPCHelpMan{"omni_sendtrade",
               "\nPlace a trade offer on the distributed token exchange.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to trade with\n"},
                   {"propertyidforsale", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to list for sale\n"},
                   {"amountforsale", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens to list for sale\n"},
                   {"propertiddesired", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens desired in exchange\n"},
                   {"amountdesired", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens desired in exchange\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendtrade", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\" 31 \"250.0\" 1 \"10.0\"")
                   + HelpExampleRpc("omni_sendtrade", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\", 31, \"250.0\", 1, \"10.0\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(request.params[1]);
    int64_t amountForSale = ParseAmount(request.params[2], isPropertyDivisible(propertyIdForSale));
    uint32_t propertyIdDesired = ParsePropertyId(request.params[3]);
    int64_t amountDesired = ParseAmount(request.params[4], isPropertyDivisible(propertyIdDesired));

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
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendcanceltradesbyprice(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 5)
        throw runtime_error(
            RPCHelpMan{"omni_sendcanceltradesbyprice",
               "\nCancel offers on the distributed token exchange with the specified price.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to trade with\n"},
                   {"propertyidforsale", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens listed for sale\n"},
                   {"amountforsale", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens to listed for sale\n"},
                   {"propertiddesired", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens desired in exchange\n"},
                   {"amountdesired", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens desired in exchange\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendcanceltradesbyprice", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\" 31 \"100.0\" 1 \"5.0\"")
                   + HelpExampleRpc("omni_sendcanceltradesbyprice", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\", 31, \"100.0\", 1, \"5.0\"")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(request.params[1]);
    int64_t amountForSale = ParseAmount(request.params[2], isPropertyDivisible(propertyIdForSale));
    uint32_t propertyIdDesired = ParsePropertyId(request.params[3]);
    int64_t amountDesired = ParseAmount(request.params[4], isPropertyDivisible(propertyIdDesired));

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
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendcanceltradesbypair(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 3)
        throw runtime_error(
            RPCHelpMan{"omni_sendcanceltradesbypair",
               "\nCancel all offers on the distributed token exchange with the given currency pair.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to trade with\n"},
                   {"propertyidforsale", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens listed for sale\n"},
                   {"propertiddesired", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens desired in exchange\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendcanceltradesbypair", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\" 1 31")
                   + HelpExampleRpc("omni_sendcanceltradesbypair", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\", 1, 31")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyIdForSale = ParsePropertyId(request.params[1]);
    uint32_t propertyIdDesired = ParsePropertyId(request.params[2]);

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
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendcancelalltrades(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            RPCHelpMan{"omni_sendcancelalltrades",
               "\nCancel all offers on the distributed token exchange.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to trade with\n"},
                   {"ecosystem", RPCArg::Type::NUM, RPCArg::Optional::NO, "the ecosystem of the offers to cancel (1 for main ecosystem, 2 for test ecosystem)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendcancelalltrades", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\" 1")
                   + HelpExampleRpc("omni_sendcancelalltrades", "\"3BydPiSLPP3DR5cf726hDQ89fpqWLxPKLR\", 1")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint8_t ecosystem = ParseEcosystem(request.params[1]);

    // perform checks
    // TODO: check, if there are matching offers to cancel

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExCancelEcosystem(ecosystem);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendchangeissuer(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 3)
        throw runtime_error(
            RPCHelpMan{"omni_sendchangeissuer",
               "\nChange the issuer on record of the given tokens.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address associated with the tokens\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to transfer administrative control to\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendchangeissuer", "\"1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu\" \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 3")
                   + HelpExampleRpc("omni_sendchangeissuer", "\"1ARjWDkZ7kT9fwjPrjcQyvbXDkEySzKHwu\", \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 3")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    std::string toAddress = ParseAddress(request.params[1]);
    uint32_t propertyId = ParsePropertyId(request.params[2]);

    // perform checks
    RequireExistingProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_ChangeIssuer(propertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, toAddress, "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendenablefreezing(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            RPCHelpMan{"omni_sendenablefreezing",
               "\nEnables address freezing for a centrally managed property.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the issuer of the tokens\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendenablefreezing", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 3")
                   + HelpExampleRpc("omni_sendenablefreezing", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 3")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyId = ParsePropertyId(request.params[1]);

    // perform checks
    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_EnableFreezing(propertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_senddisablefreezing(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            RPCHelpMan{"omni_senddisablefreezing",
               "\nDisables address freezing for a centrally managed property.\n"
               "\nIMPORTANT NOTE:  Disabling freezing for a property will UNFREEZE all frozen addresses for that property!",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the issuer of the tokens\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_senddisablefreezing", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 3")
                   + HelpExampleRpc("omni_senddisablefreezing", "\"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 3")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint32_t propertyId = ParsePropertyId(request.params[1]);

    // perform checks
    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);
    RequireTokenIssuer(fromAddress, propertyId);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DisableFreezing(propertyId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendfreeze(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 4)
        throw runtime_error(
            RPCHelpMan{"omni_sendfreeze",
               "\nFreeze an address for a centrally managed token.\n"
               "\nNote: Only the issuer may freeze tokens, and only if the token is of the managed type with the freezing option enabled.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from (must be the issuer of the property)\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to freeze tokens for\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the property to freeze tokens for (must be managed type and have freezing option enabled)\n"},
                   {"amount", RPCArg::Type::NUM, RPCArg::Optional::NO, "the amount of tokens to freeze (note: this is unused - once frozen an address cannot send any transactions for the property)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendfreeze", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 1 0")
                   + HelpExampleRpc("omni_sendfreeze", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 1, 0")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    std::string refAddress = ParseAddress(request.params[1]);
    uint32_t propertyId = ParsePropertyId(request.params[2]);
    int64_t amount = ParseAmount(request.params[3], isPropertyDivisible(propertyId));

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
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendunfreeze(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 4)
        throw runtime_error(
            RPCHelpMan{"omni_sendunfreeze",
               "\nUnfreezes an address for a centrally managed token.\n"
               "\nNote: Only the issuer may unfreeze tokens.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from (must be the issuer of the property)\n"},
                   {"toaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to unfreeze tokens for\n"},
                   {"propertyid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the property to unfreeze tokens for (must be managed type and have freezing option enabled)\n"},
                   {"amount", RPCArg::Type::NUM, RPCArg::Optional::NO, "the amount of tokens to unfreeze (note: this is unused - once frozen an address cannot send any transactions for the property)\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendunfreeze", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\" 1 0")
                   + HelpExampleRpc("omni_sendunfreeze", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", \"3HTHRxu3aSDV4deakjC7VmsiUp7c6dfbvs\", 1, 0")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    std::string refAddress = ParseAddress(request.params[1]);
    uint32_t propertyId = ParsePropertyId(request.params[2]);
    int64_t amount = ParseAmount(request.params[3], isPropertyDivisible(propertyId));

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
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendactivation(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 4)
        throw runtime_error(
            RPCHelpMan{"omni_sendactivation",
               "\nActivate a protocol feature.\n"
               "\nNote: Omni Core ignores activations from unauthorized sources.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"featureid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the feature to activate\n"},
                   {"block", RPCArg::Type::NUM, RPCArg::Optional::NO, "the activation block\n"},
                   {"minclientversion", RPCArg::Type::NUM, RPCArg::Optional::NO, "the minimum supported client version\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendactivation", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" 1 370000 999")
                   + HelpExampleRpc("omni_sendactivation", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", 1, 370000, 999")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint16_t featureId = request.params[1].get_int();
    uint32_t activationBlock = request.params[2].get_int();
    uint32_t minClientVersion = request.params[3].get_int();

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_ActivateFeature(featureId, activationBlock, minClientVersion);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_senddeactivation(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            RPCHelpMan{"omni_senddeactivation",
               "\nDeactivate a protocol feature.  For Emergency Use Only.\n"
               "\nNote: Omni Core ignores deactivations from unauthorized sources.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"featureid", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the feature to activate\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_senddeactivation", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" 1")
                   + HelpExampleRpc("omni_senddeactivation", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", 1")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    uint16_t featureId = request.params[1].get_int64();

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_DeactivateFeature(featureId);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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

static UniValue omni_sendalert(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    std::unique_ptr<interfaces::Wallet> pwallet = interfaces::MakeWallet(wallet);

    if (request.fHelp || request.params.size() != 4)
        throw runtime_error(
            RPCHelpMan{"omni_sendalert",
               "\nCreates and broadcasts an Omni Core alert.\n"
               "\nNote: Omni Core ignores alerts from unauthorized sources.\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"alerttype", RPCArg::Type::NUM, RPCArg::Optional::NO, "the alert type\n"},
                   {"expiryvalue", RPCArg::Type::NUM, RPCArg::Optional::NO, "the value when the alert expires (depends on alert type)\n"},
                   {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "the user-faced alert message\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   HelpExampleCli("omni_sendalert", "")
                   + HelpExampleRpc("omni_sendalert", "")
               }
            }.ToString());

    // obtain parameters & info
    std::string fromAddress = ParseAddress(request.params[0]);
    int64_t tempAlertType = request.params[1].get_int64();
    if (tempAlertType < 1 || 65535 < tempAlertType) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Alert type is out of range");
    }
    uint16_t alertType = static_cast<uint16_t>(tempAlertType);
    int64_t tempExpiryValue = request.params[2].get_int64();
    if (tempExpiryValue < 1 || 4294967295LL < tempExpiryValue) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Expiry value is out of range");
    }
    uint32_t expiryValue = static_cast<uint32_t>(tempExpiryValue);
    std::string alertMessage = ParseText(request.params[3]);

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_OmniCoreAlert(alertType, expiryValue, alertMessage);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit, pwallet.get());

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


static UniValue trade_MP(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 6)
        throw runtime_error(
            RPCHelpMan{"trade_MP",
               "\nNote: this command is deprecated, and was replaced by:\n",
               {
                   {"fromaddress", RPCArg::Type::STR, RPCArg::Optional::NO, "the address to send from\n"},
                   {"propertyidforsale", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens to list for sale\n"},
                   {"amountforsale", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens to listed for sale\n"},
                   {"propertyiddesired", RPCArg::Type::NUM, RPCArg::Optional::NO, "the identifier of the tokens desired in exchange\n"},
                   {"amountdesired", RPCArg::Type::STR, RPCArg::Optional::NO, "the amount of tokens desired in exchange\n"},
                   {"action", RPCArg::Type::NUM, RPCArg::Optional::NO, "trade action to take\n"},
               },
               RPCResult{
                   "\"hash\"                  (string) the hex-encoded transaction hash\n"
               },
               RPCExamples{
                   " - sendtrade_OMNI\n"
                   " - sendcanceltradebyprice_OMNI\n"
                   " - sendcanceltradebypair_OMNI\n"
                   " - sendcanceltradebypair_OMNI\n"
               }
            }.ToString());

    UniValue values(UniValue::VARR);
    uint8_t action = ParseMetaDExAction(request.params[5]);

    // Forward to the new commands, based on action value
    switch (action) {
        case CMPTransaction::ADD:
        {
            values.push_back(request.params[0]); // fromAddress
            values.push_back(request.params[1]); // propertyIdForSale
            values.push_back(request.params[2]); // amountForSale
            values.push_back(request.params[3]); // propertyIdDesired
            values.push_back(request.params[4]); // amountDesired
            return omni_sendtrade(request);
        }
        case CMPTransaction::CANCEL_AT_PRICE:
        {
            values.push_back(request.params[0]); // fromAddress
            values.push_back(request.params[1]); // propertyIdForSale
            values.push_back(request.params[2]); // amountForSale
            values.push_back(request.params[3]); // propertyIdDesired
            values.push_back(request.params[4]); // amountDesired
            return omni_sendcanceltradesbyprice(request);
        }
        case CMPTransaction::CANCEL_ALL_FOR_PAIR:
        {
            values.push_back(request.params[0]); // fromAddress
            values.push_back(request.params[1]); // propertyIdForSale
            values.push_back(request.params[3]); // propertyIdDesired
            return omni_sendcanceltradesbypair(request);
        }
        case CMPTransaction::CANCEL_EVERYTHING:
        {
            uint8_t ecosystem = 0;
            if (isMainEcosystemProperty(request.params[1].get_int64())
                    && isMainEcosystemProperty(request.params[3].get_int64())) {
                ecosystem = OMNI_PROPERTY_MSC;
            }
            if (isTestEcosystemProperty(request.params[1].get_int64())
                    && isTestEcosystemProperty(request.params[3].get_int64())) {
                ecosystem = OMNI_PROPERTY_TMSC;
            }
            values.push_back(request.params[0]); // fromAddress
            values.push_back(ecosystem);
            return omni_sendcancelalltrades(request);
        }
    }

    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid action (1,2,3,4 only)");
}

static const CRPCCommand commands[] =
{ //  category                             name                            actor (function)               okSafeMode
  //  ------------------------------------ ------------------------------- ------------------------------ ----------
    { "omni layer (transaction creation)", "omni_sendrawtx",               &omni_sendrawtx,               {"fromaddress", "rawtransaction", "referenceaddress", "redeemaddress", "referenceamount"} },
    { "omni layer (transaction creation)", "omni_send",                    &omni_send,                    {"fromaddress", "toaddress", "propertyid", "amount", "redeemaddress", "referenceamount"} },
    { "omni layer (transaction creation)", "omni_senddexsell",             &omni_senddexsell,             {"fromaddress", "propertyidforsale", "amountforsale", "amountdesired", "paymentwindow", "minacceptfee", "action"} },
    { "omni layer (transaction creation)", "omni_sendnewdexorder",         &omni_sendnewdexorder,         {"fromaddress", "propertyidforsale", "amountforsale", "amountdesired", "paymentwindow", "minacceptfee"} },
    { "omni layer (transaction creation)", "omni_sendupdatedexorder",      &omni_sendupdatedexorder,      {"fromaddress", "propertyidforsale", "amountforsale", "amountdesired", "paymentwindow", "minacceptfee"} },
    { "omni layer (transaction creation)", "omni_sendcanceldexorder",      &omni_sendcanceldexorder,      {"fromaddress", "propertyidforsale"} },
    { "omni layer (transaction creation)", "omni_senddexaccept",           &omni_senddexaccept,           {"fromaddress", "toaddress", "propertyid", "amount", "override"} },
    { "omni layer (transaction creation)", "omni_senddexpay",              &omni_senddexpay,              {"fromaddress", "toaddress", "propertyid", "amount"} },
    { "omni layer (transaction creation)", "omni_sendissuancecrowdsale",   &omni_sendissuancecrowdsale,   {"fromaddress", "ecosystem", "type", "previousid", "category", "subcategory", "name", "url", "data", "propertyiddesired", "tokensperunit", "deadline", "earlybonus", "issuerpercentage"} },
    { "omni layer (transaction creation)", "omni_sendissuancefixed",       &omni_sendissuancefixed,       {"fromaddress", "ecosystem", "type", "previousid", "category", "subcategory", "name", "url", "data", "amount"} },
    { "omni layer (transaction creation)", "omni_sendissuancemanaged",     &omni_sendissuancemanaged,     {"fromaddress", "ecosystem", "type", "previousid", "category", "subcategory", "name", "url", "data"} },
    { "omni layer (transaction creation)", "omni_sendtrade",               &omni_sendtrade,               {"fromaddress", "propertyidforsale", "amountforsale", "propertiddesired", "amountdesired"} },
    { "omni layer (transaction creation)", "omni_sendcanceltradesbyprice", &omni_sendcanceltradesbyprice, {"fromaddress", "propertyidforsale", "amountforsale", "propertiddesired", "amountdesired"} },
    { "omni layer (transaction creation)", "omni_sendcanceltradesbypair",  &omni_sendcanceltradesbypair,  {"fromaddress", "propertyidforsale", "propertiddesired"} },
    { "omni layer (transaction creation)", "omni_sendcancelalltrades",     &omni_sendcancelalltrades,     {"fromaddress", "ecosystem"} },
    { "omni layer (transaction creation)", "omni_sendsto",                 &omni_sendsto,                 {"fromaddress", "propertyid", "amount", "redeemaddress", "distributionproperty"} },
    { "omni layer (transaction creation)", "omni_sendgrant",               &omni_sendgrant,               {"fromaddress", "toaddress", "propertyid", "amount", "memo"} },
    { "omni layer (transaction creation)", "omni_sendrevoke",              &omni_sendrevoke,              {"fromaddress", "propertyid", "amount", "memo"} },
    { "omni layer (transaction creation)", "omni_sendclosecrowdsale",      &omni_sendclosecrowdsale,      {"fromaddress", "propertyid"} },
    { "omni layer (transaction creation)", "omni_sendchangeissuer",        &omni_sendchangeissuer,        {"fromaddress", "toaddress", "propertyid"} },
    { "omni layer (transaction creation)", "omni_sendall",                 &omni_sendall,                 {"fromaddress", "toaddress", "ecosystem", "redeemaddress", "referenceamount"} },
    { "omni layer (transaction creation)", "omni_sendenablefreezing",      &omni_sendenablefreezing,      {"fromaddress", "propertyid"} },
    { "omni layer (transaction creation)", "omni_senddisablefreezing",     &omni_senddisablefreezing,     {"fromaddress", "propertyid"} },
    { "omni layer (transaction creation)", "omni_sendfreeze",              &omni_sendfreeze,              {"fromaddress", "toaddress", "propertyid", "amount"} },
    { "omni layer (transaction creation)", "omni_sendunfreeze",            &omni_sendunfreeze,            {"fromaddress", "toaddress", "propertyid", "amount"} },
    { "hidden",                            "omni_senddeactivation",        &omni_senddeactivation,        {"fromaddress", "featureid"} },
    { "hidden",                            "omni_sendactivation",          &omni_sendactivation,          {"fromaddress", "featureid", "block", "minclientversion"} },
    { "hidden",                            "omni_sendalert",               &omni_sendalert,               {"fromaddress", "alerttype", "expiryvalue", "message"} },
    { "omni layer (transaction creation)", "omni_funded_send",             &omni_funded_send,             {"fromaddress", "toaddress", "propertyid", "amount", "feeaddress"} },
    { "omni layer (transaction creation)", "omni_funded_sendall",          &omni_funded_sendall,          {"fromaddress", "toaddress", "ecosystem", "feeaddress"} },

    /* deprecated: */
    { "hidden",                            "sendrawtx_MP",                 &omni_sendrawtx,               {"fromaddress", "rawtransaction", "referenceaddress", "redeemaddress", "referenceamount"} },
    { "hidden",                            "send_MP",                      &omni_send,                    {"fromaddress", "toaddress", "propertyid", "amount", "redeemaddress", "referenceamount"} },
    { "hidden",                            "sendtoowners_MP",              &omni_sendsto,                 {"fromaddress", "propertyid", "amount", "redeemaddress", "distributionproperty"} },
    { "hidden",                            "trade_MP",                     &trade_MP,                     {"fromaddress", "propertyidforsale", "amountforsale", "propertiddesired", "amountdesired", "action"} },
};

void RegisterOmniTransactionCreationRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
