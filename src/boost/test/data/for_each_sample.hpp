//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines for_each_sample algorithm
// ***************************************************************************

#ifndef BOOST_TEST_DATA_FOR_EACH_SAMPLE_HPP_102211GER
#define BOOST_TEST_DATA_FOR_EACH_SAMPLE_HPP_102211GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/size.hpp>
#include <boost/test/data/index_sequence.hpp>
#include <boost/test/data/monomorphic/sample_merge.hpp>
#include <boost/test/data/monomorphic/fwd.hpp>

// STL
#include <tuple>

#include <boost/test/detail/suppress_warnings.hpp>

// needed for std::min
#include <algorithm>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {

// ************************************************************************** //
// **************              data::invoke_action             ************** //
// ************************************************************************** //

template<typename Action, typename T>
inline void
invoke_action( Action const& action, T && arg, std::false_type /* is_tuple */ )
{
    action( std::forward<T>(arg) );
}

//____________________________________________________________________________//

template<typename Action, typename T, std::size_t ...I>
inline void
invoke_action_impl( Action const& action,
                    T && args,
                    index_sequence<I...> const& )
{
    action( std::get<I>(std::forward<T>(args))... );
}

//____________________________________________________________________________//

template<typename Action, typename T>
inline void
invoke_action( Action const& action, T&& args, std::true_type /* is_tuple */ )
{
    invoke_action_impl( action,
                        std::forward<T>(args),
                        typename make_index_sequence< 0,
                                                      std::tuple_size<typename std::decay<T>::type>::value
                                                    >::type{} );

}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                for_each_sample               ************** //
// ************************************************************************** //

template<typename DataSet, typename Action>
inline typename std::enable_if<monomorphic::is_dataset<DataSet>::value,void>::type
for_each_sample( DataSet const &    samples,
                 Action const&      act,
                 data::size_t       number_of_samples = BOOST_TEST_DS_INFINITE_SIZE )
{
    data::size_t size = (std::min)( samples.size(), number_of_samples );
    BOOST_TEST_DS_ASSERT( !size.is_inf(), "Dataset has infinite size. Please specify the number of samples" );

    auto it = samples.begin();

    while( size-- > 0 ) {
        invoke_action( act,
                       *it,
                       typename monomorphic::ds_detail::is_tuple<decltype(*it)>::type());
        ++it;
    }
}

//____________________________________________________________________________//

template<typename DataSet, typename Action>
inline typename std::enable_if<!monomorphic::is_dataset<DataSet>::value,void>::type
for_each_sample( DataSet &&     samples,
                 Action const&      act,
                 data::size_t       number_of_samples = BOOST_TEST_DS_INFINITE_SIZE )
{
    data::for_each_sample( data::make( std::forward<DataSet>(samples) ),
                           act,
                           number_of_samples );
}

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_FOR_EACH_SAMPLE_HPP_102211GER

