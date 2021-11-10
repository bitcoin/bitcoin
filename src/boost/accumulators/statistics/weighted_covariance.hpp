///////////////////////////////////////////////////////////////////////////////
// weighted_covariance.hpp
//
//  Copyright 2006 Daniel Egloff, Olivier Gygi. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_WEIGHTED_COVARIANCE_HPP_DE_01_01_2006
#define BOOST_ACCUMULATORS_STATISTICS_WEIGHTED_COVARIANCE_HPP_DE_01_01_2006

#include <vector>
#include <limits>
#include <numeric>
#include <functional>
#include <complex>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/range.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/covariance.hpp> // for numeric::outer_product() and type traits
#include <boost/accumulators/statistics/weighted_mean.hpp>

namespace boost { namespace accumulators
{

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // weighted_covariance_impl
    //
    /**
        @brief Weighted Covariance Estimator

        An iterative Monte Carlo estimator for the weighted covariance \f$\mathrm{Cov}(X,X')\f$, where \f$X\f$ is a sample
        and \f$X'\f$ a variate, is given by:

        \f[
            \hat{c}_n = \frac{\bar{w}_n-w_n}{\bar{w}_n} \hat{c}_{n-1} + \frac{w_n}{\bar{w}_n-w_n}(X_n - \hat{\mu}_n)(X_n' - \hat{\mu}_n'),
            \quad n\ge2,\quad\hat{c}_1 = 0,
        \f]

        \f$\hat{\mu}_n\f$ and \f$\hat{\mu}_n'\f$ being the weighted means of the samples and variates and
        \f$\bar{w}_n\f$ the sum of the \f$n\f$ first weights \f$w_i\f$.
    */
    template<typename Sample, typename Weight, typename VariateType, typename VariateTag>
    struct weighted_covariance_impl
      : accumulator_base
    {
        typedef typename numeric::functional::multiplies<Weight, typename numeric::functional::fdiv<Sample, std::size_t>::result_type>::result_type weighted_sample_type;
        typedef typename numeric::functional::multiplies<Weight, typename numeric::functional::fdiv<VariateType, std::size_t>::result_type>::result_type weighted_variate_type;
        // for boost::result_of
        typedef typename numeric::functional::outer_product<weighted_sample_type, weighted_variate_type>::result_type result_type;

        template<typename Args>
        weighted_covariance_impl(Args const &args)
          : cov_(
                numeric::outer_product(
                    numeric::fdiv(args[sample | Sample()], (std::size_t)1)
                      * numeric::one<Weight>::value
                  , numeric::fdiv(args[parameter::keyword<VariateTag>::get() | VariateType()], (std::size_t)1)
                      * numeric::one<Weight>::value
                )
            )
        {
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            std::size_t cnt = count(args);

            if (cnt > 1)
            {
                extractor<tag::weighted_mean_of_variates<VariateType, VariateTag> > const some_weighted_mean_of_variates = {};

                this->cov_ = this->cov_ * (sum_of_weights(args) - args[weight]) / sum_of_weights(args)
                           + numeric::outer_product(
                                 some_weighted_mean_of_variates(args) - args[parameter::keyword<VariateTag>::get()]
                               , weighted_mean(args) - args[sample]
                             ) * args[weight] / (sum_of_weights(args) - args[weight]);
            }
        }

        result_type result(dont_care) const
        {
            return this->cov_;
        }

        // make this accumulator serializeable
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & cov_;
        }

    private:
        result_type cov_;
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::weighted_covariance
//
namespace tag
{
    template<typename VariateType, typename VariateTag>
    struct weighted_covariance
      : depends_on<count, sum_of_weights, weighted_mean, weighted_mean_of_variates<VariateType, VariateTag> >
    {
        typedef accumulators::impl::weighted_covariance_impl<mpl::_1, mpl::_2, VariateType, VariateTag> impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::weighted_covariance
//
namespace extract
{
    extractor<tag::abstract_covariance> const weighted_covariance = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(weighted_covariance)
}

using extract::weighted_covariance;

}} // namespace boost::accumulators

#endif
