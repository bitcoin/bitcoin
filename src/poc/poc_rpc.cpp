// Copyright (c) 2017-2020 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poc/poc.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <key_io.h>
#include <net.h>
#include <poc/passphrase.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <util/strencodings.h>
#include <univalue.h>
#include <validation.h>

#include <iomanip>
#include <sstream>

static UniValue getMiningInfo(const JSONRPCRequest& request)
{
    if (request.fHelp) {
        throw std::runtime_error(
            "getMiningInfo\n"
            "\nGet current mining information.\n"
            "\nResult:\n"
            "{\n"
            "  [ height ]                  (integer) Next block height\n"
            "  [ generationSignature ]     (string) Current block generation signature\n"
            "  [ baseTarget ]              (string) Current block base target \n"
            "  [ targetDeadline ]          (number) Max acceptable deadline \n"
            "}\n"
        );
    }

    UniValue result(UniValue::VOBJ);

    LOCK(cs_main);
    const CBlockIndex *pindexLast = ChainActive().Tip();
    if (pindexLast == nullptr || pindexLast->nHeight < 1) {
        result.pushKV("result", "error");
        result.pushKV("errorCode", "400");
        result.pushKV("errorDescription", "Block chain tip is empty!");
        return result;
    }
    if (pindexLast->nHeight != 1 && ChainstateActive().IsInitialBlockDownload()) {
        result.pushKV("result", "error");
        result.pushKV("errorCode", "400");
        result.pushKV("errorDescription", "Is initial block downloading!");
        return result;
    }
    if (pindexLast->nHeight == 1 && Params().GetConsensus().nBeginMiningTime > GetTime()) {
        result.pushKV("result", "error");
        result.pushKV("errorCode", "400");
        result.pushKV("errorDescription", "Waiting for begining!");
        return result;
    }

    result.pushKV("height", pindexLast->nHeight + 1);
    result.pushKV("generationSignature", HexStr(pindexLast->GetNextGenerationSignature()));
    result.pushKV("baseTarget", std::to_string(pindexLast->nBaseTarget));
    result.pushKV("targetDeadline", (uint64_t) poc::MAX_TARGET_DEADLINE);

    return result;
}

static UniValue submitNonce(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 5) {
        throw std::runtime_error(
            "submitNonce \"nonce\" \"plotterId\" (height \"address\" checkBind)\n"
            "\nSubmit mining nonce.\n"
            "\nArguments:\n"
            "1. \"nonce\"           (string, required) Nonce\n"
            "2. \"plotterId\"       (string, required) Plotter ID\n"
            "3. \"height\"          (integer, optional) Target height for mining\n"
            "4. \"address\"         (string, optional) Target address or private key (QTCIP007) for mining\n"
            "5. \"checkBind\"       (boolean, optional, true) Check bind for QTCIP006\n"
            "\nResult:\n"
            "{\n"
            "  [ result ]                  (string) Submit result: 'success' or others \n"
            "  [ deadline ]                (integer, optional) Current block generation signature\n"
            "  [ height ]                  (integer, optional) Target block height\n"
            "  [ targetDeadline ]          (number) Current acceptable deadline \n"
            "}\n"
        );
    }

    UniValue result(UniValue::VOBJ);

    uint64_t nNonce = static_cast<uint64_t>(std::stoull(request.params[0].get_str()));
    uint64_t nPlotterId = static_cast<uint64_t>(std::stoull(request.params[1].get_str()));

    int nTargetHeight = 0;
    if (request.params.size() >= 3) {
        nTargetHeight = request.params[2].isNum() ? request.params[2].get_int() : std::stoi(request.params[2].get_str());
    }

    std::string generateTo;
    if (request.params.size() >= 4) {
        generateTo = request.params[3].get_str();
    }

    bool fCheckBind = true;
    if (request.params.size() >= 5) {
        fCheckBind = request.params[4].get_bool();
    }

    LOCK(cs_main);
    const CBlockIndex *pindexMining = ChainActive()[nTargetHeight < 1 ? ChainActive().Height() : (nTargetHeight - 1)];
    if (pindexMining == nullptr || pindexMining->nHeight < 1) {
        result.pushKV("result", "error");
        result.pushKV("errorCode", "400");
        result.pushKV("errorDescription", "Invalid mining height!");
        return result;
    }
    if (pindexMining->nHeight != 1 && ChainstateActive().IsInitialBlockDownload()) {
        result.pushKV("result", "error");
        result.pushKV("errorCode", "400");
        result.pushKV("errorDescription", "Is initial block downloading!");
        return result;
    }
    if (pindexMining->nHeight == 1 && Params().GetConsensus().nBeginMiningTime > GetTime()) {
        result.pushKV("result", "error");
        result.pushKV("errorCode", "400");
        result.pushKV("errorDescription", "Waiting for begining!");
        return result;
    }

    try {
        uint64_t bestDeadline = 0;
        uint64_t deadline = poc::AddNonce(bestDeadline, *pindexMining, nNonce, nPlotterId, generateTo, fCheckBind, Params().GetConsensus());
        result.pushKV("result", "success");
        result.pushKV("deadline", deadline);
        result.pushKV("height", pindexMining->nHeight + 1);
        result.pushKV("targetDeadline", (bestDeadline == 0 ? poc::MAX_TARGET_DEADLINE : bestDeadline));
    } catch (const UniValue& objError) {
        result.pushKV("result", "error");
        result.pushKV("errorCode", objError.isObject() ? objError["code"].getValStr() : "400");
        result.pushKV("errorDescription", objError.isObject() ? objError["message"].getValStr() : objError.getValStr());
    } catch (const std::exception& e) {
        result.pushKV("result", "error");
        result.pushKV("errorCode", "500");
        result.pushKV("errorDescription", e.what());
    }
    return result;
}

static UniValue addSignPrivkey(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "addsignprivkey \"privkey\"\n"
            "\nAdd private key for signature.\n"
            "\nArguments:\n"
            "1. \"privkey\"      (string, required) The string of the private key\n"
            "\nResult:\n"
            "Qitcoin mining address\n"
        );
    }

    CTxDestination dest = poc::AddMiningSignaturePrivkey(DecodeSecret(request.params[0].get_str()));
    if (!IsValidDestination(dest))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    return EncodeDestination(dest);
}

static UniValue listSignAddresses(const JSONRPCRequest& request)
{
    if (request.fHelp) {
        throw std::runtime_error(
            "listsignaddresses\n"
            "\nList signature addresses for signature.\n"
            "\nResult:\n"
            "Qitcoin address\n"
        );
    }

    UniValue addresses(UniValue::VARR);
    for (const CTxDestination &dest : poc::GetMiningSignatureAddresses()) {
        addresses.push_back(EncodeDestination(dest));
    }

    return addresses;
}

static UniValue getPlotterId(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "getplotterid \"passphrase\"\n"
            "\nGet potter id from passphrase.\n"
            "\nArguments:\n"
            "1. \"passphrase\"      (string, required) The string of the passphrase\n"
            "\nResult:\n"
            "Plotter id\n"
        );
    }

    return PocLegacy::GeneratePlotterId(request.params[0].get_str());;
}

static UniValue getNewPlotter(const JSONRPCRequest& request)
{
    if (request.fHelp) {
        throw std::runtime_error(
            "getnewplotter\n"
            "\nGet new plotter account.\n"
            "\nResult:\n"
            "{\n"
            "  [ passphrase ]              (string) The passphrase\n"
            "  [ plotterId ]               (string) The plotter ID from passphrase\n"
            "}\n"
        );
    }

    std::string passphrase = poc::generatePassphrase();
    uint64_t plotterID = PocLegacy::GeneratePlotterId(passphrase);

    UniValue result(UniValue::VOBJ);
    result.pushKV("passphrase", passphrase);
    result.pushKV("plotterId", std::to_string(plotterID));
    return result;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)                  argNames
  //  --------------------- ------------------------  ----------------------  ----------
    { "poc",                "addsignprivkey",         &addSignPrivkey,        { "privkey" } },
    { "poc",                "listsignaddresses",      &listSignAddresses,     { } },
    { "poc",                "getplotterid",           &getPlotterId,          { "passPhrase" } },
    { "poc",                "getnewplotter",          &getNewPlotter,         { } },

    //! Burst mining compatible
    { "hidden",             "getMiningInfo",          &getMiningInfo,         { } },
    { "hidden",             "submitNonce",            &submitNonce,           { "nonce", "plotterId", "height", "address", "checkBind" } },
};

void RegisterPoCRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++) {
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
    }
}
