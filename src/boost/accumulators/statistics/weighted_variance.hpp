///////////////////////////////////////////////////////////////////////////////
// weighted_variance.hpp
//
//  Copyright 2005 Daniel Egloff, Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_WEIGHTED_VARIANCE_HPP_EAN_28_10_2005
#define BOOST_ACCUMULATORS_STATISTICS_WEIGHTED_VARIANCE_HPP_EAN_28_10_2005

#include <boost/mpl/placeholders.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/weighted_sum.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>
#include <boost/accumulators/statistics/weighted_moment.hpp>

namespace boost { namespace accumulators
{

namespace impl
{
    //! Lazy calculation of variance of weighted samples.
    /*!
        The default implementation of the variance of weighted samples is based on the second moment
        \f$\widehat{m}_n^{(2)}\f$ (weighted_moment<2>) and the mean\f$ \hat{\mu}_n\f$ (weighted_mean):
        \f[
            \hat{\sigma}_n^2 = \widehat{m}_n^{(2)}-\hat{\mu}_n^2,
        \f]
        where \f$n\f$ is the number of samples.
    */
    template<typename Sample, typename Weight, typename MeanFeature>
    struct lazy_weighted_variance_impl
      : accumulator_base
    {
        typedef typename numeric::functional::multiplies<Sample, Weight>::result_type weighted_sample;
        // for boost::result_of
        typedef typename numeric::functional::fdiv<weighted_sample, Weight>::result_type result_type;

        lazy_weighted_variance_impl(dont_care) {}

        template<typename Args>
        result_type result(Args const &args) const
        {
            extractor<MeanFeature> const some_mean = {};
            result_type tmp = some_mean(args);
            return accumulators::weighted_moment<2>(args) - tmp * tmp;
        }
    };

    //! Iterative calculation of variance of weighted samples.
    /*!
        Iterative calculation of variance of weighted samples:
        \f[
            \hat{\sigma}_n^2 =
                \frac{\bar{w}_n - w_n}{\bar{w}_n}\hat{\sigma}_{n - 1}^2
              + \frac{w_n}{\bar{w}_n - w_n}\left(X_n - \hat{\mu}_n\right)^2
            ,\quad n\ge2,\quad\hat{\sigma}_0^2 = 0.
        \f]
        where \f$\bar{w}_n\f$ is the sum of the \f$n\f$ weights \f$w_i\f$ and \f$\hat{\mu}_n\f$
        the estimate of the mean of the weighted samples. Note that the sample variance is not defined for
        \f$n <= 1\f$.
    */
    template<typename Sample, typename Weight, typename MeanFeature, typename Tag>
    struct weighted_variance_impl
      : accumulator_base
    {
        typedef typename numeric::functional::multiplies<Sample, Weight>::result_type weighted_sample;
        // for boost::result_of
        typedef typename numeric::functional::fdiv<weighted_sample, Weight>::result_type result_type;

        template<typename Args>
        weighted_variance_impl(Args const &args)
          : weighted_variance(numeric::fdiv(args[sample | Sample()], numeric::one<Weight>::value))
        {
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            std::size_t cnt = count(args);

            if(cnt > 1)
            {
                extractor<MeanFeature> const some_mean = {};

                result_type tmp = args[parameter::keyword<Tag>::get()] - some_mean(args);

                this->weighted_variance =
                    numeric::fdiv(this->weighted_variance * (sum_of_weights(args) - args[weight]), sum_of_weights(args))
                  + numeric::fdiv(tmp * tmp * args[weight], sum_of_weights(args) - args[weight] );
            }
        }

        result_type result(dont_care) const
        {
            return this->weighted_variance;
        }

        // make this accumulator serializeable
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        {
            ar & weighted_variance;
        }

    private:
        result_type weighted_variance;
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::weighted_variance
// tag::immediate_weighted_variance
//
namespace tag
{
    struct lazy_weighted_variance
      : depends_on<weighted_moment<2>, weighted_mean>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::lazy_weighted_variance_impl<mpl::_1, mpl::_2, weighted_mean> impl;
    };

    struct weighted_variance
      : depends_on<count, immediate_weighted_mean>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::weighted_variance_impl<mpl::_1, mpl::_2, immediate_weighted_mean, sample> impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::weighted_variance
// extract::immediate_weighted_variance
//
namespace extract
{
    extractor<tag::lazy_weighted_variance> const lazy_weighted_variance = {};
    extractor<tag::weighted_variance> const weighted_variance = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(lazy_weighted_variance)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(weighted_variance)
}

using extract::lazy_weighted_variance;
using extract::weighted_variance;

// weighted_variance(lazy) -> lazy_weighted_variance
template<>
struct as_feature<tag::weighted_variance(lazy)>
{
    typedef tag::lazy_weighted_variance type;
};

// weighted_variance(immediate) -> weighted_variance
template<>
struct as_feature<tag::weighted_variance(immediate)>
{
    typedef tag::weighted_variance type;
};

////////////////////////////////////////////////////////////////////////////
//// droppable_accumulator<weighted_variance_impl>
////  need to specialize droppable lazy weighted_variance to cache the result at the
////  point the accumulator is dropped.
///// INTERNAL ONLY
/////
//template<typename Sample, typename Weight, typename MeanFeature>
//struct droppable_accumulator<impl::weighted_variance_impl<Sample, Weight, MeanFeature> >
//  : droppable_accumulator_base<
//        with_cached_result<impl::weighted_variance_impl<Sample, Weight, MeanFeature> >
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
