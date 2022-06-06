# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

add_library(bitcoin_crypto STATIC EXCLUDE_FROM_ALL)
target_sources(bitcoin_crypto
  PRIVATE
    aes.cpp
    chacha_poly_aead.cpp
    chacha20.cpp
    hkdf_sha256_32.cpp
    hmac_sha256.cpp
    hmac_sha512.cpp
    poly1305.cpp
    muhash.cpp
    ripemd160.cpp
    sha1.cpp
    sha256.cpp
    sha3.cpp
    sha512.cpp
    siphash.cpp
)
target_include_directories(bitcoin_crypto
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/src
)
