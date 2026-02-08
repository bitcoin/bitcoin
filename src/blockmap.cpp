#include <blockmap.h>

#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

#include <memory>
#include <utility>

std::pair<BlockMap::iterator, bool> BlockMap::try_emplace(const uint256& hash)
{
    auto it = find(hash);
    if (it != end()) {
        return {it, false};
    }

    if (m_impl.read().recent.size() > MAX_RECENT_SIZE) {
        promote_all_recent();
    }

    auto block_ptr = std::make_shared<CBlockIndex>(hash);
    return try_emplace_impl(hash, std::move(block_ptr));
}

std::pair<BlockMap::iterator, bool> BlockMap::try_emplace(const uint256& hash, const CBlockHeader& header)
{
    auto it = find(hash);
    if (it != end()) {
        return {it, false};
    }

    if (m_impl.read().recent.size() > MAX_RECENT_SIZE) {
        promote_all_recent();
    }

    auto block_ptr = std::make_shared<CBlockIndex>(header);
    return try_emplace_impl(hash, std::move(block_ptr));
}

BlockMap::iterator BlockMap::find(const uint256& hash)
{
    const auto& impl = m_impl.read();

    auto recent_it = impl.recent.find(hash);
    if (recent_it != impl.recent.end()) {
        auto& stable_map = impl.stable.read();

        return iterator(stable_map.end(), stable_map.end(),
                        recent_it, impl.recent.end());
    }

    const auto& stable_map = impl.stable.read();
    auto stable_it = stable_map.find(hash);
    if (stable_it != stable_map.end()) {
        return iterator(stable_it, stable_map.end(),
                        impl.recent.begin(), impl.recent.end());
    }

    return end();
}

BlockMap::const_iterator BlockMap::find(const uint256& hash) const
{
    const auto& impl = m_impl.read();

    auto recent_it = impl.recent.find(hash);
    if (recent_it != impl.recent.end()) {
        auto& stable_map = impl.stable.read();

        return const_iterator(stable_map.end(), stable_map.end(),
                              recent_it, impl.recent.end());
    }

    const auto& stable_map = impl.stable.read();
    auto stable_it = stable_map.find(hash);
    if (stable_it != stable_map.end()) {
        return const_iterator(stable_it, stable_map.end(),
                              impl.recent.begin(), impl.recent.end());
    }

    return end();
}

bool BlockMap::contains(const uint256& hash) const
{
    const auto& impl = m_impl.read();

    if (impl.recent.contains(hash)) {
        return true;
    }

    return impl.stable.read().contains(hash);
}

size_t BlockMap::size() const
{
    const auto& impl = m_impl.read();
    return impl.stable.read().size() + impl.recent.size();
}

bool BlockMap::empty() const
{
    const auto& impl = m_impl.read();
    return impl.stable.read().empty() && impl.recent.empty();
}

void BlockMap::promote_all_recent()
{
    auto& impl = m_impl.write();

    if (impl.recent.empty()) {
        return;
    }

    impl.stable.write(
        [&](const InnerMap& old_stable) {
            InnerMap new_stable = old_stable;
            for (const auto& [hash, block_ptr] : impl.recent) {
                new_stable.emplace(hash, block_ptr);
            }
            return new_stable;
        },
        [&](InnerMap& stable_map) {
            for (const auto& [hash, block_ptr] : impl.recent) {
                stable_map.emplace(hash, block_ptr);
            }
        });

    impl.recent.clear();
}

BlockMap::iterator BlockMap::erase(iterator pos)
{
    if (pos == end()) {
        return end();
    }

    auto& impl = m_impl.write();

    if (pos.m_in_stable) {
        uint256 hash = pos.m_stable_it->first;

        impl.stable.write(
            [&](const InnerMap& old_stable) {
                InnerMap new_stable = old_stable;
                new_stable.erase(hash);
                return new_stable;
            },
            [&](InnerMap& stable_map) {
                stable_map.erase(hash);
            });

        auto& stable_map = impl.stable.read();
        auto next_stable_it = stable_map.find(hash);
        if (next_stable_it == stable_map.end()) {
            next_stable_it = stable_map.begin();
        }

        return iterator(next_stable_it, stable_map.end(),
                        impl.recent.begin(), impl.recent.end());
    } else {
        auto next_it = impl.recent.erase(pos.m_recent_it);
        auto& stable_map = impl.stable.read();
        return iterator(stable_map.end(), stable_map.end(),
                        next_it, impl.recent.end());
    }
}
