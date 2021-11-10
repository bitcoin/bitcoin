// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef OBJ_MGR_ARG_FROM_PYTHON_DWA2002628_HPP
# define OBJ_MGR_ARG_FROM_PYTHON_DWA2002628_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/detail/referent_storage.hpp>
# include <boost/python/detail/destroy.hpp>
# include <boost/python/detail/construct.hpp>
# include <boost/python/converter/object_manager.hpp>
# include <boost/python/detail/raw_pyobject.hpp>
# include <boost/python/tag.hpp>

//
// arg_from_python converters for Python type wrappers, to be used as
// base classes for specializations.
//
namespace boost { namespace python { namespace converter { 

template <class T>
struct object_manager_value_arg_from_python
{
    typedef T result_type;
    
    object_manager_value_arg_from_python(PyObject*);
    bool convertible() const;
    T operator()() const;
 private:
    PyObject* m_source;
};

// Used for converting reference-to-object-manager arguments from
// python. The process used here is a little bit odd. Upon
// construction, we build the object manager object in the m_result
// object, *forcing* it to accept the source Python object by casting
// its pointer to detail::borrowed_reference. This is supposed to
// bypass any type checking of the source object. The convertible
// check then extracts the owned object and checks it. If the check
// fails, nothing else in the program ever gets to touch this strange
// "forced" object.
template <class Ref>
struct object_manager_ref_arg_from_python
{
    typedef Ref result_type;
    
    object_manager_ref_arg_from_python(PyObject*);
    bool convertible() const;
    Ref operator()() const;
    ~object_manager_ref_arg_from_python();
 private:
    typename python::detail::referent_storage<Ref>::type m_result;
};

//
// implementations
//

template <class T>
inline object_manager_value_arg_from_python<T>::object_manager_value_arg_from_python(PyObject* x)
    : m_source(x)
{
}
    
template <class T>
inline bool object_manager_value_arg_from_python<T>::convertible() const
{
    return object_manager_traits<T>::check(m_source);
}

template <class T>
inline T object_manager_value_arg_from_python<T>::operator()() const
{
    return T(python::detail::borrowed_reference(m_source));
}

template <class Ref>
inline object_manager_ref_arg_from_python<Ref>::object_manager_ref_arg_from_python(PyObject* x)
{
# if defined(__EDG_VERSION__) && __EDG_VERSION__ <= 243
    // needed for warning suppression
    python::detail::borrowed_reference x_ = python::detail::borrowed_reference(x);
    python::detail::construct_referent<Ref>(m_result.bytes, x_);
# else 
    python::detail::construct_referent<Ref>(m_result.bytes, (python::detail::borrowed_reference)x);
# endif 
}

template <class Ref>
inline object_manager_ref_arg_from_python<Ref>::~object_manager_ref_arg_from_python()
{
    python::detail::destroy_referent<Ref>(this->m_result.bytes);
}

namespace detail
{
  template <class T>
  inline bool object_manager_ref_check(T const& x)
  {
      return object_manager_traits<T>::check(get_managed_object(x, tag));
  }
}

template <class Ref>
inline bool object_manager_ref_arg_from_python<Ref>::convertible() const
{
    return detail::object_manager_ref_check(
        python::detail::void_ptr_to_reference(this->m_result.bytes, (Ref(*)())0));
}

template <class Ref>
inline Ref object_manager_ref_arg_from_python<Ref>::operator()() const
{
    return python::detail::void_ptr_to_reference(
        this->m_result.bytes, (Ref(*)())0);
}

}}} // namespace boost::python::converter

#endif // OBJ_MGR_ARG_FROM_PYTHON_DWA2002628_HPP
