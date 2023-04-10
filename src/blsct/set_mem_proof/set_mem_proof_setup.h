// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_SETUP_H
#define NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_SETUP_H

#include <vector>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/elements.h>
#include <blsct/building_block/generator_deriver.h>

// N is the maximum size of the set of membership public key
class SetMemProofSetup {
public:
    using Scalar = Mcl::Scalar;
    using Point = Mcl::Point;
    using Scalars = Elements<Scalar>;
    using Points = Elements<Point>;

    const size_t N = 1ull << 10;   // N must be a power of 2

    // Generators
    const Point g = Point::GetBasePoint();
    const Point h = m_deriver.Derive(g, 0);
    const Points hs = GenGenerators(h, N);

    // Hash functions
    Scalar Hash(const std::vector<uint8_t>& msg, uint8_t index) const;
    Scalar H1(const std::vector<uint8_t>& msg) const;
    Scalar H2(const std::vector<uint8_t>& msg) const;
    Scalar H3(const std::vector<uint8_t>& msg) const;
    Scalar H4(const std::vector<uint8_t>& msg) const;

    Point H5(const std::vector<uint8_t>& msg) const;
    Point H6(const std::vector<uint8_t>& msg) const;
    Point H7(const std::vector<uint8_t>& msg) const;

    Point PedersenCommitment(const Scalar& m, const Scalar& f) const;

#ifndef BOOST_UNIT_TEST
private:
#endif
    inline static const GeneratorDeriver m_deriver = GeneratorDeriver("set_membership_proof");
    static Points GenGenerators(const Point& base_point, const size_t& size);
};

#endif // NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_SETUP_H
