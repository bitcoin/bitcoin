// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/chain.h>

#include <sync.h>
#include <util/system.h>
#include <validation.h>

#include <memory>
#include <utility>

namespace interfaces {
namespace {

class LockImpl : public Chain::Lock
{
};

class LockingStateImpl : public LockImpl, public UniqueLock<CCriticalSection>
{
    using UniqueLock::UniqueLock;
};

class ChainImpl : public Chain
{
public:
    std::unique_ptr<Chain::Lock> lock(bool try_lock) override
    {
        auto result = MakeUnique<LockingStateImpl>(::cs_main, "cs_main", __FILE__, __LINE__, try_lock);
        if (try_lock && result && !*result) return {};
        // std::move necessary on some compilers due to conversion from
        // LockingStateImpl to Lock pointer
        return std::move(result);
    }
    std::unique_ptr<Chain::Lock> assumeLocked() override { return MakeUnique<LockImpl>(); }
};

} // namespace

std::unique_ptr<Chain> MakeChain() { return MakeUnique<ChainImpl>(); }

} // namespace interfaces
