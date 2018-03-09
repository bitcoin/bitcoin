// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "asset.h"
#include "assetallocation.h"
#include "alias.h"
#include "init.h"
#include "validation.h"
#include "util.h"
#include "random.h"
#include "base58.h"
#include "core_io.h"
#include "rpc/server.h"
#include "wallet/wallet.h"
#include "chainparams.h"
#include "coincontrol.h"
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <chrono>
using namespace std::chrono;
using namespace std;
extern void SendMoneySyscoin(const vector<unsigned char> &vchAlias, const vector<unsigned char> &vchWitness, const CRecipient &aliasRecipient, CRecipient &aliasPaymentRecipient, vector<CRecipient> &vecSend, CWalletTx& wtxNew, CCoinControl* coinControl, bool fUseInstantSend=false, bool transferAlias=false);
bool IsAssetOp(int op) {
    return op == OP_ASSET_ACTIVATE
        || op == OP_ASSET_UPDATE
        || op == OP_ASSET_TRANSFER
		|| op == OP_ASSET_SEND;
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
bool CAsset::UnserializeFromData(const vector<unsigned char> &vchData, const vector<unsigned char> &vchHash) {
    try {
        CDataStream dsAsset(vchData, SER_NETWORK, PROTOCOL_VERSION);
        dsAsset >> *this;

		vector<unsigned char> vchAssetData;
		Serialize(vchAssetData);
		const uint256 &calculatedHash = Hash(vchAssetData.begin(), vchAssetData.end());
		const vector<unsigned char> &vchRandAsset = vchFromValue(calculatedHash.GetHex());
		if(vchRandAsset != vchHash)
		{
			SetNull();
			return false;
		}

    } catch (std::exception &e) {
		SetNull();
        return false;
    }
	return true;
}
bool CAsset::UnserializeFromTx(const CTransaction &tx) {
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nOut))
	{
		SetNull();
		return false;
	}
	if(!UnserializeFromData(vchData, vchHash))
	{	
		return false;
	}
    return true;
}
void CAsset::Serialize( vector<unsigned char> &vchData) {
    CDataStream dsAsset(SER_NETWORK, PROTOCOL_VERSION);
    dsAsset << *this;
	vchData = vector<unsigned char>(dsAsset.begin(), dsAsset.end());

}
void CAssetDB::WriteAssetIndex(const CAsset& asset, const int& op) {
	UniValue oName(UniValue::VOBJ);
	if (BuildAssetIndexerJson(asset, oName)) {
		GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "asset");
	}
	WriteAssetIndexHistory(asset, op);
}
void CAssetDB::WriteAssetIndexHistory(const CAsset& asset, const int &op) {
	UniValue oName(UniValue::VOBJ);
	if (BuildAssetIndexerHistoryJson(asset, oName)) {
		oName.push_back(Pair("op", assetFromOp(op)));
		GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "assethistory");
	}
}
bool GetAsset(const vector<unsigned char> &vchAsset,
        CAsset& txPos) {
    if (!passetdb || !passetdb->ReadAsset(vchAsset, txPos))
        return false;
    return true;
}
bool DecodeAndParseAssetTx(const CTransaction& tx, int& op, int& nOut,
		vector<vector<unsigned char> >& vvch, char &type)
{
	CAsset asset;
	bool decode = DecodeAssetTx(tx, op, nOut, vvch);
	bool parse = asset.UnserializeFromTx(tx);
	if (decode&&parse) {
		type = ASSET;
		return true;
	}
	return false;
}
bool DecodeAssetTx(const CTransaction& tx, int& op, int& nOut,
        vector<vector<unsigned char> >& vvch) {
    bool found = false;


    // Strict check - bug disallowed
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& out = tx.vout[i];
        vector<vector<unsigned char> > vvchRead;
        if (DecodeAssetScript(out.scriptPubKey, op, vvchRead)) {
            nOut = i; found = true; vvch = vvchRead;
            break;
        }
    }
    if (!found) vvch.clear();
    return found;
}


bool DecodeAssetScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch, CScript::const_iterator& pc) {
    opcodetype opcode;
	vvch.clear();
    if (!script.GetOp(pc, opcode)) return false;
    if (opcode < OP_1 || opcode > OP_16) return false;
    op = CScript::DecodeOP_N(opcode);
	if (op != OP_SYSCOIN_ASSET)
		return false;
	if (!script.GetOp(pc, opcode))
		return false;
	if (opcode < OP_1 || opcode > OP_16)
		return false;
	op = CScript::DecodeOP_N(opcode);
	if (!IsAssetOp(op))
		return false;

	bool found = false;
	for (;;) {
		vector<unsigned char> vch;
		if (!script.GetOp(pc, opcode, vch))
			return false;
		if (opcode == OP_DROP || opcode == OP_2DROP)
		{
			found = true;
			break;
		}
		if (!(opcode >= 0 && opcode <= OP_PUSHDATA4))
			return false;
		vvch.push_back(vch);
	}

	// move the pc to after any DROP or NOP
	while (opcode == OP_DROP || opcode == OP_2DROP) {
		if (!script.GetOp(pc, opcode))
			break;
	}

	pc--;
	return found;
}
bool DecodeAssetScript(const CScript& script, int& op,
        vector<vector<unsigned char> > &vvch) {
    CScript::const_iterator pc = script.begin();
    return DecodeAssetScript(script, op, vvch, pc);
}
bool RemoveAssetScriptPrefix(const CScript& scriptIn, CScript& scriptOut) {
    int op;
    vector<vector<unsigned char> > vvch;
    CScript::const_iterator pc = scriptIn.begin();

    if (!DecodeAssetScript(scriptIn, op, vvch, pc))
		return false;
	scriptOut = CScript(pc, scriptIn.end());
	return true;
}
bool CheckAssetInputs(const CTransaction &tx, int op, int nOut, const vector<vector<unsigned char> > &vvchArgs, const std::vector<unsigned char> &vvchAlias,
        bool fJustCheck, int nHeight, sorted_vector<CAssetAllocationTuple> &revertedAssetAllocations, string &errorMessage, bool bSanityCheck) {
	if (!paliasdb || !passetdb)
		return false;
	if (tx.IsCoinBase() && !fJustCheck && !bSanityCheck)
	{
		LogPrintf("*Trying to add asset in coinbase transaction, skipping...");
		return true;
	}
	if (fDebug && !bSanityCheck)
		LogPrintf("*** ASSET %d %d %s %s\n", nHeight,
			chainActive.Tip()->nHeight, tx.GetHash().ToString().c_str(),
			fJustCheck ? "JUSTCHECK" : "BLOCK");

	// unserialize asset from txn, check for valid
	CAsset theAsset;
	CAssetAllocation theAssetAllocation;
	vector<unsigned char> vchData;
	vector<unsigned char> vchHash;
	int nDataOut;
	if(!GetSyscoinData(tx, vchData, vchHash, nDataOut) || (op != OP_ASSET_SEND &&!theAsset.UnserializeFromData(vchData, vchHash)) || (op == OP_ASSET_SEND && !theAssetAllocation.UnserializeFromData(vchData, vchHash)))
	{
		errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR ERRCODE: 2001 - " + _("Cannot unserialize data inside of this transaction relating to a asset");
		return true;
	}

	if(fJustCheck)
	{
		if(vvchArgs.size() != 1)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2002 - " + _("Asset arguments incorrect size");
			return error(errorMessage.c_str());
		}

					
		if(vchHash != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2003 - " + _("Hash provided doesn't match the calculated hash of the data");
			return true;
		}
			
	}
	
	CAliasIndex alias;
	string retError = "";
	if(fJustCheck)
	{
		if (op != OP_ASSET_SEND) {
			if (theAsset.sCategory.size() > MAX_NAME_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("Asset category too big");
				return error(errorMessage.c_str());
			}
			if (theAsset.vchPubData.size() > MAX_VALUE_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2007 - " + _("Asset public data too big");
				return error(errorMessage.c_str());
			}
		}
		switch (op) {
		case OP_ASSET_ACTIVATE:
			if (theAsset.vchAsset.size() > MAX_ID_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2004 - " + _("asset name too long");
				return error(errorMessage.c_str());
			}
			if(!boost::algorithm::starts_with(stringFromVch(theAsset.sCategory), "assets"))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2013 - " + _("Must use a asset category");
				return error(errorMessage.c_str());
			}
			if(theAsset.bUseInputRanges && theAsset.listAllocationInputs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2013 - " + _("Must specify input range");
				return error(errorMessage.c_str());
			}
			if (!theAsset.bUseInputRanges && !theAsset.listAllocationInputs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2013 - " + _("Cannot specify input range for this asset");
				return error(errorMessage.c_str());
			}

			if (theAsset.fInterestRate < 0 || theAsset.fInterestRate > 1)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2013 - " + _("Interest must be between 0 and 1");
				return error(errorMessage.c_str());
			}
			if (theAsset.nBalance <= 0 || !AssetRange(theAsset.nBalance))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Initial balance out of money range");
				return true;
			}
			if (theAsset.nMaxSupply > 0 && !AssetRange(theAsset.nMaxSupply))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Max supply out of money range");
				return true;
			}
			break;

		case OP_ASSET_UPDATE:
			if(theAsset.sCategory.size() > 0 && !boost::algorithm::istarts_with(stringFromVch(theAsset.sCategory), "assets"))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2017 - " + _("Must use a asset category");
				return error(errorMessage.c_str());
			}
			if (theAsset.nBalance < 0)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Balance must be greator than or equal to 0");
				return error(errorMessage.c_str());
			}
			if (theAsset.fInterestRate < 0 || theAsset.fInterestRate > 1)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2013 - " + _("Interest must be between 0 and 1");
				return error(errorMessage.c_str());
			}
			break;

		case OP_ASSET_TRANSFER:
			if (!theAssetAllocation.listSendingAllocationInputs.empty() || !theAssetAllocation.listSendingAllocationAmounts.empty() || !theAsset.listAllocationInputs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Cannot transfer input allocations");
				return error(errorMessage.c_str());
			}
			break;
		case OP_ASSET_SEND:
			if (theAssetAllocation.listSendingAllocationInputs.empty() && theAssetAllocation.listSendingAllocationAmounts.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Asset send must send an input or transfer balance");
				return error(errorMessage.c_str());
			}
			if (theAssetAllocation.listSendingAllocationInputs.size() > 250 || theAssetAllocation.listSendingAllocationAmounts.size() > 250)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Too many receivers in one allocation send, maximum of 250 is allowed at once");
				return error(errorMessage.c_str());
			}
			if (theAssetAllocation.vchMemo.size() > MAX_MEMO_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2007 - " + _("memo too long, must be 128 character or less");
				return error(errorMessage.c_str());
			}
			break;
		default:
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Asset transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}
	if (!fJustCheck) {
		const string &user1 = stringFromVch(vvchAlias);
		string user2 = "";
		string user3 = "";
		if (op == OP_ASSET_TRANSFER) {
			user2 = stringFromVch(theAsset.vchAlias);
		}
		string strResponseEnglish = "";
		string strResponse = GetSyscoinTransactionDescription(op, strResponseEnglish, ASSET);
		CAsset dbAsset;
		if (!GetAsset(op == OP_ASSET_SEND ? theAssetAllocation.vchAsset : theAsset.vchAsset, dbAsset))
		{
			if (op != OP_ASSET_ACTIVATE) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("Failed to read from asset DB");
				return true;
			}
		}

		if (op == OP_ASSET_UPDATE || op == OP_ASSET_TRANSFER || op == OP_ASSET_SEND)
		{
			if (dbAsset.vchAlias != vvchAlias)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot edit this asset. Asset owner must sign off on this change");
				return true;
			}
		}
		if (op == OP_ASSET_UPDATE) {
			CAmount increaseBalanceByAmount = theAsset.nBalance;
			theAsset.nBalance = dbAsset.nBalance;
			if (!theAsset.listAllocationInputs.empty()) {
				if(!dbAsset.bUseInputRanges)
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("This asset does not use input ranges");
					return true;
				}
				// ensure the new inputs being added are greator than the last input
				for (auto&input : theAsset.listAllocationInputs) {
					if(input.start < (dbAsset.nTotalSupply/COIN))
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Cannot edit this asset. New asset inputs must be added to the end of the supply");
						return true;
					}
				}
				vector<CRange> outputMerge;
				increaseBalanceByAmount = validateRangesAndGetCount(theAsset.listAllocationInputs)*COIN;
				if (increaseBalanceByAmount == 0)
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Invalid input ranges");
					return true;
				}
				dbAsset.listAllocationInputs.insert(std::end(dbAsset.listAllocationInputs), std::begin(theAsset.listAllocationInputs), std::end(theAsset.listAllocationInputs));
				mergeRanges(dbAsset.listAllocationInputs, outputMerge);
				theAsset.listAllocationInputs = outputMerge;
			}
			theAsset.nBalance += increaseBalanceByAmount;
			// increase total supply
			theAsset.nTotalSupply += increaseBalanceByAmount;
			if (!AssetRange(theAsset.nTotalSupply))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Total supply out of money range");
				return true;
			}
			const CAmount &nMaxSupply = dbAsset.nMaxSupply > 0 ? dbAsset.nMaxSupply : MAX_ASSET;
			if (theAsset.nTotalSupply > nMaxSupply)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2026 - " + _("Total supply cannot exceed maximum supply");
				return true;
			}

		}
		else if (op != OP_ASSET_ACTIVATE) {
			theAsset.nBalance = dbAsset.nBalance;
			theAsset.nTotalSupply = dbAsset.nBalance;
			theAsset.nMaxSupply = dbAsset.nMaxSupply;
			theAsset.bUseInputRanges = dbAsset.bUseInputRanges;
			theAsset.bCanAdjustInterestRate = dbAsset.bCanAdjustInterestRate;
		}

		if (op == OP_ASSET_SEND) {
			theAsset = dbAsset;

			CAssetAllocation dbAssetAllocation;
			const CAssetAllocationTuple allocationTuple(theAssetAllocation.vchAsset, vvchAlias);
			GetAssetAllocation(allocationTuple, dbAssetAllocation);
			if (!theAssetAllocation.listSendingAllocationAmounts.empty()) {
				if (dbAsset.bUseInputRanges) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Invalid asset send, request to send amounts but asset uses input ranges");
					return true;
				}
				// check balance is sufficient on sender
				CAmount nTotal = 0;
				for (auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
					nTotal += amountTuple.second;
					if (amountTuple.second <= 0)
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Receiving amount must be positive");
						return true;
					}
				}
				if (theAsset.nBalance < nTotal) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Sender balance is insufficient");
					return true;
				}
				for (auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {

					if (!bSanityCheck) {
						CAssetAllocation receiverAllocation;
						const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.vchAsset, amountTuple.first);
						// don't need to check for existance of allocation because it may not exist, may be creating it here for the first time for receiver
						GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
						if (receiverAllocation.IsNull()) {
							receiverAllocation.vchAlias = receiverAllocationTuple.vchAlias;
							receiverAllocation.vchAsset = receiverAllocationTuple.vchAsset;
							receiverAllocation.nLastInterestClaimHeight = nHeight;
						}
						receiverAllocation.txHash = tx.GetHash();
						if (theAsset.fInterestRate > 0) {
							if (receiverAllocation.nHeight > 0) {
								AccumulateInterestSinceLastClaim(receiverAllocation, nHeight);
							}
						}
						receiverAllocation.fInterestRate = theAsset.fInterestRate;
						receiverAllocation.nHeight = nHeight;
						receiverAllocation.vchMemo = theAssetAllocation.vchMemo;
						receiverAllocation.nBalance += amountTuple.second;
						// adjust sender balance
						theAsset.nBalance -= amountTuple.second;
						if (!passetallocationdb->WriteAssetAllocation(receiverAllocation, INT64_MAX, fJustCheck))
						{
							errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to asset allocation DB");
							continue;
						}
						if (strResponse != "") {
							paliasdb->WriteAliasIndexTxHistory(user1, stringFromVch(receiverAllocation.vchAlias), user3, tx.GetHash(), nHeight, strResponseEnglish, receiverAllocationTuple.ToString());
						}
					}
				}
			}
			else if (!theAssetAllocation.listSendingAllocationInputs.empty()) {
				if (!dbAsset.bUseInputRanges) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Invalid asset send, request to send input ranges but asset uses amounts");
					return true;
				}
				// check balance is sufficient on sender
				CAmount nTotal = 0;
				vector<CAmount> rangeTotals;
				for (auto& inputTuple : theAssetAllocation.listSendingAllocationInputs) {
					const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.vchAsset, inputTuple.first);
					const unsigned int rangeTotal = validateRangesAndGetCount(inputTuple.second);
					if (rangeTotal == 0)
					{
						errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Invalid input range");
						return true;
					}
					const CAmount rangeTotalAmount = rangeTotal*COIN;
					rangeTotals.push_back(rangeTotalAmount);
					nTotal += rangeTotalAmount;
				}
				if (theAsset.nBalance < nTotal) {
					errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Sender balance is insufficient");
					return true;
				}
				for (unsigned int i = 0; i < theAssetAllocation.listSendingAllocationInputs.size(); i++) {
					InputRanges &input = theAssetAllocation.listSendingAllocationInputs[i];
					CAssetAllocation receiverAllocation;

					const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.vchAsset, input.first);
					// ensure entire allocation range being subtracted exists on sender (full inclusion check)
					if (!doesRangeContain(dbAsset.listAllocationInputs, input.second))
					{
						errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2025 - " + _("Input not found");
						return true;
					}
					if (!bSanityCheck) {
						if (!GetAssetAllocation(receiverAllocationTuple, receiverAllocation)) {
							receiverAllocation.SetNull();
							receiverAllocation.vchAlias = receiverAllocationTuple.vchAlias;
							receiverAllocation.vchAsset = receiverAllocationTuple.vchAsset;
							receiverAllocation.nLastInterestClaimHeight = nHeight;
						}
						
						receiverAllocation.txHash = tx.GetHash();
						if (theAsset.fInterestRate > 0) {
							if (receiverAllocation.nHeight > 0) {
								AccumulateInterestSinceLastClaim(receiverAllocation, nHeight);
							}
						}
						receiverAllocation.fInterestRate = theAsset.fInterestRate;
						receiverAllocation.nHeight = nHeight;
						receiverAllocation.vchMemo = theAssetAllocation.vchMemo;
						// figure out receivers added ranges and balance
						vector<CRange> outputMerge;
						receiverAllocation.listAllocationInputs.insert(std::end(receiverAllocation.listAllocationInputs), std::begin(input.second), std::end(input.second));
						mergeRanges(receiverAllocation.listAllocationInputs, outputMerge);
						receiverAllocation.listAllocationInputs = outputMerge;
						receiverAllocation.nBalance += rangeTotals[i];
		

						// figure out senders subtracted ranges and balance
						vector<CRange> outputSubtract;
						subtractRanges(dbAsset.listAllocationInputs, input.second, outputSubtract);
						theAsset.listAllocationInputs = outputSubtract;
						theAsset.nBalance -= rangeTotals[i];
						if (!passetallocationdb->WriteAssetAllocation(receiverAllocation, INT64_MAX, fJustCheck))
						{
							errorMessage = "SYSCOIN_ASSET_ALLOCATION_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to asset allocation DB");
							return error(errorMessage.c_str());
						}

						if (strResponse != "") {
							paliasdb->WriteAliasIndexTxHistory(user1, stringFromVch(receiverAllocation.vchAlias), user3, tx.GetHash(), nHeight, strResponseEnglish, receiverAllocationTuple.ToString());
						}
					}
				}
			}
		}
		else if (op != OP_ASSET_ACTIVATE)
		{
			if (theAsset.vchAlias.empty())
				theAsset.vchAlias = dbAsset.vchAlias;
			if (theAsset.vchPubData.empty())
				theAsset.vchPubData = dbAsset.vchPubData;
			if (theAsset.sCategory.empty())
				theAsset.sCategory = dbAsset.sCategory;

			if (op == OP_ASSET_UPDATE) {
				if (!theAsset.bCanAdjustInterestRate && theAsset.fInterestRate != dbAsset.fInterestRate) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("Cannot adjust interest rate for this asset");
					return true;
				}
			}
			if (op == OP_ASSET_TRANSFER)
			{
				// cannot adjust allocation inputs upon transfer, balance's and maxsupply or interest rate variables are also non-alterable and set to default in an above if statement
				theAsset.listAllocationInputs = dbAsset.listAllocationInputs;
				theAsset.fInterestRate = dbAsset.fInterestRate;
			}

		}
		if (op == OP_ASSET_ACTIVATE)
		{
			if (GetAsset(theAsset.vchAsset, theAsset))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2027 - " + _("Asset already exists");
				return true;
			}
			// starting supply is the supplied balance upon init
			theAsset.nTotalSupply = theAsset.nBalance;
		}
		if (!bSanityCheck) {
			if (strResponse != "") {
				paliasdb->WriteAliasIndexTxHistory(user1, user2, user3, tx.GetHash(), nHeight, strResponseEnglish, stringFromVch(theAsset.vchAsset));
			}
		}
		// set the asset's txn-dependent values
		theAsset.nHeight = nHeight;
		theAsset.txHash = tx.GetHash();
		// write asset, if asset send, only write on pow since asset -> asset allocation is not 0-conf compatible
		if (!bSanityCheck) {
			if (!passetdb->WriteAsset(theAsset, op))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Failed to write to asset DB");
				return error(errorMessage.c_str());
			}
			// debug
			if (fDebug)
				LogPrintf("CONNECTED ASSET: op=%s asset=%s hash=%s height=%d fJustCheck=%d\n",
					assetFromOp(op).c_str(),
					stringFromVch(op == OP_ASSET_SEND ? theAssetAllocation.vchAsset : theAsset.vchAsset).c_str(),
					tx.GetHash().ToString().c_str(),
					nHeight,
					fJustCheck ? 1 : 0);
		}
	}
    return true;
}

UniValue assetnew(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 10)
        throw runtime_error(
			"assetnew [name] [alias] [public] [category=assets] [supply] [max_supply] [use_inputranges] [interest_rate] [can_adjust_interest_rate] [witness]\n"
						"<name> name, 20 characters max.\n"
						"<alias> An alias you own.\n"
                        "<public> public data, 256 characters max.\n"
						"<category> category, 256 characters max. Defaults to assets.\n"
						"<supply> Initial supply of asset. Can mint more supply up to total_supply amount or if total_supply is -1 then minting is uncapped.\n"
						"<max_supply> Maximum supply of this asset. Set to -1 for uncapped.\n"
						"<use_inputranges> If this asset uses an input for every token, useful if you need to keep track of a token regardless of ownership.\n"
						"<interest_rate> The annual interest rate if any. Money supply is still capped to total supply. Should be between 0 and 1 and represents a percentage divided by 100.\n"
						"<can_adjust_interest_rate> Ability to adjust interest rate through assetupdate in the future.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
						+ HelpRequiringPassphrase());
    vector<unsigned char> vchName = vchFromString(params[0].get_str());
	vector<unsigned char> vchAlias = vchFromValue(params[1]);
	vector<unsigned char> vchPubData = vchFromString(params[2].get_str());
	string strCategory = "assets";
	strCategory = params[3].get_str();
	vector<unsigned char> vchWitness;
	CAmount nBalance = AssetAmountFromValue(params[4]);
	CAmount nMaxSupply = -1*COIN;
	if(params[5].get_str() != "-1")
		nMaxSupply = AssetAmountFromValue(params[5]);
	bool bUseInputRanges = params[6].get_bool();
	float fInterestRate = params[7].get_real();
	bool bCanAdjustInterestRate = params[8].get_bool();
	vchWitness = vchFromValue(params[9]);
	// check for alias existence in DB
	CAliasIndex theAlias;

	if (!GetAlias(vchAlias, theAlias))
		throw runtime_error("SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2500 - " + _("failed to read alias from alias DB"));

	
    // this is a syscoin transaction
    CWalletTx wtx;

    CScript scriptPubKeyOrig;
	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);


    CScript scriptPubKey,scriptPubKeyAlias;

	// calculate net
    // build asset object
    CAsset newAsset;
	newAsset.vchAsset = vchName;
	newAsset.sCategory = vchFromString(strCategory);
	newAsset.vchPubData = vchPubData;
	newAsset.vchAlias = vchAlias;
	newAsset.nBalance = nBalance;
	newAsset.nMaxSupply = nMaxSupply;
	newAsset.bUseInputRanges = bUseInputRanges;
	newAsset.fInterestRate = fInterestRate;
	newAsset.bCanAdjustInterestRate = bCanAdjustInterestRate;
	if (bUseInputRanges)
	{
		CRange range(0, (nBalance/COIN) - 1);
		newAsset.listAllocationInputs.push_back(range);
	}
	vector<unsigned char> data;
	newAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());

    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_ACTIVATE) << vchHashAsset << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyOrig;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;

	// use the script pub key to create the vecsend which sendmoney takes and puts it into vout
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, aliasPaymentRecipient);
		
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);

	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;	
	SendMoneySyscoin(vchAlias, vchWitness, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}

UniValue assetupdate(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 6)
        throw runtime_error(
			"assetupdate [asset] [public] [category=assets] [supply] [interest_rate] [witness]\n"
						"Perform an update on an asset you control.\n"
						"<asset> Asset name.\n"
                        "<public> Public data, 256 characters max.\n"                
						"<category> Category, 256 characters max. Defaults to assets\n"
						"<supply> New supply of asset. Can mint more supply up to total_supply amount or if max_supply is -1 then minting is uncapped.\n"
						"<interest_rate> The annual interest rate if any. Money supply is still capped to total supply. Should be between 0 and 1 and represents a percentage divided by 100. Can only set if this asset allows adjustment of interest rate.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAsset = vchFromValue(params[0]);

	string strData = "";
	string strPubData = "";
	string strCategory = "";
	strPubData = params[1].get_str();
	strCategory = params[2].get_str();
	CAmount nBalance = AssetAmountFromValue(params[3]);
	float fInterestRate = params[4].get_real();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[5]);
    // this is a syscoind txn
    CWalletTx wtx;
    CScript scriptPubKeyOrig;
	CAsset theAsset;
	
    if (!GetAsset( vchAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2504 - " + _("Could not find a asset with this key"));

	CAliasIndex theAlias;

	if (!GetAlias(theAsset.vchAlias, theAlias))
		throw runtime_error("SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2505 - " + _("Failed to read alias from alias DB"));

	CAsset copyAsset = theAsset;
	theAsset.ClearAsset();
	CSyscoinAddress aliasAddress;
	GetAddress(theAlias, &aliasAddress, scriptPubKeyOrig);

    // create ASSETUPDATE txn keys
    CScript scriptPubKey;

	if(strPubData != stringFromVch(theAsset.vchPubData))
		theAsset.vchPubData = vchFromString(strPubData);
	if(strCategory != stringFromVch(theAsset.sCategory))
		theAsset.sCategory = vchFromString(strCategory);

	theAsset.nBalance = nBalance;
	theAsset.fInterestRate = fInterestRate;
	// if using input ranges merge in the new balance
	if (copyAsset.bUseInputRanges && nBalance > 0)
	{
		unsigned int balance = (nBalance/COIN);
		CRange range(0, balance-1);
		if (!copyAsset.listAllocationInputs.empty()) {
			range.start = (copyAsset.nTotalSupply/COIN);
			range.end = range.start+(balance-1);
		}
		theAsset.listAllocationInputs.push_back(range);
	}

	vector<unsigned char> data;
	theAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_UPDATE) << vchHashAsset << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);
	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << theAlias.vchAlias << theAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyOrig, aliasPaymentRecipient);
		
	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;	
	SendMoneySyscoin(theAlias.vchAlias, vchWitness, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
 	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}

UniValue assettransfer(const UniValue& params, bool fHelp) {
 if (fHelp || params.size() != 3)
        throw runtime_error(
			"assettransfer [asset] [alias] [witness]\n"
						"Transfer a asset allocation you own to another alias.\n"
						"<asset> Asset name.\n"
						"<alias> alias to transfer to.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"	
						+ HelpRequiringPassphrase());

    // gather & validate inputs
	vector<unsigned char> vchAsset = vchFromValue(params[0]);
	vector<unsigned char> vchAlias = vchFromValue(params[1]);

	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[2]);
	// check for alias existence in DB
	CAliasIndex toAlias;
	if (!GetAlias(vchAlias, toAlias))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2509 - " + _("Failed to read transfer alias from DB"));

    // this is a syscoin txn
    CWalletTx wtx;
    CScript scriptPubKeyOrig, scriptPubKeyFromOrig;

	CAsset theAsset;
    if (!GetAsset( vchAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2510 - " + _("Could not find a asset with this key"));

	CAliasIndex fromAlias;
	if(!GetAlias(theAsset.vchAlias, fromAlias))
	{
		 throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2511 - " + _("Could not find the asset alias"));
	}

	CSyscoinAddress sendAddr;
	GetAddress(toAlias, &sendAddr, scriptPubKeyOrig);
	CSyscoinAddress fromAddr;
	GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);

	CAsset copyAsset = theAsset;
	theAsset.ClearAsset();
    CScript scriptPubKey;
	theAsset.vchAlias = toAlias.vchAlias;


	vector<unsigned char> data;
	theAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_TRANSFER) << vchHashAsset << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
    // send the asset pay txn
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << fromAlias.vchAlias << fromAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyFromOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyFromOrig, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	
	
	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(fromAlias.vchAlias, vchWitness, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}
UniValue assetsend(const UniValue& params, bool fHelp) {
	if (fHelp || params.size() != 5)
		throw runtime_error(
			"assetsend [asset] [aliasfrom] ( [{\"aliasto\":\"aliasname\",\"amount\":amount},...] or [{\"aliasto\":\"aliasname\",\"ranges\":[{\"start\":index,\"end\":index},...]},...] ) [memo] [witness]\n"
			"Send an asset you own to another alias as an asset allocation.\n"
			"<asset> Asset name.\n"
			"<aliasfrom> Alias to transfer from.\n"
			"<aliasto> Alias to transfer to.\n"
			"<amount> Quantity of asset to send.\n"
			"<ranges> Ranges of inputs to send in integers specified in the start and end fields.\n"
			"<memo> Message to include in this asset allocation transfer.\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction.\n"
			"The third parameter can be either an array of alias and amounts if sending amount pairs or an array of alias and array of start/end pairs of indexes for input ranges.\n"
			+ HelpRequiringPassphrase());

	// gather & validate inputs
	vector<unsigned char> vchAsset = vchFromValue(params[0]);
	vector<unsigned char> vchAliasFrom = vchFromValue(params[1]);
	UniValue valueTo = params[2];
	vector<unsigned char> vchMemo = vchFromValue(params[3]);
	vector<unsigned char> vchWitness = vchFromValue(params[4]);
	if (!valueTo.isArray())
		throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Array of receivers not found");

	// check for alias existence in DB
	CAliasIndex fromAlias;
	if (!GetAlias(vchAliasFrom, fromAlias))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2509 - " + _("Failed to read transfer alias from DB"));

	// this is a syscoin txn
	CWalletTx wtx;
	CScript scriptPubKeyFromOrig;

	CAsset theAsset;
	if (!GetAsset(vchAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2510 - " + _("Could not find a asset with this key"));

	CAliasIndex toAlias;
	CAssetAllocation theAssetAllocation;
	theAssetAllocation.vchAsset = vchAsset;
	theAssetAllocation.vchMemo = vchMemo;

	UniValue receivers = valueTo.get_array();
	for (unsigned int idx = 0; idx < receivers.size(); idx++) {
		const UniValue& receiver = receivers[idx];
		if (!receiver.isObject())
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"alias'\",\"inputranges\" or \"amount\"}");

	
		UniValue receiverObj = receiver.get_obj();
		vector<unsigned char> vchAliasTo = vchFromValue(find_value(receiverObj, "aliasto"));
		if (!GetAlias(vchAliasTo, toAlias))
			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2509 - " + _("Failed to read recipient alias from DB"));

		UniValue inputRangeObj = find_value(receiverObj, "ranges");
		UniValue amountObj = find_value(receiverObj, "amount");
		if (inputRangeObj.isArray()) {
			UniValue inputRanges = inputRangeObj.get_array();
			vector<CRange> vectorOfRanges;
			for (unsigned int rangeIndex = 0; rangeIndex < inputRanges.size(); rangeIndex++) {
				const UniValue& inputRangeObj = inputRanges[rangeIndex];
				if (!inputRangeObj.isObject())
					throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"start'\",\"end\"}");
				UniValue startRangeObj = find_value(inputRangeObj, "start");
				UniValue endRangeObj = find_value(inputRangeObj, "end");
				if (!startRangeObj.isNum())
					throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "start range not found for an input");
				if (!endRangeObj.isNum())
					throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "end range not found for an input");
				vectorOfRanges.push_back(CRange(startRangeObj.get_int(), endRangeObj.get_int()));
			}
			theAssetAllocation.listSendingAllocationInputs.push_back(make_pair(vchAliasTo, vectorOfRanges));
		}
		else if (amountObj.isNum()){
			const CAmount &amount = AssetAmountFromValue(amountObj);
			if (amount <= 0)
				throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
			theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(vchAliasTo, amount));
		}
		else
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected inputrange as string or amount as number in receiver array");

	}

	CSyscoinAddress fromAddr;
	GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);

	CScript scriptPubKey;

	CAssetAllocationTuple assetAllocationTuple(vchAsset, vchAliasFrom);
	if (!GetBoolArg("-unittest", false)) {
		// check to see if a transaction for this asset/alias tuple has arrived before minimum latency period
		ArrivalTimesMap arrivalTimes;
		passetallocationdb->ReadISArrivalTimes(assetAllocationTuple, arrivalTimes);
		const int64_t & nNow = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
		for (auto& arrivalTime : arrivalTimes) {
			// if this tx arrived within the minimum latency period flag it as potentially conflicting
			if ((nNow - (arrivalTime.second / 1000)) < ZDAG_MINIMUM_LATENCY_SECONDS) {
				throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2510 - " + _("Please wait a few more seconds and try again..."));
			}
		}
	}
	if (assetAllocationConflicts.find(assetAllocationTuple) != assetAllocationConflicts.end())
		throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 2510 - " + _("This asset allocation is involved in a conflict which must be resolved with Proof-Of-Work. Please wait for a block confirmation and try again..."));


	vector<unsigned char> data;
	theAssetAllocation.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());

	vector<unsigned char> vchHashAsset = vchFromValue(hash.GetHex());
	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_SEND) << vchHashAsset << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyFromOrig;
	// send the asset pay txn
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CScript scriptPubKeyAlias;
	scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << fromAlias.vchAlias << fromAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
	scriptPubKeyAlias += scriptPubKeyFromOrig;
	CRecipient aliasRecipient;
	CreateRecipient(scriptPubKeyAlias, aliasRecipient);
	CRecipient aliasPaymentRecipient;
	CreateAliasRecipient(scriptPubKeyFromOrig, aliasPaymentRecipient);

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	CCoinControl coinControl;
	coinControl.fAllowOtherInputs = false;
	coinControl.fAllowWatchOnly = false;
	SendMoneySyscoin(fromAlias.vchAlias, vchWitness, aliasRecipient, aliasPaymentRecipient, vecSend, wtx, &coinControl);
	UniValue res(UniValue::VARR);
	res.push_back(EncodeHexTx(wtx));
	return res;
}

UniValue assetinfo(const UniValue& params, bool fHelp) {
    if (fHelp || 2 != params.size())
        throw runtime_error("assetinfo <asset> <getinputs>\n"
                "Show stored values of a single asset and its. Set getinputs to true if you want to get the allocation inputs, if applicable.\n");

    vector<unsigned char> vchAsset = vchFromValue(params[0]);
	bool bGetInputs = params[1].get_bool();
	UniValue oAsset(UniValue::VOBJ);

	CAsset txPos;
	if (!passetdb || !passetdb->ReadAsset(vchAsset, txPos))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 5536 - " + _("Failed to read from asset DB"));

	if(!BuildAssetJson(txPos, bGetInputs, oAsset))
		oAsset.clear();
    return oAsset;
}
bool BuildAssetJson(const CAsset& asset, const bool bGetInputs, UniValue& oAsset)
{
    oAsset.push_back(Pair("_id", stringFromVch(asset.vchAsset)));
    oAsset.push_back(Pair("txid", asset.txHash.GetHex()));
    oAsset.push_back(Pair("height", (int)asset.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= asset.nHeight) {
		CBlockIndex *pindex = chainActive[asset.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oAsset.push_back(Pair("time", nTime));
	oAsset.push_back(Pair("publicvalue", stringFromVch(asset.vchPubData)));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.vchAlias)));
	oAsset.push_back(Pair("balance", ValueFromAmount(asset.nBalance)));
	oAsset.push_back(Pair("total_supply", ValueFromAmount(asset.nTotalSupply)));
	oAsset.push_back(Pair("max_supply", asset.nMaxSupply));
	oAsset.push_back(Pair("interest_rate", asset.fInterestRate));
	oAsset.push_back(Pair("can_adjust_interest_rate", asset.bCanAdjustInterestRate));

	if (bGetInputs) {
		UniValue oAssetAllocationInputsArray(UniValue::VARR);
		for (auto& input : asset.listAllocationInputs) {
			UniValue oAssetAllocationInputObj(UniValue::VOBJ);
			oAssetAllocationInputObj.push_back(Pair("start", (int)input.start));
			oAssetAllocationInputObj.push_back(Pair("end", (int)input.end));
			oAssetAllocationInputsArray.push_back(oAssetAllocationInputObj);
		}
		oAsset.push_back(Pair("inputs", oAssetAllocationInputsArray));
	}
	return true;
}
bool BuildAssetIndexerHistoryJson(const CAsset& asset, UniValue& oAsset)
{
	oAsset.push_back(Pair("_id", asset.txHash.GetHex()));
	oAsset.push_back(Pair("asset", stringFromVch(asset.vchAsset)));
	oAsset.push_back(Pair("height", (int)asset.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= asset.nHeight) {
		CBlockIndex *pindex = chainActive[asset.nHeight];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oAsset.push_back(Pair("time", nTime));
	oAsset.push_back(Pair("publicvalue", stringFromVch(asset.vchPubData)));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.vchAlias)));
	return true;
}
bool BuildAssetIndexerJson(const CAsset& asset, UniValue& oAsset)
{
	oAsset.push_back(Pair("_id", stringFromVch(asset.vchAsset)));
	oAsset.push_back(Pair("height", (int)asset.nHeight));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.vchAlias)));
	return true;
}
void AssetTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry)
{
	string opName = assetFromOp(op);
	CAsset asset;
	if(!asset.UnserializeFromData(vchData, vchHash))
		return;

	CAsset dbAsset;
	GetAsset(asset.vchAsset, dbAsset);
	

	entry.push_back(Pair("txtype", opName));
	entry.push_back(Pair("_id", stringFromVch(asset.vchAsset)));

	if(!asset.vchPubData.empty() && asset.vchPubData != dbAsset.vchPubData)
		entry.push_back(Pair("publicvalue", stringFromVch(asset.vchPubData)));

	if(!asset.vchAlias.empty() && asset.vchAlias != dbAsset.vchAlias)
		entry.push_back(Pair("alias", stringFromVch(asset.vchAlias)));

	if (!asset.sCategory.empty() && asset.sCategory != dbAsset.sCategory)
		entry.push_back(Pair("category", stringFromVch(asset.sCategory)));

	if (asset.fInterestRate != dbAsset.fInterestRate)
		entry.push_back(Pair("interest_rate", asset.fInterestRate));

	if (asset.nBalance != dbAsset.nBalance)
		entry.push_back(Pair("balance", ValueFromAmount(asset.nBalance)));

}
CAmount AssetAmountFromValue(const UniValue& value)
{
	if (!value.isNum() && !value.isStr())
		throw JSONRPCError(RPC_TYPE_ERROR, "Amount is not a number or string");
	CAmount amount;
	if (!ParseFixedPoint(value.getValStr(), 8, &amount))
		throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
	if (!AssetRange(amount))
		throw JSONRPCError(RPC_TYPE_ERROR, "Amount out of range");
	return amount;
}




