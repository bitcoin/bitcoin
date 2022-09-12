// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Diverse arithmetic operations in the bls curve
// inspired by https://github.com/b-g-goodell/research-lab/blob/master/source-code/StringCT-java/src/how/monero/hodl/bulletproof/Bulletproof.java
// and https://github.com/monero-project/monero/blob/master/src/ringct/bulletproofs.cc

#include <boost/algorithm/string.hpp>

#include <blsct/b2ulletproofs.h>
#include <tinyformat.h>
#include <utiltime.h>

bool BLSInitResult = bls::BLS::Init();

static std::vector<Scalar> VectorPowers(const Scalar &x, size_t n);
static std::vector<Scalar> VectorDup(const Scalar &x, size_t n);
static Scalar InnerProduct(const std::vector<Scalar> &a, const std::vector<Scalar> &b);

Scalar BulletproofsRangeproof::one;
Scalar BulletproofsRangeproof::two;

std::vector<bls::G1Element> BulletproofsRangeproof::Hi, BulletproofsRangeproof::Gi;
std::vector<Scalar> BulletproofsRangeproof::oneN;
std::vector<Scalar> BulletproofsRangeproof::twoN;
Scalar BulletproofsRangeproof::ip12;

boost::mutex BulletproofsRangeproof::init_mutex;

bls::G1Element BulletproofsRangeproof::G;
std::map<TokenId, bls::G1Element> BulletproofsRangeproof::H;

// Calculate base point
static bls::G1Element GetBaseG1Element(const bls::G1Element &base, size_t idx, std::string tokId = "", uint64_t tokNftId = -1)
{
    static const std::string salt("bulletproof");
    std::vector<uint8_t> data =  base.Serialize();
    std::string toHash = HexStr(data) + salt + std::to_string(idx) + tokId + (tokNftId != -1 ? "nft"+ std::to_string(tokNftId) : "");

    CHashWriter ss(SER_GETHASH, 0);
    ss << toHash;
    uint256 hash = ss.GetHash();

    uint8_t dest[1];
    dest[0] = 0x0;

    std::vector<unsigned char> vMcl(48);

    if (tokId != "")
    {
        Fp p;
        auto vHash = std::vector<unsigned char>(hash.begin(), hash.end());
        p.setLittleEndianMod(&vHash[0], 32);
        G1 g;
        mapToG1(g, p);
        g.serialize(&vMcl[0], 48);
    }

    bls::G1Element e = tokId == "" ? 
        bls::G1Element::FromMessage(std::vector<unsigned char>(hash.begin(), hash.end()), dest, 1) : 
        bls::G1Element::FromByteVector(vMcl);

    CHECK_AND_ASSERT_THROW_MES(e != bls::G1Element::Infinity(), "Exponent is point at infinity");

    return e;
}

// Initialize bases and constants
bool BulletproofsRangeproof::Init()
{
    boost::lock_guard<boost::mutex> lock(BulletproofsRangeproof::init_mutex);

    static bool fInit = false;

    if (fInit)
        return true;

    initPairing(mcl::BLS12_381);

    Fp::setETHserialization(true);
    Fr::setETHserialization(true);

    BulletproofsRangeproof::one = 1;
    BulletproofsRangeproof::two = 2;

    BulletproofsRangeproof::G = bls::G1Element::Generator();
    BulletproofsRangeproof::H[TokenId()] = GetBaseG1Element(BulletproofsRangeproof::G, 0);

    BulletproofsRangeproof::Hi.resize(maxMN);
    BulletproofsRangeproof::Gi.resize(maxMN);

    for (size_t i = 0; i < maxMN; ++i)
    {
        BulletproofsRangeproof::Hi[i] = GetBaseG1Element(BulletproofsRangeproof::H[TokenId()], i * 2 + 1);
        BulletproofsRangeproof::Gi[i] = GetBaseG1Element(BulletproofsRangeproof::H[TokenId()], i * 2 + 2);
    }

    BulletproofsRangeproof::oneN = VectorDup(BulletproofsRangeproof::one, maxN);
    BulletproofsRangeproof::twoN = VectorPowers(BulletproofsRangeproof::two, maxN);
    BulletproofsRangeproof::ip12 = InnerProduct(BulletproofsRangeproof::oneN, BulletproofsRangeproof::twoN);

    fInit = true;

    return true;
}

Generators BulletproofsRangeproof::GetGenerators(const TokenId& tokenId)
{
    // H maps tokenId to G1Point

    // if G1Point for the tokenId has been creaded, use it
    if (BulletproofsRangeproof::H.count(tokenId)) {
        return {
            BulletproofsRangeproof::G, 
            BulletproofsRangeproof::H[tokenId], 
            BulletproofsRangeproof::Gi, 
            BulletproofsRangeproof::Hi
        };
    }

    // Otherwise create it
    BulletproofsRangeproof::H[tokenId] = GetBaseG1Element(
        BulletproofsRangeproof::G, 0, tokenId.token.ToString(), tokenId.subid);

    return {
        BulletproofsRangeproof::G, 
        BulletproofsRangeproof::H[tokenId], 
        BulletproofsRangeproof::Gi, 
        BulletproofsRangeproof::Hi
    };
}

class MultiexpData {
public:
    G1Point base;
    Scalar exp;

    MultiexpData() {}
    MultiexpData(G1Point base, Scalar exp) : base(base), exp(exp) {}
};

G1Point CrossVectorExponent(
    size_t size,  // must be: size <= max_mn
    const std::vector<bls::G1Element> &A, size_t A_beg, // must be: size + Ao <= A.size()
    const std::vector<bls::G1Element> &B, size_t B_beg, // must be: size + Bo <= B.size()
    const std::vector<Scalar> &a, size_t a_beg, // must be: size + ao <= a.size() 
    const std::vector<Scalar> &b, size_t b_beg, // must be: size + bo <= b.size() 
    const std::vector<Scalar> *scale, // must be: size == scale->size() / 2
    const bls::G1Element *extra_point, // both extra_point and extra_scalar must be present or none are specified
    const Scalar *extra_scalar
) {
    std::vector<MultiexpData> multiexp_data;
    multiexp_data.resize(size * 2 + (!!extra_point));

    for (size_t i = 0; i < size; ++i) {
        multiexp_data[i*2].exp = a[a_beg + i];
        multiexp_data[i*2].base = A[A_beg + i];

        multiexp_data[i*2+1].exp = b[b_beg + i];
        if (scale)
            // if scale is specified, multiply exp by scale[B_beg + i] 
            multiexp_data[i*2+1].exp =  multiexp_data[i*2+1].exp * (*scale)[B_beg + i];
        multiexp_data[i*2+1].base = B[B_beg + i];
    }

    if (extra_point) {
        multiexp_data.back().exp = *extra_scalar;  // append to the end
        multiexp_data.back().base = *extra_point;  // append to the end
    }

    // returns G1Point
    // return MultiExp(multiexp_data);
    G1Point xs // size = multiexp_data.size()
    Scalars ys // size = multiexp_data.size()]
    G1Point z;

    // convert exp and base to corresponding byte strings
    for (size_t i = 0; i < multiexp_data.size(); ++i) {
        std::vector<uint8_t> base = multiexp_data[i].base.GetVch();
        std::vector<uint8_t> exp = multiexp_data[i].exp.GetVch();

        xs[i].deserialize(&base[0], base.size());
        ys[i].deserialize(&exp[0], exp.size());
    }

    // multiply and aggregate
    // z = (xs * ys).Sum()
    G1::mulVec(z, xs, ys, multiexp_data.size());

    // convert G1 to G1Element
    std::vector<uint8_t> res(48);
    z.serialize(&res[0], 48);
    G1Point result = G1Point::SetVch(res);

    return result;
}

void BulletproofsRangeproof::Prove(
    std::vector<Scalar> v, 
    bls::G1Element nonce, 
    const std::vector<uint8_t>& message, 
    const TokenId& tokenId, 
    const std::vector<Scalar>& useGammas
)
{
    if (pow(2, BulletproofsRangeproof::logN) > maxN /* = 64 */)
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
            gamma[i] = HashG1Element(nonce, 100+i);
        }
    }

    // This hash is updated for Fiat-Shamir throughout the proof
    CHashWriter hasher(0,0);

    for (unsigned int j = 0; j < v.size(); j++)
    {
        bls::G1Element gammaElement = gens.G * gamma[j].bn;
        bls::G1Element valueElement = gens.H * v[j].bn;
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
    sM = sM << 8 * 8;

    alpha = HashG1Element(nonce, 1);
    alpha = alpha + (v[0] | sM);

    this->A = VectorCommitment(aL, aR);
    {
        bls::G1Element alphaElement = gens.G * alpha.bn;
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
    rho = HashG1Element(nonce, 2);

    this->S = VectorCommitment(sL, sR);
    {
        bls::G1Element rhoElement = gens.G * rho.bn;
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
    Scalar tau1 = HashG1Element(nonce, 3);
    Scalar tau2 = HashG1Element(nonce, 4);
    std::vector<unsigned char> secondMessage = message.size() > 23 ? 
        std::vector<unsigned char>(message.begin()+23, message.end()) : 
        std::vector<unsigned char>();
    Scalar sM2 = secondMessage;
    tau1 = tau1 + sM2;

    {
        bls::G1Element t1Element = gens.H * t1.bn;
        bls::G1Element t2Element = gens.H * t2.bn;
        bls::G1Element tau1Element = gens.G * tau1.bn;
        bls::G1Element tau2Element = gens.G * tau2.bn;

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

    // adding z^2 * gamma?
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

    std::vector<bls::G1Element> gprime(nprime);
    std::vector<bls::G1Element> hprime(nprime);
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
        tmp = cL * x_ip;  // what is this??
        this->L[round] = CrossVectorExponent(nprime, gprime, nprime, hprime, 0, aprime, 0, bprime, nprime, scale, &gens.H, &tmp);
        tmp = cR * x_ip;  // what is this??
        this->R[round] = CrossVectorExponent(nprime, gprime, 0, hprime, nprime, aprime, nprime, bprime, 0, scale, &gens.H, &tmp);

        // PAPER LINES 25-27
        hasher << this->L[round];
        hasher << this->R[round];

        w[round] = hasher.GetHash();  // w is x in paper

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
    std::vector<bls::G1Element> V;
    size_t logM, inv_offset;
};

bool VerifyBulletproof(const std::vector<std::pair<int, BulletproofsRangeproof>>& proofs, std::vector<RangeproofEncodedData>& vData, const std::vector<bls::G1Element>& nonces, const bool &fOnlyRecover, const TokenId& tokenId)
{
    bool fRecover = false;

    auto nStart = GetTimeMicros();

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
            Scalar alpha = HashG1Element(nonces[j], 1);
            Scalar rho = HashG1Element(nonces[j], 2);
            Scalar tau1 = HashG1Element(nonces[j], 3);
            Scalar tau2 = HashG1Element(nonces[j], 4);
            Scalar gamma = HashG1Element(nonces[j], 100);
            Scalar excess = (proof.mu - rho*pd.x) - alpha;
            Scalar amount = (excess & Scalar(0xFFFFFFFFFFFFFFFF));

            RangeproofEncodedData data;
            data.index = p.first;
            data.amount = amount.GetInt64();

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
                bls::G1Element gammaElement = gens.G*gamma.bn;
                bls::G1Element valueElement = gens.H*amount.bn;
                bool fIsMine = ((gammaElement + valueElement) == pd.V[0]);

                if (fIsMine)
                    vData.push_back(data);
            }

            j++;
        }
    }

    if (fOnlyRecover)
        return true;

    size_t maxMN = 1u << max_length;

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
            k = k - (zpow[j+2]*BulletproofsRangeproof::ip12);
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

    bls::G1Element mexp = MultiExp(multiexpdata);

    //std::cout << strprintf("%s: took %.3fms\n", __func__, (GetTimeMicros()-nStart)/1000);

    return mexp == bls::G1Element::Infinity();
}
