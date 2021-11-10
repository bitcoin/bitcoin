//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief simple dataset size abstraction (can be infinite)
// ***************************************************************************

#ifndef BOOST_TEST_DATA_SIZE_HPP_102211GER
#define BOOST_TEST_DATA_SIZE_HPP_102211GER

// Boost.Test
#include <boost/test/data/config.hpp>

// STL
#include <iosfwd>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {

// ************************************************************************** //
// **************                    size_t                    ************** //
// ************************************************************************** //

//! Utility for handling the size of a datasets
class size_t {
    struct dummy { void nonnull() {} };
    typedef void (dummy::*safe_bool)();
public:
    // Constructors
    size_t( std::size_t s = 0 )         : m_value( s ), m_infinity( false ) {}
    explicit        size_t( bool )      : m_value( 0 ), m_infinity( true ) {}
    template<typename T>
    size_t( T v )                       : m_value( static_cast<std::size_t>(v) ), m_infinity( false ) {}

    // Access methods
    std::size_t     value() const       { return m_value; }
    bool            is_inf() const      { return m_infinity; }
    operator        safe_bool() const   { return is_inf() || m_value != 0 ? &dummy::nonnull : 0; }

    // Unary operators
    data::size_t    operator--()        { if( !is_inf() ) m_value--; return *this; }
    data::size_t    operator--(int)     { data::size_t res(*this); if( !is_inf() ) m_value--; return res; }
    data::size_t    operator++()        { if( !is_inf() ) m_value++; return *this; }
    data::size_t    operator++(int)     { data::size_t res(*this); if( !is_inf() ) m_value++; return res; }

    // Binary operators
    data::size_t&   operator+=( std::size_t rhs ) { if( !is_inf() ) m_value += rhs; return *this; }
    data::size_t&   operator+=( data::size_t rhs )
    {
        if( !is_inf() ) {
            if( rhs.is_inf() )
                *this = rhs;
            else
                m_value += rhs.value();
        }
        return *this;
    }
    data::size_t&   operator-=( std::size_t rhs ) { if( !is_inf() ) m_value -= rhs; return *this; }
    data::size_t&   operator-=( data::size_t rhs )
    {
        if( !is_inf() ) {
            if( value() < rhs.value() )
                m_value = 0;
            else
                m_value -= rhs.value();
        }
        return *this;
    }

private:
    // Data members
    std::size_t     m_value;
    bool            m_infinity;
};

namespace { const data::size_t BOOST_TEST_DS_INFINITE_SIZE( true ); }

//____________________________________________________________________________//

// Binary operators
inline bool operator>(data::size_t lhs, std::size_t rhs)    { return lhs.is_inf()  || (lhs.value() > rhs); }
inline bool operator>(std::size_t lhs, data::size_t rhs)    { return !rhs.is_inf() && (lhs > rhs.value()); }
inline bool operator>(data::size_t lhs, data::size_t rhs)   { return lhs.is_inf() ^ rhs.is_inf() ? lhs.is_inf() : lhs.value() > rhs.value(); }

inline bool operator>=(data::size_t lhs, std::size_t rhs )  { return lhs.is_inf()  || (lhs.value() >= rhs); }
inline bool operator>=(std::size_t lhs, data::size_t rhs)   { return !rhs.is_inf() && (lhs >= rhs.value()); }
inline bool operator>=(data::size_t lhs, data::size_t rhs)  { return lhs.is_inf() ^ rhs.is_inf() ? lhs.is_inf() : lhs.value() >= rhs.value(); }

inline bool operator<(data::size_t lhs, std::size_t rhs)    { return !lhs.is_inf() && (lhs.value() < rhs); }
inline bool operator<(std::size_t lhs, data::size_t rhs)    { return rhs.is_inf()  || (lhs < rhs.value()); }
inline bool operator<(data::size_t lhs, data::size_t rhs)   { return lhs.is_inf() ^ rhs.is_inf() ? rhs.is_inf() : lhs.value() < rhs.value(); }

inline bool operator<=(data::size_t lhs, std::size_t rhs)   { return !lhs.is_inf() && (lhs.value() <= rhs); }
inline bool operator<=(std::size_t lhs, data::size_t rhs)   { return rhs.is_inf()  || (lhs <= rhs.value()); }
inline bool operator<=(data::size_t lhs, data::size_t rhs)  { return lhs.is_inf() ^ rhs.is_inf() ? rhs.is_inf() : lhs.value() <= rhs.value(); }

inline bool operator==(data::size_t lhs, std::size_t rhs)   { return !lhs.is_inf() && (lhs.value() == rhs); }
inline bool operator==(std::size_t lhs, data::size_t rhs)   { return !rhs.is_inf() && (lhs == rhs.value()); }
inline bool operator==(data::size_t lhs, data::size_t rhs)  { return !(lhs.is_inf() ^ rhs.is_inf()) && lhs.value() == rhs.value(); }

inline bool operator!=(data::size_t lhs, std::size_t rhs)   { return lhs.is_inf() || (lhs.value() != rhs); }
inline bool operator!=(std::size_t lhs, data::size_t rhs)   { return rhs.is_inf() || (lhs != rhs.value()); }
inline bool operator!=(data::size_t lhs, data::size_t rhs)  { return lhs.is_inf() ^ rhs.is_inf() || lhs.value() != rhs.value(); }

inline data::size_t operator+(data::size_t lhs, std::size_t rhs)  { return lhs.is_inf() ? lhs : data::size_t( lhs.value()+rhs ); }
inline data::size_t operator+(std::size_t lhs, data::size_t rhs)  { return rhs.is_inf() ? rhs : data::size_t( lhs+rhs.value() ); }
inline data::size_t operator+(data::size_t lhs, data::size_t rhs) { return lhs.is_inf() || rhs.is_inf() ? data::size_t(true) : data::size_t( lhs.value()+rhs.value() ); }

inline data::size_t operator*(data::size_t lhs, std::size_t rhs)  { return lhs.is_inf() ? lhs : data::size_t( lhs.value()*rhs ); }
inline data::size_t operator*(std::size_t lhs, data::size_t rhs)  { return rhs.is_inf() ? rhs : data::size_t( lhs*rhs.value() ); }
inline data::size_t operator*(data::size_t lhs, data::size_t rhs) { return lhs.is_inf() || rhs.is_inf() ? data::size_t(true) : data::size_t( lhs.value()*rhs.value() ); }

//____________________________________________________________________________//

template<typename CharT1, typename Tr>
inline std::basic_ostream<CharT1,Tr>&
operator<<( std::basic_ostream<CharT1,Tr>& os, data::size_t const& s )
{
    if( s.is_inf() )
        os << "infinity";
    else
        os << s.value();

    return os;
}

//____________________________________________________________________________//

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_SIZE_HPP_102211GER

