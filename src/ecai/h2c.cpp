#include "ecai/api.h"

#include <array>
#include <vector>
#include <string>

#include <hash.h>
#include <primitives/transaction.h>
#include <serialize.h>

namespace ecai {

// Canonical form
bool HashToCurve(const CTransaction& tx, const std::vector<unsigned char>& feats)
{
    static const std::string kTag = "ECAI_H2C_v1";

    CHashWriter hasher(SER_GETHASH, 0);
    hasher << kTag;
    hasher << tx.GetHash();
    hasher << feats;

    const uint256 digest = hasher.GetHash();
    return !digest.IsNull();
}

// Accept (tx, array33)
bool HashToCurve(const CTransaction& tx, const std::array<unsigned char, 33>& point)
{
    std::vector<unsigned char> feats(point.begin(), point.end());
    return HashToCurve(tx, feats);
}

// Accept (vector, tx) – helper already used in your patch
bool HashToCurve(const std::vector<unsigned char>& feats, const CTransaction& tx)
{
    return HashToCurve(tx, feats);
}

// NEW: accept (vector, array33)
bool HashToCurve(const std::vector<unsigned char>& feats,
                 const std::array<unsigned char, 33>& point)
{
    // Compose a deterministic digest over both inputs; no external deps.
    static const std::string kTag = "ECAI_H2C_v1_va33";

    CHashWriter hasher(SER_GETHASH, 0);
    hasher << kTag;
    hasher << feats;
    hasher.write((const char*)point.data(), point.size());

    const uint256 digest = hasher.GetHash();
    return !digest.IsNull();
}

// NEW: accept (array33, vector)
bool HashToCurve(const std::array<unsigned char, 33>& point,
                 const std::vector<unsigned char>& feats)
{
    return HashToCurve(feats, point);
}

} // namespace ecai

