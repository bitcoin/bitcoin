///////////////////////////////////////////////////////////////////////////////
// tail_mean.hpp
//
//  Copyright 2006 Daniel Egloff, Olivier Gygi. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_TAIL_MEAN_HPP_DE_01_01_2006
#define BOOST_ACCUMULATORS_STATISTICS_TAIL_MEAN_HPP_DE_01_01_2006

#include <numeric>
#include <vector>
#include <limits>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/tail.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>
#include <boost/accumulators/statistics/parameters/quantile_probability.hpp>

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4127) // conditional expression is constant
#endif

namespace boost { namespace accumulators
{

namespace impl
{

    ///////////////////////////////////////////////////////////////////////////////
    // coherent_tail_mean_impl
    //
    /**
        @brief Estimation of the coherent tail mean based on order statistics (for both left and right tails)

        The coherent tail mean \f$\widehat{CTM}_{n,\alpha}(X)\f$ is equal to the non-coherent tail mean \f$\widehat{NCTM}_{n,\alpha}(X)\f$
        plus a correction term that ensures coherence in case of non-continuous distributions.

        \f[
            \widehat{CTM}_{n,\alpha}^{\mathrm{right}}(X) = \widehat{NCTM}_{n,\alpha}^{\mathrm{right}}(X) +
            \frac{1}{\lceil n(1-\alpha)\rceil}\hat{q}_{n,\alpha}(X)\left(1 - \alpha - \frac{1}{n}\lceil n(1-\alpha)\rceil \right)
        \f]

        \f[
            \widehat{CTM}_{n,\alpha}^{\mathrm{left}}(X) = \widehat{NCTM}_{n,\alpha}^{\mathrm{left}}(X) +
            \frac{1}{\lceil n\alpha\rceil}\hat{q}_{n,\alpha}(X)\left(\alpha - \frac{1}{n}\lceil n\alpha\rceil \right)
        \f]
    */
    template<typename Sample, typename LeftRight>
    struct coherent_tail_mean_impl
      : accumulator_base
    {
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type float_type;
        // for boost::result_of
        typedef float_type result_type;

        coherent_tail_mean_impl(dont_care) {}

        template<typename Args>
        result_type result(Args const &args) const
        {
            std::size_t cnt = count(args);

            std::size_t n = static_cast<std::size_t>(
                std::ceil(
                    cnt * ( ( is_same<LeftRight, left>::value ) ? args[quantile_probability] : 1. - args[quantile_probability] )
                )
            );

            extractor<tag::non_coherent_tail_mean<LeftRight> > const some_non_coherent_tail_mean = {};

            return some_non_coherent_tail_mean(args)
                 + numeric::fdiv(quantile(args), n)
                 * (
                     ( is_same<LeftRight, left>::value ) ? args[quantile_probability] : 1. - args[quantile_probability]
                     - numeric::fdiv(n, count(args))
                   );
        }
        
        // serialization is done by accumulators it depends on
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version) {}
    };

    ///////////////////////////////////////////////////////////////////////////////
    // non_coherent_tail_mean_impl
    //
    /**
        @brief Estimation of the (non-coherent) tail mean based on order statistics (for both left and right tails)

        An estimation of the non-coherent tail mean \f$\widehat{NCTM}_{n,\alpha}(X)\f$ is given by the mean of the
        \f$\lceil n\alpha\rceil\f$ smallest samples (left tail) or the mean of the  \f$\lceil n(1-\alpha)\rceil\f$
        largest samples (right tail), \f$n\f$ being the total number of samples and \f$\alpha\f$ the quantile level:

        \f[
            \widehat{NCTM}_{n,\alpha}^{\mathrm{right}}(X) = \frac{1}{\lceil n(1-\alpha)\rceil} \sum_{i=\lceil \alpha n \rceil}^n X_{i:n}
        \f]

        \f[
            \widehat{NCTM}_{n,\alpha}^{\mathrm{left}}(X) = \frac{1}{\lceil n\alpha\rceil} \sum_{i=1}^{\lceil \alpha n \rceil} X_{i:n}
        \f]

        It thus requires the caching of at least the \f$\lceil n\alpha\rceil\f$ smallest or the \f$\lceil n(1-\alpha)\rceil\f$
        largest samples.

        @param quantile_probability
    */
    template<typename Sample, typename LeftRight>
    struct non_coherent_tail_mean_impl
      : accumulator_base
    {
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type float_type;
        // for boost::result_of
        typedef float_type result_type;

        non_coherent_tail_mean_impl(dont_care) {}

        template<typename Args>
        result_type result(Args const &args) const
        {
            std::size_t cnt = count(args);

            std::size_t n = static_cast<std::size_t>(
                std::ceil(
                    cnt * ( ( is_same<LeftRight, left>::value ) ? args[quantile_probability] : 1. - args[quantile_probability] )
                )
            );

            // If n is in a valid range, return result, otherwise return NaN or throw exception
            if (n <= static_cast<std::size_t>(tail(args).size()))
                return numeric::fdiv(
                    std::accumulate(
                        tail(args).begin()
                      , tail(args).begin() + n
                      , Sample(0)
                    )
                  , n
                );
            else
            {
                if (std::numeric_limits<result_type>::has_quiet_NaN)
                {
                    return std::numeric_limits<result_type>::quiet_NaN();
                }
                else
                {
                    std::ostringstream msg;
                    msg << "index n = " << n << " is not in valid range [0, " << tail(args).size() << ")";
                    boost::throw_exception(std::runtime_error(msg.str()));
                    return Sample(0);
                }
            }
        }
        
        // serialization is done by accumulators it depends on
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version) {}
    };

} // namespace impl


///////////////////////////////////////////////////////////////////////////////
// tag::coherent_tail_mean<>
// tag::non_coherent_tail_mean<>
//
namespace tag
{
    template<typename LeftRight>
    struct coherent_tail_mean
      : depends_on<count, quantile, non_coherent_tail_mean<LeftRight> >
    {
        typedef accumulators::impl::coherent_tail_mean_impl<mpl::_1, LeftRight> impl;
    };

    template<typename LeftRight>
    struct non_coherent_tail_mean
      : depends_on<count, tail<LeftRight> >
    {
        typedef accumulators::impl::non_coherent_tail_mean_impl<mpl::_1, LeftRight> impl;
    };

    struct abstract_non_coherent_tail_mean
      : depends_on<>
    {
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::non_coherent_tail_mean;
// extract::coherent_tail_mean;
//
namespace extract
{
    extractor<tag::abstract_non_coherent_tail_mean> const non_coherent_tail_mean = {};
    extractor<tag::tail_mean> const coherent_tail_mean = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(non_coherent_tail_mean)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(coherent_tail_mean)
}

using extract::non_coherent_tail_mean;
using extract::coherent_tail_mean;

// for the purposes of feature-based dependency resolution,
// coherent_tail_mean<LeftRight> provides the same feature as tail_mean
template<typename LeftRight>
struct feature_of<tag::coherent_tail_mean<LeftRight> >
  : feature_of<tag::tail_mean>
{
};

template<typename LeftRight>
struct feature_of<tag::non_coherent_tail_mean<LeftRight> >
  : feature_of<tag::abstract_non_coherent_tail_mean>
{
};

// So that non_coherent_tail_mean can be automatically substituted
// with weighted_non_coherent_tail_mean when the weight parameter is non-void.
template<typename LeftRight>
struct as_weighted_feature<tag::non_coherent_tail_mean<LeftRight> >
{
    typedef tag::non_coherent_weighted_tail_mean<LeftRight> type;
};

template<typename LeftRight>
struct feature_of<tag::non_coherent_weighted_tail_mean<LeftRight> >
  : feature_of<tag::non_coherent_tail_mean<LeftRight> >
{};

// NOTE that non_coherent_tail_mean cannot be feature-grouped with tail_mean,
// which is the base feature for coherent tail means, since (at least for
// non-continuous distributions) non_coherent_tail_mean is a different measure!

}} // namespace boost::accumulators

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif
