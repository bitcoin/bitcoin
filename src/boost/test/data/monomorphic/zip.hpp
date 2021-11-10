//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines monomorphic dataset based on zipping of 2 other monomorphic datasets
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_ZIP_HPP_102211GER
#define BOOST_TEST_DATA_MONOMORPHIC_ZIP_HPP_102211GER

// Boost.Test
#include <boost/test/data/config.hpp>

#if !defined(BOOST_TEST_NO_ZIP_COMPOSITION_AVAILABLE) || defined(BOOST_TEST_DOXYGEN_DOC__)

#include <boost/test/data/monomorphic/fwd.hpp>
#include <boost/test/data/monomorphic/sample_merge.hpp>

#include <boost/core/enable_if.hpp>
#include <boost/mpl/identity.hpp>

#include <boost/test/detail/suppress_warnings.hpp>


namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

// ************************************************************************** //
// **************                       zip                    ************** //
// ************************************************************************** //

//! Zip datasets
//!
//! A zip of two datasets is a dataset whose arity is the sum of the operand datasets arity. The size is given by
//! the function creating the instance (see @c operator^ on datasets).
template<typename DataSet1, typename DataSet2>
class zip {
    typedef typename boost::decay<DataSet1>::type   dataset1_decay;
    typedef typename boost::decay<DataSet2>::type   dataset2_decay;

    typedef typename dataset1_decay::iterator       dataset1_iter;
    typedef typename dataset2_decay::iterator       dataset2_iter;

public:
    enum { arity = dataset1_decay::arity + dataset2_decay::arity };

    struct iterator {
        // Constructor
        explicit    iterator( dataset1_iter iter1, dataset2_iter iter2 )
        : m_iter1( std::move( iter1 ) )
        , m_iter2( std::move( iter2 ) )
        {}

        using iterator_sample = decltype(
            sample_merge( *std::declval<dataset1_iter>(),
                          *std::declval<dataset2_iter>()) );

        // forward iterator interface
        auto            operator*() const -> iterator_sample {
            return sample_merge( *m_iter1, *m_iter2 );
        }
        void            operator++()        { ++m_iter1; ++m_iter2; }

    private:
        // Data members
        dataset1_iter   m_iter1;
        dataset2_iter   m_iter2;
    };

    //! Constructor
    //!
    //! The datasets are moved and not copied.
    zip( DataSet1&& ds1, DataSet2&& ds2/*, data::size_t size*/ )
    : m_ds1( std::forward<DataSet1>( ds1 ) )
    , m_ds2( std::forward<DataSet2>( ds2 ) )
    //, m_size( size )
    {}

    //! Move constructor
    zip( zip&& j )
    : m_ds1( std::forward<DataSet1>( j.m_ds1 ) )
    , m_ds2( std::forward<DataSet2>( j.m_ds2 ) )
    //, m_size( j.m_size )
    {}

    // dataset interface
    data::size_t    size() const    { return zip_size(); }
    iterator        begin() const   { return iterator( m_ds1.begin(), m_ds2.begin() ); }

private:
    // Data members
    DataSet1        m_ds1;
    DataSet2        m_ds2;
    //data::size_t    m_size;
  
  
    //! Handles the sise of the resulting zipped dataset.
    data::size_t zip_size() const
    {
        data::size_t ds1_size = m_ds1.size();
        data::size_t ds2_size = m_ds2.size();

        if( ds1_size == ds2_size )
            return ds1_size;

        if( ds1_size == 1 || ds1_size.is_inf() )
            return ds2_size;

        if( ds2_size == 1  || ds2_size.is_inf() )
            return ds1_size;

        BOOST_TEST_DS_ERROR( "Can't zip datasets of different sizes" );
    }
};

//____________________________________________________________________________//

//! Zipped datasets results in a dataset.
template<typename DataSet1, typename DataSet2>
struct is_dataset<zip<DataSet1,DataSet2>> : mpl::true_ {};

//____________________________________________________________________________//

namespace result_of {

//! Result type of the zip operator.
template<typename DS1Gen, typename DS2Gen>
struct zip {
    typedef monomorphic::zip<typename DS1Gen::type,typename DS2Gen::type> type;
};

} // namespace result_of

//____________________________________________________________________________//

//! Overload operator for zip support
template<typename DataSet1, typename DataSet2>
inline typename boost::lazy_enable_if_c<is_dataset<DataSet1>::value && is_dataset<DataSet2>::value,
                                        result_of::zip<mpl::identity<DataSet1>,mpl::identity<DataSet2>>
>::type
operator^( DataSet1&& ds1, DataSet2&& ds2 )
{
    return zip<DataSet1,DataSet2>( std::forward<DataSet1>( ds1 ),
                                   std::forward<DataSet2>( ds2 )/*,
                                   ds_detail::zip_size( ds1, ds2 )*/ );
}

//____________________________________________________________________________//

//! @overload boost::unit_test::data::monomorphic::operator^()
template<typename DataSet1, typename DataSet2>
inline typename boost::lazy_enable_if_c<is_dataset<DataSet1>::value && !is_dataset<DataSet2>::value,
                                        result_of::zip<mpl::identity<DataSet1>,data::result_of::make<DataSet2>>
>::type
operator^( DataSet1&& ds1, DataSet2&& ds2 )
{
    return std::forward<DataSet1>( ds1 ) ^ data::make( std::forward<DataSet2>( ds2 ) );
}

//____________________________________________________________________________//

//! @overload boost::unit_test::data::monomorphic::operator^()
template<typename DataSet1, typename DataSet2>
inline typename boost::lazy_enable_if_c<!is_dataset<DataSet1>::value && is_dataset<DataSet2>::value,
                                        result_of::zip<data::result_of::make<DataSet1>,mpl::identity<DataSet2>>
>::type
operator^( DataSet1&& ds1, DataSet2&& ds2 )
{
    return data::make( std::forward<DataSet1>( ds1 ) ) ^ std::forward<DataSet2>( ds2 );
}

} // namespace monomorphic
} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_NO_ZIP_COMPOSITION_AVAILABLE

#endif // BOOST_TEST_DATA_MONOMORPHIC_ZIP_HPP_102211GER

