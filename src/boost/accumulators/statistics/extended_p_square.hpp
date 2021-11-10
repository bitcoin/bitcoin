///////////////////////////////////////////////////////////////////////////////
// extended_p_square.hpp
//
//  Copyright 2005 Daniel Egloff. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_EXTENDED_SINGLE_HPP_DE_01_01_2006
#define BOOST_ACCUMULATORS_STATISTICS_EXTENDED_SINGLE_HPP_DE_01_01_2006

#include <vector>
#include <functional>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/accumulators/accumulators_fwd.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/times2_iterator.hpp>
#include <boost/serialization/vector.hpp>

namespace boost { namespace accumulators
{
///////////////////////////////////////////////////////////////////////////////
// probabilities named parameter
//
BOOST_PARAMETER_NESTED_KEYWORD(tag, extended_p_square_probabilities, probabilities)

BOOST_ACCUMULATORS_IGNORE_GLOBAL(extended_p_square_probabilities)

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // extended_p_square_impl
    //  multiple quantile estimation
    /**
        @brief Multiple quantile estimation with the extended \f$P^2\f$ algorithm

        Extended \f$P^2\f$ algorithm for estimation of several quantiles without storing samples.
        Assume that \f$m\f$ quantiles \f$\xi_{p_1}, \ldots, \xi_{p_m}\f$ are to be estimated.
        Instead of storing the whole sample cumulative distribution, the algorithm maintains only
        \f$m+2\f$ principal markers and \f$m+1\f$ middle markers, whose positions are updated
        with each sample and whose heights are adjusted (if necessary) using a piecewise-parablic
        formula. The heights of these central markers are the current estimates of the quantiles
        and returned as an iterator range.

        For further details, see

        K. E. E. Raatikainen, Simultaneous estimation of several quantiles, Simulation, Volume 49,
        Number 4 (October), 1986, p. 159-164.

        The extended \f$ P^2 \f$ algorithm generalizes the \f$ P^2 \f$ algorithm of

        R. Jain and I. Chlamtac, The P^2 algorithm for dynamic calculation of quantiles and
        histograms without storing observations, Communications of the ACM,
        Volume 28 (October), Number 10, 1985, p. 1076-1085.

        @param extended_p_square_probabilities A vector of quantile probabilities.
    */
    template<typename Sample>
    struct extended_p_square_impl
      : accumulator_base
    {
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type float_type;
        typedef std::vector<float_type> array_type;
        // for boost::result_of
        typedef iterator_range<
            detail::lvalue_index_iterator<
                permutation_iterator<
                    typename array_type::const_iterator
                  , detail::times2_iterator
                >
            >
        > result_type;

        template<typename Args>
        extended_p_square_impl(Args const &args)
          : probabilities(
                boost::begin(args[extended_p_square_probabilities])
              , boost::end(args[extended_p_square_probabilities])
            )
          , heights(2 * probabilities.size() + 3)
          , actual_positions(heights.size())
          , desired_positions(heights.size())
          , positions_increments(heights.size())
        {
            std::size_t num_quantiles = this->probabilities.size();
            std::size_t num_markers = this->heights.size();

            for(std::size_t i = 0; i < num_markers; ++i)
            {
                this->actual_positions[i] = i + 1;
            }

            this->positions_increments[0] = 0.;
            this->positions_increments[num_markers - 1] = 1.;

            for(std::size_t i = 0; i < num_quantiles; ++i)
            {
                this->positions_increments[2 * i + 2] = probabilities[i];
            }

            for(std::size_t i = 0; i <= num_quantiles; ++i)
            {
                this->positions_increments[2 * i + 1] =
                    0.5 * (this->positions_increments[2 * i] + this->positions_increments[2 * i + 2]);
            }

            for(std::size_t i = 0; i < num_markers; ++i)
            {
                this->desired_positions[i] = 1. + 2. * (num_quantiles + 1.) * this->positions_increments[i];
            }
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            std::size_t cnt = count(args);

            // m+2 principal markers and m+1 middle markers
            std::size_t num_markers = 2 * this->probabilities.size() + 3;

            // first accumulate num_markers samples
            if(cnt <= num_markers)
            {
                this->heights[cnt - 1] = args[sample];

                // complete the initialization of heights by sorting
                if(cnt == num_markers)
                {
                    std::sort(this->heights.begin(), this->heights.end());
                }
            }
            else
            {
                std::size_t sample_cell = 1;

                // find cell k = sample_cell such that heights[k-1] <= sample < heights[k]
                if(args[sample] < this->heights[0])
                {
                    this->heights[0] = args[sample];
                    sample_cell = 1;
                }
                else if(args[sample] >= this->heights[num_markers - 1])
                {
                    this->heights[num_markers - 1] = args[sample];
                    sample_cell = num_markers - 1;
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

                // update actual positions of all markers above sample_cell index
                for(std::size_t i = sample_cell; i < num_markers; ++i)
                {
                    ++this->actual_positions[i];
                }

                // update desired positions of all markers
                for(std::size_t i = 0; i < num_markers; ++i)
                {
                    this->desired_positions[i] += this->positions_increments[i];
                }

                // adjust heights and actual positions of markers 1 to num_markers-2 if necessary
                for(std::size_t i = 1; i <= num_markers - 2; ++i)
                {
                    // offset to desired position
                    float_type d = this->desired_positions[i] - this->actual_positions[i];

                    // offset to next position
                    float_type dp = this->actual_positions[i+1] - this->actual_positions[i];

                    // offset to previous position
                    float_type dm = this->actual_positions[i-1] - this->actual_positions[i];

                    // height ds
                    float_type hp = (this->heights[i+1] - this->heights[i]) / dp;
                    float_type hm = (this->heights[i-1] - this->heights[i]) / dm;

                    if((d >= 1 && dp > 1) || (d <= -1 && dm < -1))
                    {
                        short sign_d = static_cast<short>(d / std::abs(d));

                        float_type h = this->heights[i] + sign_d / (dp - dm) * ((sign_d - dm)*hp
                                     + (dp - sign_d) * hm);

                        // try adjusting heights[i] using p-squared formula
                        if(this->heights[i - 1] < h && h < this->heights[i + 1])
                        {
                            this->heights[i] = h;
                        }
                        else
                        {
                            // use linear formula
                            if(d > 0)
                            {
                                this->heights[i] += hp;
                            }
                            if(d < 0)
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
            // for i in [1,probabilities.size()], return heights[i * 2]
            detail::times2_iterator idx_begin = detail::make_times2_iterator(1);
            detail::times2_iterator idx_end = detail::make_times2_iterator(this->probabilities.size() + 1);

            return result_type(
                make_permutation_iterator(this->heights.begin(), idx_begin)
              , make_permutation_iterator(this->heights.begin(), idx_end)
            );
        }

    public:
        // make this accumulator serializeable
        // TODO: do we need to split to load/save and verify that the parameters did not change?
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & probabilities;
            ar & heights;
            ar & actual_positions;
            ar & desired_positions;
            ar & positions_increments;
        }

    private:
        array_type probabilities;         // the quantile probabilities
        array_type heights;               // q_i
        array_type actual_positions;      // n_i
        array_type desired_positions;     // d_i
        array_type positions_increments;  // f_i
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::extended_p_square
//
namespace tag
{
    struct extended_p_square
      : depends_on<count>
      , extended_p_square_probabilities
    {
        typedef accumulators::impl::extended_p_square_impl<mpl::_1> impl;

        #ifdef BOOST_ACCUMULATORS_DOXYGEN_INVOKED
        /// tag::extended_p_square::probabilities named parameter
        static boost::parameter::keyword<tag::probabilities> const probabilities;
        #endif
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::extended_p_square
//
namespace extract
{
    extractor<tag::extended_p_square> const extended_p_square = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(extended_p_square)
}

using extract::extended_p_square;

// So that extended_p_square can be automatically substituted with
// weighted_extended_p_square when the weight parameter is non-void
template<>
struct as_weighted_feature<tag::extended_p_square>
{
    typedef tag::weighted_extended_p_square type;
};

template<>
struct feature_of<tag::weighted_extended_p_square>
  : feature_of<tag::extended_p_square>
{
};

}} // namespace boost::accumulators

#endif
