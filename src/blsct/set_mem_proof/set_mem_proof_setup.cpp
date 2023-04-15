#include <blsct/arith/elements.h>
#include <blsct/set_mem_proof/set_mem_proof_setup.h>
#include <blsct/building_block/generator_deriver.h>
#include <crypto/sha256.h>
#include <ctokens/tokenid.h>

using Scalar = Mcl::Scalar;
using Point = Mcl::Point;
using Points = Elements<Point>;

const SetMemProofSetup& SetMemProofSetup::Get()
{
    static SetMemProofSetup* x = nullptr;
    if (x == nullptr) {
        Point g = Point::GetBasePoint();
        Point h = m_deriver.Derive(g, 0);
        Points hs = GenGenerators(h, N);
        x = new SetMemProofSetup(g, h, hs);
    }
    return *x;
}

Points SetMemProofSetup::GenGenerators(const Point& base_point, const size_t& size)
{
    Points ps;

    for (size_t i=0; i<size; ++i) {
        Point p = m_deriver.Derive(base_point, i);
        ps.Add(p);
    }
    return ps;
}

Scalar SetMemProofSetup::Hash(const std::vector<uint8_t>& msg, uint8_t index) const
{
    std::vector<uint8_t> vec;
    vec.resize(msg.size() + 1);
    vec[0] = index;
    std::copy(msg.begin(), msg.end(), &vec[1]);

    CSHA256 hasher; // creating local instance for thread safely
    hasher.Write(&vec[0], vec.size());
    uint8_t hash[CSHA256::OUTPUT_SIZE];
    hasher.Finalize(hash);

    Scalar ret;
    if (mclBnFr_setLittleEndianMod(&ret.m_fr, hash, CSHA256::OUTPUT_SIZE) == -1) {
        throw std::runtime_error("Hash size is greater than or equal to m_fr size * 2. Check code");
    }
    return ret;
}

Scalar SetMemProofSetup::H1(const std::vector<uint8_t>& msg) const
{
    return Hash(msg, 1);
}

Scalar SetMemProofSetup::H2(const std::vector<uint8_t>& msg) const
{
    Scalar ret = Hash(msg, 2);
    return ret;
}

Scalar SetMemProofSetup::H3(const std::vector<uint8_t>& msg) const
{
    Scalar ret = Hash(msg, 3);
    return ret;
}

Scalar SetMemProofSetup::H4(const std::vector<uint8_t>& msg) const
{
    Scalar ret = Hash(msg, 4);
    return ret;
}

Point SetMemProofSetup::H5(const std::vector<uint8_t>& msg) const
{
    Scalar x;
    x.SetVch(msg);
    return x.GetHashWithSalt(5);
}

Point SetMemProofSetup::H6(const std::vector<uint8_t>& msg) const
{
    Scalar x;
    x.SetVch(msg);
    return x.GetHashWithSalt(6);
}

Point SetMemProofSetup::H7(const std::vector<uint8_t>& msg) const
{
    Scalar x;
    x.SetVch(msg);
    return x.GetHashWithSalt(7);
}
