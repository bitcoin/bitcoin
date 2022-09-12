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

#include <amount.h>
#include <blsct/scalar.h>
#include <ctokens/tokenid.h>
#include <bls.hpp>
#include <streams.h>
#include <utilstrencodings.h>

#define MCL_DONT_USE_XBYAK
#define MCL_DONT_USE_OPENSSL

#include <mcl/bls12_381.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

using namespace mcl::bn;

static const size_t maxN = 64;
static const size_t maxMessageSize = 54;
static const size_t maxM = 16;
static const size_t maxMN = maxM*maxN;

static const std::vector<uint8_t> balanceMsg = {'B', 'L', 'S', 'C', 'T', 'B', 'A', 'L', 'A', 'N', 'C', 'E'};

class MultiexpData {
public:
    bls::G1Element base;
    Scalar exp;

    MultiexpData() {}
    MultiexpData(bls::G1Element base_, Scalar exp_) : base(base_), exp(exp_){}
};

struct Generators {
    bls::G1Element G;
    bls::G1Element H;
    std::vector<bls::G1Element> Gi;
    std::vector<bls::G1Element> Hi;
};

class BulletproofsRangeproof
{
public:
    BulletproofsRangeproof() {}

    BulletproofsRangeproof(std::vector<unsigned char> vData) {
        try
        {
            CDataStream strm(vData, 0, 0);
            strm >> *this;
        }
        catch(...)
        {

        }
    }

    std::vector<unsigned char> GetVch() const
    {
        try
        {
            CDataStream strm(0, 0);
            strm << *this;
            return std::vector<unsigned char>(strm.begin(), strm.end());
        }
        catch(...)
        {

        }
        return std::vector<unsigned char>();
    }

    static bls::G1Element g1_zero;

    static bool Init();

    static Generators GetGenerators(const TokenId& tokenId=TokenId());

    void Prove(std::vector<Scalar> v, bls::G1Element nonce, const std::vector<uint8_t>& message = std::vector<uint8_t>(), const TokenId& tokenId=TokenId(), const std::vector<Scalar>& useGammas=std::vector<Scalar>());

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

    std::vector<bls::G1Element> GetValueCommitments() const { return V; }

    static const size_t logN = 6;

    static bls::G1Element G;
    static std::map<TokenId, bls::G1Element> H;

    static Scalar one;
    static Scalar two;

    static std::vector<bls::G1Element> Hi, Gi;  // hh, gg
    static std::vector<Scalar> oneN;  // one_n
    static std::vector<Scalar> twoN;  // tow_n
    static Scalar ip12;

    static boost::mutex init_mutex;

    std::vector<bls::G1Element> V;
    std::vector<bls::G1Element> L;
    std::vector<bls::G1Element> R;
    bls::G1Element A;
    bls::G1Element S;
    bls::G1Element T1;
    bls::G1Element T2;
    Scalar taux;   // tau_x
    Scalar mu;
    Scalar a;
    Scalar b;
    Scalar t;  // t_hat
};

struct RangeproofEncodedData
{
    CAmount amount;
    Scalar gamma;
    std::string message;
    int index;
    bool valid = false;
};

bool VerifyBulletproof(const std::vector<std::pair<int, BulletproofsRangeproof>>& proofs, std::vector<RangeproofEncodedData>& data, const std::vector<bls::G1Element>& nonces, const bool &fOnlyRecover = false, const TokenId& tokenId=TokenId());

#endif // NAVCOIN_BLSCT_BULLETPROOFS_H
