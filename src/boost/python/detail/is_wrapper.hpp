// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef IS_WRAPPER_DWA2004723_HPP
# define IS_WRAPPER_DWA2004723_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/mpl/bool.hpp>

namespace boost { namespace python {

template <class T> class wrapper;

namespace detail
{
  typedef char (&is_not_wrapper)[2];
  is_not_wrapper is_wrapper_helper(...);
  template <class T>
  char is_wrapper_helper(wrapper<T> const volatile*);

  // A metafunction returning true iff T is [derived from] wrapper<U> 
  template <class T>
  struct is_wrapper
    : mpl::bool_<(sizeof(detail::is_wrapper_helper((T*)0)) == 1)>
  {};

}}} // namespace boost::python::detail

#endif // IS_WRAPPER_DWA2004723_HPP
