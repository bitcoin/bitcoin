// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "platform/specialtx.h"
#include "platform/nf-token/nf-token-reg-tx-builder.h"
#include "platform/nf-token/nf-tokens-manager.h"
#include "specialtx-rpc-utils.h"
#include "rpc-nf-token.h"

json_spirit::Value nftoken(const json_spirit::Array& params, bool fHelp)
{
    std::string command = Platform::GetCommand(params, "usage: nftoken register");

    if (command == "register")
        return Platform::RegisterNfToken(params, fHelp);

    throw std::runtime_error("Invalid command: " + command);
}

namespace Platform
{
    void RegisterNfTokenHelp()
    {
        static std::string helpMessage =
                "nftoken register \"nfTokenProtocol\" \"tokenId\" \"tokenOwnerAddr\" \"tokenMetadataAdminAddr\" \"metadata\"\n"
                "Creates and sends a new non-fungible token transaction.\n"
                "\nArguments:\n"
                "1. \"nfTokenProtocol\"           (string, required) A non-fungible token protocol to use in the token creation.\n"
                "                                 The type name must be valid and registered previously.\n"

                "2. \"nfTokenId\"                 (string, required) The token id in hex.\n"
                "                                 The token id must be unique in the token type space.\n"

                "3. \"nfTokenOwnerAddr\"          (string, required) The token owner key, can be used in any operations with the token.\n"
                "                                 The private key belonging to this address may be or may be not known in your wallet.\n"

                "4. \"nfTokenMetadataAdminAddr\"  (string, optional, default = \"0\") The metadata token administration key, can be used to modify token metadata.\n"
                "                                 The private key does not have to be known by your wallet. Can be set to 0.\n"

                "5. \"nfTokenMetadata\"           (string, optional) metadata describing the token.\n"
                "                                 It may contain text or binary in hex/base64 //TODO .\n";//TODO: add examples

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value RegisterNfToken(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 4 || params.size() > 6)
            RegisterNfTokenHelp();

        CKey ownerPrivKey;
        NfTokenRegTxBuilder nfTokenRegTxBuilder;
        nfTokenRegTxBuilder.SetTokenProtocol(params[1]).SetTokenId(params[2]).SetTokenOwnerKey(params[3], ownerPrivKey);

        if (params.size() > 4)
            nfTokenRegTxBuilder.SetMetadataAdminKey(params[4]);
        if (params.size() > 5)
            nfTokenRegTxBuilder.SetMetadata(params[5]);

        NfTokenRegTx nfTokenRegTx = nfTokenRegTxBuilder.BuildTx();

        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_NF_TOKEN_REGISTER;

        FundSpecialTx(tx, nfTokenRegTx);
        SignSpecialTxPayload(tx, nfTokenRegTx, ownerPrivKey);
        SetTxPayload(tx, nfTokenRegTx);

        std::string result = SignAndSendSpecialTx(tx);
        return result;
    }

    void ListNfTokenTxsHelp()
    {
        static std::string helpMessage =
                "nftoken list\n"
                "Lists all nftoken registration (for now) transactions on chain (or in your wallet:TODO)\n";
                "\nArguments:\n"
                //"1. \"tx-id-or-nftoken-id\" (boolean, optional)\n"
                "1. \"height\"              (numeric, optional) If height is not specified, it defaults to the current chain-tip\n"
                "2. \"verbose\"             (boolean, optional, default=false) true for a detailed list, false for an array of transaction IDs\n";
        throw std::runtime_error(helpMessage);
    }

    json_spirit::Object BuildNftRecord(const NfTokenIndex & nftIndex)
    {
        json_spirit::Object nftJsonObj;
        nftJsonObj.push_back(json_spirit::Pair("block hash", nftIndex.blockHash.GetHex()));
        nftJsonObj.push_back(json_spirit::Pair("reg tx hash", nftIndex.regTxHash.GetHex()));
        nftJsonObj.push_back(json_spirit::Pair("height", nftIndex.height));
        //nftJsonObj.push_back(json_spirit::Pair("timestamp", ));
        nftJsonObj.push_back(json_spirit::Pair("NFT protocol id", nftIndex.nfToken->tokenProtocolId));
        nftJsonObj.push_back(json_spirit::Pair("NFT id", nftIndex.nfToken->tokenId.GetHex()));
        nftJsonObj.push_back(json_spirit::Pair("NFT owner address", nftIndex.nfToken->tokenOwnerKeyId.GetHex()));
        nftJsonObj.push_back(json_spirit::Pair("Metadata amdin address", nftIndex.nfToken->metadataAdminKeyId.GetHex()));
        //nftJsonObj.push_back(json_spirit::Pair("Metadata", nftIndex.nfToken->metadata));
        return nftJsonObj;
    }

    json_spirit::Value ListNfTokenTxs(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 1 || params.size() > 3)
            ListNfTokenTxsHelp();

        int height = (params.size() > 1) ? params[1].get_int() : chainActive.Height();
        if (height < 0 || height > chainActive.Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");

        bool verbose = (params.size() > 2) ? ParseBoolV(params[2], "verbose") : false;

        json_spirit::Array nftList;

        auto tokensRange = NfTokensManager::Instance().NftIndexRangeByHeight(height);
        for (const auto & nftIndex : tokensRange)
        {
            if (verbose)
            {
                nftList.push_back(BuildNftRecord(nftIndex));
            }
            else
            {
                json_spirit::Object hashObj;
                hashObj.push_back(json_spirit::Pair("reg tx hash", nftIndex.regTxHash.GetHex()));
                nftList.push_back(hashObj);
            }
        }

        return nftList;
    }
}
