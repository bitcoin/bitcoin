#ifndef CROWNCOIN_BLOCKWITNESS_H
#define CROWNCOIN_BLOCKWITNESS_H

#include <primitives/transaction.h>

#include <vector>

class BlockWitness
{
public:
    CTxIn m_vin;
    uint256 m_hashBlock;

    BlockWitness(const CTxIn& vin, const uint256& hashBlock) : m_vin(vin), m_hashBlock(hashBlock) {}
    bool operator<(const BlockWitness& other) const
    {
        if (m_hashBlock != other.m_hashBlock)
            return m_hashBlock < other.m_hashBlock;
        return m_vin.prevout < other.m_vin.prevout;
    }
};

#endif //CROWNCOIN_BLOCKWITNESS_H
