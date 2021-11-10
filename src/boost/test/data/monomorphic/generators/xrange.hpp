//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
///@file
///Defines range generator
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_GENERATORS_XRANGE_HPP_112011GER
#define BOOST_TEST_DATA_MONOMORPHIC_GENERATORS_XRANGE_HPP_112011GER

// Boost.Test
#include <boost/test/data/config.hpp>

#include <boost/test/data/monomorphic/generators/keywords.hpp>
#include <boost/test/data/monomorphic/generate.hpp>

// Boost
#include <boost/optional.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_unsigned.hpp>

// STL
#include <limits>
#include <cmath>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

// ************************************************************************** //
// **************             monomorphic::xrange_t            ************** //
// ************************************************************************** //


/*!@brief Generator for the range sequences
 *
 * This class implements the generator concept (see @ref boost::unit_test::data::monomorphic::generated_by) for implementing
 * a range like sequence of numbers.
 */
template<typename SampleType, typename StepType=SampleType>
class xrange_t {
public:
    typedef SampleType sample;

    xrange_t( SampleType const& begin_, StepType const& step_, data::size_t size_ )
    : m_begin( begin_ )
    , m_curr( begin_ )
    , m_step( step_ )
    , m_index( 0 )
    , m_size( size_ )
    {}

    // Generator interface
    data::size_t    capacity() const { return m_size; }
    SampleType      next()
    {
        if( m_index == m_size )
            return m_curr;

        SampleType res = m_curr;

        m_curr += m_step;
        ++m_index;

        return res;
    }
    void                reset()
    {
        m_curr  = m_begin;
        m_index = 0;
    }

private:
    // Data members
    SampleType      m_begin;
    SampleType      m_curr;
    StepType        m_step;
    data::size_t    m_index;
    data::size_t    m_size;
};

//____________________________________________________________________________//

namespace ds_detail {

template<typename SampleType, typename StepType=SampleType>
struct make_xrange {
    static StepType    abs( StepType s, boost::true_type* )   { return s; }
    static StepType    abs( StepType s, boost::false_type* )  { return std::abs(s); }

    typedef xrange_t<SampleType, StepType> range_gen;

    template<typename Params>
    static generated_by<range_gen>
    _( Params const& params )
    {
        SampleType           begin_val  = params.has( data::begin )  ? params[data::begin] : SampleType();
        optional<SampleType> end_val    = params.has( data::end )    ? params[data::end]   : optional<SampleType>();
        StepType             step_val   = params.has( data::step )   ? params[data::step]  : 1;

        BOOST_TEST_DS_ASSERT( step_val != 0, "Range step can't be zero" );

        data::size_t size;
        if( !end_val.is_initialized() )
            size = BOOST_TEST_DS_INFINITE_SIZE;
        else {
            BOOST_TEST_DS_ASSERT( (step_val < 0) ^ (begin_val < *end_val), "Invalid step direction" );

            SampleType  abs_distance    = step_val < 0 ? begin_val - *end_val : *end_val-begin_val;
            StepType    abs_step        = make_xrange::abs(step_val, (typename boost::is_unsigned<StepType>::type*)0 );
            std::size_t s = static_cast<std::size_t>(abs_distance/abs_step);

            if( static_cast<SampleType>(s*abs_step) < abs_distance )
                s++;

            size = s;
        }

        return generated_by<range_gen>( range_gen( begin_val, step_val, size ) );
    }
};

} // namespace ds_detail
} // namespace monomorphic

//____________________________________________________________________________//

//! Creates a range (sequence) dataset.
//!
//! The following overloads are available:
//! @code
//! auto d = xrange();
//! auto d = xrange(end_val);
//! auto d = xrange(end_val, param);
//! auto d = xrange(begin_val, end_val);
//! auto d = xrange(begin_val, end_val, step_val);
//! auto d = xrange(param);
//! @endcode
//!
//! - @c begin_val indicates the start of the sequence (default to 0).
//! - @c end_val is the end of the sequence. If ommited, the dataset has infinite size.\n
//! - @c step_val is the step between two consecutive elements of the sequence, and defaults to 1.\n
//! - @c param is the named parameters that describe the sequence. The following parameters are accepted:
//!   - @c begin: same meaning @c begin_val
//!   - @c end: same meaning as @c end_val
//!   - @c step: same meaning as @c step_val
//!
//!
//! The returned value is an object that implements the dataset API.
//!
//! @note the step size cannot be null, and it should be positive if @c begin_val < @c end_val, negative otherwise.
template<typename SampleType, typename Params>
inline monomorphic::generated_by<monomorphic::xrange_t<SampleType>>
xrange( Params const& params )
{
    return monomorphic::ds_detail::make_xrange<SampleType>::_( params );
}

//____________________________________________________________________________//

/// @overload boost::unit_test::data::xrange()
template<typename SampleType>
inline monomorphic::generated_by<monomorphic::xrange_t<SampleType>>
xrange( SampleType const& end_val )
{
    return monomorphic::ds_detail::make_xrange<SampleType>::_( data::end=end_val );
}

//____________________________________________________________________________//

/// @overload boost::unit_test::data::xrange()
template<typename SampleType, typename Params>
inline typename enable_if_c<nfp::is_named_param_pack<Params>::value,
                            monomorphic::generated_by<monomorphic::xrange_t<SampleType>>>::type
xrange( SampleType const& end_val, Params const& params )
{
    return monomorphic::ds_detail::make_xrange<SampleType>::_(( params, data::end=end_val ));
}

//____________________________________________________________________________//

/// @overload boost::unit_test::data::xrange()
template<typename SampleType>
inline monomorphic::generated_by<monomorphic::xrange_t<SampleType>>
xrange( SampleType const& begin_val, SampleType const& end_val )
{
    return monomorphic::ds_detail::make_xrange<SampleType>::_((
                data::begin=begin_val,
                data::end=end_val ));
}

//____________________________________________________________________________//



/// @overload boost::unit_test::data::xrange()
template<typename SampleType,typename StepType>
inline monomorphic::generated_by<monomorphic::xrange_t<SampleType>>
xrange( SampleType const& begin_val, SampleType const& end_val, StepType const& step_val )
{
    return monomorphic::ds_detail::make_xrange<SampleType,StepType>::_(( 
                data::begin=begin_val, 
                data::end=end_val,
                data::step=step_val ));
}

//____________________________________________________________________________//

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_GENERATORS_XRANGE_HPP_112011GER
