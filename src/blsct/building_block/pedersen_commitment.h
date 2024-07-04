// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_BUILDING_BLOCK_PEDERSEN_COMMITMENT_H
#define NAVCOIN_BLSCT_BUILDING_BLOCK_PEDERSEN_COMMITMENT_H

template <typename T>
class PedersenCommitment {
public:
    PedersenCommitment(
        const typename T::Point& g,
        const typename T::Point& h
    ): m_g{g}, m_h{h} {}

    typename T::Point Commit(
        const typename T::Scalar& m,
        const typename T::Scalar& f
    ) const {
        return m_g * m + m_h * f;
    }

private:
    const typename T::Point m_g;
    const typename T::Point m_h;
};

#endif // NAVCOIN_BLSCT_BUILDING_BLOCK_PEDERSEN_COMMITMENT_H
