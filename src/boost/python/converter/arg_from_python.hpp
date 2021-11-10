// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef ARG_FROM_PYTHON_DWA2002127_HPP
# define ARG_FROM_PYTHON_DWA2002127_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/converter/from_python.hpp>
# include <boost/python/detail/indirect_traits.hpp>
# include <boost/python/detail/type_traits.hpp>
# include <boost/python/converter/rvalue_from_python_data.hpp>
# include <boost/mpl/eval_if.hpp>
# include <boost/mpl/if.hpp>
# include <boost/mpl/identity.hpp>
# include <boost/mpl/and.hpp>
# include <boost/mpl/or.hpp>
# include <boost/mpl/not.hpp>
# include <boost/python/converter/registry.hpp>
# include <boost/python/converter/registered.hpp>
# include <boost/python/converter/registered_pointee.hpp>
# include <boost/python/detail/void_ptr.hpp>
# include <boost/python/back_reference.hpp>
# include <boost/python/detail/referent_storage.hpp>
# include <boost/python/converter/obj_mgr_arg_from_python.hpp>

namespace boost { namespace python
{
  template <class T> struct arg_from_python;
}}

// This header defines Python->C++ function argument converters,
// parametrized on the argument type.

namespace boost { namespace python { namespace converter {

//
// lvalue converters
//
//   These require that an lvalue of the type U is stored somewhere in
//   the Python object being converted.

// Used when T == U*const&
template <class T>
struct pointer_cref_arg_from_python
{
    typedef T result_type;
    
    pointer_cref_arg_from_python(PyObject*);
    T operator()() const;
    bool convertible() const;
    
 private: // storage for a U*
    // needed because not all compilers will let us declare U* as the
    // return type of operator() -- we return U*const& instead
    typename python::detail::referent_storage<T>::type m_result;
};

// Base class for pointer and reference converters
struct arg_lvalue_from_python_base
{
 public: // member functions
    arg_lvalue_from_python_base(void* result);
    bool convertible() const;
    
 protected: // member functions
    void*const& result() const;
    
 private: // data members
    void* m_result;
};

// Used when T == U* 
template <class T>
struct pointer_arg_from_python : arg_lvalue_from_python_base
{
    typedef T result_type;
    
    pointer_arg_from_python(PyObject*);
    T operator()() const;
};

// Used when T == U& and (T != V const& or T == W volatile&)
template <class T>
struct reference_arg_from_python : arg_lvalue_from_python_base
{
    typedef T result_type;
    
    reference_arg_from_python(PyObject*);
    T operator()() const;
};

// ===================

//
// rvalue converters
//
//   These require only that an object of type T can be created from
//   the given Python object, but not that the T object exist
//   somewhere in storage.
//

// Used when T is a plain value (non-pointer, non-reference) type or
// a (non-volatile) const reference to a plain value type.
template <class T>
struct arg_rvalue_from_python
{
    typedef typename boost::python::detail::add_lvalue_reference<
        T
        // We can't add_const here, or it would be impossible to pass
        // auto_ptr<U> args from Python to C++
    >::type result_type;
    
    arg_rvalue_from_python(PyObject*);
    bool convertible() const;

# if _MSC_FULL_VER > 13102196
    typename arg_rvalue_from_python<T>::
# endif
    result_type operator()();
    
 private:
    rvalue_from_python_data<result_type> m_data;
    PyObject* m_source;
};


// ==================

// Converts to a (PyObject*,T) bundle, for when you need a reference
// back to the Python object
template <class T>
struct back_reference_arg_from_python
    : boost::python::arg_from_python<typename T::type>
{
    typedef T result_type;
    
    back_reference_arg_from_python(PyObject*);
    T operator()();
 private:
    typedef boost::python::arg_from_python<typename T::type> base;
    PyObject* m_source;
};


// ==================

template <class C, class T, class F>
struct if_2
{
    typedef typename mpl::eval_if<C, mpl::identity<T>, F>::type type;
};

// This metafunction selects the appropriate arg_from_python converter
// type for an argument of type T.
template <class T>
struct select_arg_from_python
{
    typedef typename if_2<
        is_object_manager<T>
      , object_manager_value_arg_from_python<T>
      , if_2<
            is_reference_to_object_manager<T>
          , object_manager_ref_arg_from_python<T>
          , if_2<
                is_pointer<T>
              , pointer_arg_from_python<T>
              , if_2<
                    mpl::and_<
                        indirect_traits::is_reference_to_pointer<T>
                      , indirect_traits::is_reference_to_const<T>
                      , mpl::not_<indirect_traits::is_reference_to_volatile<T> >
                        >
                  , pointer_cref_arg_from_python<T>
                  , if_2<
                        mpl::or_<
                            indirect_traits::is_reference_to_non_const<T>
                          , indirect_traits::is_reference_to_volatile<T>
                        >
                      , reference_arg_from_python<T>
                      , mpl::if_<
                            boost::python::is_back_reference<T>
                          , back_reference_arg_from_python<T>
                          , arg_rvalue_from_python<T>
                        >
                    >
                >
            >
        >
    >::type type;
};

// ==================

//
// implementations
//

// arg_lvalue_from_python_base
//
inline arg_lvalue_from_python_base::arg_lvalue_from_python_base(void* result)
    : m_result(result)
{
}

inline bool arg_lvalue_from_python_base::convertible() const
{
    return m_result != 0;
}

inline void*const& arg_lvalue_from_python_base::result() const
{
    return m_result;
}

// pointer_cref_arg_from_python
//
namespace detail
{
  // null_ptr_reference -- a function returning a reference to a null
  // pointer of type U. Needed so that extractors for T*const& can
  // convert Python's None.
  template <class T>
  struct null_ptr_owner
  {
      static T value;
  };
  template <class T> T null_ptr_owner<T>::value = 0;
  
  template <class U>
  inline U& null_ptr_reference(U&(*)())
  {
      return null_ptr_owner<U>::value;
  }
}

template <class T>
inline pointer_cref_arg_from_python<T>::pointer_cref_arg_from_python(PyObject* p)
{
    // T == U*const&: store a U* in the m_result storage. Nonzero
    // indicates success.  If find returns nonzero, it's a pointer to
    // a U object.
    python::detail::write_void_ptr_reference(
        m_result.bytes
        , p == Py_None ? p : converter::get_lvalue_from_python(p, registered_pointee<T>::converters)
        , (T(*)())0);
}

template <class T>
inline bool pointer_cref_arg_from_python<T>::convertible() const
{
    return python::detail::void_ptr_to_reference(m_result.bytes, (T(*)())0) != 0;
}
template <class T>
inline T pointer_cref_arg_from_python<T>::operator()() const
{
    return (*(void**)m_result.bytes == Py_None)  // None ==> 0
        ? detail::null_ptr_reference((T(*)())0)
        // Otherwise, return a U*const& to the m_result storage.
        : python::detail::void_ptr_to_reference(m_result.bytes, (T(*)())0);
}

// pointer_arg_from_python
//
template <class T>
inline pointer_arg_from_python<T>::pointer_arg_from_python(PyObject* p)
    : arg_lvalue_from_python_base(
        p == Py_None ? p : converter::get_lvalue_from_python(p, registered_pointee<T>::converters))
{
}

template <class T>
inline T pointer_arg_from_python<T>::operator()() const
{
    return (result() == Py_None) ? 0 : T(result());
}

// reference_arg_from_python
//
template <class T>
inline reference_arg_from_python<T>::reference_arg_from_python(PyObject* p)
    : arg_lvalue_from_python_base(converter::get_lvalue_from_python(p,registered<T>::converters))
{
}

template <class T>
inline T reference_arg_from_python<T>::operator()() const
{
    return python::detail::void_ptr_to_reference(result(), (T(*)())0);
}


// arg_rvalue_from_python
//
template <class T>
inline arg_rvalue_from_python<T>::arg_rvalue_from_python(PyObject* obj)
    : m_data(converter::rvalue_from_python_stage1(obj, registered<T>::converters))
    , m_source(obj)
{
}

template <class T>
inline bool arg_rvalue_from_python<T>::convertible() const
{
    return m_data.stage1.convertible != 0;
}

template <class T>
inline typename arg_rvalue_from_python<T>::result_type
arg_rvalue_from_python<T>::operator()()
{
    if (m_data.stage1.construct != 0)
        m_data.stage1.construct(m_source, &m_data.stage1);
    
    return python::detail::void_ptr_to_reference(m_data.stage1.convertible, (result_type(*)())0);
}

// back_reference_arg_from_python
//
template <class T>
back_reference_arg_from_python<T>::back_reference_arg_from_python(PyObject* x)
  : base(x), m_source(x)
{
}

template <class T>
inline T
back_reference_arg_from_python<T>::operator()()
{
    return T(m_source, base::operator()());
}

}}} // namespace boost::python::converter

#endif // ARG_FROM_PYTHON_DWA2002127_HPP
