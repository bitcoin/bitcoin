// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/eip_2333/bls12_381_keygen.h>
#include <tinyformat.h>

// Inputs
// - parent_SK, the secret key of the parent node, a big endian encoded integer
// - index, the index of the desired child node, an integer 0 <= index < 2^32
// Outputs
// - child_SK, the secret key of the child node, a big endian encoded integer
std::vector<uint8_t> BLS12_381_KeyGen::derive_child_SK(const std::vector<uint8_t>& parent_SK, const uint32_t& index)
{
    // 0. compressed_lamport_PK = parent_SK_to_lamport_PK(parent_SK, index)
    // 1. SK = HKDF_mod_r(compressed_lamport_PK)
    // 2. return SK
    std::vector<uint8_t> ret;
    return ret;
}

// Inputs
// - seed, the source entropy for the entire tree, a octet string >= 256 bits in length
// Outputs
// - SK, the secret key of master node within the tree, a big endian encoded integer
std::array<uint8_t,BLS12_381_KeyGen::K> BLS12_381_KeyGen::derive_master_SK(const std::vector<uint8_t>& seed, const std::vector<uint8_t>& SK)
{
    // 0. SK = HKDF_mod_r(seed)
    // 1. return SK
    std::array<uint8_t,BLS12_381_KeyGen::K> ret;
    return ret;
}

std::array<uint8_t,CHMAC_SHA256::OUTPUT_SIZE> BLS12_381_KeyGen::HKDF_Extract(const std::vector<uint8_t>& IKM, const MclScalar& salt)
{
    auto salt_vec = salt.GetVch(true);
    auto hmac_sha256 = CHMAC_SHA256(&salt_vec[0], salt_vec.size());
    hmac_sha256.Write(&IKM[0], IKM.size());

    std::array<uint8_t,CHMAC_SHA256::OUTPUT_SIZE> hash;
    hmac_sha256.Finalize(&hash[0]);

    return hash;
}

std::array<uint8_t,BLS12_381_KeyGen::L> BLS12_381_KeyGen::HKDF_Expand(const std::array<uint8_t,BLS12_381_KeyGen::K>& PRK, const std::vector<uint8_t>& info)
{
    std::array<uint8_t,K> prev;
    std::array<uint8_t,L> output;

    auto output_it = output.begin();
    for (uint8_t i=1; i != 0; ++i) {  // range of i is [0, 255]
        auto hmac_sha256 = CHMAC_SHA256(&PRK[0], PRK.size());
        if (i > 1) {
            hmac_sha256.Write(&prev[0], prev.size());
        }
        hmac_sha256.Write(&info[0], info.size());
        hmac_sha256.Write(static_cast<unsigned char*>(&i), 1);

        hmac_sha256.Finalize(&prev[0]);
        std::copy_n(prev.begin(), K, output_it);
        std::advance(output_it, K);
    }
    return output;
}

// Input:
// - x, nonnegative integer to be converted
// - xLen, intended length of the resulting octet string
// Output:
// - X, corresponding octet string of length xLen
std::vector<uint8_t> BLS12_381_KeyGen::I2OSP(const MclScalar& x, const size_t& xLen)
{
    if (xLen == 0) {
        throw std::runtime_error("Output size cannot be zero");
    }
    // TODO dropping preceding zeros after generating 32-byte octet string. check if this is acceptable
    auto ret = x.GetVch(true);
    if (ret.size() == xLen) {
        return ret;
    } else if (ret.size() > xLen) {
        auto s = strprintf("Input too large. Expected octet length <= %d, but got %d", xLen, ret.size());
        throw std::runtime_error(s);
    } else {
        // prepend 0 padding to make the octet string size xLen
        auto padded_ret = std::vector<uint8_t>(xLen - ret.size());
        padded_ret.insert(padded_ret.end(), ret.begin(), ret.end());
        return padded_ret;
    }
}

// Input:
// - X, octet string to be converted
// Output:
// - x, corresponding nonnegative integer
MclScalar BLS12_381_KeyGen::OS2IP(const std::vector<uint8_t>& X)
{
    MclScalar x(X);
    return x;
}

// flip_bits is a function that returns the bitwise negation of its input
MclScalar BLS12_381_KeyGen::flip_bits(const MclScalar& s)
{
    return s.Negate();
}

// a function that takes in an octet string and splits it into K-byte chunks which are returned as an array
// expects that length of octet string is L
std::array<std::array<uint8_t,BLS12_381_KeyGen::K>,BLS12_381_KeyGen::N> BLS12_381_KeyGen::bytes_split(const std::vector<uint8_t>& octet_string)
{
    if (octet_string.size() != N * K) {
        auto s = strprintf("Expected octet string to be of length %ld, but got %ld", 255 * BLS12_381_KeyGen::K, octet_string.size());
        throw std::runtime_error(s);
    }
    std::array<std::array<uint8_t,K>,N> ret;
    size_t i = 0;

    for (auto it = octet_string.begin(); it != octet_string.end(); std::advance(it, BLS12_381_KeyGen::K), ++i) {
        std::copy(it, it + K, ret[i].begin());
    }
    return ret;
}

// Inputs
// - IKM, a secret octet string
// - salt, an octet string
//
// Outputs
// - lamport_SK, an array of 255 32-octet strings
std::array<uint8_t,BLS12_381_KeyGen::L> BLS12_381_KeyGen::IKM_to_lamport_SK(const std::vector<uint8_t>& IKM, const std::vector<uint8_t>& salt)
{
    // 0. PRK = HKDF-Extract(salt, IKM)
    // 1. OKM = HKDF-Expand(PRK, "" , L)
    // 2. lamport_SK = bytes_split(OKM, K)
    // 3. return lamport_SK
    std::array<uint8_t,L> ret;
    return ret;
}

// Inputs
// - parent_SK, the BLS Secret Key of the parent node
// - index, the index of the desired child node, an integer 0 <= index < 2^32
//
// Outputs
// - lamport_PK, the compressed lamport PK, a 32 octet string Inputs
std::array<uint8_t,BLS12_381_KeyGen::K> BLS12_381_KeyGen::parent_SK_to_lamport_PK(const std::vector<uint8_t>& parent_SK, const uint32_t& index)
{
    // 0. salt = I2OSP(index, 4)
    // 1. IKM = I2OSP(parent_SK, 32)
    // 2. lamport_0 = IKM_to_lamport_SK(IKM, salt)
    // 3. not_IKM = flip_bits(IKM)
    // 4. lamport_1 = IKM_to_lamport_SK(not_IKM, salt)
    // 5. lamport_PK = ""
    // 6. for i  in 1, .., 255
    //        lamport_PK = lamport_PK | SHA256(lamport_0[i])
    // 7. for i  in 1, .., 255
    //        lamport_PK = lamport_PK | SHA256(lamport_1[i])
    // 8. compressed_lamport_PK = SHA256(lamport_PK)
    // 9. return compressed_lamport_PK

    std::array<uint8_t,K> ret;
    return ret;
}

// Inputs
// - IKM, a secret octet string >= 256 bits in length
// - key_info, an optional octet string (default="", the empty string)
// Outputs
// - SK, the corresponding secret key, an integer 0 <= SK < r.
MclScalar BLS12_381_KeyGen::HKDF_mod_r(const std::vector<uint8_t>& IKM, const std::vector<uint8_t>& key_info)
{
    // 1. salt = "BLS-SIG-KEYGEN-SALT-"
    // 2. SK = 0
    // 3. while SK == 0:
    // 4.     salt = H(salt)
    // 5.     PRK = HKDF-Extract(salt, IKM || I2OSP(0, 1))
    // 6.     OKM = HKDF-Expand(PRK, key_info || I2OSP(L, 2), L)
    // 7.     SK = OS2IP(OKM) mod r
    // 8. return SK

    MclScalar s;
    return s;
}
