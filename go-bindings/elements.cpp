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
#include "dashbls/bls.hpp"
#include "blschia.h"
#include "error.h"
#include "elements.h"

// G1Element
int CG1ElementSize() {
    return bls::G1Element::SIZE;
}

CG1Element CG1ElementFromBytes(const void* data, bool* didErr) {
    bls::G1Element* el = nullptr;
    try {
        el = new bls::G1Element(
            bls::G1Element::FromBytes(bls::Bytes((uint8_t*)(data), bls::G1Element::SIZE))
        );
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return el;
}

CG1Element CG1ElementGenerator() {
    return new bls::G1Element(bls::G1Element::Generator());
}

bool CG1ElementIsValid(const CG1Element el) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    return elPtr->IsValid();
}

uint32_t CG1ElementGetFingerprint(const CG1Element el) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    return elPtr->GetFingerprint();
}

void* CG1ElementSerialize(const CG1Element el) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    const std::vector<uint8_t> serialized = elPtr->Serialize();
    uint8_t* buffer = (uint8_t*)malloc(bls::G1Element::SIZE);
    memcpy(buffer, serialized.data(), bls::G1Element::SIZE);
    return (void*)buffer;
}

bool CG1ElementIsEqual(const CG1Element el1, const CG1Element el2) {
    const bls::G1Element* el1Ptr = (bls::G1Element*)el1;
    const bls::G1Element* el2Ptr = (bls::G1Element*)el2;
    return *el1Ptr == *el2Ptr;
}

CG1Element CG1ElementAdd(const CG1Element el1, const CG1Element el2) {
    const bls::G1Element* el1Ptr = (bls::G1Element*)el1;
    const bls::G1Element* el2Ptr = (bls::G1Element*)el2;
    return new bls::G1Element((*el1Ptr) + (*el2Ptr));
}

CG1Element CG1ElementMul(const CG1Element el, const CPrivateKey sk) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G1Element(*elPtr * *skPtr);
}

CG1Element CG1ElementNegate(const CG1Element el) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    return new bls::G1Element(elPtr->Negate());
}

void CG1ElementFree(const CG1Element el) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    delete elPtr;
}

// G2Element
int CG2ElementSize() {
    return bls::G2Element::SIZE;
}

CG2Element CG2ElementFromBytes(const void* data, bool* didErr) {
    bls::G2Element* el = nullptr;
    try {
        el = new bls::G2Element(
            bls::G2Element::FromBytes(bls::Bytes((uint8_t*)data, bls::G2Element::SIZE))
        );
        *didErr = false;
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    return el;
}

CG2Element CG2ElementGenerator() {
    return new bls::G2Element(bls::G2Element::Generator());
}

bool CG2ElementIsValid(const CG2Element el) {
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    return elPtr->IsValid();
}

void* CG2ElementSerialize(const CG2Element el) {
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    const std::vector<uint8_t> serialized = elPtr->Serialize();
    uint8_t* buffer = (uint8_t*)malloc(bls::G2Element::SIZE);
    memcpy(buffer, serialized.data(), bls::G2Element::SIZE);
    return (void*)buffer;
}

bool CG2ElementIsEqual(const CG2Element el1, const CG2Element el2) {
    const bls::G2Element* el1Ptr = (bls::G2Element*)el1;
    const bls::G2Element* el2Ptr = (bls::G2Element*)el2;
    return *el1Ptr == *el2Ptr;
}

CG2Element CG2ElementAdd(const CG2Element el1, const CG2Element el2) {
    bls::G2Element* el1Ptr = (bls::G2Element*)el1;
    bls::G2Element* el2Ptr = (bls::G2Element*)el2;
    return new bls::G2Element(*el1Ptr + *el2Ptr);
}

CG2Element CG2ElementMul(const CG2Element el, const CPrivateKey sk) {
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(*elPtr * *skPtr);
}

CG2Element CG2ElementNegate(const CG2Element el) {
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    return new bls::G2Element(elPtr->Negate());
}

void CG2ElementFree(const CG2Element el) {
    bls::G2Element* elPtr = (bls::G2Element*)el;
    delete elPtr;
}
