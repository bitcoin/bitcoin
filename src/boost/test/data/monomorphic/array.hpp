//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
///@file
///Defines monomorphic dataset based on C type arrays
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_ARRAY_HPP_121411GER
#define BOOST_TEST_DATA_MONOMORPHIC_ARRAY_HPP_121411GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/monomorphic/fwd.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

// ************************************************************************** //
// **************                     array                    ************** //
// ************************************************************************** //

/// Dataset view of a C array
template<typename T>
class array {
public:
    typedef T sample;

    enum { arity = 1 };

    typedef T const* iterator;

    // Constructor
    array( T const* arr_, std::size_t size_ )
    : m_arr( arr_ )
    , m_size( size_ )
    {}

    // dataset interface
    data::size_t    size() const    { return m_size; }
    iterator        begin() const   { return m_arr; }

private:
    // Data members
    T const*        m_arr;
    std::size_t     m_size;
};

//____________________________________________________________________________//

//! An array dataset is a dataset
template<typename T>
struct is_dataset<array<T>> : mpl::true_ {};

} // namespace monomorphic

//____________________________________________________________________________//

//! @overload boost::unit_test::data::make()
template<typename T, std::size_t size>
inline monomorphic::array<typename boost::remove_const<T>::type>
make( T (&a)[size] )
{
    return monomorphic::array<typename boost::remove_const<T>::type>( a, size );
}

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_ARRAY_HPP_121411GER

