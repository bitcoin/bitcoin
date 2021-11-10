// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef VALUE_IS_XXX_DWA2003224_HPP
# define VALUE_IS_XXX_DWA2003224_HPP

# include <boost/config.hpp>
# include <boost/mpl/bool.hpp>
# include <boost/preprocessor/enum_params.hpp>

# include <boost/python/detail/type_traits.hpp>
#  include <boost/python/detail/is_xxx.hpp>

namespace boost { namespace python { namespace detail {

#  define BOOST_PYTHON_VALUE_IS_XXX_DEF(name, qualified_name, nargs)    \
template <class X_>                                                     \
struct value_is_##name                                                  \
{                                                                       \
    BOOST_PYTHON_IS_XXX_DEF(name,qualified_name,nargs)                  \
    BOOST_STATIC_CONSTANT(bool, value = is_##name<                      \
                               typename remove_cv<                      \
                                  typename remove_reference<X_>::type   \
                               >::type                                  \
                           >::value);                                   \
    typedef mpl::bool_<value> type;                                     \
                                                                        \
};                                                              

}}} // namespace boost::python::detail

#endif // VALUE_IS_XXX_DWA2003224_HPP
