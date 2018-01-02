// Copyright (c) 2015-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CERT_H
#define CERT_H

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
bool CheckCertInputs(const CTransaction &tx, int op, int nOut, const std::vector<std::vector<unsigned char> > &vvchArgs, const std::vector<std::vector<unsigned char> > &vvchAliasArgs, bool fJustCheck, int nHeight, std::string &errorMessage, bool dontaddtodb=false);
bool DecodeCertTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch);
bool DecodeAndParseCertTx(const CTransaction& tx, int& op, int& nOut, std::vector<std::vector<unsigned char> >& vvch, char& type);
bool DecodeCertScript(const CScript& script, int& op, std::vector<std::vector<unsigned char> > &vvch);
bool IsCertOp(int op);
void CertTxToJSON(const int op, const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash, UniValue &entry);
std::string certFromOp(int op);
bool RemoveCertScriptPrefix(const CScript& scriptIn, CScript& scriptOut);
class CCert {
public:
	std::vector<unsigned char> vchCert;
	std::vector<unsigned char> vchAlias;
	// to modify alias in certtransfer
	std::vector<unsigned char> vchLinkAlias;
    uint256 txHash;
    uint64_t nHeight;
	// 1 can edit, 2 can edit/transfer
	unsigned char nAccessFlags;
	std::vector<unsigned char> vchTitle;
	std::vector<unsigned char> vchPubData;
	std::vector<unsigned char> sCategory;
    CCert() {
        SetNull();
    }
    CCert(const CTransaction &tx) {
        SetNull();
        UnserializeFromTx(tx);
    }
	inline void ClearCert()
	{
		vchPubData.clear();
		vchTitle.clear();
		sCategory.clear();
	}
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {		
		READWRITE(vchTitle);
		READWRITE(vchPubData);
		READWRITE(txHash);
		READWRITE(VARINT(nHeight));
		READWRITE(vchLinkAlias);
		READWRITE(nAccessFlags);
		READWRITE(vchCert);
		READWRITE(sCategory);
		READWRITE(vchAlias);
	}
    inline friend bool operator==(const CCert &a, const CCert &b) {
        return (a.vchCert == b.vchCert
        );
    }

    inline CCert operator=(const CCert &b) {
		vchTitle = b.vchTitle;
		vchPubData = b.vchPubData;
        txHash = b.txHash;
        nHeight = b.nHeight;
		vchAlias = b.vchAlias;
		vchLinkAlias = b.vchLinkAlias;
		nAccessFlags = b.nAccessFlags;
		vchCert = b.vchCert;
		sCategory = b.sCategory;
        return *this;
    }

    inline friend bool operator!=(const CCert &a, const CCert &b) {
        return !(a == b);
    }
	inline void SetNull() { sCategory.clear(); vchTitle.clear(); nAccessFlags = 2; vchLinkAlias.clear(); vchCert.clear(); nHeight = 0; txHash.SetNull(); vchAlias.clear(); vchPubData.clear(); }
    inline bool IsNull() const { return (vchCert.empty()); }
    bool UnserializeFromTx(const CTransaction &tx);
	bool UnserializeFromData(const std::vector<unsigned char> &vchData, const std::vector<unsigned char> &vchHash);
	void Serialize(std::vector<unsigned char>& vchData);
};


class CCertDB : public CDBWrapper {
public:
    CCertDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "certificates", nCacheSize, fMemory, fWipe) {}

    bool WriteCert(const CCert& cert, const CCert& prevCert, const int &op, const bool &fJustCheck) {
		bool writeState =  Write(make_pair(std::string("certi"), cert.vchCert), cert);
		if (!fJustCheck && !prevCert.IsNull())
			writeState = writeState && Write(make_pair(std::string("certp"), cert.vchCert), prevCert);
		else if (fJustCheck)
			writeState = writeState && Write(make_pair(std::string("certl"), cert.vchCert), fJustCheck);
		WriteCertIndex(cert, op);
        return writeState;
    }

    bool EraseCert(const std::vector<unsigned char>& vchCert, bool cleanup = false) {
		bool eraseState = Erase(make_pair(std::string("certi"), vchCert));
		Erase(make_pair(std::string("certp"), vchCert));
		Erase(make_pair(std::string("certf"), vchCert));
		EraseISLock(vchCert);
		EraseCertIndex(vchCert, cleanup);
        return eraseState;
    }

    bool ReadCert(const std::vector<unsigned char>& vchCert, CCert& cert) {
        return Read(make_pair(std::string("certi"), vchCert), cert);
    }
	bool ReadFirstCert(const std::vector<unsigned char>& vchCert, CCert& cert) {
		return Read(make_pair(std::string("certf"), vchCert), cert);
	}
	bool ReadLastCert(const std::vector<unsigned char>& vchGuid, CCert& cert) {
		return Read(make_pair(std::string("certp"), vchGuid), cert);
	}
	bool ReadISLock(const std::vector<unsigned char>& vchGuid, bool& lock) {
		return Read(make_pair(std::string("certl"), vchGuid), lock);
	}
	bool EraseISLock(const std::vector<unsigned char>& vchGuid) {
		return Erase(make_pair(std::string("certl"), vchGuid));
	}
	bool WriteFirstCert(const std::vector<unsigned char>& vchCert, const CCert& cert) {
		return Write(make_pair(std::string("certf"), vchCert), cert);
	}
	bool CleanupDatabase(int &servicesCleaned);
	void WriteCertIndex(const CCert& cert, const int &op);
	void EraseCertIndex(const std::vector<unsigned char>& vchCert, bool cleanup);
	void WriteCertIndexHistory(const CCert& cert, const int &op);
	void EraseCertIndexHistory(const std::vector<unsigned char>& vchCert, bool cleanup);
	void EraseCertIndexHistory(const std::string& id);

};
bool GetCert(const std::vector<unsigned char> &vchCert,CCert& txPos);
bool GetFirstCert(const std::vector<unsigned char> &vchCert, CCert& txPos);
bool BuildCertJson(const CCert& cert, UniValue& oName);
bool BuildCertIndexerJson(const CCert& cert,UniValue& oName);
bool BuildCertIndexerHistoryJson(const CCert& cert, UniValue& oName);
uint64_t GetCertExpiration(const CCert& cert);
#endif // CERT_H
