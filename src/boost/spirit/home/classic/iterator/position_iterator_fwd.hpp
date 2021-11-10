/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    Copyright (c) 2002-2006 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_POSITION_ITERATOR_FWD_HPP)
#define BOOST_SPIRIT_POSITION_ITERATOR_FWD_HPP

#include <string>
#include <iterator> // for std::iterator_traits
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/nil.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    template <typename String = std::string> 
    struct file_position_base;
    
    typedef file_position_base<std::string> file_position;

    template <typename String = std::string> 
    struct file_position_without_column_base;

    typedef file_position_without_column_base<std::string> file_position_without_column;

    template <
        typename ForwardIteratorT,
        typename PositionT = file_position_base<
            std::basic_string<
                typename std::iterator_traits<ForwardIteratorT>::value_type
            > 
        >,
        typename SelfT = nil_t
    >
    class position_iterator;

    template
    <
        typename ForwardIteratorT,
        typename PositionT = file_position_base<
            std::basic_string<
                typename std::iterator_traits<ForwardIteratorT>::value_type
            > 
        >
    >
    class position_iterator2;

    template <typename PositionT> class position_policy;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

