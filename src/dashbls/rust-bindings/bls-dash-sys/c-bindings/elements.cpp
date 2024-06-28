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
#include "bls.hpp"
#include "blschia.h"
#include "error.h"
#include "elements.h"

// G1Element
int G1ElementSize() {
    return bls::G1Element::SIZE;
}

G1Element G1ElementFromBytes(const void* data, bool legacy, bool* didErr) {
    bls::G1Element* el = nullptr;
    try {
        el = new bls::G1Element(
            bls::G1Element::FromBytes(bls::Bytes((uint8_t*)(data), bls::G1Element::SIZE), legacy)
        );
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return el;
}

G1Element G1ElementGenerator() {
    return new bls::G1Element(bls::G1Element::Generator());
}

bool G1ElementIsValid(const G1Element el) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    return elPtr->IsValid();
}

uint32_t G1ElementGetFingerprint(const G1Element el, const bool legacy) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    return elPtr->GetFingerprint(legacy);
}

void* G1ElementSerialize(const G1Element el, const bool legacy) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    const std::vector<uint8_t> serialized = elPtr->Serialize(legacy);
    uint8_t* buffer = (uint8_t*)malloc(bls::G1Element::SIZE);
    memcpy(buffer, serialized.data(), bls::G1Element::SIZE);
    return (void*)buffer;
}

bool G1ElementIsEqual(const G1Element el1, const G1Element el2) {
    const bls::G1Element* el1Ptr = (bls::G1Element*)el1;
    const bls::G1Element* el2Ptr = (bls::G1Element*)el2;
    return *el1Ptr == *el2Ptr;
}

G1Element G1ElementAdd(const G1Element el1, const G1Element el2) {
    const bls::G1Element* el1Ptr = (bls::G1Element*)el1;
    const bls::G1Element* el2Ptr = (bls::G1Element*)el2;
    return new bls::G1Element((*el1Ptr) + (*el2Ptr));
}

G1Element G1ElementMul(const G1Element el, const PrivateKey sk) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G1Element(*elPtr * *skPtr);
}

G1Element G1ElementNegate(const G1Element el) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    return new bls::G1Element(elPtr->Negate());
}

G1Element G1ElementCopy(const G1Element el) {
    return new bls::G1Element(((bls::G1Element*)el)->Copy());
}

void G1ElementFree(const G1Element el) {
    const bls::G1Element* elPtr = (bls::G1Element*)el;
    delete elPtr;
}

// G2Element
int G2ElementSize() {
    return bls::G2Element::SIZE;
}

G2Element G2ElementFromBytes(const void* data, const bool legacy, bool* didErr) {
    bls::G2Element* el = nullptr;
    try {
        el = new bls::G2Element(
            bls::G2Element::FromBytes(bls::Bytes((uint8_t*)data, bls::G2Element::SIZE), legacy)
        );
        *didErr = false;
    } catch(const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    return el;
}

G2Element G2ElementGenerator() {
    return new bls::G2Element(bls::G2Element::Generator());
}

bool G2ElementIsValid(const G2Element el) {
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    return elPtr->IsValid();
}

void* G2ElementSerialize(const G2Element el, const bool legacy) {
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    const std::vector<uint8_t> serialized = elPtr->Serialize(legacy);
    uint8_t* buffer = (uint8_t*)malloc(bls::G2Element::SIZE);
    memcpy(buffer, serialized.data(), bls::G2Element::SIZE);
    return (void*)buffer;
}

bool G2ElementIsEqual(const G2Element el1, const G2Element el2) {
    const bls::G2Element* el1Ptr = (bls::G2Element*)el1;
    const bls::G2Element* el2Ptr = (bls::G2Element*)el2;
    return *el1Ptr == *el2Ptr;
}

G2Element G2ElementAdd(const G2Element el1, const G2Element el2) {
    bls::G2Element* el1Ptr = (bls::G2Element*)el1;
    bls::G2Element* el2Ptr = (bls::G2Element*)el2;
    return new bls::G2Element(*el1Ptr + *el2Ptr);
}

G2Element G2ElementMul(const G2Element el, const PrivateKey sk) {
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(*elPtr * *skPtr);
}

G2Element G2ElementNegate(const G2Element el) {
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    return new bls::G2Element(elPtr->Negate());
}

G2Element G2ElementCopy(const G1Element el) {
    return new bls::G2Element(((bls::G2Element*)el)->Copy());
}

void G2ElementFree(const G2Element el) {
    bls::G2Element* elPtr = (bls::G2Element*)el;
    delete elPtr;
}
