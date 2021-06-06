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

#include "SignatureWrapper.h"

namespace js_wrappers {
InsecureSignatureWrapper::InsecureSignatureWrapper(const InsecureSignature &signature) : JSWrapper(signature) {}

const size_t InsecureSignatureWrapper::SIGNATURE_SIZE = InsecureSignature::SIGNATURE_SIZE;

std::vector <InsecureSignature> InsecureSignatureWrapper::Unwrap(
        std::vector <js_wrappers::InsecureSignatureWrapper> sigWrappers) {
    std::vector <InsecureSignature> signatures;
    for (auto &sigWrapper : sigWrappers) {
        signatures.push_back(sigWrapper.GetWrappedInstance());
    }
    return signatures;
}

InsecureSignatureWrapper InsecureSignatureWrapper::FromBytes(val buffer) {
    std::vector <uint8_t> bytes = helpers::toVector(buffer);
    InsecureSignature sig = InsecureSignature::FromBytes(bytes.data());
    return InsecureSignatureWrapper(sig);
}

InsecureSignatureWrapper InsecureSignatureWrapper::Aggregate(val insecureSignatureWrappers) {
    std::vector <InsecureSignature> signatures = InsecureSignatureWrapper::Unwrap(
            helpers::toVectorFromJSArray<InsecureSignatureWrapper>(insecureSignatureWrappers));
    InsecureSignature aggregatedSignature = InsecureSignature::Aggregate(signatures);
    return InsecureSignatureWrapper(aggregatedSignature);
}

bool InsecureSignatureWrapper::Verify(val hashesBuffers, val pubKeyWrappersArray) const {
    std::vector <std::vector<uint8_t>> hashesVectors = helpers::jsBuffersArrayToVector(hashesBuffers);
    std::vector<const uint8_t *> hashes;
    for (auto &i : hashesVectors) {
        hashes.push_back(i.data());
    }
    std::vector <PublicKey> pubKeysVector = PublicKeyWrapper::Unwrap(
            helpers::toVectorFromJSArray<PublicKeyWrapper>(pubKeyWrappersArray));

    return wrapped.Verify(hashes, pubKeysVector);
}

InsecureSignatureWrapper InsecureSignatureWrapper::DivideBy(val insecureSignatureWrappers) const {
    std::vector <InsecureSignature> signatures = InsecureSignatureWrapper::Unwrap(
            helpers::toVectorFromJSArray<InsecureSignatureWrapper>(insecureSignatureWrappers));
    InsecureSignature dividedSignature = wrapped.DivideBy(signatures);
    return InsecureSignatureWrapper(dividedSignature);
}

val InsecureSignatureWrapper::Serialize() const {
    std::vector <uint8_t> bytes = wrapped.Serialize();
    return helpers::toUint8Array(bytes);
}

///

SignatureWrapper::SignatureWrapper(const Signature &signature) : JSWrapper(signature) {}

const size_t SignatureWrapper::SIGNATURE_SIZE = Signature::SIGNATURE_SIZE;


std::vector <Signature> SignatureWrapper::Unwrap(std::vector <js_wrappers::SignatureWrapper> sigWrappers) {
    std::vector <Signature> signatures;
    for (auto &sigWrapper : sigWrappers) {
        signatures.push_back(sigWrapper.GetWrappedInstance());
    }
    return signatures;
}

SignatureWrapper SignatureWrapper::FromSignature(const Signature &signature) {
    return SignatureWrapper(signature);
}

SignatureWrapper SignatureWrapper::FromBytes(val buffer) {
    std::vector <uint8_t> bytes = helpers::toVector(buffer);
    Signature sig = Signature::FromBytes(bytes.data());
    return SignatureWrapper(sig);
}

SignatureWrapper
SignatureWrapper::FromBytesAndAggregationInfo(val buffer, const AggregationInfoWrapper &infoWrapper) {
    AggregationInfo info = infoWrapper.GetWrappedInstance();
    std::vector <uint8_t> bytes = helpers::toVector(buffer);
    Signature sig = Signature::FromBytes(bytes.data(), info);
    return SignatureWrapper(sig);
}

SignatureWrapper SignatureWrapper::AggregateSigs(val signatureWrappers) {
    std::vector <Signature> signatures = SignatureWrapper::Unwrap(
            helpers::toVectorFromJSArray<SignatureWrapper>(signatureWrappers));
    Signature aggregatedSignature = Signature::Aggregate(signatures);
    return SignatureWrapper(aggregatedSignature);
}

SignatureWrapper SignatureWrapper::FromInsecureSignature(const InsecureSignatureWrapper signature) {
    Signature sig = Signature::FromInsecureSig(signature.GetWrappedInstance());
    return SignatureWrapper(sig);
}

SignatureWrapper SignatureWrapper::FromInsecureSignatureAndInfo(const InsecureSignatureWrapper signature,
                                                                const AggregationInfoWrapper info) {
    Signature sig = Signature::FromInsecureSig(signature.GetWrappedInstance(), info.GetWrappedInstance());
    return SignatureWrapper(sig);
}

val SignatureWrapper::Serialize() const {
    return helpers::toUint8Array(wrapped.Serialize());
}

bool SignatureWrapper::Verify() const {
    return wrapped.Verify();
}

AggregationInfoWrapper SignatureWrapper::GetAggregationInfo() const {
    AggregationInfo in = AggregationInfo(*wrapped.GetAggregationInfo());
    AggregationInfoWrapper aw = AggregationInfoWrapper(in);
    return aw;
}

void SignatureWrapper::SetAggregationInfo(const AggregationInfoWrapper &newAggregationInfo) {
    wrapped.SetAggregationInfo(newAggregationInfo.GetWrappedInstance());
}

SignatureWrapper SignatureWrapper::DivideBy(val signatureWrappers) const {
    std::vector <Signature> signatures = SignatureWrapper::Unwrap(
            helpers::toVectorFromJSArray<SignatureWrapper>(signatureWrappers));
    Signature dividedSig = wrapped.DivideBy(signatures);
    return SignatureWrapper(dividedSig);
}
}  // namespace js_wrappers
