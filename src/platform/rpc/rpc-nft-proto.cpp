// Copyright (c) 2014-2019 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <platform/nf-token/nf-token-protocol.h>
#include "platform/specialtx.h"
#include "platform/platform-utils.h"
#include "specialtx-rpc-utils.h"
#include "rpc-nft-proto.h"
#include "rpcserver.h"

json_spirit::Value nftproto(const json_spirit::Array& params, bool fHelp)
{
    std::string command = Platform::GetCommand(params, "usage: nftproto register|list|get");

    if (command == "register")
        return Platform::RegisterNftProtocol(params, fHelp);
    else if (command == "list")
        return Platform::ListNftProtocols(params, fHelp);
    else if (command == "get")
        return Platform::GetNftProtocol(params, fHelp);

    throw std::runtime_error("Invalid command: " + command);
}

namespace Platform
{
    json_spirit::Value RegisterNftProtocol(const json_spirit::Array& params, bool fHelp)
    {
        if (params.size() < 4 || params.size() > 10)
            RegisterNftProtocolHelp();
    }

    void RegisterNftProtocolHelp()
    {
        static std::string helpMessage =
                R"(nftproto register
Register a new NFT protocol

Arguments:
1. "tokenProtocolId"         (string, required) A non-fungible token protocol ID/symbol
                             NF token protocol unique symbol/identifier, can be an a abbreviated name describing this NF token type.
                             Represented as a base32 string, can only contain characters: .abcdefghijklmnopqrstuvwxyz12345
                             Minimum length 3 symbols, maximum length 12 symbols
2. "tokenProtocolName"       (string, required) Full readable name for this NF token type/protocol.
                             Minimum length 3 symbols, maximum length 24 symbols
3. "tokenProtocolOwnerAddr"  (string, required) The NFT protocol owner address, it can be used to generate new NFTs or update the protocol settings
                             The private key belonging to this address may be or may be not known in your wallet.
4. "nftRegSign"              (numeric, optional) Defines who must sign an NFT registration transaction.
                             1 (SelfSign): every transaction should be signed by the NFT owner
                             2 (SignByCreator): every transaction should be signed by the owner/creator of the NFT protocol
                             3 (SignAny): every transaction can be signed any key
                             Default: SignByCreator
5. "tokenMetadataMimeType"   (string, optional) MIME type describing metadata content type
                             Examples: text/json, text/plain, application/x-binary etc. Default: text/plain
6. "tokenMetadataSchemaUri"  (string, optional) URI to schema (json/xml/binary) describing metadata format.
                             Default: "". Arbitrary data can be written by default.

7. "isTokenTransferable"     (string, optional) Defines if this NF token type can be transferred. Default: true
8. "isTokenImmutable"        (string, optional) Defines if this NF token id can be changed during token lifetime. Default: true
9. "isMetadataEmbedded"      (string, optional) Defines if metadata is embedded or contains a URI
                             It's recommended to use embedded metadata only if it's shorter than the URI. Default: false
Examples:
)"
+ HelpExampleCli("nftproto", R"(register "doc" "Doc Proof" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 3 "text/plain" "" true true false)")
+ HelpExampleCli("nftproto", R"(register "crd" "Crown ID" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 1 "application/x-binary" "https://binary-schema" false true false)")
+ HelpExampleRpc("nftproto", R"(register "doc" "Doc Proof" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 3 "text/plain" "" true true false)")
+ HelpExampleRpc("nftproto", R"(register "crd" "Crown ID" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 1 "application/x-binary" "https://binary-schema" false true false)")
+ HelpExampleRpc("nftproto", R"(register "cks" "ERC721 CryptoKitties" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 2 "text/plain" "" true true false)");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value ListNftProtocols(const json_spirit::Array& params, bool fHelp)
    {

    }

    void ListNftProtocolsHelp()
    {

    }

    json_spirit::Value GetNftProtocol(const json_spirit::Array& params, bool fHelp)
    {

    }

    void GetNftProtocolHelp()
    {

    }
}