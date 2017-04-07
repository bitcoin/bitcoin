#ifndef SERVICE_H
#define SERVICE_H

#include "rpc/server.h"
#include "dbwrapper.h"
#include "script/script.h"
#include "serialize.h"
class CWalletTx;
class CTransaction;
class CReserveKey;
class CCoinsViewCache;
class CCoins;
class CBlock;
class CAliasIndex;
bool CheckServiceInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const CCoinsViewCache &inputs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);
bool DecodeServiceTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseServiceTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeServiceScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsServiceOp(int op);
int IndexOfServiceOutput(const CTransaction& tx);
bool EncryptMessage(const std::vector<unsigned char> &strPublicKey, const std::string &strMessage, std::string &strCipherText);
bool DecryptPrivateKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCipherText, std::string &strMessage);
bool DecryptPrivateKey(const CAliasIndex& alias, std::string &strKey);
bool DecryptMessage(const CAliasIndex& alias, const std::vector<unsigned char> &vchCipherText, std::string &strMessage);
void ServiceTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string serviceFromOp(int op);
bool RemoveServiceScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
class CService {
public:
	std::vector<unsigned char> vchService;
	std::vector<unsigned char> vchAlias;
	std::vector<unsigned char> vchWitnessAlias;
    std::vector<unsigned char> vchPrivateData;
	std::vector<unsigned char> vchPublicData;
    uint256 txid;
    uint64_t nHeight;
	unsigned char nSafetyLevel;
	// 1 can edit, 2 can edit/transfer
	unsigned char nAccessFlags;
    CService() {
        SetNull();
    }
    CService(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	void ClearService()
	{
		vchPrivateData.clear();
		vchPublicData.clear();

	}
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {		
		READWRITE(vchPrivateData);
		READWRITE(vchPublicData);
		READWRITE(txid);
		READWRITE(VARINT(nHeight));
		READWRITE(vchWitnessAlias);
		READWRITE(bTransferViewOnly);
		READWRITE(vchService);
		READWRITE(VARINT(safetyLevel));
		READWRITE(vchAlias);
	}
    friend bool operator==(const CService &a, const CService &b) {
        return (
        a.vchPrivateData == b.vchPrivateData
		&& a.vchPublicData == b.vchPublicData
        && a.txid == b.txid
        && a.nHeight == b.nHeight
		&& a.vchAlias == b.vchAlias
		&& a.vchWitnessAlias == b.vchWitnessAlias
		&& a.bTransferViewOnly == b.bTransferViewOnly
		&& a.safetyLevel == b.safetyLevel
		&& a.vchService == b.vchService
        );
    }

    CService operator=(const CService &b) {
        vchPrivateData = b.vchPrivateData;
		vchPublicData = b.vchPublicData;
        txid = b.txid;
        nHeight = b.nHeight;
		vchAlias = b.vchAlias;
		vchWitnessAlias = b.vchWitnessAlias;
		bTransferViewOnly = b.bTransferViewOnly;
		safetyLevel = b.safetyLevel;
		vchService = b.vchService;
        return *this;
    }

    friend bool operator!=(const CService &a, const CService &b) {
        return !(a == b);
    }
    bool GetServiceFromList(std::vector<CService> &serviceList) {
        if(serviceList.size() == 0) return false;
		CService myService = serviceList.front();
		if(nHeight <= 0)
		{
			*this = myService;
			return true;
		}
			
		// find the closest service without going over in height, assuming serviceList orders entries by nHeight ascending
        for(std::vector<CService>::reverse_iterator it = serviceList.rbegin(); it != serviceList.rend(); ++it) {
            const CService &c = *it;
			// skip if this height is greater than our service height
			if(c.nHeight > nHeight)
				continue;
            myService = c;
			break;
        }
        *this = myService;
        return true;
    }
    void SetNull() { bTransferViewOnly = false; vchWitnessAlias.clear(); vchService.clear(); safetyLevel = 0; nHeight = 0; txid.SetNull(); vchAlias.clear(); vchPrivateData.clear(); vchPublicData.clear();}
    bool IsNull() const { return (bTransferViewOnly == false && vchWitnessAlias.empty() && vchService.empty() && safetyLevel == 0 && txid.IsNull() &&  nHeight == 0 && vchPrivateData.empty() && vchPublicData.empty() && vchAlias.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CServiceDB : public CDBWrapper {
public:
    CServiceDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "services", nCacheSize, fMemory, fWipe) {}

    bool WriteService(const std::vector<unsigned char>& name, const std::vector<CService>& vtxPos) {
        return Write(make_pair(std::string("servicei"), name), vtxPos);
    }
    bool EraseService(const std::vector<unsigned char>& name) {
        return Erase(make_pair(std::string("servicei"), name));
    }

    bool ReadService(const std::vector<unsigned char>& name, std::vector<CService>& vtxPos) {
        return Read(make_pair(std::string("servicei"), name), vtxPos);
    }

    bool ExistsService(const std::vector<unsigned char>& name) {
        return Exists(make_pair(std::string("servicei"), name));
    }

	bool GetDBServices(std::vector<CService>& services, const uint64_t& nExpireFilter, const std::vector<std::string>& aliasArray);
	bool CleanupDatabase(int &servicesCleaned);

};
bool GetTxOfService(const std::vector<unsigned char> &vchService,
        CService& txPos, CTransaction& tx, bool skipExpiresCheck=false);
bool GetTxAndVtxOfService(const std::vector<unsigned char> &vchService,
					   CService& txPos, CTransaction& tx, std::vector<CService> &vtxPos, bool skipExpiresCheck=false);
bool GetVtxOfService(const std::vector<unsigned char> &vchService,
					   CService& txPos, std::vector<CService> &vtxPos, bool skipExpiresCheck=false);
void PutToServiceList(std::vector<CService> &serviceList, CService& index);
bool BuildServiceJson(const CService& service, const CAliasIndex& alias, UniValue& oName, const std::string &strWalletless="");
bool BuildServiceStatsJson(const std::vector<CService> &services, UniValue& oServiceStats);
uint64_t GetServiceExpiration(const CService& service);
#endif // SERVICE_H
