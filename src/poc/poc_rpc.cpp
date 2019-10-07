// Copyright (c) 2017-2018 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poc/poc.h>
#include <base58.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <net.h>
#include <poc/passphrase.h>
#include <rpc/safemode.h>
#include <rpc/server.h>
#include <util.h>
#include <utilstrencodings.h>
#include <univalue.h>
#include <validation.h>

#include <iomanip>
#include <sstream>

namespace poc { namespace rpc {

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

    if (IsInitialBlockDownload()) {
        throw std::runtime_error("Is initial block downloading!");
    }

    LOCK(cs_main);
    const CBlockIndex *pindexLast = chainActive.Tip();
    if (pindexLast == nullptr) {
        throw std::runtime_error("Block chain tip is empty!");
    }

    UniValue result(UniValue::VOBJ);
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
            "4. \"address\"         (string, optional) Target address or private key (BHDIP007) for mining\n"
            "5. \"checkBind\"       (boolean, optional, true) Check bind for BHDIP006\n"
            "\nResult:\n"
            "{\n"
            "  [ result ]                  (string) Submit result: 'success' or others \n"
            "  [ deadline ]                (integer, optional) Current block generation signature\n"
            "  [ height ]                  (integer, optional) Target block height\n"
            "  [ targetDeadline ]          (number) Current acceptable deadline \n"
            "}\n"
        );
    }

    if (IsInitialBlockDownload())
        throw std::runtime_error("Is initial block downloading!");

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
    const CBlockIndex *pindexMining = chainActive[nTargetHeight < 1 ? chainActive.Height() : (nTargetHeight - 1)];
    if (pindexMining == nullptr)
        throw std::runtime_error("Invalid mining height");

    UniValue result(UniValue::VOBJ);
    try {
        uint64_t bestDeadline = 0;
        uint64_t deadline = AddNonce(bestDeadline, *pindexMining, nNonce, nPlotterId, generateTo, fCheckBind, Params().GetConsensus());
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
            "BitcoinHD mining address\n"
        );
    }

    std::string address;
    if (!poc::AddMiningSignaturePrivkey(request.params[0].get_str(), &address))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    return address;
}

static UniValue listSignAddresses(const JSONRPCRequest& request)
{
    if (request.fHelp) {
        throw std::runtime_error(
            "listsignaddresses\n"
            "\nList signature addresses for signature.\n"
            "\nResult:\n"
            "BitcoinHD address\n"
        );
    }

    UniValue addresses(UniValue::VARR);
    for (const std::string &address : poc::GetMiningSignatureAddresses()) {
        addresses.push_back(address);
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

    std::string passphrase = poc::generatePassPhrase();
    uint64_t plotterID = PocLegacy::GeneratePlotterId(passphrase);

    UniValue result(UniValue::VOBJ);
    result.pushKV("passphrase", passphrase);
    result.pushKV("plotterId", std::to_string(plotterID));
    return result;
}

}}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)                  argNames
  //  --------------------- ------------------------  -----------------------           ----------
    { "hidden",             "getMiningInfo",          &poc::rpc::getMiningInfo,         { } },
    { "hidden",             "submitNonce",            &poc::rpc::submitNonce,           { "nonce", "plotterId", "height", "address", "checkBind" } },
    { "poc",                "addsignprivkey",         &poc::rpc::addSignPrivkey,        { "privkey" } },
    { "poc",                "listsignaddresses",      &poc::rpc::listSignAddresses,     { } },
    { "poc",                "getplotterid",           &poc::rpc::getPlotterId,          { "passPhrase" } },
    { "poc",                "getnewplotter",          &poc::rpc::getNewPlotter,         { } },
};

void RegisterPoCRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++) {
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
    }
}
