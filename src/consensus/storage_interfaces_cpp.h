// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_STORAGE_INTERFACES_CPP_H
#define BITCOIN_CONSENSUS_STORAGE_INTERFACES_CPP_H

#include "uint256.h"

// CPP storage interfaces

class CBlockIndexView
{
public:
    CBlockIndexView() {};
    virtual ~CBlockIndexView() {};

    virtual const uint256 GetBlockHash() const = 0;
    //! Efficiently find an ancestor of this block.
    virtual const CBlockIndexView* GetAncestorView(int64_t height) const = 0;
    virtual int64_t GetHeight() const = 0;
    virtual int32_t GetVersion() const = 0;
    virtual int32_t GetTime() const = 0;
    virtual int32_t GetBits() const = 0;
    // Potential optimizations
    virtual const CBlockIndexView* GetPrev() const
    {
        return GetAncestorView(GetHeight() - 1);
    };
    virtual int64_t GetMedianTimePast() const = 0;
};

#endif // BITCOIN_CONSENSUS_STORAGE_INTERFACES_CPP_H
