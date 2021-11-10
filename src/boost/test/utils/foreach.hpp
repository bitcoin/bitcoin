//  (C) Copyright Eric Niebler 2004-2005
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
//  Description : this is an abridged version of an excelent BOOST_FOREACH facility
//  presented by Eric Niebler. I am so fond of it so I can't wait till it
//  going to be accepted into Boost. Also I need version with less number of dependencies
//  and more portable. This version doesn't support rvalues and will reeveluate it's
//  parameters, but should be good enough for my purposes.
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_FOREACH_HPP
#define BOOST_TEST_UTILS_FOREACH_HPP

// Boost.Test
#include <boost/test/detail/config.hpp>

// Boost
#include <boost/type.hpp>
#include <boost/mpl/bool.hpp>

#include <boost/type_traits/is_const.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace for_each {

// ************************************************************************** //
// **************                  static_any                  ************** //
// ************************************************************************** //

struct static_any_base
{
    operator bool() const { return false; }
};

//____________________________________________________________________________//

template<typename Iter>
struct static_any : static_any_base
{
    static_any( Iter const& t ) : m_it( t ) {}

    mutable Iter m_it;
};

//____________________________________________________________________________//

typedef static_any_base const& static_any_t;

//____________________________________________________________________________//

template<typename Iter>
inline Iter&
static_any_cast( static_any_t a, Iter* = 0 )
{
    return static_cast<Iter&>( static_cast<static_any<Iter> const&>( a ).m_it );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                   is_const                   ************** //
// ************************************************************************** //

template<typename C>
inline is_const<C>
is_const_coll( C& )
{
    return is_const<C>();
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                     begin                    ************** //
// ************************************************************************** //

template<typename C>
inline static_any<BOOST_DEDUCED_TYPENAME C::iterator>
begin( C& t, mpl::false_ )
{
    return static_any<BOOST_DEDUCED_TYPENAME C::iterator>( t.begin() );
}

//____________________________________________________________________________//

template<typename C>
inline static_any<BOOST_DEDUCED_TYPENAME C::const_iterator>
begin( C const& t, mpl::true_ )
{
    return static_any<BOOST_DEDUCED_TYPENAME C::const_iterator>( t.begin() );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                      end                     ************** //
// ************************************************************************** //

template<typename C>
inline static_any<BOOST_DEDUCED_TYPENAME C::iterator>
end( C& t, mpl::false_ )
{
    return static_any<BOOST_DEDUCED_TYPENAME C::iterator>( t.end() );
}

//____________________________________________________________________________//

template<typename C>
inline static_any<BOOST_DEDUCED_TYPENAME C::const_iterator>
end( C const& t, mpl::true_ )
{
    return static_any<BOOST_DEDUCED_TYPENAME C::const_iterator>( t.end() );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                      done                    ************** //
// ************************************************************************** //

template<typename C>
inline bool
done( static_any_t cur, static_any_t end, C&, mpl::false_ )
{
    return  static_any_cast<BOOST_DEDUCED_TYPENAME C::iterator>( cur ) ==
            static_any_cast<BOOST_DEDUCED_TYPENAME C::iterator>( end );
}

//____________________________________________________________________________//

template<typename C>
inline bool
done( static_any_t cur, static_any_t end, C const&, mpl::true_ )
{
    return  static_any_cast<BOOST_DEDUCED_TYPENAME C::const_iterator>( cur ) ==
            static_any_cast<BOOST_DEDUCED_TYPENAME C::const_iterator>( end );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                      next                    ************** //
// ************************************************************************** //

template<typename C>
inline void
next( static_any_t cur, C&, mpl::false_ )
{
    ++static_any_cast<BOOST_DEDUCED_TYPENAME C::iterator>( cur );
}

//____________________________________________________________________________//

template<typename C>
inline void
next( static_any_t cur, C const&, mpl::true_ )
{
    ++static_any_cast<BOOST_DEDUCED_TYPENAME C::const_iterator>( cur );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                      prev                    ************** //
// ************************************************************************** //

template<typename C>
inline void
prev( static_any_t cur, C&, mpl::false_ )
{
    --static_any_cast<BOOST_DEDUCED_TYPENAME C::iterator>( cur );
}

//____________________________________________________________________________//

template<typename C>
inline void
prev( static_any_t cur, C const&, mpl::true_ )
{
    --static_any_cast<BOOST_DEDUCED_TYPENAME C::const_iterator>( cur );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                      deref                   ************** //
// ************************************************************************** //

template<class RefType,typename C>
inline RefType
deref( static_any_t cur, C&, ::boost::type<RefType>, mpl::false_ )
{
    return *static_any_cast<BOOST_DEDUCED_TYPENAME C::iterator>( cur );
}

//____________________________________________________________________________//

template<class RefType,typename C>
inline RefType
deref( static_any_t cur, C const&, ::boost::type<RefType>, mpl::true_ )
{
    return *static_any_cast<BOOST_DEDUCED_TYPENAME C::const_iterator>( cur );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************              BOOST_TEST_FOREACH              ************** //
// ************************************************************************** //

#define BOOST_TEST_FE_ANY                   ::boost::unit_test::for_each::static_any_t
#define BOOST_TEST_FE_IS_CONST( COL )       ::boost::unit_test::for_each::is_const_coll( COL )

#define BOOST_TEST_FE_BEG( COL )            \
    ::boost::unit_test::for_each::begin(    \
        COL,                                \
        BOOST_TEST_FE_IS_CONST( COL ) )     \
/**/

#define BOOST_TEST_FE_END( COL )            \
    ::boost::unit_test::for_each::end(      \
        COL,                                \
        BOOST_TEST_FE_IS_CONST( COL ) )     \
/**/

#define BOOST_TEST_FE_DONE( COL )           \
    ::boost::unit_test::for_each::done(     \
        BOOST_TEST_FE_CUR_VAR,              \
        BOOST_TEST_FE_END_VAR,              \
        COL,                                \
        BOOST_TEST_FE_IS_CONST( COL ) )     \
/**/

#define BOOST_TEST_FE_NEXT( COL )           \
    ::boost::unit_test::for_each::next(     \
        BOOST_TEST_FE_CUR_VAR,              \
        COL,                                \
        BOOST_TEST_FE_IS_CONST( COL ) )     \
/**/

#define BOOST_TEST_FE_PREV( COL )           \
    ::boost::unit_test::for_each::prev(     \
        BOOST_TEST_FE_CUR_VAR,              \
        COL,                                \
        BOOST_TEST_FE_IS_CONST( COL ) )     \
/**/

#define BOOST_FOREACH_NOOP(COL)             \
    ((void)&(COL))

#define BOOST_TEST_FE_DEREF( COL, RefType ) \
    ::boost::unit_test::for_each::deref(    \
        BOOST_TEST_FE_CUR_VAR,              \
        COL,                                \
        ::boost::type<RefType >(),          \
        BOOST_TEST_FE_IS_CONST( COL ) )     \
/**/

#if BOOST_WORKAROUND( BOOST_MSVC, == 1310 )
#define BOOST_TEST_LINE_NUM
#else
#define BOOST_TEST_LINE_NUM     __LINE__
#endif

#define BOOST_TEST_FE_CUR_VAR   BOOST_JOIN( _fe_cur_, BOOST_TEST_LINE_NUM )
#define BOOST_TEST_FE_END_VAR   BOOST_JOIN( _fe_end_, BOOST_TEST_LINE_NUM )
#define BOOST_TEST_FE_CON_VAR   BOOST_JOIN( _fe_con_, BOOST_TEST_LINE_NUM )

#define BOOST_TEST_FOREACH( RefType, var, COL )                                             \
if( BOOST_TEST_FE_ANY BOOST_TEST_FE_CUR_VAR = BOOST_TEST_FE_BEG( COL ) ) {} else            \
if( BOOST_TEST_FE_ANY BOOST_TEST_FE_END_VAR = BOOST_TEST_FE_END( COL ) ) {} else            \
for( bool BOOST_TEST_FE_CON_VAR = true;                                                     \
          BOOST_TEST_FE_CON_VAR && !BOOST_TEST_FE_DONE( COL );                              \
          BOOST_TEST_FE_CON_VAR ? BOOST_TEST_FE_NEXT( COL ) : BOOST_FOREACH_NOOP( COL ))    \
                                                                                            \
    if( (BOOST_TEST_FE_CON_VAR = false, false) ) {} else                                    \
    for( RefType var = BOOST_TEST_FE_DEREF( COL, RefType );                                 \
         !BOOST_TEST_FE_CON_VAR; BOOST_TEST_FE_CON_VAR = true )                             \
/**/

#define BOOST_TEST_REVERSE_FOREACH( RefType, var, COL )                                     \
if( BOOST_TEST_FE_ANY BOOST_TEST_FE_CUR_VAR = BOOST_TEST_FE_END( COL ) ) {} else            \
if( BOOST_TEST_FE_ANY BOOST_TEST_FE_END_VAR = BOOST_TEST_FE_BEG( COL ) ) {} else            \
for( bool BOOST_TEST_FE_CON_VAR = true;                                                     \
          BOOST_TEST_FE_CON_VAR && !BOOST_TEST_FE_DONE( COL ); )                            \
                                                                                            \
    if( (BOOST_TEST_FE_CON_VAR = false, false) ) {} else                                    \
    if( (BOOST_TEST_FE_PREV( COL ), false) ) {} else                                        \
    for( RefType var = BOOST_TEST_FE_DEREF( COL, RefType );                                 \
         !BOOST_TEST_FE_CON_VAR; BOOST_TEST_FE_CON_VAR = true )                             \
/**/

//____________________________________________________________________________//

} // namespace for_each
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_FOREACH_HPP
