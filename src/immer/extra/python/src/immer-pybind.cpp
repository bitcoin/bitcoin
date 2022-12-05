//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <pybind11/pybind11.h>

#include <immer/vector.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>

namespace {

struct heap_t
{
    template <typename ...Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        return PyMem_Malloc(size);
    }

    template <typename ...Tags>
    static void deallocate(std::size_t, void* obj, Tags...)
    {
        PyMem_Free(obj);
    }
};

using memory_t = immer::memory_policy<
    immer::unsafe_free_list_heap_policy<heap_t>,
    immer::unsafe_refcount_policy>;

} // anonymous namespace

namespace py = pybind11;

PYBIND11_PLUGIN(immer_python_module)
{
    py::module m("immer", R"pbdoc(
        Immer
        -----
        .. currentmodule:: immer
        .. autosummary::
           :toctree: _generate
           Vector
    )pbdoc");

    using vector_t = immer::vector<py::object, memory_t>;

    py::class_<vector_t>(m, "Vector")
        .def(py::init<>())
        .def("__len__", &vector_t::size)
        .def("__getitem__",
             [] (const vector_t& v, std::size_t i) {
                 if (i > v.size())
                     throw py::index_error{"Index out of range"};
                 return v[i];
             })
        .def("append",
             [] (const vector_t& v, py::object x) {
                 return v.push_back(std::move(x));
             })
        .def("set",
             [] (const vector_t& v, std::size_t i, py::object x) {
                 return v.set(i, std::move(x));
             });

#ifdef VERSION_INFO
    m.attr("__version__") = py::str(VERSION_INFO);
#else
    m.attr("__version__") = py::str("dev");
#endif

    return m.ptr();
}
