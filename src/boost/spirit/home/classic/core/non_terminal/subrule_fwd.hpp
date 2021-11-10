/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_SUBRULE_FWD_HPP)
#define BOOST_SPIRIT_SUBRULE_FWD_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/non_terminal/parser_context.hpp>

namespace boost { namespace spirit  {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    template <int ID, typename ContextT = parser_context<> >
    struct subrule; 

    template <int ID, typename DefT, typename ContextT = parser_context<> >
    struct subrule_parser;

    template <typename ScannerT, typename ListT>
    struct subrules_scanner;

    template <typename FirstT, typename RestT>
    struct subrule_list; 

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

