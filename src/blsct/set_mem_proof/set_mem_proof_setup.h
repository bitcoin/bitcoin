// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_SETUP_H
#define NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_SETUP_H

#include <vector>
#include <blsct/arith/elements.h>
#include <blsct/building_block/generator_deriver.h>
#include <blsct/building_block/pedersen_commitment.h>
#include <blsct/range_proof/generators.h>
#include <mutex>

// N is the maximum size of the set of membership public key
template <typename T>
class SetMemProofSetup {
public:
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Scalars = Elements<Scalar>;
    using Points = Elements<Point>;

    static inline const size_t N = 1ull << 10;   // N must be a power of 2

    static const SetMemProofSetup& Get();

    // Generators
    const Point g;
    const Point h;
    const Points hs;

    const range_proof::GeneratorsFactory<T>& Gf() const;

    // Hash functions
    Scalar H1(const std::vector<uint8_t>& msg) const;
    Scalar H2(const std::vector<uint8_t>& msg) const;
    Scalar H3(const std::vector<uint8_t>& msg) const;
    Scalar H4(const std::vector<uint8_t>& msg) const;

    Point H5(const std::vector<uint8_t>& msg) const;

    const PedersenCommitment<T> pedersen;

#ifndef BOOST_UNIT_TEST
private:
#endif
    SetMemProofSetup(
        const Point& g,
        const Point& h,
        const Points& hs,
        const PedersenCommitment<T>& pedersen
    ): g{g}, h{h}, hs{hs}, pedersen{pedersen} {}

    static Point GenPoint(const std::vector<uint8_t>& msg, const uint64_t& i);
    static Points GenGenerators(const Point& base_point, const size_t& size);

    inline static const GeneratorDeriver m_deriver = GeneratorDeriver<Point>("set_membership_proof");

    inline static range_proof::GeneratorsFactory<T>* m_gf;
    inline static std::mutex m_init_mutex;
    inline static bool m_is_initialized = false;
};

#endif // NAVCOIN_BLSCT_SET_MEM_PROOF_SET_MEM_SETUP_H
