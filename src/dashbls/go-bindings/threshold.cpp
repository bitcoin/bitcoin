// Copyright (c) 2021 The Dash Core developers

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vector>
#include <stdint.h>
#include "dashbls/bls.hpp"
#include "privatekey.h"
#include "elements.h"
#include "blschia.h"
#include "threshold.h"
#include "utils.hpp"
#include "error.h"

std::vector<bls::Bytes> toVectorHashes(void** elems, const size_t len) {
    std::vector<bls::Bytes> vec;
    vec.reserve(len);
    for (int i = 0 ; i < len; ++i) {
        vec.push_back(
            bls::Bytes((uint8_t*)elems[i], HashSize)
        );
    }
    return vec;
}

CPrivateKey CThresholdPrivateKeyShare(void** sks, const size_t sksLen, const void* hash, bool* didErr) {
    bls::PrivateKey* sk = nullptr;
    try {
        sk = new bls::PrivateKey(
            bls::Threshold::PrivateKeyShare(
                toBLSVector<bls::PrivateKey>(sks, sksLen),
                bls::Bytes((uint8_t*)hash, HashSize)
            )
        );
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return sk;
}

CPrivateKey CThresholdPrivateKeyRecover(void** sks,
                                        const size_t sksLen,
                                        void** hashes,
                                        const size_t hashesLen,
                                        bool* didErr) {
    bls::PrivateKey* sk = nullptr;
    std::vector<bls::Bytes> pop = toVectorHashes(hashes, hashesLen);
    try {
        sk = new bls::PrivateKey(
            bls::Threshold::PrivateKeyRecover(
                toBLSVector<bls::PrivateKey>(sks, sksLen),
                toVectorHashes(hashes, hashesLen)
            )
        );
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return sk;
}

CG1Element CThresholdPublicKeyShare(void** pks, const size_t pksLen, const void* hash, bool* didErr) {
    bls::G1Element* el = nullptr;
    try {
        el = new bls::G1Element(
            bls::Threshold::PublicKeyShare(
                toBLSVector<bls::G1Element>(pks, pksLen),
                bls::Bytes((uint8_t*)hash, HashSize)
            )
        );
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return el;
}

CG1Element CThresholdPublicKeyRecover(void** pks,
                                      const size_t pksLen,
                                      void** hashes,
                                      const size_t hashesLen,
                                      bool* didErr) {
    bls::G1Element* el = nullptr;
    try {
        el = new bls::G1Element(
            bls::Threshold::PublicKeyRecover(
                toBLSVector<bls::G1Element>(pks, pksLen),
                toVectorHashes(hashes, hashesLen)
            )
        );
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return el;
}

CG2Element CThresholdSignatureShare(void** sigs, const size_t sigsLen, const void* hash, bool* didErr) {
    bls::G2Element* el = nullptr;
    try {
        el = new bls::G2Element(
            bls::Threshold::SignatureShare(
                toBLSVector<bls::G2Element>(sigs, sigsLen),
                bls::Bytes((uint8_t*)hash, HashSize)
            )
        );
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return el;
}

CG2Element CThresholdSignatureRecover(void** sigs,
                                      const size_t sigsLen,
                                      void** hashes,
                                      const size_t hashesLen,
                                      bool* didErr) {
    bls::G2Element* el = nullptr;
    try {
        el = new bls::G2Element(
            bls::Threshold::SignatureRecover(
                toBLSVector<bls::G2Element>(sigs, sigsLen),
                toVectorHashes(hashes, hashesLen)
            )
        );
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return el;
}

CG2Element CThresholdSign(const CPrivateKey sk, const void* hash) {
    bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(
        bls::Threshold::Sign(*skPtr, bls::Bytes((uint8_t*)hash, HashSize))
    );
}

bool CThresholdVerify(const CG1Element pk, const void* hash, const CG2Element sig) {
    bls::G1Element* pkPtr = (bls::G1Element*)pk;
    bls::G2Element* sigPtr = (bls::G2Element*)sig;
    return bls::Threshold::Verify(*pkPtr, bls::Bytes((uint8_t*)hash, HashSize), *sigPtr);
}
