/*=============================================================================
    Copyright (c) 2002 Juan Carlos Arevalo-Baeza
    Copyright (c) 2002-2006 Hartmut Kaiser
    Copyright (c) 2003 Giovanni Bajo
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_ITERATOR_IMPL_POSITION_ITERATOR_IPP
#define BOOST_SPIRIT_CLASSIC_ITERATOR_IMPL_POSITION_ITERATOR_IPP

#include <boost/config.hpp>
#include <boost/iterator_adaptors.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/spirit/home/classic/core/nil.hpp>  // for nil_t
#include <iterator> // for std::iterator_traits

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  position_policy<file_position_without_column>
//
//  Specialization to handle file_position_without_column. Only take care of
//  newlines since no column tracking is needed.
//
///////////////////////////////////////////////////////////////////////////////
template <typename String>
class position_policy<file_position_without_column_base<String> > {

public:
    void next_line(file_position_without_column_base<String>& pos)
    {
        ++pos.line;
    }

    void set_tab_chars(unsigned int /*chars*/){}
    void next_char(file_position_without_column_base<String>& /*pos*/)    {}
    void tabulation(file_position_without_column_base<String>& /*pos*/)   {}
};

///////////////////////////////////////////////////////////////////////////////
//
//  position_policy<file_position>
//
//  Specialization to handle file_position. Track characters and tabulation
//  to compute the current column correctly.
//
//  Default tab size is 4. You can change this with the set_tabchars member
//  of position_iterator.
//
///////////////////////////////////////////////////////////////////////////////
template <typename String>
class position_policy<file_position_base<String> > {

public:
    position_policy()
        : m_CharsPerTab(4)
    {}

    void next_line(file_position_base<String>& pos)
    {
        ++pos.line;
        pos.column = 1;
    }

    void set_tab_chars(unsigned int chars)
    {
        m_CharsPerTab = chars;
    }

    void next_char(file_position_base<String>& pos)
    {
        ++pos.column;
    }

    void tabulation(file_position_base<String>& pos)
    {
        pos.column += m_CharsPerTab - (pos.column - 1) % m_CharsPerTab;
    }

private:
    unsigned int m_CharsPerTab;
};

/* namespace boost::spirit { */ namespace iterator_ { namespace impl {

template <typename T>
struct make_const : boost::add_const<T>
{};

template <typename T>
struct make_const<T&>
{
    typedef typename boost::add_const<T>::type& type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  position_iterator_base_generator
//
//  Metafunction to generate the iterator type using boost::iterator_adaptors,
//  hiding all the metaprogramming thunking code in it. It is used
//  mainly to keep the public interface (position_iterator) cleanear.
//
///////////////////////////////////////////////////////////////////////////////
template <typename MainIterT, typename ForwardIterT, typename PositionT>
struct position_iterator_base_generator
{
private:
    typedef std::iterator_traits<ForwardIterT> traits;
    typedef typename traits::value_type value_type;
    typedef typename traits::iterator_category iter_category_t;
    typedef typename traits::reference reference;

    // Position iterator is always a non-mutable iterator
    typedef typename boost::add_const<value_type>::type const_value_type;

public:
    // Check if the MainIterT is nil. If it's nil, it means that the actual
    //  self type is position_iterator. Otherwise, it's a real type we
    //  must use
    typedef typename boost::mpl::if_<
        typename boost::is_same<MainIterT, nil_t>::type,
        position_iterator<ForwardIterT, PositionT, nil_t>,
        MainIterT
    >::type main_iter_t;

    typedef boost::iterator_adaptor<
        main_iter_t,
        ForwardIterT,
        const_value_type,
        boost::forward_traversal_tag,
        typename make_const<reference>::type
    > type;
};

}}

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} /* namespace boost::spirit::iterator_::impl */

#endif
