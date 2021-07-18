#include <pybind11/pybind11.h>
#include "../../../src/crypto/chacha20.h"
#include <iostream>

namespace py = pybind11;

PYBIND11_MODULE(chacha20_bindings, m) {
    py::class_<ChaCha20>(m, "ChaCha20")
        .def(py::init<>()) // empty constructor
        .def("SetKey", [](ChaCha20 &c, const py::bytes &b) {
            std::string s = std::string(b);
            unsigned char* k = reinterpret_cast<unsigned char*>(&s[0]);

            c.SetKey(k, s.size());
        })
        .def("SetIV", &ChaCha20::SetIV)
        .def("Seek", &ChaCha20::Seek)
        .def("Keystream", [](ChaCha20 &c, size_t l) {
            unsigned char out[l];

            c.Keystream(out, l);

            return py::bytes(reinterpret_cast<char const*>(out), l);
        })
        .def("Crypt", [](ChaCha20 &c, const py::bytes &b) {
            std::string s = std::string(b);
            unsigned char* m = reinterpret_cast<unsigned char*>(&s[0]);
            unsigned char out[s.size()];

            c.Crypt(m, out, s.size());

            return py::bytes(reinterpret_cast<char const*>(out), s.size());
        });
}
