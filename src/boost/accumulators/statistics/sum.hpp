///////////////////////////////////////////////////////////////////////////////
// sum.hpp
//
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_SUM_HPP_EAN_28_10_2005
#define BOOST_ACCUMULATORS_STATISTICS_SUM_HPP_EAN_28_10_2005

#include <boost/mpl/placeholders.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/framework/parameters/weight.hpp>
#include <boost/accumulators/framework/accumulators/external_accumulator.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/count.hpp>

namespace boost { namespace accumulators
{

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // sum_impl
    template<typename Sample, typename Tag>
    struct sum_impl
      : accumulator_base
    {
        // for boost::result_of
        typedef Sample result_type;

        template<typename Args>
        sum_impl(Args const &args)
          : sum(args[parameter::keyword<Tag>::get() | Sample()])
        {
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            // what about overflow?
            this->sum += args[parameter::keyword<Tag>::get()];
        }

        result_type result(dont_care) const
        {
            return this->sum;
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & sum;
        }

    private:
        Sample sum;
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::sum
// tag::sum_of_weights
// tag::sum_of_variates
//
namespace tag
{
    struct sum
      : depends_on<>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::sum_impl<mpl::_1, tag::sample> impl;
    };

    struct sum_of_weights
      : depends_on<>
    {
        typedef mpl::true_ is_weight_accumulator;
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::sum_impl<mpl::_2, tag::weight> impl;
    };

    template<typename VariateType, typename VariateTag>
    struct sum_of_variates
      : depends_on<>
    {
        /// INTERNAL ONLY
        ///
        typedef mpl::always<accumulators::impl::sum_impl<VariateType, VariateTag> > impl;
    };

    struct abstract_sum_of_variates
      : depends_on<>
    {
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::sum
// extract::sum_of_weights
// extract::sum_of_variates
//
namespace extract
{
    extractor<tag::sum> const sum = {};
    extractor<tag::sum_of_weights> const sum_of_weights = {};
    extractor<tag::abstract_sum_of_variates> const sum_of_variates = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(sum)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(sum_of_weights)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(sum_of_variates)
}

using extract::sum;
using extract::sum_of_weights;
using extract::sum_of_variates;

// So that mean can be automatically substituted with
// weighted_mean when the weight parameter is non-void.
template<>
struct as_weighted_feature<tag::sum>
{
    typedef tag::weighted_sum type;
};

template<>
struct feature_of<tag::weighted_sum>
  : feature_of<tag::sum>
{};

template<typename VariateType, typename VariateTag>
struct feature_of<tag::sum_of_variates<VariateType, VariateTag> >
  : feature_of<tag::abstract_sum_of_variates>
{
};

}} // namespace boost::accumulators

#endif
