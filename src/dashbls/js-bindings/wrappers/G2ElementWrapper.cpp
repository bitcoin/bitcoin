// Copyright 2020 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "G2ElementWrapper.h"

namespace js_wrappers {
G2ElementWrapper::G2ElementWrapper(const G2Element &signature) : JSWrapper(signature) {}

G2ElementWrapper::G2ElementWrapper() : JSWrapper(G2Element()) { }

const size_t G2ElementWrapper::SIZE = G2Element::SIZE;


std::vector <G2Element> G2ElementWrapper::Unwrap(std::vector <js_wrappers::G2ElementWrapper> sigWrappers) {
    std::vector <G2Element> signatures;
    for (auto &sigWrapper : sigWrappers) {
        signatures.push_back(sigWrapper.GetWrappedInstance());
    }
    return signatures;
}

G2ElementWrapper G2ElementWrapper::FromG2Element(const G2Element &signature) {
    return G2ElementWrapper(signature);
}

G2ElementWrapper G2ElementWrapper::FromBytes(val buffer) {
    std::vector <uint8_t> bytes = helpers::toVector(buffer);
    const bls::Bytes bytesView(bytes);
    G2Element sig = G2Element::FromBytes(bytesView);
    return G2ElementWrapper(sig);
}

G2ElementWrapper G2ElementWrapper::AggregateSigs(val signatureWrappers) {
    std::vector <G2Element> signatures = G2ElementWrapper::Unwrap(
            helpers::toVectorFromJSArray<G2ElementWrapper>(signatureWrappers));
    return G2ElementWrapper::FromG2Element(BasicSchemeMPL().Aggregate(signatures));
}

G2ElementWrapper G2ElementWrapper::Generator() {
    return G2ElementWrapper(G2Element::Generator());
}

G2ElementWrapper G2ElementWrapper::Deepcopy() {
    return G2ElementWrapper(GetWrappedInstance());
}

G2ElementWrapper G2ElementWrapper::Negate() {
    return G2ElementWrapper(GetWrappedInstance().Negate());
}

val G2ElementWrapper::Serialize() const {
    return helpers::toUint8Array(wrapped.Serialize());
}

G2ElementWrapper G2ElementWrapper::Add(const G2ElementWrapper& other) {
    return G2ElementWrapper(GetWrappedInstance() + other.GetWrappedInstance());
}

G2ElementWrapper G2ElementWrapper::Mul(const BignumWrapper& other) {
    bn_t factor;
    // Since the type of bn_t is bn_st[0], we can do this dance to make a
    // temporary copy of the struct itself to hand to G2ElementWrapper's
    // multiply.  The type was clearly intended to by by-value in argument lists
    // (degrade to ptr) but value semantics in C++ complicates matters.
    factor[0] = *(&other.GetWrappedInstance());
    return G2ElementWrapper(GetWrappedInstance() * factor);
}

bool G2ElementWrapper::EqualTo(const G2ElementWrapper& others) {
    return GetWrappedInstance() == others.GetWrappedInstance();
}
}  // namespace js_wrappers
