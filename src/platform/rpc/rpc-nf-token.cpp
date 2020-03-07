// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <spork.h>
#include <platform/nf-token/nf-token-protocol.h>
#include "primitives/transaction.h"
#include "platform/specialtx.h"
#include "platform/platform-utils.h"
#include "platform/nf-token/nf-token-reg-tx-builder.h"
#include "platform/nf-token/nf-tokens-manager.h"
#include "platform/nf-token/nft-protocols-manager.h"
#include "specialtx-rpc-utils.h"
#include "rpc-nf-token.h"

json_spirit::Value nftoken(const json_spirit::Array& params, bool fHelp)
{
    if (!IsSporkActive(SPORK_17_NFT_TX))
    {
        throw std::runtime_error("NFT spork is off");
    }

    std::string command = Platform::GetCommand(params, "usage: nftoken register(issue)|list|get|getbytxid|totalsupply|balanceof|ownerof");

    if (command == "register" || command == "issue")
        return Platform::RegisterNfToken(params, fHelp);
    else if (command == "list")
        return Platform::ListNfTokenTxs(params, fHelp);
    else if (command == "get")
        return Platform::GetNfToken(params, fHelp);
    else if (command == "getbytxid")
        return Platform::GetNfTokenByTxId(params,fHelp);
    else if (command == "totalsupply")
        return Platform::NfTokenTotalSupply(params, fHelp);
    else if (command == "balanceof")
        return Platform::NfTokenBalanceOf(params, fHelp);
    else if (command == "ownerof")
        return Platform::NfTokenOwnerOf(params, fHelp);

    throw std::runtime_error("Invalid command: " + command);
}

namespace Platform
{
    void RegisterNfTokenHelp()
    {
        static std::string helpMessage =
                "nftoken register/issue \"nfTokenProtocol\" \"tokenId\" \"tokenOwnerAddr\" \"tokenMetadataAdminAddr\" \"metadata\" \"nfTokenRegistrar\"\n"
                "Creates and sends a new non-fungible token transaction.\n"
                "\nArguments:\n"
                "1. \"nfTokenProtocol\"           (string, required) A non-fungible token protocol symbol to use in the token creation.\n"
                "                                 The protocol name must be valid and registered previously.\n"

                "2. \"nfTokenId\"                 (string, required) The token id in hex.\n"
                "                                 The token id must be unique in the token type space.\n"

                "3. \"nfTokenOwnerAddr\"          (string, required) The token owner address, can be used in any operations with the token.\n"
                "                                 The private key belonging to this address should be or should be not known in your wallet. Depending on the NFT protocol nftRegSign field.\n"
                "                                 NftRegSign::SelfSign - the key should be known in your wallet.\n"
                "                                 NftRegSign::SignPayer - an additional registrar private key should be provided.\n"
                "                                 NftRegSign::SignByCreator - NFT protocol key should be used to sign transactions.\n"

                "4. \"nfTokenMetadataAdminAddr\"  (string, optional, default = \"0\") The metadata token administration address, can be used to modify token metadata.\n"
                "                                 The private key does not have to be known by your wallet. Can be set to 0.\n"

                "5. \"nfTokenMetadata\"           (string, optional) metadata describing the token.\n"
                "                                 It may contain text or binary in hex/base64 //TODO .\n"
                "Examples: "
                + HelpExampleCli("nftoken", R"(register/issue doc a103d4bdfaa7d22591c4dacda81ba540e37f705bae41681c082b102e647aa8e8 CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc CRWS78YF1kbWAyfceS6RfiTVzP17csUnhUux "https://metadata-json.link")")
                + HelpExampleRpc("nftoken", R"(register/issue doc a103d4bdfaa7d22591c4dacda81ba540e37f705bae41681c082b102e647aa8e8 CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc CRWS78YF1kbWAyfceS6RfiTVzP17csUnhUux "https://metadata-json.link")");


        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value RegisterNfToken(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 4 || params.size() > 6)
            RegisterNfTokenHelp();

        NfTokenRegTxBuilder nfTokenRegTxBuilder;
        nfTokenRegTxBuilder.SetTokenProtocol(params[1]).SetTokenId(params[2]).SetTokenOwnerKey(params[3]);

        if (params.size() > 4)
            nfTokenRegTxBuilder.SetMetadataAdminKey(params[4]);
        if (params.size() > 5)
            nfTokenRegTxBuilder.SetMetadata(params[5]);

        NfTokenRegTx nfTokenRegTx = nfTokenRegTxBuilder.BuildTx();
        auto nfToken = nfTokenRegTx.GetNfToken();

        auto nftProtoIndex = NftProtocolsManager::Instance().GetNftProtoIndex(nfToken.tokenProtocolId);
        if (nftProtoIndex.IsNull())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "The NFT protocol is not registered");

        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_NF_TOKEN_REGISTER;

        FundSpecialTx(tx, nfTokenRegTx);

        CKey signerKey;
        switch (nftProtoIndex.NftProtoPtr()->nftRegSign)
        {
        case SignByCreator:
            signerKey = GetPrivKeyFromWallet(nftProtoIndex.NftProtoPtr()->tokenProtocolOwnerId);
            break;
        case SelfSign:
            signerKey = GetPrivKeyFromWallet(nfToken.tokenOwnerKeyId);
            break;
        case SignPayer:
        {
            if (GetPayerPrivKeyForNftTx(tx, signerKey))
                break;
        }
        default:
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Registrar address is missing or invalid");
        }

        SignSpecialTxPayload(tx, nfTokenRegTx, signerKey);
        SetTxPayload(tx, nfTokenRegTx);

        std::string result = SignAndSendSpecialTx(tx);
        return result;
    }

    void ListNfTokenTxsHelp()
    {
        static std::string helpMessage =
                R"(nftoken list
Lists all nftoken records on chain

Arguments:
1. nfTokenProtocol    (string, optional) The non-fungible token protocol symbol of the registered token record
                      The protocol name must be valid and registered previously. If "*" is set, it will list NFTs for all protocols.
2. nfTokenOwnerAddr   (string, optional) The token owner address, it can be used in any operations with the token.
                      The private key belonging to this address may be or may be not known in your wallet. If "*" is set, it will list NFTs for all addresses.
3. count              (numeric, optional, default=20) The number of transactions to return
4. skipFromTip        (numeric, optional, default=0) The number of transactions to skip from tip
5. height             (numeric, optional) If height is not specified, it defaults to the current chain-tip
                      To explicitly use the current tip height, set it to "*".
6. regTxOnly          (boolean, optional, default=false) false for a detailed list, true for an array of transaction IDs

Examples:
List the most recent 20 NFT records
)"
+ HelpExampleCli("nftoken", R"(list)")
+ R"(List the most recent 20 records of the "doc" NFT protocol
)"
+ HelpExampleCli("nftoken", R"(list "doc")")
+ HelpExampleCli("nftoken", R"(list "doc" "*")")
+ R"(List the most recent 20 records of the "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address
)"
+ HelpExampleCli("nftoken", R"(list "*" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc")")
+ R"(List the most recent 20 records of the "doc" NFT protocol and "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address
)"
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc")")
 + R"(List the most recent 20 records of the "doc" NFT protocol and "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address up to the most recent block
)"
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 20 0)")
+ R"(List recent 100 records skipping 50 from the end of the "doc" NFT protocol and "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address up to 5050st block
)"
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 100 50 5050)")
+ R"(List recent 100 records skipping 50 from the end of the "doc" NFT protocol and "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address up to 5050st block. List only registration tx IDs.
)"
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 100 50 5050 true)")
+ R"(As JSON-RPC calls
)"
+ HelpExampleRpc("nftoken", R"(list "*" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc")")
+ HelpExampleRpc("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 20 0)")
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 100 50 5050)");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Object BuildNftRecord(const NfTokenIndex & nftIndex)
    {
        json_spirit::Object nftJsonObj;

        nftJsonObj.push_back(json_spirit::Pair("blockHash", nftIndex.BlockIndex()->phashBlock->ToString()));
        nftJsonObj.push_back(json_spirit::Pair("registrationTxHash", nftIndex.RegTxHash().ToString()));
        nftJsonObj.push_back(json_spirit::Pair("height", nftIndex.BlockIndex()->nHeight));
        //auto blockTime = static_cast<time_t>(nftIndex.BlockIndex()->nTime);
        //std::string timeStr(asctime(gmtime(&blockTime)));
        //nftJsonObj.push_back(json_spirit::Pair("timestamp", timeStr));
        nftJsonObj.push_back(json_spirit::Pair("timestamp", nftIndex.BlockIndex()->GetBlockTime()));

        nftJsonObj.push_back(json_spirit::Pair("nftProtocolId", ProtocolName{nftIndex.NfTokenPtr()->tokenProtocolId}.ToString()));
        nftJsonObj.push_back(json_spirit::Pair("nftId", nftIndex.NfTokenPtr()->tokenId.ToString()));
        nftJsonObj.push_back(json_spirit::Pair("nftOwnerKeyId", CBitcoinAddress(nftIndex.NfTokenPtr()->tokenOwnerKeyId).ToString()));
        nftJsonObj.push_back(json_spirit::Pair("metadataAdminKeyId", CBitcoinAddress(nftIndex.NfTokenPtr()->metadataAdminKeyId).ToString()));

        std::string textMeta(nftIndex.NfTokenPtr()->metadata.begin(), nftIndex.NfTokenPtr()->metadata.end());
        nftJsonObj.push_back(json_spirit::Pair("metadata", textMeta));

        return nftJsonObj;
    }

    json_spirit::Value ListNfTokenTxs(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.empty() || params.size() > 7)
            ListNfTokenTxsHelp();

        uint64_t nftProtoId = NfToken::UNKNOWN_TOKEN_PROTOCOL;
        if (params.size() > 1)
        {
            std::string nftProtoIdStr = params[1].get_str();
            if (nftProtoIdStr != "*") {
                nftProtoId = StringToProtocolName(nftProtoIdStr.c_str());
                if (nftProtoId == NfToken::UNKNOWN_TOKEN_PROTOCOL)
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "NFT protocol ID contains invalid characters");
            }
        }

        CKeyID filterKeyId;
        if (params.size() > 2 && params[2].get_str() != "*")
        {
            filterKeyId = ParsePubKeyIDFromAddress(params[2].get_str(), "nfTokenOwnerAddr");
        }

        static unsigned const int defaultTxsCount = 20;
        static unsigned const int defaultSkipFromTip = 0;
        unsigned int count = (params.size() > 3) ? ParseUInt32V(params[3], "count") : defaultTxsCount;
        unsigned int skipFromTip = (params.size() > 4) ? ParseUInt32V(params[4], "skipFromTip") : defaultSkipFromTip;

        unsigned int height = (params.size() > 5 && params[5].get_str() != "*") ? ParseUInt32V(params[5], "height") : chainActive.Height();
        if (height < 0 || height > chainActive.Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height is out of range");

        bool regTxOnly = (params.size() > 6) ? ParseBoolV(params[6], "regTxOnly") : false;

        json_spirit::Array nftList;

        auto nftIndexHandler = [&](const NfTokenIndex & nftIndex) -> bool
        {
            if (regTxOnly)
            {
                json_spirit::Object hashObj;
                hashObj.push_back(json_spirit::Pair("registrationTxHash", nftIndex.RegTxHash().GetHex()));
                nftList.push_back(hashObj);
            }
            else
            {
                nftList.push_back(BuildNftRecord(nftIndex));
            }
            return true;
        };

        if (nftProtoId == NfToken::UNKNOWN_TOKEN_PROTOCOL && filterKeyId.IsNull())
            NfTokensManager::Instance().ProcessNftIndexRangeByHeight(nftIndexHandler, height, count, skipFromTip);
        else if (nftProtoId != NfToken::UNKNOWN_TOKEN_PROTOCOL && filterKeyId.IsNull())
            NfTokensManager::Instance().ProcessNftIndexRangeByHeight(nftIndexHandler, nftProtoId, height, count, skipFromTip);
        else if (nftProtoId == NfToken::UNKNOWN_TOKEN_PROTOCOL && !filterKeyId.IsNull())
            NfTokensManager::Instance().ProcessNftIndexRangeByHeight(nftIndexHandler, filterKeyId, height, count, skipFromTip);
        else if (nftProtoId != NfToken::UNKNOWN_TOKEN_PROTOCOL && !filterKeyId.IsNull())
            NfTokensManager::Instance().ProcessNftIndexRangeByHeight(nftIndexHandler, nftProtoId, filterKeyId, height, count, skipFromTip);

        return nftList;
    }

    void GetNfTokenHelp()
    {
        static std::string helpMessage =
                R"(nftoken get
Get an nftoken record by an NFT protocol ID and token ID
Arguments:

1. "nfTokenProtocol"  (string, required) The non-fungible token protocol symbol of the registered token record
                      The protocol name must be valid and registered previously.
2. "nfTokenId"        (string, required) The token id in hex.
                      The token id must be registered previously.

Examples:
)"
+ HelpExampleCli("nftoken", R"(get "doc" "a103d4bdfaa7d22591c4dacda81ba540e37f705bae41681c082b102e647aa8e8")")
+ HelpExampleRpc("nftoken", R"(get "doc" "a103d4bdfaa7d22591c4dacda81ba540e37f705bae41681c082b102e647aa8e8")");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value GetNfToken(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 3)
            GetNfTokenHelp();

        uint64_t tokenProtocolId = StringToProtocolName(params[1].get_str().c_str());
        if (tokenProtocolId == NfToken::UNKNOWN_TOKEN_PROTOCOL)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "NFT protocol ID contains invalid characters");
        uint256 tokenId = ParseHashV(params[2].get_str(), "nfTokenId");

        auto nftIndex = NfTokensManager::Instance().GetNfTokenIndex(tokenProtocolId, tokenId);
        if (nftIndex.IsNull())
            throw std::runtime_error("Can't find an NFT record: " + params[1].get_str() + " " + tokenId.ToString());

        return BuildNftRecord(nftIndex);
    }

    void GetNfTokenByTxIdHelp()
    {
        static std::string helpMessage =
                R"(nftoken getbytxid
Get an nftoken record by a transaction ID
Arguments:

1. "registrationTxId"   (string, required) The NFT registration transaction ID

Examples:
)"
+ HelpExampleCli("nftoken", R"(getbytxid "3840804e62350b6337ca0b4653547477aa46dab2677c0514a8dccf80b51a899a")")
+ HelpExampleRpc("nftoken", R"(getbytxid "3840804e62350b6337ca0b4653547477aa46dab2677c0514a8dccf80b51a899a")");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value GetNfTokenByTxId(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 2)
            GetNfTokenByTxIdHelp();

        uint256 regTxHash = ParseHashV(params[1].get_str(), "registrationTxHash");
        auto nftIndex = NfTokensManager::Instance().GetNfTokenIndex(regTxHash);
        if (nftIndex.IsNull())
            throw std::runtime_error("Can't find an NFT record by tx id: " + regTxHash.ToString());
        return BuildNftRecord(nftIndex);
    }

    void NfTokenTotalSupplyHelp()
    {
        static std::string helpMessage = R"(nftoken totalsupply
Get NFTs current total supply

nArguments:
1. "nfTokenProtocol"  (string, optional) The non-fungible token protocol symbol
                      The protocol name must be valid and registered previously.

Examples:
)"
+ HelpExampleCli("nftoken",   "totalsupply")
+ HelpExampleCli("nftoken", R"(totalsupply "doc")")
+ HelpExampleRpc("nftoken", R"(totalsupply "doc")");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value NfTokenTotalSupply(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.empty() || params.size() > 2)
            NfTokenTotalSupplyHelp();

        std::size_t totalSupply = 0;
        if (params.size() == 2)
        {
            uint64_t tokenProtocolId = StringToProtocolName(params[1].get_str().c_str());
            if (tokenProtocolId == NfToken::UNKNOWN_TOKEN_PROTOCOL)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "NFT protocol ID contains invalid characters");
            totalSupply = NfTokensManager::Instance().TotalSupply(tokenProtocolId);
        }
        else
        {
            totalSupply = NfTokensManager::Instance().TotalSupply();
        }

        return static_cast<uint64_t>(totalSupply);
    }

    void NfTokenBalanceOfHelp()
    {
        static std::string helpMessage = R"(nftoken balanceof
Get balance of tokens belonging to a certain address within a protocol set or in a global set

Arguments:
1. "nfTokenOwnerAddr" (string, required) The token owner address, it can be used in any operations with the token.
                      The private key belonging to this address may be or may be not known in your wallet.
2. "nfTokenProtocol"  (string, optional) The non-fungible token protocol symbol of the registered token record
                      The protocol name must be valid and registered previously.

Examples:
Display balance of "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address
)"
+ HelpExampleCli("nftoken", R"(balanceof "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc")")
+ R"(Display balance of "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address within the "doc" protocol context
)"
+ HelpExampleRpc("nftoken", R"(balanceof "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" "doc")");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value NfTokenBalanceOf(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 2 || params.size() > 3)
            NfTokenBalanceOfHelp();

        CKeyID filterKeyId = ParsePubKeyIDFromAddress(params[1].get_str(), "nfTokenOwnerAddr");
        std::size_t balance = 0;

        if (params.size() == 3)
        {
            uint64_t tokenProtocolId = StringToProtocolName(params[2].get_str().c_str());
            if (tokenProtocolId == NfToken::UNKNOWN_TOKEN_PROTOCOL)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "NFT protocol ID contains invalid characters");
            balance = NfTokensManager::Instance().BalanceOf(tokenProtocolId, filterKeyId);
        }
        else
        {
            balance = NfTokensManager::Instance().BalanceOf(filterKeyId);
        }

        return static_cast<uint64_t>(balance);
    }

    void NfTokenOwnerOfHelp()
    {
        static std::string helpMessage = R"(nftoken ownerof
Get address of the NFT owner by using its protocol symbol and token ID

1. "nfTokenProtocol"  (string, required) The non-fungible token protocol symbol of the registered token record
                      The protocol name must be valid and registered previously.
2. "nfTokenId"        (string, required) The token id in hex.
                      The token id must be registered previously.

Examples:
)"
+ HelpExampleCli("nftoken", R"(ownerof "doc" "a103d4bdfaa7d22591c4dacda81ba540e37f705bae41681c082b102e647aa8e8")")
+ HelpExampleRpc("nftoken", R"(ownerof "doc" "a103d4bdfaa7d22591c4dacda81ba540e37f705bae41681c082b102e647aa8e8")");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value NfTokenOwnerOf(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 3)
            NfTokenOwnerOfHelp();

        uint64_t tokenProtocolId = StringToProtocolName(params[1].get_str().c_str());
        if (tokenProtocolId == NfToken::UNKNOWN_TOKEN_PROTOCOL)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "NFT protocol ID contains invalid characters");
        uint256 tokenId = ParseHashV(params[2].get_str(), "nfTokenId");

        CKeyID ownerId = NfTokensManager::Instance().OwnerOf(tokenProtocolId, tokenId);
        if (ownerId.IsNull())
            throw std::runtime_error("Can't find an NFT record: " + std::to_string(tokenProtocolId) + " " + tokenId.ToString());

        return CBitcoinAddress(ownerId).ToString();
    }
}
