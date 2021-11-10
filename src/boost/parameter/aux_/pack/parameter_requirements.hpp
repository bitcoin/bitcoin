// Copyright Daniel Wallin, David Abrahams 2005.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PACK_PARAMETER_REQUIREMENTS_HPP
#define BOOST_PARAMETER_AUX_PACK_PARAMETER_REQUIREMENTS_HPP

namespace boost { namespace parameter { namespace aux {

    // Used to pass static information about parameter requirements through
    // the satisfies() overload set (below).  The matched function is never
    // invoked, but its type indicates whether a parameter matches at
    // compile-time.
    template <typename Keyword, typename Predicate, typename HasDefault>
    struct parameter_requirements
    {
        typedef Keyword keyword;
        typedef Predicate predicate;
        typedef HasDefault has_default;
    };
}}} // namespace boost::parameter::aux

#endif  // include guard

