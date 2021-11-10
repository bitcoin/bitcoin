//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
///@file
///Defines monomorphic dataset based on C++11 initializer_list template
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_INITIALIZATION_LIST_HPP_091515GER
#define BOOST_TEST_DATA_MONOMORPHIC_INITIALIZATION_LIST_HPP_091515GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/monomorphic/fwd.hpp>

#include <boost/core/ignore_unused.hpp>

#include <vector>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

// ************************************************************************** //
// **************                initializer_list              ************** //
// ************************************************************************** //

/// Dataset view from an initializer_list or variadic template arguments
///
/// The data should be stored in the dataset, and since the elements
/// are passed by an @c std::initializer_list , it implies a copy of
/// the elements.
template<typename T>
class init_list {
public:
    enum { arity = 1 };

    typedef typename std::vector<T>::const_iterator iterator;

    //! Constructor copies content of initializer_list
    init_list( std::initializer_list<T> il )
    : m_data( il )
    {}
  
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_TEST_ERRONEOUS_INIT_LIST)
    //! Variadic template initialization
    template <class ...Args>
    init_list( Args&& ... args ) {
      int dummy[] = { 0, (m_data.emplace_back(std::forward<Args&&>(args)), 0)... };
      boost::ignore_unused(dummy);
    }
#endif

    //! dataset interface
    data::size_t    size() const    { return m_data.size(); }
    iterator        begin() const   { return m_data.begin(); }

private:
    // Data members
    std::vector<T> m_data;
};

//! Specialization of init_list for type bool
template <>
class init_list<bool> {
public:
    typedef bool sample;

    enum { arity = 1 };

    //! Constructor copies content of initializer_list
    init_list( std::initializer_list<bool>&& il )
    : m_data( std::forward<std::initializer_list<bool>>( il ) )
    {}
  
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_TEST_ERRONEOUS_INIT_LIST)
    //! Variadic template initialization
    template <class ...Args>
    init_list( Args&& ... args ) : m_data{ args... }
    { }
#endif

    struct non_proxy_iterator {
        std::vector<bool>::const_iterator iterator;
        non_proxy_iterator(std::vector<bool>::const_iterator &&it)
        : iterator(std::forward<std::vector<bool>::const_iterator>(it))
        {}

        bool operator*() const {
            return *iterator;
        }

        non_proxy_iterator& operator++() {
            ++iterator;
            return *this;
        }
    };

    typedef non_proxy_iterator iterator;

    //! dataset interface
    data::size_t    size() const    { return m_data.size(); }
    iterator        begin() const   { return m_data.begin(); }

private:
    // Data members
    std::vector<bool> m_data;
};

//____________________________________________________________________________//

//! An array dataset is a dataset
template<typename T>
struct is_dataset<init_list<T>> : mpl::true_ {};

} // namespace monomorphic

//____________________________________________________________________________//

//! @overload boost::unit_test::data::make()
template<typename T>
inline monomorphic::init_list<T>
make( std::initializer_list<T>&& il )
{
    return monomorphic::init_list<T>( std::forward<std::initializer_list<T>>( il ) );
}

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_TEST_ERRONEOUS_INIT_LIST)
template<class T, class ...Args>
inline typename std::enable_if<
  !monomorphic::has_dataset<T, Args...>::value,
  monomorphic::init_list<T>
>::type
make( T&& arg0, Args&&... args )
{
    return monomorphic::init_list<T>( std::forward<T>(arg0), std::forward<Args>( args )... );
}
#endif


} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_INITIALIZATION_LIST_HPP_091515GER

