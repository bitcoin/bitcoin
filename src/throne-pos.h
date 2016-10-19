

// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef THRONE_POS_H
#define THRONE_POS_H

#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "util.h"
#include "script.h"
#include "base58.h"
#include "main.h"

using namespace std;
using namespace boost;

class CThroneScanning;
class CThroneScanningError;

extern map<uint256, CThroneScanningError> mapThroneScanningErrors;
extern CThroneScanning mnscan;

static const int MIN_THRONE_POS_PROTO_VERSION = 70076;

/*
	1% of the network is scanned every 2.5 minutes, making a full
	round of scanning take about 4.16 hours. We're targeting about
	a day of proof-of-service errors for complete removal from the
	throne system.
*/
static const int THRONE_SCANNING_ERROR_THESHOLD = 6;

#define SCANNING_SUCCESS                       1
#define SCANNING_ERROR_NO_RESPONSE             2
#define SCANNING_ERROR_IX_NO_RESPONSE          3
#define SCANNING_ERROR_MAX                     3

void ProcessMessageThronePOS(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);

class CThroneScanning
{
public:
    void DoThronePOSChecks();
    void CleanThroneScanningErrors();
};

// Returns how many thrones are allowed to scan each block
int GetCountScanningPerBlock();

class CThroneScanningError
{
public:
    CTxIn vinThroneA;
    CTxIn vinThroneB;
    int nErrorType;
    int nExpiration;
    int nBlockHeight;
    std::vector<unsigned char> vchThroNeSignature;

    CThroneScanningError ()
    {
        vinThroneA = CTxIn();
        vinThroneB = CTxIn();
        nErrorType = 0;
        nExpiration = GetTime()+(60*60);
        nBlockHeight = 0;
    }

    CThroneScanningError (CTxIn& vinThroneAIn, CTxIn& vinThroneBIn, int nErrorTypeIn, int nBlockHeightIn)
    {
    	vinThroneA = vinThroneAIn;
    	vinThroneB = vinThroneBIn;
    	nErrorType = nErrorTypeIn;
    	nExpiration = GetTime()+(60*60);
    	nBlockHeight = nBlockHeightIn;
    }

    CThroneScanningError (CTxIn& vinThroneBIn, int nErrorTypeIn, int nBlockHeightIn)
    {
        //just used for IX, ThroneA is any client
        vinThroneA = CTxIn();
        vinThroneB = vinThroneBIn;
        nErrorType = nErrorTypeIn;
        nExpiration = GetTime()+(60*60);
        nBlockHeight = nBlockHeightIn;
    }

    uint256 GetHash() const {return SerializeHash(*this);}

    bool SignatureValid();
    bool Sign();
    bool IsExpired() {return GetTime() > nExpiration;}
    void Relay();
    bool IsValid() {
    	return (nErrorType > 0 && nErrorType <= SCANNING_ERROR_MAX);
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(vinThroneA);
        READWRITE(vinThroneB);
        READWRITE(nErrorType);
        READWRITE(nExpiration);
        READWRITE(nBlockHeight);
        READWRITE(vchThroNeSignature);
    )
};


#endif