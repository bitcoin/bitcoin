#include <mw/mmr/Index.h>
#include <mw/mmr/LeafIndex.h>

using namespace mmr;

Index Index::At(const uint64_t position) noexcept
{
    return Index(position, CalculateHeight(position));
}

uint64_t Index::GetLeafIndex() const noexcept
{
    assert(IsLeaf());

    return CalculateLeafIndex(m_position);
}

Index Index::GetParent() const noexcept
{
    if (CalculateHeight(m_position + 1) == (m_height + 1))
    {
        // Index points to a right sibling, so the next node is the parent.
        return Index(m_position + 1, m_height + 1);
    }
    else
    {
        // Index is the left sibling, so the parent node is Index + 2^(height + 1).
        return Index(m_position + (1ULL << (m_height + 1)), m_height + 1);
    }
}

Index Index::GetSibling() const noexcept
{
    if (CalculateHeight(m_position + 1) == (m_height + 1))
    {
        // Index points to a right sibling, so add 1 and subtract 2^(height + 1) to get the left sibling.
        return Index(m_position + 1 - (1ULL << (m_height + 1)), m_height);
    }
    else
    {
        // Index is the left sibling, so add 2^(height + 1) - 1 to get the right sibling.
        return Index(m_position + (1ULL << (m_height + 1)) - 1, m_height);
    }
}

Index Index::GetLeftChild() const noexcept
{
    assert(m_height > 0);

    return Index(m_position - (1ULL << m_height), m_height - 1);
}

Index Index::GetRightChild() const noexcept
{
    assert(m_height > 0);

    return Index(m_position - 1, m_height - 1);
}

uint64_t Index::CalculateHeight(const uint64_t position) noexcept
{
    uint64_t height = position;
    uint64_t peakSize = BitUtil::FillOnesToRight(position);
    while (peakSize != 0)
    {
        if (height >= peakSize)
        {
            height -= peakSize;
        }

        peakSize >>= 1;
    }

    return height;
}

uint64_t Index::CalculateLeafIndex(const uint64_t position) noexcept
{
    uint64_t leafIndex = 0;

    uint64_t peakSize = BitUtil::FillOnesToRight(position);
    uint64_t numLeft = position;
    while (peakSize != 0)
    {
        if (numLeft >= peakSize)
        {
            leafIndex += ((peakSize + 1) / 2);
            numLeft -= peakSize;
        }

        peakSize >>= 1;
    }

    return leafIndex;
}