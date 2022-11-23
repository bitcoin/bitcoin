//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

extern "C"
{
#include <Python.h>
#include <structmember.h>
}

#include <immer/algorithm.hpp>
#include <immer/refcount/unsafe_refcount_policy.hpp>
#include <immer/vector.hpp>

#include <iostream>

namespace {

struct heap_t
{
    template <typename... Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        return PyMem_Malloc(size);
    }

    template <typename... Tags>
    static void deallocate(std::size_t, void* obj, Tags...)
    {
        PyMem_Free(obj);
    }
};

struct object_t
{
    struct wrap_t
    {};
    struct adopt_t
    {};

    PyObject* ptr_ = nullptr;

    object_t() = delete;
    ~object_t() { Py_XDECREF(ptr_); }

    explicit object_t(PyObject* p, wrap_t)
        : ptr_{p}
    {}
    explicit object_t(PyObject* p, adopt_t)
        : ptr_{p}
    {
        assert(p);
        Py_INCREF(p);
    }

    static object_t wrap(PyObject* p) { return object_t{p, wrap_t{}}; }
    static object_t adopt(PyObject* p) { return object_t{p, adopt_t{}}; }

    object_t(const object_t& o)
        : ptr_(o.ptr_)
    {
        Py_INCREF(ptr_);
    }
    object_t(object_t&& o) { std::swap(ptr_, o.ptr_); }

    object_t& operator=(const object_t& o)
    {
        Py_XINCREF(o.ptr_);
        Py_XDECREF(ptr_);
        ptr_ = o.ptr_;
        return *this;
    }
    object_t& operator=(object_t&& o)
    {
        std::swap(ptr_, o.ptr_);
        return *this;
    }

    PyObject* release()
    {
        auto p = ptr_;
        ptr_   = nullptr;
        return p;
    }

    PyObject* get() const { return ptr_; }
};

using memory_t =
    immer::memory_policy<immer::unsafe_free_list_heap_policy<heap_t>,
                         immer::unsafe_refcount_policy,
                         immer::no_lock_policy>;

using vector_impl_t = immer::vector<object_t, memory_t>;

struct vector_t
{
    PyObject_HEAD vector_impl_t impl;
    PyObject* in_weakreflist;

    static PyTypeObject type;
};

vector_t* empty_vector = nullptr;

vector_t* make_vector()
{
    auto* v = PyObject_GC_New(vector_t, &vector_t::type);
    new (&v->impl) vector_impl_t{};
    v->in_weakreflist = nullptr;
    PyObject_GC_Track((PyObject*) v);
    return v;
}

vector_t* make_vector(vector_impl_t&& impl)
{
    auto v = PyObject_GC_New(vector_t, &vector_t::type);
    new (&v->impl) vector_impl_t{std::move(impl)};
    v->in_weakreflist = nullptr;
    PyObject_GC_Track((PyObject*) v);
    return v;
}

auto todo()
{
    PyErr_SetString(PyExc_RuntimeError, "immer: todo!");
    return nullptr;
}

void vector_dealloc(vector_t* self)
{
    if (self->in_weakreflist != nullptr)
        PyObject_ClearWeakRefs((PyObject*) self);

    PyObject_GC_UnTrack((PyObject*) self);
    Py_TRASHCAN_SAFE_BEGIN(self);

    self->impl.~vector_impl_t();

    PyObject_GC_Del(self);
    Py_TRASHCAN_SAFE_END(self);
}

PyObject* vector_to_list(vector_t* self)
{
    auto list = PyList_New(self->impl.size());
    auto idx  = 0;
    immer::for_each(self->impl, [&](auto&& obj) {
        auto o = obj.get();
        Py_INCREF(o);
        PyList_SET_ITEM(list, idx, o);
        ++idx;
    });
    return list;
}

PyObject* vector_repr(vector_t* self)
{
    auto list      = vector_to_list(self);
    auto list_repr = PyObject_Repr(list);
    Py_DECREF(list);

    if (!list_repr)
        return nullptr;

#if PY_MAJOR_VERSION >= 3
    auto s = PyUnicode_FromFormat("%s%U%s", "immer.vector(", list_repr, ")");
    Py_DECREF(list_repr);
#else
    auto s = PyString_FromString("immer.vector(");
    PyString_ConcatAndDel(&s, list_repr);
    PyString_ConcatAndDel(&s, PyString_FromString(")"));
#endif
    return s;
}

Py_ssize_t vector_len(vector_t* self) { return self->impl.size(); }

PyObject* vector_extend(vector_t* self, PyObject* iterable) { return todo(); }

vector_t* vector_append(vector_t* self, PyObject* obj)
{
    assert(obj != nullptr);
    return make_vector(self->impl.push_back(object_t::adopt(obj)));
}

vector_t* vector_set(vector_t* self, PyObject* args)
{
    PyObject* obj = nullptr;
    Py_ssize_t pos;
    // the n parses for size, the O parses for a Python object
    if (!PyArg_ParseTuple(args, "nO", &pos, &obj)) {
        return nullptr;
    }
    if (pos < 0)
        pos += self->impl.size();
    if (pos < 0 || pos > (Py_ssize_t) self->impl.size()) {
        PyErr_Format(PyExc_IndexError, "Index out of range: %zi", pos);
        return nullptr;
    }
    return make_vector(self->impl.set(pos, object_t::adopt(obj)));
}

PyObject* vector_new(PyTypeObject* subtype, PyObject* args, PyObject* kwds)
{
    Py_INCREF(empty_vector);
    return (PyObject*) empty_vector;
}

long vector_hash(vector_t* self)
{
    todo();
    return 0;
}

PyObject* vector_get_item(vector_t* self, Py_ssize_t pos)
{
    if (pos < 0)
        pos += self->impl.size();
    if (pos < 0 || pos >= (Py_ssize_t) self->impl.size()) {
        PyErr_Format(PyExc_IndexError, "Index out of range: %zi", pos);
        return nullptr;
    }
    auto r = self->impl[pos];
    return r.release();
}

PyObject* vector_subscript(vector_t* self, PyObject* item)
{
    if (PyIndex_Check(item)) {
        auto i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return nullptr;
        return vector_get_item(self, i);
    } else if (PySlice_Check(item)) {
        return todo();
    } else {
        PyErr_Format(PyExc_TypeError,
                     "vector indices must be integers, not %.200s",
                     Py_TYPE(item)->tp_name);
        return nullptr;
    }
}

PyObject* vector_repeat(vector_t* self, Py_ssize_t n) { return todo(); }

int vector_traverse(vector_t* self, visitproc visit, void* arg)
{
    auto result = 0;
    immer::all_of(self->impl, [&](auto&& o) {
        return 0 == (result = [&] {
                   Py_VISIT(o.get());
                   return 0;
               }());
    });
    return result;
}

PyObject* vector_richcompare(PyObject* v, PyObject* w, int op)
{
    return todo();
}

PyObject* vector_iter(PyObject* self) { return todo(); }

PyMethodDef vector_methods[] = {
    {"append", (PyCFunction) vector_append, METH_O, "Appends an element"},
    {"set",
     (PyCFunction) vector_set,
     METH_VARARGS,
     "Inserts an element at the specified position"},
    {"extend", (PyCFunction) vector_extend, METH_O | METH_COEXIST, "Extend"},
    {"tolist", (PyCFunction) vector_to_list, METH_NOARGS, "Convert to list"},
    {0}};

PyMemberDef vector_members[] = {
    {0} /* sentinel */
};

PySequenceMethods vector_sequence_methods = {
    (lenfunc) vector_len,           /* sq_length */
    (binaryfunc) vector_extend,     /* sq_concat */
    (ssizeargfunc) vector_repeat,   /* sq_repeat */
    (ssizeargfunc) vector_get_item, /* sq_item */
    0,                              /* sq_slice */
    0,                              /* sq_ass_item */
    0,                              /* sq_ass_slice */
    0,                              /* sq_contains */
    0,                              /* sq_inplace_concat */
    0,                              /* sq_inplace_repeat */
};

PyMappingMethods vector_mapping_methods = {
    (lenfunc) vector_len, (binaryfunc) vector_subscript, 0};

PyTypeObject vector_t::type = {
    PyVarObject_HEAD_INIT(NULL, 0) "immer.Vector", /* tp_name        */
    sizeof(vector_t),                              /* tp_basicsize   */
    0,                                             /* tp_itemsize    */
    (destructor) vector_dealloc,                   /* tp_dealloc     */
    0,                                             /* tp_print       */
    0,                                             /* tp_getattr     */
    0,                                             /* tp_setattr     */
    0,                                             /* tp_compare     */
    (reprfunc) vector_repr,                        /* tp_repr        */
    0,                                             /* tp_as_number   */
    &vector_sequence_methods,                      /* tp_as_sequence */
    &vector_mapping_methods,                       /* tp_as_mapping  */
    (hashfunc) vector_hash,                        /* tp_hash        */
    0,                                             /* tp_call        */
    0,                                             /* tp_str         */
    0,                                             /* tp_getattro    */
    0,                                             /* tp_setattro    */
    0,                                             /* tp_as_buffer   */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,       /* tp_flags       */
    "",                                            /* tp_doc         */
    (traverseproc) vector_traverse,                /* tp_traverse       */
    0,                                             /* tp_clear          */
    vector_richcompare,                            /* tp_richcompare    */
    offsetof(vector_t, in_weakreflist),            /* tp_weaklistoffset */
    vector_iter,                                   /* tp_iter           */
    0,                                             /* tp_iternext       */
    vector_methods,                                /* tp_methods        */
    vector_members,                                /* tp_members        */
    0,                                             /* tp_getset         */
    0,                                             /* tp_base           */
    0,                                             /* tp_dict           */
    0,                                             /* tp_descr_get      */
    0,                                             /* tp_descr_set      */
    0,                                             /* tp_dictoffset     */
    0,                                             /* tp_init           */
    0,                                             /* tp_alloc */
    vector_new,                                    /* tp_new   */
};

#if PY_MAJOR_VERSION >= 3
struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "immer_python_module", /* m_name */
    "",                    /* m_doc */
    -1,                    /* m_size */
    / module_methods,      /* m_methods */
    0,                     /* m_reload */
    0,                     /* m_traverse */
    0,                     /* m_clear */
    0,                     /* m_free */
};
#endif

PyMethodDef module_methods[] = {{0, 0, 0, 0}};

PyObject* module_init()
{
    if (PyType_Ready(&vector_t::type) < 0)
        return nullptr;

#if PY_MAJOR_VERSION >= 3
    auto m = PyModule_Create(&module_def);
#else
    auto m = Py_InitModule3("immer_python_module", module_methods, "");
#endif
    if (!m)
        return nullptr;

    if (!empty_vector)
        empty_vector = make_vector();

    Py_INCREF(&vector_t::type);
    PyModule_AddObject(m, "Vector", (PyObject*) &vector_t::type);
    return m;
}

} // anonymous namespace

extern "C"
{
#if PY_MAJOR_VERSION >= 3
    PyMODINIT_FUNC PyInit_immer_python_module() { return module_init(); }
#else
    PyMODINIT_FUNC initimmer_python_module() { module_init(); }
#endif
}
