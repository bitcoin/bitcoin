// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STALETIPMESSAGE_H
#define BITCOIN_STALETIPMESSAGE_H

#include <primitives/block.h>
#include <serialize.h>
#include <uint256.h>
#include <util/check.h>

#include <cstddef>
#include <cstdint>
#include <ios>
#include <string>
#include <vector>

/** Maximum number of headers in a `staletip` message, and so the maximum
 *  number of blocks a tracked stale branch may have after its fork point.
 *  From BIP 332. */
inline constexpr size_t MAX_STALETIP_HEADERS{20};

/** A block header without its previous block hash, as serialized in `staletip`
 *  messages. The omitted previous block hash is reconstructed from the
 *  preceding header in the message (or from the fork point for the first
 *  header), saving 32 bytes per header. */
struct StaleTipCompressedHeader {
    int32_t version{0};
    uint256 merkle_root{};
    uint32_t time{0};
    uint32_t bits{0};
    uint32_t nonce{0};

    bool operator==(const StaleTipCompressedHeader&) const = default;

    SERIALIZE_METHODS(StaleTipCompressedHeader, obj)
    {
        READWRITE(obj.version, obj.merkle_root, obj.time, obj.bits, obj.nonce);
    }
};

/** Thrown when deserializing a `staletip` payload that declares more than
 *  MAX_STALETIP_HEADERS headers. It is a separate type so that the caller can
 *  ignore such a message without disconnecting the peer, as longer branches
 *  are valid but relayed via normal headers sync. Deserialization stops at the
 *  header count, so the rest of the payload is left unread. */
class StaleTipHeadersLimitExceeded : public std::ios_base::failure
{
public:
    explicit StaleTipHeadersLimitExceeded(const std::string& message) : std::ios_base::failure{message} {}
};

/** Contents of a `staletip` P2P message, announcing a stale branch. */
struct StaleTipMessage
{
    //! Block hash of the last common ancestor of the stale tip and the
    //! announcer's active chain.
    uint256 fork_point{};
    //! Compressed headers from the first block after the fork point up to the
    //! stale tip, in height order.
    std::vector<StaleTipCompressedHeader> headers;
    //! Whether the announcer can provide the stale tip's block data on request.
    bool have_block{false};

    /** Decompress the headers by restoring their previous-block hashes,
     *  beginning with `fork_point`.
     *
     *  The headers are continuous by construction, but no proof-of-work,
     *  consensus, or contextual validation has been performed. */
    std::vector<CBlockHeader> DecompressHeaders() const;

    bool operator==(const StaleTipMessage&) const = default;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        Assert(!headers.empty());
        Assert(headers.size() <= MAX_STALETIP_HEADERS);

        s << fork_point;
        WriteCompactSize(s, headers.size());
        for (const auto& header : headers) {
            s << header;
        }
        s << uint8_t{have_block};
    }

    /**
     * @throws StaleTipHeadersLimitExceeded if more than MAX_STALETIP_HEADERS
     *         headers are declared, std::ios_base::failure for any other
     *         malformed payload.
     */
    template <typename Stream>
    void Unserialize(Stream& s)
    {
        s >> fork_point;

        const uint64_t size{ReadCompactSize(s)};
        if (size == 0) {
            throw std::ios_base::failure("staletip headers empty");
        }
        if (size > MAX_STALETIP_HEADERS) {
            throw StaleTipHeadersLimitExceeded{"staletip headers limit exceeded"};
        }

        headers.clear();
        headers.reserve(size);
        for (uint64_t i{0}; i < size; ++i) {
            headers.emplace_back();
            s >> headers.back();
        }

        uint8_t have_block_byte{0};
        s >> have_block_byte;
        if (have_block_byte > 1) {
            throw std::ios_base::failure("staletip invalid have_block");
        }
        have_block = have_block_byte;
    }
};

#endif // BITCOIN_STALETIPMESSAGE_H
