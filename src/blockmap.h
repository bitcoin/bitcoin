#ifndef BITCOIN_BLOCKMAP_H
#define BITCOIN_BLOCKMAP_H

#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/copy_on_write.h>
#include <util/hasher.h>

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>

class BlockMap
{
private:
    using BlockPtr = std::shared_ptr<CBlockIndex>;
    using InnerMap = std::unordered_map<uint256, BlockPtr, BlockHasher>;

    struct Impl {
        stlab::copy_on_write<InnerMap> stable;

        InnerMap recent;

        Impl() = default;
        Impl(InnerMap stable_map, InnerMap recent_map) : stable(std::move(stable_map)), recent(std::move(recent_map)) {}
    };

    stlab::copy_on_write<Impl> m_impl;

    static constexpr size_t MAX_RECENT_SIZE = 1000;

public:
    class const_iterator;
    class iterator
    {
        friend class BlockMap;
        friend class const_iterator;

    private:
        InnerMap::const_iterator m_stable_it;
        InnerMap::const_iterator m_stable_end;
        InnerMap::const_iterator m_recent_it;
        InnerMap::const_iterator m_recent_end;
        bool m_in_stable;

        mutable std::pair<uint256, CBlockIndex*> m_cached_value;

        void update_cache() const
        {
            if (m_in_stable) {
                m_cached_value = {m_stable_it->first, m_stable_it->second.get()};
            } else {
                m_cached_value = {m_recent_it->first, m_recent_it->second.get()};
            }
        }

        void advance_if_at_end()
        {
            if (m_in_stable && m_stable_it == m_stable_end) {
                m_in_stable = false;
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const uint256, CBlockIndex*>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

    public:
        iterator() : m_in_stable(false), m_cached_value({uint256(), nullptr}) {}

        iterator(InnerMap::const_iterator stable_begin, InnerMap::const_iterator stable_end,
                 InnerMap::const_iterator recent_begin, InnerMap::const_iterator recent_end,
                 bool at_end = false)
            : m_stable_it(stable_begin), m_stable_end(stable_end), m_recent_it(recent_begin), m_recent_end(recent_end), m_in_stable(!at_end && stable_begin != stable_end), m_cached_value({uint256(), nullptr})
        {
            if (at_end) {
                m_in_stable = false;
                m_recent_it = recent_end;
            } else if (m_in_stable || m_recent_it != m_recent_end) {
                update_cache();
            }
        }

        reference operator*() const
        {
            return reinterpret_cast<const value_type&>(m_cached_value);
        }

        pointer operator->() const
        {
            return reinterpret_cast<const value_type*>(&m_cached_value);
        }

        iterator& operator++()
        {
            if (m_in_stable) {
                ++m_stable_it;
                advance_if_at_end();
            } else {
                ++m_recent_it;
            }

            bool at_end = m_in_stable ? false : (m_recent_it == m_recent_end);
            if (!at_end) {
                update_cache();
            }

            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const
        {
            if (m_in_stable != other.m_in_stable) return false;
            if (m_in_stable) {
                return m_stable_it == other.m_stable_it;
            } else {
                return m_recent_it == other.m_recent_it;
            }
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }
    };

    class const_iterator
    {
    private:
        InnerMap::const_iterator m_stable_it;
        InnerMap::const_iterator m_stable_end;
        InnerMap::const_iterator m_recent_it;
        InnerMap::const_iterator m_recent_end;
        bool m_in_stable;

        mutable std::pair<uint256, CBlockIndex*> m_cached_value;

        void update_cache() const
        {
            if (m_in_stable) {
                m_cached_value = {m_stable_it->first, m_stable_it->second.get()};
            } else {
                m_cached_value = {m_recent_it->first, m_recent_it->second.get()};
            }
        }

        void advance_if_at_end()
        {
            if (m_in_stable && m_stable_it == m_stable_end) {
                m_in_stable = false;
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const uint256, const CBlockIndex*>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

    public:
        const_iterator() : m_in_stable(false), m_cached_value({uint256(), nullptr}) {}

        const_iterator(InnerMap::const_iterator stable_begin, InnerMap::const_iterator stable_end,
                       InnerMap::const_iterator recent_begin, InnerMap::const_iterator recent_end,
                       bool at_end = false)
            : m_stable_it(stable_begin), m_stable_end(stable_end), m_recent_it(recent_begin), m_recent_end(recent_end), m_in_stable(!at_end && stable_begin != stable_end), m_cached_value({uint256(), nullptr})
        {
            if (at_end) {
                m_in_stable = false;
                m_recent_it = recent_end;
            } else if (m_in_stable || m_recent_it != m_recent_end) {
                update_cache();
            }
        }

        const_iterator(const iterator& other)
            : m_stable_it(other.m_stable_it),
              m_stable_end(other.m_stable_end),
              m_recent_it(other.m_recent_it),
              m_recent_end(other.m_recent_end),
              m_in_stable(other.m_in_stable),
              m_cached_value(other.m_cached_value)
        {
        }

        reference operator*() const
        {
            return reinterpret_cast<const value_type&>(m_cached_value);
        }

        pointer operator->() const
        {
            return reinterpret_cast<const value_type*>(&m_cached_value);
        }

        const_iterator& operator++()
        {
            if (m_in_stable) {
                ++m_stable_it;
                advance_if_at_end();
            } else {
                ++m_recent_it;
            }

            bool at_end = m_in_stable ? false : (m_recent_it == m_recent_end);
            if (!at_end) {
                update_cache();
            }

            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator& other) const
        {
            if (m_in_stable != other.m_in_stable) return false;
            if (m_in_stable) {
                return m_stable_it == other.m_stable_it;
            } else {
                return m_recent_it == other.m_recent_it;
            }
        }

        bool operator!=(const const_iterator& other) const
        {
            return !(*this == other);
        }
    };

    iterator begin()
    {
        auto& impl = m_impl.read();
        auto& stable_map = impl.stable.read();
        return iterator(
            stable_map.begin(), stable_map.end(),
            impl.recent.begin(), impl.recent.end(),
            false);
    }

    iterator end()
    {
        auto& impl = m_impl.read();
        auto& stable_map = impl.stable.read();
        return iterator(
            stable_map.begin(), stable_map.end(),
            impl.recent.begin(), impl.recent.end(),
            true);
    }

    const_iterator begin() const
    {
        auto& impl = m_impl.read();
        auto& stable_map = impl.stable.read();
        return const_iterator(
            stable_map.begin(), stable_map.end(),
            impl.recent.begin(), impl.recent.end(),
            false);
    }

    const_iterator end() const
    {
        auto& impl = m_impl.read();
        auto& stable_map = impl.stable.read();
        return const_iterator(
            stable_map.begin(), stable_map.end(),
            impl.recent.begin(), impl.recent.end(),
            true);
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    const_iterator cend() const
    {
        return end();
    }

    BlockMap() = default;
    BlockMap(const BlockMap&) = default;
    BlockMap& operator=(const BlockMap&) = default;
    BlockMap(BlockMap&&) noexcept = default;
    BlockMap& operator=(BlockMap&&) noexcept = default;

    /**
     * Insert a new block index (default constructed).
     * Returns a pair of (pointer, inserted).
     * If block already exists, returns (pointer to exisitng, false).
     */
    std::pair<iterator, bool> try_emplace(const uint256& hash);

    /**
     * Insert a new block index constructed from CBlockHeader.
     * Returns pair of (pointer, inserted).
     * If block already exists, returns (pointer to existing, false).
     */
    std::pair<iterator, bool> try_emplace(const uint256& hash, const CBlockHeader& header);

    CBlockIndex* operator[](const uint256& hash)
    {
        auto [it, inserted] = try_emplace(hash);
        return it->second;
    }

    /**
     * Find a block index by hash.
     * Returns mutable pointer (for use under cs_main lock).
     */
    iterator find(const uint256& hash);

    /**
     * Find a block index by hash (const version).
     * Safe to use with snapshots.
     */
    const_iterator find(const uint256& hash) const;

    /**
     * Chec if block exists in the map.
     */
    bool contains(const uint256& hash) const;

    /**
     * Check if the map is empty (no blocks in stable or recent).
     */
    bool empty() const;

    /**
     * Get total number of blocks.
     */
    size_t size() const;

    /**
     * Erase a block at iterator position.
     * Returns iterator to the element following the erased element.
     * If pos == end(), returns end().
     * */
    iterator erase(iterator pos);

private:
    template <typename BlockPtrType>
    std::pair<iterator, bool> try_emplace_impl(const uint256& hash, BlockPtrType&& block_ptr)
    {
        auto& impl = m_impl.write();
        const auto& stable_map = impl.stable.read();

        auto stable_it = stable_map.find(hash);
        if (stable_it != stable_map.end()) {
            return {iterator(stable_it, stable_map.end(),
                             impl.recent.begin(), impl.recent.end()),
                    false};
        }

        auto recent_it = impl.recent.find(hash);
        if (recent_it != impl.recent.end()) {
            return {iterator(stable_map.end(), stable_map.end(),
                             recent_it, impl.recent.end()),
                    false};
        }

        auto [it, inserted] = impl.recent.emplace(hash, std::forward<BlockPtrType>(block_ptr));

        return {iterator(stable_map.end(), stable_map.end(),
                         it, impl.recent.end()),
                inserted};
    }

    /**
     * Promote all blocks from recent to stable.
     * Called when recent exceeds MAX_RECENT_SIZE threshold.
     */
    void promote_all_recent();
};


#endif // BITCOIN_BLOCKMAP_H
