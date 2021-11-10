// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_DETAIL_STORAGE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_STORAGE_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/decay.hpp>

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#   include <utility> // for std::forward, std::move
#endif

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4521)
#endif

namespace boost {
namespace type_erasure {
namespace detail {

struct storage
{
    storage() {}
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    storage(storage& other) : data(other.data) {}
    storage(const storage& other) : data(other.data) {}
    storage(storage&& other) : data(other.data) {}
    storage& operator=(const storage& other) { data = other.data; return *this; }
    template<class T>
    explicit storage(T&& arg) : data(new typename boost::decay<T>::type(std::forward<T>(arg))) {}
#else
    template<class T>
    explicit storage(const T& arg) : data(new typename boost::decay<T>::type(arg)) {}
#endif
    void* data;
};


#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
T extract(T arg) { return std::forward<T>(arg); }

#else

template<class T>
T extract(T arg) { return arg; }

#endif

template<class T>
T extract(storage& arg)
{
    return *static_cast<typename ::boost::remove_reference<T>::type*>(arg.data);
}

template<class T>
T extract(const storage& arg)
{
    return *static_cast<const typename ::boost::remove_reference<T>::type*>(arg.data);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
T extract(storage&& arg)
{
    return std::move(*static_cast<typename ::boost::remove_reference<T>::type*>(arg.data));
}

#endif

}
}
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
