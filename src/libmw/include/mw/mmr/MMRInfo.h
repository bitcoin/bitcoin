#pragma once

#include <mw/common/Traits.h>
#include <mw/models/crypto/Hash.h>
#include <boost/optional.hpp>

/// <summary>
/// Represents the state of the MMR files with the matching index.
/// </summary>
struct MMRInfo : public Traits::ISerializable
{
    MMRInfo()
        : version(0), index(0), pruned(mw::Hash()), compact_index(0), compacted(boost::none) { }
    MMRInfo(uint8_t version, uint32_t index_in, mw::Hash pruned_in, uint32_t compact_index_in, boost::optional<mw::Hash> compacted_in)
        : version(version), index(index_in), pruned(std::move(pruned_in)), compact_index(compact_index_in), compacted(std::move(compacted_in)) { }

    // Version byte that allows for future modifications to the MMRInfo schema.
    uint8_t version;

    // File number of the PMMR files.
    uint32_t index;

    // Hash of latest header this PMMR represents.
    mw::Hash pruned;

    // File number of the PruneList bitset.
    uint32_t compact_index;

    // Hash of the header this PMMR was compacted for.
    // You cannot rewind beyond this point.
    boost::optional<mw::Hash> compacted;
    
    IMPL_SERIALIZABLE(MMRInfo, obj)
    {
        READWRITE(obj.version, obj.index, obj.pruned, obj.compact_index, obj.compacted);
    }
};