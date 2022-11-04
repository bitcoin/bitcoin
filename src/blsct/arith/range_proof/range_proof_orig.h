// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#ifndef NAVCOIN_BLSCT_BULLETPROOFS_H
#define NAVCOIN_BLSCT_BULLETPROOFS_H

#ifdef _WIN32
/* Avoid redefinition warning. */
#undef ERROR
#undef WSIZE
#undef DOUBLE
#endif

#include <consensus/amount.h>
#include <blsct/arith/scalar.h>
#include <blsct/arith/g1point.h>
#include <ctokens/tokenid.h>
//#include <bls.hpp>
#include <streams.h>
//#include <utilstrencodings.h>

#define MCL_DONT_USE_XBYAK
#define MCL_DONT_USE_OPENSSL

//#include <mcl/bls12_381.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

// using namespace mcl::bn;

static const size_t maxN = 64;
static const size_t maxMessageSize = 54;
static const size_t maxM = 16;
static const size_t maxMN = maxM*maxN;

static const std::vector<uint8_t> balanceMsg = {'B', 'L', 'S', 'C', 'T', 'B', 'A', 'L', 'A', 'N', 'C', 'E'};

class MultiexpData {
public:
    G1Point base;
    Scalar exp;

    MultiexpData() {}
    MultiexpData(G1Point base_, Scalar exp_) : base(base_), exp(exp_){}
};

struct Generators {
    G1Point G;
    G1Point H;
    std::vector<G1Point> Gi;
    std::vector<G1Point> Hi;
};

class BulletproofsRangeproof
{
public:
    BulletproofsRangeproof() {}

    // BulletproofsRangeproof(std::vector<unsigned char> vData) {
    //     try
    //     {
    //         CDataStream strm(vData, 0, 0);
    //         strm >> *this;
    //     }
    //     catch(...)
    //     {

    //     }
    // }

    // std::vector<unsigned char> GetVch() const
    // {
    //     try
    //     {
    //         CDataStream strm(0, 0);
    //         strm << *this;
    //         return std::vector<unsigned char>(strm.begin(), strm.end());
    //     }
    //     catch(...)
    //     {

    //     }
    //     return std::vector<unsigned char>();
    // }

    static G1Point g1_zero;

    static bool Init();

    static Generators GetGenerators(const TokenId& tokenId=TokenId());

    void Prove(std::vector<Scalar> v, G1Point nonce, const std::vector<uint8_t>& message = std::vector<uint8_t>(), const TokenId& tokenId=TokenId(), const std::vector<Scalar>& useGammas=std::vector<Scalar>());

    bool operator==(const BulletproofsRangeproof& rh) const {
        return (V == rh.V &&
                L == rh.L &&
                R == rh.R &&
                A == rh.A &&
                S == rh.S &&
                T1 == rh.T1 &&
                T2 == rh.T2 &&
                taux == rh.taux &&
                mu == rh.mu &&
                a == rh.a &&
                b == rh.b &&
                t == rh.t);
    }

    // template <typename Stream>
    // void Serialize(Stream& s, int nType, int nVersion) const
    // {
    //     ::WriteCompactSize(s, V.size());
    //     for (auto&it: V){
    //         ::Serialize(s, it, nType, nVersion);
    //     }
    //     ::WriteCompactSize(s, L.size());
    //     for (auto&it: L){
    //         ::Serialize(s, it, nType, nVersion);
    //     }
    //     ::WriteCompactSize(s, R.size());
    //     for (auto&it: R){
    //         ::Serialize(s, it, nType, nVersion);
    //     }
    //     ::Serialize(s, A, nType, nVersion);
    //     ::Serialize(s, S, nType, nVersion);
    //     ::Serialize(s, T1, nType, nVersion);
    //     ::Serialize(s, T2, nType, nVersion);
    //     ::Serialize(s, taux, nType, nVersion);
    //     ::Serialize(s, mu, nType, nVersion);
    //     ::Serialize(s, a, nType, nVersion);
    //     ::Serialize(s, b, nType, nVersion);
    //     ::Serialize(s, t, nType, nVersion);
    // }

    // template <typename Stream>
    // void Unserialize(Stream& s, int nType, int nVersion)
    // {
    //         size_t v_size, l_size, r_size;
    //         v_size=::ReadCompactSize(s);
    //         for (auto i=0; i<v_size; i++)
    //         {
    //             G1Point n;
    //             ::Unserialize(s, n, nType, nVersion);
    //             V.push_back(n);
    //         }
    //         l_size=::ReadCompactSize(s);
    //         for (auto i=0; i<l_size; i++)
    //         {
    //             G1Point n;
    //             ::Unserialize(s, n, nType, nVersion);
    //             L.push_back(n);
    //         }
    //         r_size=::ReadCompactSize(s);
    //         for (auto i=0; i<r_size; i++)
    //         {
    //             G1Point n;
    //             ::Unserialize(s, n, nType, nVersion);
    //             R.push_back(n);
    //         }
    //         ::Unserialize(s, A, nType, nVersion);
    //         ::Unserialize(s, S, nType, nVersion);
    //         ::Unserialize(s, T1, nType, nVersion);
    //         ::Unserialize(s, T2, nType, nVersion);
    //         ::Unserialize(s, taux, nType, nVersion);
    //         ::Unserialize(s, mu, nType, nVersion);
    //         ::Unserialize(s, a, nType, nVersion);
    //         ::Unserialize(s, b, nType, nVersion);
    //         ::Unserialize(s, t, nType, nVersion);
    // }

    std::vector<G1Point> GetValueCommitments() const { return V; }

    static const size_t logN = 6;

    static G1Point* G;
    static std::map<TokenId, G1Point> H;

    static Scalar* one;
    static Scalar* two;

    static std::vector<G1Point> Hi, Gi;  // hh, gg
    static std::vector<Scalar> oneN;  // one_n
    static std::vector<Scalar> twoN;  // tow_n
    static Scalar* ip12;

    static boost::mutex init_mutex;

    std::vector<G1Point> V;
    std::vector<G1Point> L;
    std::vector<G1Point> R;
    G1Point A;
    G1Point S;
    G1Point T1;
    G1Point T2;
    Scalar taux;
    Scalar mu;
    Scalar a;
    Scalar b;
    Scalar t;
};

struct RangeproofEncodedData
{
    CAmount amount;
    Scalar gamma;
    std::string message;
    int index;
    bool valid = false;
};

bool VerifyBulletproof(const std::vector<std::pair<int, BulletproofsRangeproof>>& proofs, std::vector<RangeproofEncodedData>& data, const std::vector<G1Point>& nonces, const bool &fOnlyRecover = false, const TokenId& tokenId=TokenId());

#endif // NAVCOIN_BLSCT_BULLETPROOFS_H
