// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <messagesigner.h>
#include <key_io.h>
#include <rpc/util.h>
#include <wallet/wallet.h>
#include <wallet/rpc/util.h>
#include <wallet/rpc/wallet.h>
#include <wallet/scriptpubkeyman.h>
#include <util/fees.h>
#include <consensus/validation.h>
#include <validation.h>
#include <services/nevmconsensus.h>
#include <rpc/auxpow_miner.h>
#include <wallet/rpc/spend.h>
#include <rpc/server.h>
#include <wallet/coincontrol.h>
#include <nevm/sha3.h>
using namespace wallet;

static RPCHelpMan signmessagebech32()
{
    return RPCHelpMan{"signmessagebech32",
                "\nSign a message with the private key of an address (p2pkh or p2wpkh)" +
        HELP_REQUIRING_PASSPHRASE,
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The syscoin address to use for the private key."},
                    {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "Message to sign."},
                },
                RPCResult{
                    RPCResult::Type::STR, "signature", "The signature of the message encoded in base 64"
                },
                RPCExamples{
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessagebech32", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\" \"message\"") +
            "\nAs a JSON-RPC signmessagebech32\n"
            + HelpExampleRpc("signmessagebech32", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\", \"message\"")
                },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);

    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

    EnsureWalletIsUnlocked(*pwallet);

    std::string strAddress = request.params[0].get_str();
    std::string strMessage = request.params[1].get_str();

    CTxDestination dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");
    }

    auto keyid = GetKeyForDestination(spk_man, dest);
    if (keyid.IsNull()) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    }
    CKey vchSecret;
    if (!spk_man.GetKey(keyid, vchSecret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    }
    std::vector<unsigned char> vchSig;
    if(!CMessageSigner::SignMessage(strMessage, vchSig, vchSecret)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "SignMessage failed");
    }
   
    if (!CMessageSigner::VerifyMessage(vchSecret.GetPubKey(), vchSig, strMessage)) {
        LogPrintf("Sign -- VerifyMessage() failed\n");
        return false;
    }
    return EncodeBase64(vchSig);
},
    };
} 


static RPCHelpMan syscoincreaterawnevmblob()
{
    return RPCHelpMan{"syscoincreaterawnevmblob",
        "\nCreate NEVM blob data used by rollups via a custom raw parameters\n",
        {
            {"versionhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Version hash of the blob"},
            {"data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "data in hex"},
            {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
            {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, std::string() + "The fee estimate mode, must be one of (case insensitive):\n"
                        "       \"" + FeeModes("\"\n\"") + "\""},
            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB."}
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
            HelpExampleCli("syscoincreaterawnevmblob", "\"versionhash\" \"data\" 6 economical 25")
            + HelpExampleRpc("syscoincreaterawnevmblob", "\"versionhash\" \"data\" 6 economical 25")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{ 
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    EnsureWalletIsUnlocked(*pwallet);
    CNEVMData nevmData;
    std::vector<uint8_t> vchData = ParseHex(request.params[1].get_str());
    if(vchData.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Empty input, are you sure you passed in hex?");  
    }
    nevmData.vchVersionHash = ParseHex(request.params[0].get_str());
    std::vector<unsigned char> data;
    nevmData.SerializeData(data);

    CScript scriptData;
    scriptData << OP_RETURN << data;
    CCoinControl coin_control;
    coin_control.m_version = SYSCOIN_TX_VERSION_NEVM_DATA_SHA3;
    coin_control.m_nevmdata = vchData;
    CTxDestination dest;
    ExtractDestination(scriptData, dest);
    std::vector<CRecipient> recipient{CRecipient{dest, 0, false}};
    mapValue_t mapValue;
    return SendMoney(*pwallet, coin_control, recipient, mapValue, true);
},
    };
}

static RPCHelpMan syscoincreatenevmblob()
{
    return RPCHelpMan{"syscoincreatenevmblob",
        "\nCreate NEVM blob data used by rollups\n",
        {
            {"data", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "blob in hex"},
            {"overwrite_existing", RPCArg::Type::BOOL, RPCArg::Default{true}, "true to overwrite an existing blob if it exists, false to return versionhash of data on duplicate."},
            {"conf_target", RPCArg::Type::NUM, RPCArg::DefaultHint{"wallet -txconfirmtarget"}, "Confirmation target in blocks"},
            {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"unset"}, std::string() + "The fee estimate mode, must be one of (case insensitive):\n"
                        "       \"" + FeeModes("\"\n\"") + "\""},
            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::DefaultHint{"not set, fall back to wallet fee estimation"}, "Specify a fee rate in " + CURRENCY_ATOM + "/vB."}
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
            HelpExampleCli("syscoincreatenevmblob", "\"data\"")
            + HelpExampleRpc("syscoincreatenevmblob", "\"data\"")
        },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{ 
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    EnsureWalletIsUnlocked(*pwallet);
    const std::vector<uint8_t> &vchData = ParseHex(request.params[0].get_str());
    if(vchData.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Empty input, are you sure you passed in hex?");  
    }
    bool bOverwrite{false};
    if(request.params.size() > 1) {
        bOverwrite = request.params[1].get_bool();
    }
    // process new vector in batch checking the blobs
    BlockValidationState state;
    const std::vector<uint8_t> &vchVersionHash = dev::sha3(vchData).asBytes();
    int64_t mpt = -1;
    if(pnevmdatadb->ReadMTP(vchVersionHash, mpt) && !bOverwrite) {
        UniValue resObj(UniValue::VOBJ);
        resObj.pushKVEnd("versionhash", HexStr(vchVersionHash));
        return resObj;
    }

    UniValue paramsSend(UniValue::VARR);
    paramsSend.push_back(HexStr(vchVersionHash));
    paramsSend.push_back(HexStr(vchData));
    paramsSend.push_back(request.params[2]);
    paramsSend.push_back(request.params[3]);
    paramsSend.push_back(request.params[4]);
    node::JSONRPCRequest requestSend;
    requestSend.context = request.context;
    requestSend.params = paramsSend;
    requestSend.URI = request.URI;
    UniValue resObj = syscoincreaterawnevmblob().HandleRequest(requestSend);
    if(!resObj.isNull()) {
        if(!resObj.find_value("txid").isNull()) {
            UniValue resRet(UniValue::VOBJ);
            resObj.pushKVEnd("versionhash", HexStr(vchVersionHash));
            resObj.pushKVEnd("datasize", vchData.size());
            return resObj;
        } else {
            throw JSONRPCError(RPC_DATABASE_ERROR, "Transaction not complete or could not find txid");   
        }  
    }
    throw JSONRPCError(RPC_DATABASE_ERROR, "Transaction not complete or invalid");
},
    };
}

namespace
{

/**
 * Helper class that keeps track of reserved keys that are used for mining
 * coinbases.  We also keep track of the block hash(es) that have been
 * constructed based on the key, so that we can mark it as keep and get a
 * fresh one when one of those blocks is submitted.
 */
class ReservedKeysForMining
{

private:

  /**
   * The per-wallet data that we store.
   */
  struct PerWallet
  {

    /**
     * The current coinbase script.  This has been taken out of the wallet
     * already (and marked as "keep"), but is reused until a block actually
     * using it is submitted successfully.
     */
    CScript coinbaseScript;

    /** All block hashes (in hex) that are based on the current script.  */
    std::set<std::string> blockHashes;

    explicit PerWallet (const CScript& scr)
      : coinbaseScript(scr)
    {}

    PerWallet (PerWallet&&) = default;

  };

  /**
   * Data for each wallet that we have.  This is keyed by CWallet::GetName,
   * which is not perfect; but it will likely work in most cases, and even
   * when two different wallets are loaded with the same name (after each
   * other), the worst that can happen is that we mine to an address from
   * the other wallet.
   */
  std::map<std::string, PerWallet> data;


public:
  /** Lock for this instance.  */
  mutable RecursiveMutex cs;
  ReservedKeysForMining () = default;

  /**
   * Retrieves the key to use for mining at the moment.
   */
  CScript
  GetCoinbaseScript (CWallet* pwallet) EXCLUSIVE_LOCKS_REQUIRED(cs)
  {
    LOCK (pwallet->cs_wallet);

    const auto mit = data.find (pwallet->GetName ());
    if (mit != data.end ())
      return mit->second.coinbaseScript;

    ReserveDestination rdest(pwallet, pwallet->m_default_address_type);
    CTxDestination dest;
    auto op_dest = rdest.GetReservedDestination(false);
    rdest.KeepDestination ();
    if (!op_dest) {
         throw JSONRPCError (RPC_WALLET_KEYPOOL_RAN_OUT,
                          "Error: Keypool ran out,"
                          " please call keypoolrefill first");
    } else {
        dest = *op_dest;
        CScript res = GetScriptForDestination (dest);
        data.emplace (pwallet->GetName (), PerWallet (res));
        return res;
    }
  }

  /**
   * Adds the block hash (given as hex string) of a newly constructed block
   * to the set of blocks for the current key.
   */
  void
  AddBlockHash (const CWallet* pwallet, const std::string& hashHex)  EXCLUSIVE_LOCKS_REQUIRED(cs)
  {
    const auto mit = data.find (pwallet->GetName ());
    assert (mit != data.end ());
    mit->second.blockHashes.insert (hashHex);
  }

  /**
   * Marks a block as submitted, releasing the key for it (if any).
   */
  void
  MarkBlockSubmitted (const CWallet* pwallet, const std::string& hashHex)  EXCLUSIVE_LOCKS_REQUIRED(cs)
  {
    const auto mit = data.find (pwallet->GetName ());
    if (mit == data.end ())
      return;

    if (mit->second.blockHashes.count (hashHex) > 0)
      data.erase (mit);
  }

};

ReservedKeysForMining g_mining_keys;

} // anonymous namespace

static RPCHelpMan getauxblock()
{
    return RPCHelpMan{"getauxblock",
                "\nCreates or submits a merge-mined block.\n"
                "\nWithout arguments, creates a new block and returns information\n"
                "required to merge-mine it.  With arguments, submits a solved\n"
                "auxpow for a previously returned block.\n",
                {
                    {"hash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "Hash of the block to submit"},
                    {"auxpow", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "Serialised auxpow found"},
                },
                {
                    RPCResult{"without arguments",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::STR_HEX, "hash", "hash of the created block"},
                            {RPCResult::Type::NUM, "chainid", "chain ID for this block"},
                            {RPCResult::Type::STR_HEX, "previousblockhash", "hash of the previous block"},
                            {RPCResult::Type::NUM, "coinbasevalue", "value of the block's coinbase"},
                            {RPCResult::Type::STR, "bits", "compressed target of the block"},
                            {RPCResult::Type::NUM, "height", "height of the block"},
                            {RPCResult::Type::STR_HEX, "_target", "target in reversed byte order, deprecated"},
                        },
                    },
                    RPCResult{"with arguments",
                        RPCResult::Type::BOOL, "", "whether the submitted block was correct"
                    },
                },
                RPCExamples{
                    HelpExampleCli("getauxblock", "")
                    + HelpExampleCli("getauxblock", "\"hash\" \"serialised auxpow\"")
                    + HelpExampleRpc("getauxblock", "")
                },
    [&](const RPCHelpMan& self, const node::JSONRPCRequest& request) -> UniValue
{
    /* RPCHelpMan::Check is not applicable here since we have the
       custom check for exactly zero or two arguments.  */
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    CWallet* const pwallet = wallet.get();
    if (pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Private keys are disabled for this wallet");
    }
    LOCK(g_mining_keys.cs);
    /* Create a new block */
    if (request.params.size() == 0)
    {
        const CScript coinbaseScript = g_mining_keys.GetCoinbaseScript(pwallet);
        UniValue res = AuxpowMiner::get().createAuxBlock(request, coinbaseScript);
        g_mining_keys.AddBlockHash(pwallet, res["hash"].get_str ());
        return res;
    }

    /* Submit a block instead.  */
    assert(request.params.size() == 2);
    const std::string& hash = request.params[0].get_str();

    const bool fAccepted
        = AuxpowMiner::get().submitAuxBlock(request, hash, request.params[1].get_str());
    if (fAccepted)
        g_mining_keys.MarkBlockSubmitted(pwallet, hash);

    return fAccepted;
},
    };
}

Span<const CRPCCommand> wallet::GetNEVMWalletRPCCommands()
{
    static const CRPCCommand commands[]{ 
        {"syscoinwallet", &signmessagebech32},
        {"syscoinwallet", &syscoincreatenevmblob},
        {"syscoinwallet", &syscoincreaterawnevmblob},
        /** Auxpow wallet functions */
        {"syscoinwallet", &getauxblock},
    };
// clang-format on
    return commands;
}
