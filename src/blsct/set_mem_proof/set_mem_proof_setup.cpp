#include <blsct/arith/elements.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/set_mem_proof/set_mem_proof_setup.h>
#include <blsct/building_block/generator_deriver.h>
#include <ctokens/tokenid.h>
#include <util/strencodings.h>

template <typename T>
const SetMemProofSetup<T>& SetMemProofSetup<T>::Get()
{
    using Point = typename T::Point;
    using Points = Elements<Point>;
    static SetMemProofSetup<T>* x = nullptr;

    std::lock_guard<std::mutex> lock(m_init_mutex);
    if (m_is_initialized) return *x;

    Point g = Point::GetBasePoint();
    Point h = m_deriver.Derive(g, 0);
    Points hs = GenGenerators(h, N);
    PedersenCommitment<T> pedersen_commitment(g, h);
    x = new SetMemProofSetup<T>(g, h, hs, pedersen_commitment);

    m_is_initialized = true;
    return *x;
}
template
const SetMemProofSetup<Mcl>& SetMemProofSetup<Mcl>::Get();

template <typename T>
typename SetMemProofSetup<T>::Points SetMemProofSetup<T>::GenGenerators(
    const typename T::Point& base_point,
    const size_t& size
) {
    Points ps;

    for (size_t i=0; i<size; ++i) {
        Point p = m_deriver.Derive(base_point, i);
        ps.Add(p);
    }
    return ps;
}
template
typename SetMemProofSetup<Mcl>::Points SetMemProofSetup<Mcl>::GenGenerators(
    const typename Mcl::Point& base_point,
    const size_t& size
);

template <typename T>
typename T::Scalar SetMemProofSetup<T>::H1(const std::vector<uint8_t>& msg) const
{
    Scalar ret(msg, 1);
    return ret;
}
template
typename Mcl::Scalar SetMemProofSetup<Mcl>::H1(const std::vector<uint8_t>& msg) const;

template <typename T>
typename T::Scalar SetMemProofSetup<T>::H2(const std::vector<uint8_t>& msg) const
{
    Scalar ret(msg, 2);
    return ret;
}
template
typename Mcl::Scalar SetMemProofSetup<Mcl>::H2(const std::vector<uint8_t>& msg) const;

template <typename T>
typename T::Scalar SetMemProofSetup<T>::H3(const std::vector<uint8_t>& msg) const
{
    Scalar ret(msg, 3);
    return ret;
}
template
typename Mcl::Scalar SetMemProofSetup<Mcl>::H3(const std::vector<uint8_t>& msg) const;

template <typename T>
typename T::Scalar SetMemProofSetup<T>::H4(const std::vector<uint8_t>& msg) const
{
    Scalar ret(msg, 4);
    return ret;
}
template
typename Mcl::Scalar SetMemProofSetup<Mcl>::H4(const std::vector<uint8_t>& msg) const;

template <typename T>
typename T::Point SetMemProofSetup<T>::GenPoint(const std::vector<uint8_t>& msg, const uint64_t& i)
{
    HashWriter hasher{};
    hasher << msg;
    hasher << i;
    uint256 hash = hasher.GetHash();
    Point p(hash);
    return p;
}
template
typename Mcl::Point SetMemProofSetup<Mcl>::GenPoint(const std::vector<uint8_t>& msg, const uint64_t& i);

template <typename T>
typename T::Point SetMemProofSetup<T>::H5(const std::vector<uint8_t>& msg) const
{
    return GenPoint(msg, 5);
}
template
typename Mcl::Point SetMemProofSetup<Mcl>::H5(const std::vector<uint8_t>& msg) const;

template <typename T>
typename T::Point SetMemProofSetup<T>::H6(const std::vector<uint8_t>& msg) const
{
    return GenPoint(msg, 6);
}
template
typename Mcl::Point SetMemProofSetup<Mcl>::H6(const std::vector<uint8_t>& msg) const;

template <typename T>
typename T::Point SetMemProofSetup<T>::H7(const std::vector<uint8_t>& msg) const
{
    return GenPoint(msg, 7);
}
template
typename Mcl::Point SetMemProofSetup<Mcl>::H7(const std::vector<uint8_t>& msg) const;
