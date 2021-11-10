// Copyright Daniel Wallin 2006.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_USE_DEFAULT_TAG_HPP
#define BOOST_PARAMETER_USE_DEFAULT_TAG_HPP

#include <boost/config.hpp>

namespace boost { namespace parameter { namespace aux {

    struct use_default_tag
    {
        inline BOOST_CONSTEXPR BOOST_DEFAULTED_FUNCTION(use_default_tag(), {})

        inline BOOST_CONSTEXPR BOOST_DEFAULTED_FUNCTION(
            use_default_tag(use_default_tag const&), {}
        )

        inline BOOST_CONSTEXPR use_default_tag operator()() const
        {
            return *this;
        }
    };
}}} // namespace boost::parameter::aux

#endif  // include guard

