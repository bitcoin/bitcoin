// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

#include "../src/privatekey.hpp"
#include "../src/bls.hpp"

namespace py = pybind11;
using namespace bls;

PYBIND11_MODULE(blspy, m) {
    py::class_<bn_t*>(m, "bn_ptr");

    py::class_<AggregationInfo>(m, "AggregationInfo")
        .def("from_msg_hash", [](const PublicKey &pk, const py::bytes &b) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(b)[0]);
            return AggregationInfo::FromMsgHash(pk, input);
        })
        .def("from_msg", [](const PublicKey &pk, const py::bytes &b) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(b)[0]);
            return AggregationInfo::FromMsg(pk, input, len(b));
        })
        .def("merge_infos", &AggregationInfo::MergeInfos)
        .def("get_pubkeys", &AggregationInfo::GetPubKeys)
        .def("get_msg_hashes", [](const AggregationInfo &self) {
            std::vector<uint8_t*> msgHashes = self.GetMessageHashes();
            std::vector<py::bytes> ret;
            for (const uint8_t* msgHash : msgHashes) {
                ret.push_back(py::bytes(reinterpret_cast<const char*>(msgHash), BLS::MESSAGE_HASH_LEN));
            }
            return ret;
        })
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self < py::self)
        .def("__repr__", [](const AggregationInfo &a) {
            std::stringstream s;
            s << a;
            return "<AggregationInfo " + s.str() + ">";
        });

    py::class_<PrivateKey>(m, "PrivateKey")
        .def_property_readonly_static("PRIVATE_KEY_SIZE", [](py::object self) {
            return PrivateKey::PRIVATE_KEY_SIZE;
        })
        .def("from_seed", [](const py::bytes &b) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(b)[0]);
            return PrivateKey::FromSeed(input, len(b));
        })
        .def("from_bytes", [](const py::bytes &b) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(b)[0]);
            return PrivateKey::FromBytes(input);
        })
        .def("serialize", [](const PrivateKey &k) {
            uint8_t* output = Util::SecAlloc<uint8_t>(PrivateKey::PRIVATE_KEY_SIZE);
            k.Serialize(output);
            py::bytes ret = py::bytes(reinterpret_cast<char*>(output), PrivateKey::PRIVATE_KEY_SIZE);
            Util::SecFree(output);
            return ret;
        })
        .def("get_public_key", [](const PrivateKey &k) {
            return k.GetPublicKey();
        })
        .def("aggregate", &PrivateKey::Aggregate)
        .def("sign", [](const PrivateKey &k, const py::bytes &msg) {
            uint8_t* input = reinterpret_cast<uint8_t*>(&std::string(msg)[0]);
            return k.Sign(input, len(msg));
        })
        .def("sign_prehashed", [](const PrivateKey &k, const py::bytes &msg) {
            uint8_t* input = reinterpret_cast<uint8_t*>(&std::string(msg)[0]);
            return k.SignPrehashed(input);
        })
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("__repr__", [](const PrivateKey &k) {
            uint8_t* output = Util::SecAlloc<uint8_t>(PrivateKey::PRIVATE_KEY_SIZE);
            k.Serialize(output);
            std::string ret = "<PrivateKey " + Util::HexStr(output, PrivateKey::PRIVATE_KEY_SIZE) + ">";
            Util::SecFree(output);
            return ret;
        });


    py::class_<PublicKey>(m, "PublicKey")
        .def_property_readonly_static("PUBLIC_KEY_SIZE", [](py::object self) {
            return PublicKey::PUBLIC_KEY_SIZE;
        })
        .def("from_bytes", [](const py::bytes &b) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(b)[0]);
            return PublicKey::FromBytes(input);
        })
        .def("aggregate", &PublicKey::Aggregate)
        .def("get_fingerprint", &PublicKey::GetFingerprint)
        .def("serialize", [](const PublicKey &pk) {
            uint8_t* output = new uint8_t[PublicKey::PUBLIC_KEY_SIZE];
            pk.Serialize(output);
            py::bytes ret = py::bytes(reinterpret_cast<char*>(output), PublicKey::PUBLIC_KEY_SIZE);
            delete[] output;
            return ret;
        })
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("__repr__", [](const PublicKey &pk) {
            std::stringstream s;
            s << pk;
            return "<PublicKey " + s.str() + ">";
        });


    py::class_<Signature>(m, "Signature")
        .def_property_readonly_static("SIGNATURE_SIZE", [](py::object self) {
            return Signature::SIGNATURE_SIZE;
        })
        .def("from_bytes", [](const py::bytes &b) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(b)[0]);
            return Signature::FromBytes(input);
        })
        .def("serialize", [](const Signature &sig) {
            uint8_t* output = new uint8_t[Signature::SIGNATURE_SIZE];
            sig.Serialize(output);
            py::bytes ret = py::bytes(reinterpret_cast<char*>(output), Signature::SIGNATURE_SIZE);
            delete[] output;
            return ret;
        })
        .def("verify", &Signature::Verify)
        .def("aggregate", &Signature::AggregateSigs)
        .def("divide_by", &Signature::DivideBy)
        .def("set_aggregation_info", &Signature::SetAggregationInfo)
        .def("get_aggregation_info", [](const Signature &sig) {
            return *sig.GetAggregationInfo();
        })
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("__repr__", [](const Signature &sig) {
            std::stringstream s;
            s << sig;
            return "<Signature " + s.str() + ">";
        });

    py::class_<ChainCode>(m, "ChainCode")
        .def_property_readonly_static("CHAIN_CODE_KEY_SIZE", [](py::object self) {
            return ChainCode::CHAIN_CODE_SIZE;
        })
        .def("from_bytes", [](const py::bytes &b) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(b)[0]);
            return ChainCode::FromBytes(input);
        })
        .def("serialize", [](const ChainCode &cc) {
            uint8_t* output = new uint8_t[ChainCode::CHAIN_CODE_SIZE];
            cc.Serialize(output);
            py::bytes ret = py::bytes(reinterpret_cast<char*>(output),
                    ChainCode::CHAIN_CODE_SIZE);
            delete[] output;
            return ret;
        })
        .def("__repr__", [](const ChainCode &cc) {
            uint8_t* output = new uint8_t[ChainCode::CHAIN_CODE_SIZE];
            cc.Serialize(output);
            std::string ret = "<ChainCode " + Util::HexStr(output,
                    ChainCode::CHAIN_CODE_SIZE) + ">";
            Util::SecFree(output);
            return ret;
        });



    py::class_<ExtendedPublicKey>(m, "ExtendedPublicKey")
        .def_property_readonly_static("EXTENDED_PUBLIC_KEY_SIZE", [](py::object self) {
            return ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE;
        })
        .def("from_bytes", [](const py::bytes &b) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(b)[0]);
            return ExtendedPublicKey::FromBytes(input);
        })
        .def("public_child", &ExtendedPublicKey::PublicChild)
        .def("get_version", &ExtendedPublicKey::GetVersion)
        .def("get_depth", &ExtendedPublicKey::GetDepth)
        .def("get_parent_fingerprint", &ExtendedPublicKey::GetParentFingerprint)
        .def("get_child_number", &ExtendedPublicKey::GetChildNumber)
        .def("get_chain_code", &ExtendedPublicKey::GetChainCode)
        .def("get_public_key", &ExtendedPublicKey::GetPublicKey)
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("serialize", [](const ExtendedPublicKey &pk) {
            uint8_t* output = new uint8_t[
                    ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE];
            pk.Serialize(output);
            py::bytes ret = py::bytes(reinterpret_cast<char*>(output),
                    ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE);
            delete[] output;
            return ret;
        })
        .def("__repr__", [](const ExtendedPublicKey &pk) {
            uint8_t* output = new uint8_t[
                    ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE];
            pk.Serialize(output);
            std::string ret = "<ExtendedPublicKey " + Util::HexStr(output,
                    ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE) + ">";
            Util::SecFree(output);
            return ret;
        });

    py::class_<ExtendedPrivateKey>(m, "ExtendedPrivateKey")
        .def_property_readonly_static("EXTENDED_PRIVATE_KEY_SIZE", [](py::object self) {
            return ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE;
        })
        .def("from_seed", [](const py::bytes &seed) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(seed)[0]);
            return ExtendedPrivateKey::FromSeed(input, len(seed));
        })
        .def("from_bytes", [](const py::bytes &b) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(b)[0]);
            return ExtendedPrivateKey::FromBytes(input);
        })
        .def("private_child", &ExtendedPrivateKey::PrivateChild)
        .def("public_child", &ExtendedPrivateKey::PublicChild)
        .def("get_version", &ExtendedPrivateKey::GetVersion)
        .def("get_depth", &ExtendedPrivateKey::GetDepth)
        .def("get_parent_fingerprint", &ExtendedPrivateKey::GetParentFingerprint)
        .def("get_child_number", &ExtendedPrivateKey::GetChildNumber)
        .def("get_chain_code", &ExtendedPrivateKey::GetChainCode)
        .def("get_private_key", &ExtendedPrivateKey::GetPrivateKey)
        .def("get_public_key", &ExtendedPrivateKey::GetPublicKey)
        .def("get_extended_public_key", &ExtendedPrivateKey::GetExtendedPublicKey)
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("serialize", [](const ExtendedPrivateKey &k) {
            uint8_t* output = Util::SecAlloc<uint8_t>(
                    ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE);
            k.Serialize(output);
            py::bytes ret = py::bytes(reinterpret_cast<char*>(output),
                    ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE);
            Util::SecFree(output);
            return ret;
        })
        .def("__repr__", [](const ExtendedPrivateKey &k) {
            uint8_t* output = Util::SecAlloc<uint8_t>(
                    ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE);
            k.Serialize(output);
            std::string ret = "<ExtendedPrivateKey " + Util::HexStr(output,
                    ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE) + ">";
            Util::SecFree(output);
            return ret;
        });


    py::class_<BLS>(m, "BLS")
        .def_property_readonly_static("MESSAGE_HASH_LEN", [](py::object self) {
            return BLS::MESSAGE_HASH_LEN;
        });

    py::class_<Util>(m, "Util")
        .def("hash256", [](const py::bytes &message) {
            const uint8_t* input = reinterpret_cast<const uint8_t*>(&std::string(message)[0]);
            uint8_t output[BLS::MESSAGE_HASH_LEN];
            Util::Hash256(output, input, len(message));
            return py::bytes(reinterpret_cast<char*>(output), BLS::MESSAGE_HASH_LEN);
        });

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
