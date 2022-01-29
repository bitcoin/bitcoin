#pragma once

// Copyright (c) 2018-2020 David Burkett
// Copyright (c) 2020 The Litecoin Developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Macros.h>
#include <mw/models/block/Block.h>

MW_NAMESPACE

/// <summary>
/// Interface for iterating sequentially blocks in the chain.
/// </summary>
class IChainIterator
{
public:
    virtual ~IChainIterator() = default;

    virtual void Next() noexcept = 0;
    virtual bool Valid() const noexcept = 0;

    virtual uint64_t GetHeight() const = 0;
    virtual mw::Header::CPtr GetHeader() const = 0;
    virtual mw::Block::CPtr GetBlock() const = 0;
};

/// <summary>
/// Interface for accessing blocks in the chain.
/// </summary>
class IChain
{
public:
    using Ptr = std::shared_ptr<mw::IChain>;

    virtual ~IChain() = default;

    virtual std::unique_ptr<IChainIterator> NewIterator() = 0;
};

END_NAMESPACE // mw