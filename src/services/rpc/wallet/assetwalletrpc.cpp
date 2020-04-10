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
#include <chainparams.h>
extern std::string EncodeDestination(const CTxDestination& dest);
extern CTxDestination DecodeDestination(const std::string& str);
extern UniValue ValueFromAmount(const CAmount& amount);
extern std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags = 0);
extern bool DecodeHexTx(CMutableTransaction& tx, const std::string& hex_tx, bool try_no_witness = false, bool try_witness = true);
extern RecursiveMutex cs_setethstatus;
using namespace std;
std::vector<CTxIn> savedtxins;
UniValue syscointxfund(CWallet* const pwallet, const JSONRPCRequest& request);
void CreateFeeRecipient(CScript& scriptPubKey, CRecipient& recipient)
{
    CRecipient recp = { scriptPubKey, 0, false };
    recipient = recp;
}
UniValue syscointxfund_helper(CWallet* const pwallet, const string& strAddress, const int &nVersion, const vector<CRecipient> &vecSend) {
    CMutableTransaction txNew;
    if(nVersion > 0)
        txNew.nVersion = nVersion;
        
    // vouts to the payees
    for (const auto& recipient : vecSend)
    {
        CTxOut txout(recipient.nAmount, recipient.scriptPubKey);
        txNew.vout.emplace_back(txout);
    }
    UniValue paramsFund(UniValue::VARR);
    paramsFund.push_back(EncodeHexTx(CTransaction(txNew)));
    paramsFund.push_back(strAddress);
    
    JSONRPCRequest request;
    request.params = paramsFund;
    return syscointxfund(pwallet, request);
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
UniValue syscointxfund(CWallet* const pwallet, const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    RPCHelpMan{"syscointxfund",
        "\nFunds a new syscoin transaction with inputs used from wallet or an array of addresses specified. Note that any inputs to the transaction added prior to calling this will not be accounted and new outputs will be added every time you call this function.\n",
        {
            {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The raw syscoin transaction output given from rpc"},
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address funding this transaction."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
            }},
        RPCExamples{
            HelpExampleCli("syscointxfund", "<hexstring> \"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\"")
            + HelpExampleRpc("syscointxfund", "<hexstring>, \"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\"")
            + HelpRequiringPassphrase(pwallet)
        }
    }.Check(request);
    const string &hexstring = params[0].get_str();
    const string &strAddress = params[1].get_str();
    CMutableTransaction txIn, tx;
    // decode as non-witness
    if (!DecodeHexTx(txIn, hexstring, true, false))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Could not send raw transaction: Cannot decode transaction from hex string: " + hexstring);
    CTransaction txIn_t(txIn);
    UniValue addressArray(UniValue::VARR);  
 
    CScript scriptPubKeyFromOrig = GetScriptForDestination(DecodeDestination(strAddress));
    addressArray.push_back("addr(" + strAddress + ")"); 
    
    
   // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(cs_main);
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = ::ChainstateActive().CoinsTip();
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : txIn.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }
        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }
    FeeCalculation fee_calc;
    CCoinControl coin_control;
    tx = txIn;
    const bool& isZdagTx = IsZdagTx(tx.nVersion);
    tx.vin.clear();
  
    // # vin (with IX)*FEE + # vout*FEE + (10 + # vin)*FEE + 34*FEE (for change output)
    CAmount nFees =  GetMinimumFee(*pwallet, 10+34, coin_control,  &fee_calc);
    for (auto& vin : txIn.vin) {
        const Coin& coin = view.AccessCoin(vin.prevout);
        if(coin.IsSpent()){
            continue;
        }
        {
            LOCK(pwallet->cs_wallet);
            if (pwallet->IsLockedCoin(vin.prevout.hash, vin.prevout.n) && vin.prevout != outPointLastSender){
                continue;
            }
        }
        if(fTPSTest && fTPSTestEnabled){
            if(std::find(savedtxins.begin(), savedtxins.end(), vin) == savedtxins.end())
                savedtxins.emplace_back(vin);
            else{
                LogPrint(BCLog::SYS, "Skipping saved output in syscointxfund...\n");
                continue;
            }
        }
        // disable RBF for syscoin zdag tx's, should use CPFP
        if(isZdagTx)
            vin.nSequence = CTxIn::SEQUENCE_FINAL - 1;
        tx.vin.emplace_back(vin);
        int numSigs = 0;
        std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(coin.out.scriptPubKey);
        CCountSigsVisitor(*provider, numSigs).Process(coin.out.scriptPubKey);
        if(isZdagTx)
            numSigs *= 2;
        nFees += GetMinimumFee(*pwallet, numSigs * 200, coin_control, &fee_calc);
    }
    txIn_t = CTransaction(txIn);
   
    for (auto& vout : txIn_t.vout) {
        unsigned int nBytes = ::GetSerializeSize(vout, PROTOCOL_VERSION);
        // double the fee for bandwidth costs of double spend relay
        if(isZdagTx)
            nBytes *= 2;
        nFees += GetMinimumFee(*pwallet, nBytes, coin_control, &fee_calc);
    } 
    CAssetAllocation allocation;
    if(IsSyscoinTx(tx.nVersion)) {
        allocation = CAssetAllocation(tx);
        if(allocation.IsNull()) {
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "No funds found in addresses provided");
        }
    }

    UniValue paramsBalance(UniValue::VARR);
    paramsBalance.push_back("start");
    paramsBalance.push_back(addressArray);
    JSONRPCRequest request1;
    request1.params = paramsBalance;

    UniValue resUTXOs = scantxoutset(request1);
    UniValue utxoArray(UniValue::VARR);
    if (resUTXOs.isObject()) {
        const UniValue& resUtxoUnspents = find_value(resUTXOs.get_obj(), "unspents");
        if (!resUtxoUnspents.isArray())
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "No unspent outputs found in addresses provided");
        utxoArray = resUtxoUnspents.get_array();
    }
    else
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "No funds found in addresses provided");
    const uint32_t &nAsset = allocation.nAsset;
    for (const auto &voutAsset: allocation.voutAssets) {
        nDesiredAmount = voutAsset.nValue;
        nCurrentAmount = 0;
        for (unsigned int i = 0; i < utxoArray.size(); i++)
        {
            const UniValue& utxoObj = utxoArray[i].get_obj();
            const string &strTxid = find_value(utxoObj, "txid").get_str();
            const uint256& txid = uint256S(strTxid);
            const uint32_t& nOut = find_value(utxoObj, "vout").get_uint();
            const std::vector<unsigned char> &data(ParseHex(find_value(utxoObj, "scriptPubKey").get_str()));
            const CScript& scriptPubKey = CScript(data.begin(), data.end());
            const CUniValue &assetGuidVal = find_value(utxoObj, "asset_guid");
            const CUniValue &assetAmountVal = find_value(utxoObj, "asset_amount");
            CAmount nValue;
            if(!assetAmountVal.IsNull() && !assetGuidVal.IsNull()) {
                if (assetGuidVal.get_uint() != nAsset)
                    continue;
                nValue = AmountFromValue(assetAmountVal);
            }
            else
                continue;
            CTxIn txIn(txid, nOut, scriptPubKey);
            if (std::find(tx.vin.begin(), tx.vin.end(), txIn) != tx.vin.end())
                continue;
            {
                LOCK(pwallet->cs_wallet);
                if (pwallet->IsLockedCoin(txid, nOut))
                    continue;
            }
            // hack for double spend zdag4 test so we can spend multiple inputs of an address within a block and get different inputs every time we call this function
            if(fTPSTest && fTPSTestEnabled){
                if(std::find(savedtxins.begin(), savedtxins.end(), txIn) == savedtxins.end())
                    savedtxins.emplace_back(txIn);
                else{
                    LogPrint(BCLog::SYS, "Skipping saved output in syscointxfund...\n");
                    continue;
                }
            }
            int numSigs = 0;
            std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(scriptPubKey);
            CCountSigsVisitor(*provider, numSigs).Process(scriptPubKey);
            if(isZdagTx){
                // double relay fee for zdag tx to account for dbl bandwidth on dbl spend relays
                numSigs *= 2;
                // disable RBF for syscoin tx's, should use CPFP
                txIn.nSequence = CTxIn::SEQUENCE_FINAL - 1;
            }
            // add fees to account for every input added to this transaction
            nFees += GetMinimumFee(*pwallet, numSigs * 200, coin_control, &fee_calc);
        
            tx.vin.emplace_back(txIn);
            nCurrentAmount += nValue;
            if (nCurrentAmount >= nDesiredAmount) {
                break;
            }
        }
    }
    
    // for gas
    nCurrentAmount = view.GetValueIn(tx);   
    // add total output amount of transaction to desired amount
    nDesiredAmount = txIn_t.GetValueOut();
    // convert to sys transactions should start with 0 because the output is minted based on allocation burn
    if(txIn_t.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN) 
        nDesiredAmount = 0;
    if (nCurrentAmount < (nDesiredAmount + nFees)) {
        for (unsigned int i = 0; i < utxoArray.size(); i++)
        {
            const UniValue& utxoObj = utxoArray[i].get_obj();
            const string &strTxid = find_value(utxoObj, "txid").get_str();
            const uint256& txid = uint256S(strTxid);
            const uint32_t& nOut = find_value(utxoObj, "vout").get_uint();
            const std::vector<unsigned char> &data(ParseHex(find_value(utxoObj, "scriptPubKey").get_str()));
            const CScript& scriptPubKey = CScript(data.begin(), data.end());
            const CAmount &nValue = AmountFromValue(find_value(utxoObj, "amount"));
            CTxIn txIn(txid, nOut, scriptPubKey);
            if (std::find(tx.vin.begin(), tx.vin.end(), txIn) != tx.vin.end())
                continue;
            const CUniValue &assetGuidVal = find_value(utxoObj, "asset_guid");
            const CUniValue &assetAmountVal = find_value(utxoObj, "asset_amount");
            // skip asset inputs when funding gas
            if(!assetAmountVal.IsNull() && !assetGuidVal.IsNull()) {
                continue;
            }               
            {
                LOCK(pwallet->cs_wallet);
                if (pwallet->IsLockedCoin(txid, nOut))
                    continue;
            }
            // hack for double spend zdag4 test so we can spend multiple inputs of an address within a block and get different inputs every time we call this function
            if(fTPSTest && fTPSTestEnabled){
                if(std::find(savedtxins.begin(), savedtxins.end(), txIn) == savedtxins.end())
                    savedtxins.emplace_back(txIn);
                else{
                    LogPrint(BCLog::SYS, "Skipping saved output in syscointxfund...\n");
                    continue;
                }
            }
            int numSigs = 0;
            std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(scriptPubKey);
            CCountSigsVisitor(*provider, numSigs).Process(scriptPubKey);
            if(isZdagTx){
                // double relay fee for zdag tx to account for dbl bandwidth on dbl spend relays
                numSigs *= 2;
                // disable RBF for syscoin tx's, should use CPFP
                txIn.nSequence = CTxIn::SEQUENCE_FINAL - 1;
            }
            // add fees to account for every input added to this transaction
            nFees += GetMinimumFee(*pwallet, numSigs * 200, coin_control, &fee_calc);
        
            tx.vin.emplace_back(txIn);
            nCurrentAmount += nValue;
            if (nCurrentAmount >= (nDesiredAmount + nFees)) {
                break;
            }
        }
    }

  
    const CAmount &nChange = nCurrentAmount - nDesiredAmount - nFees;
    if (nChange < 0)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds, short by: " + ValueFromAmount(-1*nChange).write() + " SYS");
        
    // change back to funding address
    const CTxDestination & dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest))
        throw runtime_error("Change address is not valid");
    CTxOut changeOut(nChange, GetScriptForDestination(dest));
    if (IsDust(changeOut, pwallet->chain().relayDustFee()))
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds. Change is set to dust (too low): " + ValueFromAmount(changeOut).write() + " SYS");
    tx.vout.emplace_back(changeOut);
    
    // pass back new raw transaction
    UniValue res(UniValue::VOBJ);
    res.__pushKV("hex", EncodeHexTx(CTransaction(tx)));
    return res;
}
UniValue syscoinburntoassetallocation(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    const UniValue &params = request.params;
    RPCHelpMan{"syscoinburntoassetallocation",
        "\nBurns Syscoin to the SYSX asset\n",
        {
            {"funding_address", RPCArg::Type::STR, RPCArg::Optional::NO, "Funding address to burn SYS from"},
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid of SYSX"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of SYS to burn."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
            }},
        RPCExamples{
            HelpExampleCli("syscoinburntoassetallocation", "\"funding_address\" \"asset_guid\" \"amount\"")
            + HelpExampleRpc("syscoinburntoassetallocation", "\"funding_address\", \"asset_guid\", \"amount\"")
        }
    }.Check(request);
    std::string strAddressFrom = params[0].get_str();
    const uint32_t &nAsset = params[1].get_uint();          	
	CAssetAllocation theAssetAllocation;
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");
        
    UniValue amountObj = params[2];
	CAmount nAmount = AssetAmountFromValue(amountObj, theAsset.nPrecision);

    theAssetAllocation.SetNull();
    // from burn to allocation
    theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(witnessAddress.nVersion, witnessAddress.vchWitnessProgram), nAmount));
    vector<unsigned char> data;
    theAssetAllocation.Serialize(data); 


    vector<CRecipient> vecSend;
    CScript scriptData;
    scriptData << OP_RETURN << data;  
    CRecipient burn;
    CreateFeeRecipient(scriptData, burn);
    burn.nAmount = nAmount;
    vecSend.push_back(burn);
    UniValue res = syscointxfund_helper(pwallet, strAddressFrom, SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION, "", vecSend);
    return res;
}
UniValue assetnew(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    const UniValue &params = request.params;
    RPCHelpMan{"assetnew",
    "\nCreate a new asset\n",
    {
        {"address", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "An address that you own."},
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
            {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
            {RPCResult::Type::NUM, "asset_guid", "The guid of asset to be created"},
        }},
    RPCExamples{
    HelpExampleCli("assetnew", "\"myaddress\" \"CAT\" \"publicvalue\" \"contractaddr\" 8 100 1000 31 {}")
    + HelpExampleRpc("assetnew", "\"myaddress\", \"CAT\", \"publicvalue\", \"contractaddr\", 8, 100, 1000, 31, {}")
    }
    }.Check(request);
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
    string vchWitness;
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
    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    if(!fUnitTest){
        // 500 SYS fee for new asset
        fee.nAmount += 500*COIN;
    }
    vecSend.push_back(fee);
    std::vector<CTxIn> savedtxinsCopy;
    // save copy of txins for tps test
    if(fTPSTest && fTPSTestEnabled)
        savedtxinsCopy = savedtxins;
    // two rounds one to get input to determine the right asset guid and second to do the real tx
    UniValue res = syscointxfund_helper(pwallet, strAddress, SYSCOIN_TX_VERSION_ASSET_ACTIVATE, vecSend);
    // replace copy because savedtxins is mutated
    if(fTPSTest && fTPSTestEnabled)
        savedtxins =  savedtxinsCopy;
    const UniValue& resHex = find_value(res.get_obj(), "hex");
    const std::string& strHex = resHex.get_str();
    CMutableTransaction txIn;
    // decode as non-witness
    if (!DecodeHexTx(txIn, strHex, true, false))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Could not send raw transaction: Cannot decode transaction from hex string: " + strHex);
    data.clear();
    vecSend.clear();
    // generate deterministic guid based on input txid
    newAsset.assetAllocation.nAsset = GenerateSyscoinGuid(txIn.vin[0].prevout);
    newAsset.Serialize(data);
    scriptData.clear();
    scriptData << OP_RETURN << data;
    CreateFeeRecipient(scriptData, fee);
    if(!fUnitTest){
        // 500 SYS fee for new asset
        fee.nAmount += 500*COIN;
    }
    vecSend.push_back(fee);
    res = syscointxfund_helper(pwallet, strAddress, SYSCOIN_TX_VERSION_ASSET_ACTIVATE, vchWitness, vecSend);
    res.__pushKV("asset_guid", newAsset.assetAllocation.nAsset);
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
            },
            {"witness", RPCArg::Type::STR, RPCArg::Optional::NO, "Witness address that will sign for web-of-trust notarization of this transaction."}
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
            }},
        RPCExamples{
            HelpExampleCli("assetupdate", "\"assetguid\" \"publicvalue\" \"contractaddress\" \"supply\" \"update_flags\" {} \"\"")
            + HelpExampleRpc("assetupdate", "\"assetguid\", \"publicvalue\", \"contractaddress\", \"supply\", \"update_flags\", {}, \"\"")
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
    string vchWitness;
    vchWitness = params[6].get_str();
    
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
    

    vector<CRecipient> vecSend;


    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);
    return syscointxfund_helper(pwallet, theAsset.witnessAddress.ToString(), SYSCOIN_TX_VERSION_ASSET_UPDATE, vchWitness, vecSend);
}

UniValue assetsendmany(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    const UniValue &params = request.params;
    RPCHelpMan{"assetsendmany",
    "\nSend an asset you own to another address/addresses as an asset allocation. Maximum recipients is 250.\n",
    {
        {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid."},
        {"array", RPCArg::Type::ARR, RPCArg::Optional::NO, "Array of asset send objects.",
            {
                {"", RPCArg::Type::OBJ, RPCArg::Optional::NO, "An assetsend obj",
                    {
                        {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address to transfer to"},
                        {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Quantity of asset to send"}
                    }
                }
            },
            "[assetsendobjects,...]"
        },
    },
    RPCResult{
        RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
        }},
    RPCExamples{
        HelpExampleCli("assetsendmany", "\"assetguid\" '[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\' \"\"")
        + HelpExampleCli("assetsendmany", "\"assetguid\" \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\" \"\"")
        + HelpExampleRpc("assetsendmany", "\"assetguid\",\'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\' \"\"")
        + HelpExampleRpc("assetsendmany", "\"assetguid\",\"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\" \"\"")
    }
    }.Check(request);
    // gather & validate inputs
    const uint32_t &nAsset = params[0].get_uint();
    UniValue valueTo = params[1];
    string vchWitness = params[2].get_str();
    if (!valueTo.isArray())
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Array of receivers not found");

    CAsset theAsset;
    if (!GetAsset(nAsset, theAsset))
        throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");


    vector<CRecipient> vecSend;
    CAssetAllocation theAssetAllocation;
    theAssetAllocation.nAsset = nAsset;
    UniValue receivers = valueTo.get_array();
    for (unsigned int idx = 0; idx < receivers.size(); idx++) {
        const UniValue& receiver = receivers[idx];
        if (!receiver.isObject())
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"asset_guid'\, \"address'\", \"amount\"}");

        const UniValue &receiverObj = receiver.get_obj();
        const uint32_t &nAsset = find_value(receiverObj, "asset_guid").get_uint();
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
        CRecipient recp = { scriptPubKey, nAmount, false };
        vecSend.push_back(recp);
    }

    CScript scriptPubKey;
    vector<unsigned char> data;
    theAssetAllocation.Serialize(data);

    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);
    return syscointxfund_helper(pwallet, theAsset.witnessAddress.ToString(), SYSCOIN_TX_VERSION_ASSET_SEND, vchWitness, vecSend);
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
            {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
        }},
    RPCExamples{
        HelpExampleCli("assetsend", "\"assetguid\" \"address\" \"amount\"")
        + HelpExampleRpc("assetsend", "\"assetguid\", \"address\", \"amount\"")
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
    paramsFund.push_back("");
    JSONRPCRequest requestMany;
    requestMany.params = paramsFund;
    requestMany.URI = request.URI;
    return assetsendmany(requestMany);          
}

UniValue assetallocationsendmany(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
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
                },
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
            }},
        RPCExamples{
            HelpExampleCli("assetallocationsendmany", "\"assetguid\" \'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\'")
            + HelpExampleCli("assetallocationsendmany", "\\\"assetguid\\\" \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\"")
            + HelpExampleRpc("assetallocationsendmany", "\"assetguid\",  \'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\', \"\"")
            + HelpExampleRpc("assetallocationsendmany", "\\\"assetguid\\\",  \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\"")
        }
    }.Check(request);

	// gather & validate inputs
	const uint32_t &nAsset = params[0].get_uint();
	UniValue valueTo = params[1];
	vector<unsigned char> vchWitness;
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
		UniValue amountObj = find_value(receiverObj, "amount");
		if (amountObj.isNum() || amountObj.isStr()) {
			const CAmount &amount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
			if (amount <= 0)
				throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
			theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(recpt.nVersion, recpt.vchWitnessProgram), amount));
            nTotalSending += amount;
        }
		else
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected amount as number in receiver array");

	}
    const CAmount &nAuxFee = getAuxFee(stringFromVch(theAsset.vchPubData), nTotalSending, theAsset.nPrecision, auxFeeAddress);
    if(nAuxFee > 0){
        theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(auxFeeAddress.nVersion, auxFeeAddress.vchWitnessProgram), nAuxFee));
        nTotalSending += nAuxFee;
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
	return syscointxfund_helper(pwallet, strAddress, SYSCOIN_TX_VERSION_ALLOCATION_SEND, strWitness, vecSend);
}
template <typename T>
inline std::string int_to_hex(T val, size_t width=sizeof(T)*2)
{
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(width) << std::hex << (val|0);
    return ss.str();
}
UniValue assetallocationburn(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
	const UniValue &params = request.params;
    RPCHelpMan{"assetallocationburn",
        "\nBurn an asset allocation in order to use the bridge or move back to Syscoin\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address that owns this asset allocation"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of asset to burn to SYSX"},
            {"ethereum_destination_address", RPCArg::Type::STR, RPCArg::Optional::NO, "The 20 byte (40 character) hex string of the ethereum destination address. Leave empty to burn to Syscoin."}
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
            }},
        RPCExamples{
            HelpExampleCli("assetallocationburn", "\"asset_guid\" \"address\" \"amount\" \"ethereum_destination_address\"")
            + HelpExampleRpc("assetallocationburn", "\"asset_guid\", \"address\", \"amount\", \"ethereum_destination_address\"")
        }
    }.Check(request);

    const uint32_t &nAsset = params[0].get_uint();
	std::string strAddress = params[1].get_str();
    	
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
    // if no eth address provided just send as a std asset allocation send but to burn address
    if(ethAddress.empty() || ethAddress == "''"){
        theAssetAllocation.SetNull();
        theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(burnWitness.nVersion, burnWitness.vchWitnessProgram), amount));      
      
        vector<unsigned char> data;
        theAssetAllocation.Serialize(data);  
        scriptData << OP_RETURN << data;
        CScript scriptPubKeyFromOrig = GetScriptForDestination(DecodeDestination(strAddress));
        CRecipient recp = { scriptPubKeyFromOrig, amount, false };
        vecSend.push_back(recp);
        nVersion = SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN;
    }
    else{
        // convert to hex string because otherwise cscript will push a cscriptnum which is 4 bytes but we want 8 byte hex representation of an int64 pushed
        const std::string amountHex = int_to_hex(amount);
        const std::string witnessVersionHex = int_to_hex(assetAllocationTuple.witnessAddress.nVersion);
        const std::string assetHex = int_to_hex(nAsset);
        const std::string precisionHex = int_to_hex(theAsset.nPrecision);
        scriptData << OP_RETURN << ParseHex(assetHex) << ParseHex(amountHex) << ParseHex(ethAddress) << ParseHex(precisionHex) << theAsset.vchContract << ParseHex(witnessVersionHex) << assetAllocationTuple.witnessAddress.vchWitnessProgram;
        nVersion = SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM;
    }
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee); 
	return syscointxfund_helper(pwallet, strAddress, nVersion, "", vecSend);
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
    const UniValue &params = request.params;
    RPCHelpMan{"assetallocationmint",
        "\nMint assetallocation to come back from the bridge\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Mint to this address."},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of asset to mint.  Note that fees (in SYS) will be taken from the owner address"},
            {"blocknumber", RPCArg::Type::NUM, RPCArg::Optional::NO, "Block number of the block that included the burn transaction on Ethereum."},
            {"tx_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Transaction hex."},
            {"txroot_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction merkle root that commits this transaction to the block header."},
            {"txmerkleproof_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The list of parent nodes of the Merkle Patricia Tree for SPV proof of transaction merkle root."},
            {"merklerootpath_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The merkle path to walk through the tree to recreate the merkle hash for both transaction and receipt root."},
            {"receipt_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "Transaction Receipt Hex."},
            {"receiptroot_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction receipt merkle root that commits this receipt to the block header."},
            {"receiptmerkleproof_hex", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The list of parent nodes of the Merkle Patricia Tree for SPV proof of transaction receipt merkle root."},
            {"witness", RPCArg::Type::STR, "\"\"", "Witness address that will sign for web-of-trust notarization of this transaction."}
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
            }},
        RPCExamples{
            HelpExampleCli("assetallocationmint", "\"assetguid\" \"address\" \"amount\" \"blocknumber\" \"tx_hex\" \"txroot_hex\" \"txmerkleproof_hex\" \"txmerkleproofpath_hex\" \"receipt_hex\" \"receiptroot_hex\" \"receiptmerkleproof\" \"witness\"")
            + HelpExampleRpc("assetallocationmint", "\"assetguid\", \"address\", \"amount\", \"blocknumber\", \"tx_hex\", \"txroot_hex\", \"txmerkleproof_hex\", \"txmerkleproofpath_hex\", \"receipt_hex\", \"receiptroot_hex\", \"receiptmerkleproof\", \"\"")
        }
    }.Check(request);
    const uint32_t &nAsset = params[0].get_uint();
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");            
    std::string strAddress = params[1].get_str();
    UniValue amountValue = request.params[2];
    const CAmount &nAmount = AssetAmountFromValue(amountValue, theAsset.nPrecision);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for assetallocationmint");  

    const uint32_t &nBlockNumber = params[3].get_uint(); 
    
    string vchTxValue = params[4].get_str();
    string vchTxRoot = params[5].get_str();
    string vchTxParentNodes = params[6].get_str();

    // find byte offset of tx data in the parent nodes
    size_t pos = vchTxParentNodes.find(vchTxValue);
    if(pos == std::string::npos || vchTxParentNodes.size() > (USHRT_MAX*2)){
        throw JSONRPCError(RPC_TYPE_ERROR, "Could not find tx value in tx parent nodes");  
    }
    uint16_t posTxValue = (uint16_t)pos/2;
    string vchTxPath = params[7].get_str();
 
    string vchReceiptValue = params[8].get_str();
    string vchReceiptRoot = params[9].get_str();
    string vchReceiptParentNodes = params[10].get_str();
    pos = vchReceiptParentNodes.find(vchReceiptValue);
    if(pos == std::string::npos || vchReceiptParentNodes.size() > (USHRT_MAX*2)){
        throw JSONRPCError(RPC_TYPE_ERROR, "Could not find receipt value in receipt parent nodes");  
    }
    uint16_t posReceiptValue = (uint16_t)pos/2;
    string strWitness = params[11].get_str();
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
       
    vector<unsigned char> data;
    mintSyscoin.Serialize(data);
    
    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);
    return syscointxfund_helper(pwallet, strAddress, SYSCOIN_TX_VERSION_ALLOCATION_MINT, strWitness, vecSend);
}

UniValue assetallocationsend(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    RPCHelpMan{"assetallocationsend",
        "\nSend an asset allocation you own to another address.\n",
        {
            {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The asset guid"},
            {"address_sender", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the allocation from"},
            {"address_receiver", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the allocation to"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The quantity of asset to send"}
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
            }},
        RPCExamples{
            HelpExampleCli("assetallocationsend", "\"assetguid\" \"addressfrom\" \"address\" \"amount\"")
            + HelpExampleRpc("assetallocationsend", "\"assetguid\", \"addressfrom\", \"address\", \"amount\"")
        }
    }.Check(request);
    const uint32_t &nAsset = params[0].get_uint();
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw JSONRPCError(RPC_DATABASE_ERROR, "Could not find a asset with this key");            
    UniValue amountValue = request.params[3];
    CAmount nAmount = AssetAmountFromValue(amountValue, theAsset.nPrecision);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for assetallocationsend");          
    UniValue output(UniValue::VARR);
    UniValue outputObj(UniValue::VOBJ);
    outputObj.__pushKV("address", params[2].get_str());
    outputObj.__pushKV("amount", ValueFromAssetAmount(nAmount, theAsset.nPrecision));
    output.push_back(outputObj);
    UniValue paramsFund(UniValue::VARR);
    paramsFund.push_back(nAsset);
    paramsFund.push_back(params[1].get_str());
    paramsFund.push_back(output);
    paramsFund.push_back("");
    JSONRPCRequest requestMany;
    requestMany.params = paramsFund;
    requestMany.URI = request.URI;
    return assetallocationsendmany(requestMany);          
}


UniValue sendfrom(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    RPCHelpMan{"sendfrom",
        "\nSend an amount to a given address from a specified address.",   
        {
            {"funding_address", RPCArg::Type::STR, RPCArg::Optional::NO, "The syscoin address is sending from."},
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The syscoin address to send to."},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount in " + CURRENCY_UNIT + " to send. eg 0.1"}
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "hex", "the unsigned funded transaction hexstring."},
            }},
        RPCExamples{
            HelpExampleCli("sendfrom", "\"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleCli("sendfrom", "\"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleCli("sendfrom", "\"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleRpc("sendfrom", "\"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1")
        }
    }.Check(request);

    CTxDestination from = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(from)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid from address");
    }

    CTxDestination dest = DecodeDestination(request.params[1].get_str());
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid to address");
    }

    // Amount
    CAmount nAmount = AmountFromValue(request.params[2]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");

    CScript scriptPubKey = GetScriptForDestination(dest);
    CRecipient recipient = {scriptPubKey, nAmount, false};
	std::vector<CRecipient> vecSend;
	vecSend.push_back(recipient);
    std::string strWitness = "";
    return syscointxfund_helper(pwallet, EncodeDestination(from), 0, strWitness, vecSend);
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
    { "syscoinwallet",            "syscoinburntoassetallocation",     &syscoinburntoassetallocation,  {"funding_address","asset_guid","amount"} }, 
    { "syscoinwallet",            "convertaddresswallet",             &convertaddresswallet,          {"address","label","rescan"} },
    { "syscoinwallet",            "assetallocationburn",              &assetallocationburn,           {"asset_guid","address","amount","ethereum_destination_address"} }, 
    { "syscoinwallet",            "assetallocationmint",              &assetallocationmint,           {"asset_guid","address","amount","blocknumber","tx_hex","txroot_hex","txmerkleproof_hex","txmerkleproofpath_hex","receipt_hex","receiptroot_hex","receiptmerkleproof","witness"} },     
    { "syscoinwallet",            "assetnew",                         &assetnew,                      {"address","symbol","description","contract","precision","total_supply","max_supply","update_flags","aux_fees","witness"}},
    { "syscoinwallet",            "assetupdate",                      &assetupdate,                   {"asset_guid","description","contract","supply","update_flags","aux_fees","witness"}},
    { "syscoinwallet",            "assettransfer",                    &assettransfer,                 {"asset_guid","address","witness"}},
    { "syscoinwallet",            "assetsend",                        &assetsend,                     {"asset_guid","address","amount"}},
    { "syscoinwallet",            "assetsendmany",                    &assetsendmany,                 {"asset_guid","inputs"}},
    { "syscoinwallet",            "assetallocationsend",              &assetallocationsend,           {"asset_guid","address_sender","address_receiver","amount"}},
    { "syscoinwallet",            "assetallocationsendmany",          &assetallocationsendmany,       {"asset_guid","address","inputs","witness"}},
    { "syscoinwallet",            "sendfrom",                         &sendfrom,                      {"funding_address","address","amount"}},
};
// clang-format on

void RegisterAssetWalletRPCCommands(interfaces::Chain& chain, std::vector<std::unique_ptr<interfaces::Handler>>& handlers)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        handlers.emplace_back(chain.handleRpc(commands[vcidx]));
}
