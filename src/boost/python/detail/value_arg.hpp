// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef VALUE_ARG_DWA2004312_HPP
# define VALUE_ARG_DWA2004312_HPP

# include <boost/python/detail/copy_ctor_mutates_rhs.hpp>
# include <boost/mpl/if.hpp>
# include <boost/python/detail/indirect_traits.hpp>

namespace boost { namespace python { namespace detail { 

template <class T>
struct value_arg
  : mpl::if_<
        copy_ctor_mutates_rhs<T>
      , T
      , typename add_lvalue_reference<
            typename add_const<T>::type
        >::type
  >
{};
  
}}} // namespace boost::python::detail

#endif // VALUE_ARG_DWA2004312_HPP
