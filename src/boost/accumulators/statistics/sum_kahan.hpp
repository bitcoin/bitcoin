///////////////////////////////////////////////////////////////////////////////
// sum_kahan.hpp
//
//  Copyright 2010 Gaetano Mendola, 2011 Simon West. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_SUM_KAHAN_HPP_EAN_26_07_2010
#define BOOST_ACCUMULATORS_STATISTICS_SUM_KAHAN_HPP_EAN_26_07_2010

#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/sum.hpp>
#include <boost/accumulators/statistics/weighted_sum_kahan.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace boost { namespace accumulators
{

namespace impl
{

#if _MSC_VER > 1400
# pragma float_control(push)
# pragma float_control(precise, on)
#endif

template<typename Sample, typename Tag>
struct sum_kahan_impl
  : accumulator_base
{
    typedef Sample result_type;

    ////////////////////////////////////////////////////////////////////////////
    // sum_kahan_impl
    /**
        @brief Kahan summation algorithm

        The Kahan summation algorithm reduces the numerical error obtained with standard
        sequential sum.

    */
    template<typename Args>
    sum_kahan_impl(Args const & args)
      : sum(args[parameter::keyword<Tag>::get() | Sample()]),
        compensation(boost::numeric_cast<Sample>(0.0))
    {
    }

    template<typename Args>
    void 
#if BOOST_ACCUMULATORS_GCC_VERSION > 40305
    __attribute__((__optimize__("no-associative-math")))
#endif
    operator ()(Args const & args)
    {
        const Sample myTmp1 = args[parameter::keyword<Tag>::get()] - this->compensation;
        const Sample myTmp2 = this->sum + myTmp1;
        this->compensation = (myTmp2 - this->sum) - myTmp1;
        this->sum = myTmp2;
    }

    result_type result(dont_care) const
    {
      return this->sum;
    }

    // make this accumulator serializeable
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version)
    { 
        ar & sum;
        ar & compensation;
    }

private:
    Sample sum;
    Sample compensation;
};

#if _MSC_VER > 1400
# pragma float_control(pop)
#endif

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::sum_kahan
// tag::sum_of_weights_kahan
// tag::sum_of_variates_kahan
//
namespace tag
{

    struct sum_kahan
      : depends_on<>
    {
        /// INTERNAL ONLY
        ///
        typedef impl::sum_kahan_impl< mpl::_1, tag::sample > impl;
    };

    struct sum_of_weights_kahan
      : depends_on<>
    {
        typedef mpl::true_ is_weight_accumulator;
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::sum_kahan_impl<mpl::_2, tag::weight> impl;
    };

    template<typename VariateType, typename VariateTag>
    struct sum_of_variates_kahan
      : depends_on<>
    {
        /// INTERNAL ONLY
        ///
        typedef mpl::always<accumulators::impl::sum_kahan_impl<VariateType, VariateTag> > impl;
    };

} // namespace tag

///////////////////////////////////////////////////////////////////////////////
// extract::sum_kahan
// extract::sum_of_weights_kahan
// extract::sum_of_variates_kahan
//
namespace extract
{
    extractor<tag::sum_kahan> const sum_kahan = {};
    extractor<tag::sum_of_weights_kahan> const sum_of_weights_kahan = {};
    extractor<tag::abstract_sum_of_variates> const sum_of_variates_kahan = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(sum_kahan)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(sum_of_weights_kahan)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(sum_of_variates_kahan)
} // namespace extract

using extract::sum_kahan;
using extract::sum_of_weights_kahan;
using extract::sum_of_variates_kahan;

// sum(kahan) -> sum_kahan
template<>
struct as_feature<tag::sum(kahan)>
{
    typedef tag::sum_kahan type;
};

// sum_of_weights(kahan) -> sum_of_weights_kahan
template<>
struct as_feature<tag::sum_of_weights(kahan)>
{
    typedef tag::sum_of_weights_kahan type;
};

// So that sum_kahan can be automatically substituted with
// weighted_sum_kahan when the weight parameter is non-void.
template<>
struct as_weighted_feature<tag::sum_kahan>
{
    typedef tag::weighted_sum_kahan type;
};

template<>
struct feature_of<tag::weighted_sum_kahan>
  : feature_of<tag::sum>
{};

// for the purposes of feature-based dependency resolution,
// sum_kahan provides the same feature as sum
template<>
struct feature_of<tag::sum_kahan>
  : feature_of<tag::sum>
{
};

// for the purposes of feature-based dependency resolution,
// sum_of_weights_kahan provides the same feature as sum_of_weights
template<>
struct feature_of<tag::sum_of_weights_kahan>
  : feature_of<tag::sum_of_weights>
{
};

template<typename VariateType, typename VariateTag>
struct feature_of<tag::sum_of_variates_kahan<VariateType, VariateTag> >
  : feature_of<tag::abstract_sum_of_variates>
{
};

}} // namespace boost::accumulators

#endif

