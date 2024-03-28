// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BAG_H
#define BITCOIN_UTIL_BAG_H

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

/**
 * A container that holds a number of elements. Elements can be added and
 * removed, and the order how the elements are extracted is unspecified
 * and subject to optimization.
 */
template <typename T>
class Bag final
{
    std::vector<T> m_data{};

public:
    /**
     * Adds all elements from data to the bag.
     */
    void push(std::vector<T>&& data)
    {
        m_data.insert(m_data.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
    }

    /**
     * Removes n elements from the bag and puts them into out, replacing elements in out if it holds any.
     * If the bag holds less than n elements only the available elements will be put into out.
     */
    void pop(std::size_t n, std::vector<T>& out)
    {
        n = std::min(n, m_data.size());
        auto it = m_data.end() - n;
        out.assign(std::make_move_iterator(it), std::make_move_iterator(m_data.end()));
        m_data.erase(it, m_data.end());
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_data.empty();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_data.size();
    }
};

#endif // BITCOIN_UTIL_BAG_H
