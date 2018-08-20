// Copyright (c) 2017-2018 The Syscoin Core developers
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
#include "wallet/coincontrol.h"
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp> // for to_upper()
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <chrono>
using namespace std::chrono;
using namespace std;
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
		vector<unsigned char> vchSerializedData;
		Serialize(vchSerializedData);
		const uint256 &calculatedHash = Hash(vchSerializedData.begin(), vchSerializedData.end());
		const vector<unsigned char> &vchRand = vchFromValue(calculatedHash.GetHex());
		if (vchRand != vchHash) {
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
	if (IsArgSet("-zmqpubassetrecord")) {
		UniValue oName(UniValue::VOBJ);
		if (BuildAssetIndexerJson(asset, oName)) {
			GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "assetrecord");
		}
	}
	WriteAssetIndexHistory(asset, op);
}
void CAssetDB::WriteAssetIndexHistory(const CAsset& asset, const int &op) {
	if (IsArgSet("-zmqpubassethistory")) {
		UniValue oName(UniValue::VOBJ);
		if (BuildAssetIndexerHistoryJson(asset, oName)) {
			oName.push_back(Pair("op", assetFromOp(op)));
			GetMainSignals().NotifySyscoinUpdate(oName.write().c_str(), "assethistory");
		}
	}
}
bool GetAsset(const vector<unsigned char> &vchAsset,
        CAsset& txPos) {
    if (!passetdb || !passetdb->ReadAsset(vchAsset, txPos))
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
		type = ASSET;
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
		vvch.emplace_back(std::move(vch));
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
bool CheckAssetInputs(const CTransaction &tx, const CCoinsViewCache &inputs, int op, const vector<vector<unsigned char> > &vvchArgs, const vector<unsigned char> &vchAlias,
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
		errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR ERRCODE: 2000 - " + _("Cannot unserialize data inside of this transaction relating to an asset");
		return true;
	}

	if(fJustCheck)
	{
		if(vvchArgs.size() != 1)
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2001 - " + _("Asset arguments incorrect size");
			return error(errorMessage.c_str());
		}

					
		if(vchHash != vvchArgs[0])
		{
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2002 - " + _("Hash provided doesn't match the calculated hash of the data");
			return true;
		}
			
	}

	string retError = "";
	if(fJustCheck)
	{
		if (op != OP_ASSET_SEND) {
			if (theAsset.sCategory.size() > MAX_NAME_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2003 - " + _("Asset category too big");
				return error(errorMessage.c_str());
			}
			if (theAsset.vchPubData.size() > MAX_VALUE_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2004 - " + _("Asset public data too big");
				return error(errorMessage.c_str());
			}
		}
		switch (op) {
		case OP_ASSET_ACTIVATE:
			if (theAsset.vchAsset.size() > MAX_GUID_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2005 - " + _("asset guid too long");
				return error(errorMessage.c_str());
			}
			if (theAsset.vchSymbol.size() < MIN_SYMBOL_LENGTH || theAsset.vchSymbol.size() > MAX_SYMBOL_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2006 - " + _("asset symbol must be between 1 and 8 characters in length");
				return error(errorMessage.c_str());
			}
			if(!boost::algorithm::starts_with(stringFromVch(theAsset.sCategory), "assets"))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2007 - " + _("Must use a asset category");
				return error(errorMessage.c_str());
			}
			if(theAsset.bUseInputRanges && theAsset.listAllocationInputs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2008 - " + _("Must specify input range");
				return error(errorMessage.c_str());
			}
			if (!theAsset.bUseInputRanges && !theAsset.listAllocationInputs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2009 - " + _("Cannot specify input range for this asset");
				return error(errorMessage.c_str());
			}

			if (theAsset.fInterestRate < 0 || theAsset.fInterestRate > 1)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2010 - " + _("Interest must be between 0 and 1");
				return error(errorMessage.c_str());
			}
			if ((theAsset.fInterestRate != 0 || theAsset.bCanAdjustInterestRate) && theAsset.bUseInputRanges)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2011 - " + _("Interest cannot be set on this type of asset");
				return error(errorMessage.c_str());
			}
			if (!AssetRange(theAsset.nBalance, theAsset.nPrecision, theAsset.bUseInputRanges))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2012 - " + _("Initial balance out of money range");
				return true;
			}
			if (theAsset.nPrecision > 8)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2013 - " + _("Precision must be between 0 and 8");
				return true;
			}
			if (theAsset.nMaxSupply != -1 && !AssetRange(theAsset.nMaxSupply, theAsset.nPrecision, theAsset.bUseInputRanges))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2014 - " + _("Max supply out of money range");
				return true;
			}
			if (theAsset.nBalance > theAsset.nMaxSupply)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Total supply cannot exceed maximum supply");
				return true;
			}
			if(nHeight < Params().GetConsensus().nShareFeeBlock && CSyscoinAddress(stringFromVch(theAsset.vchAliasOrAddress)).IsValid())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2015 - " + _("Please wait until the fork to create this type of asset. It is on block: ") + boost::lexical_cast<string>(Params().GetConsensus().nShareFeeBlock);
				return true;
			}
			break;

		case OP_ASSET_UPDATE:
			if(theAsset.sCategory.size() > 0 && !boost::algorithm::istarts_with(stringFromVch(theAsset.sCategory), "assets"))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2016 - " + _("Must use a asset category");
				return error(errorMessage.c_str());
			}
			if (theAsset.nBalance < 0)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2017 - " + _("Balance must be greator than or equal to 0");
				return error(errorMessage.c_str());
			}
			if (theAsset.fInterestRate < 0 || theAsset.fInterestRate > 1)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2018 - " + _("Interest must be between 0 and 1");
				return error(errorMessage.c_str());
			}
			break;

		case OP_ASSET_TRANSFER:
			if (!theAssetAllocation.listSendingAllocationInputs.empty() || !theAssetAllocation.listSendingAllocationAmounts.empty() || !theAsset.listAllocationInputs.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2019 - " + _("Cannot transfer input allocations");
				return error(errorMessage.c_str());
			}
			break;
		case OP_ASSET_SEND:
			if (theAssetAllocation.listSendingAllocationInputs.empty() && theAssetAllocation.listSendingAllocationAmounts.empty())
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2020 - " + _("Asset send must send an input or transfer balance");
				return error(errorMessage.c_str());
			}
			if (theAssetAllocation.listSendingAllocationInputs.size() > 250 || theAssetAllocation.listSendingAllocationAmounts.size() > 250)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2021 - " + _("Too many receivers in one allocation send, maximum of 250 is allowed at once");
				return error(errorMessage.c_str());
			}
			if (theAssetAllocation.vchMemo.size() > MAX_MEMO_LENGTH)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2022 - " + _("memo too long, must be 128 character or less");
				return error(errorMessage.c_str());
			}
			break;
		default:
			errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2023 - " + _("Asset transaction has unknown op");
			return error(errorMessage.c_str());
		}
	}
	if (!fJustCheck) {
		string strResponseEnglish = "";
		string strResponseGUID = "";
		CTransaction txTmp;
		GetSyscoinTransactionDescription(txTmp, op, strResponseEnglish, ASSET, strResponseGUID);
		CAsset dbAsset;
		if (!GetAsset(op == OP_ASSET_SEND ? theAssetAllocation.vchAsset : theAsset.vchAsset, dbAsset))
		{
			if (op != OP_ASSET_ACTIVATE) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2024 - " + _("Failed to read from asset DB");
				return true;
			}
		}
		const vector<unsigned char> &vchOwner = op == OP_ASSET_SEND ? theAssetAllocation.vchAliasOrAddress : theAsset.vchAliasOrAddress;
		const vector<unsigned char> &vchThisAlias = vchAlias.empty() ? vchOwner : vchAlias;
		const string &user1 = stringFromVch(dbAsset.IsNull()? vchThisAlias : dbAsset.vchAliasOrAddress);
		string user2 = "";
		string user3 = "";
		if (op == OP_ASSET_TRANSFER) {
			user2 = stringFromVch(vchThisAlias);
		}

		if (op == OP_ASSET_UPDATE) {
			if (vchAlias.empty()) {
				if (!FindAssetOwnerInTx(inputs, tx, user1))
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot update this asset. Asset owner must sign off on this change");
					return true;
				}
			}
			else if (dbAsset.vchAliasOrAddress != vchAlias) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot update this asset. Asset owner must sign off on this change");
				return true;
			}

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
					if(input.start < dbAsset.nTotalSupply)
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2027 - " + _("Cannot edit this asset. New asset inputs must be added to the end of the supply: ") + boost::lexical_cast<std::string>(input.start) + " vs " + boost::lexical_cast<std::string>(dbAsset.nTotalSupply);
						return true;
					}
				}
				vector<CRange> outputMerge;
				increaseBalanceByAmount = validateRangesAndGetCount(theAsset.listAllocationInputs);
				if (increaseBalanceByAmount == 0)
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2028 - " + _("Invalid input ranges");
					return true;
				}
				dbAsset.listAllocationInputs.insert(std::end(dbAsset.listAllocationInputs), std::begin(theAsset.listAllocationInputs), std::end(theAsset.listAllocationInputs));
				mergeRanges(dbAsset.listAllocationInputs, outputMerge);
				theAsset.listAllocationInputs = outputMerge;
			}
			theAsset.nBalance += increaseBalanceByAmount;
			// increase total supply
			theAsset.nTotalSupply += increaseBalanceByAmount;
			if (!AssetRange(theAsset.nTotalSupply, dbAsset.nPrecision, dbAsset.bUseInputRanges))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2029 - " + _("Total supply out of money range");
				return true;
			}
			if (theAsset.nTotalSupply > dbAsset.nMaxSupply)
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2030 - " + _("Total supply cannot exceed maximum supply");
				return true;
			}

		}
		else if (op != OP_ASSET_ACTIVATE) {
			// these fields cannot change after activation
			theAsset.nBalance = dbAsset.nBalance;
			theAsset.nTotalSupply = dbAsset.nBalance;
			theAsset.nMaxSupply = dbAsset.nMaxSupply;
		}

		if (op == OP_ASSET_SEND) {
			LOCK(cs_assetallocation);
			if (!vchAlias.empty() && CSyscoinAddress(user1).IsValid()) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1016 - " + _("This asset cannot be sent because owner is an alias but the alias is also a valid syscoin address");
				return true;
			}
			if (vchAlias.empty()) {
				if (dbAsset.vchAliasOrAddress != theAssetAllocation.vchAliasOrAddress || !FindAssetOwnerInTx(inputs, tx, user1))
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot send this asset. Asset owner must sign off on this change");
					return true;
				}
			}
			else if (dbAsset.vchAliasOrAddress != vchAlias) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot send this asset. Asset owner must sign off on this change");
				return true;
			}
			theAsset = dbAsset;

			CAssetAllocation dbAssetAllocation;
			const CAssetAllocationTuple allocationTuple(theAssetAllocation.vchAsset, dbAsset.vchAliasOrAddress);
			GetAssetAllocation(allocationTuple, dbAssetAllocation);
			if (!theAssetAllocation.listSendingAllocationAmounts.empty()) {
				if (dbAsset.bUseInputRanges) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2031 - " + _("Invalid asset send, request to send amounts but asset uses input ranges");
					return true;
				}
				// check balance is sufficient on sender
				CAmount nTotal = 0;
				for (auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
					nTotal += amountTuple.second;
					if (amountTuple.second <= 0)
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2032 - " + _("Receiving amount must be positive");
						return true;
					}
				}
				if (theAsset.nBalance < nTotal) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2033 - " + _("Sender balance is insufficient");
					return true;
				}
				for (auto& amountTuple : theAssetAllocation.listSendingAllocationAmounts) {
					if (!bSanityCheck) {
						CAssetAllocation receiverAllocation;
						const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.vchAsset, amountTuple.first);
						// don't need to check for existance of allocation because it may not exist, may be creating it here for the first time for receiver
						GetAssetAllocation(receiverAllocationTuple, receiverAllocation);
						if (receiverAllocation.IsNull()) {
							receiverAllocation.vchAliasOrAddress = receiverAllocationTuple.vchAliasOrAddress;
							receiverAllocation.vchAsset = receiverAllocationTuple.vchAsset;
							receiverAllocation.nLastInterestClaimHeight = nHeight;
							receiverAllocation.nHeight = nHeight;
							receiverAllocation.fInterestRate = dbAsset.fInterestRate;
						}
						receiverAllocation.txHash = tx.GetHash();
						if (theAsset.fInterestRate > 0) {
							AccumulateInterestSinceLastClaim(receiverAllocation, nHeight);
						}
						receiverAllocation.fInterestRate = theAsset.fInterestRate;
						receiverAllocation.nHeight = nHeight;
						receiverAllocation.vchMemo = theAssetAllocation.vchMemo;
						receiverAllocation.nBalance += amountTuple.second;
						const string& receiverAddress = stringFromVch(receiverAllocation.vchAliasOrAddress);
						// adjust sender balance
						theAsset.nBalance -= amountTuple.second;
						if (!passetallocationdb->WriteAssetAllocation(receiverAllocation, dbAsset.nBalance - nTotal, amountTuple.second, dbAsset, INT64_MAX, user1, receiverAddress, fJustCheck))
						{
							errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2034 - " + _("Failed to write to asset allocation DB");
							continue;
						}
					}
				}
			}
			else if (!theAssetAllocation.listSendingAllocationInputs.empty()) {
				if (!dbAsset.bUseInputRanges) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2035 - " + _("Invalid asset send, request to send input ranges but asset uses amounts");
					return true;
				}
				// check balance is sufficient on sender
				CAmount nTotal = 0;
				vector<CAmount> rangeTotals;
				for (auto& inputTuple : theAssetAllocation.listSendingAllocationInputs) {
					const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.vchAsset, inputTuple.first);
					const unsigned int &rangeTotal = validateRangesAndGetCount(inputTuple.second);
					if (rangeTotal == 0)
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2036 - " + _("Invalid input range");
						return true;
					}
					const CAmount rangeTotalAmount = rangeTotal;
					rangeTotals.emplace_back(std::move(rangeTotalAmount));
					nTotal += rangeTotals.back();
				}
				if (theAsset.nBalance < nTotal) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2037 - " + _("Sender balance is insufficient");
					return true;
				}
				for (unsigned int i = 0; i < theAssetAllocation.listSendingAllocationInputs.size(); i++) {
					InputRanges &input = theAssetAllocation.listSendingAllocationInputs[i];
					CAssetAllocation receiverAllocation;

					const CAssetAllocationTuple receiverAllocationTuple(theAssetAllocation.vchAsset, input.first);
					// ensure entire allocation range being subtracted exists on sender (full inclusion check)
					if (!doesRangeContain(dbAsset.listAllocationInputs, input.second))
					{
						errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2038 - " + _("Input not found");
						return true;
					}
					if (!bSanityCheck) {						
						if (!GetAssetAllocation(receiverAllocationTuple, receiverAllocation)) {
							receiverAllocation.vchAliasOrAddress = receiverAllocationTuple.vchAliasOrAddress;
							receiverAllocation.vchAsset = receiverAllocationTuple.vchAsset;
							receiverAllocation.nLastInterestClaimHeight = nHeight;
							receiverAllocation.nHeight = nHeight;
							receiverAllocation.fInterestRate = dbAsset.fInterestRate;
						}

						receiverAllocation.txHash = tx.GetHash();
						receiverAllocation.fInterestRate = theAsset.fInterestRate;
						receiverAllocation.nHeight = nHeight;
						receiverAllocation.vchMemo = theAssetAllocation.vchMemo;
						// figure out receivers added ranges and balance
						vector<CRange> outputMerge;
						receiverAllocation.listAllocationInputs.insert(std::end(receiverAllocation.listAllocationInputs), std::begin(input.second), std::end(input.second));
						mergeRanges(receiverAllocation.listAllocationInputs, outputMerge);
						receiverAllocation.listAllocationInputs = outputMerge;
						receiverAllocation.nBalance += rangeTotals[i];

						const string& receiverAddress = stringFromVch(receiverAllocation.vchAliasOrAddress);
						// figure out senders subtracted ranges and balance
						vector<CRange> outputSubtract;
						subtractRanges(dbAsset.listAllocationInputs, input.second, outputSubtract);
						theAsset.listAllocationInputs = outputSubtract;
						theAsset.nBalance -= rangeTotals[i];
						if (!passetallocationdb->WriteAssetAllocation(receiverAllocation, dbAsset.nBalance - nTotal, rangeTotals[i], dbAsset, INT64_MAX, user1, receiverAddress, fJustCheck))
						{
							errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2039 - " + _("Failed to write to asset allocation DB");
							return error(errorMessage.c_str());
						}
					}
				}
			}
		}
		else if (op != OP_ASSET_ACTIVATE)
		{
			// these fields cannot change after activation
			theAsset.bUseInputRanges = dbAsset.bUseInputRanges;
			theAsset.bCanAdjustInterestRate = dbAsset.bCanAdjustInterestRate;
			theAsset.nPrecision = dbAsset.nPrecision;
			theAsset.vchSymbol = dbAsset.vchSymbol;
			if (theAsset.vchAliasOrAddress.empty())
				theAsset.vchAliasOrAddress = dbAsset.vchAliasOrAddress;
			if (theAsset.vchPubData.empty())
				theAsset.vchPubData = dbAsset.vchPubData;
			if (theAsset.sCategory.empty())
				theAsset.sCategory = dbAsset.sCategory;

			if (op == OP_ASSET_UPDATE) {
				if (!theAsset.bCanAdjustInterestRate && theAsset.fInterestRate != dbAsset.fInterestRate) {
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2040 - " + _("Cannot adjust interest rate for this asset");
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
			if (vchAlias.empty()) {
				if (!FindAssetOwnerInTx(inputs, tx, user1))
				{
					errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot create this asset. Asset owner must sign off on this change");
					return true;
				}
			}
			else if (theAsset.vchAliasOrAddress != vchAlias) {
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 1015 - " + _("Cannot create this asset. Asset owner must sign off on this change");
				return true;
			}
			string assetUpper = stringFromVch(theAsset.vchSymbol);
			boost::algorithm::to_upper(assetUpper);
			theAsset.vchSymbol = vchFromString(assetUpper);
			if (GetAsset(theAsset.vchAsset, theAsset))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2041 - " + _("Asset already exists");
				return true;
			}
			// starting supply is the supplied balance upon init
			theAsset.nTotalSupply = theAsset.nBalance;
			// with input ranges precision is forced to 0
			if(theAsset.bUseInputRanges)
				theAsset.nPrecision = 0;
		}
		// set the asset's txn-dependent values
		theAsset.nHeight = nHeight;
		theAsset.txHash = tx.GetHash();
		// write asset, if asset send, only write on pow since asset -> asset allocation is not 0-conf compatible
		if (!bSanityCheck) {
			if (!passetdb->WriteAsset(theAsset, op))
			{
				errorMessage = "SYSCOIN_ASSET_CONSENSUS_ERROR: ERRCODE: 2042 - " + _("Failed to write to asset DB");
				return error(errorMessage.c_str());
			}
			// debug
			if (fDebug)
				LogPrintf("CONNECTED ASSET: op=%s asset=%s symbol=%s hash=%s height=%d fJustCheck=%d\n",
					assetFromOp(op).c_str(),
					stringFromVch(op == OP_ASSET_SEND ? theAssetAllocation.vchAsset : theAsset.vchAsset).c_str(), stringFromVch(theAsset.vchSymbol).c_str(),
					tx.GetHash().ToString().c_str(),
					nHeight,
					fJustCheck ? 1 : 0);
		}
	}
    return true;
}

UniValue assetnew(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 11)
        throw runtime_error(
			"assetnew [symbol] [owner] [public value] [category=assets] [precision=8] [use_inputranges] [supply] [max_supply] [interest_rate] [can_adjust_interest_rate] [witness]\n"
						"<symbol> symbol of asset in uppercase, 1 characters miniumum, 8 characters max.\n"
						"<owner> An alias or address that you own.\n"
                        "<public value> public data, 256 characters max.\n"
						"<category> category, 256 characters max. Defaults to assets.\n"
						"<precision> Precision of balances. Must be between 0 and 8. The lower it is the higher possible max_supply is available since the supply is represented as a 64 bit integer. With a precision of 8 the max supply is 10 billion.\n"
						"<use_inputranges> If this asset uses an input for every token, useful if you need to keep track of a token regardless of ownership. If set to true, precision is forced to 0. Maximum supply with input ranges is 10 million.\n"
						"<supply> Initial supply of asset. Can mint more supply up to total_supply amount or if total_supply is -1 then minting is uncapped.\n"
						"<max_supply> Maximum supply of this asset. Set to -1 for uncapped. Depends on the precision value that is set, the lower the precision the higher max_supply can be.\n"
						"<interest_rate> The annual interest rate if any. Money supply is still capped to total supply. Should be between 0 and 1 and represents a percentage divided by 100.\n"
						"<can_adjust_interest_rate> Ability to adjust interest rate through assetupdate in the future.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction. Only applicable asset is owned by an alias.\n"
						+ HelpRequiringPassphrase());
    vector<unsigned char> vchName = vchFromString(params[0].get_str());
	string strName = stringFromVch(vchName);
	boost::algorithm::to_upper(strName);
	vector<unsigned char> vchAliasOrAddress = vchFromValue(params[1]);
	vector<unsigned char> vchPubData = vchFromString(params[2].get_str());
	string strCategory = "assets";
	strCategory = params[3].get_str();
	int precision = params[4].get_int();
	bool bUseInputRanges = params[5].get_bool();
	vector<unsigned char> vchWitness;
	UniValue param6 = params[6];
	UniValue param7 = params[7];
	CAmount nBalance = AssetAmountFromValue(param6, precision, bUseInputRanges);
	CAmount nMaxSupply = AssetAmountFromValue(param7, precision, bUseInputRanges);
	
	float fInterestRate = params[8].get_real();
	bool bCanAdjustInterestRate = params[9].get_bool();
	vchWitness = vchFromValue(params[10]);

	string strAddressFrom;
	string strAliasOrAddress = stringFromVch(vchAliasOrAddress);
	const CSyscoinAddress address(strAliasOrAddress);
	if (address.IsValid()) {
		strAddressFrom = strAliasOrAddress;
	}
	else {
		ToLowerCase(vchAliasOrAddress);
		strAliasOrAddress = stringFromVch(vchAliasOrAddress);
	}

	CScript scriptPubKeyFromOrig;
	CAliasIndex fromAlias;
	if (!strAddressFrom.empty()) {
		scriptPubKeyFromOrig = GetScriptForDestination(address.Get());
	}
	else {
		// check for alias existence in DB
		if (!GetAlias(vchAliasOrAddress, fromAlias))
			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1502 - " + _("Failed to read sender alias from DB"));
		CSyscoinAddress fromAddr;
		GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);
	}

    CScript scriptPubKey;

	// calculate net
    // build asset object
    CAsset newAsset;
	newAsset.vchSymbol = vchFromString(strName);
	newAsset.vchAsset = vchFromString(GenerateSyscoinGuid());
	newAsset.sCategory = vchFromString(strCategory);
	newAsset.vchPubData = vchPubData;
	newAsset.vchAliasOrAddress = vchAliasOrAddress;
	newAsset.nBalance = nBalance;
	newAsset.nMaxSupply = nMaxSupply;
	newAsset.bUseInputRanges = bUseInputRanges;
	newAsset.fInterestRate = fInterestRate;
	newAsset.nPrecision = precision;
	newAsset.bCanAdjustInterestRate = bCanAdjustInterestRate;
	if (bUseInputRanges)
	{
		CRange range(0, nBalance - 1);
		newAsset.listAllocationInputs.push_back(range);
	}
	vector<unsigned char> data;
	newAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromString(hash.GetHex());

    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_ACTIVATE) << vchHashAsset << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyFromOrig;

	// use the script pub key to create the vecsend which sendmoney takes and puts it into vout
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CRecipient aliasRecipient;
	if (strAddressFrom.empty()) {
		CScript scriptPubKeyAlias;
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << fromAlias.vchAlias << fromAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += scriptPubKeyFromOrig;
		CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);
	}

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);

	UniValue res = syscointxfund_helper(vchAliasOrAddress, vchWitness, aliasRecipient, vecSend);
	res.push_back(stringFromVch(newAsset.vchAsset));
	return res;
}

UniValue assetupdate(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || params.size() != 6)
        throw runtime_error(
			"assetupdate [asset] [public value] [category=assets] [supply] [interest_rate] [witness]\n"
						"Perform an update on an asset you control.\n"
						"<asset> Asset guid.\n"
                        "<public value> Public data, 256 characters max.\n"                
						"<category> Category, 256 characters max. Defaults to assets\n"
						"<supply> New supply of asset. Can mint more supply up to total_supply amount or if max_supply is -1 then minting is uncapped. If greator than zero, minting is assumed otherwise set to 0 to not mint any additional tokens.\n"
						"<interest_rate> The annual interest rate if any. Money supply is still capped to total supply. Should be between 0 and 1 and represents a percentage divided by 100. Can only set if this asset allows adjustment of interest rate.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction. Only applicable asset is owned by an alias.\n"
						+ HelpRequiringPassphrase());
	vector<unsigned char> vchAsset = vchFromValue(params[0]);
	string strData = "";
	string strPubData = "";
	string strCategory = "";
	strPubData = params[1].get_str();
	strCategory = params[2].get_str();
	
	float fInterestRate = params[4].get_real();
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[5]);

    CScript scriptPubKeyFromOrig;
	CAsset theAsset;
	string strAddress;

    if (!GetAsset( vchAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2501 - " + _("Could not find a asset with this key"));

	string strAddressFrom;
	const string& strAliasOrAddress = stringFromVch(theAsset.vchAliasOrAddress);
	const CSyscoinAddress address(strAliasOrAddress);
	if (address.IsValid()) {
		strAddressFrom = strAliasOrAddress;
	}

	UniValue param3 = params[3];
	CAmount nBalance = 0;
	if(param3.get_str() != "0")
		nBalance = AssetAmountFromValue(param3, theAsset.nPrecision, theAsset.bUseInputRanges);
	CAliasIndex theAlias;
	
	CAliasIndex fromAlias;
	if (!strAddressFrom.empty()) {
		scriptPubKeyFromOrig = GetScriptForDestination(address.Get());
	}
	else {
		if (!GetAlias(theAsset.vchAliasOrAddress, fromAlias))
			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1502 - " + _("Failed to read sender alias from DB"));
		CSyscoinAddress fromAddr;
		GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);
	}
	CAsset copyAsset = theAsset;
	theAsset.ClearAsset();

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
		unsigned int balance = nBalance;
		CRange range;
		range.start = copyAsset.nTotalSupply;
		range.end = range.start+(balance-1);
		theAsset.listAllocationInputs.push_back(range);
	}

	vector<unsigned char> data;
	theAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromString(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_UPDATE) << vchHashAsset << OP_2DROP << OP_DROP;
    scriptPubKey += scriptPubKeyFromOrig;

	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CRecipient aliasRecipient;
	if (strAddressFrom.empty()) {
		CScript scriptPubKeyAlias;
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << fromAlias.vchAlias << fromAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += scriptPubKeyFromOrig;
		CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);
	}

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	
	
	return syscointxfund_helper(vchFromString(strAliasOrAddress), vchWitness, aliasRecipient, vecSend);
}

UniValue assettransfer(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
 if (request.fHelp || params.size() != 3)
        throw runtime_error(
			"assettransfer [asset] [ownerto] [witness]\n"
						"Transfer a asset allocation you own to another address.\n"
						"<asset> Asset guid.\n"
						"<ownerto> Alias or address to transfer to.\n"
						"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction. Only applicable asset is owned by an alias.\n"	
						+ HelpRequiringPassphrase());

    // gather & validate inputs
	vector<unsigned char> vchAsset = vchFromValue(params[0]);
	vector<unsigned char> vchAliasOrAddressTo = vchFromValue(params[1]);
	vector<unsigned char> vchWitness;
	vchWitness = vchFromValue(params[2]);

    CScript scriptPubKeyOrig, scriptPubKeyFromOrig;
	string strAddress;
	CAsset theAsset;
    if (!GetAsset( vchAsset, theAsset))
        throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2505 - " + _("Could not find a asset with this key"));
	
	string strAddressFrom;
	const string& strAliasOrAddress = stringFromVch(theAsset.vchAliasOrAddress);
	const CSyscoinAddress addressFrom(strAliasOrAddress);
	if (addressFrom.IsValid()) {
		strAddressFrom = strAliasOrAddress;
	}


	const CSyscoinAddress addressTo(stringFromVch(vchAliasOrAddressTo));
	if (addressTo.IsValid()) {
		scriptPubKeyOrig = GetScriptForDestination(addressTo.Get());
	}
	else {
		CAliasIndex toAlias;
		// check for alias existence in DB
		ToLowerCase(vchAliasOrAddressTo);
		if (!GetAlias(vchAliasOrAddressTo, toAlias))
			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1502 - " + _("Failed to read receiving alias from DB"));
		CSyscoinAddress toAddr;
		GetAddress(toAlias, &toAddr, scriptPubKeyOrig);
	}

	CAliasIndex fromAlias;
	if (!strAddressFrom.empty()) {
		scriptPubKeyFromOrig = GetScriptForDestination(addressFrom.Get());
	}
	else {
		// check for alias existence in DB
		if (!GetAlias(theAsset.vchAliasOrAddress, fromAlias))
			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1502 - " + _("Failed to read sender alias from DB"));
		CSyscoinAddress fromAddr;
		GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);

	}
	

	CAsset copyAsset = theAsset;
	theAsset.ClearAsset();
    CScript scriptPubKey;
	
	theAsset.vchAliasOrAddress = vchAliasOrAddressTo;

	vector<unsigned char> data;
	theAsset.Serialize(data);
    uint256 hash = Hash(data.begin(), data.end());
 	
    vector<unsigned char> vchHashAsset = vchFromString(hash.GetHex());
    scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_TRANSFER) << vchHashAsset << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyOrig;
    // send the asset pay txn
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CRecipient aliasRecipient;
	if (strAddressFrom.empty()) {
		CScript scriptPubKeyAlias;
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << fromAlias.vchAlias << fromAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += scriptPubKeyFromOrig;
		CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);
	}

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);
	
	return syscointxfund_helper(vchFromString(strAliasOrAddress), vchWitness, aliasRecipient, vecSend);
}
UniValue assetsend(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
	if (request.fHelp || params.size() != 5)
		throw runtime_error(
			"assetsend [asset] [tmp] ([{\"ownerto\":\"aliasname or address\",\"amount\":amount},...]  or [{\"ownerto\":\"aliasname or address\",\"ranges\":[{\"start\":index,\"end\":index},...]},...]) [memo] [witness]\n"
			"Send an asset you own to another address/address as an asset allocation. Maximimum recipients is 250.\n"
			"<asset> Asset guid.\n"
			"<owner> Alias or address that owns this asset allocation.\n"
			"<ownerto> Alias or address to transfer to.\n"
			"<amount> Quantity of asset to send.\n"
			"<ranges> Ranges of inputs to send in integers specified in the start and end fields.\n"
			"<memo> Message to include in this asset allocation transfer.\n"
			"<witness> Witness alias name that will sign for web-of-trust notarization of this transaction. Only applicable asset is owned by an alias.\n"
			"The third parameter can be either an array of address and amounts if sending amount pairs or an array of address and array of start/end pairs of indexes for input ranges.\n"
			+ HelpRequiringPassphrase());
	// gather & validate inputs
	vector<unsigned char> vchAsset = vchFromValue(params[0]);
	UniValue valueTo = params[2];
	vector<unsigned char> vchMemo = vchFromValue(params[3]);
	vector<unsigned char> vchWitness = vchFromValue(params[4]);
	if (!valueTo.isArray())
		throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Array of receivers not found");

	CAsset theAsset;
	if (!GetAsset(vchAsset, theAsset))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2507 - " + _("Could not find a asset with this key"));

	string strAddressFrom;
	const string& strAliasOrAddress = stringFromVch(theAsset.vchAliasOrAddress);
	const CSyscoinAddress addressFrom(strAliasOrAddress);
	if (addressFrom.IsValid()) {
		strAddressFrom = strAliasOrAddress;
	}

	string strAddress;

	CScript scriptPubKeyFromOrig;
	CAliasIndex fromAlias;
	if (!strAddressFrom.empty()) {
		scriptPubKeyFromOrig = GetScriptForDestination(addressFrom.Get());
	}
	else {
		if (!GetAlias(theAsset.vchAliasOrAddress, fromAlias))
			throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1502 - " + _("Failed to read sender alias from DB"));
		CSyscoinAddress fromAddr;
		GetAddress(fromAlias, &fromAddr, scriptPubKeyFromOrig);
	}

	CAliasIndex toAlias;
	CAssetAllocation theAssetAllocation;
	theAssetAllocation.vchMemo = vchMemo;
	theAssetAllocation.vchAsset = vchAsset;
	theAssetAllocation.vchAliasOrAddress = theAsset.vchAliasOrAddress;

	UniValue receivers = valueTo.get_array();
	for (unsigned int idx = 0; idx < receivers.size(); idx++) {
		const UniValue& receiver = receivers[idx];
		if (!receiver.isObject())
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"ownerto'\",\"inputranges\" or \"amount\"}");

		UniValue receiverObj = receiver.get_obj();
		UniValue toObj = find_value(receiverObj, "ownerto");
		if(toObj.isNull())
			toObj = find_value(receiverObj, "aliasto");
		vector<unsigned char> vchAliasOrAddressTo;
		vchAliasOrAddressTo = vchFromValue(toObj);
		if (!CSyscoinAddress(stringFromVch(vchAliasOrAddressTo)).IsValid()) {
			ToLowerCase(vchAliasOrAddressTo);
			if (!GetAlias(vchAliasOrAddressTo, toAlias))
				throw runtime_error("SYSCOIN_ASSET_ALLOCATION_RPC_ERROR: ERRCODE: 1501 - " + _("Failed to read recipient alias from DB"));
		}

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
			theAssetAllocation.listSendingAllocationInputs.push_back(make_pair(vchAliasOrAddressTo, vectorOfRanges));
		}
		else if (amountObj.isNum()) {
			const CAmount &amount = AssetAmountFromValue(amountObj, theAsset.nPrecision, theAsset.bUseInputRanges);
			if (amount <= 0)
				throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "amount must be positive");
			theAssetAllocation.listSendingAllocationAmounts.push_back(make_pair(vchAliasOrAddressTo, amount));
		}
		else
			throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected inputrange as string or amount as number in receiver array");

	}

	CScript scriptPubKey;

	const CAssetAllocationTuple assetAllocationTuple(vchAsset, theAsset.vchAliasOrAddress);
	if (!fUnitTest) {
		// check to see if a transaction for this asset/address tuple has arrived before minimum latency period
		ArrivalTimesMap arrivalTimes;
		passetallocationdb->ReadISArrivalTimes(assetAllocationTuple, arrivalTimes);
		const int64_t & nNow = GetTimeMillis();
		for (auto& arrivalTime : arrivalTimes) {
			// if this tx arrived within the minimum latency period flag it as potentially conflicting
			if ((nNow - (arrivalTime.second / 1000)) < ZDAG_MINIMUM_LATENCY_SECONDS) {
				throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2509 - " + _("Please wait a few more seconds and try again..."));
			}
		}
	}
	if (assetAllocationConflicts.find(assetAllocationTuple) != assetAllocationConflicts.end())
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2510 - " + _("This asset allocation is involved in a conflict which must be resolved with Proof-Of-Work. Please wait for a block confirmation and try again..."));


	vector<unsigned char> data;
	theAssetAllocation.Serialize(data);
	uint256 hash = Hash(data.begin(), data.end());
	vector<unsigned char> vchHashAsset = vchFromString(hash.GetHex());
	if(!theAssetAllocation.UnserializeFromData(data, vchHashAsset))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2510 - " + _("Could not unserialize asset allocation data"));

	scriptPubKey << CScript::EncodeOP_N(OP_SYSCOIN_ASSET) << CScript::EncodeOP_N(OP_ASSET_SEND) << vchHashAsset << OP_2DROP << OP_DROP;
	scriptPubKey += scriptPubKeyFromOrig;
	// send the asset pay txn
	vector<CRecipient> vecSend;
	CRecipient recipient;
	CreateRecipient(scriptPubKey, recipient);
	vecSend.push_back(recipient);

	CRecipient aliasRecipient;
	if (strAddressFrom.empty()) {
		CScript scriptPubKeyAlias;
		scriptPubKeyAlias << CScript::EncodeOP_N(OP_SYSCOIN_ALIAS) << CScript::EncodeOP_N(OP_ALIAS_UPDATE) << fromAlias.vchAlias << fromAlias.vchGUID << vchFromString("") << vchWitness << OP_2DROP << OP_2DROP << OP_2DROP;
		scriptPubKeyAlias += scriptPubKeyFromOrig;
		CreateAliasRecipient(scriptPubKeyAlias, aliasRecipient);
	}

	CScript scriptData;
	scriptData << OP_RETURN << data;
	CRecipient fee;
	CreateFeeRecipient(scriptData, data, fee);
	vecSend.push_back(fee);


	return syscointxfund_helper(vchFromString(strAliasOrAddress), vchWitness, aliasRecipient, vecSend);
}

UniValue assetinfo(const JSONRPCRequest& request) {
	const UniValue &params = request.params;
    if (request.fHelp || 2 != params.size())
        throw runtime_error("assetinfo <asset> <getinputs>\n"
                "Show stored values of a single asset and its. Set getinputs to true if you want to get the allocation inputs, if applicable.\n");

    vector<unsigned char> vchAsset = vchFromValue(params[0]);
	bool bGetInputs = params[1].get_bool();
	UniValue oAsset(UniValue::VOBJ);

	CAsset txPos;
	if (!passetdb || !passetdb->ReadAsset(vchAsset, txPos))
		throw runtime_error("SYSCOIN_ASSET_RPC_ERROR: ERRCODE: 2511 - " + _("Failed to read from asset DB"));

	if(!BuildAssetJson(txPos, bGetInputs, oAsset))
		oAsset.clear();
    return oAsset;
}
bool BuildAssetJson(const CAsset& asset, const bool bGetInputs, UniValue& oAsset)
{
    oAsset.push_back(Pair("_id", stringFromVch(asset.vchAsset)));
	oAsset.push_back(Pair("symbol", stringFromVch(asset.vchSymbol)));
    oAsset.push_back(Pair("txid", asset.txHash.GetHex()));
    oAsset.push_back(Pair("height", (int)asset.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= asset.nHeight-1) {
		CBlockIndex *pindex = chainActive[asset.nHeight-1];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oAsset.push_back(Pair("time", nTime));
	oAsset.push_back(Pair("publicvalue", stringFromVch(asset.vchPubData)));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.vchAliasOrAddress)));
	oAsset.push_back(Pair("balance", ValueFromAssetAmount(asset.nBalance, asset.nPrecision, asset.bUseInputRanges)));
	oAsset.push_back(Pair("total_supply", ValueFromAssetAmount(asset.nTotalSupply, asset.nPrecision, asset.bUseInputRanges)));
	oAsset.push_back(Pair("max_supply", ValueFromAssetAmount(asset.nMaxSupply, asset.nPrecision, asset.bUseInputRanges)));
	oAsset.push_back(Pair("interest_rate", asset.fInterestRate));
	oAsset.push_back(Pair("can_adjust_interest_rate", asset.bCanAdjustInterestRate));
	oAsset.push_back(Pair("use_input_ranges", asset.bUseInputRanges));
	oAsset.push_back(Pair("precision", (int)asset.nPrecision));
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
	oAsset.push_back(Pair("symbol", stringFromVch(asset.vchSymbol)));
	oAsset.push_back(Pair("height", (int)asset.nHeight));
	int64_t nTime = 0;
	if (chainActive.Height() >= asset.nHeight-1) {
		CBlockIndex *pindex = chainActive[asset.nHeight-1];
		if (pindex) {
			nTime = pindex->GetMedianTimePast();
		}
	}
	oAsset.push_back(Pair("time", nTime));
	oAsset.push_back(Pair("publicvalue", stringFromVch(asset.vchPubData)));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.vchAliasOrAddress)));
	oAsset.push_back(Pair("balance", ValueFromAssetAmount(asset.nBalance, asset.nPrecision, asset.bUseInputRanges)));
	oAsset.push_back(Pair("total_supply", ValueFromAssetAmount(asset.nTotalSupply, asset.nPrecision, asset.bUseInputRanges)));
	oAsset.push_back(Pair("interest_rate", asset.fInterestRate));
	return true;
}
bool BuildAssetIndexerJson(const CAsset& asset, UniValue& oAsset)
{
	oAsset.push_back(Pair("_id", stringFromVch(asset.vchAsset)));
	oAsset.push_back(Pair("symbol", stringFromVch(asset.vchSymbol)));
	oAsset.push_back(Pair("height", (int)asset.nHeight));
	oAsset.push_back(Pair("category", stringFromVch(asset.sCategory)));
	oAsset.push_back(Pair("alias", stringFromVch(asset.vchAliasOrAddress)));
	oAsset.push_back(Pair("use_input_ranges", asset.bUseInputRanges));
	oAsset.push_back(Pair("balance", ValueFromAssetAmount(asset.nBalance, asset.nPrecision, asset.bUseInputRanges)));
	oAsset.push_back(Pair("total_supply", ValueFromAssetAmount(asset.nTotalSupply, asset.nPrecision, asset.bUseInputRanges)));
	oAsset.push_back(Pair("max_supply", ValueFromAssetAmount(asset.nMaxSupply, asset.nPrecision, asset.bUseInputRanges)));
	oAsset.push_back(Pair("interest_rate", asset.fInterestRate));
	oAsset.push_back(Pair("precision", (int)asset.nPrecision));
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
	entry.push_back(Pair("symbol", stringFromVch(asset.vchSymbol)));

	if(!asset.vchPubData.empty() && asset.vchPubData != dbAsset.vchPubData)
		entry.push_back(Pair("publicvalue", stringFromVch(asset.vchPubData)));

	if (!asset.vchAliasOrAddress.empty() && asset.vchAliasOrAddress != dbAsset.vchAliasOrAddress)
		entry.push_back(Pair("owner", stringFromVch(asset.vchAliasOrAddress)));

	if (!asset.sCategory.empty() && asset.sCategory != dbAsset.sCategory)
		entry.push_back(Pair("category", stringFromVch(asset.sCategory)));

	if (asset.fInterestRate != dbAsset.fInterestRate)
		entry.push_back(Pair("interest_rate", asset.fInterestRate));

	if (asset.nBalance != dbAsset.nBalance)
		entry.push_back(Pair("balance", ValueFromAssetAmount(asset.nBalance, dbAsset.nPrecision, dbAsset.bUseInputRanges)));

	CAssetAllocation assetallocation;
	if (assetallocation.UnserializeFromData(vchData, vchHash)) {
		UniValue oAssetAllocationReceiversArray(UniValue::VARR);
		if (!assetallocation.listSendingAllocationAmounts.empty()) {
			for (auto& amountTuple : assetallocation.listSendingAllocationAmounts) {
				UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
				oAssetAllocationReceiversObj.push_back(Pair("owner", stringFromVch(amountTuple.first)));
				oAssetAllocationReceiversObj.push_back(Pair("amount", ValueFromAssetAmount(amountTuple.second, dbAsset.nPrecision, dbAsset.bUseInputRanges)));
				oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
			}

		}
		else if (!assetallocation.listSendingAllocationInputs.empty()) {
			for (auto& inputTuple : assetallocation.listSendingAllocationInputs) {
				UniValue oAssetAllocationReceiversObj(UniValue::VARR);
				oAssetAllocationReceiversObj.push_back(Pair("owner", stringFromVch(inputTuple.first)));
				for (auto& inputRange : inputTuple.second) {
					UniValue oInput(UniValue::VOBJ);
					oInput.push_back(Pair("start", (int)inputRange.start));
					oInput.push_back(Pair("end", (int)inputRange.end));
					oAssetAllocationReceiversObj.push_back(oInput);
				}
				oAssetAllocationReceiversArray.push_back(Pair("inputs", oAssetAllocationReceiversObj));
			}
		}
		entry.push_back(Pair("allocations", oAssetAllocationReceiversArray));
	}

}

UniValue ValueFromAssetAmount(const CAmount& amount,int precision, bool isInputRange)
{
	if (isInputRange)
		precision = 0;
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
CAmount AssetAmountFromValue(UniValue& value, int precision, bool isInputRange)
{
	if (isInputRange)
		precision = 0;
	if(precision < 0 || precision > 8)
		throw JSONRPCError(RPC_TYPE_ERROR, "Precision must be between 0 and 8");
	if (!value.isNum() && !value.isStr())
		throw JSONRPCError(RPC_TYPE_ERROR, "Amount is not a number or string");
	if (value.isStr() && value.get_str() == "-1") {
		if(!isInputRange)
			value.setInt((int64_t)(MAX_ASSET / ((int)powf(10, precision))));
		else
			value.setInt(MAX_INPUTRANGE_ASSET);
	}
	CAmount amount;
	if (!ParseFixedPoint(value.getValStr(), precision, &amount))
		throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
	if (!AssetRange(amount, isInputRange))
		throw JSONRPCError(RPC_TYPE_ERROR, "Amount out of range");
	return amount;
}
bool AssetRange(const CAmount& amount, int precision, bool isInputRange)
{
	if (isInputRange)
		precision = 0;
	if (precision < 0 || precision > 8)
		throw JSONRPCError(RPC_TYPE_ERROR, "Precision must be between 0 and 8");
	bool sign = amount < 0;
	int64_t n_abs = (sign ? -amount : amount);
	int64_t quotient = n_abs;
	if (precision > 0) {
		int64_t divByAmount = powf(10, precision);
		quotient = n_abs / divByAmount;
	}
	if (!AssetRange(quotient, isInputRange))
		return false;
	return true;
}
bool CAssetDB::ScanAssets(const int count, const int from, const UniValue& oOptions, UniValue& oRes) {
	string strTxid = "";
	vector<unsigned char> vchAliasOrAddress, vchAsset;
	int nStartBlock = 0;
	if (!oOptions.isNull()) {
		const UniValue &txid = find_value(oOptions, "txid");
		if (txid.isStr()) {
			strTxid = txid.get_str();
		}
		const UniValue &assetObj = find_value(oOptions, "asset");
		if (assetObj.isStr()) {
			vchAsset = vchFromValue(assetObj);
		}

		const UniValue &alias = find_value(oOptions, "owner");
		if (alias.isStr()) {
			vchAliasOrAddress = vchFromValue(alias);
		}

		const UniValue &startblock = find_value(oOptions, "startblock");
		if (startblock.isNum()) {
			nStartBlock = startblock.get_int();
		}
	}
	boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
	pcursor->SeekToFirst();
	CAsset txPos;
	pair<string, vector<unsigned char> > key;
	bool bGetInputs = true;
	
	int index = 0;
	while (pcursor->Valid()) {
		boost::this_thread::interruption_point();
		try {
			if (pcursor->GetKey(key) && key.first == "asseti") {
				pcursor->GetValue(txPos);
				if (nStartBlock > 0 && txPos.nHeight < nStartBlock)
				{
					pcursor->Next();
					continue;
				}
				if (!strTxid.empty() && strTxid != txPos.txHash.GetHex())
				{
					pcursor->Next();
					continue;
				}
				if (!vchAsset.empty() && vchAsset != txPos.vchAsset)
				{
					pcursor->Next();
					continue;
				}
				if (!vchAliasOrAddress.empty() && vchAliasOrAddress != txPos.vchAliasOrAddress)
				{
					pcursor->Next();
					continue;
				}
				UniValue oAsset(UniValue::VOBJ);
				if (!BuildAssetJson(txPos, bGetInputs, oAsset))
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
			"[count]          (numeric, optional, unbounded=0, default=10) The number of results to return, 0 to return all.\n"
			"[from]           (numeric, optional, default=0) The number of results to skip.\n"
			"[options]        (object, optional) A json object with options to filter results\n"
			"    {\n"
			"      \"txid\":txid					(string) Transaction ID to filter results for\n"
			"	   \"asset\":guid					(string) Asset GUID to filter.\n"
			"      \"owner\":string					(string) Alias or address to filter.\n"
			"      \"startblock\":block 			(number) Earliest block to filter from. Block number is the block at which the transaction would have confirmed.\n"
			"    }\n"
			+ HelpExampleCli("listassets", "0")
			+ HelpExampleCli("listassets", "10 10")
			+ HelpExampleCli("listassets", "0 0 '{\"owner\":\"SfaT8dGhk1zaQkk8bujMfgWw3szxReej4S\"}'")
			+ HelpExampleCli("listassets", "0 0 '{\"asset\":\"32bff1fa844c124\",\"owner\":\"SfaT8dGhk1zaQkk8bujMfgWw3szxReej4S\",\"startblock\":0}'")
		);
	UniValue options;
	int count = 10;
	int from = 0;
	if (params.size() > 0) {
		count = params[0].get_int();
		if (count == 0) {
			count = INT_MAX;
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
