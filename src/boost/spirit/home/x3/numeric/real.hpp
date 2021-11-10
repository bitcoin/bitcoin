/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_REAL_APRIL_18_2006_0850AM)
#define BOOST_SPIRIT_X3_REAL_APRIL_18_2006_0850AM

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>
#include <boost/spirit/home/x3/numeric/real_policies.hpp>
#include <boost/spirit/home/x3/support/numeric_utils/extract_real.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename T, typename RealPolicies = real_policies<T> >
    struct real_parser : parser<real_parser<T, RealPolicies> >
    {
        typedef T attribute_type;
        static bool const has_attribute = true;

        constexpr real_parser()
        	: policies() {}

        constexpr real_parser(RealPolicies const& policies)
        	: policies(policies) {}

        template <typename Iterator, typename Context>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, T& attr_) const
        {
            x3::skip_over(first, last, context);
            return extract_real<T, RealPolicies>::parse(first, last, attr_, policies);
        }

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, unused_type, Attribute& attr_param) const
        {
            // this case is called when Attribute is not T
            T attr_;
            if (parse(first, last, context, unused, attr_))
            {
                traits::move_to(attr_, attr_param);
                return true;
            }
            return false;
        }

        RealPolicies policies;
    };

    typedef real_parser<float> float_type;
    constexpr float_type float_ = {};

    typedef real_parser<double> double_type;
    constexpr double_type double_ = {};

    typedef real_parser<long double> long_double_type;
    constexpr long_double_type long_double = {};

}}}

#endif
