///////////////////////////////////////////////////////////////////////////////
// p_square_quantile.hpp
//
//  Copyright 2005 Daniel Egloff. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_P_SQUARE_QUANTILE_HPP_DE_01_01_2006
#define BOOST_ACCUMULATORS_STATISTICS_P_SQUARE_QUANTILE_HPP_DE_01_01_2006

#include <cmath>
#include <functional>
#include <boost/array.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/parameters/quantile_probability.hpp>
#include <boost/serialization/boost_array.hpp>

namespace boost { namespace accumulators
{

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // p_square_quantile_impl
    //  single quantile estimation
    /**
        @brief Single quantile estimation with the \f$P^2\f$ algorithm

        The \f$P^2\f$ algorithm estimates a quantile dynamically without storing samples. Instead of
        storing the whole sample cumulative distribution, only five points (markers) are stored. The heights
        of these markers are the minimum and the maximum of the samples and the current estimates of the
        \f$(p/2)\f$-, \f$p\f$- and \f$(1+p)/2\f$-quantiles. Their positions are equal to the number
        of samples that are smaller or equal to the markers. Each time a new samples is recorded, the
        positions of the markers are updated and if necessary their heights are adjusted using a piecewise-
        parabolic formula.

        For further details, see

        R. Jain and I. Chlamtac, The P^2 algorithm for dynamic calculation of quantiles and
        histograms without storing observations, Communications of the ACM,
        Volume 28 (October), Number 10, 1985, p. 1076-1085.

        @param quantile_probability
    */
    template<typename Sample, typename Impl>
    struct p_square_quantile_impl
      : accumulator_base
    {
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type float_type;
        typedef array<float_type, 5> array_type;
        // for boost::result_of
        typedef float_type result_type;

        template<typename Args>
        p_square_quantile_impl(Args const &args)
          : p(is_same<Impl, for_median>::value ? float_type(0.5) : args[quantile_probability | float_type(0.5)])
          , heights()
          , actual_positions()
          , desired_positions()
          , positions_increments()
        {
            for(std::size_t i = 0; i < 5; ++i)
            {
                this->actual_positions[i] = i + float_type(1.);
            }

            this->desired_positions[0] = float_type(1.);
            this->desired_positions[1] = float_type(1.) + float_type(2.) * this->p;
            this->desired_positions[2] = float_type(1.) + float_type(4.) * this->p;
            this->desired_positions[3] = float_type(3.) + float_type(2.) * this->p;
            this->desired_positions[4] = float_type(5.);


            this->positions_increments[0] = float_type(0.);
            this->positions_increments[1] = this->p / float_type(2.);
            this->positions_increments[2] = this->p;
            this->positions_increments[3] = (float_type(1.) + this->p) / float_type(2.);
            this->positions_increments[4] = float_type(1.);
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            std::size_t cnt = count(args);

            // accumulate 5 first samples
            if(cnt <= 5)
            {
                this->heights[cnt - 1] = args[sample];

                // complete the initialization of heights by sorting
                if(cnt == 5)
                {
                    std::sort(this->heights.begin(), this->heights.end());
                }
            }
            else
            {
                std::size_t sample_cell = 1; // k

                // find cell k such that heights[k-1] <= args[sample] < heights[k] and adjust extreme values
                if (args[sample] < this->heights[0])
                {
                    this->heights[0] = args[sample];
                    sample_cell = 1;
                }
                else if (this->heights[4] <= args[sample])
                {
                    this->heights[4] = args[sample];
                    sample_cell = 4;
                }
                else
                {
                    typedef typename array_type::iterator iterator;
                    iterator it = std::upper_bound(
                        this->heights.begin()
                      , this->heights.end()
                      , args[sample]
                    );

                    sample_cell = std::distance(this->heights.begin(), it);
                }

                // update positions of markers above sample_cell
                for(std::size_t i = sample_cell; i < 5; ++i)
                {
                    ++this->actual_positions[i];
                }

                // update desired positions of all markers
                for(std::size_t i = 0; i < 5; ++i)
                {
                    this->desired_positions[i] += this->positions_increments[i];
                }

                // adjust heights and actual positions of markers 1 to 3 if necessary
                for(std::size_t i = 1; i <= 3; ++i)
                {
                    // offset to desired positions
                    float_type d = this->desired_positions[i] - this->actual_positions[i];

                    // offset to next position
                    float_type dp = this->actual_positions[i + 1] - this->actual_positions[i];

                    // offset to previous position
                    float_type dm = this->actual_positions[i - 1] - this->actual_positions[i];

                    // height ds
                    float_type hp = (this->heights[i + 1] - this->heights[i]) / dp;
                    float_type hm = (this->heights[i - 1] - this->heights[i]) / dm;

                    if((d >= float_type(1.) && dp > float_type(1.)) || (d <= float_type(-1.) && dm < float_type(-1.)))
                    {
                        short sign_d = static_cast<short>(d / std::abs(d));

                        // try adjusting heights[i] using p-squared formula
                        float_type h = this->heights[i] + sign_d / (dp - dm) * ((sign_d - dm) * hp
                                     + (dp - sign_d) * hm);

                        if(this->heights[i - 1] < h && h < this->heights[i + 1])
                        {
                            this->heights[i] = h;
                        }
                        else
                        {
                            // use linear formula
                            if(d > float_type(0))
                            {
                                this->heights[i] += hp;
                            }
                            if(d < float_type(0))
                            {
                                this->heights[i] -= hm;
                            }
                        }
                        this->actual_positions[i] += sign_d;
                    }
                }
            }
        }

        result_type result(dont_care) const
        {
            return this->heights[2];
        }

        // make this accumulator serializeable
        // TODO: do we need to split to load/save and verify that P did not change?
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & p;
            ar & heights;
            ar & actual_positions;
            ar & desired_positions;
            ar & positions_increments;
        }

    private:
        float_type p;                    // the quantile probability p
        array_type heights;              // q_i
        array_type actual_positions;     // n_i
        array_type desired_positions;    // n'_i
        array_type positions_increments; // dn'_i
    };

} // namespace detail

///////////////////////////////////////////////////////////////////////////////
// tag::p_square_quantile
//
namespace tag
{
    struct p_square_quantile
      : depends_on<count>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::p_square_quantile_impl<mpl::_1, regular> impl;
    };
    struct p_square_quantile_for_median
      : depends_on<count>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::p_square_quantile_impl<mpl::_1, for_median> impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::p_square_quantile
// extract::p_square_quantile_for_median
//
namespace extract
{
    extractor<tag::p_square_quantile> const p_square_quantile = {};
    extractor<tag::p_square_quantile_for_median> const p_square_quantile_for_median = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(p_square_quantile)
    BOOST_ACCUMULATORS_IGNORE_GLOBAL(p_square_quantile_for_median)
}

using extract::p_square_quantile;
using extract::p_square_quantile_for_median;

// So that p_square_quantile can be automatically substituted with
// weighted_p_square_quantile when the weight parameter is non-void
template<>
struct as_weighted_feature<tag::p_square_quantile>
{
    typedef tag::weighted_p_square_quantile type;
};

template<>
struct feature_of<tag::weighted_p_square_quantile>
  : feature_of<tag::p_square_quantile>
{
};

}} // namespace boost::accumulators

#endif
