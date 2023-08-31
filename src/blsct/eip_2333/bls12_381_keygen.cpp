// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/eip_2333/bls12_381_keygen.h>
#include <cmath>
#include <crypto/sha256.h>
#include <tinyformat.h>

std::array<uint8_t,BLS12_381_KeyGen::DigestSize> BLS12_381_KeyGen::HKDF_Extract(const std::vector<uint8_t>& salt, const std::vector<uint8_t>& IKM)
{
    auto hmac_sha256 = CHMAC_SHA256(&salt[0], salt.size());
    hmac_sha256.Write(&IKM[0], IKM.size());

    std::array<uint8_t,CHMAC_SHA256::OUTPUT_SIZE> hash;
    hmac_sha256.Finalize(&hash[0]);

    return hash;
}

template <size_t L>
std::array<uint8_t,L> BLS12_381_KeyGen::HKDF_Expand(const std::array<uint8_t,BLS12_381_KeyGen::DigestSize>& PRK, const std::vector<uint8_t>& info)
{
    std::array<uint8_t,DigestSize> prev;
    std::array<uint8_t,L> output;
    uint8_t n;

    const size_t num_loops = std::ceil(static_cast<float>(L) / DigestSize);
    auto output_it = output.begin();

    for (size_t i=1; i<=num_loops; ++i) {
        auto hmac_sha256 = CHMAC_SHA256(&PRK[0], PRK.size());
        if (i > 1) {
            hmac_sha256.Write(&prev[0], prev.size());
        }
        if (info.size() > 0){
            hmac_sha256.Write(&info[0], info.size());
        }
        n = static_cast<uint8_t>(i);
        hmac_sha256.Write(&n, 1);
        hmac_sha256.Finalize(&prev[0]);

        // if less than DigestSize bytes are remaining to copy, copy only the remaining bytes. otherwiese copy DigestSize bytes.
        size_t bytes2copy = i * DigestSize > L ? L - (i - 1) * DigestSize : DigestSize;
        std::copy_n(prev.cbegin(), bytes2copy, output_it);

        std::advance(output_it, DigestSize);
    }
    return output;
}
template std::array<uint8_t,8160ul> BLS12_381_KeyGen::HKDF_Expand<8160ul>(const std::array<uint8_t,BLS12_381_KeyGen::DigestSize>& PRK, const std::vector<uint8_t>& info);
template std::array<uint8_t,48ul> BLS12_381_KeyGen::HKDF_Expand<48ul>(const std::array<uint8_t,BLS12_381_KeyGen::DigestSize>& PRK, const std::vector<uint8_t>& info);

std::vector<uint8_t> BLS12_381_KeyGen::I2OSP(const MclScalar& x, const size_t& xLen)
{
    if (xLen == 0) {
        throw std::runtime_error(std::string(__func__) + ": Output size cannot be zero");
    }
    // TODO dropping preceding zeros after generating 32-byte octet string. check if this is acceptable
    auto ret = x.GetVch(true);
    if (ret.size() == xLen) {
        return ret;
    } else if (ret.size() > xLen) {
        throw std::runtime_error(std::string(__func__) + strprintf(": Input too large. Expected octet length <= %ld, but got %ld", xLen, ret.size()));
    } else {
        // prepend 0 padding to make the octet string size xLen
        auto padded_ret = std::vector<uint8_t>(xLen - ret.size());
        padded_ret.insert(padded_ret.end(), ret.cbegin(), ret.cend());
        return padded_ret;
    }
}

MclScalar BLS12_381_KeyGen::OS2IP(const std::array<uint8_t,48ul>& X)
{
    std::vector<uint8_t> vec(X.cbegin(), X.cend());
    MclScalar s(vec);
    return s;
}

std::vector<uint8_t> BLS12_381_KeyGen::flip_bits(const std::vector<uint8_t>& vec)
{
    std::vector<uint8_t> ret(vec.size());
    std::transform(vec.cbegin(), vec.cend(), ret.begin(),
        [](uint8_t x) { return static_cast<uint8_t>(~x); });
    return ret;
}

BLS12_381_KeyGen::LamportChunks BLS12_381_KeyGen::bytes_split(const std::array<uint8_t,8160>& octet_string)
{
    std::array<std::array<uint8_t,DigestSize>,NumLamportChunks> ret;
    size_t i = 0;

    for (auto it = octet_string.cbegin(); it != octet_string.cend(); std::advance(it, DigestSize), ++i) {
        std::copy(it, it + DigestSize, ret[i].begin());
    }
    return ret;
}

MclScalar BLS12_381_KeyGen::HKDF_mod_r(const std::vector<uint8_t>& IKM)
{
    const auto L = 48;  // `ceil((3 * ceil(log2(r))) / 16)`, where `r` is the order of the BLS 12-381 curve
    const std::string salt_str = "BLS-SIG-KEYGEN-SALT-";
    const MclScalar zero(0);

    std::vector<uint8_t> salt(salt_str.cbegin(), salt_str.cend());
    CSHA256 sha256;
    std::array<uint8_t, DigestSize> hash;
    MclScalar SK;

    while (true) {
        sha256.Write(&salt[0], salt.size());
        sha256.Finalize(&hash[0]);
        salt = std::vector<uint8_t>(hash.cbegin(), hash.cend());

        std::vector<uint8_t> IKM_zero(IKM.cbegin(), IKM.cend());
        auto i2osp_zero = I2OSP(0, 1);
        IKM_zero.insert(IKM_zero.end(), i2osp_zero.cbegin(), i2osp_zero.cend());
        auto PRK = HKDF_Extract(salt, IKM_zero);
        auto info = I2OSP(L, 2);
        auto OKM = HKDF_Expand<L>(PRK, info);

        SK = OS2IP(OKM);
        if (!SK.IsZero()) return SK;
        sha256.Reset();
    }
}

BLS12_381_KeyGen::LamportChunks
BLS12_381_KeyGen::IKM_to_lamport_SK(const std::vector<uint8_t>& IKM, const std::vector<uint8_t>& salt)
{
    auto PRK = HKDF_Extract(salt, IKM);

    const auto L = NumLamportChunks * DigestSize;
    std::vector<uint8_t> info(0);
    auto OKM = HKDF_Expand<L>(PRK, info);

    auto lamport_SK = bytes_split(OKM);

    return lamport_SK;
}

std::array<uint8_t,BLS12_381_KeyGen::DigestSize>
BLS12_381_KeyGen::parent_SK_to_lamport_PK(const MclScalar& parent_SK, const uint32_t& index)
{
    auto salt = I2OSP(index, 4);
    auto IKM = I2OSP(parent_SK, 32);

    auto lamport_0 = IKM_to_lamport_SK(IKM, salt);
    auto not_IKM = flip_bits(IKM);
    auto lamport_1 = IKM_to_lamport_SK(not_IKM, salt);

    std::vector<uint8_t> lamport_PK;
    CSHA256 sha256;
    std::array<uint8_t,DigestSize> hash;

    for (auto lamport_chunks : std::vector<LamportChunks> {lamport_0, lamport_1}) {
        for (size_t i=1; i<=255; ++i) {
            auto chunk = lamport_chunks[i-1];
            sha256.Write(&chunk[0], DigestSize);
            sha256.Finalize(&hash[0]);
            sha256.Reset();
            lamport_PK.insert(lamport_PK.end(), hash.cbegin(), hash.cend());
        }
    }

    sha256.Write(&lamport_PK[0], lamport_PK.size());
    sha256.Finalize(&hash[0]);
    return hash;
}

MclScalar BLS12_381_KeyGen::derive_master_SK(const std::vector<uint8_t>& seed)
{
    if (seed.size() < 32) {
        throw std::runtime_error(std::string(__func__) + ": Seed must be an 256-bit octet string or larger");
    }
    auto SK = HKDF_mod_r(seed);
    return SK;
}

MclScalar BLS12_381_KeyGen::derive_child_SK(const MclScalar& parent_SK, const uint32_t& index)
{
    auto comp_PK = parent_SK_to_lamport_PK(parent_SK, index);
    auto SK = HKDF_mod_r(std::vector<uint8_t>(comp_PK.cbegin(), comp_PK.cend()));
    return SK;
}
