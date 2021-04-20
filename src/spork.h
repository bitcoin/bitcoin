// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SPORK_H
#define BITCOIN_SPORK_H

#include <hash.h>
#include <net.h>
#include <utilstrencodings.h>
#include <key.h>

#include <unordered_map>
#include <unordered_set>

class CSporkMessage;
class CSporkManager;

/*
    Don't ever reuse these IDs for other sporks
    - This would result in old clients getting confused about which spork is for what
*/
enum SporkId : int32_t {
    SPORK_2_INSTANTSEND_ENABLED                            = 10001,
    SPORK_3_INSTANTSEND_BLOCK_FILTERING                    = 10002,
    SPORK_9_SUPERBLOCKS_ENABLED                            = 10008,
    SPORK_17_QUORUM_DKG_ENABLED                            = 10016,
    SPORK_19_CHAINLOCKS_ENABLED                            = 10018,
    SPORK_21_QUORUM_ALL_CONNECTED                          = 10020,
    SPORK_23_QUORUM_POSE                                   = 10022,

    SPORK_INVALID                                          = -1,
};
template<> struct is_serializable_enum<SporkId> : std::true_type {};

namespace std
{
    template<> struct hash<SporkId>
    {
        std::size_t operator()(SporkId const& id) const noexcept
        {
            return std::hash<int>{}(id);
        }
    };
}

struct CSporkDef
{
    SporkId sporkId{SPORK_INVALID};
    int64_t defaultValue{0};
    std::string name;
};

extern std::vector<CSporkDef> sporkDefs;
extern CSporkManager sporkManager;

/**
 * Sporks are network parameters used primarily to prevent forking and turn
 * on/off certain features. They are a soft consensus mechanism.
 *
 * We use 2 main classes to manage the spork system.
 *
 * SporkMessages - low-level constructs which contain the sporkID, value,
 *                 signature and a signature timestamp
 * SporkManager  - a higher-level construct which manages the naming, use of
 *                 sporks, signatures and verification, and which sporks are active according
 *                 to this node
 */

/**
 * CSporkMessage is a low-level class used to encapsulate Spork messages and
 * serialize them for transmission to other peers. This includes the internal
 * spork ID, value, spork signature and timestamp for the signature.
 */
class CSporkMessage
{
private:
    std::vector<unsigned char> vchSig;

public:
    SporkId nSporkID;
    int64_t nValue;
    int64_t nTimeSigned;

    CSporkMessage(SporkId nSporkID, int64_t nValue, int64_t nTimeSigned) :
        nSporkID(nSporkID),
        nValue(nValue),
        nTimeSigned(nTimeSigned)
        {}

    CSporkMessage() :
        nSporkID((SporkId)0),
        nValue(0),
        nTimeSigned(0)
        {}


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nSporkID);
        READWRITE(nValue);
        READWRITE(nTimeSigned);
        READWRITE(vchSig);
    }

    /**
     * GetHash returns the double-sha256 hash of the serialized spork message.
     */
    uint256 GetHash() const;

    /**
     * GetSignatureHash returns the hash of the serialized spork message
     * without the signature included. The intent of this method is to get the
     * hash to be signed.
     */
    uint256 GetSignatureHash() const;

    /**
     * Sign will sign the spork message with the given key.
     */
    bool Sign(const CKey& key);

    /**
     * CheckSignature will ensure the spork signature matches the provided public
     * key hash.
     */
    bool CheckSignature(const CKeyID& pubKeyId) const;

    /**
     * GetSignerKeyID is used to recover the spork address of the key used to
     * sign this spork message.
     *
     * This method was introduced along with the multi-signer sporks feature,
     * in order to identify which spork key signed this message.
     */
    bool GetSignerKeyID(CKeyID& retKeyidSporkSigner) const;

    /**
     * Relay is used to send this spork message to other peers.
     */
    void Relay(CConnman& connman) const;
};

/**
 * CSporkManager is a higher-level class which manages the node's spork
 * messages, rules for which sporks should be considered active/inactive, and
 * processing for certain sporks (e.g. spork 12).
 */
class CSporkManager
{
private:
    static const std::string SERIALIZATION_VERSION_STRING;

    std::unordered_map<SporkId, CSporkDef*> sporkDefsById;
    std::unordered_map<std::string, CSporkDef*> sporkDefsByName;

    mutable std::unordered_map<SporkId, bool> mapSporksCachedActive;
    mutable std::unordered_map<SporkId, int64_t> mapSporksCachedValues;

    mutable CCriticalSection cs;
    std::unordered_map<uint256, CSporkMessage> mapSporksByHash;
    std::unordered_map<SporkId, std::map<CKeyID, CSporkMessage> > mapSporksActive;

    std::set<CKeyID> setSporkPubKeyIDs;
    int nMinSporkKeys;
    CKey sporkPrivKey;

    /**
     * SporkValueIsActive is used to get the value agreed upon by the majority
     * of signed spork messages for a given Spork ID.
     */
    bool SporkValueIsActive(SporkId nSporkID, int64_t& nActiveValueRet) const;

public:

    CSporkManager();

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        std::string strVersion;
        if(ser_action.ForRead()) {
            READWRITE(strVersion);
            if (strVersion != SERIALIZATION_VERSION_STRING) {
                return;
            }
        } else {
            strVersion = SERIALIZATION_VERSION_STRING;
            READWRITE(strVersion);
        }
        // we don't serialize pubkey ids because pubkeys should be
        // hardcoded or be setted with cmdline or options, should
        // not reuse pubkeys from previous dashd run
        LOCK(cs);
        READWRITE(mapSporksByHash);
        READWRITE(mapSporksActive);
        // we don't serialize private key to prevent its leakage
    }

    /**
     * Clear is used to clear all in-memory active spork messages. Since spork
     * public and private keys are set in init.cpp, we do not clear them here.
     *
     * This method was introduced along with the spork cache.
     */
    void Clear();

    /**
     * CheckAndRemove is defined to fulfill an interface as part of the on-disk
     * cache used to cache sporks between runs. If sporks that are restored
     * from cache do not have valid signatures when compared against the
     * current spork private keys, they are removed from in-memory storage.
     *
     * This method was introduced along with the spork cache.
     */
    void CheckAndRemove();

    /**
     * ProcessSpork is used to handle the 'getsporks' and 'spork' p2p messages.
     *
     * For 'getsporks', it sends active sporks to the requesting peer. For 'spork',
     * it validates the spork and adds it to the internal spork storage and
     * performs any necessary processing.
     */
    void ProcessSpork(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);

    /**
     * UpdateSpork is used by the spork RPC command to set a new spork value, sign
     * and broadcast the spork message.
     */
    bool UpdateSpork(SporkId nSporkID, int64_t nValue, CConnman& connman);

    /**
     * IsSporkActive returns a bool for time-based sporks, and should be used
     * to determine whether the spork can be considered active or not.
     *
     * For value-based sporks such as SPORK_5_INSTANTSEND_MAX_VALUE, the spork
     * value should not be considered a timestamp, but an integer value
     * instead, and therefore this method doesn't make sense and should not be
     * used.
     */
    bool IsSporkActive(SporkId nSporkID) const;

    /**
     * GetSporkValue returns the spork value given a Spork ID. If no active spork
     * message has yet been received by the node, it returns the default value.
     */
    int64_t GetSporkValue(SporkId nSporkID) const;

    /**
     * GetSporkIDByName returns the internal Spork ID given the spork name.
     */
    SporkId GetSporkIDByName(const std::string& strName) const;

    /**
     * GetSporkNameByID returns the spork name as a string, given a Spork ID.
     */
    std::string GetSporkNameByID(SporkId nSporkID) const;

    /**
     * GetSporkByHash returns a spork message given a hash of the spork message.
     *
     * This is used when a requesting peer sends a MSG_SPORK inventory message with
     * the hash, to quickly lookup and return the full spork message. We maintain a
     * hash-based index of sporks for this reason, and this function is the access
     * point into that index.
     */
    bool GetSporkByHash(const uint256& hash, CSporkMessage &sporkRet) const;

    /**
     * SetSporkAddress is used to set a public key ID which will be used to
     * verify spork signatures.
     *
     * This can be called multiple times to add multiple keys to the set of
     * valid spork signers.
     */
    bool SetSporkAddress(const std::string& strAddress);

    /**
     * SetMinSporkKeys is used to set the required spork signer threshold, for
     * a spork to be considered active.
     *
     * This value must be at least a majority of the total number of spork
     * keys, and for obvious resons cannot be larger than that number.
     */
    bool SetMinSporkKeys(int minSporkKeys);

    /**
     * SetPrivKey is used to set a spork key to enable setting / signing of
     * spork values.
     *
     * This will return false if the private key does not match any spork
     * address in the set of valid spork signers (see SetSporkAddress).
     */
    bool SetPrivKey(const std::string& strPrivKey);

    /**
     * ToString returns the string representation of the SporkManager.
     */
    std::string ToString() const;
};

#endif // BITCOIN_SPORK_H
