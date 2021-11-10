// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef VOID_RETURN_DWA200274_HPP
# define VOID_RETURN_DWA200274_HPP

# include <boost/config.hpp>

namespace boost { namespace python { namespace detail { 

struct void_return
{
    void_return() {}
 private: 
    void operator=(void_return const&);
};

template <class T>
struct returnable
{
    typedef T type;
};

# ifdef BOOST_NO_VOID_RETURNS
template <>
struct returnable<void>
{
    typedef void_return type;
};

#  ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
template <> struct returnable<const void> : returnable<void> {};
template <> struct returnable<volatile void> : returnable<void> {};
template <> struct returnable<const volatile void> : returnable<void> {};
#  endif

# endif // BOOST_NO_VOID_RETURNS

}}} // namespace boost::python::detail

#endif // VOID_RETURN_DWA200274_HPP
