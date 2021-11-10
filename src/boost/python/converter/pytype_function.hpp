// Copyright David Abrahams 2002,  Nikolay Mladenov 2007.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef WRAP_PYTYPE_NM20070606_HPP
# define WRAP_PYTYPE_NM20070606_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/converter/registered.hpp>
#  include <boost/python/detail/unwind_type.hpp>
#  include <boost/python/detail/type_traits.hpp>


namespace boost { namespace python {

namespace converter
{
template <PyTypeObject const* python_type>
struct wrap_pytype
{
    static PyTypeObject const* get_pytype()
    {
        return python_type;
    }
};

typedef PyTypeObject const* (*pytype_function)();

#ifndef BOOST_PYTHON_NO_PY_SIGNATURES



namespace detail
{
struct unwind_type_id_helper{
    typedef python::type_info result_type;
    template <class U>
    static result_type execute(U* ){
        return python::type_id<U>();
    }
};

template <class T>
inline python::type_info unwind_type_id_(boost::type<T>* = 0, mpl::false_ * =0)
{
    return boost::python::detail::unwind_type<unwind_type_id_helper, T> ();
}

inline python::type_info unwind_type_id_(boost::type<void>* = 0, mpl::true_* =0)
{
    return type_id<void>();
}

template <class T>
inline python::type_info unwind_type_id(boost::type<T>* p= 0)
{
    return unwind_type_id_(p, (mpl::bool_<boost::python::detail::is_void<T>::value >*)0 );
}
}


template <class T>
struct expected_pytype_for_arg
{
    static PyTypeObject const *get_pytype()
    {
        const converter::registration *r=converter::registry::query(
            detail::unwind_type_id_((boost::type<T>*)0, (mpl::bool_<boost::python::detail::is_void<T>::value >*)0 )
            );
        return r ? r->expected_from_python_type(): 0;
    }
};


template <class T>
struct registered_pytype
{
    static PyTypeObject const *get_pytype()
    {
        const converter::registration *r=converter::registry::query(
            detail::unwind_type_id_((boost::type<T>*) 0, (mpl::bool_<boost::python::detail::is_void<T>::value >*)0 )
            );
        return r ? r->m_class_object: 0;
    }
};


template <class T>
struct registered_pytype_direct
{
    static PyTypeObject const* get_pytype()
    {
        return registered<T>::converters.m_class_object;
    }
};

template <class T>
struct expected_from_python_type : expected_pytype_for_arg<T>{};

template <class T>
struct expected_from_python_type_direct
{
    static PyTypeObject const* get_pytype()
    {
        return registered<T>::converters.expected_from_python_type();
    }
};

template <class T>
struct to_python_target_type
{
    static PyTypeObject const *get_pytype()
    {
        const converter::registration *r=converter::registry::query(
            detail::unwind_type_id_((boost::type<T>*)0, (mpl::bool_<boost::python::detail::is_void<T>::value >*)0 )
            );
        return r ? r->to_python_target_type(): 0;
    }
};

template <class T>
struct to_python_target_type_direct
{
    static PyTypeObject const *get_pytype()
    {
        return registered<T>::converters.to_python_target_type();
    }
};
#endif

}}} // namespace boost::python

#endif // WRAP_PYTYPE_NM20070606_HPP
