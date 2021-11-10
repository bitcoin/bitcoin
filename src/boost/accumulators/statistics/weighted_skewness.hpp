///////////////////////////////////////////////////////////////////////////////
// weighted_skewness.hpp
//
//  Copyright 2006 Olivier Gygi, Daniel Egloff. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_WEIGHTED_SKEWNESS_HPP_EAN_28_10_2005
#define BOOST_ACCUMULATORS_STATISTICS_WEIGHTED_SKEWNESS_HPP_EAN_28_10_2005

#include <limits>
#include <boost/mpl/placeholders.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/weighted_moment.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>

namespace boost { namespace accumulators
{

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // weighted_skewness_impl
    /**
        @brief Skewness estimation for weighted samples

        The skewness of a sample distribution is defined as the ratio of the 3rd central moment and the \f$ 3/2 \f$-th power $
        of the 2nd central moment (the variance) of the samples. The skewness can also be expressed by the simple moments:

        \f[
            \hat{g}_1 =
                \frac
                {\widehat{m}_n^{(3)}-3\widehat{m}_n^{(2)}\hat{\mu}_n+2\hat{\mu}_n^3}
                {\left(\widehat{m}_n^{(2)} - \hat{\mu}_n^{2}\right)^{3/2}}
        \f]

        where \f$ \widehat{m}_n^{(i)} \f$ are the \f$ i \f$-th moment and \f$ \hat{\mu}_n \f$ the mean (first moment) of the
        \f$ n \f$ samples.

        The skewness estimator for weighted samples is formally identical to the estimator for unweighted samples, except that
        the weighted counterparts of all measures it depends on are to be taken.
    */
    template<typename Sample, typename Weight>
    struct weighted_skewness_impl
      : accumulator_base
    {
        typedef typename numeric::functional::multiplies<Sample, Weight>::result_type weighted_sample;
        // for boost::result_of
        typedef typename numeric::functional::fdiv<weighted_sample, weighted_sample>::result_type result_type;

        weighted_skewness_impl(dont_care) {}

        template<typename Args>
        result_type result(Args const &args) const
        {
            return numeric::fdiv(
                        accumulators::weighted_moment<3>(args)
                        - 3. * accumulators::weighted_moment<2>(args) * weighted_mean(args)
                        + 2. * weighted_mean(args) * weighted_mean(args) * weighted_mean(args)
                      , ( accumulators::weighted_moment<2>(args) - weighted_mean(args) * weighted_mean(args) )
                      * std::sqrt( accumulators::weighted_moment<2>(args) - weighted_mean(args) * weighted_mean(args) )
                   );
        }
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::weighted_skewness
//
namespace tag
{
    struct weighted_skewness
      : depends_on<weighted_mean, weighted_moment<2>, weighted_moment<3> >
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::weighted_skewness_impl<mpl::_1, mpl::_2> impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::weighted_skewness
//
namespace extract
{
    extractor<tag::weighted_skewness> const weighted_skewness = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(weighted_skewness)
}

using extract::weighted_skewness;

}} // namespace boost::accumulators

#endif
