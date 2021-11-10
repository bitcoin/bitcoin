/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_SYMBOLS_FWD_HPP)
#define BOOST_SPIRIT_SYMBOLS_FWD_HPP

#include <boost/spirit/home/classic/namespace.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    namespace impl
    {
        template <typename CharT, typename T>
        class tst;
    }

    template
    <
        typename T = int,
        typename CharT = char,
        typename SetT = impl::tst<T, CharT>
    >
    class symbols;

    template <typename T, typename SetT>
    class symbol_inserter;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

