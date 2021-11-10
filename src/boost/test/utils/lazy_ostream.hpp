//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  Description : contains definition for all test tools in test toolbox
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_LAZY_OSTREAM_HPP
#define BOOST_TEST_UTILS_LAZY_OSTREAM_HPP

// Boost.Test
#include <boost/test/detail/config.hpp>

// STL
#include <iosfwd>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

// ************************************************************************** //
// **************                  lazy_ostream                ************** //
// ************************************************************************** //

namespace boost {
namespace unit_test {

class BOOST_TEST_DECL lazy_ostream {
public:
    virtual                 ~lazy_ostream()                                         {}

    static lazy_ostream&    instance()                                              { return inst; }

    #if !defined(BOOST_EMBTC)
      
    friend std::ostream&    operator<<( std::ostream& ostr, lazy_ostream const& o ) { return o( ostr ); }

    #else
      
    friend std::ostream&    operator<<( std::ostream& ostr, lazy_ostream const& o );

    #endif
      
    // access method
    bool                    empty() const                                           { return m_empty; }

    // actual printing interface; to be accessed only by this class and children
    virtual std::ostream&   operator()( std::ostream& ostr ) const                  { return ostr; }
protected:
    explicit                lazy_ostream( bool p_empty = true ) : m_empty( p_empty )    {}

private:
    // Data members
    bool                    m_empty;
    static lazy_ostream     inst;
};

#if defined(BOOST_EMBTC)

    inline std::ostream&    operator<<( std::ostream& ostr, lazy_ostream const& o ) { return o( ostr ); }

#endif
    
//____________________________________________________________________________//

template<typename PrevType, typename T, typename StorageT=T const&>
class lazy_ostream_impl : public lazy_ostream {
public:
    lazy_ostream_impl( PrevType const& prev, T const& value )
    : lazy_ostream( false )
    , m_prev( prev )
    , m_value( value )
    {
    }

    std::ostream&   operator()( std::ostream& ostr ) const BOOST_OVERRIDE
    {
        return m_prev(ostr) << m_value;
    }
private:
    // Data members
    PrevType const&         m_prev;
    StorageT                m_value;
};

//____________________________________________________________________________//

template<typename T>
inline lazy_ostream_impl<lazy_ostream,T>
operator<<( lazy_ostream const& prev, T const& v )
{
    return lazy_ostream_impl<lazy_ostream,T>( prev, v );
}

//____________________________________________________________________________//

template<typename PrevPrevType, typename TPrev, typename T>
inline lazy_ostream_impl<lazy_ostream_impl<PrevPrevType,TPrev>,T>
operator<<( lazy_ostream_impl<PrevPrevType,TPrev> const& prev, T const& v )
{
    typedef lazy_ostream_impl<PrevPrevType,TPrev> PrevType;
    return lazy_ostream_impl<PrevType,T>( prev, v );
}

//____________________________________________________________________________//

#if BOOST_TEST_USE_STD_LOCALE

template<typename R,typename S>
inline lazy_ostream_impl<lazy_ostream,R& (BOOST_TEST_CALL_DECL *)(S&),R& (BOOST_TEST_CALL_DECL *)(S&)>
operator<<( lazy_ostream const& prev, R& (BOOST_TEST_CALL_DECL *man)(S&) )
{
    typedef R& (BOOST_TEST_CALL_DECL * ManipType)(S&);

    return lazy_ostream_impl<lazy_ostream,ManipType,ManipType>( prev, man );
}

//____________________________________________________________________________//

template<typename PrevPrevType, typename TPrev,typename R,typename S>
inline lazy_ostream_impl<lazy_ostream_impl<PrevPrevType,TPrev>,R& (BOOST_TEST_CALL_DECL *)(S&),R& (BOOST_TEST_CALL_DECL *)(S&)>
operator<<( lazy_ostream_impl<PrevPrevType,TPrev> const& prev, R& (BOOST_TEST_CALL_DECL *man)(S&) )
{
    typedef R& (BOOST_TEST_CALL_DECL * ManipType)(S&);

    return lazy_ostream_impl<lazy_ostream_impl<PrevPrevType,TPrev>,ManipType,ManipType>( prev, man );
}

//____________________________________________________________________________//

#endif

#define BOOST_TEST_LAZY_MSG( M ) (::boost::unit_test::lazy_ostream::instance() << M)

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_LAZY_OSTREAM_HPP
