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
#include "wrappers/PrivateKeyWrapper.h"
#include "wrappers/SchemeMPLWrapper.h"
#include "wrappers/UtilWrapper.h"

using namespace emscripten;

namespace js_wrappers {
EMSCRIPTEN_BINDINGS(blsjs) {
    class_<AugSchemeMPLWrapper>("AugSchemeMPL")
        .class_function("skToG1", &AugSchemeMPLWrapper::SkToG1)
        .class_function("keyGen", &AugSchemeMPLWrapper::KeyGen)
        .class_function("sign", &AugSchemeMPLWrapper::Sign)
        .class_function("signPrepend", &AugSchemeMPLWrapper::SignPrepend)
        .class_function("verify", &AugSchemeMPLWrapper::Verify)
        .class_function("aggregate", &AugSchemeMPLWrapper::Aggregate)
        .class_function("aggregateVerify", &AugSchemeMPLWrapper::AggregateVerify)
        .class_function("deriveChildSk", &AugSchemeMPLWrapper::DeriveChildSk)
        .class_function("deriveChildSkUnhardened", &AugSchemeMPLWrapper::DeriveChildSkUnhardened)
        .class_function("deriveChildPkUnhardened", &AugSchemeMPLWrapper::DeriveChildPkUnhardened);

    class_<SchemeMPLWrapper<BasicSchemeMPL>>("BasicSchemeMPL")
        .class_function("skToG1", &SchemeMPLWrapper<BasicSchemeMPL>::SkToG1)
        .class_function("keyGen", &SchemeMPLWrapper<BasicSchemeMPL>::KeyGen)
        .class_function("sign", &SchemeMPLWrapper<BasicSchemeMPL>::Sign)
        .class_function("verify", &SchemeMPLWrapper<BasicSchemeMPL>::Verify)
        .class_function("aggregate", &SchemeMPLWrapper<BasicSchemeMPL>::Aggregate)
        .class_function("aggregateVerify", &SchemeMPLWrapper<BasicSchemeMPL>::AggregateVerify)
        .class_function("deriveChildSk", &SchemeMPLWrapper<BasicSchemeMPL>::DeriveChildSk)
        .class_function("deriveChildSkUnhardened", &SchemeMPLWrapper<BasicSchemeMPL>::DeriveChildSkUnhardened)
        .class_function("deriveChildPkUnhardened", &SchemeMPLWrapper<BasicSchemeMPL>::DeriveChildPkUnhardened)
        .class_function("verifySecure", &SchemeMPLWrapper<BasicSchemeMPL>::VerifySecure);

    class_<PopSchemeMPLWrapper>("PopSchemeMPL")
        .class_function("skToG1", &PopSchemeMPLWrapper::SkToG1)
        .class_function("keyGen", &PopSchemeMPLWrapper::KeyGen)
        .class_function("sign", &PopSchemeMPLWrapper::Sign)
        .class_function("verify", &PopSchemeMPLWrapper::Verify)
        .class_function("aggregate", &PopSchemeMPLWrapper::Aggregate)
        .class_function("aggregateVerify", &PopSchemeMPLWrapper::AggregateVerify)
        .class_function("deriveChildSk", &PopSchemeMPLWrapper::DeriveChildSk)
        .class_function("deriveChildSkUnhardened", &PopSchemeMPLWrapper::DeriveChildSkUnhardened)
        .class_function("deriveChildPkUnhardened", &PopSchemeMPLWrapper::DeriveChildPkUnhardened)
        .class_function("popProve", &PopSchemeMPLWrapper::PopProve)
        .class_function("popVerify", &PopSchemeMPLWrapper::PopVerify)
        .class_function("fastAggregateVerify", &PopSchemeMPLWrapper::FastAggregateVerify);


    class_<G1ElementWrapper>("G1Element")
        .class_property("SIZE", &G1ElementWrapper::SIZE)
        .constructor<>()
        .class_function("fromBytes", &G1ElementWrapper::FromBytes)
        .class_function("generator", &G2ElementWrapper::Generator)
        .function("serialize", &G1ElementWrapper::Serialize)
        .function("negate", &G1ElementWrapper::Negate)
        .function("deepcopy", &G1ElementWrapper::Deepcopy)
        .function("getFingerprint", &G1ElementWrapper::GetFingerprint)
        .function("add", &G1ElementWrapper::Add)
        .function("mul", &G1ElementWrapper::Mul)
        .function("equalTo", &G1ElementWrapper::EqualTo);

    class_<G2ElementWrapper>("G2Element")
        .class_property("SIZE", &G2ElementWrapper::SIZE)
        .constructor<>()
        .class_function("fromBytes", &G2ElementWrapper::FromBytes)
        .class_function("fromG2", &G2ElementWrapper::FromG2Element)
        .class_function("aggregateSigs", &G2ElementWrapper::AggregateSigs)
        .class_function("generator", &G2ElementWrapper::Generator)
        .function("serialize", &G2ElementWrapper::Serialize)
        .function("negate", &G2ElementWrapper::Negate)
        .function("deepcopy", &G2ElementWrapper::Deepcopy)
        .function("add", &G2ElementWrapper::Add)
        .function("mul", &G2ElementWrapper::Mul)
        .function("equalTo", &G2ElementWrapper::EqualTo);

    class_<PrivateKeyWrapper>("PrivateKey")
        .class_property("PRIVATE_KEY_SIZE", &PrivateKeyWrapper::PRIVATE_KEY_SIZE)
        .class_function("fromBytes", &PrivateKeyWrapper::FromBytes)
        .class_function("aggregate", &PrivateKeyWrapper::Aggregate)
        .function("deepcopy", &PrivateKeyWrapper::Deepcopy)
        .function("serialize", &PrivateKeyWrapper::Serialize)
        .function("getG1", &PrivateKeyWrapper::GetG1)
        .function("getG2", &PrivateKeyWrapper::GetG2)
        .function("mulG1", &PrivateKeyWrapper::MulG1)
        .function("mulG2", &PrivateKeyWrapper::MulG2)
        .function("equalTo", &PrivateKeyWrapper::EqualTo);

    class_<BignumWrapper>("Bignum")
        .class_function("fromString", &BignumWrapper::FromString)
        .function("toString", &BignumWrapper::ToString);

    class_<UtilWrapper>("Util")
        .class_function("hash256", &UtilWrapper::Hash256)
        .class_function("hexStr", &UtilWrapper::HexStr);
};
}  // namespace js_wrappers
