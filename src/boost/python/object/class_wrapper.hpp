// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef CLASS_WRAPPER_DWA20011221_HPP
# define CLASS_WRAPPER_DWA20011221_HPP

# include <boost/python/to_python_converter.hpp>
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
# include <boost/python/converter/pytype_function.hpp>
#endif
# include <boost/ref.hpp>

namespace boost { namespace python { namespace objects { 

//
// These two classes adapt the static execute function of a class
// MakeInstance execute() function returning a new PyObject*
// reference. The first one is used for class copy constructors, and
// the second one is used to handle smart pointers.
//

template <class Src, class MakeInstance>
struct class_cref_wrapper
    : to_python_converter<Src,class_cref_wrapper<Src,MakeInstance> ,true>
{
    static PyObject* convert(Src const& x)
    {
        return MakeInstance::execute(boost::ref(x));
    }
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
    static PyTypeObject const *get_pytype() { return converter::registered_pytype_direct<Src>::get_pytype(); }
#endif
};

template <class Src, class MakeInstance>
struct class_value_wrapper
    : to_python_converter<Src,class_value_wrapper<Src,MakeInstance> ,true>
{
    static PyObject* convert(Src x)
    {
        return MakeInstance::execute(x);
    }
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
    static PyTypeObject const *get_pytype() { return MakeInstance::get_pytype(); }
#endif
};

}}} // namespace boost::python::objects

#endif // CLASS_WRAPPER_DWA20011221_HPP
