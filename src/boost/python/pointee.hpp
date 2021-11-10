// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef POINTEE_DWA2002323_HPP
# define POINTEE_DWA2002323_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/detail/type_traits.hpp>

namespace boost { namespace python {

namespace detail
{
  template <bool is_ptr = true>
  struct pointee_impl
  {
      template <class T> struct apply : detail::remove_pointer<T> {};
  };

  template <>
  struct pointee_impl<false>
  {
      template <class T> struct apply
      {
          typedef typename T::element_type type;
      };
  };
}

template <class T>
struct pointee
    : detail::pointee_impl<
        detail::is_pointer<T>::value
      >::template apply<T>
{
};

}} // namespace boost::python

#endif // POINTEE_DWA2002323_HPP
