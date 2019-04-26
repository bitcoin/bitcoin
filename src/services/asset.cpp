// Copyright (c) 2013-2019 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <core_io.h>
#include <wallet/wallet.h>
#include <wallet/rpcwallet.h>
#include <chainparams.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <wallet/coincontrol.h>
#include <rpc/util.h>
#include <key_io.h>
#include <policy/policy.h>
#include <consensus/validation.h>
#include <outputtype.h>
#include <boost/thread.hpp>
#include <merkleblock.h>
#include <services/assetconsensus.h>
#include <util/system.h>
#include <masternodeconfig.h>
extern CAmount GetMinimumFee(const CWallet& wallet, unsigned int nTxBytes, const CCoinControl& coin_control, FeeCalculation* feeCalc);

extern AssetBalanceMap mempoolMapAssetBalances;
extern ArrivalTimesMapImpl arrivalTimesMap;
extern CCriticalSection cs_assetallocation;
extern CCriticalSection cs_assetallocationarrival;
std::unique_ptr<CAssetDB> passetdb;
std::unique_ptr<CAssetAllocationDB> passetallocationdb;
std::unique_ptr<CAssetAllocationMempoolDB> passetallocationmempooldb;
std::unique_ptr<CEthereumTxRootsDB> pethereumtxrootsdb;
std::unique_ptr<CAssetIndexDB> passetindexdb;
std::vector<CTxIn> savedtxins;
// SYSCOIN service rpc functions
UniValue syscoinburn(const JSONRPCRequest& request);
UniValue syscoinmint(const JSONRPCRequest& request);
UniValue syscointxfund(const JSONRPCRequest& request);


UniValue syscoinlistreceivedbyaddress(const JSONRPCRequest& request);
UniValue syscoindecoderawtransaction(const JSONRPCRequest& request);

UniValue assetnew(const JSONRPCRequest& request);
UniValue assetupdate(const JSONRPCRequest& request);
UniValue addressbalance(const JSONRPCRequest& request);
UniValue assettransfer(const JSONRPCRequest& request);
UniValue assetsend(const JSONRPCRequest& request);
UniValue assetsendmany(const JSONRPCRequest& request);
UniValue assetinfo(const JSONRPCRequest& request);
UniValue listassets(const JSONRPCRequest& request);
UniValue syscoinsetethstatus(const JSONRPCRequest& request);
UniValue syscoinsetethheaders(const JSONRPCRequest& request);
UniValue getblockhashbytxid(const JSONRPCRequest& request);
UniValue syscoingetspvproof(const JSONRPCRequest& request);
using namespace std::chrono;
using namespace std;

int GetSyscoinDataOutput(const CTransaction& tx) {
	for (unsigned int i = 0; i<tx.vout.size(); i++) {
		if (tx.vout[i].scriptPubKey.IsUnspendable())
			return i;
	}
	return -1;
}
CAmount getaddressbalance(const string& strAddress)
{
    UniValue paramsUTXO(UniValue::VARR);
    UniValue utxoParams(UniValue::VARR);
    utxoParams.push_back("addr(" + strAddress + ")");
    paramsUTXO.push_back("start");
    paramsUTXO.push_back(utxoParams);
    JSONRPCRequest request;
    request.params = paramsUTXO;
    UniValue resUTXOs = scantxoutset(request);
    return AmountFromValue(find_value(resUTXOs.get_obj(), "total_amount"));
}
string stringFromValue(const UniValue& value) {
	string strName = value.get_str();
	return strName;
}
vector<unsigned char> vchFromValue(const UniValue& value) {
	string strName = value.get_str();
	unsigned char *strbeg = (unsigned char*)strName.c_str();
	return vector<unsigned char>(strbeg, strbeg + strName.size());
}

std::vector<unsigned char> vchFromString(const std::string &str) {
	unsigned char *strbeg = (unsigned char*)str.c_str();
	return vector<unsigned char>(strbeg, strbeg + str.size());
}
string stringFromVch(const vector<unsigned char> &vch) {
	string res;
	vector<unsigned char>::const_iterator vi = vch.begin();
	while (vi != vch.end()) {
		res += (char)(*vi);
		vi++;
	}
	return res;
}
bool GetSyscoinData(const CTransaction &tx, vector<unsigned char> &vchData, int& nOut)
{
	nOut = GetSyscoinDataOutput(tx);
	if (nOut == -1)
		return false;

	const CScript &scriptPubKey = tx.vout[nOut].scriptPubKey;
	return GetSyscoinData(scriptPubKey, vchData);
}
bool GetSyscoinData(const CScript &scriptPubKey, vector<unsigned char> &vchData)
{
	CScript::const_iterator pc = scriptPubKey.begin();
	opcodetype opcode;
	if (!scriptPubKey.GetOp(pc, opcode))
		return false;
	if (opcode != OP_RETURN)
		return false;
	if (!scriptPubKey.GetOp(pc, opcode, vchData))
		return false;
	return true;
}
bool GetSyscoinBurnData(const CTransaction &tx, CAssetAllocation* theAssetAllocation)
{   
    if(!theAssetAllocation) 
        return false;  
    std::vector<unsigned char> vchEthAddress;
    uint32_t nAssetFromScript;
    CAmount nAmountFromScript;
    CWitnessAddress burnWitnessAddress;
    if(!GetSyscoinBurnData(tx, nAssetFromScript, burnWitnessAddress, nAmountFromScript, vchEthAddress)){
        return false;
    }
    theAssetAllocation->SetNull();
    theAssetAllocation->assetAllocationTuple.nAsset = nAssetFromScript;
    theAssetAllocation->assetAllocationTuple.witnessAddress = burnWitnessAddress;
    theAssetAllocation->listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(0, vchFromString("burn")), nAmountFromScript));
    return true;

} 
bool GetSyscoinBurnData(const CTransaction &tx, uint32_t& nAssetFromScript, CWitnessAddress& burnWitnessAddress, CAmount &nAmountFromScript, std::vector<unsigned char> &vchEthAddress)
{
    if(tx.nVersion != SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Invalid transaction version\n");
        return false;
    }
    int nOut = GetSyscoinDataOutput(tx);
    if (nOut == -1){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Data index must be positive\n");
        return false;
    }

    const CScript &scriptPubKey = tx.vout[nOut].scriptPubKey;
    std::vector<std::vector< unsigned char> > vvchArgs;
    if(!GetSyscoinBurnData(scriptPubKey, vvchArgs)){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Cannot get burn data\n");
        return false;
    }
        
    if(vvchArgs.size() != 5){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Wrong argument size %d\n", vvchArgs.size());
        return false;
    }
          
    if(vvchArgs[0].size() != 4){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: nAssetFromScript - Wrong argument size %d\n", vvchArgs[0].size());
        return false;
    }
        
    nAssetFromScript  = static_cast<uint32_t>(vvchArgs[0][3]);
    nAssetFromScript |= static_cast<uint32_t>(vvchArgs[0][2]) << 8;
    nAssetFromScript |= static_cast<uint32_t>(vvchArgs[0][1]) << 16;
    nAssetFromScript |= static_cast<uint32_t>(vvchArgs[0][0]) << 24;
            
    if(vvchArgs[1].size() != 8){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: nAmountFromScript - Wrong argument size %d\n", vvchArgs[1].size());
        return false; 
    }
    uint64_t result = static_cast<uint64_t>(vvchArgs[1][7]);
    result |= static_cast<uint64_t>(vvchArgs[1][6]) << 8;
    result |= static_cast<uint64_t>(vvchArgs[1][5]) << 16;
    result |= static_cast<uint64_t>(vvchArgs[1][4]) << 24; 
    result |= static_cast<uint64_t>(vvchArgs[1][3]) << 32;  
    result |= static_cast<uint64_t>(vvchArgs[1][2]) << 40;  
    result |= static_cast<uint64_t>(vvchArgs[1][1]) << 48;  
    result |= static_cast<uint64_t>(vvchArgs[1][0]) << 56;   
    nAmountFromScript = (CAmount)result;
    
    if(vvchArgs[2].empty()){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Ethereum address empty\n");
        return false; 
    }
    vchEthAddress = vvchArgs[2]; 
    if(vvchArgs[3].size() != 1){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Witness address version - Wrong argument size %d\n", vvchArgs[3].size());
        return false;
    }
    const unsigned char &nWitnessVersion = static_cast<unsigned char>(vvchArgs[3][0]);
    
    if(vvchArgs[4].empty()){
        LogPrint(BCLog::SYS, "GetSyscoinBurnData: Witness address empty\n");
        return false;
    }     
    

    burnWitnessAddress = CWitnessAddress(nWitnessVersion, vvchArgs[4]);   
    return true; 
}
bool GetSyscoinBurnData(const CScript &scriptPubKey, std::vector<std::vector<unsigned char> > &vchData)
{
    CScript::const_iterator pc = scriptPubKey.begin();
    opcodetype opcode;
    if (!scriptPubKey.GetOp(pc, opcode))
        return false;
    if (opcode != OP_RETURN)
        return false;
    vector<unsigned char> vchArg;
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    vchData.push_back(vchArg);
    vchArg.clear();
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    vchData.push_back(vchArg);
    vchArg.clear();       
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    vchData.push_back(vchArg);
    vchArg.clear();        
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    vchData.push_back(vchArg);
    vchArg.clear();   
    if (!scriptPubKey.GetOp(pc, opcode, vchArg))
        return false;
    vchData.push_back(vchArg);
    vchArg.clear();              
    return true;
}


string assetFromTx(const int &nVersion) {
    switch (nVersion) {
    case SYSCOIN_TX_VERSION_ASSET_ACTIVATE:
        return "assetactivate";
    case SYSCOIN_TX_VERSION_ASSET_UPDATE:
        return "assetupdate";
    case SYSCOIN_TX_VERSION_ASSET_TRANSFER:
        return "assettransfer";
	case SYSCOIN_TX_VERSION_ASSET_SEND:
		return "assetsend";
    default:
        return "<unknown asset op>";
    }
}
bool CAsset::UnserializeFromData(const vector<unsigned char> &vchData) {
    try {
		CDataStream dsAsset(vchData, SER_NETWORK, PROTOCOL_VERSION);
		dsAsset >> *this;
    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	return true;
}
bool CMintSyscoin::UnserializeFromData(const vector<unsigned char> &vchData) {
    try {
        CDataStream dsMS(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsMS >> *this;
    } catch (std::exception &e) {
        SetNull();
        return false;
    }
    return true;
}
bool CAsset::UnserializeFromTx(const CTransaction &tx) {
	vector<unsigned char> vchData;
	int nOut;
	if (!IsAssetTx(tx.nVersion) || !GetSyscoinData(tx, vchData, nOut))
	{
		SetNull();
		return false;
	}
	if(!UnserializeFromData(vchData))
	{	
		return false;
	}
    return true;
}
bool CMintSyscoin::UnserializeFromTx(const CTransaction &tx) {
    vector<unsigned char> vchData;
    int nOut;
    if (!IsSyscoinMintTx(tx.nVersion) || !GetSyscoinData(tx, vchData, nOut))
    {
        SetNull();
        return false;
    }
    if(!UnserializeFromData(vchData))
    {   
        return false;
    }
    return true;
}
void LockMasternodesInDefaultWallet(){
    LogPrintf("Using masternode config file %s\n", GetMasternodeConfigFile().string());
    CWallet* const pwallet = GetDefaultWallet();
    if(gArgs.GetBoolArg("-mnconflock", true) && pwallet && (masternodeConfig.getCount() > 0)) {
        LOCK(pwallet->cs_wallet);
        LogPrintf("Locking Masternodes:\n");
        uint256 mnTxHash;
        uint32_t outputIndex;
        for (const auto& mne : masternodeConfig.getEntries()) {
            mnTxHash.SetHex(mne.getTxHash());
            outputIndex = (uint32_t)atoi(mne.getOutputIndex());
            COutPoint outpoint = COutPoint(mnTxHash, outputIndex);
            // don't lock non-spendable outpoint (i.e. it's already spent or it's not from this wallet at all)
            if(pwallet->IsMine(CTxIn(outpoint)) != ISMINE_SPENDABLE) {
                LogPrintf("  %s %s - IS NOT SPENDABLE, was not locked\n", mne.getTxHash(), mne.getOutputIndex());
                continue;
            }
            pwallet->LockCoin(outpoint);
            LogPrintf("  %s %s - locked successfully\n", mne.getTxHash(), mne.getOutputIndex());
        }
    }
}
bool FlushSyscoinDBs() {
    bool ret = true;
	 {
        if (passetallocationmempooldb != nullptr)
        {
            ResyncAssetAllocationStates();
            {
                LOCK(cs_assetallocation);
                LogPrintf("Flushing Asset Allocation Mempool Balances...size %d\n", mempoolMapAssetBalances.size());
                passetallocationmempooldb->WriteAssetAllocationMempoolBalances(mempoolMapAssetBalances);
                mempoolMapAssetBalances.clear();
            }
            {
                LOCK(cs_assetallocationarrival);
                LogPrintf("Flushing Asset Allocation Arrival Times...size %d\n", arrivalTimesMap.size());
                passetallocationmempooldb->WriteAssetAllocationMempoolArrivalTimes(arrivalTimesMap);
                arrivalTimesMap.clear();
            }
            if (!passetallocationmempooldb->Flush()) {
                LogPrintf("Failed to write to asset allocation mempool database!");
                ret = false;
            }            
        }
	 }
     if (pethereumtxrootsdb != nullptr)
     {
        if(!pethereumtxrootsdb->PruneTxRoots())
        {
            LogPrintf("Failed to write to prune Ethereum TX Roots database!");
            ret = false;
        }
        if (!pethereumtxrootsdb->Flush()) {
            LogPrintf("Failed to write to ethereum tx root database!");
            ret = false;
        } 
     }
	return ret;
}
void CTxMemPool::removeExpiredMempoolBalances(setEntries& stage){ 
    vector<vector<unsigned char> > vvch;
    int count = 0;
    for (const txiter& it : stage) {
        const CTransaction& tx = it->GetTx();
        if(IsAssetAllocationTx(tx.nVersion)){
            CAssetAllocation allocation(tx);
            if(allocation.assetAllocationTuple.IsNull())
                continue;
            if(ResetAssetAllocation(allocation.assetAllocationTuple.ToString(), tx.GetHash())){
                count++;
            }
        }
    }
    if(count > 0)
         LogPrint(BCLog::SYS, "removeExpiredMempoolBalances removed %d expired asset allocation transactions from mempool balances\n", count);  
}

bool FindAssetOwnerInTx(const CCoinsViewCache &inputs, const CTransaction& tx, const CWitnessAddress &witnessAddressToMatch) {
	CTxDestination dest;
    int witnessversion;
    std::vector<unsigned char> witnessprogram;
	for (unsigned int i = 0; i < tx.vin.size(); i++) {
		const Coin& prevCoins = inputs.AccessCoin(tx.vin[i].prevout);
		if (prevCoins.IsSpent()) {
			continue;
		}
        if (prevCoins.out.scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram) && witnessAddressToMatch.vchWitnessProgram == witnessprogram && witnessAddressToMatch.nVersion == (unsigned char)witnessversion)
            return true;
	}
	return false;
}
void CreateFeeRecipient(CScript& scriptPubKey, CRecipient& recipient)
{
	CRecipient recp = { scriptPubKey, 0, false };
	recipient = recp;
}
UniValue SyscoinListReceived(const CWallet* pwallet, bool includeempty = true, bool includechange = false)
{
    LOCK(pwallet->cs_wallet);
	map<string, int> mapAddress;
	UniValue ret(UniValue::VARR);
  
	const std::map<CKeyID, int64_t>& mapKeyPool = pwallet->GetAllReserveKeys();
	for (const std::pair<const CTxDestination, CAddressBookData>& item : pwallet->mapAddressBook) {

		const CTxDestination& dest = item.first;
		const string& strAccount = item.second.name;

		isminefilter filter = ISMINE_SPENDABLE;
		isminefilter mine = IsMine(*pwallet, dest);
		if (!(mine & filter))
			continue;

		const string& strAddress = EncodeDestination(dest);

        const CAmount& nBalance = getaddressbalance(strAddress);
		UniValue obj(UniValue::VOBJ);
		if (includeempty || (!includeempty && nBalance > 0)) {
			obj.pushKV("balance", ValueFromAmount(nBalance));
			obj.pushKV("label", strAccount);
			const CKeyID *keyID = boost::get<CKeyID>(&dest);
			if (keyID && !pwallet->mapAddressBook.count(dest) && !mapKeyPool.count(*keyID)) {
				if (!includechange)
					continue;
				obj.pushKV("change", true);
			}
			else
				obj.pushKV("change", false);
			ret.push_back(obj);
		}
		mapAddress[strAddress] = 1;
	}

	vector<COutput> vecOutputs;
	{
        auto locked_chain = pwallet->chain().lock();
        
		pwallet->AvailableCoins(*locked_chain, vecOutputs, true, nullptr, 1, MAX_MONEY, MAX_MONEY, 0, 0, 9999999);
	}
	for(const COutput& out: vecOutputs) {
		CTxDestination address;
		if (!ExtractDestination(out.tx->tx->vout[out.i].scriptPubKey, address))
			continue;

		const string& strAddress = EncodeDestination(address);
		if (mapAddress.find(strAddress) != mapAddress.end())
			continue;

		UniValue paramsBalance(UniValue::VARR);
		UniValue balanceParams(UniValue::VARR);
		balanceParams.push_back("addr(" + strAddress + ")");
		paramsBalance.push_back("start");
		paramsBalance.push_back(balanceParams);
		JSONRPCRequest request;
		request.params = paramsBalance;
		UniValue resBalance = scantxoutset(request);
		UniValue obj(UniValue::VOBJ);
		obj.pushKV("address", strAddress);
		const CAmount& nBalance = AmountFromValue(find_value(resBalance.get_obj(), "total_amount"));
		if (includeempty || (!includeempty && nBalance > 0)) {
			obj.pushKV("balance", ValueFromAmount(nBalance));
			obj.pushKV("label", "");
			const CKeyID *keyID = boost::get<CKeyID>(&address);
			if (keyID && !pwallet->mapAddressBook.count(address) && !mapKeyPool.count(*keyID)) {
				if (!includechange)
					continue;
				obj.pushKV("change", true);
			}
			else
				obj.pushKV("change", false);
			ret.push_back(obj);
		}
		mapAddress[strAddress] = 1;

	}
	return ret;
}
UniValue syscointxfund_helper(const string& strAddress, const int &nVersion, const string &vchWitness, vector<CRecipient> &vecSend) {
	CMutableTransaction txNew;
    if(nVersion > 0)
	    txNew.nVersion = nVersion;

	COutPoint witnessOutpoint, addressOutpoint;
	if (!vchWitness.empty() && vchWitness != "''")
	{
		string strWitnessAddress;
		strWitnessAddress = vchWitness;
		addressunspent(strWitnessAddress, witnessOutpoint);
		if (witnessOutpoint.IsNull())
		{
			throw runtime_error("SYSCOIN_RPC_ERROR ERRCODE: 9000 - " + _("This transaction requires a witness but no enough outputs found for witness address: ") + strWitnessAddress);
		}
		Coin pcoinW;
		if (GetUTXOCoin(witnessOutpoint, pcoinW) && !pcoinW.IsSpent())
			txNew.vin.push_back(CTxIn(witnessOutpoint, pcoinW.out.scriptPubKey));
	}
    addressunspent(strAddress, addressOutpoint);
    if (addressOutpoint.IsNull())
    {
        throw runtime_error("SYSCOIN_RPC_ERROR ERRCODE: 9000 - " + _("Not enough outputs found for address: ") + strAddress);
    }
    Coin pcoin;
    if (GetUTXOCoin(addressOutpoint, pcoin) && !pcoin.IsSpent()){
        CTxIn txIn(addressOutpoint, pcoin.out.scriptPubKey);
        // hack for double spend zdag4 test so we can spend multiple inputs of an address within a block and get different inputs everytime we call this function
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
			"syscointxfund\n"
			"\nFunds a new syscoin transaction with inputs used from wallet or an array of addresses specified. Note that any inputs to the transaction added prior to calling this will not be accounted and new outputs will be added everytime you call this function.\n"
			"\nArguments:\n"
			"  \"hexstring\" (string, required) The raw syscoin transaction output given from rpc (ie: assetnew, assetupdate)\n"
			"  \"address\"  (string, required) Address belonging to this asset transaction. \n"
			"  \"output_index\"  (number, optional) Output index from available UTXOs in address. Defaults to selecting all that are needed to fund the transaction. \n"
			"\nExamples:\n"
			+ HelpExampleCli("syscointxfund", " <hexstring> \"175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W\"")
			+ HelpExampleCli("syscointxfund", " <hexstring> \"175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W\" 0")
			+ HelpRequiringPassphrase(pwallet));
	const string &hexstring = params[0].get_str();
    const string &strAddress = params[1].get_str();
	CMutableTransaction tx;
    // decode as non-witness
	if (!DecodeHexTx(tx, hexstring, true, false))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5500 - " + _("Could not send raw transaction: Cannot decode transaction from hex string: ") + hexstring);

	UniValue addressArray(UniValue::VARR);	
	int output_index = -1;
    if (params.size() > 2) {
        output_index = params[2].get_int();
    }
 
    CScript scriptPubKeyFromOrig = GetScriptForDestination(DecodeDestination(strAddress));
    addressArray.push_back("addr(" + strAddress + ")"); 
    
    
    CTransaction txIn_t(tx);    
    LOCK(cs_main);
    CCoinsViewCache view(pcoinsTip.get());
    
    // add total output amount of transaction to desired amount
    CAmount nDesiredAmount = txIn_t.GetValueOut();
    // mint transactions should start with 0 because the output is minted based on spv proof
    if(txIn_t.nVersion == SYSCOIN_TX_VERSION_MINT) 
        nDesiredAmount = 0;
    CAmount nCurrentAmount = view.GetValueIn(txIn_t);

    FeeCalculation fee_calc;
    CCoinControl coin_control;

    // # vin (with IX)*FEE + # vout*FEE + (10 + # vin)*FEE + 34*FEE (for change output)
    CAmount nFees =  GetMinimumFee(*pwallet, 10+34, coin_control,  &fee_calc);
    for (auto& vin : tx.vin) {
        Coin coin;
        if (!GetUTXOCoin(vin.prevout, coin))
            continue;
        int numSigs = 0;
        CCountSigsVisitor(*pwallet, numSigs).Process(coin.out.scriptPubKey);
        nFees += GetMinimumFee(*pwallet, numSigs * 200, coin_control, &fee_calc);
    }
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
            // hack for double spend zdag4 test so we can spend multiple inputs of an address within a block and get different inputs everytime we call this function
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
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(CTransaction(tx)));
	return res;
}
UniValue syscoinburn(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 4 != params.size())
		throw runtime_error(
			"syscoinburn [funding_address] [amount] [burn_to_sysx] [ethereum_destination_address]\n"
            "<funding_address> Funding address to burn SYS from.\n"
			"<amount> Amount of SYS to burn. Note that fees are applied on top. It is not inclusive of fees.\n"
			"<burn_to_sysx> Boolean. Set to true if you are provably burning SYS to go to SYSX. False if you are provably burning SYS forever.\n");
      
    string fundingAddress = params[0].get_str();      
	CAmount nAmount = AmountFromValue(params[1]);
	bool bBurnToSYSX = params[2].get_bool();
    string ethAddress = params[3].get_str();
    boost::erase_all(ethAddress, "0x");  // strip 0x if exist

   
	vector<CRecipient> vecSend;
	CScript scriptData;
	scriptData << OP_RETURN;
	if (bBurnToSYSX){
		scriptData << ParseHex(ethAddress);
    }

	CMutableTransaction txNew;
    if(bBurnToSYSX)
        txNew.nVersion = SYSCOIN_TX_VERSION_BURN;
    
    
    
    CRecipient burn;
    CreateFeeRecipient(scriptData, burn);
    burn.nAmount = nAmount;
    vecSend.push_back(burn);
    
    UniValue res = syscointxfund_helper(fundingAddress, bBurnToSYSX? SYSCOIN_TX_VERSION_BURN: 0, "", vecSend);
    return res;
}
UniValue syscoinmint(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 8 != params.size())
		throw runtime_error(
			"syscoinmint [address] [amount] [blocknumber] [tx_hex] [txroot_hex] [txmerkleproof_hex] [txmerkleroofpath_hex] [witness]\n"
			"<address> Mint to this address.\n"
			"<amount> Amount of SYS to mint. Note that fees are applied on top. It is not inclusive of fees.\n"
            "<blocknumber> Block number of the block that included the burn transaction on Ethereum.\n"
            "<tx_hex> Raw transaction hex of the burn transaction on Ethereum.\n"
            "<txroot_hex> The transaction merkle root that commits this transaction to the block header.\n"
            "<txmerkleproof_hex> The list of parent nodes of the Merkle Patricia Tree for SPV proof.\n"
            "<txmerkleroofpath_hex> The merkle path to walk through the tree to recreate the merkle root.\n"
            "<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n");

	string vchAddress = params[0].get_str();
	CAmount nAmount = AmountFromValue(params[1]);
    uint32_t nBlockNumber = (uint32_t)params[2].get_int();
    string vchValue = params[3].get_str();
    boost::erase_all(vchValue, "'");
    string vchTxRoot = params[4].get_str();
    boost::erase_all(vchTxRoot, "'");
    string vchParentNodes = params[5].get_str();
    boost::erase_all(vchParentNodes, "'");
    string vchPath = params[6].get_str();
    boost::erase_all(vchPath, "'");
    string strWitnessAddress = params[7].get_str();
    
	vector<CRecipient> vecSend;
	const CTxDestination &dest = DecodeDestination(vchAddress);
    
	CScript scriptPubKeyFromOrig = GetScriptForDestination(dest);
    if(!fGethSynced){
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5502 - " + _("Geth is not synced, please wait until it syncs up and try again"));
    }

    CMintSyscoin mintSyscoin;
    mintSyscoin.vchValue = ParseHex(vchValue);
    mintSyscoin.vchTxRoot = ParseHex(vchTxRoot);
    mintSyscoin.nBlockNumber = nBlockNumber;
    mintSyscoin.vchParentNodes = ParseHex(vchParentNodes);
    mintSyscoin.vchPath = ParseHex(vchPath);
    
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
UniValue syscoindecoderawtransaction(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 1 != params.size())
		throw runtime_error("syscoindecoderawtransaction <hexstring>\n"
			"Decode raw syscoin transaction (serialized, hex-encoded) and display information pertaining to the service that is included in the transactiion data output(OP_RETURN)\n"
			"<hexstring> The transaction hex string.\n");
	string hexstring = params[0].get_str();
	CMutableTransaction tx;
	if(!DecodeHexTx(tx, hexstring, false, true))
        DecodeHexTx(tx, hexstring, true, true);
	CTransaction rawTx(tx);
	if (rawTx.IsNull())
		throw runtime_error("SYSCOIN_RPC_ERROR: ERRCODE: 5512 - " + _("Could not decode transaction"));
	
    UniValue output(UniValue::VOBJ);
    if(!DecodeSyscoinRawtransaction(rawTx, output))
        throw runtime_error("SYSCOIN_RPC_ERROR: ERRCODE: 5512 - " + _("Not a Syscoin transaction"));
	return output;
}
bool IsSyscoinMintTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT || nVersion == SYSCOIN_TX_VERSION_MINT;
}
bool IsAssetTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || nVersion == SYSCOIN_TX_VERSION_ASSET_UPDATE || nVersion == SYSCOIN_TX_VERSION_ASSET_TRANSFER || nVersion == SYSCOIN_TX_VERSION_ASSET_SEND;
}
bool IsAssetAllocationTx(const int &nVersion){
    return nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_MINT || nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_BURN || 
        nVersion == SYSCOIN_TX_VERSION_ASSET_ALLOCATION_SEND ;
}
bool IsSyscoinTx(const int &nVersion){
    return IsAssetTx(nVersion) || IsAssetAllocationTx(nVersion) || IsSyscoinMintTx(nVersion);
}
bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output){
    vector<vector<unsigned char> > vvch;
    bool found = false;
    if(IsSyscoinMintTx(rawTx.nVersion)){
        found = AssetMintTxToJson(rawTx, output);
    }
    else if (IsAssetTx(rawTx.nVersion) || IsAssetAllocationTx(rawTx.nVersion)){
        found = SysTxToJSON(rawTx, output);
    }
    
    return found;
}
bool SysTxToJSON(const CTransaction& tx, UniValue& output)
{
    bool found = false;
	if (IsAssetTx(tx.nVersion) && tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND)
		found = AssetTxToJSON(tx, output);
    else if(tx.nVersion == SYSCOIN_TX_VERSION_BURN)
        found = SysBurnTxToJSON(tx, output);        
	else if (IsAssetAllocationTx(tx.nVersion) || tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND)
		found = AssetAllocationTxToJSON(tx, output);
    return found;
}
bool SysBurnTxToJSON(const CTransaction &tx, UniValue &entry)
{
    int nHeight = 0;
    uint256 hash_block;
    CBlockIndex* blockindex = nullptr;
    CTransactionRef txRef;
    if (GetTransaction(tx.GetHash(), txRef, Params().GetConsensus(), hash_block, blockindex) && blockindex)
        nHeight = blockindex->nHeight; 
    entry.pushKV("txtype", "syscoinburn");
    entry.pushKV("_id", tx.GetHash().GetHex());
    entry.pushKV("txid", tx.GetHash().GetHex());
    entry.pushKV("height", nHeight);
    UniValue oOutputArray(UniValue::VARR);
    for (const auto& txout : tx.vout){
        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address))
            continue;
        UniValue oOutputObj(UniValue::VOBJ);
        const string& strAddress = EncodeDestination(address);
        oOutputObj.pushKV("address", strAddress);
        oOutputObj.pushKV("amount", ValueFromAmount(txout.nValue));   
        oOutputArray.push_back(oOutputObj);
    }
    
    entry.pushKV("outputs", oOutputArray);
    entry.pushKV("total", ValueFromAmount(tx.GetValueOut()));
    entry.pushKV("confirmed", nHeight > 0);  
    return true;
}
int GenerateSyscoinGuid()
{
    int rand = 0;
    while(rand <= SYSCOIN_TX_VERSION_MINT)
	    rand = GetRand(std::numeric_limits<int>::max());
    return rand;
}
unsigned int addressunspent(const string& strAddressFrom, COutPoint& outpoint)
{
	UniValue paramsUTXO(UniValue::VARR);
	UniValue utxoParams(UniValue::VARR);
	utxoParams.push_back("addr(" + strAddressFrom + ")");
	paramsUTXO.push_back("start");
	paramsUTXO.push_back(utxoParams);
	JSONRPCRequest request;
	request.params = paramsUTXO;
	UniValue resUTXOs = scantxoutset(request);
	UniValue utxoArray(UniValue::VARR);
    if (resUTXOs.isObject()) {
        const UniValue& resUtxoUnspents = find_value(resUTXOs.get_obj(), "unspents");
        if (!resUtxoUnspents.isArray())
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5501 - " + _("No unspent outputs found in addresses provided"));
        utxoArray = resUtxoUnspents.get_array();
    }   
    else
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5501 - " + _("No unspent outputs found in addresses provided"));
        
	unsigned int count = 0;
	{
		LOCK(mempool.cs);
		for (unsigned int i = 0; i < utxoArray.size(); i++)
		{
			const UniValue& utxoObj = utxoArray[i].get_obj();
			const uint256& txid = uint256S(find_value(utxoObj, "txid").get_str());
			const int& nOut = find_value(utxoObj, "vout").get_int();

			const COutPoint &outPointToCheck = COutPoint(txid, nOut);

			if (mempool.mapNextTx.find(outPointToCheck) != mempool.mapNextTx.end())
				continue;
			if (outpoint.IsNull())
				outpoint = outPointToCheck;
			count++;
		}
	}
	return count;
}

UniValue syscoinlistreceivedbyaddress(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 0)
		throw runtime_error(
			"syscoinlistreceivedbyaddress\n"
			"\nList balances by receiving address.\n"
			"\nResult:\n"
			"[\n"
			"  {\n"
			"    \"address\" : \"receivingaddress\",    (string) The receiving address\n"
			"    \"amount\" : x.xxx,					(numeric) The total amount in " + CURRENCY_UNIT + " received by the address\n"
			"    \"label\" : \"label\"                  (string) A comment for the address/transaction, if any\n"
			"  }\n"
			"  ,...\n"
			"]\n"

			"\nExamples:\n"
			+ HelpExampleCli("syscoinlistreceivedbyaddress", "")
		);

	return SyscoinListReceived(pwallet, true, false);
}

bool IsOutpointMature(const COutPoint& outpoint)
{
	Coin coin;
	GetUTXOCoin(outpoint, coin);
	if (coin.IsSpent())
		return false;
	int numConfirmationsNeeded = 0;
	if (coin.IsCoinBase())
		numConfirmationsNeeded = COINBASE_MATURITY - 1;

	if (coin.nHeight > -1 && chainActive.Tip())
		return (chainActive.Height() - coin.nHeight) >= numConfirmationsNeeded;

	// don't have chainActive or coin height is neg 1 or less
	return false;

}
void CAsset::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    dsAsset << *this;
	vchData = vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}
void CMintSyscoin::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsMint(SER_NETWORK, PROTOCOL_VERSION);
    dsMint << *this;
    vchData = vector<unsigned char>(dsMint.begin(), dsMint.end());

}
void WriteAssetIndexTXID(const uint32_t& nAsset, const uint256& txid){
    int64_t page;
    if(!passetindexdb->ReadAssetPage(page)){
        page = 0;
        if(!passetindexdb->WriteAssetPage(page))
           LogPrint(BCLog::SYS, "Failed to write asset page\n");                  
    }
    std::vector<uint256> TXIDS;
    passetindexdb->ReadIndexTXIDs(nAsset, page, TXIDS);
    // new page needed
    if(((int)TXIDS.size()) >= fAssetIndexPageSize){
        TXIDS.clear();
        page++;
        if(!passetindexdb->WriteAssetPage(page))
            LogPrint(BCLog::SYS, "Failed to write asset page\n");
    }
    TXIDS.push_back(txid);
    if(!passetindexdb->WriteIndexTXIDs(nAsset, page, TXIDS))
        LogPrint(BCLog::SYS, "Failed to write asset index txids\n");
}
void CAssetDB::WriteAssetIndex(const CTransaction& tx, const CAsset& dbAsset, const int& nHeight) {
	if (fZMQAsset || fAssetIndex) {
		UniValue oName(UniValue::VOBJ);
        // assetsends write allocation indexes
        if(tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND && AssetTxToJSON(tx, dbAsset, nHeight, oName)){
            if(fZMQAsset)
                GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "assetrecord");
            if(fAssetIndex)
            {
                if(!fAssetIndexGuids.empty() && std::find(fAssetIndexGuids.begin(),fAssetIndexGuids.end(),dbAsset.nAsset) == fAssetIndexGuids.end()){
                    LogPrint(BCLog::SYS, "Asset cannot be indexed because it is not set in -assetindexguids list\n");
                    return;
                }
                const uint256& txid = tx.GetHash();
                WriteAssetIndexTXID(dbAsset.nAsset, txid);
                if(!passetindexdb->WritePayload(txid, oName))
                    LogPrint(BCLog::SYS, "Failed to write asset index payload\n");
            }
        }
	}
}
bool GetAsset(const int &nAsset,
        CAsset& txPos) {
    if (passetdb == nullptr || !passetdb->ReadAsset(nAsset, txPos))
        return false;
    return true;
}



UniValue assetnew(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 8)
        throw runtime_error(
			"assetnew [address] [public value] [contract] [precision=8] [supply] [max_supply] [update_flags] [witness]\n"
						"<address> An address that you own.\n"
                        "<public value> public data, 256 characters max.\n"
                        "<contract> Ethereum token contract for SyscoinX bridge. Must be in hex and not include the '0x' format tag. For example contract '0xb060ddb93707d2bc2f8bcc39451a5a28852f8d1d' should be set as 'b060ddb93707d2bc2f8bcc39451a5a28852f8d1d'. Leave empty for no smart contract bridge.\n" 
						"<precision> Precision of balances. Must be between 0 and 8. The lower it is the higher possible max_supply is available since the supply is represented as a 64 bit integer. With a precision of 8 the max supply is 10 billion.\n"
						"<supply> Initial supply of asset. Can mint more supply up to total_supply amount or if total_supply is -1 then minting is uncapped.\n"
						"<max_supply> Maximum supply of this asset. Set to -1 for uncapped. Depends on the precision value that is set, the lower the precision the higher max_supply can be.\n"
						"<update_flags> Ability to update certain fields. Must be decimal value which is a bitmask for certain rights to update. The bitmask represents 0x01(1) to give admin status (needed to update flags), 0x10(2) for updating public data field, 0x100(4) for updating the smart contract/burn method signature fields, 0x1000(8) for updating supply, 0x10000(16) for being able to update flags (need admin access to update flags as well). 0x11111(31) for all.\n"
						"<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n");
	string vchAddress = params[0].get_str();
	vector<unsigned char> vchPubData = vchFromString(params[1].get_str());
    string strContract = params[2].get_str();
    if(!strContract.empty())
         boost::erase_all(strContract, "0x");  // strip 0x in hex str if exist
   
	int precision = params[3].get_int();
	string vchWitness;
	UniValue param4 = params[4];
	UniValue param5 = params[5];
	CAmount nBalance = AssetAmountFromValue(param4, precision);
	CAmount nMaxSupply = AssetAmountFromValue(param5, precision);
	int nUpdateFlags = params[6].get_int();
	vchWitness = params[7].get_str();

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
	newAsset.vchPubData = vchPubData;
    newAsset.vchContract = ParseHex(strContract);
	newAsset.witnessAddress = CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex));
	newAsset.nBalance = nBalance;
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
	res.push_back((int)newAsset.nAsset);
	return res;
}
UniValue addressbalance(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 1)
        throw runtime_error(
            "addressbalance [address]\n");
    string address = params[0].get_str();
    UniValue res(UniValue::VARR);
    res.push_back(ValueFromAmount(getaddressbalance(address)));
    return res;
}

UniValue assetupdate(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 6)
        throw runtime_error(
			"assetupdate [asset] [public value] [contract] [supply] [update_flags] [witness]\n"
				"Perform an update on an asset you control.\n"
				"<asset> Asset guid.\n"
                "<public value> Public data, 256 characters max.\n"
                "<contract> Ethereum token contract for SyscoinX bridge. Leave empty for no smart contract bridge.\n"             
				"<supply> New supply of asset. Can mint more supply up to total_supply amount or if max_supply is -1 then minting is uncapped. If greator than zero, minting is assumed otherwise set to 0 to not mint any additional tokens.\n"
                "<update_flags> Ability to update certain fields. Must be decimal value which is a bitmask for certain rights to update. The bitmask represents 0x01(1) to give admin status (needed to update flags), 0x10(2) for updating public data field, 0x100(4) for updating the smart contract/burn method signature fields, 0x1000(8) for updating supply, 0x10000(16) for being able to update flags (need admin access to update flags as well). 0x11111(31) for all.\n"
                "<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n");
	const int &nAsset = params[0].get_int();
	string strData = "";
	string strPubData = "";
	string strCategory = "";
	strPubData = params[1].get_str();
    string strContract = params[2].get_str();
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
    
	UniValue param3 = params[3];
	CAmount nBalance = 0;
	if(param3.get_str() != "0")
		nBalance = AssetAmountFromValue(param3, theAsset.nPrecision);
	
	if(strPubData != stringFromVch(theAsset.vchPubData))
		theAsset.vchPubData = vchFromString(strPubData);
    else
        theAsset.vchPubData.clear();
    if(vchContract != theAsset.vchContract)
        theAsset.vchContract = vchContract;
    else
        theAsset.vchContract.clear();

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
	return syscointxfund_helper(theAsset.witnessAddress.ToString(), SYSCOIN_TX_VERSION_ASSET_UPDATE, vchWitness, vecSend);
}

UniValue assettransfer(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 3)
        throw runtime_error(
			"assettransfer [asset] [address] [witness]\n"
						"Transfer an asset you own to another address.\n"
						"<asset> Asset guid.\n"
						"<address> Address to transfer to.\n"
						"<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n"	);

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

    CWitnessAddress fromWitnessAddress;
    fromWitnessAddress.vchWitnessProgram = theAsset.witnessAddress.vchWitnessProgram;
    fromWitnessAddress.nVersion = theAsset.witnessAddress.nVersion;
	theAsset.ClearAsset();
    CScript scriptPubKey;
	theAsset.witnessAddress = CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex));

	vector<unsigned char> data;
	theAsset.Serialize(data);


	vector<CRecipient> vecSend;
    

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);
	return syscointxfund_helper(fromWitnessAddress.ToString(), SYSCOIN_TX_VERSION_ASSET_TRANSFER, vchWitness, vecSend);
}
UniValue assetsend(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 3)
        throw runtime_error(
            "assetsend [asset] [addressTo] [amount]\n"
            "Send an asset you own to another address.\n"
            "<asset> Asset guid.\n"
            "<addressTo> Address to transfer to.\n"
            "<amount> Quantity of asset to send.\n");
            
    UniValue output(UniValue::VARR);
    UniValue outputObj(UniValue::VOBJ);
    outputObj.pushKV("address", params[1].get_str());
    outputObj.pushKV("amount", params[2].get_str());
    output.push_back(outputObj);
    UniValue paramsFund(UniValue::VARR);
    paramsFund.push_back(params[0].get_int());
    paramsFund.push_back(output);
    paramsFund.push_back("");
    JSONRPCRequest requestMany;
    requestMany.params = paramsFund;
    return assetsendmany(requestMany);          
}
UniValue assetsendmany(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 3)
		throw runtime_error(
			"assetsend [asset] ([{\"address\":\"address\",\"amount\":amount},...] [witness]\n"
			"Send an asset you own to another address/address as an asset allocation. Maximimum recipients is 250.\n"
			"<asset> Asset guid.\n"
			"<address> Address to transfer to.\n"
			"<amount> Quantity of asset to send.\n"
			"<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n");
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

		UniValue receiverObj = receiver.get_obj();
		string toStr = find_value(receiverObj, "address").get_str();
        CTxDestination dest = DecodeDestination(toStr);
		if(!IsValidDestination(dest))
			throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2509 - " + _("Asset must be sent to a valid syscoin address"));

        UniValue detail = DescribeAddress(dest);
        if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
        string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
        unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();    
                		
		UniValue amountObj = find_value(receiverObj, "amount");
		if (amountObj.isNum() || amountObj.isStr()) {
			const CAmount &amount = AssetAmountFromValue(amountObj, theAsset.nPrecision);
			if (amount <= 0)
				throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
			theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(CWitnessAddress(witnessVersion,ParseHex(witnessProgramHex)), amount));
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

UniValue assetinfo(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || 1 != params.size())
        throw runtime_error("assetinfo <asset>\n"
                "Show stored values of a single asset and its.\n");

    const int &nAsset = params[0].get_int();
	UniValue oAsset(UniValue::VOBJ);

	CAsset txPos;
	if (passetdb == nullptr || !passetdb->ReadAsset(nAsset, txPos))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2511 - " + _("Failed to read from asset DB"));

	if(!BuildAssetJson(txPos, oAsset))
		oAsset.clear();
    return oAsset;
}
bool BuildAssetJson(const CAsset& asset, UniValue& oAsset)
{
    oAsset.pushKV("_id", (int)asset.nAsset);
    oAsset.pushKV("txid", asset.txHash.GetHex());
	oAsset.pushKV("publicvalue", stringFromVch(asset.vchPubData));
	oAsset.pushKV("address", asset.witnessAddress.ToString());
    oAsset.pushKV("contract", asset.vchContract.empty()? "" : "0x"+HexStr(asset.vchContract));
	oAsset.pushKV("balance", ValueFromAssetAmount(asset.nBalance, asset.nPrecision));
	oAsset.pushKV("total_supply", ValueFromAssetAmount(asset.nTotalSupply, asset.nPrecision));
	oAsset.pushKV("max_supply", ValueFromAssetAmount(asset.nMaxSupply, asset.nPrecision));
	oAsset.pushKV("update_flags", asset.nUpdateFlags);
	oAsset.pushKV("precision", (int)asset.nPrecision);
	return true;
}
bool AssetTxToJSON(const CTransaction& tx, UniValue &entry)
{
	CAsset asset(tx);
	if(asset.IsNull())
		return false;

	CAsset dbAsset;
	if (!GetAsset(asset.nAsset, dbAsset))
        dbAsset.nPrecision = asset.nPrecision;
    
    int nHeight = 0;
    uint256 hash_block;
    CBlockIndex* blockindex = nullptr;
    CTransactionRef txRef;
    if (GetTransaction(tx.GetHash(), txRef, Params().GetConsensus(), hash_block, blockindex) && blockindex)
        nHeight = blockindex->nHeight; 
        	

	entry.pushKV("txtype", assetFromTx(tx.nVersion));
	entry.pushKV("_id", (int)asset.nAsset);
    entry.pushKV("txid", tx.GetHash().GetHex());
    entry.pushKV("height", nHeight);
    
	if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || (!asset.vchPubData.empty() && dbAsset.vchPubData != asset.vchPubData))
		entry.pushKV("publicvalue", stringFromVch(asset.vchPubData));
        
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || (!asset.vchContract.empty() && dbAsset.vchContract != asset.vchContract))
        entry.pushKV("contract", "0x"+HexStr(asset.vchContract));
        
	if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || (!asset.witnessAddress.IsNull() && dbAsset.witnessAddress != asset.witnessAddress))
		entry.pushKV("address", asset.witnessAddress.ToString());

	if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || asset.nUpdateFlags != dbAsset.nUpdateFlags)
		entry.pushKV("update_flags", asset.nUpdateFlags);
              
	if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || asset.nBalance != dbAsset.nBalance)
		entry.pushKV("balance", ValueFromAssetAmount(asset.nBalance, dbAsset.nPrecision));
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE){
        entry.pushKV("total_supply", ValueFromAssetAmount(asset.nTotalSupply, dbAsset.nPrecision)); 
        entry.pushKV("precision", asset.nPrecision);  
    }         
     return true;
}
bool AssetTxToJSON(const CTransaction& tx, const CAsset& dbAsset, const int& nHeight, UniValue &entry)
{
    CAsset asset(tx);
    if(asset.IsNull() || dbAsset.IsNull())
        return false;

    entry.pushKV("txtype", assetFromTx(tx.nVersion));
    entry.pushKV("_id", (int)asset.nAsset);
    entry.pushKV("txid", tx.GetHash().GetHex());
    entry.pushKV("height", nHeight);

    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || (!asset.vchPubData.empty() && dbAsset.vchPubData != asset.vchPubData))
        entry.pushKV("publicvalue", stringFromVch(asset.vchPubData));
        
    if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE|| (!asset.vchContract.empty() && dbAsset.vchContract != asset.vchContract))
        entry.pushKV("contract", "0x"+HexStr(asset.vchContract));
        
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || (!asset.witnessAddress.IsNull() && dbAsset.witnessAddress != asset.witnessAddress))
        entry.pushKV("address", asset.witnessAddress.ToString());

    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || asset.nUpdateFlags != dbAsset.nUpdateFlags)
        entry.pushKV("update_flags", asset.nUpdateFlags);
              
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE || asset.nBalance != dbAsset.nBalance)
        entry.pushKV("balance", ValueFromAssetAmount(asset.nBalance, dbAsset.nPrecision));
    if (tx.nVersion == SYSCOIN_TX_VERSION_ASSET_ACTIVATE){
        entry.pushKV("total_supply", ValueFromAssetAmount(asset.nTotalSupply, dbAsset.nPrecision)); 
        entry.pushKV("precision", asset.nPrecision);  
    }  
    return true;
}
UniValue ValueFromAssetAmount(const CAmount& amount,int precision)
{
	if (precision < 0 || precision > 8)
		throw JSONRPCError(RPC_TYPE_ERROR, "Precision must be between 0 and 8");
	bool sign = amount < 0;
	int64_t n_abs = (sign ? -amount : amount);
	int64_t quotient = n_abs;
	int64_t divByAmount = 1;
	int64_t remainder = 0;
	string strPrecision = "0";
	if (precision > 0) {
		divByAmount = pow(10, precision);
		quotient = n_abs / divByAmount;
		remainder = n_abs % divByAmount;
		strPrecision = boost::lexical_cast<string>(precision);
	}

	return UniValue(UniValue::VSTR,
		strprintf("%s%d.%0" + strPrecision + "d", sign ? "-" : "", quotient, remainder));
}
CAmount AssetAmountFromValue(UniValue& value, int precision)
{
	if(precision < 0 || precision > 8)
		throw JSONRPCError(RPC_TYPE_ERROR, "Precision must be between 0 and 8");
	if (!value.isNum() && !value.isStr())
		throw JSONRPCError(RPC_TYPE_ERROR, "Amount is not a number or string");
	if (value.isStr() && value.get_str() == "-1") {
		value.setInt((int64_t)(MAX_ASSET / ((int)pow(10, precision))));
	}
	CAmount amount;
	if (!ParseFixedPoint(value.getValStr(), precision, &amount))
		throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
	if (!AssetRange(amount))
		throw JSONRPCError(RPC_TYPE_ERROR, "Amount out of range");
	return amount;
}
CAmount AssetAmountFromValueNonNeg(const UniValue& value, int precision)
{
	if (precision < 0 || precision > 8)
		throw JSONRPCError(RPC_TYPE_ERROR, "Precision must be between 0 and 8");
	if (!value.isNum() && !value.isStr())
		throw JSONRPCError(RPC_TYPE_ERROR, "Amount is not a number or string");
	CAmount amount;
	if (!ParseFixedPoint(value.getValStr(), precision, &amount))
		throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
	if (!AssetRange(amount))
		throw JSONRPCError(RPC_TYPE_ERROR, "Amount out of range");
	return amount;
}
bool AssetRange(const CAmount& amount, int precision)
{

	if (precision < 0 || precision > 8)
		throw JSONRPCError(RPC_TYPE_ERROR, "Precision must be between 0 and 8");
	bool sign = amount < 0;
	int64_t n_abs = (sign ? -amount : amount);
	int64_t quotient = n_abs;
	if (precision > 0) {
		int64_t divByAmount = pow(10, precision);
		quotient = n_abs / divByAmount;
	}
	if (!AssetRange(quotient))
		return false;
	return true;
}
bool CAssetDB::Flush(const AssetMap &mapAssets){
    if(mapAssets.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : mapAssets) {
        if(key.second.IsNull())
            batch.Erase(key.first);
        else
            batch.Write(key.first, key.second);
    }
    LogPrint(BCLog::SYS, "Flushing %d assets\n", mapAssets.size());
    return WriteBatch(batch);
}
bool CAssetDB::ScanAssets(const int count, const int from, const UniValue& oOptions, UniValue& oRes) {
	string strTxid = "";
	vector<CWitnessAddress > vecWitnessAddresses;
    uint32_t nAsset = 0;
	if (!oOptions.isNull()) {
		const UniValue &txid = find_value(oOptions, "txid");
		if (txid.isStr()) {
			strTxid = txid.get_str();
		}
		const UniValue &assetObj = find_value(oOptions, "asset");
		if (assetObj.isNum()) {
			nAsset = boost::lexical_cast<uint32_t>(assetObj.get_int());
		}

		const UniValue &owners = find_value(oOptions, "addresses");
		if (owners.isArray()) {
			const UniValue &ownersArray = owners.get_array();
			for (unsigned int i = 0; i < ownersArray.size(); i++) {
				const UniValue &owner = ownersArray[i].get_obj();
                const CTxDestination &dest = DecodeDestination(owner.get_str());
                UniValue detail = DescribeAddress(dest);
                if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
                    throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
                string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
                unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();   
				const UniValue &ownerStr = find_value(owner, "address");
				if (ownerStr.isStr()) 
					vecWitnessAddresses.push_back(CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)));
			}
		}
	}
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CAsset txPos;
	uint32_t key;
	int index = 0;
	while (pcursor->Valid()) {
		boost::this_thread::interruption_point();
		try {
			if (pcursor->GetKey(key) && (nAsset == 0 || nAsset != key)) {
				pcursor->GetValue(txPos);
				if (!strTxid.empty() && strTxid != txPos.txHash.GetHex())
				{
					pcursor->Next();
					continue;
				}
				if (!vecWitnessAddresses.empty() && std::find(vecWitnessAddresses.begin(), vecWitnessAddresses.end(), txPos.witnessAddress) == vecWitnessAddresses.end())
				{
					pcursor->Next();
					continue;
				}
				UniValue oAsset(UniValue::VOBJ);
				if (!BuildAssetJson(txPos, oAsset))
				{
					pcursor->Next();
					continue;
				}
				index += 1;
				if (index <= from) {
					pcursor->Next();
					continue;
				}
				oRes.push_back(oAsset);
				if (index >= count + from)
					break;
			}
			pcursor->Next();
		}
		catch (std::exception &e) {
			return error("%s() : deserialize error", __PRETTY_FUNCTION__);
		}
	}
	return true;
}
UniValue listassets(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 3 < params.size())
		throw runtime_error("listassets [count] [from] [{options}]\n"
			"scan through all assets.\n"
			"[count]          (numeric, optional, default=10) The number of results to return.\n"
			"[from]           (numeric, optional, default=0) The number of results to skip.\n"
			"[options]        (object, optional) A json object with options to filter results\n"
			"    {\n"
			"      \"txid\":txid					(string) Transaction ID to filter results for\n"
			"	   \"asset\":guid					(number) Asset GUID to filter.\n"
			"	   \"addresses\"			        (array) a json array with owners\n"
			"		[\n"
			"			{\n"
			"				\"address\":string		(string) Address to filter.\n"
			"			} \n"
			"			,...\n"
			"		]\n"
			"    }\n"
			+ HelpExampleCli("listassets", "0")
			+ HelpExampleCli("listassets", "10 10")
			+ HelpExampleCli("listassets", "0 0 '{\"addresses\":[{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"},{\"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"}]}'")
			+ HelpExampleCli("listassets", "0 0 '{\"asset\":3473733}'")
		);
	UniValue options;
	int count = 10;
	int from = 0;
	if (params.size() > 0) {
		count = params[0].get_int();
		if (count == 0) {
			count = 10;
		} else
		if (count < 0) {
			throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("'count' must be 0 or greater"));
		}
	}
	if (params.size() > 1) {
		from = params[1].get_int();
		if (from < 0) {
			throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("'from' must be 0 or greater"));
		}
	}
	if (params.size() > 2) {
		options = params[2];
	}

	UniValue oRes(UniValue::VARR);
	if (!passetdb->ScanAssets(count, from, options, oRes))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Scan failed"));
	return oRes;
}
bool CAssetIndexDB::ScanAssetIndex(int64_t page, const UniValue& oOptions, UniValue& oRes) {
    if(!pblockindexdb)
        return false;
    CAssetAllocationTuple assetTuple;
    uint32_t nAsset = 0;
    if (!oOptions.isNull()) {
        const UniValue &assetObj = find_value(oOptions, "asset");
        if (assetObj.isNum()) {
            nAsset = boost::lexical_cast<uint32_t>(assetObj.get_int());
        }
        else
            return false;

        const UniValue &addressObj = find_value(oOptions, "address");
        if (addressObj.isStr()) {
            const CTxDestination &dest = DecodeDestination(addressObj.get_str());
            UniValue detail = DescribeAddress(dest);
            if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
                throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
            string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
            unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();   
            assetTuple = CAssetAllocationTuple(nAsset, CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)));
        }
    }
    else
        return false;
    vector<uint256> vecTX;
    int64_t pageFound;
    bool scanAllocation = !assetTuple.IsNull();
    if(scanAllocation){
        if(!ReadAssetAllocationPage(pageFound))
            return true;
    }
    else{
        if(!ReadAssetPage(pageFound))
            return true;
    }
    if(pageFound < page)
        return false;
    // order by highest page first
    page = pageFound - page;
    if(scanAllocation){
        if(!ReadIndexTXIDs(assetTuple, page, vecTX))
            return false;
    }
    else{
        if(!ReadIndexTXIDs(nAsset, page, vecTX))
            return false;
    }
    // reverse order LIFO
    std::reverse(vecTX.begin(), vecTX.end());
    uint256 block_hash;
    for(const uint256& txid: vecTX){
        UniValue oObj(UniValue::VOBJ);
        if(!ReadPayload(txid, oObj))
            continue;
        if(pblockindexdb && pblockindexdb->ReadBlockHash(txid, block_hash)){
            oObj.pushKV("block_hash", block_hash.GetHex());        
        }
        else
            oObj.pushKV("block_hash", "");
           
        oRes.push_back(oObj);
    }
    
    return true;
}
UniValue getblockhashbytxid(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getblockhash txid\n"
            "\nReturns hash of block in best-block-chain at txid provided.\n"
            "\nArguments:\n"
            "1. txid         (string, required) A transaction that is in the block\n"
            "\nResult:\n"
            "\"hash\"         (string) The block hash\n"
            "\nExamples:\n"
            + HelpExampleCli("getblockhashbytxid", "dfc7eac24fa89b0226c64885f7bedaf132fc38e8980b5d446d76707027254490")
            + HelpExampleRpc("getblockhashbytxid", "dfc7eac24fa89b0226c64885f7bedaf132fc38e8980b5d446d76707027254490")
        );
    LOCK(cs_main);

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    uint256 blockhash;
    if(!pblockindexdb || !pblockindexdb->ReadBlockHash(hash, blockhash))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block hash not found in asset index");

    const CBlockIndex* pblockindex = LookupBlockIndex(blockhash);
    if (!pblockindex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    return pblockindex->GetBlockHash().GetHex();
}
UniValue syscoingetspvproof(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "syscoingetspvproof txid (blockhash) \n"
            "\nReturns SPV proof for use with inter-chain transfers.\n"
            "\nArguments:\n"
            "1. txid         (string, required) A transaction that is in the block\n"
            "1. blockhash    (string, not-required) Block containing txid\n"
            "\nResult:\n"
        "\"proof\"         (string) JSON representation of merkl/ nj   nk ne proof (transaction index, siblings and block header and some other information useful for moving coins/assets to another chain)\n"
            "\nExamples:\n"
            + HelpExampleCli("syscoingetspvproof", "dfc7eac24fa89b0226c64885f7bedaf132fc38e8980b5d446d76707027254490")
            + HelpExampleRpc("syscoingetspvproof", "dfc7eac24fa89b0226c64885f7bedaf132fc38e8980b5d446d76707027254490")
        );
    LOCK(cs_main);
    UniValue res(UniValue::VOBJ);
    uint256 txhash = ParseHashV(request.params[0], "parameter 1");
    uint256 blockhash;
    if(request.params.size() > 1)
        blockhash = ParseHashV(request.params[1], "parameter 2");
    if(!pblockindexdb || !pblockindexdb->ReadBlockHash(txhash, blockhash))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block hash not found in asset index");
    
    CBlockIndex* pblockindex = LookupBlockIndex(blockhash);
    if (!pblockindex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }
    CTransactionRef tx;
    uint256 hash_block;
    if (!GetTransaction(txhash, tx, Params().GetConsensus(), hash_block, pblockindex))   
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not found"); 

    CBlock block;
    if (IsBlockPruned(pblockindex)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not available (pruned data)");
    }

    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        // Block not found on disk. This could be because we have the block
        // header in our index but don't have the block (for example if a
        // non-whitelisted node sends us an unrequested long chain of valid
        // blocks, we add the headers to our index, but don't accept the
        // block).
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
    }   
    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << pblockindex->GetBlockHeader(Params().GetConsensus());
    const std::string &rawTx = EncodeHexTx(CTransaction(*tx), PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
    res.pushKV("transaction",rawTx);
    // get first 80 bytes of header (non auxpow part)
    res.pushKV("header", HexStr(ssBlock.begin(), ssBlock.begin()+80));
    UniValue siblings(UniValue::VARR);
    // store the index of the transaction we are looking for within the block
    int nIndex = 0;
    for (unsigned int i = 0;i < block.vtx.size();i++) {
        const uint256 &txHashFromBlock = block.vtx[i]->GetHash();
        if(txhash == txHashFromBlock)
            nIndex = i;
        siblings.push_back(txHashFromBlock.GetHex());
    }
    res.pushKV("siblings", siblings);
    res.pushKV("index", nIndex);
    UniValue assetVal;
    try{
        UniValue paramsDecode(UniValue::VARR);
        paramsDecode.push_back(rawTx);   
        JSONRPCRequest requestDecodeRPC;
        requestDecodeRPC.params = paramsDecode;
        UniValue resDecode = syscoindecoderawtransaction(requestDecodeRPC);
        assetVal = find_value(resDecode.get_obj(), "asset"); 
    }
    catch(const runtime_error& e){
    }
    if(!assetVal.isNull()) {
        CAsset asset;
        if (!GetAsset(assetVal.get_int(), asset))
             throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("Asset not found"));
        if(asset.vchContract.empty())
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("Asset contract is empty"));
         res.pushKV("contract", HexStr(asset.vchContract));    
                   
    }
    else{
        res.pushKV("contract", HexStr(Params().GetConsensus().vchSYSXContract));
    }
    return res;
}
UniValue listassetindex(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 2 != params.size())
        throw runtime_error("listassetindex [page] [options]\n"
            "Scan through all asset index and return paged results based on page number passed in. Requires assetindex config parameter enabled and optional assetindexpagesize which is 25 by default.\n"
            "[page]           (numeric, default=0) Return specific page number of transactions. Lower page number means more recent transactions.\n"
            "[options]        (array, required) A json object with options to filter results\n"
            "    {\n"
            "      \"asset\":guid                   (number) Asset GUID to filter.\n"
            "      \"address\":string               (string, optional) Address to filter. Leave empty to scan globally through asset.\n"
            "    }\n"
            + HelpExampleCli("listassetindex", "0 '{\"asset\":92922}'")
            + HelpExampleCli("listassetindex", "2 '{\"asset\":92922, \"address\":\"sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7\"}'")
        );
    if(!fAssetIndex){
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("You must start syscoin with -assetindex enabled"));
    }
    UniValue options;
    int64_t page = params[0].get_int64();
   
    if (page < 0) {
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("'page' must be 0 or greater"));
    }

    options = params[1];
    
    UniValue oRes(UniValue::VARR);
    if (!passetindexdb->ScanAssetIndex(page, options, oRes))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("Scan failed"));
    return oRes;
}
UniValue listassetindexassets(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 1 != params.size())
        throw runtime_error("listassetindexassets [address]\n"
            "Return a list of assets an address is associated with.\n"
            "[asset]          (numeric, required) Address to find assets associated with.\n"
            + HelpExampleCli("listassetindex", "sys1qw40fdue7g7r5ugw0epzk7xy24tywncm26hu4a7")
        );
    if(!fAssetIndex){
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("You must start syscoin with -assetindex enabled"));
    }       
    const CTxDestination &dest = DecodeDestination(params[0].get_str());
    UniValue detail = DescribeAddress(dest);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();   
 
    UniValue oRes(UniValue::VARR);
    std::vector<uint32_t> assetGuids;
    if (!passetindexdb->ReadAssetsByAddress(CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex)), assetGuids))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 1510 - " + _("Lookup failed"));
        
    for(const uint32_t& guid: assetGuids){
        UniValue oObj(UniValue::VOBJ);
        oObj.pushKV("asset", (int)guid);
        oRes.push_back(oObj);
    }
    return oRes;
}
UniValue syscoinstopgeth(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 0 != params.size())
        throw runtime_error("syscoinstopgeth\n"
            "Stops Geth and the relayer from running.\n");
    if(!StopRelayerNode(relayerPID))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Could not stop relayer"));
    if(!StopGethNode(gethPID))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Could not stop Geth"));
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("status", "success");
    return ret;
}
UniValue syscoinstartgeth(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 0 != params.size())
        throw runtime_error("syscoinstartgeth\n"
            "Starts Geth and the relayer.\n");
    
    StopRelayerNode(relayerPID);
    StopGethNode(gethPID);
    int wsport = gArgs.GetArg("-gethwebsocketport", 8546);
    bool bGethTestnet = gArgs.GetBoolArg("-gethtestnet", false);
    if(!StartGethNode(gethPID, bGethTestnet, wsport))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Could not start Geth"));
    int rpcport = gArgs.GetArg("-rpcport", BaseParams().RPCPort());
    const std::string& rpcuser = gArgs.GetArg("-rpcuser", "u");
    const std::string& rpcpassword = gArgs.GetArg("-rpcpassword", "p");
    if(!StartRelayerNode(relayerPID, rpcport, rpcuser, rpcpassword, wsport))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Could not stop relayer"));
    
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("status", "success");
    return ret;
}
UniValue syscoinsetethstatus(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 2 != params.size())
        throw runtime_error("syscoinsetethstatus [syncing_status] [highestBlock]\n"
            "Sets ethereum syncing and network status for indication status of network sync.\n"
            "[syncing_status]      Syncing status either 'syncing' or 'synced'.\n"
            "[highestBlock]        What the highest block height on Ethereum is found to be. Usually coupled with syncing_status of 'syncing'. Set to 0 if syncing_status is 'synced'.\n" 
            + HelpExampleCli("syscoinsetethstatus", "syncing 7000000")
            + HelpExampleCli("syscoinsetethstatus", "synced 0"));
    string status = params[0].get_str();
    int highestBlock = params[1].get_int();
    
    if(highestBlock > 0){
        LOCK(cs_ethsyncheight);
        fGethSyncHeight = highestBlock;
    }
    fGethSyncStatus = status; 
    if(!fGethSynced && fGethCurrentHeight >= fGethSyncHeight)       
        fGethSynced = fGethSyncStatus == "synced";

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("status", "success");
    return ret;
}
UniValue syscoinsetethheaders(const JSONRPCRequest& request) {
    const UniValue &params = request.params;
    if (request.fHelp || 1 != params.size())
        throw runtime_error("syscoinsetethheaders [headers]\n"
            "Sets Ethereum headers in Syscoin to validate transactions through the SYSX bridge.\n"
            "[headers]         A JSON objects representing an array of arrays (block number, tx root) from Ethereum blockchain.\n"
            + HelpExampleCli("syscoinsetethheaders", "\"[[7043888,\\\"0xd8ac75c7b4084c85a89d6e28219ff162661efb8b794d4b66e6e9ea52b4139b10\\\"],...]\""));  

    EthereumTxRootMap txRootMap;       
    const UniValue &headerArray = params[0].get_array();
    for(size_t i =0;i<headerArray.size();i++){
        const UniValue &tupleArray = headerArray[i].get_array();
        if(tupleArray.size() != 2)
            throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2512 - " + _("Invalid size in a blocknumber/txroot tuple, should be size of 2"));
        uint32_t nHeight = (uint32_t)tupleArray[0].get_int();
        {
            LOCK(cs_ethsyncheight);
            if(nHeight > fGethSyncHeight)
                fGethSyncHeight = nHeight;
        }
        if(nHeight > fGethCurrentHeight)
            fGethCurrentHeight = nHeight;
        string txRoot = tupleArray[1].get_str();
        boost::erase_all(txRoot, "0x");  // strip 0x
        const vector<unsigned char> &vchTxRoot = ParseHex(txRoot);
        txRootMap.emplace(std::piecewise_construct,  std::forward_as_tuple(nHeight),  std::forward_as_tuple(vchTxRoot));
    } 
    bool res = pethereumtxrootsdb->FlushWrite(txRootMap) && pethereumtxrootsdb->PruneTxRoots();
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("status", res? "success": "fail");
    return ret;
}
bool CEthereumTxRootsDB::PruneTxRoots() {
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    vector<uint32_t> vecHeightKeys;
    uint32_t key;
    int32_t cutoffHeight;
    {
        LOCK(cs_ethsyncheight);
        // cutoff to keep blocks is ~3 week of blocks is about 120k blocks
        cutoffHeight = fGethSyncHeight - (MAX_ETHEREUM_TX_ROOTS*3);
        if(cutoffHeight < 0){
            LogPrint(BCLog::SYS, "Nothing to prune fGethSyncHeight = %d\n", fGethSyncHeight);
            return true;
        }
    }
    std::vector<unsigned char> txPos;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
       
            if (pcursor->GetKey(key) && key < (uint32_t)cutoffHeight) {
                vecHeightKeys.emplace_back(std::move(key));
            }
            pcursor->Next();
        }
        catch (std::exception &e) {
            return error("%s() : deserialize error", __PRETTY_FUNCTION__);
        }
    }
    {
        LOCK(cs_ethsyncheight);
        WriteHighestHeight(fGethSyncHeight);
    }
    
    WriteCurrentHeight(fGethCurrentHeight);      
    FlushErase(vecHeightKeys);
    return true;
}
bool CEthereumTxRootsDB::Init(){
    bool highestHeight = false;
    {
        LOCK(cs_ethsyncheight);
        highestHeight = ReadHighestHeight(fGethSyncHeight);
    }
    return highestHeight && ReadCurrentHeight(fGethCurrentHeight);
    
}
bool CAssetIndexDB::FlushErase(const std::vector<uint256> &vecTXIDs){
    if(vecTXIDs.empty() || !fAssetIndex)
        return true;

    CDBBatch batch(*this);
    for (const uint256 &txid : vecTXIDs) {
        // erase payload
        batch.Erase(txid);
    }
    LogPrint(BCLog::SYS, "Flushing %d asset index removals\n", vecTXIDs.size());
    return WriteBatch(batch);
}
bool CEthereumTxRootsDB::FlushErase(const std::vector<uint32_t> &vecHeightKeys){
    if(vecHeightKeys.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : vecHeightKeys) {
        batch.Erase(key);
    }
    LogPrint(BCLog::SYS, "Flushing, erasing %d ethereum tx roots\n", vecHeightKeys.size());
    return WriteBatch(batch);
}
bool CEthereumTxRootsDB::FlushWrite(const EthereumTxRootMap &mapTxRoots){
    if(mapTxRoots.empty())
        return true;
    CDBBatch batch(*this);
    for (const auto &key : mapTxRoots) {
        batch.Write(key.first, key.second);
    }
    LogPrint(BCLog::SYS, "Flushing, writing %d ethereum tx roots\n", mapTxRoots.size());
    return WriteBatch(batch);
}
