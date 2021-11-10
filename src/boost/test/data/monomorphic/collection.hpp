//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
///@file
///Defines monomorphic dataset based on forward iterable sequence
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_COLLECTION_HPP_102211GER
#define BOOST_TEST_DATA_MONOMORPHIC_COLLECTION_HPP_102211GER

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
// **************                  collection                  ************** //
// ************************************************************************** //


//!@brief Dataset from a forward iterable container (collection)
//!
//! This dataset is applicable to any container implementing a forward iterator. Note that
//! container with one element will be considered as singletons.
//! This dataset is constructible with the @ref boost::unit_test::data::make function.
template<typename C>
class collection {
    typedef typename boost::decay<C>::type col_type;
public:
    typedef typename col_type::value_type sample;

    enum { arity = 1 };

    typedef typename col_type::const_iterator iterator;

    //! Constructor consumed a temporary collection or stores a reference 
    explicit        collection( C&& col ) : m_col( std::forward<C>(col) ) {}

    //! Move constructor
    collection( collection&& c ) : m_col( std::forward<C>( c.m_col ) ) {}

    //! Returns the underlying collection
    C const&        col() const             { return m_col; }

    //! dataset interface
    data::size_t    size() const            { return m_col.size(); }
    iterator        begin() const           { return m_col.begin(); }

private:
    // Data members
    C               m_col;
};

//____________________________________________________________________________//

//! A collection from a forward iterable container is a dataset.
template<typename C>
struct is_dataset<collection<C>> : mpl::true_ {};

} // namespace monomorphic

//____________________________________________________________________________//

//! @overload boost::unit_test::data::make()
template<typename C>
inline typename std::enable_if<is_container_forward_iterable<C>::value,monomorphic::collection<C>>::type
make( C&& c )
{
    return monomorphic::collection<C>( std::forward<C>(c) );
}

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_COLLECTION_HPP_102211GER

