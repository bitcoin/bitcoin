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

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dashbls/bls.hpp>
#include <dashbls/elements.hpp>
#include <dashbls/hdkeys.hpp>
#include <dashbls/privatekey.hpp>
#include <dashbls/schemes.hpp>

namespace py = pybind11;
using namespace bls;


PYBIND11_MODULE(blspy, m)
{
    py::class_<PrivateKey>(m, "PrivateKey")
        .def_property_readonly_static(
            "PRIVATE_KEY_SIZE",
            [](py::object self) { return PrivateKey::PRIVATE_KEY_SIZE; })
        .def(
            "from_bytes",
            [](py::buffer const b) {
                py::buffer_info info = b.request();
                if (info.format != py::format_descriptor<uint8_t>::format() ||
                    info.ndim != 1)
                    throw std::runtime_error("Incompatible buffer format!");

                if ((int)info.size != PrivateKey::PRIVATE_KEY_SIZE) {
                    throw std::invalid_argument(
                        "Length of bytes object not equal to PrivateKey::SIZE");
                }
                auto data_ptr = reinterpret_cast<const uint8_t *>(info.ptr);
                std::array<uint8_t, PrivateKey::PRIVATE_KEY_SIZE> data;
                std::copy(data_ptr, data_ptr + PrivateKey::PRIVATE_KEY_SIZE, data.data());
                py::gil_scoped_release release;
                return PrivateKey::FromBytes(data);
            })
        .def(
            "__bytes__",
            [](const PrivateKey &k) {
                uint8_t *output =
                    Util::SecAlloc<uint8_t>(PrivateKey::PRIVATE_KEY_SIZE);
                {
                    py::gil_scoped_release release;
                    k.Serialize(output);
                }
                py::bytes ret = py::bytes(
                    reinterpret_cast<char *>(output),
                    PrivateKey::PRIVATE_KEY_SIZE);
                Util::SecFree(output);
                return ret;
            })
        .def(
            "__deepcopy__",
            [](const PrivateKey &k, const py::object &memo) {
                return PrivateKey(k);
            })
        .def("get_g1", [](const PrivateKey &k) {
                py::gil_scoped_release release;
                return k.GetG1Element();
            })
        .def("aggregate", &PrivateKey::Aggregate, py::call_guard<py::gil_scoped_release>())
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("__repr__", [](const PrivateKey &k) {
            py::gil_scoped_release release;
            uint8_t *output = Util::SecAlloc<uint8_t>(PrivateKey::PRIVATE_KEY_SIZE);
            k.Serialize(output);
            std::string ret =
                "<PrivateKey " +
                Util::HexStr(output, PrivateKey::PRIVATE_KEY_SIZE) + ">";
            Util::SecFree(output);
            return ret;
        });

    py::class_<Util>(m, "Util").def("hash256", [](const py::bytes &message) {
        std::string str(message);
        const uint8_t *input = reinterpret_cast<const uint8_t *>(str.data());
        uint8_t output[BLS::MESSAGE_HASH_LEN];
        {
            py::gil_scoped_release release;
            Util::Hash256(output, (const uint8_t *)str.data(), str.size());
        }
        return py::bytes(
            reinterpret_cast<char *>(output), BLS::MESSAGE_HASH_LEN);
    });

    py::class_<BasicSchemeMPL>(m, "BasicSchemeMPL")
        .def("sk_to_g1", [](const PrivateKey &seckey){
            py::gil_scoped_release release;
            return BasicSchemeMPL().SkToG1(seckey);
        })
        .def(
            "key_gen",
            [](const py::bytes &b) {
                std::string str(b);
                py::gil_scoped_release release;
                const vector<uint8_t> inputVec(str.begin(), str.end());
                return BasicSchemeMPL().KeyGen(inputVec);
            })
        .def("derive_child_sk", [](const PrivateKey& sk, uint32_t index){
            py::gil_scoped_release release;
            return BasicSchemeMPL().DeriveChildSk(sk, index);
        })
        .def("derive_child_sk_unhardened", [](const PrivateKey& sk, uint32_t index){
            py::gil_scoped_release release;
            return BasicSchemeMPL().DeriveChildSkUnhardened(sk, index);
        })
        .def("derive_child_pk_unhardened", [](const G1Element& pk, uint32_t index){
            py::gil_scoped_release release;
            return BasicSchemeMPL().DeriveChildPkUnhardened(pk, index);
        })
        .def("aggregate", [](const vector<G2Element> &signatures) {
            py::gil_scoped_release release;
            return BasicSchemeMPL().Aggregate(signatures);
        })
        .def(
            "sign",
            [](const PrivateKey &pk, const py::bytes &msg) {
                std::string s(msg);
                py::gil_scoped_release release;
                vector<uint8_t> v(s.begin(), s.end());
                return BasicSchemeMPL().Sign(pk, v);
            })
        .def(
            "verify",
            [](const G1Element &pk,
               const py::bytes &msg,
               const G2Element &sig) {
                std::string s(msg);
                py::gil_scoped_release release;
                vector<uint8_t> v(s.begin(), s.end());
                return BasicSchemeMPL().Verify(pk, v, sig);
            })
        .def(
            "aggregate_verify",
            [](const vector<G1Element> &pks,
               const vector<py::bytes> &msgs,
               const G2Element &sig) {
                vector<vector<uint8_t>> vecs(msgs.size());
                for (int i = 0; i < (int)msgs.size(); ++i) {
                    std::string s(msgs[i]);
                    vecs[i] = vector<uint8_t>(s.begin(), s.end());
                }
                py::gil_scoped_release release;
                return BasicSchemeMPL().AggregateVerify(pks, vecs, sig);
            })
        .def(
            "g2_from_message",
            [](const py::bytes &msg) {
                const auto msg_str = std::string(msg);
                py::gil_scoped_release release;
                const auto msg_bytes = Bytes((const uint8_t *)msg_str.c_str(), msg_str.size());
                return G2Element::FromMessage(
                    msg_bytes,
                    (const uint8_t *)BasicSchemeMPL::CIPHERSUITE_ID.c_str(),
                    BasicSchemeMPL::CIPHERSUITE_ID.size()
                );
            });

    py::class_<AugSchemeMPL>(m, "AugSchemeMPL")
        .def("sk_to_g1", [](const PrivateKey &seckey){
            py::gil_scoped_release release;
            return AugSchemeMPL().SkToG1(seckey);
        })
        .def(
            "key_gen",
            [](const py::bytes &b) {
                std::string str(b);
                py::gil_scoped_release release;
                const vector<uint8_t> inputVec(str.begin(), str.end());
                return AugSchemeMPL().KeyGen(inputVec);
            })
        .def("derive_child_sk", [](const PrivateKey& sk, uint32_t index){
            py::gil_scoped_release release;
            return AugSchemeMPL().DeriveChildSk(sk, index);
        })
        .def("derive_child_sk_unhardened", [](const PrivateKey& sk, uint32_t index){
            py::gil_scoped_release release;
            return AugSchemeMPL().DeriveChildSkUnhardened(sk, index);
        })
        .def("derive_child_pk_unhardened", [](const G1Element& pk, uint32_t index){
            py::gil_scoped_release release;
            return AugSchemeMPL().DeriveChildPkUnhardened(pk, index);
        })
        .def("aggregate", [](const vector<G2Element>& signatures) {
            py::gil_scoped_release release;
            return AugSchemeMPL().Aggregate(signatures);
        })
        .def(
            "sign",
            [](const PrivateKey &pk, const py::bytes &msg) {
                std::string s(msg);
                py::gil_scoped_release release;
                vector<uint8_t> v(s.begin(), s.end());
                return AugSchemeMPL().Sign(pk, v);
            })
        .def(
            "sign",
            [](const PrivateKey &pk,
               const py::bytes &msg,
               const G1Element &prepend_pk) {
                std::string s(msg);
                py::gil_scoped_release release;
                vector<uint8_t> v(s.begin(), s.end());
                return AugSchemeMPL().Sign(pk, v, prepend_pk);
            })
        .def(
            "verify",
            [](const G1Element &pk,
               const py::bytes &msg,
               const G2Element &sig) {
                std::string s(msg);
                py::gil_scoped_release release;
                vector<uint8_t> v(s.begin(), s.end());
                return AugSchemeMPL().Verify(pk, v, sig);
            })
        .def(
            "aggregate_verify",
            [](const vector<G1Element> &pks,
               const vector<py::bytes> &msgs,
               const G2Element &sig) {
                vector<vector<uint8_t>> vecs(msgs.size());
                for (int i = 0; i < (int)msgs.size(); ++i) {
                    std::string s(msgs[i]);
                    vecs[i] = vector<uint8_t>(s.begin(), s.end());
                }
                py::gil_scoped_release release;
                return AugSchemeMPL().AggregateVerify(pks, vecs, sig);
            })
        .def(
            "g2_from_message",
            [](const py::bytes &msg) {
                const auto msg_str = std::string(msg);
                py::gil_scoped_release release;
                const auto msg_bytes = Bytes((const uint8_t *)msg_str.c_str(), msg_str.size());
                return G2Element::FromMessage(
                    msg_bytes,
                    (const uint8_t *)AugSchemeMPL::CIPHERSUITE_ID.c_str(),
                    AugSchemeMPL::CIPHERSUITE_ID.size()
                );
            });

    py::class_<PopSchemeMPL>(m, "PopSchemeMPL")
        .def("sk_to_g1", [](const PrivateKey &seckey){
            py::gil_scoped_release release;
            return PopSchemeMPL().SkToG1(seckey);
        })
        .def(
            "key_gen",
            [](const py::bytes &b) {
                std::string str(b);
                py::gil_scoped_release release;
                const vector<uint8_t> inputVec(str.begin(), str.end());
                return PopSchemeMPL().KeyGen(inputVec);
            })
        .def("derive_child_sk", [](const PrivateKey& sk, uint32_t index){
            py::gil_scoped_release release;
            return PopSchemeMPL().DeriveChildSk(sk, index);
        })
        .def("derive_child_sk_unhardened", [](const PrivateKey& sk, uint32_t index){
            py::gil_scoped_release release;
            return PopSchemeMPL().DeriveChildSkUnhardened(sk, index);
        })
        .def("derive_child_pk_unhardened", [](const G1Element& pk, uint32_t index){
            py::gil_scoped_release release;
            return PopSchemeMPL().DeriveChildPkUnhardened(pk, index);
        })
        .def("aggregate", [](const vector<G2Element>& signatures) {
            py::gil_scoped_release release;
            return PopSchemeMPL().Aggregate(signatures);
        })
        .def(
            "sign",
            [](const PrivateKey &pk, const py::bytes &msg) {
                std::string s(msg);
                py::gil_scoped_release release;
                vector<uint8_t> v(s.begin(), s.end());
                return PopSchemeMPL().Sign(pk, v);
            })
        .def(
            "verify",
            [](const G1Element &pk,
               const py::bytes &msg,
               const G2Element &sig) {
                std::string s(msg);
                py::gil_scoped_release release;
                vector<uint8_t> v(s.begin(), s.end());
                return PopSchemeMPL().Verify(pk, v, sig);
            })
        .def(
            "aggregate_verify",
            [](const vector<G1Element> &pks,
               const vector<py::bytes> &msgs,
               const G2Element &sig) {
                vector<vector<uint8_t>> vecs(msgs.size());
                for (int i = 0; i < (int)msgs.size(); ++i) {
                    std::string s(msgs[i]);
                    vecs[i] = vector<uint8_t>(s.begin(), s.end());
                }
                py::gil_scoped_release release;
                return PopSchemeMPL().AggregateVerify(pks, vecs, sig);
            })
        .def(
            "g2_from_message",
            [](const py::bytes &msg) {
                const auto msg_str = std::string(msg);
                py::gil_scoped_release release;
                const auto msg_bytes = Bytes((const uint8_t *)msg_str.c_str(), msg_str.size());
                return G2Element::FromMessage(
                    msg_bytes,
                    (const uint8_t *)PopSchemeMPL::CIPHERSUITE_ID.c_str(),
                    PopSchemeMPL::CIPHERSUITE_ID.size()
                );
            })
        .def("pop_prove", [](const PrivateKey& privateKey){
            py::gil_scoped_release release;
            return PopSchemeMPL().PopProve(privateKey);
        })
        .def("pop_verify", [](const G1Element& pubkey, const G2Element& signature){
            py::gil_scoped_release release;
            return PopSchemeMPL().PopVerify(pubkey, signature);
        })
        .def(
            "fast_aggregate_verify",
            [](const vector<G1Element> &pks,
               const py::bytes &msg,
               const G2Element &sig) {
                std::string s(msg);
                py::gil_scoped_release release;
                vector<uint8_t> v(s.begin(), s.end());
                return PopSchemeMPL().FastAggregateVerify(pks, v, sig);
            });

    py::class_<G1Element>(m, "G1Element")
        .def_property_readonly_static(
            "SIZE", [](py::object self) { return G1Element::SIZE; })
        .def(py::init([](){
            py::gil_scoped_release release;
            return G1Element();
        }))
        .def(py::init(&G1Element::FromByteVector), py::call_guard<py::gil_scoped_release>())
        .def(py::init([](py::int_ pyint) {
            std::array<uint8_t, G1Element::SIZE> buffer{};
            if (_PyLong_AsByteArray(
                    (PyLongObject *)pyint.ptr(),
                    buffer.data(),
                    G1Element::SIZE,
                    0,
                    0) < 0) {
                throw std::invalid_argument("Failed to cast int to G1Element");
            }
            py::gil_scoped_release release;
            return G1Element::FromBytes(buffer);
        }))
        .def(py::init([](py::buffer const b) {
            py::buffer_info info = b.request();
            if (info.format != py::format_descriptor<uint8_t>::format() ||
                info.ndim != 1)
                throw std::runtime_error("Incompatible buffer format!");

            if ((int)info.size != G1Element::SIZE) {
                throw std::invalid_argument(
                    "Length of bytes object not equal to G1Element::SIZE");
            }
            auto data_ptr = static_cast<uint8_t *>(info.ptr);
            std::array<uint8_t, G1Element::SIZE> data;
            std::copy(data_ptr, data_ptr + G1Element::SIZE, data.data());
            py::gil_scoped_release release;
            return G1Element::FromBytes(data);
        }))
        .def(
            "from_bytes",
            [](py::buffer const b) {
                py::buffer_info info = b.request();
                if (info.format != py::format_descriptor<uint8_t>::format() ||
                    info.ndim != 1)
                    throw std::runtime_error("Incompatible buffer format!");

                if ((int)info.size != G1Element::SIZE) {
                    throw std::invalid_argument(
                        "Length of bytes object not equal to G1Element::SIZE");
                }
                auto data_ptr = reinterpret_cast<const uint8_t *>(info.ptr);
                std::array<uint8_t, G1Element::SIZE> data;
                std::copy(data_ptr, data_ptr + G1Element::SIZE, data.data());
                py::gil_scoped_release release;
                return G1Element::FromBytes(data);
            })
        .def(
            "from_bytes_unchecked",
            [](py::buffer const b) {
              py::buffer_info info = b.request();
              if (info.format != py::format_descriptor<uint8_t>::format() ||
                  info.ndim != 1)
                  throw std::runtime_error("Incompatible buffer format!");

              if ((int)info.size != G1Element::SIZE) {
                  throw std::invalid_argument(
                      "Length of bytes object not equal to G1Element::SIZE");
              }
              auto data_ptr = reinterpret_cast<const uint8_t *>(info.ptr);
              return G1Element::FromBytesUnchecked({data_ptr, G1Element::SIZE});
            })
        .def("generator", &G1Element::Generator)
        .def("from_message", py::overload_cast<const std::vector<uint8_t>&, const uint8_t*, int>(&G1Element::FromMessage), py::call_guard<py::gil_scoped_release>())
        .def("pair", &G1Element::Pair, py::call_guard<py::gil_scoped_release>())
        .def("negate", &G1Element::Negate, py::call_guard<py::gil_scoped_release>())
        .def("get_fingerprint", &G1Element::GetFingerprint, py::call_guard<py::gil_scoped_release>())

        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(
            "__deepcopy__",
            [](const G1Element &g1, const py::object &memo) {
                return G1Element(g1);
            })
        .def(
            "__add__",
            [](G1Element &self, G1Element &other) {
                py::gil_scoped_release release;
                return self + other;
            },
            py::is_operator())
        .def(
            "__mul__",
            [](G1Element &self, bn_t other) {
                py::gil_scoped_release release;
                return self * (*(bn_t *)&other);
            },
            py::is_operator())
        .def(
            "__rmul__",
            [](G1Element &self, bn_t other) {
                py::gil_scoped_release release;
                return self * (*(bn_t *)&other);
            },
            py::is_operator())
        .def(
            "__and__",
            [](G1Element &self, G2Element &other) {
                py::gil_scoped_release release;
                return self & other;
            },
            py::is_operator())
        .def(
            "__repr__",
            [](const G1Element &ele) {
                py::gil_scoped_release release;
                std::stringstream s;
                s << ele;
                return "<G1Element " + s.str() + ">";
            })
        .def(
            "__str__",
            [](const G1Element &ele) {
                py::gil_scoped_release release;
                std::stringstream s;
                s << ele;
                return s.str();
            })
        .def(
            "__bytes__",
            [](const G1Element &ele) {
                vector<uint8_t> out;
                {
                    py::gil_scoped_release release;
                    out = ele.Serialize();
                }
                py::bytes ans = py::bytes(
                    reinterpret_cast<const char *>(out.data()), G1Element::SIZE);
                return ans;
            })
        .def("__deepcopy__", [](const G1Element &ele, const py::object &memo) {
            return G1Element(ele);
        });

    py::class_<G2Element>(m, "G2Element")
        .def_property_readonly_static(
            "SIZE", [](py::object self) { return G2Element::SIZE; })
        .def(py::init([](){
            return G2Element();
        }))
        .def(py::init(&G2Element::FromByteVector), py::call_guard<py::gil_scoped_release>())
        .def(py::init([](py::buffer const b) {
            py::buffer_info info = b.request();
            if (info.format != py::format_descriptor<uint8_t>::format() ||
                info.ndim != 1)
                throw std::runtime_error("Incompatible buffer format!");

            if ((int)info.size != G2Element::SIZE) {
                throw std::invalid_argument(
                    "Length of bytes object not equal to G2Element::SIZE");
            }
            auto data_ptr = static_cast<uint8_t *>(info.ptr);
            std::array<uint8_t, G2Element::SIZE> data;
            std::copy(data_ptr, data_ptr + G2Element::SIZE, data.data());
            py::gil_scoped_release release;
            return G2Element::FromBytes(data);
        }))
        .def(py::init([](py::int_ pyint) {
            std::array<uint8_t, G2Element::SIZE> buffer{};
            if (_PyLong_AsByteArray(
                    (PyLongObject *)pyint.ptr(),
                    buffer.data(),
                    G2Element::SIZE,
                    0,
                    0) < 0) {
                throw std::invalid_argument("Failed to cast int to G2Element");
            }
            py::gil_scoped_release release;
            return G2Element::FromBytes(buffer);
        }))
        .def(
            "from_bytes",
            [](py::buffer const b) {
                py::buffer_info info = b.request();
                if (info.format != py::format_descriptor<uint8_t>::format() ||
                    info.ndim != 1)
                    throw std::runtime_error("Incompatible buffer format!");

                if ((int)info.size != G2Element::SIZE) {
                    throw std::invalid_argument(
                        "Length of bytes object not equal to G2Element::SIZE");
                }
                auto data_ptr = reinterpret_cast<const uint8_t *>(info.ptr);
                std::array<uint8_t, G2Element::SIZE> data;
                std::copy(data_ptr, data_ptr + G2Element::SIZE, data.data());
                py::gil_scoped_release release;
                return G2Element::FromBytes(data);
            })
        .def(
            "from_bytes_unchecked",
            [](py::buffer const b) {
              py::buffer_info info = b.request();
              if (info.format != py::format_descriptor<uint8_t>::format() ||
                  info.ndim != 1)
                  throw std::runtime_error("Incompatible buffer format!");

              if ((int)info.size != G2Element::SIZE) {
                  throw std::invalid_argument(
                      "Length of bytes object not equal to G2Element::SIZE");
              }
              auto data_ptr = reinterpret_cast<const uint8_t *>(info.ptr);
              return G2Element::FromBytesUnchecked({data_ptr, G2Element::SIZE});
            })
        .def("generator", &G2Element::Generator)
        .def("from_message", py::overload_cast<const std::vector<uint8_t>&, const uint8_t*, int, bool>(&G2Element::FromMessage), py::call_guard<py::gil_scoped_release>())
        .def("pair", &G2Element::Pair, py::call_guard<py::gil_scoped_release>())
        .def("negate", &G2Element::Negate, py::call_guard<py::gil_scoped_release>())
        .def(
            "__deepcopy__",
            [](const G2Element &g2, const py::object &memo) {
                return G2Element(g2);
            })
        .def(py::self == py::self)
        .def(py::self != py::self)

        .def(
            "__add__",
            [](G2Element &self, G2Element &other) {
                py::gil_scoped_release release;
                return self + other;
            },
            py::is_operator())
        .def(
            "__mul__",
            [](G2Element &self, bn_t other) {
                py::gil_scoped_release release;
                return self * (*(bn_t *)&other);
            },
            py::is_operator())
        .def(
            "__rmul__",
            [](G2Element &self, bn_t other) {
                py::gil_scoped_release release;
                return self * (*(bn_t *)&other);
            },
            py::is_operator())

        .def(
            "__repr__",
            [](const G2Element &ele) {
                py::gil_scoped_release release;
                std::stringstream s;
                s << ele;
                return "<G2Element " + s.str() + ">";
            })
        .def(
            "__str__",
            [](const G2Element &ele) {
                py::gil_scoped_release release;
                std::stringstream s;
                s << ele;
                return s.str();
            })
        .def(
            "__bytes__",
            [](const G2Element &ele) {
                vector<uint8_t> out;
                {
                    py::gil_scoped_release release;
                    out = ele.Serialize();
                }
                py::bytes ans = py::bytes(
                    reinterpret_cast<const char *>(out.data()), G2Element::SIZE);
                return ans;
            })
        .def("__deepcopy__", [](const G2Element &ele, const py::object &memo) {
            return G2Element(ele);
        });

    py::class_<GTElement>(m, "GTElement")
        .def_property_readonly_static(
            "SIZE", [](py::object self) { return GTElement::SIZE; })
        .def(py::init(&GTElement::FromByteVector), py::call_guard<py::gil_scoped_release>())
        .def(py::init([](py::buffer const b) {
            py::buffer_info info = b.request();
            if (info.format != py::format_descriptor<uint8_t>::format() ||
                info.ndim != 1)
                throw std::runtime_error("Incompatible buffer format!");

            if ((int)info.size != GTElement::SIZE) {
                throw std::invalid_argument(
                    "Length of bytes object not equal to G2Element::SIZE");
            }
            auto data_ptr = static_cast<uint8_t *>(info.ptr);
            std::array<uint8_t, GTElement::SIZE> data;
            std::copy(data_ptr, data_ptr + GTElement::SIZE, data.data());
            py::gil_scoped_release release;
            return GTElement::FromBytes(data);
        }))
        .def(py::init([](py::int_ pyint) {
            std::array<uint8_t, G1Element::SIZE> buffer{};
            if (_PyLong_AsByteArray(
                    (PyLongObject *)pyint.ptr(),
                    buffer.data(),
                    GTElement::SIZE,
                    0,
                    0) < 0) {
                throw std::invalid_argument("Failed to cast int to GTElement");
            }
            py::gil_scoped_release release;
            return GTElement::FromBytes(buffer);
        }))
        .def(
            "from_bytes",
            [](py::buffer const b) {
                py::buffer_info info = b.request();
                if (info.format != py::format_descriptor<uint8_t>::format() ||
                    info.ndim != 1)
                    throw std::runtime_error("Incompatible buffer format!");

                if ((int)info.size != GTElement::SIZE) {
                    throw std::invalid_argument(
                        "Length of bytes object not equal to GTElement::SIZE");
                }
                auto data_ptr = reinterpret_cast<const uint8_t *>(info.ptr);
                std::array<uint8_t, GTElement::SIZE> data;
                std::copy(data_ptr, data_ptr + GTElement::SIZE, data.data());
                py::gil_scoped_release release;
                return GTElement::FromBytes(data);
            })
        .def(
            "from_bytes_unchecked",
            [](py::buffer const b) {
                py::buffer_info info = b.request();
                if (info.format != py::format_descriptor<uint8_t>::format() ||
                    info.ndim != 1)
                    throw std::runtime_error("Incompatible buffer format!");

                if ((int)info.size != GTElement::SIZE) {
                    throw std::invalid_argument(
                        "Length of bytes object not equal to GTElement::SIZE");
                }
                auto data_ptr = reinterpret_cast<const uint8_t *>(info.ptr);
                std::array<uint8_t, GTElement::SIZE> data;
                std::copy(data_ptr, data_ptr + GTElement::SIZE, data.data());
                py::gil_scoped_release release;
                return GTElement::FromBytesUnchecked(data);
            })
        .def("unity", &GTElement::Unity)
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(
            "__deepcopy__",
            [](const GTElement &gt, const py::object &memo) {
                return GTElement(gt);
            })
        .def(
            "__repr__",
            [](const GTElement &ele) {
                py::gil_scoped_release release;
                std::stringstream s;
                s << ele;
                return "<GTElement " + s.str() + ">";
            })
        .def(
            "__str__",
            [](const GTElement &ele) {
                py::gil_scoped_release release;
                std::stringstream s;
                s << ele;
                return s.str();
            })
        .def(
            "__bytes__",
            [](const GTElement &ele) {
                uint8_t *out = new uint8_t[GTElement::SIZE];
                {
                    py::gil_scoped_release release;
                    ele.Serialize(out);
                }
                py::bytes ans =
                    py::bytes(reinterpret_cast<char *>(out), GTElement::SIZE);
                delete[] out;
                return ans;
            })
        .def(
            "__mul__",
            [](GTElement &self, GTElement &other) {
                py::gil_scoped_release release;
                return self * other;
            },
            py::is_operator())
        .def("__deepcopy__", [](const GTElement &ele, const py::object &memo) {
            return GTElement(ele);
        });

    m.attr("PublicKeyMPL") = m.attr("G1Element");
    m.attr("SignatureMPL") = m.attr("G2Element");

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
