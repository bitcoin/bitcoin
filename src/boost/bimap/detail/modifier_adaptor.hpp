// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/modifier_adaptor.hpp
/// \brief A binary to unary functor relation modifier adaptor.

#ifndef BOOST_BIMAP_DETAIL_MODIFIER_ADAPTOR_HPP
#define BOOST_BIMAP_DETAIL_MODIFIER_ADAPTOR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

namespace boost {
namespace bimaps {
namespace detail {

/// \brief A binary to unary functor relation modifier adaptor.

template
<
    class Modifier,
    class NewArgument,
    class FirstExtractor,
    class SecondExtractor 
>
struct relation_modifier_adaptor :
    Modifier,
    FirstExtractor,
    SecondExtractor
{
    typedef NewArgument argument_type;
    typedef void result_type;

    relation_modifier_adaptor( const Modifier & m ) : Modifier(m) {}
    relation_modifier_adaptor( const Modifier & m,
                               const FirstExtractor & fe,
                               const SecondExtractor & se ) :
        Modifier(m), FirstExtractor(fe), SecondExtractor(se) {}

    void operator()( NewArgument & x ) const
    {
        Modifier::operator()(
            FirstExtractor ::operator()( x ),
            SecondExtractor::operator()( x )
        );
    }
};

/// \brief A simple unary modifier adaptor.
// This modifier is equivalent to bind( Modifier, bind( Extractor, _1 ) )
// It may be a good idea to start using Boost.Bind instead of it.

template
<
    class Modifier,
    class NewArgument,
    class Extractor
>
struct unary_modifier_adaptor :
    Modifier,
    Extractor
{
    typedef NewArgument argument_type;
    typedef void result_type;

    unary_modifier_adaptor( const Modifier & m ) : Modifier(m) {}
    unary_modifier_adaptor( const Modifier & m,
                            const Extractor & fe) :
        Modifier(m), Extractor(fe) {}

    void operator()( NewArgument & x ) const
    {
        Modifier::operator()( Extractor::operator()( x ) );
    }
};


} // namespace detail
} // namespace bimap
} // namespace boost


#endif // BOOST_BIMAP_DETAIL_MODIFIER_ADAPTOR_HPP
