// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef RETURN_VALUE_POLICY_DWA2002131_HPP
# define RETURN_VALUE_POLICY_DWA2002131_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/default_call_policies.hpp>

namespace boost { namespace python { 

template <class ResultConverterGenerator, class BasePolicy_ = default_call_policies>
struct return_value_policy : BasePolicy_
{
    typedef ResultConverterGenerator result_converter;
};

}} // namespace boost::python

#endif // RETURN_VALUE_POLICY_DWA2002131_HPP
