// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "services/asset.h"
#include "services/assetallocation.h"
#include "init.h"
#include "validation.h"
#include "util.h"
#include "core_io.h"
#include "wallet/wallet.h"
#include "chainparams.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <chrono>
#include "wallet/coincontrol.h"
#include <rpc/util.h>
#include <key_io.h>
#include <policy/policy.h>
#include <consensus/validation.h>
#include <wallet/fees.h>
#include <outputtype.h>
#include <boost/thread.hpp>
extern AssetBalanceMap mempoolMapAssetBalances;
extern ArrivalTimesMapImpl arrivalTimesMap;
unsigned int MAX_UPDATES_PER_BLOCK = 2;
std::unique_ptr<CAssetDB> passetdb;
std::unique_ptr<CAssetAllocationDB> passetallocationdb;
std::unique_ptr<CAssetAllocationMempoolDB> passetallocationmempooldb;
std::unique_ptr<CEthereumTxRootsDB> pethereumtxrootsdb;
// SYSCOIN service rpc functions
UniValue syscoinburn(const JSONRPCRequest& request);
UniValue syscoinmint(const JSONRPCRequest& request);
UniValue syscointxfund(const JSONRPCRequest& request);


UniValue syscoinlistreceivedbyaddress(const JSONRPCRequest& request);
extern UniValue sendrawtransaction(const JSONRPCRequest& request);
UniValue syscoindecoderawtransaction(const JSONRPCRequest& request);

UniValue syscoinaddscript(const JSONRPCRequest& request);
UniValue assetnew(const JSONRPCRequest& request);
UniValue assetupdate(const JSONRPCRequest& request);
UniValue addressbalance(const JSONRPCRequest& request);
UniValue assettransfer(const JSONRPCRequest& request);
UniValue assetsend(const JSONRPCRequest& request);
UniValue assetinfo(const JSONRPCRequest& request);
UniValue listassets(const JSONRPCRequest& request);
UniValue syscoinsetethstatus(const JSONRPCRequest& request);
UniValue syscoinsetethheaders(const JSONRPCRequest& request);

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
    UniValue param(UniValue::VOBJ);
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
bool GetSyscoinData(const CTransaction &tx, vector<unsigned char> &vchData, int& nOut, int &op)
{
	nOut = GetSyscoinDataOutput(tx);
	if (nOut == -1)
		return false;

	const CScript &scriptPubKey = tx.vout[nOut].scriptPubKey;
	return GetSyscoinData(scriptPubKey, vchData, op);
}
bool GetSyscoinData(const CScript &scriptPubKey, vector<unsigned char> &vchData, int &op)
{
	op = 0;
	CScript::const_iterator pc = scriptPubKey.begin();
	opcodetype opcode;
	if (!scriptPubKey.GetOp(pc, opcode))
		return false;
	if (opcode != OP_RETURN)
		return false;
	if (!scriptPubKey.GetOp(pc, opcode))
		return false;
	if (opcode < OP_1 || opcode > OP_16)
		return false;
	op = CScript::DecodeOP_N(opcode);
	if (!scriptPubKey.GetOp(pc, opcode, vchData))
		return false;
	return true;
}



string assetFromOp(int op) {
    switch (op) {
    case OP_ASSET_ACTIVATE:
        return "assetactivate";
    case OP_ASSET_UPDATE:
        return "assetupdate";
    case OP_ASSET_TRANSFER:
        return "assettransfer";
	case OP_ASSET_SEND:
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
	int nOut, op;
	if (!GetSyscoinData(tx, vchData, nOut, op) || op != OP_SYSCOIN_ASSET)
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
    int nOut, op;
    if (!GetSyscoinData(tx, vchData, nOut, op) || op != OP_SYSCOIN_MINT)
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
    int op;
    char type;
    int count = 0;
    for (const txiter& it : stage) {
        const CTransaction& tx = it->GetTx();
        if(tx.nVersion == SYSCOIN_TX_VERSION_ASSET && DecodeAndParseAssetAllocationTx(tx, op, vvch, type)){
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
bool DecodeAndParseSyscoinTx(const CTransaction& tx, int& op,
	vector<vector<unsigned char> >& vvch, char& type)
{
	return
		DecodeAndParseAssetAllocationTx
        (tx, op, vvch, type)
		|| DecodeAndParseAssetTx(tx, op, vvch, type);
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
void CreateAssetRecipient(const CScript& scriptPubKey, CRecipient& recipient)
{
	CRecipient recp = { scriptPubKey, recipient.nAmount, false };
	recipient = recp;
	const CAmount &minFee = GetFee(3000);
	recipient.nAmount = minFee;
}
void CreateRecipient(const CScript& scriptPubKey, CRecipient& recipient)
{
	CRecipient recp = { scriptPubKey, recipient.nAmount, false };
	recipient = recp;
	CTxOut txout(recipient.nAmount, scriptPubKey);
	size_t nSize = GetSerializeSize(txout, SER_DISK, 0) + 148u;
	recipient.nAmount = GetFee(nSize);
}
void CreateFeeRecipient(CScript& scriptPubKey, CRecipient& recipient)
{
	CRecipient recp = { scriptPubKey, 0, false };
	recipient = recp;
}
UniValue SyscoinListReceived(bool includeempty = true, bool includechange = false)
{
	map<string, int> mapAddress;
	UniValue ret(UniValue::VARR);
	CWallet* const pwallet = GetDefaultWallet();
    if(!pwallet)
        return ret;
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
		LOCK(pwallet->cs_wallet);
		pwallet->AvailableCoins(vecOutputs, true, nullptr, 1, MAX_MONEY, MAX_MONEY, 0, 0, 9999999, ALL_COINS);
	}
	for(const COutput& out: vecOutputs) {
		CTxDestination address;
		if (!ExtractDestination(out.tx->tx->vout[out.i].scriptPubKey, address))
			continue;

		const string& strAddress = EncodeDestination(address);
		if (mapAddress.find(strAddress) != mapAddress.end())
			continue;

		UniValue paramsBalance(UniValue::VARR);
		UniValue param(UniValue::VOBJ);
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
UniValue syscointxfund_helper(const string &vchWitness, vector<CRecipient> &vecSend, const int nVersion) {
	CMutableTransaction txNew;
	txNew.nVersion = nVersion;

	COutPoint witnessOutpoint;
	if (!vchWitness.empty() && vchWitness != "''")
	{
		string strWitnessAddress;
		strWitnessAddress = vchWitness;
		addressunspent(strWitnessAddress, witnessOutpoint);
		if (witnessOutpoint.IsNull())
		{
			throw runtime_error("SYSCOIN_RPC_ERROR ERRCODE: 9000 - " + _("This transaction requires a witness but not enough outputs found for witness address: ") + strWitnessAddress);
		}
		Coin pcoinW;
		if (GetUTXOCoin(witnessOutpoint, pcoinW))
			txNew.vin.push_back(CTxIn(witnessOutpoint, pcoinW.out.scriptPubKey));
	}

	// vouts to the payees
	for (const auto& recipient : vecSend)
	{
		CTxOut txout(recipient.nAmount, recipient.scriptPubKey);
		if (!IsDust(txout, dustRelayFee))
		{
			txNew.vout.push_back(txout);
		}
	}   

	UniValue paramsFund(UniValue::VARR);
	paramsFund.push_back(EncodeHexTx(txNew));
	return paramsFund;
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
			"\nFunds a new syscoin transaction with inputs used from wallet or an array of addresses specified.\n"
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
 
    CRecipient addressRecipient;
    CScript scriptPubKeyFromOrig = GetScriptForDestination(DecodeDestination(strAddress));
    CreateAssetRecipient(scriptPubKeyFromOrig, addressRecipient);  
    addressArray.push_back("addr(" + strAddress + ")"); 
    
    
    CTransaction txIn_t(tx);    
 
    
    // add total output amount of transaction to desired amount
    CAmount nDesiredAmount = txIn_t.GetValueOut();
    CAmount nCurrentAmount = 0;

    LOCK(cs_main);
    CCoinsViewCache view(pcoinsTip.get());
    // get value of inputs
    nCurrentAmount = view.GetValueIn(txIn_t);

    // # vin (with IX)*FEE + # vout*FEE + (10 + # vin)*FEE + 34*FEE (for change output)
    CAmount nFees = GetFee(10 + 34);

    for (auto& vin : tx.vin) {
        Coin coin;
        if (!GetUTXOCoin(vin.prevout, coin))
            continue;
        int numSigs = 0;
        CCountSigsVisitor(*pwallet, numSigs).Process(coin.out.scriptPubKey);
        nFees += GetFee(numSigs * 200);
    }
    for (auto& vout : tx.vout) {
        const unsigned int nBytes = ::GetSerializeSize(vout, SER_NETWORK, PROTOCOL_VERSION);
        nFees += GetFee(nBytes);
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

    const CAmount &minFee = GetFee(3000);
    if (nCurrentAmount < (nDesiredAmount + nFees)) {
        // only look for small inputs if addresses were passed in, if looking through wallet we do not want to fund via small inputs as we may end up spending small inputs inadvertently
        if ((tx.nVersion == SYSCOIN_TX_VERSION_ASSET ||  tx.nVersion == SYSCOIN_TX_VERSION_MINT_SYSCOIN ||  tx.nVersion == SYSCOIN_TX_VERSION_MINT_ASSET) && params.size() > 1) {
            LOCK(mempool.cs);
            int countInputs = 0;
            // fund with small inputs first
            for (unsigned int i = 0; i < utxoArray.size(); i++)
            {
                bool bOutputIndex = i == output_index || output_index < 0;
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
                // look for small inputs only, if not selecting all
                if (nValue <= minFee || (output_index >= 0 && output_index == i)) {

                    if (mempool.mapNextTx.find(outPoint) != mempool.mapNextTx.end())
                        continue;
                    {
                        LOCK(pwallet->cs_wallet);
                        if (pwallet->IsLockedCoin(txid, nOut))
                            continue;
                    }
                    if (!IsOutpointMature(outPoint))
                        continue;
                    int numSigs = 0;
                    CCountSigsVisitor(*pwallet, numSigs).Process(scriptPubKey);
                    // add fees to account for every input added to this transaction
                    nFees += GetFee(numSigs * 200);
                    tx.vin.push_back(txIn);
                    countInputs++;
                    nCurrentAmount += nValue;
                    if (nCurrentAmount >= (nDesiredAmount + nFees) || (output_index >= 0 && output_index == i)) {
                        break;
                    }
                }
            }
            if (countInputs <= 0 && !fTPSTestEnabled && tx.nVersion != SYSCOIN_TX_VERSION_MINT_SYSCOIN && tx.nVersion != SYSCOIN_TX_VERSION_MINT_ASSET)
            {
                for (unsigned int i = 0; i < MAX_UPDATES_PER_BLOCK; i++){
                    nDesiredAmount += addressRecipient.nAmount;
                    CTxOut out(addressRecipient.nAmount, addressRecipient.scriptPubKey);
                    const unsigned int nBytes = ::GetSerializeSize(out, SER_NETWORK, PROTOCOL_VERSION);
                    nFees += GetFee(nBytes);
                    tx.vout.push_back(out);
                }
            }
        }   
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
                // look for bigger inputs
                if (nValue <= minFee)
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
				int numSigs = 0;
				CCountSigsVisitor(*pwallet, numSigs).Process(scriptPubKey);
				// add fees to account for every input added to this transaction
				nFees += GetFee(numSigs * 200);
				tx.vin.push_back(txIn);
				nCurrentAmount += nValue;
				if (nCurrentAmount >= (nDesiredAmount + nFees)) {
					break;
				}
			}
		}
	}
    
  
	const CAmount &nChange = nCurrentAmount - nDesiredAmount - nFees;
	if (nChange < 0)
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5502 - " + _("Insufficient funds"));
        
    // change back to funding address
	const CTxDestination & dest = DecodeDestination(strAddress);
	if (!IsValidDestination(dest))
		throw runtime_error("Change address is not valid");
	CTxOut changeOut(nChange, GetScriptForDestination(dest));
	if (!IsDust(changeOut, dustRelayFee))
		tx.vout.push_back(changeOut);
	
    
	// pass back new raw transaction
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(tx));
	return res;
}
UniValue syscoinburn(const JSONRPCRequest& request) {
	std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
	CWallet* const pwallet = wallet.get();
	const UniValue &params = request.params;
	if (request.fHelp || 3 != params.size())
		throw runtime_error(
			"syscoinburn [amount] [burn_to_sysx] [ethereum_destination_address]\n"
			"<amount> Amount of SYS to burn. Note that fees are applied on top. It is not inclusive of fees.\n"
			"<burn_to_sysx> Boolean. Set to true if you are provably burning SYS to go to SYSX. False if you are provably burning SYS forever.\n"
            "<ethereum_destination_address> The 20 byte (40 character) hex string of the ethereum destination address.\n"
			+ HelpRequiringPassphrase(pwallet));
            
	CAmount nAmount = AmountFromValue(params[0]);
	bool bBurnToSYSX = params[1].get_bool();
    string ethAddress = params[2].get_str();
    boost::erase_all(ethAddress, "0x");  // strip 0x if exist

   
	vector<CRecipient> vecSend;
	CScript scriptData;
	scriptData << OP_RETURN;
	if (bBurnToSYSX){
		scriptData << OP_TRUE << ParseHex(ethAddress);
    }
	else
		scriptData << OP_FALSE;

	CMutableTransaction txNew;
	CTxOut txout(nAmount, scriptData);
	txNew.vout.push_back(txout);
       

	UniValue paramsFund(UniValue::VARR);
	paramsFund.push_back(EncodeHexTx(txNew));
	return paramsFund;
}
UniValue syscoinmint(const JSONRPCRequest& request) {
	std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
	CWallet* const pwallet = wallet.get();
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
            "<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n"
			+ HelpRequiringPassphrase(pwallet));

	string vchAddress = params[0].get_str();
	CAmount nAmount = AmountFromValue(params[1]);
    uint32_t nBlockNumber = (uint32_t)params[2].get_int();
    string vchValue = params[3].get_str();
    string vchTxRoot = params[4].get_str();
    string vchParentNodes = params[5].get_str();
    string vchPath = params[6].get_str();
    string strWitnessAddress = params[7].get_str();
    
	vector<CRecipient> vecSend;
	const CTxDestination &dest = DecodeDestination(vchAddress);
    
	CScript scriptPubKeyFromOrig = GetScriptForDestination(dest);

	CMutableTransaction txNew;
	txNew.nVersion = SYSCOIN_TX_VERSION_MINT_SYSCOIN;
	txNew.vout.push_back(CTxOut(nAmount, scriptPubKeyFromOrig));
    
    CMintSyscoin mintSyscoin;
    mintSyscoin.vchValue = ParseHex(vchValue);
    mintSyscoin.vchTxRoot = ParseHex(vchTxRoot);
    mintSyscoin.nBlockNumber = nBlockNumber;
    mintSyscoin.vchParentNodes = ParseHex(vchParentNodes);
    mintSyscoin.vchPath = ParseHex(vchPath);
    
    vector<unsigned char> data;
    mintSyscoin.Serialize(data);
    
    CScript scriptData;
    scriptData << OP_RETURN << CScript::EncodeOP_N(OP_SYSCOIN_MINT) << data;
    
    CTxOut txout(0, scriptData);
    txNew.vout.push_back(txout);
 
    COutPoint witnessOutpoint;
    if (!strWitnessAddress.empty() && strWitnessAddress != "''")
    {
        addressunspent(strWitnessAddress, witnessOutpoint);
        if (witnessOutpoint.IsNull())
        {
            throw runtime_error("SYSCOIN_RPC_ERROR ERRCODE: 9000 - " + _("This transaction requires a witness but not enough outputs found for witness address: ") + strWitnessAddress);
        }
        Coin pcoinW;
        if (GetUTXOCoin(witnessOutpoint, pcoinW))
            txNew.vin.push_back(CTxIn(witnessOutpoint, pcoinW.out.scriptPubKey));
    }    
    
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(txNew));
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
bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, UniValue& output){
    int op;
    vector<vector<unsigned char> > vvch;
    char ctype;
    bool found = false;
    if (rawTx.nVersion == SYSCOIN_TX_VERSION_ASSET && DecodeAndParseSyscoinTx(rawTx, op, vvch, ctype)){
        found = SysTxToJSON(op, rawTx, output, ctype);
   }
    else if(rawTx.nVersion == SYSCOIN_TX_VERSION_MINT_ASSET){
        found = AssetMintTxToJson(rawTx, output);
    }

    return found;
}
bool SysTxToJSON(const int &op, const CTransaction &tx, UniValue &entry, const char& type)
{
    bool found = false;
	if (type == OP_SYSCOIN_ASSET)
		found = AssetTxToJSON(op, tx, entry);
	else if (type == OP_SYSCOIN_ASSET_ALLOCATION)
		found = AssetAllocationTxToJSON(op, tx, entry);
    return found;
}
int GenerateSyscoinGuid()
{
    int rand = 0;
    while(rand <= SYSCOIN_TX_VERSION_MINT_ASSET)
	    rand = GetRand(std::numeric_limits<int>::max());
    return rand;
}
unsigned int addressunspent(const string& strAddressFrom, COutPoint& outpoint)
{
	UniValue paramsUTXO(UniValue::VARR);
	UniValue param(UniValue::VOBJ);
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
		const CAmount &minFee = GetFee(3000);
		for (unsigned int i = 0; i < utxoArray.size(); i++)
		{
			const UniValue& utxoObj = utxoArray[i].get_obj();
			const uint256& txid = uint256S(find_value(utxoObj, "txid").get_str());
			const int& nOut = find_value(utxoObj, "vout").get_int();
			const CAmount &nValue = AmountFromValue(find_value(utxoObj, "amount"));
			if (nValue > minFee)
				continue;
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
UniValue syscoinaddscript(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || 1 != params.size())
		throw runtime_error("syscoinaddscript redeemscript\n"
			"Add redeemscript to local wallet for signing smart contract based syscoin transactions.\n");
    CWallet* const pwallet = GetDefaultWallet();
	std::vector<unsigned char> data(ParseHex(params[0].get_str()));
    if(pwallet)
	    pwallet->AddCScript(CScript(data.begin(), data.end()));
	UniValue res(UniValue::VOBJ);
	res.pushKV("result", "success");
	return res;
}
UniValue syscoinlistreceivedbyaddress(const JSONRPCRequest& request)
{
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

	return SyscoinListReceived(true, false);
}
string GetSyscoinTransactionDescription(const CTransaction& tx, const int op, string& responseEnglish, const char &type, string& responseGUID)
{
	if (tx.IsNull()) {
		return "Null Tx";
	}
	string strResponse = "";

	if (type == OP_SYSCOIN_ASSET) {
		// message from op code
		if (op == OP_ASSET_ACTIVATE) {
			strResponse = _("Asset Activated");
			responseEnglish = "Asset Activated";
		}
		else if (op == OP_ASSET_UPDATE) {
			strResponse = _("Asset Updated");
			responseEnglish = "Asset Updated";
		}
		else if (op == OP_ASSET_TRANSFER) {
			strResponse = _("Asset Transferred");
			responseEnglish = "Asset Transferred";
		}
		else if (op == OP_ASSET_SEND) {
			strResponse = _("Asset Sent");
			responseEnglish = "Asset Sent";
		}
		if (op == OP_ASSET_SEND) {
			CAssetAllocation assetallocation(tx);
			if (!assetallocation.assetAllocationTuple.IsNull()) {
				responseGUID = assetallocation.assetAllocationTuple.ToString();
			}
		}
		else {
			CAsset asset(tx);
			if (!asset.IsNull()) {
				responseGUID = boost::lexical_cast<string>(asset.nAsset);
			}
		}
	}
	else if (type == OP_SYSCOIN_ASSET_ALLOCATION) {
		// message from op code
		if (op == OP_ASSET_ALLOCATION_SEND) {
			strResponse = _("Asset Allocation Sent");
			responseEnglish = "Asset Allocation Sent";
            CAssetAllocation assetallocation(tx);
            if (!assetallocation.assetAllocationTuple.IsNull()) {
                responseGUID = assetallocation.assetAllocationTuple.ToString();
            } 
		}
		else if (op == OP_ASSET_ALLOCATION_BURN) {
			strResponse = _("Asset Allocation Burned");
			responseEnglish = "Asset Allocation Burned";
            CAssetAllocation assetallocation(tx);
            if (!assetallocation.assetAllocationTuple.IsNull()) {
                responseGUID = assetallocation.assetAllocationTuple.ToString();
            }            
		}
	}
	else {
		strResponse = _("Unknown Op Type");
		responseEnglish = "Unknown Op Type";
		return strResponse + " " + string(1, type);
	}
	return strResponse + " " + responseGUID;
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
void CAssetDB::WriteAssetIndex(const CTransaction& tx, const CAsset& dbAsset, const int& op, const int& nHeight) {
	if (fZMQAsset) {
		UniValue oName(UniValue::VOBJ);
        if(AssetTxToJSON(op, tx, dbAsset, nHeight, oName))
            GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "assetrecord");
	}
}
bool GetAsset(const int &nAsset,
        CAsset& txPos) {
    if (passetdb == nullptr || !passetdb->ReadAsset(nAsset, txPos))
        return false;
    return true;
}
bool DecodeAndParseAssetTx(const CTransaction& tx, int& op,
		vector<vector<unsigned char> >& vvch, char &type)
{
	if (op == OP_ASSET_SEND)
		return false;
	CAsset asset;
	bool decode = DecodeAssetTx(tx, op, vvch);
	bool parse = asset.UnserializeFromTx(tx);
	if (decode&&parse) {
		type = OP_SYSCOIN_ASSET;
		return true;
	}
	return false;
}
bool DecodeAssetTx(const CTransaction& tx, int& op,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeAssetScript(out.scriptPubKey, op, vvchRead)) {
            found = true; vvch = vvchRead;
            break;
        }
    }
    if (!found) vvch.clear();
    return found;
}


bool DisconnectAssetSend(const CTransaction &tx, AssetMap &mapAssets, AssetAllocationMap &mapAssetAllocations){

    CAsset dbAsset;
    CAssetAllocation theAssetAllocation(tx);
    if(theAssetAllocation.assetAllocationTuple.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not decode asset allocation in asset send\n");
        return false;
    } 
    auto result  = mapAssets.try_emplace(theAssetAllocation.assetAllocationTuple.nAsset, std::move(emptyAsset));
    auto mapAsset = result.first;
    const bool& mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        if (!GetAsset(theAssetAllocation.assetAllocationTuple.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not get asset %d\n",theAssetAllocation.assetAllocationTuple.nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                   
    }
    CAsset& storedSenderRef = mapAsset->second;
               
               
    for(const auto& amountTuple:theAssetAllocation.listSendingAllocationAmounts){
        const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
        const std::string &receiverTupleStr = receiverAllocationTuple.ToString();
        CAssetAllocation receiverAllocation;
        auto result = mapAssetAllocations.try_emplace(std::move(receiverTupleStr), std::move(emptyAllocation));
        auto mapAssetAllocation = result.first;
        const bool &mapAssetAllocationNotFound = result.second;
        if(mapAssetAllocationNotFound){
            GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
            if (receiverAllocation.assetAllocationTuple.IsNull()) {
                receiverAllocation.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                receiverAllocation.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);
            } 
            mapAssetAllocation->second = std::move(receiverAllocation);                 
        }
        CAssetAllocation& storedReceiverAllocationRef = mapAssetAllocation->second;
                    

        // reverse allocation
        if(storedReceiverAllocationRef.nBalance >= amountTuple.second){
            storedReceiverAllocationRef.nBalance -= amountTuple.second;
            storedSenderRef.nBalance += amountTuple.second;
        } 
        if(storedReceiverAllocationRef.nBalance == 0){
            storedReceiverAllocationRef.SetNull();       
        }                                    
    }       
    return true;  
}
bool DisconnectAssetUpdate(const CTransaction &tx, AssetMap &mapAssets){
    CAsset dbAsset;
    CAsset theAsset(tx);
    if(theAsset.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not decode asset\n");
        return false;
    }
    auto result = mapAssets.try_emplace(theAsset.nAsset, std::move(emptyAsset));
    auto mapAsset = result.first;
    const bool &mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        if (!GetAsset(theAsset.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not get asset %d\n",theAsset.nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                   
    }
    CAsset& storedSenderRef = mapAsset->second;   
           
    if(theAsset.nBalance > 0){
        // reverse asset minting by the issuer
        storedSenderRef.nBalance -= theAsset.nBalance;
        storedSenderRef.nTotalSupply -= theAsset.nBalance;
        if(storedSenderRef.nBalance < 0 || storedSenderRef.nTotalSupply < 0) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Asset cannot be negative: Balance %lld, Supply: %lld\n",storedSenderRef.nBalance, storedSenderRef.nTotalSupply);
            return false;
        }                                          
    } 
        
      
    return true;  
}
bool DisconnectAssetActivate(const CTransaction &tx, AssetMap &mapAssets){
    CAsset theAsset(tx);
    
    if(theAsset.IsNull()){
        LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not decode asset in asset activate\n");
        return false;
    }
    auto result = mapAssets.try_emplace(theAsset.nAsset, std::move(emptyAsset));
    auto mapAsset = result.first;
    const bool &mapAssetNotFound = result.second;
    if(mapAssetNotFound){
        CAsset dbAsset;
        if (!GetAsset(theAsset.nAsset, dbAsset)) {
            LogPrint(BCLog::SYS,"DisconnectSyscoinTransaction: Could not get asset %d\n",theAsset.nAsset);
            return false;               
        } 
        mapAsset->second = std::move(dbAsset);                   
    }
    mapAsset->second.SetNull();  
    return true;  
}
bool CheckAssetInputs(const CTransaction &tx, const CCoinsViewCache &inputs, int op, const vector<vector<unsigned char> > &vvchArgs,
        bool fJustCheck, int nHeight, AssetMap& mapAssets, AssetAllocationMap &mapAssetAllocations, string &errorMessage, bool bSanityCheck) {
	if (passetdb == nullptr)
		return false;
	const uint256& txHash = tx.GetHash();
	if (!bSanityCheck)
		LogPrint(BCLog::SYS, "*** ASSET %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, txHash.ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");

	// unserialize asset from txn, check for valid
	CAsset theAsset;
	CAssetAllocation theAssetAllocation;
	vector<unsigned char> vchData;

	int nDataOut, type;
	if(!GetSyscoinData(tx, vchData, nDataOut, type) || (op != OP_ASSET_SEND && (type != OP_SYSCOIN_ASSET || !theAsset.UnserializeFromData(vchData))) || (op == OP_ASSET_SEND && (type != OP_SYSCOIN_ASSET_ALLOCATION || !theAssetAllocation.UnserializeFromData(vchData))))
	{
		errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR ERRCODE: 2000 - " + _("Cannot unserialize data inside of this transaction relating to an asset");
		return error(errorMessage.c_str());
	}
    

	if(fJustCheck)
	{
		if (op != OP_ASSET_SEND) {
			if (theAsset.vchPubData.size() > MAX_VALUE_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2004 - " + _("Asset public data too big");
				return error(errorMessage.c_str());
			}
		}
		switch (op) {
		case OP_ASSET_ACTIVATE:
			if (theAsset.nAsset <= SYSCOIN_TX_VERSION_MINT_ASSET)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("asset guid invalid");
				return error(errorMessage.c_str());
			}
            if (!theAsset.vchContract.empty() && theAsset.vchContract.size() != MAX_GUID_LENGTH)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Contract address not proper size");
                return error(errorMessage.c_str());
            }  
			if (theAsset.nPrecision > 8)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2013 - " + _("Precision must be between 0 and 8");
				return error(errorMessage.c_str());
			}
			if (theAsset.nMaxSupply != -1 && !AssetRange(theAsset.nMaxSupply, theAsset.nPrecision))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2014 - " + _("Max supply out of money range");
				return error(errorMessage.c_str());
			}
			if (theAsset.nBalance > theAsset.nMaxSupply)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Total supply cannot exceed maximum supply");
				return error(errorMessage.c_str());
			}
            if (!theAsset.witnessAddress.IsValid())
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Address specified is invalid");
                return error(errorMessage.c_str());
            }
            if(theAsset.nUpdateFlags > ASSET_UPDATE_ALL){
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Invalid update flags");
                return error(errorMessage.c_str());
            }          
			break;

		case OP_ASSET_UPDATE:
			if (theAsset.nBalance < 0)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2017 - " + _("Balance must be greator than or equal to 0");
				return error(errorMessage.c_str());
			}
            if (!theAssetAllocation.assetAllocationTuple.IsNull())
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2019 - " + _("Cannot update allocations");
                return error(errorMessage.c_str());
            }
            if (!theAsset.vchContract.empty() && theAsset.vchContract.size() != MAX_GUID_LENGTH)
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Contract address not proper size");
                return error(errorMessage.c_str());
            }  
            if(theAsset.nUpdateFlags > ASSET_UPDATE_ALL){
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Invalid update flags");
                return error(errorMessage.c_str());
            }           
			break;
            
		case OP_ASSET_SEND:
			if (theAssetAllocation.listSendingAllocationAmounts.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2020 - " + _("Asset send must send an input or transfer balance");
				return error(errorMessage.c_str());
			}
			if (theAssetAllocation.listSendingAllocationAmounts.size() > 250)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Too many receivers in one allocation send, maximum of 250 is allowed at once");
				return error(errorMessage.c_str());
			}
			break;
        case OP_ASSET_TRANSFER:
            break;
		default:
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2023 - " + _("Asset transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}
	if (!fJustCheck || bSanityCheck) {
		CAsset dbAsset;
        const uint32_t &nAsset = op == OP_ASSET_SEND ? theAssetAllocation.assetAllocationTuple.nAsset : theAsset.nAsset;
        auto result = mapAssets.try_emplace(nAsset, std::move(emptyAsset));
        auto mapAsset = result.first;
        const bool & mapAssetNotFound = result.second; 
		if (mapAssetNotFound)
		{
            if(!GetAsset(nAsset, dbAsset)){
    			if (op != OP_ASSET_ACTIVATE) {
    				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Failed to read from asset DB");
    				return error(errorMessage.c_str());
    			}
		    }
            else
                mapAsset->second = std::move(dbAsset); 
        }
        CAsset &storedSenderAssetRef = mapAsset->second;
		if (op == OP_ASSET_TRANSFER) {
		
            if (!FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot transfer this asset. Asset owner must sign off on this change");
                return error(errorMessage.c_str());
            }           
		}

		if (op == OP_ASSET_UPDATE) {
			if (!FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot update this asset. Asset owner must sign off on this change");
				return error(errorMessage.c_str());
			}

			if (theAsset.nBalance > 0 && !(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_SUPPLY))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update supply");
				return error(errorMessage.c_str());
			}          
            // increase total supply
            storedSenderAssetRef.nTotalSupply += theAsset.nBalance;
			storedSenderAssetRef.nBalance += theAsset.nBalance;

			if (!AssetRange(storedSenderAssetRef.nTotalSupply, storedSenderAssetRef.nPrecision))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2029 - " + _("Total supply out of money range");
				return error(errorMessage.c_str());
			}
			if (storedSenderAssetRef.nTotalSupply > storedSenderAssetRef.nMaxSupply)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2030 - " + _("Total supply cannot exceed maximum supply");
				return error(errorMessage.c_str());
			}

		}      
		if (op == OP_ASSET_SEND) {
			if (storedSenderAssetRef.witnessAddress != theAssetAllocation.assetAllocationTuple.witnessAddress || !FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot send this asset. Asset owner must sign off on this change");
				return error(errorMessage.c_str());
			}
	
			// check balance is sufficient on sender
			CAmount nTotal = 0;
			for (const auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
				nTotal += amountTuple.second;
				if (amountTuple.second <= 0)
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2032 - " + _("Receiving amount must be positive");
					return error(errorMessage.c_str());
				}
			}
			if (storedSenderAssetRef.nBalance < nTotal) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2033 - " + _("Sender balance is insufficient");
				return error(errorMessage.c_str());
			}
			for (const auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
				if (!bSanityCheck) {
                    
					CAssetAllocation receiverAllocation;
					const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.assetAllocationTuple.nAsset, amountTuple.first);
                    const string& receiverTupleStr = receiverAllocationTuple.ToString();
                    auto result = mapAssetAllocations.try_emplace(std::move(receiverTupleStr), std::move(emptyAllocation));
                    auto mapAssetAllocation = result.first;
                    const bool& mapAssetAllocationNotFound = result.second;
                   
                    if(mapAssetAllocationNotFound){
                        GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
                        if (receiverAllocation.assetAllocationTuple.IsNull()) {
                            receiverAllocation.assetAllocationTuple.nAsset = std::move(receiverAllocationTuple.nAsset);
                            receiverAllocation.assetAllocationTuple.witnessAddress = std::move(receiverAllocationTuple.witnessAddress);
                        } 
                        mapAssetAllocation->second = std::move(receiverAllocation);                
                    }
                    
                    CAssetAllocation& storedReceiverAllocationRef = mapAssetAllocation->second;
                    
                    storedReceiverAllocationRef.nBalance += amountTuple.second;
                                            
					// adjust sender balance
					storedSenderAssetRef.nBalance -= amountTuple.second;                              
				}
			}
            passetallocationdb->WriteAssetAllocationIndex(op, tx, storedSenderAssetRef, true, nHeight);  
		}
		else if (op != OP_ASSET_ACTIVATE)
		{         
			if (!theAsset.witnessAddress.IsNull())
				storedSenderAssetRef.witnessAddress = theAsset.witnessAddress;
			if (!theAsset.vchPubData.empty())
				storedSenderAssetRef.vchPubData = theAsset.vchPubData;
			else if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_DATA))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update public data");
				return error(errorMessage.c_str());
			}
                     
			if (!theAsset.vchBurnMethodSignature.empty() && op != OP_ASSET_TRANSFER)
				storedSenderAssetRef.vchBurnMethodSignature = theAsset.vchBurnMethodSignature;				
			else if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_CONTRACT))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update smart contract burn method signature");
				return error(errorMessage.c_str());
			}
            
            if (!theAsset.vchContract.empty() && op != OP_ASSET_TRANSFER)
                storedSenderAssetRef.vchContract = theAsset.vchContract;             
            else if (!(storedSenderAssetRef.nUpdateFlags & ASSET_UPDATE_CONTRACT))
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Insufficient privileges to update smart contract");
                return error(errorMessage.c_str());
            }    
                  
			if (theAsset.nUpdateFlags != storedSenderAssetRef.nUpdateFlags && (!(storedSenderAssetRef.nUpdateFlags & (ASSET_UPDATE_FLAGS | ASSET_UPDATE_ADMIN)))) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2040 - " + _("Insufficient privileges to update flags");
				return error(errorMessage.c_str());
			}
            storedSenderAssetRef.nUpdateFlags = theAsset.nUpdateFlags;


		}
		if (op == OP_ASSET_ACTIVATE)
		{
			if (GetAsset(nAsset, theAsset))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2041 - " + _("Asset already exists");
				return error(errorMessage.c_str());
			}
            storedSenderAssetRef = std::move(theAsset);
            if (!FindAssetOwnerInTx(inputs, tx, storedSenderAssetRef.witnessAddress))
            {
                errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot create this asset. Asset owner must sign off on this change");
                return error(errorMessage.c_str());
            }          
			// starting supply is the supplied balance upon init
			storedSenderAssetRef.nTotalSupply = storedSenderAssetRef.nBalance;
		}
		// set the asset's txn-dependent values
        storedSenderAssetRef.nHeight = nHeight;
		storedSenderAssetRef.txHash = txHash;
		// write asset, if asset send, only write on pow since asset -> asset allocation is not 0-conf compatible
		if (!bSanityCheck) {
            passetdb->WriteAssetIndex(tx, storedSenderAssetRef, op, nHeight);
			LogPrint(BCLog::SYS,"CONNECTED ASSET: op=%s symbol=%d hash=%s height=%d fJustCheck=%d\n",
					assetFromOp(op).c_str(),
					nAsset,
					txHash.ToString().c_str(),
					nHeight,
					fJustCheck ? 1 : 0);
		}
	}
    return true;
}

UniValue assetnew(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 9)
        throw runtime_error(
			"assetnew [address] [public value] [contract] [burn_method_signature] [precision=8] [supply] [max_supply] [update_flags] [witness]\n"
						"<address> An address that you own.\n"
                        "<public value> public data, 256 characters max.\n"
                        "<contract> Ethereum token contract for SyscoinX bridge. Must be in hex and not include the '0x' format tag. For example contract '0xb060ddb93707d2bc2f8bcc39451a5a28852f8d1d' should be set as 'b060ddb93707d2bc2f8bcc39451a5a28852f8d1d'. Leave empty for no smart contract bridge.\n" 
                        "<burn_method_signature> Ethereum token contract method signature for the burn function.  Use an ABI tool in Ethereum to find this, it mst be set so that the validation code knows the burn function was called to mint assets in Syscoin from Ethereum. Must be in hex and is 4 bytes (8 characters). ie: 'fefefefe'. Leave empty for no smart contract bridge.\n"  
						"<precision> Precision of balances. Must be between 0 and 8. The lower it is the higher possible max_supply is available since the supply is represented as a 64 bit integer. With a precision of 8 the max supply is 10 billion.\n"
						"<supply> Initial supply of asset. Can mint more supply up to total_supply amount or if total_supply is -1 then minting is uncapped.\n"
						"<max_supply> Maximum supply of this asset. Set to -1 for uncapped. Depends on the precision value that is set, the lower the precision the higher max_supply can be.\n"
						"<update_flags> Ability to update certain fields. Must be decimal value which is a bitmask for certain rights to update. The bitmask represents 0x01(1) to give admin status (needed to update flags), 0x10(2) for updating public data field, 0x100(4) for updating the smart contract/burn method signature fields, 0x1000(8) for updating supply, 0x10000(16) for being able to update flags (need admin access to update flags as well). 0x11111(31) for all.\n"
						"<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n"
						+ HelpRequiringPassphrase(pwallet));
	string vchAddress = params[0].get_str();
	vector<unsigned char> vchPubData = vchFromString(params[1].get_str());
    string strContract = params[2].get_str();
    string strMethodSig = params[3].get_str();
    if(!strContract.empty())
         boost::erase_all(strContract, "0x");  // strip 0x in hex str if exist
    if(!strMethodSig.empty())
         boost::erase_all(strMethodSig, "0x");  // strip 0x in hex str if exist
   
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
	if (IsValidDestination(address)) {
		strAddressFrom = strAddress;
	}
    UniValue detail = DescribeAddress(address);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();   
	CScript scriptPubKeyFromOrig;
	if (!strAddressFrom.empty()) {
		scriptPubKeyFromOrig = GetScriptForDestination(address);
	}
	
    CScript scriptPubKey;

	// calculate net
    // build asset object
    CAsset newAsset;
	newAsset.nAsset = GenerateSyscoinGuid();
	newAsset.vchPubData = vchPubData;
    newAsset.vchContract = ParseHex(strContract);
    newAsset.vchBurnMethodSignature = ParseHex(strMethodSig);
	newAsset.witnessAddress = CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex));
	newAsset.nBalance = nBalance;
	newAsset.nMaxSupply = nMaxSupply;
	newAsset.nPrecision = precision;
	newAsset.nUpdateFlags = nUpdateFlags;
	vector<unsigned char> data;
	newAsset.Serialize(data);

    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_ACTIVATE) << OP_2DROP;
    scriptPubKey += scriptPubKeyFromOrig;

	// use the script pub key to create the vecsend which sendmoney takes and puts it into vout
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);


	CScript scriptData;
	scriptData << OP_RETURN << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);
	UniValue res = syscointxfund_helper(vchWitness, vecSend);
	res.push_back((int)newAsset.nAsset);
	return res;
}
UniValue addressbalance(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    const UniValue &params = request.params;
    if (request.fHelp || params.size() != 1)
        throw runtime_error(
            "addressbalance [address]\n"
                        + HelpRequiringPassphrase(pwallet));
    string address = params[0].get_str();
    UniValue res(UniValue::VARR);
    res.push_back(ValueFromAmount(getaddressbalance(address)));
    return res;
}
UniValue assetupdate(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 7)
        throw runtime_error(
			"assetupdate [asset] [public value] [contract] [burn_method_signature] [supply] [update_flags] [witness]\n"
				"Perform an update on an asset you control.\n"
				"<asset> Asset guid.\n"
                "<public value> Public data, 256 characters max.\n"
                "<contract> Ethereum token contract for SyscoinX bridge. Must be in hex and not include the '0x' format tag. For example contract '0xb060ddb93707d2bc2f8bcc39451a5a28852f8d1d' should be set as 'b060ddb93707d2bc2f8bcc39451a5a28852f8d1d'. Leave empty for no smart contract bridge.\n" 
                "<burn_method_signature> Ethereum token contract method signature for the burn function.  Use an ABI tool in Ethereum to find this, it mst be set so that the validation code knows the burn function was called to mint assets in Syscoin from Ethereum. Must be in hex and is 4 bytes (8 characters). ie: 'fefefefe'. Leave empty for no smart contract bridge.\n"             
				"<supply> New supply of asset. Can mint more supply up to total_supply amount or if max_supply is -1 then minting is uncapped. If greator than zero, minting is assumed otherwise set to 0 to not mint any additional tokens.\n"
                "<update_flags> Ability to update certain fields. Must be decimal value which is a bitmask for certain rights to update. The bitmask represents 0x01(1) to give admin status (needed to update flags), 0x10(2) for updating public data field, 0x100(4) for updating the smart contract/burn method signature fields, 0x1000(8) for updating supply, 0x10000(16) for being able to update flags (need admin access to update flags as well). 0x11111(31) for all.\n"
                "<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n"
				+ HelpRequiringPassphrase(pwallet));
	const int &nAsset = params[0].get_int();
	string strData = "";
	string strPubData = "";
	string strCategory = "";
	strPubData = params[1].get_str();
    string strContract = params[2].get_str();
    string strMethodSig = params[3].get_str();
    if(!strContract.empty())
        boost::remove_erase_if(strContract, boost::is_any_of("0x")); // strip 0x in hex str if exists
    vector<unsigned char> vchContract = ParseHex(strContract);
    vector<unsigned char> vchBurnMethodSignature = ParseHex(strMethodSig);

	int nUpdateFlags = params[5].get_int();
	string vchWitness;
	vchWitness = params[6].get_str();

    CScript scriptPubKeyFromOrig;
	CAsset theAsset;

    if (!GetAsset( nAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Could not find a asset with this key"));

	string strAddressFrom;
	const string& strAddress = theAsset.witnessAddress.ToString();
	const CTxDestination &address = DecodeDestination(strAddress);
	if (IsValidDestination(address)) {
		strAddressFrom = strAddress;
	}

	UniValue param4 = params[4];
	CAmount nBalance = 0;
	if(param4.get_str() != "0")
		nBalance = AssetAmountFromValue(param4, theAsset.nPrecision);
	
	
	if (!strAddressFrom.empty()) {
		scriptPubKeyFromOrig = GetScriptForDestination(address);
	}
    

    // create ASSETUPDATE txn keys
    CScript scriptPubKey;

	if(strPubData != stringFromVch(theAsset.vchPubData))
		theAsset.vchPubData = vchFromString(strPubData);
    else
        theAsset.vchPubData.clear();
    if(vchContract != theAsset.vchContract)
        theAsset.vchContract = vchContract;
    else
        theAsset.vchContract.clear();
    if(vchBurnMethodSignature != theAsset.vchBurnMethodSignature)
        theAsset.vchBurnMethodSignature = vchBurnMethodSignature;
    else
        theAsset.vchBurnMethodSignature.clear();

	theAsset.nBalance = nBalance;
	theAsset.nUpdateFlags = nUpdateFlags;

	vector<unsigned char> data;
	theAsset.Serialize(data);

    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_UPDATE) << OP_2DROP;
    scriptPubKey += scriptPubKeyFromOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);


	CScript scriptData;
	scriptData << OP_RETURN << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);
	return syscointxfund_helper(vchWitness, vecSend);
}

UniValue assettransfer(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 3)
        throw runtime_error(
			"assettransfer [asset] [address] [witness]\n"
						"Transfer an asset you own to another address.\n"
						"<asset> Asset guid.\n"
						"<address> Address to transfer to.\n"
						"<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase(pwallet));

    // gather & validate inputs
	const int &nAsset = params[0].get_int();
	string vchAddressTo = params[1].get_str();
	string vchWitness;
	vchWitness = params[2].get_str();

    CScript scriptPubKeyOrig, scriptPubKeyFromOrig;
	CAsset theAsset;
    if (!GetAsset( nAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2505 - " + _("Could not find a asset with this key"));
	
	string strAddressFrom;
	const string& strAddress = theAsset.witnessAddress.ToString();
	const CTxDestination addressFrom = DecodeDestination(strAddress);
	if (IsValidDestination(addressFrom)) {
		strAddressFrom = strAddress;
	}


	const CTxDestination addressTo = DecodeDestination(vchAddressTo);
	if (IsValidDestination(addressTo)) {
		scriptPubKeyOrig = GetScriptForDestination(addressTo);
	}

    UniValue detail = DescribeAddress(addressTo);
    if(find_value(detail.get_obj(), "iswitness").get_bool() == false)
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Address must be a segwit based address"));
    string witnessProgramHex = find_value(detail.get_obj(), "witness_program").get_str();
    unsigned char witnessVersion = (unsigned char)find_value(detail.get_obj(), "witness_version").get_int();   
	if (!strAddressFrom.empty()) {
		scriptPubKeyFromOrig = GetScriptForDestination(addressFrom);
	}
    
	theAsset.ClearAsset();
    CScript scriptPubKey;
	theAsset.witnessAddress = CWitnessAddress(witnessVersion, ParseHex(witnessProgramHex));

	vector<unsigned char> data;
	theAsset.Serialize(data);

    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_TRANSFER) << OP_2DROP;
	scriptPubKey += scriptPubKeyOrig;
    // send the asset pay txn
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);


	CScript scriptData;
	scriptData << OP_RETURN << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);
	return syscointxfund_helper(vchWitness, vecSend);
}
UniValue assetsend(const JSONRPCRequest& request) {
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 3)
		throw runtime_error(
			"assetsend [asset] ([{\"address\":\"address\",\"amount\":amount},...] [witness]\n"
			"Send an asset you own to another address/address as an asset allocation. Maximimum recipients is 250.\n"
			"<asset> Asset guid.\n"
			"<address> Address to transfer to.\n"
			"<amount> Quantity of asset to send.\n"
			"<witness> Witness address that will sign for web-of-trust notarization of this transaction.\n"
			+ HelpRequiringPassphrase(pwallet));
	// gather & validate inputs
	const int &nAsset = params[0].get_int();
	UniValue valueTo = params[1];
	string vchWitness = params[2].get_str();
	if (!valueTo.isArray())
		throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Array of receivers not found");

	CAsset theAsset;
	if (!GetAsset(nAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2507 - " + _("Could not find a asset with this key"));

	string strAddressFrom;
	const string& strAddress = theAsset.witnessAddress.ToString();
	const CTxDestination addressFrom = DecodeDestination(strAddress);
	if (IsValidDestination(addressFrom)) {
		strAddressFrom = strAddress;
	}


	CScript scriptPubKeyFromOrig;

	if (!strAddressFrom.empty()) {
		scriptPubKeyFromOrig = GetScriptForDestination(addressFrom);
	}


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
		if (amountObj.isNum()) {
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

	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_SEND) << OP_2DROP;
	scriptPubKey += scriptPubKeyFromOrig;
	// send the asset pay txn
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CScript scriptData;
	scriptData << OP_RETURN << CScript::EncodeOP_N(OP_SYSCOIN_ASSET_ALLOCATION) << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, fee);
	vecSend.push_back(fee);

	return syscointxfund_helper(vchWitness, vecSend);
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
    oAsset.pushKV("burnsig", asset.vchBurnMethodSignature.empty()? "" : HexStr(asset.vchBurnMethodSignature));
	oAsset.pushKV("balance", ValueFromAssetAmount(asset.nBalance, asset.nPrecision));
	oAsset.pushKV("total_supply", ValueFromAssetAmount(asset.nTotalSupply, asset.nPrecision));
	oAsset.pushKV("max_supply", ValueFromAssetAmount(asset.nMaxSupply, asset.nPrecision));
	oAsset.pushKV("update_flags", asset.nUpdateFlags);
	oAsset.pushKV("precision", (int)asset.nPrecision);
	return true;
}
bool AssetTxToJSON(const int &op, const CTransaction& tx, UniValue &entry)
{
	CAsset asset(tx);
	if(!asset.IsNull())
		return false;

	CAsset dbAsset;
	GetAsset(asset.nAsset, dbAsset);
    
    int nHeight = 0;
    uint256 hash_block;
    CBlockIndex* blockindex = nullptr;
    CTransactionRef txRef;
    if (GetTransaction(tx.GetHash(), txRef, Params().GetConsensus(), hash_block, true, blockindex) && blockindex)
        nHeight = blockindex->nHeight; 
        	

	entry.pushKV("txtype", assetFromOp(op));
	entry.pushKV("_id", (int)asset.nAsset);
    entry.pushKV("txid", tx.GetHash().GetHex());
    entry.pushKV("height", nHeight);
    
	if(op == OP_ASSET_ACTIVATE || (!asset.vchPubData.empty() && dbAsset.vchPubData != asset.vchPubData))
		entry.pushKV("publicvalue", stringFromVch(asset.vchPubData));
        
    if(op == OP_ASSET_ACTIVATE || (!asset.vchContract.empty() && dbAsset.vchContract != asset.vchContract))
        entry.pushKV("contract", "0x"+HexStr(asset.vchContract));
        
    if(op == OP_ASSET_ACTIVATE || (!asset.vchBurnMethodSignature.empty() && dbAsset.vchContract != asset.vchBurnMethodSignature))
        entry.pushKV("burnsig", HexStr(asset.vchBurnMethodSignature));                  
        
	if (op == OP_ASSET_ACTIVATE || (!asset.witnessAddress.IsNull() && dbAsset.witnessAddress != asset.witnessAddress))
		entry.pushKV("address", asset.witnessAddress.ToString());

	if (op == OP_ASSET_ACTIVATE || asset.nUpdateFlags != dbAsset.nUpdateFlags)
		entry.pushKV("update_flags", asset.nUpdateFlags);
              
	if (op == OP_ASSET_ACTIVATE || asset.nBalance != dbAsset.nBalance)
		entry.pushKV("balance", ValueFromAssetAmount(asset.nBalance, dbAsset.nPrecision));
    if (op == OP_ASSET_ACTIVATE){
        entry.pushKV("total_supply", ValueFromAssetAmount(asset.nTotalSupply, dbAsset.nPrecision)); 
        entry.pushKV("precision", asset.nPrecision);  
    }         
     return true;
}
bool AssetTxToJSON(const int &op, const CTransaction& tx, const CAsset& dbAsset, const int& nHeight, UniValue &entry)
{
    CAsset asset(tx);
    if(asset.IsNull() || dbAsset.IsNull())
        return false;

    entry.pushKV("txtype", assetFromOp(op));
    entry.pushKV("_id", (int)asset.nAsset);
    entry.pushKV("txid", tx.GetHash().GetHex());
    entry.pushKV("height", nHeight);

    if(op == OP_ASSET_ACTIVATE || (!asset.vchPubData.empty() && dbAsset.vchPubData != asset.vchPubData))
        entry.pushKV("publicvalue", stringFromVch(asset.vchPubData));
        
    if(op == OP_ASSET_ACTIVATE || (!asset.vchContract.empty() && dbAsset.vchContract != asset.vchContract))
        entry.pushKV("contract", "0x"+HexStr(asset.vchContract));
        
    if(op == OP_ASSET_ACTIVATE || (!asset.vchBurnMethodSignature.empty() && dbAsset.vchContract != asset.vchBurnMethodSignature))
        entry.pushKV("burnsig", HexStr(asset.vchBurnMethodSignature));                  
        
    if (op == OP_ASSET_ACTIVATE || (!asset.witnessAddress.IsNull() && dbAsset.witnessAddress != asset.witnessAddress))
        entry.pushKV("address", asset.witnessAddress.ToString());

    if (op == OP_ASSET_ACTIVATE || asset.nUpdateFlags != dbAsset.nUpdateFlags)
        entry.pushKV("update_flags", asset.nUpdateFlags);
              
    if (op == OP_ASSET_ACTIVATE || asset.nBalance != dbAsset.nBalance)
        entry.pushKV("balance", ValueFromAssetAmount(asset.nBalance, dbAsset.nPrecision));
    if (op == OP_ASSET_ACTIVATE){
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
		divByAmount = powf(10, precision);
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
		value.setInt((int64_t)(MAX_ASSET / ((int)powf(10, precision))));
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
		int64_t divByAmount = powf(10, precision);
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
			+ HelpExampleCli("listassets", "0 0 '{\"addresses\":[{\"address\":\"SfaMwYY19Dh96B9qQcJQuiNykVRTzXMsZR\"},{\"address\":\"SfaMwYY19Dh96B9qQcJQuiNykVRTzXMsZR\"}]}'")
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
            + HelpExampleCli("syscoinsetethheaders", "syncing 7000000")
            + HelpExampleCli("syscoinsetethheaders", "synced 0"));
    string status = params[0].get_str();
    int highestBlock = params[1].get_int();
    
    if(highestBlock > 0){
        LOCK(cs_ethsyncheight);
        fGethSyncHeight = highestBlock;
    }
    fGethSyncStatus = status;        
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
        txRootMap.try_emplace(std::move(nHeight), std::move(vchTxRoot));
    } 
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("status", "success");
    return ret;
}
bool CEthereumTxRootsDB::PruneTxRoots() {
    LogPrintf("Pruning Ethereum Transaction Roots...\n");
    EthereumTxRootMap mapEraseTxRoots;
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->SeekToFirst();
    vector<uint32_t> vecHeightKeys;
    uint32_t key;
    int32_t cutoffHeight;
    {
        LOCK(cs_ethsyncheight);
        // cutoff is ~1.5 months of blocks is about 250k blocks
        cutoffHeight = fGethSyncHeight - MAX_ETHEREUM_TX_ROOTS;
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
