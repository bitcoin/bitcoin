///////////////////////////////////////////////////////////////////////////////
// mean.hpp
//
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_MEAN_HPP_EAN_28_10_2005
#define BOOST_ACCUMULATORS_STATISTICS_MEAN_HPP_EAN_28_10_2005

#include <boost/mpl/placeholders.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/sum.hpp>

namespace boost { namespace accumulators
{

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // mean_impl
    //      lazy, by default
    template<typename Sample, typename SumFeature>
    struct mean_impl
      : accumulator_base
    {
        // for boost::result_of
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type result_type;

        mean_impl(dont_care) {}

        template<typename Args>
        result_type result(Args const &args) const
        {
            extractor<SumFeature> sum;
            return numeric::fdiv(sum(args), count(args));
        }

        // serialization is done by accumulators it depends on
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version) {}
    };

    template<typename Sample, typename Tag>
    struct immediate_mean_impl
      : accumulator_base
    {
        // for boost::result_of
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type result_type;

        template<typename Args>
        immediate_mean_impl(Args const &args)
          : mean(numeric::fdiv(args[sample | Sample()], numeric::one<std::size_t>::value))
        {
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            std::size_t cnt = count(args);
            this->mean = numeric::fdiv(
                (this->mean * (cnt - 1)) + args[parameter::keyword<Tag>::get()]
              , cnt
            );
        }

        result_type result(dont_care) const
        {
            return this->mean;
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & mean;
        }

    private:
        result_type mean;
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::mean
// tag::immediate_mean
// tag::mean_of_weights
// tag::immediate_mean_of_weights
// tag::mean_of_variates
// tag::immediate_mean_of_variates
//
namespace tag
{
    struct mean
      : depends_on<count, sum>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::mean_impl<mpl::_1, sum> impl;
    };
    struct immediate_mean
      : depends_on<count>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::immediate_mean_impl<mpl::_1, tag::sample> impl;
    };
    struct mean_of_weights
      : depends_on<count, sum_of_weights>
    {
        typedef mpl::true_ is_weight_accumulator;
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::mean_impl<mpl::_2, sum_of_weights> impl;
    };
    struct immediate_mean_of_weights
      : depends_on<count>
    {
        typedef mpl::true_ is_weight_accumulator;
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::immediate_mean_impl<mpl::_2, tag::weight> impl;
    };
    template<typename VariateType, typename VariateTag>
    struct mean_of_variates
      : depends_on<count, sum_of_variates<VariateType, VariateTag> >
    {
        /// INTERNAL ONLY
        ///
        typedef mpl::always<accumulators::impl::mean_impl<VariateType, sum_of_variates<VariateType, VariateTag> > > impl;
    };
    template<typename VariateType, typename VariateTag>
    struct immediate_mean_of_variates
      : depends_on<count>
    {
        /// INTERNAL ONLY
        ///
        typedef mpl::always<accumulators::impl::immediate_mean_impl<VariateType, VariateTag> > impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::mean
// extract::mean_of_weights
// extract::mean_of_variates
//
namespace extract
{
    extractor<tag::mean> const mean = {};
    extractor<tag::mean_of_weights> const mean_of_weights = {};
    BOOST_ACCUMULATORS_DEFINE_EXTRACTOR(tag, mean_of_variates, (typename)(typename))

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(mean)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(mean_of_weights)
}

using extract::mean;
using extract::mean_of_weights;
using extract::mean_of_variates;

// mean(lazy) -> mean
template<>
struct as_feature<tag::mean(lazy)>
{
    typedef tag::mean type;
};

// mean(immediate) -> immediate_mean
template<>
struct as_feature<tag::mean(immediate)>
{
    typedef tag::immediate_mean type;
};

// mean_of_weights(lazy) -> mean_of_weights
template<>
struct as_feature<tag::mean_of_weights(lazy)>
{
    typedef tag::mean_of_weights type;
};

// mean_of_weights(immediate) -> immediate_mean_of_weights
template<>
struct as_feature<tag::mean_of_weights(immediate)>
{
    typedef tag::immediate_mean_of_weights type;
};

// mean_of_variates<VariateType, VariateTag>(lazy) -> mean_of_variates<VariateType, VariateTag>
template<typename VariateType, typename VariateTag>
struct as_feature<tag::mean_of_variates<VariateType, VariateTag>(lazy)>
{
    typedef tag::mean_of_variates<VariateType, VariateTag> type;
};

// mean_of_variates<VariateType, VariateTag>(immediate) -> immediate_mean_of_variates<VariateType, VariateTag>
template<typename VariateType, typename VariateTag>
struct as_feature<tag::mean_of_variates<VariateType, VariateTag>(immediate)>
{
    typedef tag::immediate_mean_of_variates<VariateType, VariateTag> type;
};

// for the purposes of feature-based dependency resolution,
// immediate_mean provides the same feature as mean
template<>
struct feature_of<tag::immediate_mean>
  : feature_of<tag::mean>
{
};

// for the purposes of feature-based dependency resolution,
// immediate_mean provides the same feature as mean
template<>
struct feature_of<tag::immediate_mean_of_weights>
  : feature_of<tag::mean_of_weights>
{
};

// for the purposes of feature-based dependency resolution,
// immediate_mean provides the same feature as mean
template<typename VariateType, typename VariateTag>
struct feature_of<tag::immediate_mean_of_variates<VariateType, VariateTag> >
  : feature_of<tag::mean_of_variates<VariateType, VariateTag> >
{
};

// So that mean can be automatically substituted with
// weighted_mean when the weight parameter is non-void.
template<>
struct as_weighted_feature<tag::mean>
{
    typedef tag::weighted_mean type;
};

template<>
struct feature_of<tag::weighted_mean>
  : feature_of<tag::mean>
{};

// So that immediate_mean can be automatically substituted with
// immediate_weighted_mean when the weight parameter is non-void.
template<>
struct as_weighted_feature<tag::immediate_mean>
{
    typedef tag::immediate_weighted_mean type;
};

template<>
struct feature_of<tag::immediate_weighted_mean>
  : feature_of<tag::immediate_mean>
{};

// So that mean_of_weights<> can be automatically substituted with
// weighted_mean_of_variates<> when the weight parameter is non-void.
template<typename VariateType, typename VariateTag>
struct as_weighted_feature<tag::mean_of_variates<VariateType, VariateTag> >
{
    typedef tag::weighted_mean_of_variates<VariateType, VariateTag> type;
};

template<typename VariateType, typename VariateTag>
struct feature_of<tag::weighted_mean_of_variates<VariateType, VariateTag> >
  : feature_of<tag::mean_of_variates<VariateType, VariateTag> >
{
};

// So that immediate_mean_of_weights<> can be automatically substituted with
// immediate_weighted_mean_of_variates<> when the weight parameter is non-void.
template<typename VariateType, typename VariateTag>
struct as_weighted_feature<tag::immediate_mean_of_variates<VariateType, VariateTag> >
{
    typedef tag::immediate_weighted_mean_of_variates<VariateType, VariateTag> type;
};

template<typename VariateType, typename VariateTag>
struct feature_of<tag::immediate_weighted_mean_of_variates<VariateType, VariateTag> >
  : feature_of<tag::immediate_mean_of_variates<VariateType, VariateTag> >
{
};

////////////////////////////////////////////////////////////////////////////
//// droppable_accumulator<mean_impl>
////  need to specialize droppable lazy mean to cache the result at the
////  point the accumulator is dropped.
///// INTERNAL ONLY
/////
//template<typename Sample, typename SumFeature>
//struct droppable_accumulator<impl::mean_impl<Sample, SumFeature> >
//  : droppable_accumulator_base<
//        with_cached_result<impl::mean_impl<Sample, SumFeature> >
//    >
//{
//    template<typename Args>
//    droppable_accumulator(Args const &args)
//      : droppable_accumulator::base(args)
//    {
//    }
//};

}} // namespace boost::accumulators

#endif
