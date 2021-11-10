///////////////////////////////////////////////////////////////////////////////
// tail_quantile.hpp
//
//  Copyright 2006 Daniel Egloff, Olivier Gygi. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_TAIL_QUANTILE_HPP_DE_01_01_2006
#define BOOST_ACCUMULATORS_STATISTICS_TAIL_QUANTILE_HPP_DE_01_01_2006

#include <vector>
#include <limits>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <boost/config/no_tr1/cmath.hpp>             // For ceil
#include <boost/throw_exception.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/tail.hpp>
#include <boost/accumulators/statistics/count.hpp>
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
    // tail_quantile_impl
    //  Tail quantile estimation based on order statistics
    /**
        @brief Tail quantile estimation based on order statistics (for both left and right tails)

        The estimation of a tail quantile \f$\hat{q}\f$ with level \f$\alpha\f$ based on order statistics requires the
        caching of at least the \f$\lceil n\alpha\rceil\f$ smallest or the \f$\lceil n(1-\alpha)\rceil\f$ largest samples,
        \f$n\f$ being the total number of samples. The largest of the \f$\lceil n\alpha\rceil\f$ smallest samples or the
        smallest of the \f$\lceil n(1-\alpha)\rceil\f$ largest samples provides an estimate for the quantile:

        \f[
            \hat{q}_{n,\alpha} = X_{\lceil \alpha n \rceil:n}
        \f]

        @param quantile_probability
    */
    template<typename Sample, typename LeftRight>
    struct tail_quantile_impl
      : accumulator_base
    {
        // for boost::result_of
        typedef Sample result_type;

        tail_quantile_impl(dont_care) {}

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
            if ( n < static_cast<std::size_t>(tail(args).size()))
            {
               // Note that the cached samples of the left are sorted in ascending order,
               // whereas the samples of the right tail are sorted in descending order
               return *(boost::begin(tail(args)) + n - 1);
            }
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
// tag::tail_quantile<>
//
namespace tag
{
    template<typename LeftRight>
    struct tail_quantile
      : depends_on<count, tail<LeftRight> >
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::tail_quantile_impl<mpl::_1, LeftRight> impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::tail_quantile
//
namespace extract
{
    extractor<tag::quantile> const tail_quantile = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(tail_quantile)
}

using extract::tail_quantile;

// for the purposes of feature-based dependency resolution,
// tail_quantile<LeftRight> provide the same feature as quantile
template<typename LeftRight>
struct feature_of<tag::tail_quantile<LeftRight> >
  : feature_of<tag::quantile>
{
};

// So that tail_quantile can be automatically substituted with
// weighted_tail_quantile when the weight parameter is non-void.
template<typename LeftRight>
struct as_weighted_feature<tag::tail_quantile<LeftRight> >
{
    typedef tag::weighted_tail_quantile<LeftRight> type;
};

template<typename LeftRight>
struct feature_of<tag::weighted_tail_quantile<LeftRight> >
  : feature_of<tag::tail_quantile<LeftRight> >
{};

}} // namespace boost::accumulators

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif
