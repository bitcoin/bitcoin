// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/eip-2333/derive_sk.h>

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
}

// Inputs
// - seed, the source entropy for the entire tree, a octet string >= 256 bits in length
// Outputs
// - SK, the secret key of master node within the tree, a big endian encoded integer
std::array<uint8_t,32> BLS12_381_KeyGen::derive_master_SK(std::vector<uint8_t>& seed, const std::vector<uint8_t>& SK)
{
    // 0. SK = HKDF_mod_r(seed)
    // 1. return SK
}

// HKDF-Extract is as defined in RFC5869, instantiated with SHA256
void BLS12_381_KeyGen::HKDF_Extract()
{
}

// HKDF-Expand is as defined in RFC5869, instantiated with SHA256
void BLS12_381_KeyGen::HKDF_Expand()
{
}

// I2OSP is as defined in RFC3447 (Big endian decoding)
void BLS12_381_KeyGen::I2OSP()
{
}

// OS2IP is as defined in RFC3447 (Big endian encoding)
void BLS12_381_KeyGen::OS2IP()
{
}

// flip_bits is a function that returns the bitwise negation of its input
void BLS12_381_KeyGen::flip_bits()
{
}

// a function that takes in an octet string and splits it into K-byte chunks which are returned as an array
std::vector<std::array<uint8_t, K>> BLS12_381_KeyGen::bytes_split(std::vector<uint8_t> octet_string, uint32_t chunk_size = K)
{
}

// Inputs
// - IKM, a secret octet string
// - salt, an octet string
//
// Outputs
// - lamport_SK, an array of 255 32-octet strings
std::array<uint8_t,255*32> BLS12_381_KeyGen::IKM_to_lamport_SK(const std::vector<uint8_t>& IKM, const std::vector<uint8_t>& salt)
{
    // 0. PRK = HKDF-Extract(salt, IKM)
    // 1. OKM = HKDF-Expand(PRK, "" , L)
    // 2. lamport_SK = bytes_split(OKM, K)
    // 3. return lamport_SK
}

// Inputs
// - parent_SK, the BLS Secret Key of the parent node
// - index, the index of the desired child node, an integer 0 <= index < 2^32
//
// Outputs
// - lamport_PK, the compressed lamport PK, a 32 octet string Inputs
std::array<uint8_t, 32> BLS12_381_KeyGen::parent_SK_to_lamport_PK(const std::vector<uint8_t>& parent_SK, const uint32_t& index)
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
}

// Inputs
// - IKM, a secret octet string >= 256 bits in length
// - key_info, an optional octet string (default="", the empty string)
// Outputs
// - SK, the corresponding secret key, an integer 0 <= SK < r.
std::vector<uint8_t> BLS12_381_KeyGen::HKDF_mod_r(const std::vector<uint8_t> IKM, const std::vector<uint8_t>& key_info = {})
{
    // 1. salt = "BLS-SIG-KEYGEN-SALT-"
    // 2. SK = 0
    // 3. while SK == 0:
    // 4.     salt = H(salt)
    // 5.     PRK = HKDF-Extract(salt, IKM || I2OSP(0, 1))
    // 6.     OKM = HKDF-Expand(PRK, key_info || I2OSP(L, 2), L)
    // 7.     SK = OS2IP(OKM) mod r
    // 8. return SK
}
