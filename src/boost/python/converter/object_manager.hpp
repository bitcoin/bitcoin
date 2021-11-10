// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef OBJECT_MANAGER_DWA2002614_HPP
# define OBJECT_MANAGER_DWA2002614_HPP

# include <boost/python/handle.hpp>
# include <boost/python/cast.hpp>
# include <boost/python/converter/pyobject_traits.hpp>
# include <boost/python/detail/type_traits.hpp>
# include <boost/mpl/if.hpp>
# include <boost/python/detail/indirect_traits.hpp>
# include <boost/mpl/bool.hpp>

// Facilities for dealing with types which always manage Python
// objects. Some examples are object, list, str, et. al. Different
// to_python/from_python conversion rules apply here because in
// contrast to other types which are typically embedded inside a
// Python object, these are wrapped around a Python object. For most
// object managers T, a C++ non-const T reference argument does not
// imply the existence of a T lvalue embedded in the corresponding
// Python argument, since mutating member functions on T actually only
// modify the held Python object.
//
// handle<T> is an object manager, though strictly speaking it should
// not be. In other words, even though mutating member functions of
// hanlde<T> actually modify the handle<T> and not the T object,
// handle<T>& arguments of wrapped functions will bind to "rvalues"
// wrapping the actual Python argument, just as with other object
// manager classes. Making an exception for handle<T> is simply not
// worth the trouble.
//
// borrowed<T> cv* is an object manager so that we can use the general
// to_python mechanisms to convert raw Python object pointers to
// python, without the usual semantic problems of using raw pointers.


// Object Manager Concept requirements:
//
//    T is an Object Manager
//    p is a PyObject*
//    x is a T
//
//    * object_manager_traits<T>::is_specialized == true
//
//    * T(detail::borrowed_reference(p))
//        Manages p without checking its type
//
//    * get_managed_object(x, boost::python::tag)
//        Convertible to PyObject*
//
// Additional requirements if T can be converted from_python:
//
//    * T(object_manager_traits<T>::adopt(p))
//        steals a reference to p, or throws a TypeError exception if
//        p doesn't have an appropriate type. May assume p is non-null
//
//    * X::check(p)
//        convertible to bool. True iff T(X::construct(p)) will not
//        throw.

// Forward declarations
//
namespace boost { namespace python
{
  namespace api
  {
    class object; 
  }
}}

namespace boost { namespace python { namespace converter { 


// Specializations for handle<T>
template <class T>
struct handle_object_manager_traits
    : pyobject_traits<typename T::element_type>
{
 private:
  typedef pyobject_traits<typename T::element_type> base;
  
 public:
  BOOST_STATIC_CONSTANT(bool, is_specialized = true);

  // Initialize with a null_ok pointer for efficiency, bypassing the
  // null check since the source is always non-null.
  static null_ok<typename T::element_type>* adopt(PyObject* p)
  {
      return python::allow_null(base::checked_downcast(p));
  }
};

template <class T>
struct default_object_manager_traits
{
    BOOST_STATIC_CONSTANT(
        bool, is_specialized = python::detail::is_borrowed_ptr<T>::value
        );
};

template <class T>
struct object_manager_traits
    : mpl::if_c<
         is_handle<T>::value
       , handle_object_manager_traits<T>
       , default_object_manager_traits<T>
    >::type
{
};

//
// Traits for detecting whether a type is an object manager or a
// (cv-qualified) reference to an object manager.
// 

template <class T>
struct is_object_manager
    : mpl::bool_<object_manager_traits<T>::is_specialized>
{
};

template <class T>
struct is_reference_to_object_manager
    : mpl::false_
{
};

template <class T>
struct is_reference_to_object_manager<T&>
    : is_object_manager<T>
{
};

template <class T>
struct is_reference_to_object_manager<T const&>
    : is_object_manager<T>
{
};

template <class T>
struct is_reference_to_object_manager<T volatile&>
    : is_object_manager<T>
{
};

template <class T>
struct is_reference_to_object_manager<T const volatile&>
    : is_object_manager<T>
{
};

}}} // namespace boost::python::converter

#endif // OBJECT_MANAGER_DWA2002614_HPP
