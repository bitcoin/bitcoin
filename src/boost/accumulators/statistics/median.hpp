///////////////////////////////////////////////////////////////////////////////
// median.hpp
//
//  Copyright 2006 Eric Niebler, Olivier Gygi. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_MEDIAN_HPP_EAN_28_10_2005
#define BOOST_ACCUMULATORS_STATISTICS_MEDIAN_HPP_EAN_28_10_2005

#include <boost/mpl/placeholders.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>
#include <boost/accumulators/statistics/density.hpp>
#include <boost/accumulators/statistics/p_square_cumul_dist.hpp>

namespace boost { namespace accumulators
{

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // median_impl
    //
    /**
        @brief Median estimation based on the \f$P^2\f$ quantile estimator

        The \f$P^2\f$ algorithm is invoked with a quantile probability of 0.5.
    */
    template<typename Sample>
    struct median_impl
      : accumulator_base
    {
        // for boost::result_of
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type result_type;

        median_impl(dont_care) {}

        template<typename Args>
        result_type result(Args const &args) const
        {
            return p_square_quantile_for_median(args);
        }
        
        // serialization is done by accumulators it depends on
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version) {}
    };
    ///////////////////////////////////////////////////////////////////////////////
    // with_density_median_impl
    //
    /**
        @brief Median estimation based on the density estimator

        The algorithm determines the bin in which the \f$0.5*cnt\f$-th sample lies, \f$cnt\f$ being
        the total number of samples. It returns the approximate horizontal position of this sample,
        based on a linear interpolation inside the bin.
    */
    template<typename Sample>
    struct with_density_median_impl
      : accumulator_base
    {
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type float_type;
        typedef std::vector<std::pair<float_type, float_type> > histogram_type;
        typedef iterator_range<typename histogram_type::iterator> range_type;
        // for boost::result_of
        typedef float_type result_type;

        template<typename Args>
        with_density_median_impl(Args const &args)
          : sum(numeric::fdiv(args[sample | Sample()], (std::size_t)1))
          , is_dirty(true)
        {
        }

        void operator ()(dont_care)
        {
            this->is_dirty = true;
        }


        template<typename Args>
        result_type result(Args const &args) const
        {
            if (this->is_dirty)
            {
                this->is_dirty = false;

                std::size_t cnt = count(args);
                range_type histogram = density(args);
                typename range_type::iterator it = histogram.begin();
                while (this->sum < 0.5 * cnt)
                {
                    this->sum += it->second * cnt;
                    ++it;
                }
                --it;
                float_type over = numeric::fdiv(this->sum - 0.5 * cnt, it->second * cnt);
                this->median = it->first * over + (it + 1)->first * (1. - over);
            }

            return this->median;
        }

        // make this accumulator serializeable
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & sum;
            ar & is_dirty;
            ar & median;
        }


    private:
        mutable float_type sum;
        mutable bool is_dirty;
        mutable float_type median;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // with_p_square_cumulative_distribution_median_impl
    //
    /**
        @brief Median estimation based on the \f$P^2\f$ cumulative distribution estimator

        The algorithm determines the first (leftmost) bin with a height exceeding 0.5. It
        returns the approximate horizontal position of where the cumulative distribution
        equals 0.5, based on a linear interpolation inside the bin.
    */
    template<typename Sample>
    struct with_p_square_cumulative_distribution_median_impl
      : accumulator_base
    {
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type float_type;
        typedef std::vector<std::pair<float_type, float_type> > histogram_type;
        typedef iterator_range<typename histogram_type::iterator> range_type;
        // for boost::result_of
        typedef float_type result_type;

        with_p_square_cumulative_distribution_median_impl(dont_care)
          : is_dirty(true)
        {
        }

        void operator ()(dont_care)
        {
            this->is_dirty = true;
        }

        template<typename Args>
        result_type result(Args const &args) const
        {
            if (this->is_dirty)
            {
                this->is_dirty = false;

                range_type histogram = p_square_cumulative_distribution(args);
                typename range_type::iterator it = histogram.begin();
                while (it->second < 0.5)
                {
                    ++it;
                }
                float_type over = numeric::fdiv(it->second - 0.5, it->second - (it - 1)->second);
                this->median = it->first * over + (it + 1)->first * ( 1. - over );
            }

            return this->median;
        }
        
        // make this accumulator serializeable
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & is_dirty;
            ar & median;
        }

    private:

        mutable bool is_dirty;
        mutable float_type median;
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::median
// tag::with_densisty_median
// tag::with_p_square_cumulative_distribution_median
//
namespace tag
{
    struct median
      : depends_on<p_square_quantile_for_median>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::median_impl<mpl::_1> impl;
    };
    struct with_density_median
      : depends_on<count, density>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::with_density_median_impl<mpl::_1> impl;
    };
    struct with_p_square_cumulative_distribution_median
      : depends_on<p_square_cumulative_distribution>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::with_p_square_cumulative_distribution_median_impl<mpl::_1> impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::median
// extract::with_density_median
// extract::with_p_square_cumulative_distribution_median
//
namespace extract
{
    extractor<tag::median> const median = {};
    extractor<tag::with_density_median> const with_density_median = {};
    extractor<tag::with_p_square_cumulative_distribution_median> const with_p_square_cumulative_distribution_median = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(median)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(with_density_median)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(with_p_square_cumulative_distribution_median)
}

using extract::median;
using extract::with_density_median;
using extract::with_p_square_cumulative_distribution_median;

// median(with_p_square_quantile) -> median
template<>
struct as_feature<tag::median(with_p_square_quantile)>
{
    typedef tag::median type;
};

// median(with_density) -> with_density_median
template<>
struct as_feature<tag::median(with_density)>
{
    typedef tag::with_density_median type;
};

// median(with_p_square_cumulative_distribution) -> with_p_square_cumulative_distribution_median
template<>
struct as_feature<tag::median(with_p_square_cumulative_distribution)>
{
    typedef tag::with_p_square_cumulative_distribution_median type;
};

// for the purposes of feature-based dependency resolution,
// with_density_median and with_p_square_cumulative_distribution_median
// provide the same feature as median
template<>
struct feature_of<tag::with_density_median>
  : feature_of<tag::median>
{
};

template<>
struct feature_of<tag::with_p_square_cumulative_distribution_median>
  : feature_of<tag::median>
{
};

// So that median can be automatically substituted with
// weighted_median when the weight parameter is non-void.
template<>
struct as_weighted_feature<tag::median>
{
    typedef tag::weighted_median type;
};

template<>
struct feature_of<tag::weighted_median>
  : feature_of<tag::median>
{
};

// So that with_density_median can be automatically substituted with
// with_density_weighted_median when the weight parameter is non-void.
template<>
struct as_weighted_feature<tag::with_density_median>
{
    typedef tag::with_density_weighted_median type;
};

template<>
struct feature_of<tag::with_density_weighted_median>
  : feature_of<tag::with_density_median>
{
};

// So that with_p_square_cumulative_distribution_median can be automatically substituted with
// with_p_square_cumulative_distribution_weighted_median when the weight parameter is non-void.
template<>
struct as_weighted_feature<tag::with_p_square_cumulative_distribution_median>
{
    typedef tag::with_p_square_cumulative_distribution_weighted_median type;
};

template<>
struct feature_of<tag::with_p_square_cumulative_distribution_weighted_median>
  : feature_of<tag::with_p_square_cumulative_distribution_median>
{
};

}} // namespace boost::accumulators

#endif
