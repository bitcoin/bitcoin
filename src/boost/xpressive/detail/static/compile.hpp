///////////////////////////////////////////////////////////////////////////////
// compile.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_COMPILE_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_STATIC_COMPILE_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/bool.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/proto/core.hpp>
#include <boost/xpressive/regex_traits.hpp>
#include <boost/xpressive/detail/core/regex_impl.hpp>
#include <boost/xpressive/detail/core/linker.hpp>
#include <boost/xpressive/detail/core/optimize.hpp>
#include <boost/xpressive/detail/core/adaptor.hpp>
#include <boost/xpressive/detail/core/matcher/end_matcher.hpp>
#include <boost/xpressive/detail/static/static.hpp>
#include <boost/xpressive/detail/static/visitor.hpp>
#include <boost/xpressive/detail/static/grammar.hpp>

namespace boost { namespace xpressive { namespace detail
{

    ///////////////////////////////////////////////////////////////////////////////
    // static_compile_impl2
    template<typename Xpr, typename BidiIter, typename Traits>
    void static_compile_impl2(Xpr const &xpr, shared_ptr<regex_impl<BidiIter> > const &impl, Traits const &tr)
    {
        typedef typename iterator_value<BidiIter>::type char_type;
        impl->tracking_clear();
        impl->traits_ = new traits_holder<Traits>(tr);

        // "compile" the regex and wrap it in an xpression_adaptor.
        typedef xpression_visitor<BidiIter, mpl::false_, Traits> visitor_type;
        visitor_type visitor(tr, impl);
        intrusive_ptr<matchable_ex<BidiIter> const> adxpr = make_adaptor<matchable_ex<BidiIter> >(
            typename Grammar<char_type>::template impl<Xpr const &, end_xpression, visitor_type &>()(
                xpr
              , end_xpression()
              , visitor
            )
        );

        // Link and optimize the regex
        common_compile(adxpr, *impl, visitor.traits());

        // References changed, update dependencies.
        impl->tracking_update();
    }

    ///////////////////////////////////////////////////////////////////////////////
    // pattern for imbued regexes.
    struct XpressiveLocaleModifier
      : proto::binary_expr<
            modifier_tag
          , proto::terminal<locale_modifier<proto::_> >
          , proto::_
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // static_compile_impl1
    template<typename Xpr, typename BidiIter>
    typename disable_if<proto::matches<Xpr, XpressiveLocaleModifier> >::type
    static_compile_impl1(Xpr const &xpr, shared_ptr<regex_impl<BidiIter> > const &impl)
    {
        // use default traits
        typedef typename iterator_value<BidiIter>::type char_type;
        typedef typename default_regex_traits<char_type>::type traits_type;
        traits_type tr;
        static_compile_impl2(xpr, impl, tr);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // static_compile_impl1
    template<typename Xpr, typename BidiIter>
    typename enable_if<proto::matches<Xpr, XpressiveLocaleModifier> >::type
    static_compile_impl1(Xpr const &xpr, shared_ptr<regex_impl<BidiIter> > const &impl)
    {
        // use specified traits
        typedef typename proto::result_of::value<typename proto::result_of::left<Xpr>::type>::type::locale_type locale_type;
        typedef typename regex_traits_type<locale_type, BidiIter>::type traits_type;
        static_compile_impl2(proto::right(xpr), impl, traits_type(proto::value(proto::left(xpr)).getloc()));
    }

    ///////////////////////////////////////////////////////////////////////////////
    // static_compile
    template<typename Xpr, typename BidiIter>
    void static_compile(Xpr const &xpr, shared_ptr<regex_impl<BidiIter> > const &impl)
    {
        static_compile_impl1(xpr, impl);
    }

}}} // namespace boost::xpressive::detail

#endif
