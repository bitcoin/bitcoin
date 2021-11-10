//  Boost.Varaint
//  Contains multivisitors that are implemented via preprocessor magic
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2013-2014.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#ifndef BOOST_VARIANT_DETAIL_MULTIVISITORS_PREPROCESSOR_BASED_HPP
#define BOOST_VARIANT_DETAIL_MULTIVISITORS_PREPROCESSOR_BASED_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/variant.hpp>
#include <boost/bind.hpp>

#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>

#ifndef BOOST_VARAINT_MAX_MULTIVIZITOR_PARAMS
#   define BOOST_VARAINT_MAX_MULTIVIZITOR_PARAMS 4
#endif

namespace boost { 

namespace detail { namespace variant {

    template <class VisitorT, class Visitable1T, class Visitable2T>
    struct two_variables_holder {
    private:
        VisitorT&       visitor_;
        Visitable1T&    visitable1_;
        Visitable2T&    visitable2_;

        // required to suppress warnings and ensure that we do not copy
        // this visitor
        two_variables_holder& operator=(const two_variables_holder&);

    public:
        typedef BOOST_DEDUCED_TYPENAME VisitorT::result_type result_type;

        explicit two_variables_holder(VisitorT& visitor, Visitable1T& visitable1, Visitable2T& visitable2) BOOST_NOEXCEPT 
            : visitor_(visitor)
            , visitable1_(visitable1)
            , visitable2_(visitable2)
        {}

#define BOOST_VARIANT_OPERATOR_BEG()                            \
    return ::boost::apply_visitor(                              \
    ::boost::bind<result_type>(boost::ref(visitor_), _1, _2     \
    /**/

#define BOOST_VARIANT_OPERATOR_END()                            \
    ), visitable1_, visitable2_);                               \
    /**/

#define BOOST_VARANT_VISITORS_VARIABLES_PRINTER(z, n, data)     \
    BOOST_PP_COMMA() boost::ref( BOOST_PP_CAT(vis, n) )         \
    /**/

#define BOOST_VARIANT_VISIT(z, n, data)                                                     \
    template <BOOST_PP_ENUM_PARAMS(BOOST_PP_ADD(n, 1), class VisitableUnwrapped)>           \
    result_type operator()(                                                                 \
        BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ADD(n, 1), VisitableUnwrapped, & vis)          \
    ) const                                                                                 \
    {                                                                                       \
        BOOST_VARIANT_OPERATOR_BEG()                                                        \
        BOOST_PP_REPEAT(BOOST_PP_ADD(n, 1), BOOST_VARANT_VISITORS_VARIABLES_PRINTER, ~)     \
        BOOST_VARIANT_OPERATOR_END()                                                        \
    }                                                                                       \
    /**/

BOOST_PP_REPEAT( BOOST_PP_SUB(BOOST_VARAINT_MAX_MULTIVIZITOR_PARAMS, 2), BOOST_VARIANT_VISIT, ~)
#undef BOOST_VARIANT_OPERATOR_BEG
#undef BOOST_VARIANT_OPERATOR_END
#undef BOOST_VARANT_VISITORS_VARIABLES_PRINTER
#undef BOOST_VARIANT_VISIT

    };

    template <class VisitorT, class Visitable1T, class Visitable2T>
    inline two_variables_holder<VisitorT, Visitable1T, Visitable2T> make_two_variables_holder(
            VisitorT& visitor, Visitable1T& visitable1, Visitable2T& visitable2
        ) BOOST_NOEXCEPT
    {
        return two_variables_holder<VisitorT, Visitable1T, Visitable2T>(visitor, visitable1, visitable2);
    }

    template <class VisitorT, class Visitable1T, class Visitable2T>
    inline two_variables_holder<const VisitorT, Visitable1T, Visitable2T> make_two_variables_holder(
            const VisitorT& visitor, Visitable1T& visitable1, Visitable2T& visitable2
        ) BOOST_NOEXCEPT
    {
        return two_variables_holder<const VisitorT, Visitable1T, Visitable2T>(visitor, visitable1, visitable2);
    }

}} // namespace detail::variant

#define BOOST_VARIANT_APPLY_VISITOR_BEG()                                               \
    return ::boost::apply_visitor(                                                      \
            boost::detail::variant::make_two_variables_holder(visitor, var0 , var1),    \
            var2                                                                        \
    /**/

#define BOOST_VARIANT_APPLY_VISITOR_END()                       \
    );                                                          \
    /**/

#define BOOST_VARANT_VISITORS_VARIABLES_PRINTER(z, n, data)     \
    BOOST_PP_COMMA() BOOST_PP_CAT(var, BOOST_PP_ADD(n, 3))      \
    /**/

#define BOOST_VARIANT_VISIT(z, n, data)                                                                 \
    template <class Visitor BOOST_PP_COMMA() BOOST_PP_ENUM_PARAMS(BOOST_PP_ADD(n, 3), class T)>         \
    inline BOOST_DEDUCED_TYPENAME Visitor::result_type apply_visitor(                                   \
        data BOOST_PP_COMMA() BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ADD(n, 3), T, & var)     \
    )                                                                                                   \
    {                                                                                                   \
        BOOST_VARIANT_APPLY_VISITOR_BEG()                                                               \
        BOOST_PP_REPEAT(n, BOOST_VARANT_VISITORS_VARIABLES_PRINTER, ~)                                  \
        BOOST_VARIANT_APPLY_VISITOR_END()                                                               \
    }                                                                                                   \
    /**/

BOOST_PP_REPEAT( BOOST_PP_SUB(BOOST_VARAINT_MAX_MULTIVIZITOR_PARAMS, 2), BOOST_VARIANT_VISIT, const Visitor& visitor)
BOOST_PP_REPEAT( BOOST_PP_SUB(BOOST_VARAINT_MAX_MULTIVIZITOR_PARAMS, 2), BOOST_VARIANT_VISIT, Visitor& visitor)

#undef BOOST_VARIANT_APPLY_VISITOR_BEG
#undef BOOST_VARIANT_APPLY_VISITOR_END
#undef BOOST_VARANT_VISITORS_VARIABLES_PRINTER
#undef BOOST_VARIANT_VISIT
    
} // namespace boost

#endif // BOOST_VARIANT_DETAIL_MULTIVISITORS_PREPROCESSOR_BASED_HPP

