// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <list>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <cstddef>
#include <type_traits>

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CBlockHeader()
    {
        SetNull();
    }

    SERIALIZE_METHODS(CBlockHeader, obj) { READWRITE(obj.nVersion, obj.hashPrevBlock, obj.hashMerkleRoot, obj.nTime, obj.nBits, obj.nNonce); }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};

class CompressedHeaderBitField
{
    std::byte bit_field{0};

public:
    enum class Flag : std::underlying_type_t<std::byte> {
        VERSION_BIT_0 = (1 << 0),
        VERSION_BIT_1 = (1 << 1),
        VERSION_BIT_2 = (1 << 2),
        PREV_BLOCK_HASH = (1 << 3),
        TIMESTAMP = (1 << 4),
        NBITS = (1 << 5),
    };

    inline bool IsCompressed(Flag flag) const
    {
        return (bit_field & to_byte(flag)) == to_byte(0);
    }

    inline void MarkAsUncompressed(Flag flag)
    {
        bit_field |= to_byte(flag);
    }

    inline void MarkAsCompressed(Flag flag)
    {
        bit_field &= ~to_byte(flag);
    }

    inline bool IsVersionCompressed() const
    {
        return GetVersionOffset() != 0;
    }

    inline void SetVersionOffset(uint8_t version)
    {
        bit_field &= ~VERSION_BIT_MASK;
        bit_field |= to_byte(version) & VERSION_BIT_MASK;
    }

    inline uint8_t GetVersionOffset() const
    {
        return to_uint8(bit_field & VERSION_BIT_MASK);
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        ::Serialize(s, to_uint8(bit_field));
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        uint8_t new_bit_field_value;
        ::Unserialize(s, new_bit_field_value);
        bit_field = to_byte(new_bit_field_value);
    }

private:
    static constexpr uint8_t to_uint8(const std::byte value)
    {
        return std::to_integer<uint8_t>(value);
    }

    static constexpr std::byte to_byte(const uint8_t value)
    {
        return std::byte{value};
    }

    static constexpr std::byte to_byte(const Flag flag)
    {
        return static_cast<std::byte>(flag);
    }

    static constexpr std::byte VERSION_BIT_MASK = static_cast<std::byte>(Flag::VERSION_BIT_0) | static_cast<std::byte>(Flag::VERSION_BIT_1) | static_cast<std::byte>(Flag::VERSION_BIT_2);
};

struct CompressibleBlockHeader : CBlockHeader {
    CompressedHeaderBitField bit_field;
    int16_t time_offset{0};

    CompressibleBlockHeader() = default;

    explicit CompressibleBlockHeader(CBlockHeader&& block_header)
    {
        static_assert(std::is_trivially_copyable_v<CBlockHeader>, "If CBlockHeader is not trivially copyable, please consider using std::move on the next line");
        *static_cast<CBlockHeader*>(this) = block_header;

        // When we create this from a block header, mark everything as uncompressed
        bit_field.SetVersionOffset(0);
        bit_field.MarkAsUncompressed(CompressedHeaderBitField::Flag::PREV_BLOCK_HASH);
        bit_field.MarkAsUncompressed(CompressedHeaderBitField::Flag::TIMESTAMP);
        bit_field.MarkAsUncompressed(CompressedHeaderBitField::Flag::NBITS);
    }

    SERIALIZE_METHODS(CompressibleBlockHeader, obj)
    {
        READWRITE(obj.bit_field);
        if (!obj.bit_field.IsVersionCompressed()) {
            READWRITE(obj.nVersion);
        }
        if (!obj.bit_field.IsCompressed(CompressedHeaderBitField::Flag::PREV_BLOCK_HASH)) {
            READWRITE(obj.hashPrevBlock);
        }
        READWRITE(obj.hashMerkleRoot);
        if (!obj.bit_field.IsCompressed(CompressedHeaderBitField::Flag::TIMESTAMP)) {
            READWRITE(obj.nTime);
        } else {
            READWRITE(obj.time_offset);
        }
        if (!obj.bit_field.IsCompressed(CompressedHeaderBitField::Flag::NBITS)) {
            READWRITE(obj.nBits);
        }
        READWRITE(obj.nNonce);
    }

    void Compress(const std::vector<CompressibleBlockHeader>& previous_blocks, std::list<int32_t>& last_unique_versions);

    void Uncompress(const std::vector<CBlockHeader>& previous_blocks, std::list<int32_t>& last_unique_versions);
};

class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *(static_cast<CBlockHeader*>(this)) = header;
    }

    SERIALIZE_METHODS(CBlock, obj)
    {
        READWRITEAS(CBlockHeader, obj);
        READWRITE(obj.vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        return block;
    }

    std::string ToString() const;
};


/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn) {}

    SERIALIZE_METHODS(CBlockLocator, obj)
    {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(obj.vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
