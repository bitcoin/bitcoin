#pragma once

#include <unordered_map>
#include <vector>
#include <memory>

template<typename K, typename V>
class OrderedMultimap
{
public:
    //
    // Inserts a new element with the given key and value.
    //
    void insert(const std::pair<K, std::shared_ptr<const V>>& pair)
    {
        auto iter = m_map.find(pair.first);
        if (iter != m_map.cend())
        {
            iter->second.push_back(pair.second);
        }
        else
        {
            m_map.insert({ pair.first, std::vector<std::shared_ptr<const V>>({ pair.second }) });
        }
    }

    //
    // Returns the last element with the given key.
    // If none found, returns nullptr.
    //
    std::shared_ptr<const V> find_last(const K& key) const noexcept
    {
        auto iter = m_map.find(key);
        if (iter != m_map.cend() && !iter->second.empty())
        {
            return iter->second.back();
        }

        return nullptr;
    }

    //
    // Erases the most-recently-added element with the given key.
    // 
    void erase(const K& key) noexcept
    {
        auto iter = m_map.find(key);
        if (iter != m_map.cend() && !iter->second.empty())
        {
            iter->second.resize(iter->second.size() - 1);
        }
    }

private:
    std::unordered_map<K, std::vector<std::shared_ptr<const V>>> m_map;
};