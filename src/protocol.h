// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PROTOCOL_H
#define BITCOIN_PROTOCOL_H

#include <netaddress.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <util/time.h>

#include <cstdint>
#include <limits>
#include <string>
#include <variant>

/** Message header.
 * (4) message start.
 * (12) command.
 * (4) size.
 * (4) checksum.
 */
class CMessageHeader
{
public:
    static constexpr size_t MESSAGE_START_SIZE = 4;
    static constexpr size_t COMMAND_SIZE = 12;
    static constexpr size_t MESSAGE_SIZE_SIZE = 4;
    static constexpr size_t CHECKSUM_SIZE = 4;
    static constexpr size_t MESSAGE_SIZE_OFFSET = MESSAGE_START_SIZE + COMMAND_SIZE;
    static constexpr size_t CHECKSUM_OFFSET = MESSAGE_SIZE_OFFSET + MESSAGE_SIZE_SIZE;
    static constexpr size_t HEADER_SIZE = MESSAGE_START_SIZE + COMMAND_SIZE + MESSAGE_SIZE_SIZE + CHECKSUM_SIZE;
    typedef unsigned char MessageStartChars[MESSAGE_START_SIZE];

    explicit CMessageHeader() = default;

    /** Construct a P2P message header from message-start characters, a command and the size of the message.
     * @note Passing in a `pszCommand` longer than COMMAND_SIZE will result in a run-time assertion error.
     */
    CMessageHeader(const MessageStartChars& pchMessageStartIn, const char* pszCommand, unsigned int nMessageSizeIn);

    std::string GetCommand() const;
    bool IsCommandValid() const;

    SERIALIZE_METHODS(CMessageHeader, obj) { READWRITE(obj.pchMessageStart, obj.pchCommand, obj.nMessageSize, obj.pchChecksum); }

    char pchMessageStart[MESSAGE_START_SIZE]{};
    char pchCommand[COMMAND_SIZE]{};
    uint32_t nMessageSize{std::numeric_limits<uint32_t>::max()};
    uint8_t pchChecksum[CHECKSUM_SIZE]{};
};

/**
 * Bitcoin protocol message types. When adding new message types, don't forget
 * to update allNetMessageTypes in protocol.cpp.
 */
namespace NetMsgType {

/**
 * The version message provides information about the transmitting node to the
 * receiving node at the beginning of a connection.
 */
extern const char* VERSION;
/**
 * The verack message acknowledges a previously-received version message,
 * informing the connecting node that it can begin to send other messages.
 */
extern const char* VERACK;
/**
 * The addr (IP address) message relays connection information for peers on the
 * network.
 */
extern const char* ADDR;
/**
 * The addrv2 message relays connection information for peers on the network just
 * like the addr message, but is extended to allow gossiping of longer node
 * addresses (see BIP155).
 */
extern const char *ADDRV2;
/**
 * The sendaddrv2 message signals support for receiving ADDRV2 messages (BIP155).
 * It also implies that its sender can encode as ADDRV2 and would send ADDRV2
 * instead of ADDR to a peer that has signaled ADDRV2 support by sending SENDADDRV2.
 */
extern const char *SENDADDRV2;
/**
 * The inv message (inventory message) transmits one or more inventories of
 * objects known to the transmitting peer.
 */
extern const char* INV;
/**
 * The getdata message requests one or more data objects from another node.
 */
extern const char* GETDATA;
/**
 * The merkleblock message is a reply to a getdata message which requested a
 * block using the inventory type MSG_MERKLEBLOCK.
 * @since protocol version 70001 as described by BIP37.
 */
extern const char* MERKLEBLOCK;
/**
 * The getblocks message requests an inv message that provides block header
 * hashes starting from a particular point in the block chain.
 */
extern const char* GETBLOCKS;
/**
 * The getheaders message requests a headers message that provides block
 * headers starting from a particular point in the block chain.
 * @since protocol version 31800.
 */
extern const char* GETHEADERS;
/**
 * The tx message transmits a single transaction.
 */
extern const char* TX;
/**
 * The headers message sends one or more block headers to a node which
 * previously requested certain headers with a getheaders message.
 * @since protocol version 31800.
 */
extern const char* HEADERS;
/**
 * The block message transmits a single serialized block.
 */
extern const char* BLOCK;
/**
 * The getaddr message requests an addr message from the receiving node,
 * preferably one with lots of IP addresses of other receiving nodes.
 */
extern const char* GETADDR;
/**
 * The mempool message requests the TXIDs of transactions that the receiving
 * node has verified as valid but which have not yet appeared in a block.
 * @since protocol version 60002.
 */
extern const char* MEMPOOL;
/**
 * The ping message is sent periodically to help confirm that the receiving
 * peer is still connected.
 */
extern const char* PING;
/**
 * The pong message replies to a ping message, proving to the pinging node that
 * the ponging node is still alive.
 * @since protocol version 60001 as described by BIP31.
 */
extern const char* PONG;
/**
 * The notfound message is a reply to a getdata message which requested an
 * object the receiving node does not have available for relay.
 * @since protocol version 70001.
 */
extern const char* NOTFOUND;
/**
 * The filterload message tells the receiving peer to filter all relayed
 * transactions and requested merkle blocks through the provided filter.
 * @since protocol version 70001 as described by BIP37.
 *   Only available with service bit NODE_BLOOM since protocol version
 *   70011 as described by BIP111.
 */
extern const char* FILTERLOAD;
/**
 * The filteradd message tells the receiving peer to add a single element to a
 * previously-set bloom filter, such as a new public key.
 * @since protocol version 70001 as described by BIP37.
 *   Only available with service bit NODE_BLOOM since protocol version
 *   70011 as described by BIP111.
 */
extern const char* FILTERADD;
/**
 * The filterclear message tells the receiving peer to remove a previously-set
 * bloom filter.
 * @since protocol version 70001 as described by BIP37.
 *   Only available with service bit NODE_BLOOM since protocol version
 *   70011 as described by BIP111.
 */
extern const char* FILTERCLEAR;
/**
 * Indicates that a node prefers to receive new block announcements via a
 * "headers" message rather than an "inv".
 * @since protocol version 70012 as described by BIP130.
 */
extern const char* SENDHEADERS;

/**
 * Contains a 1-byte bool and 8-byte LE version number.
 * Indicates that a node is willing to provide blocks via "cmpctblock" messages.
 * May indicate that a node prefers to receive new block announcements via a
 * "cmpctblock" message rather than an "inv", depending on message contents.
 * @since protocol version 70209 as described by BIP 152
 */
extern const char* SENDCMPCT;
/**
 * Contains a CBlockHeaderAndShortTxIDs object - providing a header and
 * list of "short txids".
 * @since protocol version 70209 as described by BIP 152
 */
extern const char* CMPCTBLOCK;
/**
 * Contains a BlockTransactionsRequest
 * Peer should respond with "blocktxn" message.
 * @since protocol version 70209 as described by BIP 152
 */
extern const char* GETBLOCKTXN;
/**
 * Contains a BlockTransactions.
 * Sent in response to a "getblocktxn" message.
 * @since protocol version 70209 as described by BIP 152
 */
extern const char* BLOCKTXN;
/**
 * getcfilters requests compact filters for a range of blocks.
 * Only available with service bit NODE_COMPACT_FILTERS as described by
 * BIP 157 & 158.
 */
extern const char* GETCFILTERS;
/**
 * cfilter is a response to a getcfilters request containing a single compact
 * filter.
 */
extern const char* CFILTER;
/**
 * getcfheaders requests a compact filter header and the filter hashes for a
 * range of blocks, which can then be used to reconstruct the filter headers
 * for those blocks.
 * Only available with service bit NODE_COMPACT_FILTERS as described by
 * BIP 157 & 158.
 */
extern const char* GETCFHEADERS;
/**
 * cfheaders is a response to a getcfheaders request containing a filter header
 * and a vector of filter hashes for each subsequent block in the requested range.
 */
extern const char* CFHEADERS;
/**
 * getcfcheckpt requests evenly spaced compact filter headers, enabling
 * parallelized download and validation of the headers between them.
 * Only available with service bit NODE_COMPACT_FILTERS as described by
 * BIP 157 & 158.
 */
extern const char* GETCFCHECKPT;
/**
 * cfcheckpt is a response to a getcfcheckpt request containing a vector of
 * evenly spaced filter headers for blocks on the requested chain.
 */
extern const char* CFCHECKPT;
/**
 * Contains a 4-byte version number and an 8-byte salt.
 * The salt is used to compute short txids needed for efficient
 * txreconciliation, as described by BIP 330.
 */
extern const char* SENDTXRCNCL;

// Dash message types
// NOTE: do NOT declare non-implmented here, we don't want them to be exposed to the outside
// TODO: add description
extern const char* SPORK;
extern const char* GETSPORKS;
extern const char* DSACCEPT;
extern const char* DSVIN;
extern const char* DSFINALTX;
extern const char* DSSIGNFINALTX;
extern const char* DSCOMPLETE;
extern const char* DSSTATUSUPDATE;
extern const char* DSTX;
extern const char* DSQUEUE;
extern const char* SENDDSQUEUE;
extern const char* SYNCSTATUSCOUNT;
extern const char* MNGOVERNANCESYNC;
extern const char* MNGOVERNANCEOBJECT;
extern const char* MNGOVERNANCEOBJECTVOTE;
extern const char* GETMNLISTDIFF;
extern const char* MNLISTDIFF;
extern const char* QSENDRECSIGS;
extern const char* QFCOMMITMENT;
extern const char* QCONTRIB;
extern const char* QCOMPLAINT;
extern const char* QJUSTIFICATION;
extern const char* QPCOMMITMENT;
extern const char* QWATCH;
extern const char* QSIGSESANN;
extern const char* QSIGSHARESINV;
extern const char* QGETSIGSHARES;
extern const char* QBSIGSHARES;
extern const char* QSIGREC;
extern const char* QSIGSHARE;
extern const char* QGETDATA;
extern const char* QDATA;
extern const char* CLSIG;
extern const char* ISDLOCK;
extern const char* MNAUTH;
extern const char* GETHEADERS2;
extern const char* SENDHEADERS2;
extern const char* HEADERS2;
extern const char* GETQUORUMROTATIONINFO;
extern const char* QUORUMROTATIONINFO;
};

/* Get a vector of all valid message types (see above) */
const std::vector<std::string>& getAllNetMessageTypes();

/* Whether the message type violates blocks-relay-only policy */
bool NetMessageViolatesBlocksOnly(const std::string& msg_type);

/** nServices flags */
enum ServiceFlags : uint64_t {
    // NOTE: When adding here, be sure to update serviceFlagToStr too
    // Nothing
    NODE_NONE = 0,
    // NODE_NETWORK means that the node is capable of serving the complete block chain. It is currently
    // set by all Dash Core non pruned nodes, and is unset by SPV clients or other light clients.
    NODE_NETWORK = (1 << 0),
    // NODE_BLOOM means the node is capable and willing to handle bloom-filtered connections.
    // Dash Core nodes used to support this by default, without advertising this bit,
    // but no longer do as of protocol version 70201 (= NO_BLOOM_VERSION)
    NODE_BLOOM = (1 << 2),
    // NODE_COMPACT_FILTERS means the node will service basic block filter requests.
    // See BIP157 and BIP158 for details on how this is implemented.
    NODE_COMPACT_FILTERS = (1 << 6),
    // NODE_NETWORK_LIMITED means the same as NODE_NETWORK with the limitation of only
    // serving the last 288 blocks
    // See BIP159 for details on how this is implemented.
    NODE_NETWORK_LIMITED = (1 << 10),
    // description will be provided
    NODE_HEADERS_COMPRESSED = (1 << 11),

    // NODE_P2P_V2 means the node supports BIP324 transport
    NODE_P2P_V2 = (1 << 12),

    // Bits 24-31 are reserved for temporary experiments. Just pick a bit that
    // isn't getting used, or one not being used much, and notify the
    // bitcoin-development mailing list. Remember that service bits are just
    // unauthenticated advertisements, so your code must be robust against
    // collisions and other cases where nodes may be advertising a service they
    // do not actually support. Other service bits should be allocated via the
    // BIP process.
};

/**
 * Convert service flags (a bitmask of NODE_*) to human readable strings.
 * It supports unknown service flags which will be returned as "UNKNOWN[...]".
 * @param[in] flags multiple NODE_* bitwise-OR-ed together
 */
std::vector<std::string> serviceFlagsToStr(uint64_t flags);

/**
 * Gets the set of service flags which are "desirable" for a given peer.
 *
 * These are the flags which are required for a peer to support for them
 * to be "interesting" to us, ie for us to wish to use one of our few
 * outbound connection slots for or for us to wish to prioritize keeping
 * their connection around.
 *
 * Relevant service flags may be peer- and state-specific in that the
 * version of the peer may determine which flags are required (eg in the
 * case of NODE_NETWORK_LIMITED where we seek out NODE_NETWORK peers
 * unless they set NODE_NETWORK_LIMITED and we are out of IBD, in which
 * case NODE_NETWORK_LIMITED suffices).
 *
 * Thus, generally, avoid calling with peerServices == NODE_NONE, unless
 * state-specific flags must absolutely be avoided. When called with
 * peerServices == NODE_NONE, the returned desirable service flags are
 * guaranteed to not change dependent on state - ie they are suitable for
 * use when describing peers which we know to be desirable, but for which
 * we do not have a confirmed set of service flags.
 *
 * If the NODE_NONE return value is changed, contrib/seeds/makeseeds.py
 * should be updated appropriately to filter for the same nodes.
 */
ServiceFlags GetDesirableServiceFlags(ServiceFlags services);

/** Set the current IBD status in order to figure out the desirable service flags */
void SetServiceFlagsIBDCache(bool status);

/**
 * A shortcut for (services & GetDesirableServiceFlags(services))
 * == GetDesirableServiceFlags(services), ie determines whether the given
 * set of service flags are sufficient for a peer to be "relevant".
 */
static inline bool HasAllDesirableServiceFlags(ServiceFlags services)
{
    return !(GetDesirableServiceFlags(services) & (~services));
}

/**
 * Checks if a peer with the given service flags may be capable of having a
 * robust address-storage DB.
 */
static inline bool MayHaveUsefulAddressDB(ServiceFlags services)
{
    return (services & NODE_NETWORK) || (services & NODE_NETWORK_LIMITED);
}

/** A CService with information about it as peer */
class CAddress : public CService
{
    static constexpr std::chrono::seconds TIME_INIT{100000000};

    /** Historically, CAddress disk serialization stored the CLIENT_VERSION, optionally OR'ed with
     *  the ADDRV2_FORMAT flag to indicate V2 serialization. The first field has since been
     *  disentangled from client versioning, and now instead:
     *  - The low bits (masked by DISK_VERSION_IGNORE_MASK) store the fixed value DISK_VERSION_INIT,
     *    (in case any code exists that treats it as a client version) but are ignored on
     *    deserialization.
     *  - The high bits (masked by ~DISK_VERSION_IGNORE_MASK) store actual serialization information.
     *    Only 0 or DISK_VERSION_ADDRV2 (equal to the historical ADDRV2_FORMAT) are valid now, and
     *    any other value triggers a deserialization failure. Other values can be added later if
     *    needed.
     *
     *  For disk deserialization, ADDRV2_FORMAT in the stream version signals that ADDRV2
     *  deserialization is permitted, but the actual format is determined by the high bits in the
     *  stored version field. For network serialization, the stream version having ADDRV2_FORMAT or
     *  not determines the actual format used (as it has no embedded version number).
     */
    static constexpr uint32_t DISK_VERSION_INIT{220000};
    static constexpr uint32_t DISK_VERSION_IGNORE_MASK{0b00000000'00000111'11111111'11111111};
    /** The version number written in disk serialized addresses to indicate V2 serializations.
     * It must be exactly 1<<29, as that is the value that historical versions used for this
     * (they used their internal ADDRV2_FORMAT flag here). */
    static constexpr uint32_t DISK_VERSION_ADDRV2{1 << 29};
    static_assert((DISK_VERSION_INIT & ~DISK_VERSION_IGNORE_MASK) == 0, "DISK_VERSION_INIT must be covered by DISK_VERSION_IGNORE_MASK");
    static_assert((DISK_VERSION_ADDRV2 & DISK_VERSION_IGNORE_MASK) == 0, "DISK_VERSION_ADDRV2 must not be covered by DISK_VERSION_IGNORE_MASK");

public:
    CAddress() : CService{} {};
    explicit CAddress(CService ipIn, ServiceFlags nServicesIn) : CService{ipIn}, nServices{nServicesIn} {};
    CAddress(CService ipIn, ServiceFlags nServicesIn, NodeSeconds time) : CService{ipIn}, nTime{time}, nServices{nServicesIn} {};

    SERIALIZE_METHODS(CAddress, obj)
    {
        // CAddress has a distinct network serialization and a disk serialization, but it should never
        // be hashed (except through CHashWriter in addrdb.cpp, which sets SER_DISK), and it's
        // ambiguous what that would mean. Make sure no code relying on that is introduced:
        assert(!(s.GetType() & SER_GETHASH));
        bool use_v2;
        if (s.GetType() & SER_DISK) {
            // In the disk serialization format, the encoding (v1 or v2) is determined by a flag version
            // that's part of the serialization itself. ADDRV2_FORMAT in the stream version only determines
            // whether V2 is chosen/permitted at all.
            uint32_t stored_format_version = DISK_VERSION_INIT;
            if (s.GetVersion() & ADDRV2_FORMAT) stored_format_version |= DISK_VERSION_ADDRV2;
            READWRITE(stored_format_version);
            stored_format_version &= ~DISK_VERSION_IGNORE_MASK; // ignore low bits
            if (stored_format_version == 0) {
                use_v2 = false;
            } else if (stored_format_version == DISK_VERSION_ADDRV2 && (s.GetVersion() & ADDRV2_FORMAT)) {
                // Only support v2 deserialization if ADDRV2_FORMAT is set.
                use_v2 = true;
            } else {
                throw std::ios_base::failure("Unsupported CAddress disk format version");
            }
        } else {
            // In the network serialization format, the encoding (v1 or v2) is determined directly by
            // the value of ADDRV2_FORMAT in the stream version, as no explicitly encoded version
            // exists in the stream.
            assert(s.GetType() & SER_NETWORK);
            use_v2 = s.GetVersion() & ADDRV2_FORMAT;
        }

        READWRITE(Using<LossyChronoFormatter<uint32_t>>(obj.nTime));
        // nServices is serialized as CompactSize in V2; as uint64_t in V1.
        if (use_v2) {
            uint64_t services_tmp;
            SER_WRITE(obj, services_tmp = obj.nServices);
            READWRITE(Using<CompactSizeFormatter<false>>(services_tmp));
            SER_READ(obj, obj.nServices = static_cast<ServiceFlags>(services_tmp));
        } else {
            READWRITE(Using<CustomUintFormatter<8>>(obj.nServices));
        }
        // Invoke V1/V2 serializer for CService parent object.
        OverrideStream<Stream> os(&s, s.GetType(), use_v2 ? ADDRV2_FORMAT : 0);
        SerReadWriteMany(os, ser_action, ReadWriteAsHelper<CService>(obj));
    }

    //! Always included in serialization. The behavior is unspecified if the value is not representable as uint32_t.
    NodeSeconds nTime{TIME_INIT};
    //! Serialized as uint64_t in V1, and as CompactSize in V2.
    ServiceFlags nServices{NODE_NONE};

    friend bool operator==(const CAddress& a, const CAddress& b)
    {
        return a.nTime == b.nTime &&
               a.nServices == b.nServices &&
               static_cast<const CService&>(a) == static_cast<const CService&>(b);
    }
};

/** getdata / inv message types.
 * These numbers are defined by the protocol. When adding a new value, be sure
 * to mention it in the respective BIP.
 */
enum GetDataMsg : uint32_t {
    UNDEFINED = 0,
    MSG_TX = 1,
    MSG_BLOCK = 2,
    // The following can only occur in getdata. Invs always use TX or BLOCK.
    MSG_FILTERED_BLOCK = 3,                           //!< Defined in BIP37
    // Dash message types
    // NOTE: we must keep this enum consistent and backwards compatible
    /* MSG_LEGACY_TXLOCK_REQUEST = 4, */              // Legacy InstantSend and not used anymore
    /* MSG_TXLOCK_VOTE = 5, */                        // Legacy InstantSend and not used anymore
    MSG_SPORK = 6,
    /* 7 - 15 were used in old Dash versions and were mainly budget and MN broadcast/ping related*/
    MSG_DSTX = 16,
    MSG_GOVERNANCE_OBJECT = 17,
    MSG_GOVERNANCE_OBJECT_VOTE = 18,
    /* 19 was used for MSG_MASTERNODE_VERIFY and is not supported anymore */
    // Nodes may always request a MSG_CMPCT_BLOCK in a getdata, however,
    // MSG_CMPCT_BLOCK should not appear in any invs except as a part of getdata.
    MSG_CMPCT_BLOCK = 20,                             //!< Defined in BIP152
    MSG_QUORUM_FINAL_COMMITMENT = 21,
    /* MSG_QUORUM_DUMMY_COMMITMENT = 22, */           // was shortly used on testnet/devnet/regtest
    MSG_QUORUM_CONTRIB = 23,
    MSG_QUORUM_COMPLAINT = 24,
    MSG_QUORUM_JUSTIFICATION = 25,
    MSG_QUORUM_PREMATURE_COMMITMENT = 26,
    /* MSG_QUORUM_DEBUG_STATUS = 27, */               // was shortly used on testnet/devnet/regtest
    MSG_QUORUM_RECOVERED_SIG = 28,
    MSG_CLSIG = 29,
    /* MSG_ISLOCK = 30, */                            // Non-deterministic InstantSend and not used anymore
    MSG_ISDLOCK = 31,
    MSG_DSQ = 32,
};

/** inv message data */
class CInv
{
public:
    CInv();
    CInv(uint32_t typeIn, const uint256& hashIn);

    SERIALIZE_METHODS(CInv, obj) { READWRITE(obj.type, obj.hash); }

    friend bool operator<(const CInv& a, const CInv& b);

    bool IsKnownType() const;
    std::string GetCommand() const;
    std::string ToString() const;

    // Single-message helper methods
    bool IsMsgTx()        const { return type == MSG_TX; }
    bool IsMsgBlk() const { return type == MSG_BLOCK; }
    bool IsMsgDstx()       const { return type == MSG_DSTX; }
    bool IsMsgFilteredBlk() const { return type == MSG_FILTERED_BLOCK; }
    bool IsMsgCmpctBlk() const { return type == MSG_CMPCT_BLOCK; }

    // Combined-message helper methods
    bool IsGenTxMsg() const
    {
        return type == MSG_TX || type == MSG_DSTX;
    }
    bool IsGenBlkMsg() const
    {
        return type == MSG_BLOCK || type == MSG_FILTERED_BLOCK || type == MSG_CMPCT_BLOCK;
    }

private:
    const char* GetCommandInternal() const;

public:
    uint32_t type;
    uint256 hash;
};

struct MisbehavingError
{
    int score;
    std::string message;

    MisbehavingError(int s) : score{s} {}

     // Constructor does a perfect forwarding reference
    template <typename T>
    MisbehavingError(int s, T&& msg) :
        score{s},
        message{std::forward<T>(msg)}
    {}
};

/**
 * This struct is a helper to return values from handlers that are processing
 * network messages but implemented outside of net_processing.cpp,
 * for example llmq's messages.
 *
 * These handlers do not supposed to know anything about PeerManager to avoid
 * circular dependencies.
 *
 * See `PeerManagerImpl::PostProcessMessage` to see how each type of return code
 * is processed.
 */
struct MessageProcessingResult
{
    //! @m_error triggers Misbehaving error with score and optional message if not nullopt
    std::optional<MisbehavingError> m_error;

    //! @m_inventory will relay these inventories to connected peers
    std::vector<CInv> m_inventory;

    //! @m_inv_filter will relay this inventory if filter matches to connected peers if not nullopt
    std::optional<std::pair<CInv, std::variant<CTransactionRef, uint256>>> m_inv_filter;

    //! @m_request_tx will ask connected peers to relay transaction if not nullopt
    std::optional<uint256> m_request_tx;

    //! @m_transactions will relay transactions to peers which is ready to accept it (some peers does not accept transactions)
    std::vector<uint256> m_transactions;

    //! @m_to_erase triggers EraseObjectRequest from PeerManager for this inventory if not nullopt
    std::optional<CInv> m_to_erase;

    MessageProcessingResult() = default;
    MessageProcessingResult(MisbehavingError error) :
        m_error(error)
    {}
    MessageProcessingResult(CInv inv) :
        m_inventory({inv})
    {
    }
};

#endif // BITCOIN_PROTOCOL_H
