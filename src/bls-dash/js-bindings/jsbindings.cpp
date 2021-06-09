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

#include <emscripten/bind.h>
#include "wrappers/ExtendedPrivateKeyWrapper.h"
#include "wrappers/ThresholdWrapper.h"
#include "wrappers/BLSWrapper.h"

using namespace emscripten;

namespace js_wrappers {
const std::string GROUP_ORDER = BLS::GROUP_ORDER;

EMSCRIPTEN_BINDINGS(blsjs) {
    class_<PrivateKeyWrapper>("PrivateKey")
        .class_property("PRIVATE_KEY_SIZE", &PrivateKeyWrapper::PRIVATE_KEY_SIZE)
        .class_function("fromSeed", &PrivateKeyWrapper::FromSeed)
        .class_function("fromBytes", &PrivateKeyWrapper::FromBytes)
        .class_function("aggregate", &PrivateKeyWrapper::Aggregate)
        .class_function("aggregateInsecure", &PrivateKeyWrapper::AggregateInsecure)
        .function("serialize", &PrivateKeyWrapper::Serialize)
        .function("sign", &PrivateKeyWrapper::Sign)
        .function("signInsecure", &PrivateKeyWrapper::SignInsecure)
        .function("signPrehashed", &PrivateKeyWrapper::SignPrehashed)
        .function("getPublicKey", &PrivateKeyWrapper::GetPublicKey);

    class_<SignatureWrapper>("Signature")
        .class_property("SIGNATURE_SIZE", &SignatureWrapper::SIGNATURE_SIZE)
        .class_function("fromBytes", &SignatureWrapper::FromBytes)
        .class_function("fromBytesAndAggregationInfo", &SignatureWrapper::FromBytesAndAggregationInfo)
        .class_function("aggregateSigs", &SignatureWrapper::AggregateSigs)
        .class_function("fromInsecureSignature", &SignatureWrapper::FromInsecureSignature)
        .class_function("FromInsecureSignatureAndInfo", &SignatureWrapper::FromInsecureSignatureAndInfo)
        .function("serialize", &SignatureWrapper::Serialize)
        .function("verify", &SignatureWrapper::Verify)
        .function("getAggregationInfo", &SignatureWrapper::GetAggregationInfo)
        .function("setAggregationInfo", &SignatureWrapper::SetAggregationInfo)
        .function("divideBy", &SignatureWrapper::DivideBy);

    class_<InsecureSignatureWrapper>("InsecureSignature")
        .class_property("SIGNATURE_SIZE", &InsecureSignatureWrapper::SIGNATURE_SIZE)
        .class_function("fromBytes", &InsecureSignatureWrapper::FromBytes)
        .class_function("aggregate", &InsecureSignatureWrapper::Aggregate)
        .function("verify", &InsecureSignatureWrapper::Verify)
        .function("divideBy", &InsecureSignatureWrapper::DivideBy)
        .function("serialize", &InsecureSignatureWrapper::Serialize);

    class_<PublicKeyWrapper>("PublicKey")
        .class_property("PUBLIC_KEY_SIZE", &PublicKeyWrapper::PUBLIC_KEY_SIZE)
        .class_function("fromBytes", &PublicKeyWrapper::FromBytes)
        .class_function("aggregate", &PublicKeyWrapper::Aggregate)
        .class_function("aggregateInsecure", &PublicKeyWrapper::AggregateInsecure)
        .function("getFingerprint", &PublicKeyWrapper::GetFingerprint)
        .function("serialize", &PublicKeyWrapper::Serialize);

    class_<AggregationInfoWrapper>("AggregationInfo")
        .class_function("fromMsgHash", &AggregationInfoWrapper::FromMsgHash)
        .class_function("fromMsg", &AggregationInfoWrapper::FromMsg)
        .class_function("fromBuffers", &AggregationInfoWrapper::FromBuffers)
        .function("getPublicKeys", &AggregationInfoWrapper::GetPubKeys)
        .function("getMessageHashes", &AggregationInfoWrapper::GetMessageHashes)
        .function("getExponents", &AggregationInfoWrapper::GetExponents);

    class_<ExtendedPrivateKeyWrapper>("ExtendedPrivateKey")
        .class_property("EXTENDED_PRIVATE_KEY_SIZE", &ExtendedPrivateKeyWrapper::EXTENDED_PRIVATE_KEY_SIZE)
        .class_function("fromSeed", &ExtendedPrivateKeyWrapper::FromSeed, allow_raw_pointers())
        .class_function("fromBytes", &ExtendedPrivateKeyWrapper::FromBytes, allow_raw_pointers())
        .function("privateChild", &ExtendedPrivateKeyWrapper::PrivateChild)
        .function("publicChild", &ExtendedPrivateKeyWrapper::PublicChild)
        .function("getVersion", &ExtendedPrivateKeyWrapper::GetVersion)
        .function("getDepth", &ExtendedPrivateKeyWrapper::GetDepth)
        .function("getParentFingerprint", &ExtendedPrivateKeyWrapper::GetParentFingerprint)
        .function("getChildNumber", &ExtendedPrivateKeyWrapper::GetChildNumber)
        .function("getChainCode", &ExtendedPrivateKeyWrapper::GetChainCode)
        .function("getPrivateKey", &ExtendedPrivateKeyWrapper::GetPrivateKey)
        .function("getPublicKey", &ExtendedPrivateKeyWrapper::GetPublicKey)
        .function("getExtendedPublicKey", &ExtendedPrivateKeyWrapper::GetExtendedPublicKey)
        .function("serialize", &ExtendedPrivateKeyWrapper::Serialize);

    class_<ExtendedPublicKeyWrapper>("ExtendedPublicKey")
        .class_property("VERSION", &ExtendedPublicKeyWrapper::VERSION)
        .class_property("EXTENDED_PUBLIC_KEY_SIZE", &ExtendedPublicKeyWrapper::EXTENDED_PUBLIC_KEY_SIZE)
        .class_function("fromBytes", &ExtendedPublicKeyWrapper::FromBytes)
        .function("publicChild", &ExtendedPublicKeyWrapper::PublicChild)
        .function("getVersion", &ExtendedPublicKeyWrapper::GetVersion)
        .function("getDepth", &ExtendedPublicKeyWrapper::GetDepth)
        .function("getParentFingerprint", &ExtendedPublicKeyWrapper::GetParentFingerprint)
        .function("getChildNumber", &ExtendedPublicKeyWrapper::GetChildNumber)
        .function("getChainCode", &ExtendedPublicKeyWrapper::GetChainCode)
        .function("getPublicKey", &ExtendedPublicKeyWrapper::GetPublicKey)
        .function("serialize", &ExtendedPublicKeyWrapper::Serialize);

    class_<ChainCodeWrapper>("ChainCode")
        .class_property("CHAIN_CODE_SIZE", &ChainCodeWrapper::CHAIN_CODE_SIZE)
        .class_function("fromBytes", &ChainCodeWrapper::FromBytes)
        .function("serialize", &ChainCodeWrapper::Serialize);

    class_<ThresholdWrapper>("Threshold")
        .class_function("create", &ThresholdWrapper::Create)
        .class_function("signWithCoefficient", &ThresholdWrapper::SignWithCoefficient)
        .class_function("aggregateUnitSigs", &ThresholdWrapper::AggregateUnitSigs)
        .class_function("verifySecretFragment", &ThresholdWrapper::VerifySecretFragment);

    constant("GROUP_ORDER", GROUP_ORDER);
    function("DHKeyExchange", &BLSWrapper::DHKeyExchange);
};
}  // namespace js_wrappers
