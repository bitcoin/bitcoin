// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#include <boost/algorithm/string.hpp>

#include <blsct/arith/range_proof/range_proof_orig.h>
#include <tinyformat.h>
#include <util/strencodings.h>
//#include <utiltime.h>)

#define CHECK_AND_ASSERT_THROW_MES(expr, message) do {if(!(expr)) throw std::runtime_error(message);} while(0)

//bool BLSInitResult = true; //bls::BLS::Init();

static std::vector<Scalar> VectorPowers(const Scalar &x, size_t n);
static std::vector<Scalar> VectorDup(const Scalar &x, size_t n);
static Scalar InnerProduct(const std::vector<Scalar> &a, const std::vector<Scalar> &b);

Scalar* BulletproofsRangeproof::one;
Scalar* BulletproofsRangeproof::two;

std::vector<G1Point> BulletproofsRangeproof::Hi, BulletproofsRangeproof::Gi;
std::vector<Scalar> BulletproofsRangeproof::oneN;
std::vector<Scalar> BulletproofsRangeproof::twoN;
Scalar* BulletproofsRangeproof::ip12;

boost::mutex BulletproofsRangeproof::init_mutex;

G1Point* BulletproofsRangeproof::G;
std::map<TokenId, G1Point> BulletproofsRangeproof::H;

// Calculate base point
static G1Point GetBaseG1Element(const G1Point &base, size_t idx, std::string tokId = "", uint64_t tokNftId = -1)
{
    static const std::string salt("bulletproof");
    std::vector<uint8_t> data =  base.GetVch();
    std::string toHash = HexStr(data) + salt + std::to_string(idx) + tokId + (tokNftId != -1 ? "nft"+ std::to_string(tokNftId) : "");

    CHashWriter ss(SER_GETHASH, 0);
    ss << toHash;
    uint256 hash = ss.GetHash();

    // uint8_t dest[1];
    // dest[0] = 0x0;

    // std::vector<unsigned char> vMcl(48);

    // if (tokId != "")
    // {
    //     Fp p;
    //     auto vHash = std::vector<unsigned char>(hash.begin(), hash.end());
    //     p.setLittleEndianMod(&vHash[0], 32);
    //     G1 g;
    //     mapToG1(g, p);
    //     g.serialize(&vMcl[0], 48);
    // }

    //G1Point e = tokId == "" ? G1Point::FromMessage(std::vector<unsigned char>(hash.begin(), hash.end()), dest, 1) : G1Point::SetVch(vMcl);
    auto vec_hash = std::vector<uint8_t>(hash.begin(), hash.end());
    auto e = G1Point::MapToG1(vec_hash);
    CHECK_AND_ASSERT_THROW_MES(!e.IsUnity(), "GetBaseG1Element: Exponent is point at infinity");

    return e;
}

// Initialize bases and constants
bool BulletproofsRangeproof::Init()
{
    boost::lock_guard<boost::mutex> lock(BulletproofsRangeproof::init_mutex);

    static bool fInit = false;

    if (fInit)
        return true;

    // initPairing(mcl::BLS12_381);

    // Fp::setETHserialization(true);
    // Fr::setETHserialization(true);

    BulletproofsRangeproof::one = new Scalar(1);
    BulletproofsRangeproof::two = new Scalar(2);

    BulletproofsRangeproof::G = new G1Point(G1Point::GetBasePoint());
    BulletproofsRangeproof::H[TokenId()] = GetBaseG1Element(*BulletproofsRangeproof::G, 0);

    BulletproofsRangeproof::Hi.resize(maxMN);
    BulletproofsRangeproof::Gi.resize(maxMN);

    for (size_t i = 0; i < maxMN; ++i)
    {
        BulletproofsRangeproof::Hi[i] = GetBaseG1Element(BulletproofsRangeproof::H[TokenId()], i * 2 + 1);
        BulletproofsRangeproof::Gi[i] = GetBaseG1Element(BulletproofsRangeproof::H[TokenId()], i * 2 + 2);
    }

    BulletproofsRangeproof::oneN = VectorDup(*BulletproofsRangeproof::one, maxN);
    BulletproofsRangeproof::twoN = VectorPowers(*BulletproofsRangeproof::two, maxN);
    auto ip12 = InnerProduct(BulletproofsRangeproof::oneN, BulletproofsRangeproof::twoN);
    BulletproofsRangeproof::ip12 = &ip12;

    fInit = true;

    return true;
}

Generators BulletproofsRangeproof::GetGenerators(const TokenId& tokenId)
{
    if (BulletproofsRangeproof::H.count(tokenId))
    {
        return {*BulletproofsRangeproof::G, BulletproofsRangeproof::H[tokenId], BulletproofsRangeproof::Gi, BulletproofsRangeproof::Hi};
    }

    BulletproofsRangeproof::H[tokenId] = GetBaseG1Element(*BulletproofsRangeproof::G, 0, tokenId.token.ToString(), tokenId.subid);

    return {*BulletproofsRangeproof::G, BulletproofsRangeproof::H[tokenId], BulletproofsRangeproof::Gi, BulletproofsRangeproof::Hi};
}

// Todo multi-exp optimization
G1Point MultiExp(std::vector<MultiexpData> multiexp_data)
{
    std::vector<mclBnG1> points;
    std::vector<mclBnFr> exps;

    for (auto me: multiexp_data) {
        points.push_back(me.base.m_p);
        exps.push_back(me.exp.m_fr);
    }
    G1Point result = G1Point::MulVec(points, exps);

    return result;
}

G1Point MultiExpLegacy(std::vector<MultiexpData> multiexp_data)
{
    G1Point result;

    for (size_t i = 0; i < multiexp_data.size(); i++)
    {
        if (i == 0)
            result = multiexp_data[i].base * multiexp_data[i].exp;
        else
        {
            G1Point temp = G1Point(multiexp_data[i].base * multiexp_data[i].exp);
            result = result + temp;
        }
    }

    return result;
}

// generates A in (44) and S in (47)
/* Given two Scalar arrays, construct a vector commitment */
static G1Point VectorCommitment(const std::vector<Scalar> &a, const std::vector<Scalar> &b, const TokenId& tokenId=TokenId())
{
    CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");
    CHECK_AND_ASSERT_THROW_MES(a.size() <= maxMN, "Incompatible sizes of a and maxN");

    Generators gens = BulletproofsRangeproof::GetGenerators(tokenId);

    std::vector<MultiexpData> multiexp_data;
    for (size_t i = 0; i < a.size(); ++i)
    {
        multiexp_data.push_back({gens.Gi[i], a[i]});
        multiexp_data.push_back({gens.Hi[i], b[i]});
    }

    return MultiExp(multiexp_data);
}

/* Given a Scalar x, construct a vector of powers [x^0, x^1, ..., x^n] */
static std::vector<Scalar> VectorPowers(const Scalar &x, size_t n)
{
    std::vector<Scalar> res(n);

    if (n == 0)
        return res;

    res[0] = 1;

    if (n == 1)
        return res;

    res[1] = x;

    for (size_t i = 2; i < n; ++i)
    {
        res[i] = res[i-1] * x;
    }

    return res;
}

static Scalar VectorPowerSum(const Scalar &x, size_t n)
{
    std::vector<Scalar> res = VectorPowers(x, n);
    Scalar ret;
    ret = 0;

    for (auto& it: res)
    {
        ret = ret + it;
    }

    return ret;
}

/* Create a vector from copies of a single value */
static std::vector<Scalar> VectorDup(const Scalar &x, size_t n)
{
    std::vector<Scalar> ret(n);

    for (auto i=0;i<n;i++)
    {
        ret[i] = x;
    }

    return ret;
}

/* Given two Scalar arrays, construct the inner product */
static Scalar InnerProduct(const std::vector<Scalar> &a, const std::vector<Scalar> &b)
{
    CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");

    Scalar res;

    for (size_t i = 0; i < a.size(); ++i)
    {
        if (i == 0)
            res = (a[i] * b[i]);
        else
            res = res + (a[i] * b[i]);
    }
    return res;
}

static std::vector<Scalar> VectorProduct(const std::vector<Scalar> &a, const std::vector<Scalar> &b)
{
    CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");

    std::vector<Scalar> res(a.size());

    for (size_t i = 0; i < a.size(); ++i)
    {
        res[i] = (a[i] * b[i]);
    }
    return res;
}

/* Subtract a number from all the elements of a vectors */
static std::vector<Scalar> VectorSubtract(const std::vector<Scalar>& a, const Scalar& b)
{

    std::vector<Scalar> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = (a[i] - b);
    }

    return ret;
}

/* Given two scalar arrays, construct the Hadamard product */
std::vector<Scalar> Hadamard(const std::vector<Scalar>& a, const std::vector<Scalar>& b)
{
    if (a.size() != b.size())
        throw std::runtime_error("Hadamard(): a and b should be of the same size");

    std::vector<Scalar> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = (a[i] * b[i]);
    }

    return ret;
}

/* Add a number to all the elements of a vectors */
static std::vector<Scalar> VectorAdd(const std::vector<Scalar>& a, const Scalar& b)
{

    std::vector<Scalar> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = (a[i] + b);
    }

    return ret;
}

static std::vector<Scalar> VectorAdd(const std::vector<Scalar>& a, const std::vector<Scalar>& b)
{
    CHECK_AND_ASSERT_THROW_MES(a.size() == b.size(), "Incompatible sizes of a and b");

    std::vector<Scalar> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = (a[i] + b[i]);
    }

    return ret;
}

/* Multiply all the elements of a vector by a number */
static std::vector<Scalar> VectorScalar(const std::vector<Scalar>& a, const Scalar& x)
{
    std::vector<Scalar> ret(a.size());

    for (unsigned int i = 0; i < a.size(); i++)
    {
        ret[i] = (a[i] * x);
    }

    return ret;
}

/* Compute the slice of a scalar vector */
static std::vector<Scalar> VectorSlice(const std::vector<Scalar>& a, unsigned int start, unsigned int stop)
{
    if (start > a.size() || stop > a.size())
        throw std::runtime_error("VectorSlice(): wrong start or stop point");

    std::vector<Scalar> ret(stop-start);

    for (unsigned int i = start; i < stop; i++)
    {
        ret[i-start] = a[i];
    }

    return ret;
}

static std::vector<G1Point> HadamardFold(
    const std::vector<G1Point> &vec, const std::vector<Scalar> *scale, const Scalar &a, const Scalar &b)
{
    if(!((vec.size() & 1) == 0))
        throw std::runtime_error("HadamardFold(): vector argument size is not even");

    const size_t sz = vec.size() / 2;
    std::vector<G1Point> out(sz);

    for (size_t n = 0; n < sz; ++n)
    {
        G1Point c0 = vec[n];
        G1Point c1 = vec[sz + n];
        Scalar sa, sb;
        if (scale) sa = a*(*scale)[n]; else sa = a;
        if (scale) sb = b*(*scale)[sz + n]; else sb = b;
        G1Point l = c0*sa;
        G1Point r = c1*sb;
        out[n] = l + r;
    }

    return out;
}

std::vector<Scalar> VectorInvert(const std::vector<Scalar>& x)
{
    std::vector<Scalar> ret(x.size());

    for (size_t i = 0; i < ret.size(); i++)
    {
        ret[i] = x[i].Invert();
    }

    return ret;
}

G1Point CrossVectorExponent(
    size_t size,
    const std::vector<G1Point> &A,
    size_t Ao,
    const std::vector<G1Point> &B,
    size_t Bo,
    const std::vector<Scalar> &a,
    size_t ao,
    const std::vector<Scalar> &b,
    size_t bo,
    const std::vector<Scalar> *scale,
    const G1Point *extra_point,
    const Scalar *extra_scalar)
{
    if (!(size + Ao <= A.size()))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for A");

    if (!(size + Bo <= B.size()))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for B");

    if (!(size + ao <= a.size()))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for a");

    if (!(size + bo <= b.size()))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for b");

    if (!(size <= maxMN))
        throw std::runtime_error("CrossVectorExponent(): Size is too large");

    if (!(!scale || size == scale->size() / 2))
        throw std::runtime_error("CrossVectorExponent(): Incompatible size for scale");

    if (!(!!extra_point == !!extra_scalar))
        throw std::runtime_error("CrossVectorExponent(): Only one of extra base/exp present");

    std::vector<MultiexpData> multiexp_data;
    multiexp_data.resize(size*2 + (!!extra_point));
    for (size_t i = 0; i < size; ++i)
    {
        multiexp_data[i*2].exp = a[ao+i];
        multiexp_data[i*2].base = A[Ao+i];
        multiexp_data[i*2+1].exp = b[bo+i];

        if (scale)
            multiexp_data[i*2+1].exp =  multiexp_data[i*2+1].exp * (*scale)[Bo+i];

        multiexp_data[i*2+1].base = B[Bo+i];
    }
    if (extra_point)
    {
        multiexp_data.back().exp = *extra_scalar;
        multiexp_data.back().base = *extra_point;
    }

    return MultiExp(multiexp_data);
}

void BulletproofsRangeproof::Prove(std::vector<Scalar> v, G1Point nonce, const std::vector<uint8_t>& message, const TokenId& tokenId, const std::vector<Scalar>& useGammas)
{
    if (pow(2, BulletproofsRangeproof::logN) > maxN)
        throw std::runtime_error("BulletproofsRangeproof::Prove(): logN value is too high");

    if (message.size() > maxMessageSize)
        throw std::runtime_error("BulletproofsRangeproof::Prove(): message size is too big");

    if (v.empty() || v.size() > maxM)
        throw std::runtime_error("BulletproofsRangeproof::Prove(): Invalid vector size");

    CHECK_AND_ASSERT_THROW_MES(!v.empty(), "sv is empty");

    Init();

    Generators gens = GetGenerators(tokenId);

    const size_t N = 1<<BulletproofsRangeproof::logN;

    size_t M, logM;
    for (logM = 0; (M = 1<<logM) <= maxM && M < v.size(); ++logM);
    CHECK_AND_ASSERT_THROW_MES(M <= maxM, "sv is too large");
    const size_t logMN = logM + BulletproofsRangeproof::logN;
    const size_t MN = M * N;

    // V is a vector with commitments in the form g2^v g^gamma
    this->V.resize(v.size());

    std::vector<Scalar> gamma;
    gamma.resize(v.size());

    if (useGammas.size() > 0 && useGammas.size() == v.size())
    {
        gamma = useGammas;
    } else {
        for (auto i = 0; i < gamma.size(); i++)
        {
            gamma[i] = nonce.GetHashWithSalt(100+i);
        }
    }

    // This hash is updated for Fiat-Shamir throughout the proof
    CHashWriter hasher(0,0);

    for (unsigned int j = 0; j < v.size(); j++)
    {
        G1Point gammaElement = gens.G*gamma[j];
        G1Point valueElement = gens.H*v[j];
        this->V[j] = gammaElement + valueElement;
        hasher << this->V[j];
    }

    // PAPER LINES 41-42
    // Value to be obfuscated is encoded in binary in aL
    // aR is aL-1
    std::vector<Scalar> aL(MN), aR(MN);

    for (size_t j = 0; j < M; ++j)
    {
        for (size_t i = 0; i < N; ++i)
        {
            if (j < v.size() && v[j].GetBit(i) == 1)
            {
                aL[j*N+i] = 1;
                aR[j*N+i] = 0;
            }
            else
            {
                aL[j*N+i] = 0;
                aR[j*N+i] = 1;
                aR[j*N+i] = aR[j*N+i].Negate();
            }
        }
    }


try_again:
    // PAPER LINES 43-44
    // Commitment to aL and aR (obfuscated with alpha)
    Scalar alpha;
    std::vector<unsigned char> firstMessage;
    firstMessage = message.size() > 23 ? std::vector<unsigned char>(message.begin(), message.begin()+23) : message;
    Scalar sM = firstMessage;
    sM = sM << 8*8;

    alpha = nonce.GetHashWithSalt(1);
    alpha = alpha + (v[0] | sM);

    this->A = VectorCommitment(aL, aR);
    {
        G1Point alphaElement = gens.G*alpha;
        this->A = this->A + alphaElement;
    }

    // PAPER LINES 45-47
    // Commitment to blinding sL and sR (obfuscated with rho)
    std::vector<Scalar> sL(MN);
    std::vector<Scalar> sR(MN);

    for (unsigned int i = 0; i < MN; i++)
    {
        sL[i] = Scalar::Rand();
        sR[i] = Scalar::Rand();
    }

    Scalar rho;
    rho = nonce.GetHashWithSalt(2);

    this->S = VectorCommitment(sL, sR);
    {
        G1Point rhoElement = gens.G*rho;
        this->S = this->S + rhoElement;
    }

    // PAPER LINES 48-50
    hasher << this->A;
    hasher << this->S;

    Scalar y;
    y = hasher.GetHash();

    if (y == 0)
        goto try_again;

    hasher << y;

    Scalar z;
    z = hasher.GetHash();

    if (z == 0)
        goto try_again;

    // Polynomial construction by coefficients
    // PAPER LINE AFTER 50
    std::vector<Scalar> l0;
    std::vector<Scalar> l1;
    std::vector<Scalar> r0;
    std::vector<Scalar> r1;

    // l(x) = (aL - z 1^n) + sL X
    l0 = VectorSubtract(aL, z);

    // l(1) is (aL - z 1^n) + sL, but this is reduced to sL
    l1 = sL;

    // This computes the ugly sum/concatenation from page 19
    // Calculation of r(0) and r(1)
    std::vector<Scalar> zerosTwos(MN);
    std::vector<Scalar> zpow(M+2);

    zpow = VectorPowers(z, M+2);

    for (unsigned int j = 0; j < M; ++j)
    {
        for (unsigned int i = 0; i < N; ++i)
        {
            CHECK_AND_ASSERT_THROW_MES(j+2 < zpow.size(), "invalid zpow index");
            CHECK_AND_ASSERT_THROW_MES(i < twoN.size(), "invalid twoN index");
            zerosTwos[j*N+i] = zpow[j+2] * BulletproofsRangeproof::twoN[i];
        }
    }

    std::vector<Scalar> yMN = VectorPowers(y, MN);
    r0 = VectorAdd(Hadamard(VectorAdd(aR, z), yMN), zerosTwos);
    r1 = Hadamard(yMN, sR);

    // Polynomial construction before PAPER LINE 51
    Scalar t1 = InnerProduct(l0, r1) + InnerProduct(l1, r0);
    Scalar t2 = InnerProduct(l1, r1);

    // PAPER LINES 52-53
    Scalar tau1 = nonce.GetHashWithSalt(3);
    Scalar tau2 = nonce.GetHashWithSalt(4);
    std::vector<unsigned char> secondMessage = message.size() > 23 ? std::vector<unsigned char>(message.begin()+23, message.end()) : std::vector<unsigned char>();
    Scalar sM2 = secondMessage;
    tau1 = tau1 + sM2;

    {
        G1Point t1Element = gens.H*t1;
        G1Point t2Element = gens.H*t2;
        G1Point tau1Element = gens.G*tau1;
        G1Point tau2Element = gens.G*tau2;

        this->T1 = t1Element + tau1Element;
        this->T2 = t2Element + tau2Element;
    }

    // PAPER LINES 54-56
    hasher << z;
    hasher << this->T1;
    hasher << this->T2;

    Scalar x;
    x = hasher.GetHash();

    if (x == 0)
        goto try_again;

    // PAPER LINES 58-59
    std::vector<Scalar> l = VectorAdd(l0, VectorScalar(l1, x));
    std::vector<Scalar> r = VectorAdd(r0, VectorScalar(r1, x));

    // PAPER LINE 60
    this->t = InnerProduct(l, r);

    // TEST
    Scalar test_t;
    Scalar t0 = InnerProduct(l0, r0);
    test_t = ((t0 + (t1*x)) + (t2*x*x));
    if (!(test_t==this->t))
        throw std::runtime_error("BulletproofsRangeproof::Prove(): L60 Invalid test");

    // PAPER LINES 61-62
    this->taux = tau1 * x;
    this->taux = this->taux + (tau2 * x*x);

    for (unsigned int j = 1; j <= M; j++) // note this starts from 1
        this->taux = (this->taux + (zpow[j+1]*(gamma[j-1])));

    this->mu = ((x * rho) + alpha);

    // PAPER LINE 63
    hasher << x;
    hasher << this->taux;
    hasher << this->mu;
    hasher << this->t;

    Scalar x_ip;
    x_ip = hasher.GetHash();

    if (x_ip == 0)
        goto try_again;

    // These are used in the inner product rounds
    unsigned int nprime = MN;

    std::vector<G1Point> gprime(nprime);
    std::vector<G1Point> hprime(nprime);
    std::vector<Scalar> aprime(nprime);
    std::vector<Scalar> bprime(nprime);

    Scalar yinv = y.Invert();

    std::vector<Scalar> yinvpow(nprime);

    yinvpow[0] = 1;
    yinvpow[1] = yinv;

    for (unsigned int i = 0; i < nprime; i++)
    {
        gprime[i] = gens.Gi[i];
        hprime[i] = gens.Hi[i];

        if(i > 1)
            yinvpow[i] = yinvpow[i-1] * yinv;

        aprime[i] = l[i];
        bprime[i] = r[i];
    }

    this->L.resize(logMN);
    this->R.resize(logMN);

    unsigned int round = 0;

    std::vector<Scalar> w(logMN);

    std::vector<Scalar>* scale = &yinvpow;

    Scalar tmp;

    while (nprime > 1)
    {
        // PAPER LINE 20
        nprime /= 2;

        // PAPER LINES 21-22
        Scalar cL = InnerProduct(VectorSlice(aprime, 0, nprime),
                                 VectorSlice(bprime, nprime, bprime.size()));

        Scalar cR = InnerProduct(VectorSlice(aprime, nprime, aprime.size()),
                                 VectorSlice(bprime, 0, nprime));

        // PAPER LINES 23-24
        tmp = cL * x_ip;
        this->L[round] = CrossVectorExponent(nprime, gprime, nprime, hprime, 0, aprime, 0, bprime, nprime, scale, &gens.H, &tmp);
        tmp = cR * x_ip;
        this->R[round] = CrossVectorExponent(nprime, gprime, 0, hprime, nprime, aprime, nprime, bprime, 0, scale, &gens.H, &tmp);

        // PAPER LINES 25-27
        hasher << this->L[round];
        hasher << this->R[round];

        w[round] = hasher.GetHash();

        if (w[round] == 0)
            goto try_again;

        Scalar winv = w[round].Invert();

        // PAPER LINES 29-31
        if (nprime > 1)
        {
            gprime = HadamardFold(gprime, NULL, winv, w[round]);
            hprime = HadamardFold(hprime, scale, w[round], winv);
        }

        // PAPER LINES 33-34
        aprime = VectorAdd(VectorScalar(VectorSlice(aprime, 0, nprime), w[round]),
                           VectorScalar(VectorSlice(aprime, nprime, aprime.size()), winv));

        bprime = VectorAdd(VectorScalar(VectorSlice(bprime, 0, nprime), winv),
                           VectorScalar(VectorSlice(bprime, nprime, bprime.size()), w[round]));

        scale = NULL;

        round += 1;
    }

    this->a = aprime[0];
    this->b = bprime[0];
}

struct proof_data_t
{
    Scalar x, y, z, x_ip;
    std::vector<Scalar> w;
    std::vector<G1Point> V;
    size_t logM, inv_offset;
};

bool VerifyBulletproof(const std::vector<std::pair<int, BulletproofsRangeproof>>& proofs, std::vector<RangeproofEncodedData>& vData, const std::vector<G1Point>& nonces, const bool &fOnlyRecover, const TokenId& tokenId)
{
    bool fRecover = false;

    // auto nStart = GetTimeMicros();

    if (nonces.size() == proofs.size())
        fRecover = true;

    if (pow(2, BulletproofsRangeproof::logN) > maxN)
        throw std::runtime_error("BulletproofsRangeproof::VerifyBulletproof(): logN value is too high");

    BulletproofsRangeproof::Init();

    Generators gens = BulletproofsRangeproof::GetGenerators(tokenId);

    unsigned int N = 1 << BulletproofsRangeproof::logN;

    size_t max_length = 0;
    size_t nV = 0;

    std::vector<proof_data_t> proof_data;

    size_t inv_offset = 0, j = 0;
    std::vector<Scalar> to_invert;

    for (auto& p: proofs)
    {
        BulletproofsRangeproof proof = p.second;
        if (!(proof.V.size() >= 1 && proof.L.size() == proof.R.size() &&
              proof.L.size() > 0))
            return false;

        max_length = std::max(max_length, proof.L.size());
        nV += proof.V.size();

        proof_data.resize(proof_data.size() + 1);
        proof_data_t &pd = proof_data.back();
        pd.V = proof.V;

        CHashWriter hasher(0,0);

        hasher << pd.V[0];

        for (unsigned int j = 1; j < pd.V.size(); j++)
            hasher << pd.V[j];

        hasher << proof.A;
        hasher << proof.S;

        pd.y = hasher.GetHash();

        hasher << pd.y;

        pd.z = hasher.GetHash();

        hasher << pd.z;
        hasher << proof.T1;
        hasher << proof.T2;

        pd.x = hasher.GetHash();

        hasher << pd.x;
        hasher << proof.taux;
        hasher << proof.mu;
        hasher << proof.t;

        pd.x_ip = hasher.GetHash();

        size_t M;
        for (pd.logM = 0; (M = 1<<pd.logM) <= maxM && M < pd.V.size(); ++pd.logM);

        const size_t rounds = pd.logM+BulletproofsRangeproof::logN;

        pd.w.resize(rounds);
        for (size_t i = 0; i < rounds; ++i)
        {
            hasher << proof.L[i];
            hasher << proof.R[i];

            pd.w[i] = hasher.GetHash();
        }

        pd.inv_offset = inv_offset;
        for (size_t i = 0; i < rounds; ++i)
        {
            to_invert.push_back(pd.w[i]);
        }
        to_invert.push_back(pd.y);
        inv_offset += rounds + 1;

        if (fRecover)
        {
            Scalar alpha = nonces[j].GetHashWithSalt(1);
            Scalar rho = nonces[j].GetHashWithSalt(2);
            Scalar tau1 = nonces[j].GetHashWithSalt(3);
            Scalar tau2 = nonces[j].GetHashWithSalt(4);
            Scalar gamma = nonces[j].GetHashWithSalt(100);
            Scalar excess = (proof.mu - rho*pd.x) - alpha;

    // create 0xFFFFFFFFFFFFFFFF mask
    Scalar int64_max(INT64_MAX);
    Scalar one(1);
    Scalar mask = (int64_max << 1) + one;
            Scalar amount = excess & mask;

            RangeproofEncodedData data;
            data.index = p.first;
            data.amount = amount.GetUint64();

            std::vector<unsigned char> vMsg = (excess>>8*8).GetVch();
            std::vector<unsigned char> vMsgTrimmed(0);

            bool fFoundNonZero = false;

            for (auto&it: vMsg)
            {
                if (it != '\0')
                    fFoundNonZero = true;
                if (fFoundNonZero)
                    vMsgTrimmed.push_back(it);
            }

            data.gamma = gamma;
            data.valid = true;

            Scalar excessMsg2 = ((proof.taux - (tau2*pd.x*pd.x) - (pd.z*pd.z*gamma)) * pd.x.Invert()) - tau1;

            std::vector<unsigned char> vMsg2 = excessMsg2.GetVch();
            std::vector<unsigned char> vMsg2Trimmed(0);

            fFoundNonZero = false;

            for (auto&it: vMsg2)
            {
                if (it != '\0')
                    fFoundNonZero = true;
                if (fFoundNonZero)
                    vMsg2Trimmed.push_back(it);
            }

            data.message = std::string(vMsgTrimmed.begin(), vMsgTrimmed.end()) + std::string(vMsg2Trimmed.begin(), vMsg2Trimmed.end());

            {
                printf("v gamma=%s\n", gamma.GetString().c_str());
                printf("v amount=%s\n", amount.GetString().c_str());
                G1Point gammaElement = gens.G*gamma;
                G1Point valueElement = gens.H*amount;
                bool fIsMine = ((gammaElement + valueElement) == pd.V[0]);
                if (fIsMine)
                    vData.push_back(data);
            }

            j++;
        }
    }

    if (fOnlyRecover)
        return true;

    size_t maxMN = 1 << max_length;

    std::vector<Scalar> inverses(to_invert.size());

    inverses = VectorInvert(to_invert);

    Scalar z1 = 0;
    Scalar z3 = 0;

    std::vector<Scalar> z4(maxMN, 0);
    std::vector<Scalar> z5(maxMN, 0);

    Scalar y0 = 0;
    Scalar y1 = 0;

    Scalar tmp;

    int proof_data_index = 0;

    std::vector<MultiexpData> multiexpdata;

    multiexpdata.reserve(nV + (2 * (10/*logM*/ + BulletproofsRangeproof::logN) + 4) * proofs.size() + 2 * maxMN);
    multiexpdata.resize(2 * maxMN);

    for (auto& p: proofs)
    {
        BulletproofsRangeproof proof = p.second;

        const proof_data_t &pd = proof_data[proof_data_index++];

        if (proof.L.size() != BulletproofsRangeproof::logN+pd.logM)
            return false;

        const size_t M = 1 << pd.logM;
        const size_t MN = M*N;

        Scalar weight_y = Scalar::Rand();
        Scalar weight_z = Scalar::Rand();

        y0 = y0 - (proof.taux * weight_y);

        std::vector<Scalar> zpow = VectorPowers(pd.z, M+3);

        Scalar k;
        Scalar ip1y = VectorPowerSum(pd.y, MN);

        k = (zpow[2]*ip1y).Negate();

        for (size_t j = 1; j <= M; ++j)
        {
            k = k - (zpow[j+2] * *BulletproofsRangeproof::ip12);
        }

        tmp = k + (pd.z*ip1y);

        tmp = (proof.t - tmp);

        y1 = y1 + (tmp * weight_y);

        for (size_t j = 0; j < pd.V.size(); j++)
        {
            tmp = zpow[j+2] * weight_y;
            multiexpdata.push_back({pd.V[j], tmp});
        }

        tmp = pd.x * weight_y;

        multiexpdata.push_back({proof.T1, tmp});

        tmp = pd.x * pd.x * weight_y;

        multiexpdata.push_back({proof.T2, tmp});
        multiexpdata.push_back({proof.A, weight_z});

        tmp = pd.x * weight_z;

        multiexpdata.push_back({proof.S, tmp});

        const size_t rounds = pd.logM+BulletproofsRangeproof::logN;

        Scalar yinvpow = 1;
        Scalar ypow = 1;

        const Scalar *winv = &inverses[pd.inv_offset];
        const Scalar yinv = inverses[pd.inv_offset + rounds];

        std::vector<Scalar> w_cache(1<<rounds, 1);
        w_cache[0] = winv[0];
        w_cache[1] = pd.w[0];

        for (size_t j = 1; j < rounds; ++j)
        {
            const size_t sl = 1<<(j+1);
            for (size_t s = sl; s-- > 0; --s)
            {
                w_cache[s] = w_cache[s/2] * pd.w[j];
                w_cache[s-1] = w_cache[s/2] * winv[j];
            }
        }

        for (size_t i = 0; i < MN; ++i)
        {
            Scalar g_scalar = proof.a;
            Scalar h_scalar;

            if (i == 0)
                h_scalar = proof.b;
            else
                h_scalar = proof.b * yinvpow;

            g_scalar = g_scalar * w_cache[i];
            h_scalar = h_scalar * w_cache[(~i) & (MN-1)];

            g_scalar = g_scalar + pd.z;

            tmp = zpow[2+i/N] * BulletproofsRangeproof::twoN[i%N];

            if (i == 0)
            {
                tmp = tmp + pd.z;
                h_scalar = h_scalar - tmp;
            }
            else
            {
                tmp = tmp + (pd.z*ypow);
                h_scalar = h_scalar - (tmp * yinvpow);
            }

            z4[i] = z4[i] - (g_scalar * weight_z);
            z5[i] = z5[i] - (h_scalar * weight_z);

            if (i == 0)
            {
                yinvpow = yinv;
                ypow = pd.y;
            }
            else if (i != MN-1)
            {
                yinvpow = yinvpow * yinv;
                ypow = ypow * pd.y;
            }
        }

        z1 = z1 + (proof.mu * weight_z);

        for (size_t i = 0; i < rounds; ++i)
        {
            tmp = pd.w[i] * pd.w[i] * weight_z;

            multiexpdata.push_back({proof.L[i], tmp});

            tmp = winv[i] * winv[i] * weight_z;

            multiexpdata.push_back({proof.R[i], tmp});
        }

        tmp = proof.t - (proof.a*proof.b);
        tmp = tmp * pd.x_ip;
        z3 = z3 + (tmp * weight_z);
    }

    tmp = y0 - z1;

    multiexpdata.push_back({gens.G, tmp});

    tmp = z3 - y1;

    multiexpdata.push_back({gens.H, tmp});

    for (size_t i = 0; i < maxMN; ++i)
    {
        multiexpdata[i * 2] = {gens.Gi[i], z4[i]};
        multiexpdata[i * 2 + 1] = {gens.Hi[i], z5[i]};
    }

    G1Point mexp = MultiExp(multiexpdata);

    //std::cout << strprintf("%s: took %.3fms\n", __func__, (GetTimeMicros()-nStart)/1000);

    return mexp.IsUnity();
}
