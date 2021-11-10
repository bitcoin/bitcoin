// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BY_VALUE_DWA20021015_HPP
# define BY_VALUE_DWA20021015_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/to_python_value.hpp>
# include <boost/python/detail/type_traits.hpp>

# include <boost/python/detail/value_arg.hpp>

namespace boost { namespace python { 

struct return_by_value
{
    template <class R>
    struct apply
    {
       typedef to_python_value<
           typename detail::value_arg<R>::type
       > type;
    };
};

}} // namespace boost::python

#endif // BY_VALUE_DWA20021015_HPP
