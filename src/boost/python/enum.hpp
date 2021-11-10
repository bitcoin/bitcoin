// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef ENUM_DWA200298_HPP
# define ENUM_DWA200298_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/object/enum_base.hpp>
# include <boost/python/converter/rvalue_from_python_data.hpp>
# include <boost/python/converter/registered.hpp>

namespace boost { namespace python { 

template <class T>
struct enum_ : public objects::enum_base
{
    typedef objects::enum_base base;

    // Declare a new enumeration type in the current scope()
    enum_(char const* name, char const* doc = 0);

    // Add a new enumeration value with the given name and value.
    inline enum_<T>& value(char const* name, T);

    // Add all of the defined enumeration values to the current scope with the
    // same names used here.
    inline enum_<T>& export_values();
 private:
    static PyObject* to_python(void const* x);
    static void* convertible_from_python(PyObject* obj);
    static void construct(PyObject* obj, converter::rvalue_from_python_stage1_data* data);
};

template <class T>
inline enum_<T>::enum_(char const* name, char const* doc )
    : base(
        name
        , &enum_<T>::to_python
        , &enum_<T>::convertible_from_python
        , &enum_<T>::construct
        , type_id<T>()
        , doc
        )
{
}

// This is the conversion function that gets registered for converting
// these enums to Python.
template <class T>
PyObject* enum_<T>::to_python(void const* x)
{
    return base::to_python(
        converter::registered<T>::converters.m_class_object
        , static_cast<long>(*(T const*)x));
}

//
// The following two static functions serve as the elements of an
// rvalue from_python converter for the enumeration type.
//

// This checks that a given Python object can be converted to the
// enumeration type.
template <class T>
void* enum_<T>::convertible_from_python(PyObject* obj)
{
    return PyObject_IsInstance(
        obj
        , upcast<PyObject>(
            converter::registered<T>::converters.m_class_object))
        
        ? obj : 0;
}

// Constructs an instance of the enumeration type in the from_python
// data.
template <class T>
void enum_<T>::construct(PyObject* obj, converter::rvalue_from_python_stage1_data* data)
{
#if PY_VERSION_HEX >= 0x03000000
    T x = static_cast<T>(PyLong_AS_LONG(obj));
#else
    T x = static_cast<T>(PyInt_AS_LONG(obj));
#endif
    void* const storage = ((converter::rvalue_from_python_storage<T>*)data)->storage.bytes;
    new (storage) T(x);
    data->convertible = storage;
}

template <class T>
inline enum_<T>& enum_<T>::value(char const* name, T x)
{
    this->add_value(name, static_cast<long>(x));
    return *this;
}

template <class T>
inline enum_<T>& enum_<T>::export_values()
{
    this->base::export_values();
    return *this;
}

}} // namespace boost::python

#endif // ENUM_DWA200298_HPP
