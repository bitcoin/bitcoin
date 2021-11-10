//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//! @file
//! Defines dataset join operation
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_JOIN_HPP_112711GER
#define BOOST_TEST_DATA_MONOMORPHIC_JOIN_HPP_112711GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/monomorphic/fwd.hpp>

#include <boost/core/enable_if.hpp>
#include <boost/mpl/identity.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

// ************************************************************************** //
// **************                      join                    ************** //
// ************************************************************************** //

//! Defines a new dataset from the concatenation of two datasets
//!
//! The size of the resulting dataset is the sum of the two underlying datasets. The arity of the datasets
//! should match.
template<typename DataSet1, typename DataSet2>
class join {
    typedef typename boost::decay<DataSet1>::type   dataset1_decay;
    typedef typename boost::decay<DataSet2>::type   dataset2_decay;

    typedef typename dataset1_decay::iterator       dataset1_iter;
    typedef typename dataset2_decay::iterator       dataset2_iter;
  
    using iter1_ret = decltype(*std::declval<DataSet1>().begin());
    using iter2_ret = decltype(*std::declval<DataSet2>().begin());

public:

    enum { arity = dataset1_decay::arity };
  
    using sample_t = typename std::conditional<
        std::is_reference<iter1_ret>::value && std::is_reference<iter2_ret>::value && std::is_same<iter1_ret, iter2_ret>::value,
        iter1_ret,
        typename std::remove_reference<iter1_ret>::type
        >::type
        ;

    struct iterator {
        // Constructor
        explicit    iterator( dataset1_iter&& it1, dataset2_iter&& it2, data::size_t first_size )
        : m_it1( std::move( it1 ) )
        , m_it2( std::move( it2 ) )
        , m_first_size( first_size )
        {}

        // forward iterator interface
        // The returned sample should be by value, as the operator* may return a temporary object
        sample_t     operator*() const   { return m_first_size > 0 ? *m_it1 : *m_it2; }
        void         operator++()        { if( m_first_size > 0 ) { --m_first_size; ++m_it1; } else ++m_it2; }

    private:
        // Data members
        dataset1_iter       m_it1;
        dataset2_iter       m_it2;
        data::size_t        m_first_size;
    };

    //! Constructor 
    join( DataSet1&& ds1, DataSet2&& ds2 )
    : m_ds1( std::forward<DataSet1>( ds1 ) )
    , m_ds2( std::forward<DataSet2>( ds2 ) )
    {}

    //! Move constructor
    join( join&& j )
    : m_ds1( std::forward<DataSet1>( j.m_ds1 ) )
    , m_ds2( std::forward<DataSet2>( j.m_ds2 ) )
    {}

    //! dataset interface
    data::size_t    size() const    { return m_ds1.size() + m_ds2.size(); }
    iterator        begin() const   { return iterator( m_ds1.begin(), m_ds2.begin(), m_ds1.size() ); }

private:
    // Data members
    DataSet1        m_ds1;
    DataSet2        m_ds2;
};

//____________________________________________________________________________//

// A joined dataset  is a dataset.
template<typename DataSet1, typename DataSet2>
struct is_dataset<join<DataSet1,DataSet2>> : mpl::true_ {};

//____________________________________________________________________________//

namespace result_of {

//! Result type of the join operation on datasets.
template<typename DataSet1Gen, typename DataSet2Gen>
struct join {
    typedef monomorphic::join<typename DataSet1Gen::type,typename DataSet2Gen::type> type;
};

} // namespace result_of

//____________________________________________________________________________//

template<typename DataSet1, typename DataSet2>
inline typename boost::lazy_enable_if_c<is_dataset<DataSet1>::value && is_dataset<DataSet2>::value,
                                        result_of::join<mpl::identity<DataSet1>,mpl::identity<DataSet2>>
>::type
operator+( DataSet1&& ds1, DataSet2&& ds2 )
{
    return join<DataSet1,DataSet2>( std::forward<DataSet1>( ds1 ),  std::forward<DataSet2>( ds2 ) );
}

//____________________________________________________________________________//

template<typename DataSet1, typename DataSet2>
inline typename boost::lazy_enable_if_c<is_dataset<DataSet1>::value && !is_dataset<DataSet2>::value,
                                        result_of::join<mpl::identity<DataSet1>,data::result_of::make<DataSet2>>
>::type
operator+( DataSet1&& ds1, DataSet2&& ds2 )
{
    return std::forward<DataSet1>( ds1 ) + data::make( std::forward<DataSet2>( ds2 ) );
}

//____________________________________________________________________________//

template<typename DataSet1, typename DataSet2>
inline typename boost::lazy_enable_if_c<!is_dataset<DataSet1>::value && is_dataset<DataSet2>::value,
                                        result_of::join<data::result_of::make<DataSet1>,mpl::identity<DataSet2>>
>::type
operator+( DataSet1&& ds1, DataSet2&& ds2 )
{
    return data::make( std::forward<DataSet1>(ds1) ) + std::forward<DataSet2>( ds2 );
}

} // namespace monomorphic
} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_JOIN_HPP_112711GER

