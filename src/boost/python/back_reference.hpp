// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BACK_REFERENCE_DWA2002510_HPP
# define BACK_REFERENCE_DWA2002510_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/object_fwd.hpp>
# include <boost/python/detail/dependent.hpp>
# include <boost/python/detail/raw_pyobject.hpp>

namespace boost { namespace python { 

template <class T>
struct back_reference
{
 private: // types
    typedef typename detail::dependent<object,T>::type source_t;
 public:
    typedef T type;
    
    back_reference(PyObject*, T);
    source_t const& source() const;
    T get() const;
 private:
    source_t m_source;
    T m_value;
};

template<typename T>
class is_back_reference
{
 public:
    BOOST_STATIC_CONSTANT(bool, value = false); 
};

template<typename T>
class is_back_reference<back_reference<T> >
{
 public:
    BOOST_STATIC_CONSTANT(bool, value = true);
};


//
// implementations
//
template <class T>
back_reference<T>::back_reference(PyObject* p, T x)
    : m_source(detail::borrowed_reference(p))
      , m_value(x)
{
}

template <class T>
typename back_reference<T>::source_t const& back_reference<T>::source() const
{
    return m_source;
}

template <class T>
T back_reference<T>::get() const
{
    return m_value;
}

}} // namespace boost::python

#endif // BACK_REFERENCE_DWA2002510_HPP
