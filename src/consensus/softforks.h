// Copyright (c) 2015 Eric Lombrozo
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SOFTFORKS_H
#define BITCOIN_SOFTFORKS_H

#include <cstddef>

class CBlockIndex;
namespace Consensus {

struct Params;
namespace VersionBits { class BlockRuleIndex; }

namespace SoftForks {

enum BipID
{
    BIP16,
    BIP30,
    BIP34,
    BIP65,
    BIP66,
    BIP9999,
};

enum VersionStatus { VALID, UNRECOGNIZED, INVALID };

VersionStatus CheckVersion(const CBlockIndex& blockIndex, const Consensus::VersionBits::BlockRuleIndex& blockRuleIndex, const Consensus::Params& consensusParams, CBlockIndex* pindexPrev = NULL);
bool IsActive(int bip, const CBlockIndex& blockIndex, const Consensus::VersionBits::BlockRuleIndex& blockRuleIndex, const Consensus::Params& consensusParams, CBlockIndex* pindexPrev = NULL);
const char* GetBipName(int bip);

}
}

#endif // BITCOIN_SOFTFORKS_H
