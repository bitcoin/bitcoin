// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <validation.h>
#include <boost/algorithm/string.hpp>
#include <rpc/util.h>
#include <wallet/rpcwallet.h>
#include <wallet/fees.h>
#include <policy/policy.h>
#include <services/assetconsensus.h>
#include <services/rpc/assetrpc.h>
#include <script/signingprovider.h>
#include <wallet/coincontrol.h>
#include <iomanip>
#include <rpc/server.h>
#include <wallet/wallet.h>
#include <wallet/coinselection.h>
#include <chainparams.h>
extern std::string EncodeDestination(const CTxDestination& dest);
extern CTxDestination DecodeDestination(const std::string& str);
extern UniValue ValueFromAmount(const CAmount& amount);
extern std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags = 0);
extern bool DecodeHexTx(CMutableTransaction& tx, const std::string& hex_tx, bool try_no_witness = false, bool try_witness = true);
extern RecursiveMutex cs_setethstatus;
extern CFeeRate dustRelayFee;
using namespace std;
void CreateFeeRecipient(CScript& scriptPubKey, CRecipient& recipient)
{
    CRecipient recp = { scriptPubKey, 0, false };
    recipient = recp;
}

class CCountSigsVisitor : public boost::static_visitor<void> {
private:
    const SigningProvider * const provider;
    int &nNumSigs;

public:
    CCountSigsVisitor(const SigningProvider& _provider, int &numSigs) : provider(&_provider), nNumSigs(numSigs) { }

    void Process(const CScript &script) {
        txnouttype type;
        std::vector<CTxDestination> vDest;
        int nRequired;
        if (ExtractDestinations(script, type, vDest, nRequired)) {
            for(const CTxDestination &dest: vDest)
                boost::apply_visitor(*this, dest);
        }
    }
    void operator()(const CKeyID &keyId) {
        nNumSigs++;
    }
    void operator()(const PKHash &pkhash) {
        nNumSigs++;
    }

    void operator()(const CScriptID &scriptId) {
        CScript script;
        if (provider && provider->GetCScript(scriptId, script))
            Process(script);
    }
    void operator()(const WitnessV0ScriptHash& scriptID)
    {
        CScript script;
        CRIPEMD160 hasher;
        uint160 hash;
        hasher.Write(scriptID.begin(), 32).Finalize(hash.begin());
        if (provider && provider->GetCScript(CScriptID(hash), script)) {
            Process(script);
        }
    }
    void operator()(const ScriptHash& scripthash)
    {
        CScriptID scriptID(scripthash);
        UniValue obj(UniValue::VOBJ);
        CScript subscript;
        if (provider && provider->GetCScript(scriptID, subscript)) {
            Process(subscript);
        }
    }

    void operator()(const WitnessV0KeyHash& keyid) {
        nNumSigs++;
    }

    template<typename X>
    void operator()(const X &none) {}
};
UniValue syscoinburntoassetallocation(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);
    const UniValue &params = request.params;
    RPCHelpMan{"syscoinburntoassetallocation",
        "\nBurns Syscoin to the SYSX asset\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid of SYSX"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of SYS to burn."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            }},
        RPCExamples{
            HelpExampleCli("syscoinburntoassetallocation", \"asset_guid\" \"amount\"")
            + HelpExampleRpc("syscoinburntoassetallocation", \"asset_guid\", \"amount\"")
        }
    }.Check(request);
    const uint32_t &nAsset = params[0].get_uint();          	
	CAssetAllocation theAssetAllocation;
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");
        
    vector<CRecipient> vecSend;
    UniValue amountObj = params[1];
	CAmount nAmount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
    theAssetAllocation.nAsset = nAsset;
    theAssetAllocation.voutAssets.push_back(nAmount);
    const CScript& scriptPubKey = GetScriptForDestination(DecodeDestination(strAddressFrom));
    CTxOut change_prototype_txout(0, scriptPubKey);
    CRecipient recp = {scriptPubKey, GetDustThreshold(change_prototype_txout, ::dustRelayFee), false };
    vecSend.push_back(recp);

    vector<unsigned char> data;
    theAssetAllocation.Serialize(data); 
    
    CScript scriptData;
    scriptData << OP_RETURN << data;  
    CRecipient burn;
    CreateFeeRecipient(scriptData, burn);
    burn.nAmount = nAmount;
    vecSend.push_back(burn);
    
    // Create and send the transaction
    CAmount nFeeRequired = 0;
    std::string strError;
    int nChangePosRet = -1;
    CCoinControl coin_control;
    coin_control.assetInfo = CAssetCoinInfo(nAsset, nAmount);
    coin_control.fAllowOtherInputs = true; // select asset + sys utxo's
    CAmount curBalance = pwallet->GetBalance(0, coin_control.m_avoid_address_reuse).m_mine_trusted;
    CTransactionRef tx;
    tx->tx.nVersion = SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION;
    if (!pwallet->CreateTransaction(*locked_chain, vecSend, tx, nFeeRequired, nChangePosRet, strError, coin_control)) {
        if (tx->GetValueOut() + nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    UniValue res;
    res.__pushKV("txid", tx.GetHash().GetHex());
    return res;
}
UniValue assetnew(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);
    const UniValue &params = request.params;
    RPCHelpMan{"assetnew",
    "\nCreate a new asset\n",
    {
        {"funding_amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Fund resulting UTXO owning the asset by this much SYS for gas."},
        {"symbol", RPCArg::Type::STR, RPCArg::Optional::NO, "Asset symbol (1-8 characters)"},
        {"description", RPCArg::Type::STR, RPCArg::Optional::NO, "Public description of the token."},
        {"contract", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Ethereum token contract for SyscoinX bridge. Must be in hex and not include the '0x' format tag. For example contract '0xb060ddb93707d2bc2f8bcc39451a5a28852f8d1d' should be set as 'b060ddb93707d2bc2f8bcc39451a5a28852f8d1d'. Leave empty for no smart contract bridge."},
        {"precision", RPCArg::Type::NUM, RPCArg::Optional::NO, "Precision of balances. Must be between 0 and 8. The lower it is the higher possible max_supply is available since the supply is represented as a 64 bit integer. With a precision of 8 the max supply is 10 billion."},
        {"total_supply", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Initial supply of asset. Can mint more supply up to total_supply amount or if total_supply is -1 then minting is uncapped."},
        {"max_supply", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Maximum supply of this asset. Set to -1 for uncapped. Depends on the precision value that is set, the lower the precision the higher max_supply can be."},
        {"update_flags", RPCArg::Type::NUM, RPCArg::Optional::NO, "Ability to update certain fields. Must be decimal value which is a bitmask for certain rights to update. The bitmask represents 0x01(1) to give admin status (needed to update flags), 0x10(2) for updating public data field, 0x100(4) for updating the smart contract field, 0x1000(8) for updating supply, 0x10000(16) for being able to update flags (need admin access to update flags as well). 0x11111(31) for all."},
        {"aux_fees", RPCArg::Type::OBJ, RPCArg::Optional::NO, "Auxiliary fee structure",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address to pay auxiliary fees to"},
                {"fee_struct", RPCArg::Type::ARR, RPCArg::Optional::NO, "Auxiliary fee structure",
                    {
                        {"", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Bound (in amount) for for the fee level based on total transaction amount"},
                        {"", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Percentage of total transaction amount applied as a fee"},
                    },
                }
            }
        }
    },
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            {RPCResult::Type::NUM, "asset_guid", "The unique identifier of the new asset"}
        }},
    RPCExamples{
    HelpExampleCli("assetnew", "\"myaddress\" \"CAT\" \"publicvalue\" \"contractaddr\" 8 100 1000 31 {}")
    + HelpExampleRpc("assetnew", "\"myaddress\", \"CAT\", \"publicvalue\", \"contractaddr\", 8, 100, 1000, 31, {}")
    }
    }.Check(request);
    CAmount nGas = AmountFromValue(params[0]);
    string strAddress = params[0].get_str();
    string strSymbol = params[1].get_str();
    string strPubData = params[2].get_str();
    if(strPubData == "''")
        strPubData.clear();
    string strContract = params[3].get_str();
    if(strContract == "''")
        strContract.clear();
    if(!strContract.empty())
         boost::erase_all(strContract, "0x");  // strip 0x in hex str if exist

    uint32_t precision = params[4].get_uint();
    UniValue param4 = params[5];
    UniValue param5 = params[6];
    
    CAmount nBalance;
    try{
        nBalance = AssetAmountFromValue(param4, precision);
    }
    catch(...){
        nBalance = 0;
    }
    CAmount nMaxSupply;
    try{
        nMaxSupply = AssetAmountFromValue(param5, precision);
    }
    catch(...){
        nMaxSupply = 0;
    }
    uint32_t nUpdateFlags = params[7].get_uint();

    // calculate net
    // build asset object
    CAsset newAsset;

    UniValue publicData(UniValue::VOBJ);
    publicData.pushKV("description", strPubData);
    UniValue feesStructArr = find_value(params[8].get_obj(), "fee_struct");
    if(feesStructArr.isArray() && feesStructArr.get_array().size() > 0)
        publicData.pushKV("aux_fees", params[8]);

    std::vector<std::pair<uint8_t, CAmount> > vecAmount;
    newAsset.assetAllocation.nAsset = 0;
    newAsset.assetAllocation.voutAssets.push_back(0);
    newAsset.strSymbol = strSymbol;
    newAsset.vchPubData = vchFromString(publicData.write());
    newAsset.vchContract = ParseHex(strContract);
    newAsset.nBalance = nBalance;
    newAsset.nTotalSupply = nBalance;
    newAsset.nMaxSupply = nMaxSupply;
    newAsset.nPrecision = precision;
    newAsset.nUpdateFlags = nUpdateFlags;
    vector<unsigned char> data;
    newAsset.Serialize(data);

    // use the script pub key to create the vecsend which sendmoney takes and puts it into vout
    vector<CRecipient> vecSend;

    if (!pwallet->CanGetAddresses()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: This wallet has no available keys");
    }

    // Parse the label first so we don't generate a key if there's an error
    std::string label = "";
    CTxDestination dest;
    std::string error;
    if (!pwallet->GetNewDestination(pwallet->m_default_address_type, label, dest, error)) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, error);
    }
    // new/send/update all have asset utxo's with 0 asset amount
    CAmount nAssetAmount = 0;
    CRecipient recp = { GetScriptForDestination(dest), nGas, false };
    vecSend.push_back(recp);
    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient opreturnRecipient;
    CreateFeeRecipient(scriptData, opreturnRecipient);
    if(!fUnitTest){
        // 500 SYS fee for new asset
        opreturnRecipient.nAmount += 500*COIN;
    }
    vecSend.push_back(opreturnRecipient);
    // Create and send the transaction
    CAmount nFeeRequired = 0;
    std::string strError;
    int nChangePosRet = -1;
    CCoinControl coin_control;
    CAmount curBalance = pwallet->GetBalance(0, coin_control.m_avoid_address_reuse).m_mine_trusted;
    CTransactionRef tx;
    tx->tx.nVersion = SYSCOIN_TX_VERSION_ASSET_ACTIVATE;
    if (!pwallet->CreateTransaction(*locked_chain, vecSend, tx, nFeeRequired, nChangePosRet, strError, coin_control)) {
        if (tx->GetValueOut() + nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    data.clear();
    vecSend.clear();
    // generate deterministic guid based on input txid
    newAsset.assetAllocation.nAsset = GenerateSyscoinGuid(tx->vin[0].prevout);
    newAsset.Serialize(data);
    scriptData.clear();
    scriptData << OP_RETURN << data;
    // create recipient aware that it is for an asset
    recp = { GetScriptForDestination(dest), nGas, false };
    vecSend.push_back(recp);
    CreateFeeRecipient(scriptData, opreturnRecipient);
    if(!fUnitTest){
        // 500 SYS fee for new asset
        opreturnRecipient.nAmount += 500*COIN;
    }
    vecSend.push_back(opreturnRecipient);
    CTransactionRef tx1;
    tx1->tx.nVersion = SYSCOIN_TX_VERSION_ASSET_ACTIVATE;
    nFeeRequired = 0;
    nChangePosRet = -1;
    // two rounds one to get input to determine the right asset guid and second to do the real tx
    if (!pwallet->CreateTransaction(*locked_chain, vecSend, tx1, nFeeRequired, nChangePosRet, strError, coin_control)) {
        if (!fSubtractFeeFromAmount && tx->GetValueOut() + nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    if(tx->vin[0].prevout != tx1->vin[0].prevout) {
        throw JSONRPCError(RPC_WALLET_ERROR, "New asset deterministic guid could not be established, two transactions do not match inputs!");
    }
    mapValue_t mapValue;
    pwallet->CommitTransaction(tx1, std::move(mapValue), {} /* orderForm */);
    UniValue res;
    res.__pushKV("txid", tx1.GetHash().GetHex());
    res.__pushKV("asset_guid", newAsset.assetAllocation.nAsset);
    return res;
}
UniValue CreateAssetUpdateTx(const uint32_t &nAsset, CWallet* const pwallet, std::vector<CRecipient>& vecSend, const CRecipient& opreturnRecipient, const int nVersion, const CRecpient* recpIn = null_ptr) {
    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);
    CCoinControl coin_control;
    coin_control.m_avoid_address_reuse = false;
    CAmount nMinimumAmountAsset = 0;
    CAmount nMaximumAmountAsset = 0;
    CAmount nMinimumSumAmountAsset = 0;
    coin_control.assetInfo = CAssetCoinInfo(nAsset, nMaximumAmountAsset);
    std::vector<COutput> vecOutputs;
    pwallet->AvailableCoins(*locked_chain, vecOutputs, true, &cctl, 0, MAX_MONEY, 0, nMinimumAmountAsset, nMaximumAmountAsset, nMinimumSumAmountAsset);
    int nNumOutputsFound = 0;
    COutput foundOutput;
    for(const auto &output: vecOutputs) {
        if(!output.fSpendable || !output.fSolvable)
            continue;
        nNumOutputsFound++;
        foundOutput = output;
    }
    if(nNumOutputsFound > 1) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Too many inputs found for this asset, should only have exactly one input");
    }
    if(nNumOutputsFound <= 0) {
        throw JSONRPCError(RPC_WALLET_ERROR, "No inputs found for this asset");
    }
    
    if (!pwallet->CanGetAddresses()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: This wallet has no available keys");
    }
    const CInputCoin &inputCoin = foundOutput.GetInputCoin();
    const CAmount &nGas = inputCoin.effective_value;  
    // Parse the label first so we don't generate a key if there's an error
    std::string label = "";
    CTxDestination dest;
    std::string error;
    if (!pwallet->GetNewDestination(pwallet->m_default_address_type, label, dest, error)) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, error);
    }
    // subtract fee from this output (it should pay the gas which was funded by asset new)
    CRecipient recp = recpIn? *recpIn: { GetScriptForDestination(dest), nGas, true };
    // order matters, after the calling function adds whatever outputs for assets, we need to add the
    // change back to a new output proceeding the asset outputs, other outputs can come after
    vecSend.push_back(recp);
    vecSend.push_back(opreturnRecipient);
    // Create and send the transaction
    CAmount nFeeRequired = 0;
    std::string strError;
    int nChangePosRet = -1;
    coin_control.fAllowOtherInputs = true; // select asset + sys utxo's
    coin_control.Select(inputCoin.outpoint);
    CAmount curBalance = pwallet->GetBalance(0, coin_control.m_avoid_address_reuse).m_mine_trusted;
    CTransactionRef tx;
    tx->tx.nVersion = nVersion;
    UniValue res;
    if (!pwallet->CreateTransaction(*locked_chain, vecSend, tx, nFeeRequired, nChangePosRet, strError, coin_control)) {
        if(nFeeRequired > nGas && ((tx->GetValueOut() + nFeeRequired) <= curBalance)) {
            // if gas runs out, try fund from new inputs
            recp = { GetScriptForDestination(dest), nGas, false };
            vecSend.clear();
            vecSend.push_back(recp);
            vecSend.push_back(opreturnRecipient);
            CTransactionRef tx1;
            tx1->tx.nVersion = nVersion;
            if (!pwallet->CreateTransaction(*locked_chain, vecSend, tx1, nFeeRequired, nChangePosRet, strError, coin_control)) {
                if (tx1->GetValueOut() + nFeeRequired > curBalance)
                    strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
                throw JSONRPCError(RPC_WALLET_ERROR, strError);
            } else {
                res.__pushKV("txid", tx1.GetHash().GetHex());
                return res;
            }
        }
        else if (tx->GetValueOut() + nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    } else {
        res.__pushKV("txid", tx1.GetHash().GetHex());
        return res;
    }
     return res;
}
UniValue assetupdate(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    const UniValue &params = request.params;
    RPCHelpMan{"assetupdate",
        "\nPerform an update on an asset you control.\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
            {"description", RPCArg::Type::STR, RPCArg::Optional::NO, "Public description of the token."},
            {"contract",  RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Ethereum token contract for SyscoinX bridge. Leave empty for no smart contract bridg."},
            {"supply", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "New supply of asset. Can mint more supply up to total_supply amount or if max_supply is -1 then minting is uncapped. If greater than zero, minting is assumed otherwise set to 0 to not mint any additional tokens."},
            {"update_flags", RPCArg::Type::NUM, RPCArg::Optional::NO, "Ability to update certain fields. Must be decimal value which is a bitmask for certain rights to update. The bitmask represents 0x01(1) to give admin status (needed to update flags), 0x10(2) for updating public data field, 0x100(4) for updating the smart contract field, 0x1000(8) for updating supply, 0x10000(16) for being able to update flags (need admin access to update flags as well). 0x11111(31) for all."},
            {"aux_fees", RPCArg::Type::OBJ, RPCArg::Optional::NO, "Auxiliary fee structure",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address to pay auxiliary fees to"},
                    {"fee_struct", RPCArg::Type::ARR, RPCArg::Optional::NO, "Auxiliary fee structure",
                        {
                            {"", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Bound (in amount) for for the fee level based on total transaction amount"},
                            {"", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Percentage of total transaction amount applied as a fee"},
                        },
                    }
                }
            }
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The transaction id"}
            }},
        RPCExamples{
            HelpExampleCli("assetupdate", "\"asset_guid\" \"description\" \"contract\" \"supply\" \"update_flags\" {}")
            + HelpExampleRpc("assetupdate", "\"asset_guid\", \"description\", \"contract\", \"supply\", \"update_flags\", {}")
        }
        }.Check(request);
    const uint32_t &nAsset = params[0].get_uint();
    string strData = "";
    string strCategory = "";
    string strPubData = params[1].get_str();
    if(strPubData == "''")
        strPubData.clear();
    string strContract = params[2].get_str();
    if(strContract == "''")
        strContract.clear();
    if(!strContract.empty())
        boost::erase_all(strContract, "0x");  // strip 0x if exist
    vector<unsigned char> vchContract = ParseHex(strContract);

    uint32_t nUpdateFlags = params[4].get_uint();
    
    CAsset theAsset;

    if (!GetAsset( nAsset, theAsset))
        throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");
        
    const std::string& oldData = stringFromVch(theAsset.vchPubData);
    const std::vector<unsigned char> oldContract(theAsset.vchContract);
    theAsset.ClearAsset();
    UniValue params3 = params[3];
    CAmount nBalance = 0;
    if((params3.isStr() && params3.get_str() != "0") || (params3.isNum() && params3.get_real() != 0))
        nBalance = AssetAmountFromValue(params3, theAsset.nPrecision);
    UniValue publicData(UniValue::VOBJ);
    publicData.pushKV("description", strPubData);
    UniValue feesStructArr = find_value(params[5].get_obj(), "fee_struct");
    if(feesStructArr.isArray() && feesStructArr.get_array().size() > 0)
        publicData.pushKV("aux_fees", params[5]);
    strPubData = publicData.write();
    if(strPubData != oldData)
        theAsset.vchPubData = vchFromString(strPubData);
    else
        theAsset.vchPubData.clear();
    
    if(vchContract != oldContract)
        theAsset.vchContract = vchContract;
    else
        theAsset.vchContract.clear();

    theAsset.assetAllocation.nAsset = nAsset;
    theAsset.assetAllocation.voutAssets.push_back(0);

    theAsset.nBalance = nBalance;
    theAsset.nUpdateFlags = nUpdateFlags;

    vector<unsigned char> data;
    theAsset.Serialize(data);
    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient opreturnRecipient;
    CreateFeeRecipient(scriptData, opreturnRecipient);
    std::vector<CRecipient> vecSend;
    return CreateAssetUpdateTx(nAsset, pwallet, vecSend, opreturnRecipient, SYSCOIN_TX_VERSION_ASSET_UPDATE);
}
UniValue assettransfer(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    const UniValue &params = request.params;
    RPCHelpMan{"assettransfer",
        "\nPerform a transfer of ownership on an asset you control.\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "New owner of asset."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            }},
        RPCExamples{
            HelpExampleCli("assettransfer", "\"asset_guid\" \"address\"")
            + HelpExampleRpc("assettransfer", "\"asset_guid\", \"address\"")
        }
        }.Check(request);
    const uint32_t &nAsset = params[0].get_uint();
    string strAddress = params[1].get_str();
   
    
    CAsset theAsset;

    if (!GetAsset( nAsset, theAsset))
        throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");
        
    if (!pwallet->CanGetAddresses()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: This wallet has no available keys");
    }

    // Parse the label first so we don't generate a key if there's an error
    std::string label = "";
    CTxDestination dest;
    std::string error;
    if (!pwallet->GetNewDestination(pwallet->m_default_address_type, label, dest, error)) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, error);
    }
    // new/send/update all have asset utxo's with 0 asset amount
    const CScript& scriptPubKey = GetScriptForDestination(dest);
    CTxOut change_prototype_txout(0, scriptPubKey);
    CRecipient recp = {scriptPubKey, GetDustThreshold(change_prototype_txout, ::dustRelayFee), false };

    theAsset.vchPubData.clear();
    theAsset.vchContract.clear();
    theAsset.assetAllocation.nAsset = nAsset;
    theAsset.assetAllocation.voutAssets.push_back(0);

    theAsset.nBalance = nBalance;
    theAsset.nUpdateFlags = nUpdateFlags;

    vector<unsigned char> data;
    theAsset.Serialize(data);
    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient opreturnRecipient;
    CreateFeeRecipient(scriptData, opreturnRecipient);
    std::vector<CRecipient> vecSend;
    return CreateAssetUpdateTx(nAsset, pwallet, vecSend, opreturnRecipient, SYSCOIN_TX_VERSION_ASSET_UPDATE, &recp);
}
UniValue assetsendmany(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    const UniValue &params = request.params;
    RPCHelpMan{"assetsendmany",
    "\nSend an asset you own to another address/addresses as an asset allocation. Maximum recipients is 250.\n",
    {
        {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid."},
        {"amounts", RPCArg::Type::ARR, RPCArg::Optional::NO, "Array of asset send objects.",
            {
                {"", RPCArg::Type::OBJ, RPCArg::Optional::NO, "An assetsend obj",
                    {
                        {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address to transfer to"},
                        {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Quantity of asset to send"}
                    }
                }
            },
            "[assetsendobjects,...]"
        }
    },
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
        }},
    RPCExamples{
        HelpExampleCli("assetsendmany", "\"asset_guid\" '[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\'")
        + HelpExampleCli("assetsendmany", "\"asset_guid\" \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\"")
        + HelpExampleRpc("assetsendmany", "\"asset_guid\",\'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\'")
        + HelpExampleRpc("assetsendmany", "\"asset_guid\",\"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\"")
    }
    }.Check(request);
    // gather & validate inputs
    const uint32_t &nAsset = params[0].get_uint();
    UniValue valueTo = params[1];
    if (!valueTo.isArray())
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Array of receivers not found");

    CAsset theAsset;
    if (!GetAsset(nAsset, theAsset))
        throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");


    CAssetAllocation theAssetAllocation;
    theAssetAllocation.nAsset = nAsset;
    UniValue receivers = valueTo.get_array();
    std::vector<CRecipient> vecSend;
    for (unsigned int idx = 0; idx < receivers.size(); idx++) {
        const UniValue& receiver = receivers[idx];
        if (!receiver.isObject())
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"asset_guid'\, \"address'\", \"amount\"}");

        const UniValue &receiverObj = receiver.get_obj();
        const std::string &toStr = find_value(receiverObj, "address").get_str(); 
        const CScript& scriptPubKey = GetScriptForDestination(EncodeDestination(toStr));             
        UniValue amountObj = find_value(receiverObj, "amount");
        CAmount nAmount;
        if (amountObj.isNum() || amountObj.isStr()) {
            nAmount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
            if (nAmount <= 0)
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
        }
        else
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected amount as number in asset output array");
        theAssetAllocation.voutAssets.push_back(nAmount);
        CTxOut change_prototype_txout(0, scriptPubKey);
        CRecipient recp = { scriptPubKey, GetDustThreshold(change_prototype_txout, ::dustRelayFee), false };
        vecSend.push_back(recp);
    }

    CScript scriptPubKey;
    vector<unsigned char> data;
    theAssetAllocation.Serialize(data);

    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient opreturnRecipient;
    CreateFeeRecipient(scriptData, opreturnRecipient);
    return CreateAssetUpdateTx(nAsset, pwallet, vecSend, opreturnRecipient, SYSCOIN_TX_VERSION_ASSET_SEND);
}

UniValue assetsend(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    RPCHelpMan{"assetsend",
    "\nSend an asset you own to another address.\n",
    {
        {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The asset guid."},
        {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the asset to (creates an asset allocation)."},
        {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The quantity of asset to send."}
    },
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
        }},
    RPCExamples{
        HelpExampleCli("assetsend", "\"asset_guid\" \"address\" \"amount\"")
        + HelpExampleRpc("assetsend", "\"asset_guid\", \"address\", \"amount\"")
        }

    }.Check(request);
    const uint32_t &nAsset = params[0].get_uint();
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");            
    UniValue amountValue = request.params[2];
    CAmount nAmount = AssetAmountFromValue(amountValue, theAsset.nPrecision);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for assetsend");
    UniValue output(UniValue::VARR);
    UniValue outputObj(UniValue::VOBJ);
    outputObj.__pushKV("address", params[1].get_str());
    outputObj.__pushKV("amount", ValueFromAssetAmount(nAmount, theAsset.nPrecision));
    output.push_back(outputObj);
    UniValue paramsFund(UniValue::VARR);
    paramsFund.push_back(nAsset);
    paramsFund.push_back(output);
    JSONRPCRequest requestMany;
    requestMany.params = paramsFund;
    requestMany.URI = request.URI;
    return assetsendmany(requestMany);          
}

UniValue assetallocationsendmany(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);
	const UniValue &params = request.params;
    RPCHelpMan{"assetallocationsendmany",
        "\nSend an asset allocation you own to another address. Maximum recipients is 250.\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
            {"amounts", RPCArg::Type::ARR, RPCArg::Optional::NO, "Array of assetallocationsend objects",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "The assetallocationsend object",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Address to transfer to"},
                            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "Quantity of asset to send"}
                        }
                    },
                    },
                    "[assetallocationsend object]..."
                }
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            }},
        RPCExamples{
            HelpExampleCli("assetallocationsendmany", "\"asset_guid\" \'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\'")
            + HelpExampleCli("assetallocationsendmany", "\\\"asset_guid\\\" \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\"")
            + HelpExampleRpc("assetallocationsendmany", "\"asset_guid\",  \'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\', \"\"")
            + HelpExampleRpc("assetallocationsendmany", "\\\"asset_guid\\\",  \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\"")
        }
    }.Check(request);

	// gather & validate inputs
	const uint32_t &nAsset = params[0].get_uint();
	UniValue valueTo = params[1];
	if (!valueTo.isArray())
		throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Array of receivers not found");

	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");

	UniValue receivers = valueTo.get_array();
    CAmount nTotalSending = 0;
	for (unsigned int idx = 0; idx < receivers.size(); idx++) {
		const UniValue& receiver = receivers[idx];
		if (!receiver.isObject())
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"address'\" or \"amount\"}");

		const UniValue &receiverObj = receiver.get_obj();
        const std::string &toStr = find_value(receiverObj, "address").get_str();
        const CScript& scriptPubKey = GetScriptForDestination(EncodeDestination(toStr));   
        CTxOut change_prototype_txout(0, scriptPubKey);
		UniValue amountObj = find_value(receiverObj, "amount");
		if (amountObj.isNum() || amountObj.isStr()) {
			const CAmount &nAmount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
			if (amount <= 0)
				throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
			theAssetAllocation.voutAssets.push_back(nAmount);
            CRecipient recp = { scriptPubKey, GetDustThreshold(change_prototype_txout, ::dustRelayFee), false };
            vecSend.push_back(recp);
        }
		else
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected amount as number in receiver array");

	}
    const CAmount &nAuxFee = getAuxFee(stringFromVch(theAsset.vchPubData), nTotalSending, theAsset.nPrecision, auxFeeAddress);
    if(nAuxFee > 0){
        theAssetAllocation.voutAssets.push_back(nAuxFee);
        const CScript& scriptPubKey = GetScriptForDestination(auxFeeAddress);
        CTxOut change_prototype_txout(0, scriptPubKey);
        CRecipient recp = {scriptPubKey, GetDustThreshold(change_prototype_txout, ::dustRelayFee), false };
        vecSend.push_back(recp);
    }

    CAmount nTotal = 0;
    for(const auto& amount: theAssetAllocation.voutAssets) {
        nTotal += amount;
    }
	vector<unsigned char> data;
	theAssetAllocation.Serialize(data);   


	// send the asset pay txn
	vector<CRecipient> vecSend;
	
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);	
    // Create and send the transaction
    CAmount nFeeRequired = 0;
    std::string strError;
    int nChangePosRet = -1;
    CCoinControl coin_control;
    coin_control.assetInfo = CAssetCoinInfo(nAsset, nTotal);
    CAmount curBalance = pwallet->GetBalance(0, coin_control.m_avoid_address_reuse).m_mine_trusted;
    CTransactionRef tx;
    tx->tx.nVersion = SYSCOIN_TX_VERSION_ALLOCATION_SEND;
    if (!pwallet->CreateTransaction(*locked_chain, vecSend, tx, nFeeRequired, nChangePosRet, strError, coin_control)) {
        if (tx->GetValueOut() + nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    UniValue res;
    res.__pushKV("txid", tx.GetHash().GetHex());
    return res;
}

UniValue assetallocationburn(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);
	const UniValue &params = request.params;
    RPCHelpMan{"assetallocationburn",
        "\nBurn an asset allocation in order to use the bridge or move back to Syscoin\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of asset to burn to SYSX"},
            {"ethereum_destination_address", RPCArg::Type::STR, RPCArg::Optional::NO, "The 20 byte (40 character) hex string of the ethereum destination address. Leave empty to burn to Syscoin."}
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            }},
        RPCExamples{
            HelpExampleCli("assetallocationburn", "\"asset_guid\" \"amount\" \"ethereum_destination_address\"")
            + HelpExampleRpc("assetallocationburn", "\"asset_guid\", \"amount\", \"ethereum_destination_address\"")
        }
    }.Check(request);

    const uint32_t &nAsset = params[0].get_uint();
    	
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");
        
    UniValue amountObj = params[2];
	CAmount amount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
    if (amount <= 0)
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
	string ethAddress = params[3].get_str();
    boost::erase_all(ethAddress, "0x");  // strip 0x if exist
    vector<CRecipient> vecSend;
    CScript scriptData;
    int nVersion = 0;

    CBurnSyscoin burnSyscoin;
    burnSyscoin.assetAllocations.nAsset = nAsset;
    burnSyscoin.assetAllocations.voutAssets.push_back(amount);
    // if no eth address provided just send as a std asset allocation send but to burn address
    if(ethAddress.empty() || ethAddress == "''") {
        nVersion = SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN;
    }
    else {
        burnSyscoin.vchEthAddress = ParseHex(ethAddress);
        nVersion = SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM;
    }
    burnSyscoin.Serialize(data);  
    scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee); 
	// Create and send the transaction
    CAmount nFeeRequired = 0;
    std::string strError;
    int nChangePosRet = -1;
    CCoinControl coin_control;
    coin_control.assetInfo = CAssetCoinInfo(nAsset, amount);
    coin_control.fAllowOtherInputs = true; // select asset + sys utxo's
    CAmount curBalance = pwallet->GetBalance(0, coin_control.m_avoid_address_reuse).m_mine_trusted;
    CTransactionRef tx;
    tx->tx.nVersion = nVersion;
    if (!pwallet->CreateTransaction(*locked_chain, vecSend, tx, nFeeRequired, nChangePosRet, strError, coin_control)) {
        if (tx->GetValueOut() + nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    UniValue res;
    res.__pushKV("txid", tx.GetHash().GetHex());
    return res;
}
std::vector<unsigned char> ushortToBytes(unsigned short paramShort)
{
     std::vector<unsigned char> arrayOfByte(2);
     for (int i = 0; i < 2; i++)
         arrayOfByte[1 - i] = (paramShort >> (i * 8));
     return arrayOfByte;
}
UniValue assetallocationmint(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);
    const UniValue &params = request.params;
    RPCHelpMan{"assetallocationmint",
        "\nMint assetallocation to come back from the bridge\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of asset to mint.  Note that fees (in SYS) will be taken from the owner address"},
            {"blocknumber", RPCArg::Type::NUM, RPCArg::Optional::NO, "Block number of the block that included the burn transaction on Ethereum."},
            {"tx_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Transaction hex."},
            {"txroot_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction merkle root that commits this transaction to the block header."},
            {"txmerkleproof_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The list of parent nodes of the Merkle Patricia Tree for SPV proof of transaction merkle root."},
            {"merklerootpath_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The merkle path to walk through the tree to recreate the merkle hash for both transaction and receipt root."},
            {"receipt_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Transaction Receipt Hex."},
            {"receiptroot_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction receipt merkle root that commits this receipt to the block header."},
            {"receiptmerkleproof_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The list of parent nodes of the Merkle Patricia Tree for SPV proof of transaction receipt merkle root."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            }},
        RPCExamples{
            HelpExampleCli("assetallocationmint", "\"asset_guid\" \"amount\" \"blocknumber\" \"tx_hex\" \"txroot_hex\" \"txmerkleproof_hex\" \"txmerkleproofpath_hex\" \"receipt_hex\" \"receiptroot_hex\" \"receiptmerkleproof\"")
            + HelpExampleRpc("assetallocationmint", "\"asset_guid\", \"amount\", \"blocknumber\", \"tx_hex\", \"txroot_hex\", \"txmerkleproof_hex\", \"txmerkleproofpath_hex\", \"receipt_hex\", \"receiptroot_hex\", \"receiptmerkleproof\"")
        }
    }.Check(request);
    const uint32_t &nAsset = params[0].get_uint();
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");            
    UniValue amountValue = request.params[1];
    const CAmount &nAmount = AssetAmountFromValue(amountValue, theAsset.nPrecision);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for assetallocationmint");  

    const uint32_t &nBlockNumber = params[2].get_uint(); 
    
    string vchTxValue = params[3].get_str();
    string vchTxRoot = params[4].get_str();
    string vchTxParentNodes = params[5].get_str();

    // find byte offset of tx data in the parent nodes
    size_t pos = vchTxParentNodes.find(vchTxValue);
    if(pos == std::string::npos || vchTxParentNodes.size() > (USHRT_MAX*2)){
        throw JSONRPCError(RPC_TYPE_ERROR, "Could not find tx value in tx parent nodes");  
    }
    uint16_t posTxValue = (uint16_t)pos/2;
    string vchTxPath = params[6].get_str();
 
    string vchReceiptValue = params[7].get_str();
    string vchReceiptRoot = params[8].get_str();
    string vchReceiptParentNodes = params[9].get_str();
    pos = vchReceiptParentNodes.find(vchReceiptValue);
    if(pos == std::string::npos || vchReceiptParentNodes.size() > (USHRT_MAX*2)){
        throw JSONRPCError(RPC_TYPE_ERROR, "Could not find receipt value in receipt parent nodes");  
    }
    uint16_t posReceiptValue = (uint16_t)pos/2;
    string strWitness = params[10].get_str();
    if(!fGethSynced){
        throw JSONRPCError(RPC_MISC_ERROR, "Geth is not synced, please wait until it syncs up and try again");
    }


    vector<CRecipient> vecSend;
    
    CMintSyscoin mintSyscoin;
    mintSyscoin.assetAllocation.nAsset = nAsset;
    mintSyscoin.assetAllocation.voutAssets.push_back(nAmount);
    mintSyscoin.nBlockNumber = nBlockNumber;
    mintSyscoin.vchTxValue = ushortToBytes(posTxValue);
    mintSyscoin.vchTxRoot = ParseHex(vchTxRoot);
    mintSyscoin.vchTxParentNodes = ParseHex(vchTxParentNodes);
    mintSyscoin.vchTxPath = ParseHex(vchTxPath);
    mintSyscoin.vchReceiptValue = ushortToBytes(posReceiptValue);
    mintSyscoin.vchReceiptRoot = ParseHex(vchReceiptRoot);
    mintSyscoin.vchReceiptParentNodes = ParseHex(vchReceiptParentNodes);
    
    EthereumTxRoot txRootDB;
    const bool &ethTxRootShouldExist = !fLiteMode && fLoaded && fGethSynced;
    if(!ethTxRootShouldExist){
        throw JSONRPCError(RPC_MISC_ERROR, "Network is not ready to accept your mint transaction please wait...");
    }
    {
        LOCK(cs_setethstatus);
        // validate that the block passed is committed to by the tx root he also passes in, then validate the spv proof to the tx root below  
        // the cutoff to keep txroots is 120k blocks and the cutoff to get approved is 40k blocks. If we are syncing after being offline for a while it should still validate up to 120k worth of txroots
        if(!pethereumtxrootsdb || !pethereumtxrootsdb->ReadTxRoots(mintSyscoin.nBlockNumber, txRootDB)){
            if(ethTxRootShouldExist){
                throw JSONRPCError(RPC_MISC_ERROR, "Missing transaction root for SPV proof at Ethereum block: " + itostr(mintSyscoin.nBlockNumber));
            }
        } 
    } 
    if(ethTxRootShouldExist){
        const int64_t &nTime = ::ChainActive().Tip()->GetMedianTimePast();
        // time must be between 1 week and 1 hour old to be accepted
        if(nTime < txRootDB.nTimestamp) {
            throw JSONRPCError(RPC_MISC_ERROR, "Invalid Ethereum timestamp, it cannot be earlier than the Syscoin median block timestamp. Please wait a few minutes and try again...");
        }
        else if((nTime - txRootDB.nTimestamp) > ((bGethTestnet == true)? 10800: 604800)) {
            throw JSONRPCError(RPC_MISC_ERROR, "The block height is too old, your SPV proof is invalid. SPV Proof must be done within 1 week of the burn transaction on Ethereum blockchain");
        } 
        
        // ensure that we wait at least 1 hour before we are allowed process this mint transaction  
        else if((nTime - txRootDB.nTimestamp) <  ((bGethTestnet == true)? 600: 3600)){
            throw JSONRPCError(RPC_MISC_ERROR, "Not enough confirmations on Ethereum to process this mint transaction. Must wait one hour for the transaction to settle.");
        }
        
    }
    const CScript& scriptPubKey = GetScriptForDestination(DecodeDestination(strAddress));
    CTxOut change_prototype_txout(0, scriptPubKey);
    CRecipient recp = {scriptPubKey, GetDustThreshold(change_prototype_txout, ::dustRelayFee), false };
    vecSend.push_back(recp);
    vector<unsigned char> data;
    mintSyscoin.Serialize(data);
    
    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);
  	// Create and send the transaction
    CAmount nFeeRequired = 0;
    std::string strError;
    int nChangePosRet = -1;
    CCoinControl coin_control;
    coin_control.assetInfo = CAssetCoinInfo(nAsset, nAmount);
    coin_control.fAllowOtherInputs = true; // select asset + sys utxo's
    CAmount curBalance = pwallet->GetBalance(0, coin_control.m_avoid_address_reuse).m_mine_trusted;
    CTransactionRef tx;
    tx->tx.nVersion = SYSCOIN_TX_VERSION_ALLOCATION_MINT;
    if (!pwallet->CreateTransaction(*locked_chain, vecSend, tx, nFeeRequired, nChangePosRet, strError, coin_control)) {
        if (tx->GetValueOut() + nFeeRequired > curBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    UniValue res;
    res.__pushKV("txid", tx.GetHash().GetHex());
    return res;  
}

UniValue assetallocationsend(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    RPCHelpMan{"assetallocationsend",
        "\nSend an asset allocation you own to another address.\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The asset guid"},
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the allocation to"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The quantity of asset to send"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            }},
        RPCExamples{
            HelpExampleCli("assetallocationsend", "\"asset_guid\" \"address\" \"amount\"")
            + HelpExampleRpc("assetallocationsend", "\"asset_guid\", \"address\", \"amount\"")
        }
    }.Check(request);
    const uint32_t &nAsset = params[0].get_uint();
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");            
    UniValue amountValue = request.params[2];
    CAmount nAmount = AssetAmountFromValue(amountValue, theAsset.nPrecision);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for assetallocationsend");          
    UniValue output(UniValue::VARR);
    UniValue outputObj(UniValue::VOBJ);
    outputObj.__pushKV("address", params[1].get_str());
    outputObj.__pushKV("amount", ValueFromAssetAmount(nAmount, theAsset.nPrecision));
    output.push_back(outputObj);
    UniValue paramsFund(UniValue::VARR);
    paramsFund.push_back(nAsset);
    paramsFund.push_back(output);
    JSONRPCRequest requestMany;
    requestMany.params = paramsFund;
    requestMany.URI = request.URI;
    return assetallocationsendmany(requestMany);          
}


UniValue convertaddresswallet(const JSONRPCRequest& request)	
{	
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);	
    CWallet* const pwallet = wallet.get();	
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {	
        return NullUniValue;	
    }	
    RPCHelpMan{"convertaddresswallet",
    "\nConvert between Syscoin 3 and Syscoin 4 formats. This should only be used with addressed based on compressed private keys only. P2WPKH can be shown as P2PKH in Syscoin 3. Adds to wallet as receiving address under label specified.",   
    {	
        {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The syscoin address to get the information of."},	
        {"label", RPCArg::Type::STR,RPCArg::Optional::NO, "Label Syscoin V4 address and store in receiving address. Set to \"\" to not add to receiving address", "An optional label"},	
        {"rescan", RPCArg::Type::BOOL, /* default */ "false", "Rescan the wallet for transactions. Useful if you provided label to add to receiving address"},	
    },	
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR, "v3address", "The syscoin 3 address validated"},
            {RPCResult::Type::STR, "v4address", "The syscoin 4 address validated"},
        },
    },		
    RPCExamples{	
        HelpExampleCli("convertaddresswallet", "\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\" \"bob\" true")	
        + HelpExampleRpc("convertaddresswallet", "\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\" \"bob\" true")	
    }}.Check(request);	


    UniValue ret(UniValue::VOBJ);	
    CTxDestination dest = DecodeDestination(request.params[0].get_str());	
    std::string strLabel = "";	
    if (!request.params[1].isNull())	
        strLabel = request.params[1].get_str();    	
    bool fRescan = false;	
    if (!request.params[2].isNull())	
        fRescan = request.params[2].get_bool();	
    // Make sure the destination is valid	
    if (!IsValidDestination(dest)) {	
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");	
    }	
    std::string currentV4Address = "";	
    std::string currentV3Address = "";	
    CTxDestination v4Dest;	
    if (auto witness_id = boost::get<WitnessV0KeyHash>(&dest)) {	
        v4Dest = dest;	
        currentV4Address =  EncodeDestination(v4Dest);	
        currentV3Address =  EncodeDestination(PKHash(*witness_id));	
    }	
    else if (auto key_id = boost::get<PKHash>(&dest)) {	
        v4Dest = WitnessV0KeyHash(*key_id);	
        currentV4Address =  EncodeDestination(v4Dest);	
        currentV3Address =  EncodeDestination(*key_id);	
    }	
    else if (auto script_id = boost::get<ScriptHash>(&dest)) {	
        v4Dest = *script_id;	
        currentV4Address =  EncodeDestination(v4Dest);	
        currentV3Address =  currentV4Address;	
    }	
    else if (boost::get<WitnessV0ScriptHash>(&dest)) {	
        v4Dest = dest;	
        currentV4Address =  EncodeDestination(v4Dest);	
        currentV3Address =  currentV4Address;	
    } 	
    else	
        strLabel = "";	
    isminetype mine = pwallet->IsMine(v4Dest);	
    if(!(mine & ISMINE_SPENDABLE)){	
        throw JSONRPCError(RPC_MISC_ERROR, "The V4 Public key or redeemscript not known to wallet, or the key is uncompressed.");	
    }	
    if(!strLabel.empty())	
    {	
        auto locked_chain = pwallet->chain().lock();	
        LOCK(pwallet->cs_wallet);   	
        CScript witprog = GetScriptForDestination(v4Dest);	
        LegacyScriptPubKeyMan* spk_man = pwallet->GetLegacyScriptPubKeyMan();	
        if(spk_man)	
            spk_man->AddCScript(witprog); // Implicit for single-key now, but necessary for multisig and for compatibility	
        pwallet->SetAddressBook(v4Dest, strLabel, "receive");	
        WalletRescanReserver reserver(pwallet);                   	
        if (fRescan) {	
            int64_t scanned_time = pwallet->RescanFromTime(0, reserver, true);	
            if (pwallet->IsAbortingRescan()) {	
                throw JSONRPCError(RPC_MISC_ERROR, "Rescan aborted by user.");	
            } else if (scanned_time > 0) {	
                throw JSONRPCError(RPC_WALLET_ERROR, "Rescan was unable to fully rescan the blockchain. Some transactions may be missing.");	
            }	
        }  	
    }	

    ret.pushKV("v3address", currentV3Address);	
    ret.pushKV("v4address", currentV4Address); 	
    return ret;	
}

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                                actor (function)                argNames
    //  --------------------- ------------------------          -----------------------         ----------

   /* assets using the blockchain, coins/points/service backed tokens*/
    { "syscoinwallet",            "syscoinburntoassetallocation",     &syscoinburntoassetallocation,  {"asset_guid","amount"} }, 
    { "syscoinwallet",            "convertaddresswallet",             &convertaddresswallet,          {"address","label","rescan"} },
    { "syscoinwallet",            "assetallocationburn",              &assetallocationburn,           {"asset_guid","amount","ethereum_destination_address"} }, 
    { "syscoinwallet",            "assetallocationmint",              &assetallocationmint,           {"asset_guid","amount","blocknumber","tx_hex","txroot_hex","txmerkleproof_hex","txmerkleproofpath_hex","receipt_hex","receiptroot_hex","receiptmerkleproof"} },     
    { "syscoinwallet",            "assetnew",                         &assetnew,                      {"funding_amount","symbol","description","contract","precision","total_supply","max_supply","update_flags","aux_fees"}},
    { "syscoinwallet",            "assetupdate",                      &assetupdate,                   {"asset_guid","description","contract","supply","update_flags","aux_fees"}},
    { "syscoinwallet",            "assettransfer",                    &assettransfer,                 {"asset_guid","address"}},
    { "syscoinwallet",            "assetsend",                        &assetsend,                     {"asset_guid","address","amount"}},
    { "syscoinwallet",            "assetsendmany",                    &assetsendmany,                 {"asset_guid","amounts"}},
    { "syscoinwallet",            "assetallocationsend",              &assetallocationsend,           {"asset_guid","address_receiver","amount"}},
    { "syscoinwallet",            "assetallocationsendmany",          &assetallocationsendmany,       {"asset_guid","amounts"}},
};
// clang-format on

void RegisterAssetWalletRPCCommands(interfaces::Chain& chain, std::vector<std::unique_ptr<interfaces::Handler>>& handlers)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        handlers.emplace_back(chain.handleRpc(commands[vcidx]));
}
