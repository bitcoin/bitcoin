// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAIN_PARAMS_H
#define BITCOIN_CHAIN_PARAMS_H

#include "uint256.h"
#include "core.h"
#include "bitcoin_core.h"

#include <vector>

#include "subsidylevels.h"

using namespace std;

#define MESSAGE_START_SIZE 4
typedef unsigned char MessageStartChars[MESSAGE_START_SIZE];

class CAddress;
class Bitcredit_CBlock;

struct CDNSSeedData {
    string name, host;
    CDNSSeedData(const string &strName, const string &strHost) : name(strName), host(strHost) {}
};

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * Bitcredit system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CChainParams
{
public:
    enum Network {
        MAIN,
        TESTNET,
        REGTEST,

        MAX_NETWORK_TYPES
    };

    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,

        MAX_BASE58_TYPES
    };

    const uint256& HashGenesisBlock() const { return hashGenesisBlock; }
    const MessageStartChars& MessageStart() const { return pchMessageStart; }
    const std::string AddrFileName() const { return addrFileName; }
    const vector<unsigned char>& AlertKey() const { return vAlertPubKey; }
    int GetDefaultPort() const { return nDefaultPort; }
    const uint256& ProofOfWorkLimit() const { return bnProofOfWorkLimit; }
    virtual bool RequireRPCPassword() const { return true; }
    const string& DataDir() const { return strDataDir; }
    virtual Network NetworkID() const = 0;
    const vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char> &Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    virtual const vector<CAddress>& FixedSeeds() const = 0;
    int RPCPort() const { return nRPCPort; }
    int ClientVersion() const { return nClientVersion; }
    int AcceptDepthLinkedBitcoinBlock() const { return nAcceptDepthLinkedBitcoinBlock; }
    int DepositLockDepth() const { return nDepositLockDepth; }
protected:
    CChainParams() {}

    uint256 hashGenesisBlock;
    MessageStartChars pchMessageStart;
    std::string addrFileName;
    // Raw pub key bytes for the broadcast alert signing key.
    vector<unsigned char> vAlertPubKey;
    int nDefaultPort;
    int nRPCPort;
    int nClientVersion;
    //How deep in the bitcoin blockchain the hashLinkedBitcoinBlock reference must be
    int nAcceptDepthLinkedBitcoinBlock;
    //At what depth is a deposit released for spending
    int nDepositLockDepth;
    uint256 bnProofOfWorkLimit;
    string strDataDir;
    vector<CDNSSeedData> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
};

class Bitcredit_CMainParams : public CChainParams {
public:
    Bitcredit_CMainParams();

    vector<SubsidyLevel> getSubsidyLevels() const { return vSubsidyLevels; }
    virtual const Bitcredit_CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }
    virtual int FastForwardClaimBitcoinBlockHeight() const { return 340874; }
    virtual uint256 FastForwardClaimBitcoinBlockHash() const { return uint256("0x000000000000000005a50dde9c4522c05b6b9051dd04b450b9c6721483c213b9"); }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    vector<SubsidyLevel> vSubsidyLevels;
    Bitcredit_CBlock genesis;
    vector<CAddress> vFixedSeeds;
};

class Bitcoin_CMainParams : public CChainParams {
public:
	Bitcoin_CMainParams();

	int SubsidyHalvingInterval() const { return nSubsidyHalvingInterval; }
    virtual const Bitcoin_CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    int nSubsidyHalvingInterval;
    Bitcoin_CBlock genesis;
    vector<CAddress> vFixedSeeds;
};

/**
 * Return the currently selected parameters. This won't change after app startup
 * outside of the unit tests.
 */
const Bitcredit_CMainParams &Bitcredit_Params();
const Bitcoin_CMainParams &Bitcoin_Params();

/** Sets the params returned by xxx_Params() to those for the given network. */
void SelectParams(CChainParams::Network network);

/**
 * Looks for -regtest or -testnet and then calls SelectParams as appropriate.
 * Returns false if an invalid combination is given.
 */
bool SelectParamsFromCommandLine();

inline bool Bitcoin_TestNet() {
    // Note: it's deliberate that this returns "false" for regression test mode.
    return Bitcoin_Params().NetworkID() == CChainParams::TESTNET;
}
inline bool Bitcredit_TestNet() {
    // Note: it's deliberate that this returns "false" for regression test mode.
    return Bitcredit_Params().NetworkID() == CChainParams::TESTNET;
}

inline bool Bitcoin_RegTest() {
    return Bitcoin_Params().NetworkID() == CChainParams::REGTEST;
}
inline bool Bitcredit_RegTest() {
    return Bitcredit_Params().NetworkID() == CChainParams::REGTEST;
}
bool FastForwardClaimStateFor(const int nBitcoinBlockHeight, const uint256 nBitcoinBlockHash);

#endif
