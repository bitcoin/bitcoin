// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef LVALUE_FROM_PYTYPE_DWA2002130_HPP
# define LVALUE_FROM_PYTYPE_DWA2002130_HPP

# include <boost/python/detail/prefix.hpp>
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
# include <boost/python/converter/pytype_function.hpp>
#endif

# include <boost/python/type_id.hpp>
# include <boost/python/converter/registry.hpp>
# include <boost/python/detail/void_ptr.hpp>
# include <boost/python/detail/type_traits.hpp>

namespace boost { namespace python {

namespace detail
{
  // Given a pointer-to-function of 1 parameter returning a reference
  // type, return the type_id of the function's return type.
  template <class T, class U>
  inline type_info extractor_type_id(T&(*)(U))
  {
      return type_id<T>();
  }

  // A function generator whose static execute() function is an lvalue
  // from_python converter using the given Extractor. U is expected to
  // be the actual type of the PyObject instance from which the result
  // is being extracted.
  template <class Extractor, class U>
  struct normalized_extractor
  {
      static inline void* execute(PyObject* op)
      {
          typedef typename add_lvalue_reference<U>::type param;
          return &Extractor::execute(
              boost::python::detail::void_ptr_to_reference(
                  op, (param(*)())0 )
              );
      }
  };

  // Given an Extractor type and a pointer to its execute function,
  // return a new object whose static execute function does the same
  // job but is a conforming lvalue from_python conversion function.
  //
  // usage: normalize<Extractor>(&Extractor::execute)
  template <class Extractor, class T, class U>
  inline normalized_extractor<Extractor,U>
  normalize(T(*)(U), Extractor* = 0)
  {
      return normalized_extractor<Extractor, U>();
  }
}

// An Extractor which extracts the given member from a Python object
// whose instances are stored as InstanceType.
template <class InstanceType, class MemberType, MemberType (InstanceType::*member)>
struct extract_member
{
    static MemberType& execute(InstanceType& c)
    {
        (void)Py_TYPE(&c); // static assertion
        return c.*member;
    }
};

// An Extractor which simply extracts the entire python object
// instance of InstanceType.
template <class InstanceType>
struct extract_identity
{
    static InstanceType& execute(InstanceType& c)
    {
        (void)Py_TYPE(&c); // static assertion
        return c;
    }
};

// Registers a from_python conversion which extracts lvalues using
// Extractor's static execute function from Python objects whose type
// object is python_type.
template <class Extractor, PyTypeObject const* python_type>
struct lvalue_from_pytype 
{
    lvalue_from_pytype()
    {
        converter::registry::insert
            ( &extract
            , detail::extractor_type_id(&Extractor::execute)
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
            , &get_pytype
#endif
            );
    }
 private:
    static void* extract(PyObject* op)
    {
        return PyObject_TypeCheck(op, const_cast<PyTypeObject*>(python_type))
            ? const_cast<void*>(
                static_cast<void const volatile*>(
                    detail::normalize<Extractor>(&Extractor::execute).execute(op)))
            : 0
            ;
    }
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
    static PyTypeObject const*get_pytype() { return python_type; }
#endif
};

}} // namespace boost::python

#endif // LVALUE_FROM_PYTYPE_DWA2002130_HPP
