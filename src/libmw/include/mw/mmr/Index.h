#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Macros.h>
#include <cstdint>
#include <cassert>
#include <memory>

MMR_NAMESPACE

class Index
{
public:
    Index() noexcept = default;
    Index(const uint64_t position, const uint64_t height) noexcept
        : m_position(position), m_height(height) { }
    static Index At(const uint64_t position) noexcept;

    bool operator==(const Index& rhs) const noexcept { return m_position == rhs.m_position && m_height == rhs.m_height; }
    bool operator!=(const Index& rhs) const noexcept { return m_position != rhs.m_position || m_height != rhs.m_height; }
    bool operator<(const Index& rhs) const noexcept { return m_position < rhs.m_position; }
    bool operator<=(const Index& rhs) const noexcept { return m_position <= rhs.m_position; }
    bool operator>(const Index& rhs) const noexcept { return m_position > rhs.m_position; }
    bool operator>=(const Index& rhs) const noexcept { return m_position >= rhs.m_position; }

    bool operator==(const uint64_t position) const noexcept { return m_position == position; }
    bool operator!=(const uint64_t position) const noexcept { return m_position != position; }
    bool operator<(const uint64_t position) const noexcept { return m_position < position; }
    bool operator<=(const uint64_t position) const noexcept { return m_position <= position; }
    bool operator>(const uint64_t position) const noexcept { return m_position > position; }
    bool operator>=(const uint64_t position) const noexcept { return m_position >= position; }

    mmr::Index& operator++() {
        *this = this->GetNext();
        return *this;
    }

    bool IsLeaf() const noexcept { return m_height == 0; }
    uint64_t GetPosition() const noexcept { return m_position; }
    uint64_t GetLeafIndex() const noexcept;

    //
    // Gets the height of the node (position).
    // position is the zero-based postorder traversal index of a node in the tree.
    //
    // Height      Index
    //
    // 2:            6              |
    //              / \             |
    //             /   \            |
    // 1:         2     5           |
    //           / \   / \          |
    // 0:       0   1 3   4         |
    //
    uint64_t GetHeight() const noexcept { return m_height; }

    Index GetNext() const noexcept { return Index::At(m_position + 1); }

    Index GetParent() const noexcept;
    Index GetSibling() const noexcept;

    uint64_t left_child_pos() const noexcept { return GetLeftChild().GetPosition(); }
    Index GetLeftChild() const noexcept;

    Index GetRightChild() const noexcept;
    uint64_t right_child_pos() const noexcept { return GetRightChild().GetPosition(); }

protected:
    static uint64_t CalculateHeight(const uint64_t position) noexcept;
    static uint64_t CalculateLeafIndex(const uint64_t position) noexcept;

    uint64_t m_position;
    uint64_t m_height;
};

END_NAMESPACE