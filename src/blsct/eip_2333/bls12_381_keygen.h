// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef NAVCOIN_BLSCT_EIP_2333_BLS12_381_KEYGEN_H
#define NAVCOIN_BLSCT_EIP_2333_BLS12_381_KEYGEN_H

#include <bls/bls384_256.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <crypto/hmac_sha256.h>
#include <crypto/sha256.h>

#include <array>

class BLS12_381_KeyGen
{
public:
    static MclScalar derive_master_SK(const std::vector<uint8_t>& seed);
    static MclScalar derive_child_SK(const MclScalar& parent_SK, const uint32_t& index);

#ifndef BOOST_UNIT_TEST
private:
#endif
    inline static const uint32_t DigestSize = CSHA256::OUTPUT_SIZE;
    inline static const uint32_t NumLamportChunks = 255;

    using LamportChunks = std::array<std::array<uint8_t,DigestSize>,NumLamportChunks>;

    static std::array<uint8_t,DigestSize> HKDF_Extract(const std::vector<uint8_t>& salt, const std::vector<uint8_t>& IKM);

    template <size_t L>
    static std::array<uint8_t,L> HKDF_Expand(const std::array<uint8_t,DigestSize>& PRK, const std::vector<uint8_t>& info);

    static std::vector<uint8_t> I2OSP(const MclScalar& x, const size_t& xLen);
    static MclScalar OS2IP(const std::array<uint8_t,48ul>& X);
    static std::vector<uint8_t> flip_bits(const std::vector<uint8_t>& vec);
    static LamportChunks bytes_split(const std::array<uint8_t,8160>& octet_string);
    static MclScalar HKDF_mod_r(const std::vector<uint8_t>& IKM);
    static LamportChunks IKM_to_lamport_SK(const std::vector<uint8_t>& IKM, const std::vector<uint8_t>& salt);
    static std::array<uint8_t,DigestSize> parent_SK_to_lamport_PK(const MclScalar& parent_SK, const uint32_t& index);
};

#endif  // NAVCOIN_BLSCT_EIP_2333_BLS12_381_KEYGEN_H

