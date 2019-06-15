// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <validation.h>
#include <boost/algorithm/string.hpp>
#include <rpc/util.h>
#include <wallet/rpcwallet.h>
#include <services/assetconsensus.h>
#include <services/rpc/assetrpc.h>
#include <keystore.h>
#include <wallet/coincontrol.h>
#include <iomanip>
#include <rpc/server.h>
#include <chainparams.h>
extern std::string EncodeDestination(const CTxDestination& dest);
extern CTxDestination DecodeDestination(const std::string& str);
extern UniValue ValueFromAmount(const CAmount& amount);
extern UniValue DescribeAddress(const CTxDestination& dest);
extern std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags = 0);
extern bool DecodeHexTx(CMutableTransaction& tx, const std::string& hex_tx, bool try_no_witness = false, bool try_witness = true);
extern ArrivalTimesMapImpl arrivalTimesMap; 
extern CCriticalSection cs_assetallocationarrival;
extern CAmount GetMinimumFee(const CWallet& wallet, unsigned int nTxBytes, const CCoinControl& coin_control, FeeCalculation* feeCalc);
extern bool IsDust(const CTxOut& txout, const CFeeRate& dustRelayFee);
extern CAmount AssetAmountFromValue(UniValue& value, int precision);
extern UniValue ValueFromAssetAmount(const CAmount& amount, int precision);
using namespace std;
std::vector<CTxIn> savedtxins;
UniValue syscointxfund(const JSONRPCRequest& request);
void CreateFeeRecipient(CScript& scriptPubKey, CRecipient& recipient)
{
    CRecipient recp = { scriptPubKey, 0, false };
    recipient = recp;
}

UniValue syscointxfund_helper(const string& strAddress, const int &nVersion, const string &vchWitness, const vector<CRecipient> &vecSend, const COutPoint& outpoint=emptyOutPoint) {
    CMutableTransaction txNew;
    if(nVersion > 0)
        txNew.nVersion = nVersion;

    COutPoint witnessOutpoint, addressOutpoint;
    if (!vchWitness.empty() && vchWitness != "''")
    {
        string strWitnessAddress;
        strWitnessAddress = vchWitness;
        addressunspent(strWitnessAddress, witnessOutpoint);
        if (witnessOutpoint.IsNull() || !IsOutpointMature(witnessOutpoint))
        {
            throw runtime_error("SYSCOIN_RPC_ERROR ERRCODE: 9000 - " + _("This transaction requires a witness but no enough outputs found for witness address: ") + strWitnessAddress);
        }
        Coin pcoinW;
        if (GetUTXOCoin(witnessOutpoint, pcoinW))
            txNew.vin.push_back(CTxIn(witnessOutpoint, pcoinW.out.scriptPubKey));
    }
    addressunspent(strAddress, addressOutpoint);

    if (!outpoint.IsNull())
        addressOutpoint = outpoint;

    if (addressOutpoint.IsNull() || !IsOutpointMature(addressOutpoint))
    {
        throw runtime_error("SYSCOIN_RPC_ERROR ERRCODE: 9000 - " + _("Not enough outputs found for address: ") + strAddress);
    }
    Coin pcoin;
    if (GetUTXOCoin(addressOutpoint, pcoin)){
        CTxIn txIn(addressOutpoint, pcoin.out.scriptPubKey);
        // hack for double spend zdag4 test so we can spend multiple inputs of an address within a block and get different inputs every time we call this function
        if(fTPSTest && fTPSTestEnabled){
            if(std::find(savedtxins.begin(), savedtxins.end(), txIn) == savedtxins.end()){
                savedtxins.push_back(txIn);
                txNew.vin.push_back(txIn);
            }   
            else{
                LogPrint(BCLog::SYS, "Skipping saved output in syscointxfund_helper...\n");
            }
        }
        else
            txNew.vin.push_back(txIn);
    }
        
    // vouts to the payees
    for (const auto& recipient : vecSend)
    {
        CTxOut txout(recipient.nAmount, recipient.scriptPubKey);
        txNew.vout.push_back(txout);
    }   
    
    UniValue paramsFund(UniValue::VARR);
    paramsFund.push_back(EncodeHexTx(CTransaction(txNew)));
    paramsFund.push_back(strAddress);
    
    JSONRPCRequest request;
    request.params = paramsFund;
    return syscointxfund(request);
}


class CCountSigsVisitor : public boost::static_visitor<void> {
private:
    const CKeyStore &keystore;
    int &nNumSigs;

public:
    CCountSigsVisitor(const CKeyStore &keystoreIn, int &numSigs) : keystore(keystoreIn), nNumSigs(numSigs) {}

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

    void operator()(const CScriptID &scriptId) {
        CScript script;
        if (keystore.GetCScript(scriptId, script))
            Process(script);
    }
    void operator()(const WitnessV0ScriptHash& scriptID)
    {
        CScriptID id;
        CRIPEMD160().Write(scriptID.begin(), 32).Finalize(id.begin());
        CScript script;
        if (keystore.GetCScript(id, script)) {
            Process(script);
        }
    }

    void operator()(const WitnessV0KeyHash& keyid) {
        nNumSigs++;
    }

    template<typename X>
    void operator()(const X &none) {}
};
UniValue syscointxfund(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    const UniValue &params = request.params;
    if (request.fHelp || 1 > params.size() || 3 < params.size())
        throw runtime_error(
            RPCHelpMan{"syscointxfund",
                "\nFunds a new syscoin transaction with inputs used from wallet or an array of addresses specified. Note that any inputs to the transaction added prior to calling this will not be accounted and new outputs will be added every time you call this function.\n",
                {
                    {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The raw syscoin transaction output given from rpc"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address funding this transaction."},
                    {"output_index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Output index from available UTXOs in address. Defaults to selecting all that are needed to fund the transaction."}
                },
                RPCResult{
                    "{\n"
                    "  \"hex\": \"hexstring\"       (string) the unsigned funded transaction hexstring.\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("syscointxfund", "<hexstring> \"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\"")
                    + HelpExampleRpc("syscointxfund", "<hexstring>, \"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\", 0")
                    + HelpRequiringPassphrase(pwallet)
                }
            }.ToString());
    const string &hexstring = params[0].get_str();
    const string &strAddress = params[1].get_str();
    CMutableTransaction txIn, tx;
    // decode as non-witness
    if (!DecodeHexTx(txIn, hexstring, true, false))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5500 - " + _("Could not send raw transaction: Cannot decode transaction from hex string: ") + hexstring);

    UniValue addressArray(UniValue::VARR);  
    int output_index = -1;
    if (params.size() > 2) {
        output_index = params[2].get_int();
    }
 
    CScript scriptPubKeyFromOrig = GetScriptForDestination(DecodeDestination(strAddress));
    addressArray.push_back("addr(" + strAddress + ")"); 
    
    
    
    LOCK(cs_main);
    CCoinsViewCache view(pcoinsTip.get());
   

    FeeCalculation fee_calc;
    CCoinControl coin_control;
    tx = txIn;
    tx.vin.clear();
    // # vin (with IX)*FEE + # vout*FEE + (10 + # vin)*FEE + 34*FEE (for change output)
    CAmount nFees =  GetMinimumFee(*pwallet, 10+34, coin_control,  &fee_calc);
    for (auto& vin : txIn.vin) {
        Coin coin;
        if (!GetUTXOCoin(vin.prevout, coin))    
            continue;
        {
            LOCK(pwallet->cs_wallet);
            if (pwallet->IsLockedCoin(vin.prevout.hash, vin.prevout.n)){
                LogPrintf("locked coin skipping...\n");
                continue;
            }
        }
        tx.vin.push_back(vin);
        int numSigs = 0;
        CCountSigsVisitor(*pwallet, numSigs).Process(coin.out.scriptPubKey);
        nFees += GetMinimumFee(*pwallet, numSigs * 200, coin_control, &fee_calc);
    }
    txIn = tx;
    CTransaction txIn_t(txIn);
    CAmount nCurrentAmount = view.GetValueIn(txIn_t);   
    // add total output amount of transaction to desired amount
    CAmount nDesiredAmount = txIn_t.GetValueOut();
    // mint transactions should start with 0 because the output is minted based on spv proof
    if(txIn_t.nVersion == SYSCOIN_TX_VERSION_MINT) 
        nDesiredAmount = 0;
   
    for (auto& vout : tx.vout) {
        const unsigned int nBytes = ::GetSerializeSize(vout, PROTOCOL_VERSION);
        nFees += GetMinimumFee(*pwallet, nBytes, coin_control, &fee_calc);
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
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5501 - " + _("No unspent outputs found in addresses provided"));
        utxoArray = resUtxoUnspents.get_array();
    }
    else
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5501 - " + _("No funds found in addresses provided"));

   
    if (nCurrentAmount < (nDesiredAmount + nFees)) {

        LOCK(mempool.cs);
        for (unsigned int i = 0; i < utxoArray.size(); i++)
        {
            const UniValue& utxoObj = utxoArray[i].get_obj();
            const string &strTxid = find_value(utxoObj, "txid").get_str();
            const uint256& txid = uint256S(strTxid);
            const int& nOut = find_value(utxoObj, "vout").get_int();
            const std::vector<unsigned char> &data(ParseHex(find_value(utxoObj, "scriptPubKey").get_str()));
            const CScript& scriptPubKey = CScript(data.begin(), data.end());
            const CAmount &nValue = AmountFromValue(find_value(utxoObj, "amount"));
            const CTxIn txIn(txid, nOut, scriptPubKey);
            const COutPoint outPoint(txid, nOut);
            if (std::find(tx.vin.begin(), tx.vin.end(), txIn) != tx.vin.end())
                continue;

            if (mempool.mapNextTx.find(outPoint) != mempool.mapNextTx.end())
                continue;
            {
                LOCK(pwallet->cs_wallet);
                if (pwallet->IsLockedCoin(txid, nOut))
                    continue;
            }
            if (!IsOutpointMature(outPoint))
                continue;
            bool locked = false;
            // spending while using a locked outpoint should be invalid
            if (plockedoutpointsdb->ReadOutpoint(outPoint, locked) && locked)
                continue;
            // hack for double spend zdag4 test so we can spend multiple inputs of an address within a block and get different inputs every time we call this function
            if(fTPSTest && fTPSTestEnabled){
                if(std::find(savedtxins.begin(), savedtxins.end(), txIn) == savedtxins.end())
                    savedtxins.push_back(txIn);
                else{
                    LogPrint(BCLog::SYS, "Skipping saved output in syscointxfund...\n");
                    continue;
                }
            }
            int numSigs = 0;
            CCountSigsVisitor(*pwallet, numSigs).Process(scriptPubKey);
            // add fees to account for every input added to this transaction
            nFees += GetMinimumFee(*pwallet, numSigs * 200, coin_control, &fee_calc);
            tx.vin.push_back(txIn);
            nCurrentAmount += nValue;
            if (nCurrentAmount >= (nDesiredAmount + nFees)) {
                break;
            }
        }
    }
    
    
  
    const CAmount &nChange = nCurrentAmount - nDesiredAmount - nFees;
    if (nChange < 0)
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5502 - " + _("Insufficient funds, short by: ") + ValueFromAmount(-1*nChange).write() + " SYS");
        
    // change back to funding address
    const CTxDestination & dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest))
        throw runtime_error("Change address is not valid");
    CTxOut changeOut(nChange, GetScriptForDestination(dest));
    if (!IsDust(changeOut, pwallet->chain().relayDustFee()))
        tx.vout.push_back(changeOut);
    
    
    // pass back new raw transaction
    UniValue res(UniValue::VOBJ);
    res.__pushKV("hex", EncodeHexTx(CTransaction(tx)));
    return res;
}
UniValue syscoinburn(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 3 != params.size())
        throw runtime_error(
            RPCHelpMan{"syscoinburn",
                "\nBurns the syscoin for bridging to Ethereum token\n",
                {
                    {"funding_address", RPCArg::Type::STR, RPCArg::Optional::NO, "Funding address to burn SYS from"},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of SYS to burn. Note that fees are applied on top. It is not inclusive of fees."},
                    {"ethereum_destination_address", RPCArg::Type::STR, RPCArg::Optional::NO, "The 20 bytes (40 character) hex string of the ethereum destination address.  Leave empty to burn as normal without the bridge"}
                },
                RPCResult{
                    "{\n"
                    "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("syscoinburn", "\"funding_address\" \"amount\" \"ethaddress\"")
                    + HelpExampleRpc("syscoinburn", "\"funding_address\", \"amount\", \"ethaddress\"")
                }
         }.ToString());
    string fundingAddress = params[0].get_str();    
    CAmount nAmount = AmountFromValue(params[1]);
    string ethAddress = params[2].get_str();
    boost::erase_all(ethAddress, "0x");  // strip 0x if exist

   
    vector<CRecipient> vecSend;
    CScript scriptData;
    scriptData << OP_RETURN;
    if (!ethAddress.empty()){
        scriptData << ParseHex(ethAddress);
    }
    
    CRecipient burn;
    CreateFeeRecipient(scriptData, burn);
    burn.nAmount = nAmount;
    vecSend.push_back(burn);
    UniValue res = syscointxfund_helper(fundingAddress, ethAddress.empty()? 0: SYSCOIN_TX_VERSION_BURN, "", vecSend);
    return res;
}
UniValue syscoinmint(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 11 != params.size())
        throw runtime_error(
                RPCHelpMan{"syscoinmint",
                "\nMint syscoin to come back from the ethereum bridge\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Mint to this address."},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of SYS to mint.  Note that fees are applied on top.  It is not inclusive of fees"},
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
                    "{\n"
                    "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("syscoinmint","\"address\" \"amount\" \"blocknumber\" \"tx_hex\" \"txroot_hex\" \"txmerkleproof\" \"txmerkleproofpath\" \"receipt_hex\" \"receiptroot_hex\" \"receiptmerkleproof\"")
                    + HelpExampleRpc("syscoinmint","\"address\", \"amount\", \"blocknumber\", \"tx_hex\", \"txroot_hex\", \"txmerkleproof\", \"txmerkleproofpath\", \"receipt_hex\", \"receiptroot_hex\", \"receiptmerkleproof\", \"\"")
                }
                }.ToString());

    string vchAddress = params[0].get_str();
    CAmount nAmount = AmountFromValue(params[1]);
    uint32_t nBlockNumber;
    if(params[2].isNum())
        nBlockNumber = (uint32_t)params[2].get_int();
    else if(params[2].isStr())
        ParseUInt32(params[2].get_str(), &nBlockNumber);
 
    string vchTxValue = params[3].get_str();
    string vchTxRoot = params[4].get_str();
    string vchTxParentNodes = params[5].get_str();
    string vchTxPath = params[6].get_str();
 
    string vchReceiptValue = params[7].get_str();
    string vchReceiptRoot = params[8].get_str();
    string vchReceiptParentNodes = params[9].get_str();
    
    string strWitnessAddress = params[10].get_str();
    
    vector<CRecipient> vecSend;
    const CTxDestination &dest = DecodeDestination(vchAddress);
    
    CScript scriptPubKeyFromOrig = GetScriptForDestination(dest);
    if(!fGethSynced){
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5502 - " + _("Geth is not synced, please wait until it syncs up and try again"));
    }
    int nBlocksLeftToEnable = ::ChainActive().Tip()->nHeight - (Params().GetConsensus().nBridgeStartBlock+500);
    if(nBlocksLeftToEnable > 0)
    {
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5502 - " + _("Bridge is not enabled yet. Blocks left to enable: ") + itostr(nBlocksLeftToEnable));
    }
    CMintSyscoin mintSyscoin;
    mintSyscoin.nBlockNumber = nBlockNumber;
    mintSyscoin.vchTxValue = ParseHex(vchTxValue);
    mintSyscoin.vchTxRoot = ParseHex(vchTxRoot);
    mintSyscoin.vchTxParentNodes = ParseHex(vchTxParentNodes);
    mintSyscoin.vchTxPath = ParseHex(vchTxPath);
    mintSyscoin.vchReceiptValue = ParseHex(vchReceiptValue);
    mintSyscoin.vchReceiptRoot = ParseHex(vchReceiptRoot);
    mintSyscoin.vchReceiptParentNodes = ParseHex(vchReceiptParentNodes);
    
    EthereumTxRoot txRootDB;
    bool bGethTestnet = gArgs.GetBoolArg("-gethtestnet", false);
    uint32_t cutoffHeight;
    const bool &ethTxRootShouldExist = !fLiteMode && fLoaded && fGethSynced;
    if(!ethTxRootShouldExist){
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Network is not ready to accept your mint transaction please wait..."));
    }
    // validate that the block passed is committed to by the tx root he also passes in, then validate the spv proof to the tx root below  
    // the cutoff to keep txroots is 120k blocks and the cutoff to get approved is 40k blocks. If we are syncing after being offline for a while it should still validate up to 120k worth of txroots
    if(!pethereumtxrootsdb || !pethereumtxrootsdb->ReadTxRoots(mintSyscoin.nBlockNumber, txRootDB)){
        if(ethTxRootShouldExist){
            throw runtime_error("SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Missing transaction root for SPV proof at Ethereum block: ") + itostr(mintSyscoin.nBlockNumber));
        }
    }  
    if(ethTxRootShouldExist){
        LOCK(cs_ethsyncheight);
        // cutoff is ~1 week of blocks is about 40K blocks
        cutoffHeight = (fGethSyncHeight - MAX_ETHEREUM_TX_ROOTS) + 100;
        if(fGethSyncHeight >= MAX_ETHEREUM_TX_ROOTS && mintSyscoin.nBlockNumber <= (uint32_t)cutoffHeight) {
            throw runtime_error("SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("The block height is too old, your SPV proof is invalid. SPV Proof must be done within 40000 blocks of the burn transaction on Ethereum blockchain"));
        } 
        
        // ensure that we wait at least ETHEREUM_CONFIRMS_REQUIRED blocks (~1 hour) before we are allowed process this mint transaction  
        // also ensure sanity test that the current height that our node thinks Eth is on isn't less than the requested block for spv proof
        if(fGethCurrentHeight <  mintSyscoin.nBlockNumber || fGethSyncHeight <= 0 || (fGethSyncHeight - mintSyscoin.nBlockNumber < (bGethTestnet? 20: ETHEREUM_CONFIRMS_REQUIRED*1.5))){
            throw runtime_error("SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Not enough confirmations on Ethereum to process this mint transaction. Blocks required: ") + itostr((ETHEREUM_CONFIRMS_REQUIRED*1.5) - (fGethSyncHeight - mintSyscoin.nBlockNumber)));
        } 
    }
          
    vector<unsigned char> data;
    mintSyscoin.Serialize(data);
    
    
    CRecipient recp = { scriptPubKeyFromOrig, nAmount, false };
    vecSend.push_back(recp);
    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);
    
    UniValue res = syscointxfund_helper(vchAddress, SYSCOIN_TX_VERSION_MINT, strWitnessAddress, vecSend);
    return res;
}

UniValue assetnew(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 9)
        throw runtime_error(
            RPCHelpMan{"assetnew",
            "\nCreate a new asset\n",
            {
                {"address", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "An address that you own."},
                {"symbol", RPCArg::Type::STR, RPCArg::Optional::NO, "Asset symbol (1-8 characters)"},
                {"public_value", RPCArg::Type::STR, RPCArg::Optional::NO, "public data, 256 characters max."},
                {"contract", RPCArg::Type::STR, RPCArg::Optional::NO, "Ethereum token contract for SyscoinX bridge. Must be in hex and not include the '0x' format tag. For example contract '0xb060ddb93707d2bc2f8bcc39451a5a28852f8d1d' should be set as 'b060ddb93707d2bc2f8bcc39451a5a28852f8d1d'. Leave empty for no smart contract bridge."},
                {"precision", RPCArg::Type::NUM, RPCArg::Optional::NO, "Precision of balances. Must be between 0 and 8. The lower it is the higher possible max_supply is available since the supply is represented as a 64 bit integer. With a precision of 8 the max supply is 10 billion."},
                {"total_supply", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Initial supply of asset. Can mint more supply up to total_supply amount or if total_supply is -1 then minting is uncapped."},
                {"max_supply", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Maximum supply of this asset. Set to -1 for uncapped. Depends on the precision value that is set, the lower the precision the higher max_supply can be."},
                {"update_flags", RPCArg::Type::NUM, RPCArg::Optional::NO, "Ability to update certain fields. Must be decimal value which is a bitmask for certain rights to update. The bitmask represents 0x01(1) to give admin status (needed to update flags), 0x10(2) for updating public data field, 0x100(4) for updating the smart contract field, 0x1000(8) for updating supply, 0x10000(16) for being able to update flags (need admin access to update flags as well). 0x11111(31) for all."},
                {"witness", RPCArg::Type::STR, RPCArg::Optional::NO, "Witness address that will sign for web-of-trust notarization of this transaction."}
            },
            RPCResult{
            "{\n"
            "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
            "  \"assetguid\": xxxx        (numeric) The guid of asset to be created\n"
            "}\n"
            },
            RPCExamples{
            HelpExampleCli("assetnew", "\"myaddress\" \"CAT\" \"publicvalue\" \"contractaddr\" 8 100 1000 31 \"\"")
            + HelpExampleRpc("assetnew", "\"myaddress\", \"CAT\", \"publicvalue\", \"contractaddr\", 8, 100, 1000, 31, \"\"")
            }
            }.ToString());
    string vchAddress = params[0].get_str();
    string strSymbol = params[1].get_str();
    string strPubData = params[2].get_str();
    if(strPubData == "''")
        strPubData.clear();
    string strContract = params[3].get_str();
    if(strContract == "''")
        strContract.clear();
    if(!strContract.empty())
         boost::erase_all(strContract, "0x");  // strip 0x in hex str if exist
   
    int precision = params[4].get_int();
    string vchWitness;
    UniValue param4 = params[5];
    UniValue param5 = params[6];
    
    CAmount nBalance = AssetAmountFromValue(param4, precision);
    CAmount nMaxSupply = AssetAmountFromValue(param5, precision);
    int nUpdateFlags = params[7].get_int();
    vchWitness = params[8].get_str();

    string strAddressFrom;
    string strAddress = vchAddress;
    const CTxDestination address = DecodeDestination(strAddress);

    UniValue detail = DescribeAddress(address);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();   


    // calculate net
    // build asset object
    CAsset newAsset;
    newAsset.nAsset = GenerateSyscoinGuid();
    newAsset.strSymbol = strSymbol;
    newAsset.vchPubData = vchFromString(strPubData);
    newAsset.vchContract = ParseHex(strContract);
    newAsset.witnessAddress = CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex));
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
    vecSend.push_back(fee);
    UniValue res = syscointxfund_helper(vchAddress, SYSCOIN_TX_VERSION_ASSET_ACTIVATE, vchWitness, vecSend);
    res.__pushKV("asset_guid", (int)newAsset.nAsset);
    return res;
}
UniValue assetupdate(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 6)
        throw runtime_error(
            RPCHelpMan{"assetupdate",
                "\nPerform an update on an asset you control.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
                    {"public_value", RPCArg::Type::STR, RPCArg::Optional::NO, "Public data, 256 characters max."},
                    {"contract",  RPCArg::Type::STR, RPCArg::Optional::NO, "Ethereum token contract for SyscoinX bridge. Leave empty for no smart contract bridg."},
                    {"supply", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "New supply of asset. Can mint more supply up to total_supply amount or if max_supply is -1 then minting is uncapped. If greator than zero, minting is assumed otherwise set to 0 to not mint any additional tokens."},
                    {"update_flags", RPCArg::Type::NUM, RPCArg::Optional::NO, "Ability to update certain fields. Must be decimal value which is a bitmask for certain rights to update. The bitmask represents 0x01(1) to give admin status (needed to update flags), 0x10(2) for updating public data field, 0x100(4) for updating the smart contract field, 0x1000(8) for updating supply, 0x10000(16) for being able to update flags (need admin access to update flags as well). 0x11111(31) for all."},
                    {"witness", RPCArg::Type::STR, RPCArg::Optional::NO, "Witness address that will sign for web-of-trust notarization of this transaction."}
                },
                RPCResult{
                    "{\n"
                    "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("assetupdate", "\"assetguid\" \"publicvalue\" \"contractaddress\" \"supply\" \"update_flags\" \"\"")
                    + HelpExampleRpc("assetupdate", "\"assetguid\", \"publicvalue\", \"contractaddress\", \"supply\", \"update_flags\", \"\"")
                }
            }.ToString());
    const int &nAsset = params[0].get_int();
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

    int nUpdateFlags = params[4].get_int();
    string vchWitness;
    vchWitness = params[5].get_str();
    
    CAsset theAsset;

    if (!GetAsset( nAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Could not find a asset with this key"));
        
    const CWitnessAddress &copyWitness = theAsset.witnessAddress;
    theAsset.ClearAsset();
    theAsset.witnessAddress = copyWitness;
    UniValue params3 = params[3];
    CAmount nBalance = 0;
    if((params3.isStr() && params3.get_str() != "0") || (params3.isNum() && params3.get_real() != 0))
        nBalance = AssetAmountFromValue(params3, theAsset.nPrecision);
    if(strPubData != stringFromVch(theAsset.vchPubData))
        theAsset.vchPubData = vchFromString(strPubData);
    else
        theAsset.vchPubData.clear();
    if(vchContract != theAsset.vchContract)
        theAsset.vchContract = vchContract;
    else
        theAsset.vchContract.clear();

    theAsset.nBalance = nBalance;
    if (theAsset.nUpdateFlags != nUpdateFlags)
        theAsset.nUpdateFlags = nUpdateFlags;
    else
        theAsset.nUpdateFlags = 0;

    vector<unsigned char> data;
    theAsset.Serialize(data);
    

    vector<CRecipient> vecSend;


    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);
    return syscointxfund_helper(theAsset.witnessAddress.ToString(), SYSCOIN_TX_VERSION_ASSET_UPDATE, vchWitness, vecSend);
}

UniValue assettransfer(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 3)
        throw runtime_error(
            RPCHelpMan{"assettransfer",
            "\nTransfer an asset you own to another address.\n",
            {
                {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid."},
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address to transfer to."},
                {"witness", RPCArg::Type::STR, RPCArg::Optional::NO, "Witness address that will sign for web-of-trust notarization of this transaction."}
            },
            RPCResult{
            "{\n"
            "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
            "}\n"
            },
            RPCExamples{
                HelpExampleCli("assettransfer", "\"asset_guid\" \"address\" \"\"")
                + HelpExampleRpc("assettransfer", "\"asset_guid\", \"address\", \"\"")
            }
            }.ToString());

    // gather & validate inputs
    const int &nAsset = params[0].get_int();
    string vchAddressTo = params[1].get_str();
    string vchWitness;
    vchWitness = params[2].get_str();

    CScript scriptPubKeyOrig, scriptPubKeyFromOrig;
    CAsset theAsset;
    if (!GetAsset( nAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2505 - " + _("Could not find a asset with this key"));
    


    const CTxDestination addressTo = DecodeDestination(vchAddressTo);


    UniValue detail = DescribeAddress(addressTo);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();   

    theAsset.ClearAsset();
    CScript scriptPubKey;
    theAsset.witnessAddressTransfer = CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex));

    vector<unsigned char> data;
    theAsset.Serialize(data);


    vector<CRecipient> vecSend;
    

    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);
    return syscointxfund_helper(theAsset.witnessAddress.ToString(), SYSCOIN_TX_VERSION_ASSET_TRANSFER, vchWitness, vecSend);
}
UniValue assetsendmany(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 3)
        throw runtime_error(
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
                {"witness", RPCArg::Type::STR, "\"\"", "Witnesses address that will sign for web-of-trust notarization of this transaction"}
            },
            RPCResult{
            "{\n"
            "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
            "}\n"
            },
            RPCExamples{
                HelpExampleCli("assetsendmany", "\"assetguid\" '[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\' \"\"")
                + HelpExampleCli("assetsendmany", "\"assetguid\" \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\" \"\"")
                + HelpExampleRpc("assetsendmany", "\"assetguid\",\'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\' \"\"")
                + HelpExampleRpc("assetsendmany", "\"assetguid\",\"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\" \"\"")
            }
            }.ToString());
    // gather & validate inputs
    const int &nAsset = params[0].get_int();
    UniValue valueTo = params[1];
    string vchWitness = params[2].get_str();
    if (!valueTo.isArray())
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Array of receivers not found");

    CAsset theAsset;
    if (!GetAsset(nAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2507 - " + _("Could not find a asset with this key"));



    CAssetAllocation theAssetAllocation;
    theAssetAllocation.assetAllocationTuple = CAssetAllocationTuple(nAsset, theAsset.witnessAddress);

    UniValue receivers = valueTo.get_array();
    for (unsigned int idx = 0; idx < receivers.size(); idx++) {
        const UniValue& receiver = receivers[idx];
        if (!receiver.isObject())
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"address'\", or \"amount\"}");

        const UniValue &receiverObj = receiver.get_obj();
        const std::string &toStr = find_value(receiverObj, "address").get_str();
        CWitnessAddress recpt;
        if(toStr != "burn"){
            CTxDestination dest = DecodeDestination(toStr);
            if(!IsValidDestination(dest))
                throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2509 - " + _("Asset must be sent to a valid syscoin address"));

            UniValue detail = DescribeAddress(dest);
            if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
                throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
            string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
            unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();    
            recpt.vchWitnessProgram = ParseHex(witnessProgramHex);
            recpt.nVersion = witnessVersion;
        } 
        else{
            recpt.vchWitnessProgram = vchFromString("burn");
            recpt.nVersion = 0;
        }               
        UniValue amountObj = find_value(receiverObj, "amount");
        if (amountObj.isNum() || amountObj.isStr()) {
            const CAmount &amount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
            if (amount <= 0)
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
            theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(recpt.nVersion, recpt.vchWitnessProgram), amount));
        }
        else
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected amount as number in receiver array");

    }

    CScript scriptPubKey;

    vector<unsigned char> data;
    theAssetAllocation.Serialize(data);
    
    vector<CRecipient> vecSend;

    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);

    return syscointxfund_helper(theAsset.witnessAddress.ToString(), SYSCOIN_TX_VERSION_ASSET_SEND, vchWitness, vecSend);
}

UniValue assetsend(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 3)
        throw runtime_error(
            RPCHelpMan{"assetsend",
            "\nSend an asset you own to another address.\n",
            {
                {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The asset guid."},
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the asset to (creates an asset allocation)."},
                {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The quantity of asset to send."}
            },
            RPCResult{
            "{\n"
            "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
            "}\n"
            },
            RPCExamples{
                HelpExampleCli("assetsend", "\"assetguid\" \"address\" \"amount\"")
                + HelpExampleRpc("assetsend", "\"assetguid\", \"address\", \"amount\"")
                }

            }.ToString());
    const uint32_t &nAsset = params[0].get_int();
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("Could not find a asset with this key"));            
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
    paramsFund.push_back((int)nAsset);
    paramsFund.push_back(output);
    paramsFund.push_back("");
    JSONRPCRequest requestMany;
    requestMany.params = paramsFund;
    return assetsendmany(requestMany);          
}

UniValue assetallocationsendmany(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 4)
		throw runtime_error(
            RPCHelpMan{"assetallocationsendmany",
                "\nSend an asset allocation you own to another address. Maximum recipients is 250.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
                    {"addressfrom", RPCArg::Type::STR, RPCArg::Optional::NO, "Address that owns this asset allocation"},
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
                     {"witness", RPCArg::Type::STR, "\"\"", "Witness address that will sign for web-of-trust notarization of this transaction"}
                },
                RPCResult{
                    "{\n"
                    "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationsendmany", "\"assetguid\" \"addressfrom\" \'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\' \"\"")
                    + HelpExampleCli("assetallocationsendmany", "\"assetguid\" \"addressfrom\" \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\" \"\"")
                    + HelpExampleRpc("assetallocationsendmany", "\"assetguid\", \"addressfrom\", \'[{\"address\":\"sysaddress1\",\"amount\":100},{\"address\":\"sysaddress2\",\"amount\":200}]\', \"\"")
                    + HelpExampleRpc("assetallocationsendmany", "\"assetguid\", \"addressfrom\", \"[{\\\"address\\\":\\\"sysaddress1\\\",\\\"amount\\\":100},{\\\"address\\\":\\\"sysaddress2\\\",\\\"amount\\\":200}]\", \"\"")
                }
            }.ToString());

	// gather & validate inputs
	const int &nAsset = params[0].get_int();
	string vchAddressFrom = params[1].get_str();
	UniValue valueTo = params[2];
	vector<unsigned char> vchWitness;
    string strWitness = params[3].get_str();
	if (!valueTo.isArray())
		throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Array of receivers not found");
	string strAddressFrom;
	const string &strAddress = vchAddressFrom;
    CTxDestination addressFrom;
    string witnessProgramHex;
    unsigned char witnessVersion = 0;
    if(strAddress != "burn"){
	    addressFrom = DecodeDestination(strAddress);
    	if (IsValidDestination(addressFrom)) {
    		strAddressFrom = strAddress;
    	
            UniValue detail = DescribeAddress(addressFrom);
            if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
                throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
            witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str(); 
            witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();    
        }  
    }
    
	CAssetAllocation theAssetAllocation;
	const CAssetAllocationTuple assetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, strAddress == "burn"? vchFromString("burn"): ParseHex(witnessProgramHex)));
	if (!GetAssetAllocation(assetAllocationTuple, theAssetAllocation))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1500 - " + _("Could not find a asset allocation with this key"));

	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("Could not find a asset with this key"));
	const COutPoint lockedOutpoint = theAssetAllocation.lockedOutpoint;
   
	theAssetAllocation.SetNull();
    theAssetAllocation.assetAllocationTuple.nAsset = std::move(assetAllocationTuple.nAsset);
    theAssetAllocation.assetAllocationTuple.witnessAddress = std::move(assetAllocationTuple.witnessAddress); 
	UniValue receivers = valueTo.get_array();
	
	for (unsigned int idx = 0; idx < receivers.size(); idx++) {
		const UniValue& receiver = receivers[idx];
		if (!receiver.isObject())
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"address'\" or \"amount\"}");

		const UniValue &receiverObj = receiver.get_obj();
        const std::string &toStr = find_value(receiverObj, "address").get_str();
        CWitnessAddress recpt;
        if(toStr != "burn"){
            CTxDestination dest = DecodeDestination(toStr);
            if(!IsValidDestination(dest))
                throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2509 - " + _("Asset must be sent to a valid syscoin address"));

            UniValue detail = DescribeAddress(dest);
            if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
                throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
            string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
            unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();    
            recpt.vchWitnessProgram = ParseHex(witnessProgramHex);
            recpt.nVersion = witnessVersion;
        } 
        else{
            recpt.vchWitnessProgram = vchFromString("burn");
            recpt.nVersion = 0;
        }  
		UniValue amountObj = find_value(receiverObj, "amount");
		if (amountObj.isNum() || amountObj.isStr()) {
			const CAmount &amount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
			if (amount <= 0)
				throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
			theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(recpt.nVersion, recpt.vchWitnessProgram), amount));
		}
		else
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected amount as number in receiver array");

	}
    
    {
        LOCK(cs_assetallocationarrival);
    	// check to see if a transaction for this asset/address tuple has arrived before minimum latency period
    	const ArrivalTimesMap &arrivalTimes = arrivalTimesMap[assetAllocationTuple.ToString()];
    	const int64_t & nNow = GetTimeMillis();
    	int minLatency = ZDAG_MINIMUM_LATENCY_SECONDS * 1000;
    	if (fUnitTest)
    		minLatency = 1000;
    	for (auto& arrivalTime : arrivalTimes) {
    		// if this tx arrived within the minimum latency period flag it as potentially conflicting
    		if ((nNow - arrivalTime.second) < minLatency) {
    			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1503 - " + _("Please wait a few more seconds and try again..."));
    		}
    	}
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
	return syscointxfund_helper(strAddressFrom, SYSCOIN_TX_VERSION_ASSET_ALLOCATION_SEND, strWitness, vecSend, lockedOutpoint);
}
template <typename T>
inline std::string int_to_hex(T val, size_t width=sizeof(T)*2)
{
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(width) << std::hex << (val|0);
    return ss.str();
}
UniValue assetallocationburn(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 4 != params.size())
		throw runtime_error(
            RPCHelpMan{"assetallocationburn",
                "\nBurn an asset allocation in order to use the bridge\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address that owns this asset allocation"},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of asset to burn to SYSX"},
                    {"ethereum_destination_address", RPCArg::Type::STR, RPCArg::Optional::NO, "The 20 byte (40 character) hex string of the ethereum destination address. Leave empty to burn as normal without the bridge.  If it is left empty this will process as a normal assetallocationsend to the burn address"}
                },
                RPCResult{
                    "{\n"
                    "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationburn", "\"asset_guid\" \"address\" \"amount\" \"ethereum_destination_address\"")
                    + HelpExampleRpc("assetallocationburn", "\"asset_guid\", \"address\", \"amount\", \"ethereum_destination_address\"")
                }
            }.ToString());

    uint32_t nAsset;
    if(params[0].isNum())
        nAsset = (uint32_t)params[0].get_int();
    else if(params[0].isStr())
        ParseUInt32(params[0].get_str(), &nAsset);
	string strAddress = params[1].get_str();
    
	const CTxDestination &addressFrom = DecodeDestination(strAddress);

    
    UniValue detail = DescribeAddress(addressFrom);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();            	
	CAssetAllocation theAssetAllocation;
	const CAssetAllocationTuple assetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)));
	if (!GetAssetAllocation(assetAllocationTuple, theAssetAllocation))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1500 - " + _("Could not find a asset allocation with this key"));
    {
        LOCK(cs_assetallocationarrival);
        // check to see if a transaction for this asset/address tuple has arrived before minimum latency period
        const ArrivalTimesMap &arrivalTimes = arrivalTimesMap[assetAllocationTuple.ToString()];
        const int64_t & nNow = GetTimeMillis();
        int minLatency = ZDAG_MINIMUM_LATENCY_SECONDS * 1000;
        if (fUnitTest)
            minLatency = 1000;
        for (auto& arrivalTime : arrivalTimes) {
            // if this tx arrived within the minimum latency period flag it as potentially conflicting
            if ((nNow - arrivalTime.second) < minLatency) {
                throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1503 - " + _("Please wait a few more seconds and try again..."));
            }
        }
    }
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("Could not find a asset with this key"));
        
    UniValue amountObj = params[2];
	CAmount amount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
	string ethAddress = params[3].get_str();
    boost::erase_all(ethAddress, "0x");  // strip 0x if exist
    // if no eth address provided just send as a std asset allocation send but to burn address
    if(ethAddress.empty()){
        UniValue output(UniValue::VARR);
        UniValue outputObj(UniValue::VOBJ);
        outputObj.__pushKV("address", "burn");
        outputObj.__pushKV("amount", ValueFromAssetAmount(amount, theAsset.nPrecision));
        output.push_back(outputObj);
        UniValue paramsFund(UniValue::VARR);
        paramsFund.push_back((int)nAsset);
        paramsFund.push_back(strAddress);
        paramsFund.push_back(output);
        paramsFund.push_back("");
        JSONRPCRequest requestMany;
        requestMany.params = paramsFund;
        return assetallocationsendmany(requestMany);  
    }
    // convert to hex string because otherwise cscript will push a cscriptnum which is 4 bytes but we want 8 byte hex representation of an int64 pushed
    const std::string amountHex = int_to_hex(amount);
    const std::string witnessVersionHex = int_to_hex(assetAllocationTuple.witnessAddress.nVersion);
    const std::string assetHex = int_to_hex(nAsset);

    
	vector<CRecipient> vecSend;


	CScript scriptData;
	scriptData << OP_RETURN << ParseHex(assetHex) << ParseHex(amountHex) << ParseHex(ethAddress) << ParseHex(witnessVersionHex) << assetAllocationTuple.witnessAddress.vchWitnessProgram;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);

	return syscointxfund_helper(strAddress, SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN, "", vecSend);
}
UniValue assetallocationmint(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 12 != params.size())
        throw runtime_error(
            RPCHelpMan{"assetallocationmint",
                "\nMint assetallocation to come back from the bridge\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Mint to this address."},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of asset to mint.  Note that fees will be taken from the owner address"},
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
                    "{\n"
                    "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationmint", "\"assetguid\" \"address\" \"amount\" \"blocknumber\" \"tx_hex\" \"txroot_hex\" \"txmerkleproof_hex\" \"txmerkleproofpath_hex\" \"receipt_hex\" \"receiptroot_hex\" \"receiptmerkleproof\" \"witness\"")
                    + HelpExampleRpc("assetallocationmint", "\"assetguid\", \"address\", \"amount\", \"blocknumber\", \"tx_hex\", \"txroot_hex\", \"txmerkleproof_hex\", \"txmerkleproofpath_hex\", \"receipt_hex\", \"receiptroot_hex\", \"receiptmerkleproof\", \"\"")
                }
            }.ToString());

    uint32_t nAsset;
    if(params[0].isNum())
        nAsset = (uint32_t)params[0].get_int();
    else if(params[0].isStr())
        ParseUInt32(params[0].get_str(), &nAsset);

    string strAddress = params[1].get_str();
    CAmount nAmount = AmountFromValue(params[2]);
    
    uint32_t nBlockNumber;
    if(params[3].isNum())
        nBlockNumber = (uint32_t)params[3].get_int();
    else if(params[3].isStr())
        ParseUInt32(params[3].get_str(), &nBlockNumber);    
    
    string vchTxValue = params[4].get_str();
    string vchTxRoot = params[5].get_str();
    string vchTxParentNodes = params[6].get_str();
    string vchTxPath = params[7].get_str();
 
    string vchReceiptValue = params[8].get_str();
    string vchReceiptRoot = params[9].get_str();
    string vchReceiptParentNodes = params[10].get_str();
    
    
    string strWitness = params[11].get_str();
    if(!fGethSynced){
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5502 - " + _("Geth is not synced, please wait until it syncs up and try again"));
    }

    int nBlocksLeftToEnable = ::ChainActive().Tip()->nHeight - (Params().GetConsensus().nBridgeStartBlock+500);
    if(nBlocksLeftToEnable > 0)
    {
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5502 - " + _("Bridge is not enabled yet. Blocks left to enable: ") + itostr(nBlocksLeftToEnable));
    }
    const CTxDestination &dest = DecodeDestination(strAddress);
    UniValue detail = DescribeAddress(dest);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();
    vector<CRecipient> vecSend;
    
    CMintSyscoin mintSyscoin;
    mintSyscoin.assetAllocationTuple = CAssetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)));
    mintSyscoin.nValueAsset = nAmount;
    mintSyscoin.nBlockNumber = nBlockNumber;
    mintSyscoin.vchTxValue = ParseHex(vchTxValue);
    mintSyscoin.vchTxRoot = ParseHex(vchTxRoot);
    mintSyscoin.vchTxParentNodes = ParseHex(vchTxParentNodes);
    mintSyscoin.vchTxPath = ParseHex(vchTxPath);
    mintSyscoin.vchReceiptValue = ParseHex(vchReceiptValue);
    mintSyscoin.vchReceiptRoot = ParseHex(vchReceiptRoot);
    mintSyscoin.vchReceiptParentNodes = ParseHex(vchReceiptParentNodes);
    
    EthereumTxRoot txRootDB;
    bool bGethTestnet = gArgs.GetBoolArg("-gethtestnet", false);
    uint32_t cutoffHeight;
    const bool &ethTxRootShouldExist = !fLiteMode && fLoaded && fGethSynced;
    if(!ethTxRootShouldExist){
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Network is not ready to accept your mint transaction please wait..."));
    }
    // validate that the block passed is committed to by the tx root he also passes in, then validate the spv proof to the tx root below  
    // the cutoff to keep txroots is 120k blocks and the cutoff to get approved is 40k blocks. If we are syncing after being offline for a while it should still validate up to 120k worth of txroots
    if(!pethereumtxrootsdb || !pethereumtxrootsdb->ReadTxRoots(mintSyscoin.nBlockNumber, txRootDB)){
        if(ethTxRootShouldExist){
            throw runtime_error("SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Missing transaction root for SPV proof at Ethereum block: ") + itostr(mintSyscoin.nBlockNumber));
        }
    }  
    if(ethTxRootShouldExist){
        LOCK(cs_ethsyncheight);
        // cutoff is ~1 week of blocks is about 40K blocks
        cutoffHeight = (fGethSyncHeight - MAX_ETHEREUM_TX_ROOTS) + 100;
        if(fGethSyncHeight >= MAX_ETHEREUM_TX_ROOTS && mintSyscoin.nBlockNumber <= (uint32_t)cutoffHeight) {
            throw runtime_error("SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("The block height is too old, your SPV proof is invalid. SPV Proof must be done within 40000 blocks of the burn transaction on Ethereum blockchain"));
        } 
        
        // ensure that we wait at least ETHEREUM_CONFIRMS_REQUIRED blocks (~1 hour) before we are allowed process this mint transaction  
        // also ensure sanity test that the current height that our node thinks Eth is on isn't less than the requested block for spv proof
        if(fGethCurrentHeight <  mintSyscoin.nBlockNumber || fGethSyncHeight <= 0 || (fGethSyncHeight - mintSyscoin.nBlockNumber < (bGethTestnet? 20: ETHEREUM_CONFIRMS_REQUIRED*1.5))){
            throw runtime_error("SYSCOIN_CONSENSUS_ERROR ERRCODE: 1001 - " + _("Not enough confirmations on Ethereum to process this mint transaction. Blocks required: ") + itostr((ETHEREUM_CONFIRMS_REQUIRED*1.5) - (fGethSyncHeight - mintSyscoin.nBlockNumber)));
        } 
    }
       
    vector<unsigned char> data;
    mintSyscoin.Serialize(data);
    
    CScript scriptData;
    scriptData << OP_RETURN << data;
    CRecipient fee;
    CreateFeeRecipient(scriptData, fee);
    vecSend.push_back(fee);
       
    return syscointxfund_helper(strAddress, SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT, strWitness, vecSend);
}

UniValue assetallocationsend(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 4)
        throw runtime_error(
            RPCHelpMan{"assetallocationsend",
                "\nSend an asset allocation you own to another address.\n",
                {
                    {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "The asset guid"},
                    {"address_sender", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the allocation from"},
                    {"address_receiver", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the allocation to"},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The quantity of asset to send"}
                },
                RPCResult{
                    "{\n"
                    "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
                    "}\n"
                },
                RPCExamples{
                    HelpExampleCli("assetallocationsend", "\"assetguid\" \"addressfrom\" \"address\" \"amount\"")
                    + HelpExampleRpc("assetallocationsend", "\"assetguid\", \"addressfrom\", \"address\", \"amount\"")
                }
            }.ToString());
    const uint32_t &nAsset = params[0].get_int();
	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("Could not find a asset with this key"));            
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
    paramsFund.push_back((int)nAsset);
    paramsFund.push_back(params[1].get_str());
    paramsFund.push_back(output);
    paramsFund.push_back("");
    JSONRPCRequest requestMany;
    requestMany.params = paramsFund;
    return assetallocationsendmany(requestMany);          
}


UniValue assetallocationlock(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 5)
		throw runtime_error(
            RPCHelpMan{"assetallocationlock",
            "\nLock an asset allocation to a specific UTXO (txid/output). This is useful for things such as hashlock and CLTV type operations where script checks are done on UTXO prior to spending which extend to an assetallocationsend.\n",
            {
                {"asset_guid", RPCArg::Type::NUM, RPCArg::Optional::NO, "Asset guid"},
                {"addressfrom", RPCArg::Type::STR, RPCArg::Optional::NO, "Address that owns this asset allocation"},
                {"txid", RPCArg::Type::STR, RPCArg::Optional::NO, "Transaction hash"},
                {"output_index", RPCArg::Type::NUM, RPCArg::Optional::NO, "Output index inside the transaction output array"},
                {"witness", RPCArg::Type::STR, "\"\"", "Witness address that will sign for web-of-trust notarization of this transaction"}
            },
            RPCResult{
            "{\n"
            "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
            "}\n"
            },
            RPCExamples{
            HelpExampleCli("assetallocationlock", "\"asset_guid\" \"addressfrom\" \"txid\" \"output_index\" \"\"")
            + HelpExampleRpc("assetallocationlock", "\"asset_guid\",\"addressfrom\",\"txid\",\"output_index\",\"\"")
            }
            }.ToString()); 
            
	// gather & validate inputs
	const int &nAsset = params[0].get_int();
	string vchAddressFrom = params[1].get_str();
	uint256 txid = uint256S(params[2].get_str());
	int outputIndex = params[3].get_int();
	vector<unsigned char> vchWitness;
	string strWitness = params[4].get_str();

	string strAddressFrom;
	const string &strAddress = vchAddressFrom;
	CTxDestination addressFrom;
	string witnessProgramHex;
	unsigned char witnessVersion = 0;
	if (strAddress != "burn") {
		addressFrom = DecodeDestination(strAddress);
		if (IsValidDestination(addressFrom)) {
			strAddressFrom = strAddress;

			UniValue detail = DescribeAddress(addressFrom);
			if (find_value(detail.get_obj(), "iswitness").get_bool() == false)
				throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
			witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
			witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();
		}
	}

	CAssetAllocation theAssetAllocation;
	const CAssetAllocationTuple assetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, strAddress == "burn" ? vchFromString("burn") : ParseHex(witnessProgramHex)));
	if (!GetAssetAllocation(assetAllocationTuple, theAssetAllocation))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1500 - " + _("Could not find a asset allocation with this key"));

	theAssetAllocation.SetNull();
	theAssetAllocation.assetAllocationTuple.nAsset = std::move(assetAllocationTuple.nAsset);
	theAssetAllocation.assetAllocationTuple.witnessAddress = std::move(assetAllocationTuple.witnessAddress);
	theAssetAllocation.lockedOutpoint = COutPoint(txid, outputIndex);
    if(!IsOutpointMature(theAssetAllocation.lockedOutpoint))
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1500 - " + _("Outpoint not mature"));
    Coin pcoin;
    GetUTXOCoin(theAssetAllocation.lockedOutpoint, pcoin);
    CTxDestination address;
    if (!ExtractDestination(pcoin.out.scriptPubKey, address))
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1500 - " + _("Could not extract destination from outpoint"));
        
    const string& strAddressDest = EncodeDestination(address);
    if(strAddressFrom != strAddressDest)    
        throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1500 - " + _("Outpoint address must match allocation owner address"));
    
    vector<unsigned char> data;
    theAssetAllocation.Serialize(data);    
	vector<CRecipient> vecSend;

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);

	return syscointxfund_helper(strAddressFrom, SYSCOIN_TX_VERSION_ASSET_ALLOCATION_LOCK, strWitness, vecSend);
}
UniValue sendfrom(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            RPCHelpMan{"sendfrom",
                "\nSend an amount to a given address from a specified address.",   
                {
                    {"funding_address", RPCArg::Type::STR, RPCArg::Optional::NO, "The syscoin address is sending from."},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The syscoin address to send to."},
                    {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount in " + CURRENCY_UNIT + " to send. eg 0.1"}
                },
                RPCResult{
                "{\n"
                "  \"hex\": \"hexstring\"       (string) the unsigned transaction hexstring.\n"
                "}\n"
                },
                RPCExamples{
                    HelpExampleCli("sendfrom", "\"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
                    + HelpExampleCli("sendfrom", "\"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
                    + HelpExampleCli("sendfrom", "\"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
                    + HelpExampleRpc("sendfrom", "\"sys1qtyf33aa2tl62xhrzhralpytka0krxvt0a4e8ee\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1")
                }
            }.ToString()); 

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
    std::string strWitness ="";
    return syscointxfund_helper(EncodeDestination(from), 0, strWitness, vecSend);
}

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                                actor (function)                argNames
    //  --------------------- ------------------------          -----------------------         ----------

   /* assets using the blockchain, coins/points/service backed tokens*/
    { "syscoinwallet",            "syscoinburn",                      &syscoinburn,                   {"funding_address","amount","ethereum_destination_address"} },
    { "syscoinwallet",            "syscoinmint",                      &syscoinmint,                   {"address","amount","blocknumber","tx_hex","txroot_hex","txmerkleproof_hex","txmerkleproofpath_hex","receipt_hex","receiptroot_hex","receiptmerkleproof","witness"} }, 
    { "syscoinwallet",            "assetallocationburn",              &assetallocationburn,           {"asset_guid","address","amount","ethereum_destination_address"} }, 
    { "syscoinwallet",            "assetallocationmint",              &assetallocationmint,           {"asset_guid","address","amount","blocknumber","tx_hex","txroot_hex","txmerkleproof_hex","txmerkleproofpath_hex","receipt_hex","receiptroot_hex","receiptmerkleproof","witness"} },     
    { "syscoinwallet",            "syscointxfund",                    &syscointxfund,                 {"hexstring","address","output_index"}},
    { "syscoinwallet",            "assetnew",                         &assetnew,                      {"address","symbol","public value","contract","precision","total_supply","max_supply","update_flags","witness"}},
    { "syscoinwallet",            "assetupdate",                      &assetupdate,                   {"asset_guid","public value","contract","supply","update_flags","witness"}},
    { "syscoinwallet",            "assettransfer",                    &assettransfer,                 {"asset_guid","address","witness"}},
    { "syscoinwallet",            "assetsend",                        &assetsend,                     {"asset_guid","address","amount"}},
    { "syscoinwallet",            "assetsendmany",                    &assetsendmany,                 {"asset_guid","inputs"}},
    { "syscoinwallet",            "assetallocationlock",              &assetallocationlock,           {"asset_guid","address","txid","output_index","witness"}},
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
