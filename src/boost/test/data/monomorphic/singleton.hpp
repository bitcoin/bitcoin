//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines single element monomorphic dataset
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_SINGLETON_HPP_102211GER
#define BOOST_TEST_DATA_MONOMORPHIC_SINGLETON_HPP_102211GER

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
// **************                    singleton                  ************** //
// ************************************************************************** //

/// Models a single element data set
template<typename T>
class singleton {
private:
    typedef typename boost::decay<T>::type sample;

public:

    enum { arity = 1 };

    struct iterator {
        // Constructor
        explicit            iterator( singleton<T> const* owner ) 
        : m_owner( owner ) 
        {}

        // forward iterator interface 
        sample const&       operator*() const   { return m_owner->value(); }
        void                operator++() {}

    private:
        singleton<T> const* m_owner;
    };

    //! Constructor
    explicit        singleton( T&& value ) : m_value( std::forward<T>( value ) ) {}

    //! Move constructor
    singleton( singleton&& s ) : m_value( std::forward<T>( s.m_value ) ) {}

    //! Value access method
    T const&        value() const   { return m_value; }

    //! dataset interface
    data::size_t    size() const    { return 1; }
    iterator        begin() const   { return iterator( this ); }

private:
    // Data members
    T               m_value;
};

//____________________________________________________________________________//

// a singleton is a dataset
template<typename T>
struct is_dataset<singleton<T>> : mpl::true_ {};

//____________________________________________________________________________//

} // namespace monomorphic

/// @overload boost::unit_test::data::make()
template<typename T>
inline typename std::enable_if<!is_container_forward_iterable<T>::value && 
                               !monomorphic::is_dataset<T>::value &&
                               !boost::is_array<typename boost::remove_reference<T>::type>::value, 
                               monomorphic::singleton<T>
>::type
make( T&& v )
{
    return monomorphic::singleton<T>( std::forward<T>( v ) );
}

//____________________________________________________________________________//

/// @overload boost::unit_test::data::make
inline monomorphic::singleton<char*>
make( char* str )
{
    return monomorphic::singleton<char*>( std::move(str) );
}

//____________________________________________________________________________//

/// @overload boost::unit_test::data::make
inline monomorphic::singleton<char const*>
make( char const* str )
{
    return monomorphic::singleton<char const*>( std::move(str) );
}

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_SINGLETON_HPP_102211GER

