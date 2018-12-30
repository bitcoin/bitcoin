// Copyright (c) 2018 The BitcoinV Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



#include <amount.h>
#include <consensus/params.h>
#include <primitives/block.h>

CAmount GetBlockSubsidyVBR(int nHeight, const Consensus::Params& consensusParams, const CBlock &block, bool print);
