// Copyright (c) 2015 The ShadowCoin developers
// Copyright (c) 2017-2018 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>

#include <util.h>
#include <utilstrencodings.h>
#include <rpc/util.h>
#include <key.h>
#include <key_io.h>
#include <key/extkey.h>
#include <random.h>
#include <chainparams.h>
#include <support/cleanse.h>
#include <key/mnemonic.h>

#include <string>
#include <univalue.h>

//typedef std::basic_string<char, std::char_traits<char>, secure_allocator<char> > SecureString;

int GetLanguageOffset(std::string sIn)
{
    int nLanguage = -1;
    std::transform(sIn.begin(), sIn.end(), sIn.begin(), ::tolower);

    for (size_t k = 1; k < WLL_MAX; ++k)
    {
        if (sIn != mnLanguagesTag[k])
            continue;
        nLanguage = k;
        break;
    };

    if (nLanguage < 1 || nLanguage >= WLL_MAX)
        throw std::runtime_error("Unknown language.");

    return nLanguage;
};

UniValue mnemonic(const JSONRPCRequest &request)
{
    static const char *help = ""
        "mnemonic new|decode|addchecksum|dumpwords|listlanguages\n"
        "mnemonic new ( \"password\" language nBytesEntropy bip44 )\n"
        "    Generate a new extended key and mnemonic\n"
        "    password, can be blank "", default blank\n"
        "    language, english|french|japanese|spanish|chinese_s|chinese_t|italian|korean, default english\n"
        "    nBytesEntropy, 16 -> 64, default 32\n"
        "    bip44, true|false, default true\n"
        "mnemonic decode \"password\" \"mnemonic\" ( bip44 )\n"
        "    Decode mnemonic\n"
        "    bip44, true|false, default true\n"
        "mnemonic addchecksum \"mnemonic\"\n"
        "    Add checksum words to mnemonic.\n"
        "    Final no of words in mnemonic must be divisible by three.\n"
        "mnemonic dumpwords ( \"language\" )\n"
        "    Print list of words.\n"
        "    language, default english\n"
         "mnemonic listlanguages\n"
        "    Print list of supported languages.\n"
        "\n";

    if (request.fHelp || request.params.size() > 5) // defaults to info, will always take at least 1 parameter
        throw std::runtime_error(help);

    std::string mode = "";

    if (request.params.size() > 0)
    {
        std::string s = request.params[0].get_str();
        std::string st = " " + s + " "; // Note the spaces
        std::transform(st.begin(), st.end(), st.begin(), ::tolower);
        static const char *pmodes = " new decode addchecksum dumpwords listlanguages ";
        if (strstr(pmodes, st.c_str()) != nullptr)
        {
            st.erase(std::remove(st.begin(), st.end(), ' '), st.end());
            mode = st;
        } else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown mode.");
        };
    };


    UniValue result(UniValue::VOBJ);

    if (mode == "new")
    {
        int nLanguage = WLL_ENGLISH;
        int nBytesEntropy = 32;
        std::string sPassword = "";
        std::string sError;

        if (request.params.size() > 1)
            sPassword = request.params[1].get_str();

        if (request.params.size() > 2)
            nLanguage = GetLanguageOffset(request.params[2].get_str());

        if (request.params.size() > 3)
        {
            std::stringstream sstr(request.params[3].get_str());

            sstr >> nBytesEntropy;
            if (!sstr)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid num bytes entropy");

            if (nBytesEntropy < 16 || nBytesEntropy > 64)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Num bytes entropy out of range [16,64].");
        };

        bool fBip44 = request.params.size() > 4 ? GetBool(request.params[4]) : true;

        if (request.params.size() > 5)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Too many parameters");

        std::vector<uint8_t> vEntropy;
        std::vector<uint8_t> vSeed;
        vEntropy.resize(nBytesEntropy);

        std::string sMnemonic;
        CExtKey ekMaster;

        for (uint32_t i = 0; i < MAX_DERIVE_TRIES; ++i)
        {
            GetStrongRandBytes2(&vEntropy[0], nBytesEntropy);

            if (0 != MnemonicEncode(nLanguage, vEntropy, sMnemonic, sError))
                throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("MnemonicEncode failed %s.", sError.c_str()).c_str());

            if (0 != MnemonicToSeed(sMnemonic, sPassword, vSeed))
                throw JSONRPCError(RPC_INTERNAL_ERROR, "MnemonicToSeed failed.");

            ekMaster.SetSeed(&vSeed[0], vSeed.size());

            if (!ekMaster.IsValid())
                continue;
            break;
        };

        CExtKey58 eKey58;
        result.pushKV("mnemonic", sMnemonic);

        if (fBip44)
        {
            eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY_BTC);
            result.pushKV("master", eKey58.ToString());

            // m / purpose' / coin_type' / account' / change / address_index
            // path "44' Params().BIP44ID()
        } else
        {
            eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY);
            result.pushKV("master", eKey58.ToString());
        };

        // In c++11 strings are definitely contiguous, and before they're very unlikely not to be
        if (sMnemonic.size() > 0)
            memory_cleanse(&sMnemonic[0], sMnemonic.size());
        if (sPassword.size() > 0)
            memory_cleanse(&sPassword[0], sPassword.size());
    } else
    if (mode == "decode")
    {
        std::string sPassword;
        std::string sMnemonic;
        std::string sError;

        if (request.params.size() > 1)
            sPassword = request.params[1].get_str();
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Must specify password.");

        if (request.params.size() > 2)
            sMnemonic = request.params[2].get_str();
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Must specify mnemonic.");

        bool fBip44 = request.params.size() > 3 ? GetBool(request.params[3]) : true;

        if (request.params.size() > 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Too many parameters");

        if (sMnemonic.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Mnemonic can't be blank.");

        std::vector<uint8_t> vEntropy;
        std::vector<uint8_t> vSeed;

        // Decode to determine validity of mnemonic
        int nLanguage = -1;
        if (0 != MnemonicDecode(nLanguage, sMnemonic, vEntropy, sError))
            throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("MnemonicDecode failed %s.", sError.c_str()).c_str());

        if (0 != MnemonicToSeed(sMnemonic, sPassword, vSeed))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "MnemonicToSeed failed.");

        CExtKey ekMaster;
        CExtKey58 eKey58;
        ekMaster.SetSeed(&vSeed[0], vSeed.size());

        if (!ekMaster.IsValid())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid key.");

        if (fBip44)
        {
            eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY_BTC);
            result.pushKV("master", eKey58.ToString());

            // m / purpose' / coin_type' / account' / change / address_index
            CExtKey ekDerived;
            ekMaster.Derive(ekDerived, BIP44_PURPOSE);
            ekDerived.Derive(ekDerived, Params().BIP44ID());

            eKey58.SetKey(ekDerived, CChainParams::EXT_SECRET_KEY);
            result.pushKV("derived", eKey58.ToString());
        } else
        {
            eKey58.SetKey(ekMaster, CChainParams::EXT_SECRET_KEY);
            result.pushKV("master", eKey58.ToString());
        };

        result.pushKV("language", MnemonicGetLanguage(nLanguage));

        if (sMnemonic.size() > 0)
            memory_cleanse(&sMnemonic[0], sMnemonic.size());
        if (sPassword.size() > 0)
            memory_cleanse(&sPassword[0], sPassword.size());
    } else
    if (mode == "addchecksum")
    {
        std::string sMnemonicIn;
        std::string sMnemonicOut;
        std::string sError;
        if (request.params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Must provide input mnemonic.");

        sMnemonicIn = request.params[1].get_str();

        if (0 != MnemonicAddChecksum(-1, sMnemonicIn, sMnemonicOut, sError))
            throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("MnemonicAddChecksum failed %s", sError.c_str()).c_str());
        result.pushKV("result", sMnemonicOut);
    } else
    if (mode == "dumpwords")
    {
        int nLanguage = WLL_ENGLISH;

        if (request.params.size() > 1)
            nLanguage = GetLanguageOffset(request.params[1].get_str());

        int nWords = 0;
        UniValue arrayWords(UniValue::VARR);

        std::string sWord, sError;
        while (0 == MnemonicGetWord(nLanguage, nWords, sWord, sError))
        {
            arrayWords.push_back(sWord);
            nWords++;
        };

        result.pushKV("words", arrayWords);
        result.pushKV("num_words", nWords);
    } else
    if (mode == "listlanguages")
    {
        for (size_t k = 1; k < WLL_MAX; ++k)
        {
            std::string sName(mnLanguagesTag[k]);
            std::string sDesc(mnLanguagesDesc[k]);
            result.pushKV(sName, sDesc);
        };
    } else
    {
        throw std::runtime_error(help);
    };

    return result;
};

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "mnemonic",           "mnemonic",               &mnemonic,               {} },
};

void RegisterMnemonicRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
