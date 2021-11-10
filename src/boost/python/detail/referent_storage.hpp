// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef REFERENT_STORAGE_DWA200278_HPP
# define REFERENT_STORAGE_DWA200278_HPP
# include <boost/mpl/if.hpp>
# include <boost/type_traits/aligned_storage.hpp>
# include <cstddef>

namespace boost { namespace python { namespace detail {

template <std::size_t size, std::size_t alignment = std::size_t(-1)>
struct aligned_storage
{
  union type
  {
    typename ::boost::aligned_storage<size, alignment>::type data;
    char bytes[size];
  };
};
      
  // Compute the size of T's referent. We wouldn't need this at all,
  // but sizeof() is broken in CodeWarriors <= 8.0
  template <class T> struct referent_size;
  
  
  template <class T>
  struct referent_size<T&>
  {
      BOOST_STATIC_CONSTANT(
          std::size_t, value = sizeof(T));
  };

// A metafunction returning a POD type which can store U, where T ==
// U&. If T is not a reference type, returns a POD which can store T.
template <class T>
struct referent_storage
{
    typedef typename aligned_storage<referent_size<T>::value, alignment_of<T>::value>::type type;
};

}}} // namespace boost::python::detail

#endif // REFERENT_STORAGE_DWA200278_HPP
