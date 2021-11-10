//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : token iterator for string and range tokenization
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_TOKEN_ITERATOR_HPP
#define BOOST_TEST_UTILS_TOKEN_ITERATOR_HPP

// Boost
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_traits.hpp>

#include <boost/test/utils/iterator/input_iterator_facade.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <boost/test/utils/named_params.hpp>
#include <boost/test/utils/foreach.hpp>

// STL
#include <iosfwd>
#include <cctype>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{ using ::ispunct; using ::isspace; }
#endif

namespace boost {
namespace unit_test {
namespace utils {

// ************************************************************************** //
// **************               ti_delimeter_type              ************** //
// ************************************************************************** //

enum ti_delimeter_type {
    dt_char,        // character is delimeter if it among explicit list of some characters
    dt_ispunct,     // character is delimeter if it satisfies ispunct functor
    dt_isspace,     // character is delimeter if it satisfies isspace functor
    dt_none         // no character is delimeter
};

namespace ut_detail {

// ************************************************************************** //
// **************             default_char_compare             ************** //
// ************************************************************************** //

template<typename CharT>
class default_char_compare {
public:
    bool operator()( CharT c1, CharT c2 )
    {
#ifdef BOOST_CLASSIC_IOSTREAMS
        return std::string_char_traits<CharT>::eq( c1, c2 );
#else
        return std::char_traits<CharT>::eq( c1, c2 );
#endif
    }
};

// ************************************************************************** //
// **************                 delim_policy                 ************** //
// ************************************************************************** //

template<typename CharT,typename CharCompare>
class delim_policy {
    typedef basic_cstring<CharT const> cstring;
public:
    // Constructor
    explicit    delim_policy( ti_delimeter_type type_ = dt_char, cstring delimeters_ = cstring() )
    : m_type( type_ )
    {
        set_delimeters( delimeters_ );
    }

    void        set_delimeters( ti_delimeter_type type_ ) { m_type = type_; }
    void        set_delimeters( cstring delimeters_ )
    {
        m_delimeters = delimeters_;

        if( !m_delimeters.is_empty() )
            m_type = dt_char;
    }
    void        set_delimeters( nfp::nil ) {}
    bool        operator()( CharT c )
    {
        switch( m_type ) {
        case dt_char: {
            BOOST_TEST_FOREACH( CharT, delim, m_delimeters )
                if( CharCompare()( delim, c ) )
                    return true;

            return false;
        }
        case dt_ispunct:
            return (std::ispunct)( c ) != 0;
        case dt_isspace:
            return (std::isspace)( c ) != 0;
        case dt_none:
            return false;
        }

        return false;
    }

private:
    // Data members
    cstring             m_delimeters;
    ti_delimeter_type   m_type;
};

// ************************************************************************** //
// **************                 token_assigner               ************** //
// ************************************************************************** //

template<typename TraversalTag>
struct token_assigner {
#if BOOST_WORKAROUND( BOOST_DINKUMWARE_STDLIB, < 306 )
    template<typename Iterator, typename C, typename T>
    static void assign( Iterator b, Iterator e, std::basic_string<C,T>& t )
    { for( ; b != e; ++b ) t += *b; }

    template<typename Iterator, typename C>
    static void assign( Iterator b, Iterator e, basic_cstring<C>& t )  { t.assign( b, e ); }
#else
    template<typename Iterator, typename Token>
    static void assign( Iterator b, Iterator e, Token& t )  { t.assign( b, e ); }
#endif
    template<typename Iterator, typename Token>
    static void append_move( Iterator& b, Token& )          { ++b; }
};

//____________________________________________________________________________//

template<>
struct token_assigner<single_pass_traversal_tag> {
    template<typename Iterator, typename Token>
    static void assign( Iterator /*b*/, Iterator /*e*/, Token& /*t*/ )  {}

    template<typename Iterator, typename Token>
    static void append_move( Iterator& b, Token& t )        { t += *b; ++b; }
};

} // namespace ut_detail

// ************************************************************************** //
// **************                  modifiers                   ************** //
// ************************************************************************** //

namespace {
nfp::keyword<struct dropped_delimeters_t >           dropped_delimeters;
nfp::keyword<struct kept_delimeters_t >              kept_delimeters;
nfp::typed_keyword<bool,struct keep_empty_tokens_t > keep_empty_tokens;
nfp::typed_keyword<std::size_t,struct max_tokens_t > max_tokens;
}

// ************************************************************************** //
// **************             token_iterator_base              ************** //
// ************************************************************************** //

template<typename Derived,
         typename CharT,
         typename CharCompare   = ut_detail::default_char_compare<CharT>,
         typename ValueType     = basic_cstring<CharT const>,
         typename Reference     = basic_cstring<CharT const>,
         typename Traversal     = forward_traversal_tag>
class token_iterator_base
: public input_iterator_facade<Derived,ValueType,Reference,Traversal> {
    typedef basic_cstring<CharT const>                                      cstring;
    typedef ut_detail::delim_policy<CharT,CharCompare>                      delim_policy;
    typedef input_iterator_facade<Derived,ValueType,Reference,Traversal>    base;

protected:
    // Constructor
    explicit    token_iterator_base()
    : m_is_dropped( dt_isspace )
    , m_is_kept( dt_ispunct )
    , m_keep_empty_tokens( false )
    , m_tokens_left( static_cast<std::size_t>(-1) )
    , m_token_produced( false )
    {
    }

    template<typename Modifier>
    void
    apply_modifier( Modifier const& m )
    {
        if( m.has( dropped_delimeters ) )
            m_is_dropped.set_delimeters( m[dropped_delimeters] );

        if( m.has( kept_delimeters ) )
            m_is_kept.set_delimeters( m[kept_delimeters] );

        if( m.has( keep_empty_tokens ) )
            m_keep_empty_tokens = true;

        nfp::opt_assign( m_tokens_left, m, max_tokens );
    }

    template<typename Iter>
    bool                    get( Iter& begin, Iter end )
    {
        typedef ut_detail::token_assigner<BOOST_DEDUCED_TYPENAME iterator_traversal<Iter>::type> Assigner;
        Iter check_point;

        this->m_value.clear();

        if( !m_keep_empty_tokens ) {
            while( begin != end && m_is_dropped( *begin ) )
                ++begin;

            if( begin == end )
                return false;

            check_point = begin;

            if( m_tokens_left == 1 )
                while( begin != end )
                    Assigner::append_move( begin, this->m_value );
            else if( m_is_kept( *begin ) )
                Assigner::append_move( begin, this->m_value );
            else
                while( begin != end && !m_is_dropped( *begin ) && !m_is_kept( *begin ) )
                    Assigner::append_move( begin, this->m_value );

            --m_tokens_left;
        }
        else { // m_keep_empty_tokens is true
            check_point = begin;

            if( begin == end ) {
                if( m_token_produced )
                    return false;

                m_token_produced = true;
            }
            if( m_is_kept( *begin ) ) {
                if( m_token_produced )
                    Assigner::append_move( begin, this->m_value );

                m_token_produced = !m_token_produced;
            }
            else if( !m_token_produced && m_is_dropped( *begin ) )
                m_token_produced = true;
            else {
                if( m_is_dropped( *begin ) )
                    check_point = ++begin;

                while( begin != end && !m_is_dropped( *begin ) && !m_is_kept( *begin ) )
                    Assigner::append_move( begin, this->m_value );

                m_token_produced = true;
            }
        }

        Assigner::assign( check_point, begin, this->m_value );

        return true;
    }

private:
    // Data members
    delim_policy            m_is_dropped;
    delim_policy            m_is_kept;
    bool                    m_keep_empty_tokens;
    std::size_t             m_tokens_left;
    bool                    m_token_produced;
};

// ************************************************************************** //
// **************          basic_string_token_iterator         ************** //
// ************************************************************************** //

template<typename CharT,
         typename CharCompare = ut_detail::default_char_compare<CharT> >
class basic_string_token_iterator
: public token_iterator_base<basic_string_token_iterator<CharT,CharCompare>,CharT,CharCompare> {
    typedef basic_cstring<CharT const> cstring;
    typedef token_iterator_base<basic_string_token_iterator<CharT,CharCompare>,CharT,CharCompare> base;
public:
    explicit    basic_string_token_iterator() {}
    explicit    basic_string_token_iterator( cstring src )
    : m_src( src )
    {
        this->init();
    }

    // warning: making the constructor accept anything else than a cstring should
    // ensure that no temporary object is created during string creation (previous
    // definition was "template<typename Src, typename Modifier> basic_string_token_iterator( Src src ..."
    // which may create a temporary string copy when called with an std::string.
    template<typename Modifier>
    basic_string_token_iterator( cstring src, Modifier const& m )
    : m_src( src )
    {
        this->apply_modifier( m );

        this->init();
    }

private:
    friend class input_iterator_core_access;

    // input iterator implementation
    bool        get()
    {
        typename cstring::iterator begin = m_src.begin();
        bool res = base::get( begin, m_src.end() );

        m_src.assign( begin, m_src.end() );

        return res;
    }

    // Data members
    cstring     m_src;
};

typedef basic_string_token_iterator<char>       string_token_iterator;
typedef basic_string_token_iterator<wchar_t>    wstring_token_iterator;

// ************************************************************************** //
// **************              range_token_iterator            ************** //
// ************************************************************************** //

template<typename Iter,
         typename CharCompare = ut_detail::default_char_compare<BOOST_DEDUCED_TYPENAME iterator_value<Iter>::type>,
         typename ValueType   = std::basic_string<BOOST_DEDUCED_TYPENAME iterator_value<Iter>::type>,
         typename Reference   = ValueType const&>
class range_token_iterator
: public token_iterator_base<range_token_iterator<Iter,CharCompare,ValueType,Reference>,
                             typename iterator_value<Iter>::type,CharCompare,ValueType,Reference> {
    typedef basic_cstring<typename ValueType::value_type> cstring;
    typedef token_iterator_base<range_token_iterator<Iter,CharCompare,ValueType,Reference>,
                                typename iterator_value<Iter>::type,CharCompare,ValueType,Reference> base;
public:
    explicit    range_token_iterator() {}
    explicit    range_token_iterator( Iter begin, Iter end = Iter() )
    : m_begin( begin ), m_end( end )
    {
        this->init();
    }
    range_token_iterator( range_token_iterator const& rhs )
    : base( rhs )
    {
        if( this->m_valid ) {
            m_begin = rhs.m_begin;
            m_end   = rhs.m_end;
        }
    }

    template<typename Modifier>
    range_token_iterator( Iter begin, Iter end, Modifier const& m )
    : m_begin( begin ), m_end( end )
    {
        this->apply_modifier( m );

        this->init();
    }

private:
    friend class input_iterator_core_access;

    // input iterator implementation
    bool        get()
    {
        return base::get( m_begin, m_end );
    }

    // Data members
    Iter m_begin;
    Iter m_end;
};

// ************************************************************************** //
// **************            make_range_token_iterator         ************** //
// ************************************************************************** //

template<typename Iter>
inline range_token_iterator<Iter>
make_range_token_iterator( Iter begin, Iter end = Iter() )
{
    return range_token_iterator<Iter>( begin, end );
}

//____________________________________________________________________________//

template<typename Iter,typename Modifier>
inline range_token_iterator<Iter>
make_range_token_iterator( Iter begin, Iter end, Modifier const& m )
{
    return range_token_iterator<Iter>( begin, end, m );
}

//____________________________________________________________________________//

} // namespace utils
} // namespace unit_test
} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_TOKEN_ITERATOR_HPP

