/* boost random/hyperexponential_distribution.hpp header file
 *
 * Copyright Marco Guazzone 2014
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * Much of the code here taken by boost::math::hyperexponential_distribution.
 * To this end, we would like to thank Paul Bristow and John Maddock for their
 * valuable feedback.
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */

#ifndef BOOST_RANDOM_HYPEREXPONENTIAL_DISTRIBUTION_HPP
#define BOOST_RANDOM_HYPEREXPONENTIAL_DISTRIBUTION_HPP


#include <boost/config.hpp>
#include <boost/core/cmath.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/vector_io.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/type_traits/has_pre_increment.hpp>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iterator>
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
# include <initializer_list>
#endif // BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#include <iostream>
#include <limits>
#include <numeric>
#include <vector>


namespace boost { namespace random {

namespace hyperexp_detail {

template <typename T>
std::vector<T>& normalize(std::vector<T>& v)
{
    if (v.size() == 0)
    {
        return v;
    }

    const T sum = std::accumulate(v.begin(), v.end(), static_cast<T>(0));
    T final_sum = 0;

    const typename std::vector<T>::iterator end = --v.end();
    for (typename std::vector<T>::iterator it = v.begin();
         it != end;
         ++it)
    {
        *it /= sum;
        final_sum += *it;
    }
    *end = 1-final_sum; // avoids round off errors thus ensuring the probabilities really sum to 1

    return v;
}

template <typename RealT>
bool check_probabilities(std::vector<RealT> const& probabilities)
{
    const std::size_t n = probabilities.size();
    RealT sum = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        if (probabilities[i] < 0
            || probabilities[i] > 1
            || !(boost::core::isfinite)(probabilities[i]))
        {
            return false;
        }
        sum += probabilities[i];
    }

    //NOTE: the check below seems to fail on some architectures.
    //      So we commented it.
    //// - We try to keep phase probabilities correctly normalized in the distribution constructors
    //// - However in practice we have to allow for a very slight divergence from a sum of exactly 1:
    ////if (std::abs(sum-1) > (std::numeric_limits<RealT>::epsilon()*2))
    //// This is from Knuth "The Art of Computer Programming: Vol.2, 3rd Ed", and can be used to
    //// check is two numbers are approximately equal
    //const RealT one = 1;
    //const RealT tol = std::numeric_limits<RealT>::epsilon()*2.0;
    //if (std::abs(sum-one) > (std::max(std::abs(sum), std::abs(one))*tol))
    //{
    //    return false;
    //}

    return true;
}

template <typename RealT>
bool check_rates(std::vector<RealT> const& rates)
{
    const std::size_t n = rates.size();
    for (std::size_t i = 0; i < n; ++i)
    {
        if (rates[i] <= 0
            || !(boost::core::isfinite)(rates[i]))
        {
            return false;
        }
    }
    return true;
}

template <typename RealT>
bool check_params(std::vector<RealT> const& probabilities, std::vector<RealT> const& rates)
{
    if (probabilities.size() != rates.size())
    {
        return false;
    }

    return check_probabilities(probabilities)
           && check_rates(rates);
}

} // Namespace hyperexp_detail


/**
 * The hyperexponential distribution is a real-valued continuous distribution
 * with two parameters, the <em>phase probability vector</em> \c probs and the
 * <em>rate vector</em> \c rates.
 *
 * A \f$k\f$-phase hyperexponential distribution is a mixture of \f$k\f$
 * exponential distributions.
 * For this reason, it is also referred to as <em>mixed exponential
 * distribution</em> or <em>parallel \f$k\f$-phase exponential
 * distribution</em>.
 *
 * A \f$k\f$-phase hyperexponential distribution is characterized by two
 * parameters, namely a <em>phase probability vector</em> \f$\mathbf{\alpha}=(\alpha_1,\ldots,\alpha_k)\f$ and a <em>rate vector</em> \f$\mathbf{\lambda}=(\lambda_1,\ldots,\lambda_k)\f$.
 *
 * A \f$k\f$-phase hyperexponential distribution is frequently used in
 * <em>queueing theory</em> to model the distribution of the superposition of
 * \f$k\f$ independent events, like, for instance, the  service time distribution
 * of a queueing station with \f$k\f$ servers in parallel where the \f$i\f$-th
 * server is chosen with probability \f$\alpha_i\f$ and its service time
 * distribution is an exponential distribution with rate \f$\lambda_i\f$
 * (Allen,1990; Papadopolous et al.,1993; Trivedi,2002).
 *
 * For instance, CPUs service-time distribution in a computing system has often
 * been observed to possess such a distribution (Rosin,1965).
 * Also, the arrival of different types of customer to a single queueing station
 * is often modeled as a hyperexponential distribution (Papadopolous et al.,1993).
 * Similarly, if a product manufactured in several parallel assemply lines and
 * the outputs are merged, the failure density of the overall product is likely
 * to be hyperexponential (Trivedi,2002).
 *
 * Finally, since the hyperexponential distribution exhibits a high Coefficient
 * of Variation (CoV), that is a CoV > 1, it is especially suited to fit
 * empirical data with large CoV (Feitelson,2014; Wolski et al.,2013) and to
 * approximate <em>long-tail probability distributions</em> (Feldmann et al.,1998).
 *
 * See (Boost,2014) for more information and examples.
 *
 * A \f$k\f$-phase hyperexponential distribution has a probability density
 * function
 * \f[
 *  f(x) = \sum_{i=1}^k \alpha_i \lambda_i e^{-x\lambda_i}
 * \f]
 * where:
 * - \f$k\f$ is the <em>number of phases</em> and also the size of the input
 *   vector parameters,
 * - \f$\mathbf{\alpha}=(\alpha_1,\ldots,\alpha_k)\f$ is the <em>phase probability
 *   vector</em> parameter, and
 * - \f$\mathbf{\lambda}=(\lambda_1,\ldots,\lambda_k)\f$ is the <em>rate vector</em>
 *   parameter.
 * .
 *
 * Given a \f$k\f$-phase hyperexponential distribution with phase probability
 * vector \f$\mathbf{\alpha}\f$ and rate vector \f$\mathbf{\lambda}\f$, the
 * random variate generation algorithm consists of the following steps (Tyszer,1999):
 * -# Generate a random variable \f$U\f$ uniformly distribution on the interval \f$(0,1)\f$.
 * -# Use \f$U\f$ to select the appropriate \f$\lambda_i\f$ (e.g., the
 *  <em>alias method</em> can possibly be used for this step).
 * -# Generate an exponentially distributed random variable \f$X\f$ with rate parameter \f$\lambda_i\f$.
 * -# Return \f$X\f$.
 * .
 *
 * References:
 * -# A.O. Allen, <em>Probability, Statistics, and Queuing Theory with Computer Science Applications, Second Edition</em>, Academic Press, 1990.
 * -# Boost C++ Libraries, <em>Boost.Math / Statistical Distributions: Hyperexponential Distribution</em>, Online: http://www.boost.org/doc/libs/release/libs/math/doc/html/dist.html , 2014.
 * -# D.G. Feitelson, <em>Workload Modeling for Computer Systems Performance Evaluation</em>, Cambridge University Press, 2014
 * -# A. Feldmann and W. Whitt, <em>Fitting mixtures of exponentials to long-tail distributions to analyze network performance models</em>, Performance Evaluation 31(3-4):245, doi:10.1016/S0166-5316(97)00003-5, 1998.
 * -# H.T. Papadopolous, C. Heavey and J. Browne, <em>Queueing Theory in Manufacturing Systems Analysis and Design</em>, Chapman & Hall/CRC, 1993, p. 35.
 * -# R.F. Rosin, <em>Determining a computing center environment</em>, Communications of the ACM 8(7):463-468, 1965.
 * -# K.S. Trivedi, <em>Probability and Statistics with Reliability, Queueing, and Computer Science Applications</em>, John Wiley & Sons, Inc., 2002.
 * -# J. Tyszer, <em>Object-Oriented Computer Simulation of Discrete-Event Systems</em>, Springer, 1999.
 * -# Wikipedia, <em>Hyperexponential Distribution</em>, Online: http://en.wikipedia.org/wiki/Hyperexponential_distribution , 2014.
 * -# Wolfram Mathematica, <em>Hyperexponential Distribution</em>, Online: http://reference.wolfram.com/language/ref/HyperexponentialDistribution.html , 2014.
 * .
 *
 * \author Marco Guazzone (marco.guazzone@gmail.com)
 */
template<class RealT = double>
class hyperexponential_distribution
{
    public: typedef RealT result_type;
    public: typedef RealT input_type;


    /**
     * The parameters of a hyperexponential distribution.
     *
     * Stores the <em>phase probability vector</em> and the <em>rate vector</em>
     * of the hyperexponential distribution.
     *
     * \author Marco Guazzone (marco.guazzone@gmail.com)
     */
    public: class param_type
    {
        public: typedef hyperexponential_distribution distribution_type;

        /**
         * Constructs a \c param_type with the default parameters
         * of the distribution.
         */
        public: param_type()
        : probs_(1, 1),
          rates_(1, 1)
        {
        }

        /**
         * Constructs a \c param_type from the <em>phase probability vector</em>
         * and <em>rate vector</em> parameters of the distribution.
         *
         * The <em>phase probability vector</em> parameter is given by the range
         * defined by [\a prob_first, \a prob_last) iterator pair, and the
         * <em>rate vector</em> parameter is given by the range defined by
         * [\a rate_first, \a rate_last) iterator pair.
         *
         * \tparam ProbIterT Must meet the requirements of \c InputIterator concept (ISO,2014,sec. 24.2.3 [input.iterators]).
         * \tparam RateIterT Must meet the requirements of \c InputIterator concept (ISO,2014,sec. 24.2.3 [input.iterators]).
         *
         * \param prob_first The iterator to the beginning of the range of non-negative real elements representing the phase probabilities; if elements don't sum to 1, they are normalized.
         * \param prob_last The iterator to the ending of the range of non-negative real elements representing the phase probabilities; if elements don't sum to 1, they are normalized.
         * \param rate_first The iterator to the beginning of the range of non-negative real elements representing the rates.
         * \param rate_last The iterator to the ending of the range of non-negative real elements representing the rates.
         *
         * References:
         * -# ISO, <em>ISO/IEC 14882-2014: Information technology - Programming languages - C++</em>, 2014
         * .
         */
        public: template <typename ProbIterT, typename RateIterT>
                param_type(ProbIterT prob_first, ProbIterT prob_last,
                           RateIterT rate_first, RateIterT rate_last)
        : probs_(prob_first, prob_last),
          rates_(rate_first, rate_last)
        {
            hyperexp_detail::normalize(probs_);

            assert( hyperexp_detail::check_params(probs_, rates_) );
        }

        /**
         * Constructs a \c param_type from the <em>phase probability vector</em>
         * and <em>rate vector</em> parameters of the distribution.
         *
         * The <em>phase probability vector</em> parameter is given by the range
         * defined by \a prob_range, and the <em>rate vector</em> parameter is
         * given by the range defined by \a rate_range.
         *
         * \tparam ProbRangeT Must meet the requirements of <a href="boost:/libs/range/doc/html/range/concepts.html">Range</a> concept.
         * \tparam RateRangeT Must meet the requirements of <a href="boost:/libs/range/doc/html/range/concepts.html">Range</a> concept.
         *
         * \param prob_range The range of non-negative real elements representing the phase probabilities; if elements don't sum to 1, they are normalized.
         * \param rate_range The range of positive real elements representing the rates.
         *
         * \note
         *  The final \c disable_if parameter is an implementation detail that
         *  differentiates between this two argument constructor and the
         *  iterator-based two argument constructor described below.
         */
        //  We SFINAE this out of existance if either argument type is
        //  incrementable as in that case the type is probably an iterator:
        public: template <typename ProbRangeT, typename RateRangeT>
                param_type(ProbRangeT const& prob_range,
                           RateRangeT const& rate_range,
                           typename boost::disable_if_c<boost::has_pre_increment<ProbRangeT>::value || boost::has_pre_increment<RateRangeT>::value>::type* = 0)
        : probs_(boost::begin(prob_range), boost::end(prob_range)),
          rates_(boost::begin(rate_range), boost::end(rate_range))
        {
            hyperexp_detail::normalize(probs_);

            assert( hyperexp_detail::check_params(probs_, rates_) );
        }

        /**
         * Constructs a \c param_type from the <em>rate vector</em> parameter of
         * the distribution and with equal phase probabilities.
         *
         * The <em>rate vector</em> parameter is given by the range defined by
         * [\a rate_first, \a rate_last) iterator pair, and the <em>phase
         * probability vector</em> parameter is set to the equal phase
         * probabilities (i.e., to a vector of the same length \f$k\f$ of the
         * <em>rate vector</em> and with each element set to \f$1.0/k\f$).
         *
         * \tparam RateIterT Must meet the requirements of \c InputIterator concept (ISO,2014,sec. 24.2.3 [input.iterators]).
         * \tparam RateIterT2 Must meet the requirements of \c InputIterator concept (ISO,2014,sec. 24.2.3 [input.iterators]).
         *
         * \param rate_first The iterator to the beginning of the range of non-negative real elements representing the rates.
         * \param rate_last The iterator to the ending of the range of non-negative real elements representing the rates.
         *
         * \note
         *  The final \c disable_if parameter is an implementation detail that
         *  differentiates between this two argument constructor and the
         *  range-based two argument constructor described above.
         *
         * References:
         * -# ISO, <em>ISO/IEC 14882-2014: Information technology - Programming languages - C++</em>, 2014
         * .
         */
        //  We SFINAE this out of existance if the argument type is
        //  incrementable as in that case the type is probably an iterator.
        public: template <typename RateIterT>
                param_type(RateIterT rate_first, 
                           RateIterT rate_last,  
                           typename boost::enable_if_c<boost::has_pre_increment<RateIterT>::value>::type* = 0)
        : probs_(std::distance(rate_first, rate_last), 1), // will be normalized below
          rates_(rate_first, rate_last)
        {
            assert(probs_.size() == rates_.size());
        }

        /**
         * Constructs a @c param_type from the "rates" parameters
         * of the distribution and with equal phase probabilities.
         *
         * The <em>rate vector</em> parameter is given by the range defined by
         * \a rate_range, and the <em>phase probability vector</em> parameter is
         * set to the equal phase probabilities (i.e., to a vector of the same
         * length \f$k\f$ of the <em>rate vector</em> and with each element set
         * to \f$1.0/k\f$).
         *
         * \tparam RateRangeT Must meet the requirements of <a href="boost:/libs/range/doc/html/range/concepts.html">Range</a> concept.
         *
         * \param rate_range The range of positive real elements representing the rates.
         */
        public: template <typename RateRangeT>
                param_type(RateRangeT const& rate_range)
        : probs_(boost::size(rate_range), 1), // Will be normalized below
          rates_(boost::begin(rate_range), boost::end(rate_range))
        {
            hyperexp_detail::normalize(probs_);

            assert( hyperexp_detail::check_params(probs_, rates_) );
        }

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
        /**
         * Constructs a \c param_type from the <em>phase probability vector</em>
         * and <em>rate vector</em> parameters of the distribution.
         *
         * The <em>phase probability vector</em> parameter is given by the
         * <em>brace-init-list</em> (ISO,2014,sec. 8.5.4 [dcl.init.list])
         * defined by \a l1, and the <em>rate vector</em> parameter is given by the
         * <em>brace-init-list</em> (ISO,2014,sec. 8.5.4 [dcl.init.list])
         * defined by \a l2.
         *
         * \param l1 The initializer list for inizializing the phase probability vector.
         * \param l2 The initializer list for inizializing the rate vector.
         *
         * References:
         * -# ISO, <em>ISO/IEC 14882-2014: Information technology - Programming languages - C++</em>, 2014
         * .
         */
        public: param_type(std::initializer_list<RealT> l1, std::initializer_list<RealT> l2)
        : probs_(l1.begin(), l1.end()),
          rates_(l2.begin(), l2.end())
        {
            hyperexp_detail::normalize(probs_);

            assert( hyperexp_detail::check_params(probs_, rates_) );
        }

        /**
         * Constructs a \c param_type from the <em>rate vector</em> parameter
         * of the distribution and with equal phase probabilities.
         *
         * The <em>rate vector</em> parameter is given by the
         * <em>brace-init-list</em> (ISO,2014,sec. 8.5.4 [dcl.init.list])
         * defined by \a l1, and the <em>phase probability vector</em> parameter is
         * set to the equal phase probabilities (i.e., to a vector of the same
         * length \f$k\f$ of the <em>rate vector</em> and with each element set
         * to \f$1.0/k\f$).
         *
         * \param l1 The initializer list for inizializing the rate vector.
         *
         * References:
         * -# ISO, <em>ISO/IEC 14882-2014: Information technology - Programming languages - C++</em>, 2014
         * .
         */
        public: param_type(std::initializer_list<RealT> l1)
        : probs_(std::distance(l1.begin(), l1.end()), 1), // Will be normalized below
          rates_(l1.begin(), l1.end())
        {
            hyperexp_detail::normalize(probs_);

            assert( hyperexp_detail::check_params(probs_, rates_) );
        }
#endif // BOOST_NO_CXX11_HDR_INITIALIZER_LIST

        /**
         * Gets the <em>phase probability vector</em> parameter of the distribtuion.
         *
         * \return The <em>phase probability vector</em> parameter of the distribution.
         *
         * \note
         *  The returned probabilities are the normalized version of the ones
         *  passed at construction time.
         */
        public: std::vector<RealT> probabilities() const
        {
            return probs_;
        }

        /**
         * Gets the <em>rate vector</em> parameter of the distribtuion.
         *
         * \return The <em>rate vector</em> parameter of the distribution.
         */
        public: std::vector<RealT> rates() const
        {
            return rates_;
        }

        /** Writes a \c param_type to a \c std::ostream. */
        public: BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, param)
        {
            detail::print_vector(os, param.probs_);
            os << ' ';
            detail::print_vector(os, param.rates_);

            return os;
        }

        /** Reads a \c param_type from a \c std::istream. */
        public: BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, param)
        {
            // NOTE: if \c std::ios_base::exceptions is set, the code below may
            //       throw in case of a I/O failure.
            //       To prevent leaving the state of \c param inconsistent:
            //       - if an exception is thrown, the state of \c param is left
            //         unchanged (i.e., is the same as the one at the beginning
            //         of the function's execution), and
            //       - the state of \c param only after reading the whole input.

            std::vector<RealT> probs;
            std::vector<RealT> rates;

            // Reads probability and rate vectors
            detail::read_vector(is, probs);
            if (!is)
            {
                return is;
            }
            is >> std::ws;
            detail::read_vector(is, rates);
            if (!is)
            {
                return is;
            }

            // Update the state of the param_type object
            if (probs.size() > 0)
            {
                param.probs_.swap(probs);
                probs.clear();
            }
            if (rates.size() > 0)
            {
                param.rates_.swap(rates);
                rates.clear();
            }

            // Adjust vector sizes (if needed)
            if (param.probs_.size() != param.rates_.size()
                || param.probs_.size() == 0)
            {
                const std::size_t np = param.probs_.size();
                const std::size_t nr = param.rates_.size();

                if (np > nr)
                {
                    param.rates_.resize(np, 1);
                }
                else if (nr > np)
                {
                    param.probs_.resize(nr, 1);
                }
                else
                {
                    param.probs_.resize(1, 1);
                    param.rates_.resize(1, 1);
                }
            }

            // Normalize probabilities
            // NOTE: this cannot be done earlier since the probability vector
            //       can be changed due to size conformance
            hyperexp_detail::normalize(param.probs_);

            //post: vector size conformance
            assert(param.probs_.size() == param.rates_.size());

            return is;
        }

        /** Returns true if the two sets of parameters are the same. */
        public: BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        {
            return lhs.probs_ == rhs.probs_
                   && lhs.rates_ == rhs.rates_;
        }
        
        /** Returns true if the two sets of parameters are the different. */
        public: BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)


        private: std::vector<RealT> probs_; ///< The <em>phase probability vector</em> parameter of the distribution
        private: std::vector<RealT> rates_; ///< The <em>rate vector</em> parameter of the distribution
    }; // param_type


    /**
     * Constructs a 1-phase \c hyperexponential_distribution (i.e., an
     * exponential distribution) with rate 1.
     */
    public: hyperexponential_distribution()
    : dd_(std::vector<RealT>(1, 1)),
      rates_(1, 1)
    {
        // empty
    }

    /**
     * Constructs a \c hyperexponential_distribution from the <em>phase
     * probability vector</em> and <em>rate vector</em> parameters of the
     * distribution.
     *
     * The <em>phase probability vector</em> parameter is given by the range
     * defined by [\a prob_first, \a prob_last) iterator pair, and the
     * <em>rate vector</em> parameter is given by the range defined by
     * [\a rate_first, \a rate_last) iterator pair.
     *
     * \tparam ProbIterT Must meet the requirements of \c InputIterator concept (ISO,2014,sec. 24.2.3 [input.iterators]).
     * \tparam RateIterT Must meet the requirements of \c InputIterator concept (ISO,2014,sec. 24.2.3 [input.iterators]).
     *
     * \param prob_first The iterator to the beginning of the range of non-negative real elements representing the phase probabilities; if elements don't sum to 1, they are normalized.
     * \param prob_last The iterator to the ending of the range of non-negative real elements representing the phase probabilities; if elements don't sum to 1, they are normalized.
     * \param rate_first The iterator to the beginning of the range of non-negative real elements representing the rates.
     * \param rate_last The iterator to the ending of the range of non-negative real elements representing the rates.
     *
     * References:
     * -# ISO, <em>ISO/IEC 14882-2014: Information technology - Programming languages - C++</em>, 2014
     * .
     */
    public: template <typename ProbIterT, typename RateIterT>
            hyperexponential_distribution(ProbIterT prob_first, ProbIterT prob_last,
                                          RateIterT rate_first, RateIterT rate_last)
    : dd_(prob_first, prob_last),
      rates_(rate_first, rate_last)
    {
        assert( hyperexp_detail::check_params(dd_.probabilities(), rates_) );
    }

    /**
     * Constructs a \c hyperexponential_distribution from the <em>phase
     * probability vector</em> and <em>rate vector</em> parameters of the
     * distribution.
     *
     * The <em>phase probability vector</em> parameter is given by the range
     * defined by \a prob_range, and the <em>rate vector</em> parameter is
     * given by the range defined by \a rate_range.
     *
     * \tparam ProbRangeT Must meet the requirements of <a href="boost:/libs/range/doc/html/range/concepts.html">Range</a> concept.
     * \tparam RateRangeT Must meet the requirements of <a href="boost:/libs/range/doc/html/range/concepts.html">Range</a> concept.
     *
     * \param prob_range The range of non-negative real elements representing the phase probabilities; if elements don't sum to 1, they are normalized.
     * \param rate_range The range of positive real elements representing the rates.
     *
     * \note
     *  The final \c disable_if parameter is an implementation detail that
     *  differentiates between this two argument constructor and the
     *  iterator-based two argument constructor described below.
     */
    //  We SFINAE this out of existance if either argument type is
    //  incrementable as in that case the type is probably an iterator:
    public: template <typename ProbRangeT, typename RateRangeT>
            hyperexponential_distribution(ProbRangeT const& prob_range,
                                          RateRangeT const& rate_range,
                                          typename boost::disable_if_c<boost::has_pre_increment<ProbRangeT>::value || boost::has_pre_increment<RateRangeT>::value>::type* = 0)
    : dd_(prob_range),
      rates_(boost::begin(rate_range), boost::end(rate_range))
    {
        assert( hyperexp_detail::check_params(dd_.probabilities(), rates_) );
    }

    /**
     * Constructs a \c hyperexponential_distribution from the <em>rate
     * vector</em> parameter of the distribution and with equal phase
     * probabilities.
     *
     * The <em>rate vector</em> parameter is given by the range defined by
     * [\a rate_first, \a rate_last) iterator pair, and the <em>phase
     * probability vector</em> parameter is set to the equal phase
     * probabilities (i.e., to a vector of the same length \f$k\f$ of the
     * <em>rate vector</em> and with each element set to \f$1.0/k\f$).
     *
     * \tparam RateIterT Must meet the requirements of \c InputIterator concept (ISO,2014,sec. 24.2.3 [input.iterators]).
     * \tparam RateIterT2 Must meet the requirements of \c InputIterator concept (ISO,2014,sec. 24.2.3 [input.iterators]).
     *
     * \param rate_first The iterator to the beginning of the range of non-negative real elements representing the rates.
     * \param rate_last The iterator to the ending of the range of non-negative real elements representing the rates.
     *
     * \note
     *  The final \c disable_if parameter is an implementation detail that
     *  differentiates between this two argument constructor and the
     *  range-based two argument constructor described above.
     *
     * References:
     * -# ISO, <em>ISO/IEC 14882-2014: Information technology - Programming languages - C++</em>, 2014
     * .
     */
    //  We SFINAE this out of existance if the argument type is
    //  incrementable as in that case the type is probably an iterator.
    public: template <typename RateIterT>
            hyperexponential_distribution(RateIterT rate_first,
                                          RateIterT rate_last,
                                          typename boost::enable_if_c<boost::has_pre_increment<RateIterT>::value>::type* = 0)
    : dd_(std::vector<RealT>(std::distance(rate_first, rate_last), 1)),
      rates_(rate_first, rate_last)
    {
        assert( hyperexp_detail::check_params(dd_.probabilities(), rates_) );
    }

    /**
     * Constructs a @c param_type from the "rates" parameters
     * of the distribution and with equal phase probabilities.
     *
     * The <em>rate vector</em> parameter is given by the range defined by
     * \a rate_range, and the <em>phase probability vector</em> parameter is
     * set to the equal phase probabilities (i.e., to a vector of the same
     * length \f$k\f$ of the <em>rate vector</em> and with each element set
     * to \f$1.0/k\f$).
     *
     * \tparam RateRangeT Must meet the requirements of <a href="boost:/libs/range/doc/html/range/concepts.html">Range</a> concept.
     *
     * \param rate_range The range of positive real elements representing the rates.
     */
    public: template <typename RateRangeT>
            hyperexponential_distribution(RateRangeT const& rate_range)
    : dd_(std::vector<RealT>(boost::size(rate_range), 1)),
      rates_(boost::begin(rate_range), boost::end(rate_range))
    {
        assert( hyperexp_detail::check_params(dd_.probabilities(), rates_) );
    }

    /**
     * Constructs a \c hyperexponential_distribution from its parameters.
     *
     * \param param The parameters of the distribution.
     */
    public: explicit hyperexponential_distribution(param_type const& param)
    : dd_(param.probabilities()),
      rates_(param.rates())
    {
        assert( hyperexp_detail::check_params(dd_.probabilities(), rates_) );
    }

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    /**
     * Constructs a \c hyperexponential_distribution from the <em>phase
     * probability vector</em> and <em>rate vector</em> parameters of the
     * distribution.
     *
     * The <em>phase probability vector</em> parameter is given by the
     * <em>brace-init-list</em> (ISO,2014,sec. 8.5.4 [dcl.init.list])
     * defined by \a l1, and the <em>rate vector</em> parameter is given by the
     * <em>brace-init-list</em> (ISO,2014,sec. 8.5.4 [dcl.init.list])
     * defined by \a l2.
     *
     * \param l1 The initializer list for inizializing the phase probability vector.
     * \param l2 The initializer list for inizializing the rate vector.
     *
     * References:
     * -# ISO, <em>ISO/IEC 14882-2014: Information technology - Programming languages - C++</em>, 2014
     * .
     */
    public: hyperexponential_distribution(std::initializer_list<RealT> const& l1, std::initializer_list<RealT> const& l2)
    : dd_(l1.begin(), l1.end()),
      rates_(l2.begin(), l2.end())
    {
        assert( hyperexp_detail::check_params(dd_.probabilities(), rates_) );
    }

    /**
     * Constructs a \c hyperexponential_distribution from the <em>rate
     * vector</em> parameter of the distribution and with equal phase
     * probabilities.
     *
     * The <em>rate vector</em> parameter is given by the
     * <em>brace-init-list</em> (ISO,2014,sec. 8.5.4 [dcl.init.list])
     * defined by \a l1, and the <em>phase probability vector</em> parameter is
     * set to the equal phase probabilities (i.e., to a vector of the same
     * length \f$k\f$ of the <em>rate vector</em> and with each element set
     * to \f$1.0/k\f$).
     *
     * \param l1 The initializer list for inizializing the rate vector.
     *
     * References:
     * -# ISO, <em>ISO/IEC 14882-2014: Information technology - Programming languages - C++</em>, 2014
     * .
     */
    public: hyperexponential_distribution(std::initializer_list<RealT> const& l1)
    : dd_(std::vector<RealT>(std::distance(l1.begin(), l1.end()), 1)),
      rates_(l1.begin(), l1.end())
    {
        assert( hyperexp_detail::check_params(dd_.probabilities(), rates_) );
    }
#endif

    /**
     * Gets a random variate distributed according to the
     * hyperexponential distribution.
     *
     * \tparam URNG Must meet the requirements of \uniform_random_number_generator.
     *
     * \param urng A uniform random number generator object.
     *
     * \return A random variate distributed according to the hyperexponential distribution.
     */
    public: template<class URNG>\
            RealT operator()(URNG& urng) const
    {
        const int i = dd_(urng);

        return boost::random::exponential_distribution<RealT>(rates_[i])(urng);
    }

    /**
     * Gets a random variate distributed according to the hyperexponential
     * distribution with parameters specified by \c param.
     *
     * \tparam URNG Must meet the requirements of \uniform_random_number_generator.
     *
     * \param urng A uniform random number generator object.
     * \param param A distribution parameter object.
     *
     * \return A random variate distributed according to the hyperexponential distribution.
     *  distribution with parameters specified by \c param.
     */
    public: template<class URNG>
            RealT operator()(URNG& urng, const param_type& param) const
    {
        return hyperexponential_distribution(param)(urng);
    }

    /** Returns the number of phases of the distribution. */
    public: std::size_t num_phases() const
    {
        return rates_.size();
    }

    /** Returns the <em>phase probability vector</em> parameter of the distribution. */
    public: std::vector<RealT> probabilities() const
    {
        return dd_.probabilities();
    }

    /** Returns the <em>rate vector</em> parameter of the distribution. */
    public: std::vector<RealT> rates() const
    {
        return rates_;
    }

    /** Returns the smallest value that the distribution can produce. */
    public: RealT min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    {
        return 0;
    }

    /** Returns the largest value that the distribution can produce. */
    public: RealT max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    {
        return std::numeric_limits<RealT>::infinity();
    }

    /** Returns the parameters of the distribution. */
    public: param_type param() const
    {
        std::vector<RealT> probs = dd_.probabilities();

        return param_type(probs.begin(), probs.end(), rates_.begin(), rates_.end());
    }

    /** Sets the parameters of the distribution. */
    public: void param(param_type const& param)
    {
        dd_.param(typename boost::random::discrete_distribution<int,RealT>::param_type(param.probabilities()));
        rates_ = param.rates();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    public: void reset()
    {
        // empty
    }

    /** Writes an @c hyperexponential_distribution to a @c std::ostream. */
    public: BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, hyperexponential_distribution, hd)
    {
        os << hd.param();
        return os;
    }

    /** Reads an @c hyperexponential_distribution from a @c std::istream. */
    public: BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, hyperexponential_distribution, hd)
    {
        param_type param;
        if(is >> param)
        {
            hd.param(param);
        }
        return is;
    }

    /**
     * Returns true if the two instances of @c hyperexponential_distribution will
     * return identical sequences of values given equal generators.
     */
    public: BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(hyperexponential_distribution, lhs, rhs)
    {
        return lhs.dd_ == rhs.dd_
               && lhs.rates_ == rhs.rates_;
    }
    
    /**
     * Returns true if the two instances of @c hyperexponential_distribution will
     * return different sequences of values given equal generators.
     */
    public: BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(hyperexponential_distribution)


    private: boost::random::discrete_distribution<int,RealT> dd_; ///< The \c discrete_distribution used to sample the phase probability and choose the rate
    private: std::vector<RealT> rates_; ///< The <em>rate vector</em> parameter of the distribution
}; // hyperexponential_distribution

}} // namespace boost::random


#endif // BOOST_RANDOM_HYPEREXPONENTIAL_DISTRIBUTION_HPP
