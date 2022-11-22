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

#ifndef JS_BINDINGS_WRAPPERS_SCHEMEMPLWRAPPER_H_
#define JS_BINDINGS_WRAPPERS_SCHEMEMPLWRAPPER_H_

#include <helpers.h>
#include "JSWrapper.h"
#include "G1ElementWrapper.h"
#include "PrivateKeyWrapper.h"

namespace js_wrappers {

// Basic scheme wrapper providing most of the methods.
template <typename SchemeMPL> class SchemeMPLWrapper : public JSWrapper<SchemeMPL> {
 public:
  static G1ElementWrapper SkToG1(const PrivateKeyWrapper &seckey) {
    return G1ElementWrapper(mpl.SkToG1(seckey.GetWrappedInstance()));
  }

  static PrivateKeyWrapper KeyGen(val seedVal) {
    std::vector <uint8_t> seed = helpers::toVector(seedVal);
    return PrivateKeyWrapper(mpl.KeyGen(seed));
  }

  static G2ElementWrapper Sign(const PrivateKeyWrapper &seckey, val messageVal) {
    std::vector <uint8_t> message = helpers::toVector(messageVal);
    return G2ElementWrapper(mpl.Sign(seckey.GetWrappedInstance(), message));
  }

  static bool Verify(const G1ElementWrapper &pubkey, val messageVal, const G2ElementWrapper &signature) {
    std::vector <uint8_t> message = helpers::toVector(messageVal);
    return mpl.Verify(pubkey.GetWrappedInstance(), message, signature.GetWrappedInstance());
  }

  static std::vector<G2Element> Unwrap(const std::vector<G2ElementWrapper> &wrappers) {
    std::vector<G2Element> unwrapped;
    for (auto &wrapper : wrappers) {
      unwrapped.push_back(wrapper.GetWrappedInstance());
    }
    return unwrapped;
  }

  static G2ElementWrapper Aggregate(val g2Elements) {
    std::vector<G2Element> signatures = G2ElementWrapper::Unwrap
      (helpers::toVectorFromJSArray<G2ElementWrapper>(g2Elements));
    return G2ElementWrapper(mpl.Aggregate(signatures));
  }

  static bool AggregateVerify(val pubkeyArray, val messagesVal, const G2ElementWrapper &signature) {
    std::vector<G1Element> pubkeys = G1ElementWrapper::Unwrap
      (helpers::toVectorFromJSArray<G1ElementWrapper>(pubkeyArray));
    std::vector<val> messagesVec = helpers::toVectorFromJSArray<val>(messagesVal);
    std::vector<std::vector<uint8_t>> messages;
    for (auto msgVal : messagesVec) {
      messages.push_back(helpers::toVector(msgVal));
    }
    return mpl.AggregateVerify(pubkeys, messages, signature.GetWrappedInstance());
  }

  static PrivateKeyWrapper DeriveChildSk(const PrivateKeyWrapper &sk, uint32_t index) {
    return PrivateKeyWrapper(mpl.DeriveChildSk(sk.GetWrappedInstance(), index));
  }

  static PrivateKeyWrapper DeriveChildSkUnhardened(const PrivateKeyWrapper &sk, uint32_t index) {
    return PrivateKeyWrapper(mpl.DeriveChildSkUnhardened(sk.GetWrappedInstance(), index));
  }

  static G1ElementWrapper DeriveChildPkUnhardened(const G1ElementWrapper &pk, uint32_t index) {
    return G1ElementWrapper(mpl.DeriveChildPkUnhardened(pk.GetWrappedInstance(), index));
  }

protected:
  static inline SchemeMPL mpl;
};

class AugSchemeMPLWrapper : public SchemeMPLWrapper<AugSchemeMPL> {
public:
  static G2ElementWrapper SignPrepend(const PrivateKeyWrapper &seckey, val messageVal, const G1ElementWrapper &prependPk) {
    std::vector <uint8_t> message = helpers::toVector(messageVal);
    return G2ElementWrapper(mpl.Sign(seckey.GetWrappedInstance(), message, prependPk.GetWrappedInstance()));
  }
};

class PopSchemeMPLWrapper : public SchemeMPLWrapper<PopSchemeMPL> {
public:
  static G2ElementWrapper PopProve(const PrivateKeyWrapper &seckey) {
    return G2ElementWrapper(mpl.PopProve(seckey.GetWrappedInstance()));
  }

  static bool PopVerify(const G1ElementWrapper &pubkey, const G2ElementWrapper &signatureProof) {
    return mpl.PopVerify(pubkey.GetWrappedInstance(), signatureProof.GetWrappedInstance());
  }

  static bool FastAggregateVerify
  (val pubkeyArray,
   val messageVal,
   const G2ElementWrapper &signature) {
    std::vector<G1Element> pubkeys = G1ElementWrapper::Unwrap
      (helpers::toVectorFromJSArray<G1ElementWrapper>(pubkeyArray));
    std::vector<uint8_t> message = helpers::toVector(messageVal);
    return mpl.FastAggregateVerify(pubkeys, message, signature.GetWrappedInstance());
  }
};

}  // namespace js_wrappers

#endif  // JS_BINDINGS_WRAPPERS_SIGNATUREWRAPPER_H_
