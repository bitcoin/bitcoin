/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ITERATOR_MULTI_PASS_FWD_HPP)
#define BOOST_SPIRIT_ITERATOR_MULTI_PASS_FWD_HPP

#include <cstddef>

#include <boost/spirit/home/classic/namespace.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    namespace multi_pass_policies
    {
        class ref_counted;
        class first_owner;
        class buf_id_check;
        class no_check;
        class std_deque;
        template<std::size_t N> class fixed_size_queue;
        class input_iterator;
        class lex_input;
        class functor_input;
    }

    template
    <
        typename InputT,
        typename InputPolicy = multi_pass_policies::input_iterator,
        typename OwnershipPolicy = multi_pass_policies::ref_counted,
        typename CheckingPolicy = multi_pass_policies::buf_id_check,
        typename StoragePolicy = multi_pass_policies::std_deque
    >
    class multi_pass;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

