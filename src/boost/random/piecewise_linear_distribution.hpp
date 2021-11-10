/* boost random/piecewise_linear_distribution.hpp header file
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_PIECEWISE_LINEAR_DISTRIBUTION_HPP_INCLUDED
#define BOOST_RANDOM_PIECEWISE_LINEAR_DISTRIBUTION_HPP_INCLUDED

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <boost/assert.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/vector_io.hpp>

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#include <initializer_list>
#endif

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost {
namespace random {

/**
 * The class @c piecewise_linear_distribution models a \random_distribution.
 */
template<class RealType = double>
class piecewise_linear_distribution {
public:
    typedef std::size_t input_type;
    typedef RealType result_type;

    class param_type {
    public:

        typedef piecewise_linear_distribution distribution_type;

        /**
         * Constructs a @c param_type object, representing a distribution
         * that produces values uniformly distributed in the range [0, 1).
         */
        param_type()
        {
            _weights.push_back(RealType(1));
            _weights.push_back(RealType(1));
            _intervals.push_back(RealType(0));
            _intervals.push_back(RealType(1));
        }
        /**
         * Constructs a @c param_type object from two iterator ranges
         * containing the interval boundaries and weights at the boundaries.
         * If there are fewer than two boundaries, then this is equivalent to
         * the default constructor and the distribution will produce values
         * uniformly distributed in the range [0, 1).
         *
         * The values of the interval boundaries must be strictly
         * increasing, and the number of weights must be the same as
         * the number of interval boundaries.  If there are extra
         * weights, they are ignored.
         */
        template<class IntervalIter, class WeightIter>
        param_type(IntervalIter intervals_first, IntervalIter intervals_last,
                   WeightIter weight_first)
          : _intervals(intervals_first, intervals_last)
        {
            if(_intervals.size() < 2) {
                _intervals.clear();
                _weights.push_back(RealType(1));
                _weights.push_back(RealType(1));
                _intervals.push_back(RealType(0));
                _intervals.push_back(RealType(1));
            } else {
                _weights.reserve(_intervals.size());
                for(std::size_t i = 0; i < _intervals.size(); ++i) {
                    _weights.push_back(*weight_first++);
                }
            }
        }
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
        /**
         * Constructs a @c param_type object from an initializer_list
         * containing the interval boundaries and a unary function
         * specifying the weights at the boundaries.  Each weight is
         * determined by calling the function at the corresponding point.
         *
         * If the initializer_list contains fewer than two elements,
         * this is equivalent to the default constructor and the
         * distribution will produce values uniformly distributed
         * in the range [0, 1).
         */
        template<class T, class F>
        param_type(const std::initializer_list<T>& il, F f)
          : _intervals(il.begin(), il.end())
        {
            if(_intervals.size() < 2) {
                _intervals.clear();
                _weights.push_back(RealType(1));
                _weights.push_back(RealType(1));
                _intervals.push_back(RealType(0));
                _intervals.push_back(RealType(1));
            } else {
                _weights.reserve(_intervals.size());
                for(typename std::vector<RealType>::const_iterator
                    iter = _intervals.begin(), end = _intervals.end();
                    iter != end; ++iter)
                {
                    _weights.push_back(f(*iter));
                }
            }
        }
#endif
        /**
         * Constructs a @c param_type object from Boost.Range ranges holding
         * the interval boundaries and the weights at the boundaries.  If
         * there are fewer than two interval boundaries, this is equivalent
         * to the default constructor and the distribution will produce
         * values uniformly distributed in the range [0, 1).  The
         * number of weights must be equal to the number of
         * interval boundaries.
         */
        template<class IntervalRange, class WeightRange>
        param_type(const IntervalRange& intervals_arg,
                   const WeightRange& weights_arg)
          : _intervals(boost::begin(intervals_arg), boost::end(intervals_arg)),
            _weights(boost::begin(weights_arg), boost::end(weights_arg))
        {
            if(_intervals.size() < 2) {
                _weights.clear();
                _weights.push_back(RealType(1));
                _weights.push_back(RealType(1));
                _intervals.clear();
                _intervals.push_back(RealType(0));
                _intervals.push_back(RealType(1));
            }
        }

        /**
         * Constructs the parameters for a distribution that approximates a
         * function.  The range of the distribution is [xmin, xmax).  This
         * range is divided into nw equally sized intervals and the weights
         * are found by calling the unary function f on the boundaries of the
         * intervals.
         */
        template<class F>
        param_type(std::size_t nw, RealType xmin, RealType xmax, F f)
        {
            std::size_t n = (nw == 0) ? 1 : nw;
            double delta = (xmax - xmin) / n;
            BOOST_ASSERT(delta > 0);
            for(std::size_t k = 0; k < n; ++k) {
                _weights.push_back(f(xmin + k*delta));
                _intervals.push_back(xmin + k*delta);
            }
            _weights.push_back(f(xmax));
            _intervals.push_back(xmax);
        }

        /**  Returns a vector containing the interval boundaries. */
        std::vector<RealType> intervals() const { return _intervals; }

        /**
         * Returns a vector containing the probability densities
         * at all the interval boundaries.
         */
        std::vector<RealType> densities() const
        {
            RealType sum = static_cast<RealType>(0);
            for(std::size_t i = 0; i < _intervals.size() - 1; ++i) {
                RealType width = _intervals[i + 1] - _intervals[i];
                sum += (_weights[i] + _weights[i + 1]) * width / 2;
            }
            std::vector<RealType> result;
            result.reserve(_weights.size());
            for(typename std::vector<RealType>::const_iterator
                iter = _weights.begin(), end = _weights.end();
                iter != end; ++iter)
            {
                result.push_back(*iter / sum);
            }
            return result;
        }

        /** Writes the parameters to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            detail::print_vector(os, parm._intervals);
            detail::print_vector(os, parm._weights);
            return os;
        }
        
        /** Reads the parameters from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            std::vector<RealType> new_intervals;
            std::vector<RealType> new_weights;
            detail::read_vector(is, new_intervals);
            detail::read_vector(is, new_weights);
            if(is) {
                parm._intervals.swap(new_intervals);
                parm._weights.swap(new_weights);
            }
            return is;
        }

        /** Returns true if the two sets of parameters are the same. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        {
            return lhs._intervals == rhs._intervals
                && lhs._weights == rhs._weights;
        }
        /** Returns true if the two sets of parameters are different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        friend class piecewise_linear_distribution;

        std::vector<RealType> _intervals;
        std::vector<RealType> _weights;
    };

    /**
     * Creates a new @c piecewise_linear_distribution that
     * produces values uniformly distributed in the range [0, 1).
     */
    piecewise_linear_distribution()
    {
        default_init();
    }
    /**
     * Constructs a piecewise_linear_distribution from two iterator ranges
     * containing the interval boundaries and the weights at the boundaries.
     * If there are fewer than two boundaries, then this is equivalent to
     * the default constructor and creates a distribution that
     * produces values uniformly distributed in the range [0, 1).
     *
     * The values of the interval boundaries must be strictly
     * increasing, and the number of weights must be equal to
     * the number of interval boundaries.  If there are extra
     * weights, they are ignored.
     *
     * For example,
     *
     * @code
     * double intervals[] = { 0.0, 1.0, 2.0 };
     * double weights[] = { 0.0, 1.0, 0.0 };
     * piecewise_constant_distribution<> dist(
     *     &intervals[0], &intervals[0] + 3, &weights[0]);
     * @endcode
     *
     * produces a triangle distribution.
     */
    template<class IntervalIter, class WeightIter>
    piecewise_linear_distribution(IntervalIter first_interval,
                                  IntervalIter last_interval,
                                  WeightIter first_weight)
      : _intervals(first_interval, last_interval)
    {
        if(_intervals.size() < 2) {
            default_init();
        } else {
            _weights.reserve(_intervals.size());
            for(std::size_t i = 0; i < _intervals.size(); ++i) {
                _weights.push_back(*first_weight++);
            }
            init();
        }
    }
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    /**
     * Constructs a piecewise_linear_distribution from an
     * initializer_list containing the interval boundaries
     * and a unary function specifying the weights.  Each
     * weight is determined by calling the function at the
     * corresponding interval boundary.
     *
     * If the initializer_list contains fewer than two elements,
     * this is equivalent to the default constructor and the
     * distribution will produce values uniformly distributed
     * in the range [0, 1).
     */
    template<class T, class F>
    piecewise_linear_distribution(std::initializer_list<T> il, F f)
      : _intervals(il.begin(), il.end())
    {
        if(_intervals.size() < 2) {
            default_init();
        } else {
            _weights.reserve(_intervals.size());
            for(typename std::vector<RealType>::const_iterator
                iter = _intervals.begin(), end = _intervals.end();
                iter != end; ++iter)
            {
                _weights.push_back(f(*iter));
            }
            init();
        }
    }
#endif
    /**
     * Constructs a piecewise_linear_distribution from Boost.Range
     * ranges holding the interval boundaries and the weights.  If
     * there are fewer than two interval boundaries, this is equivalent
     * to the default constructor and the distribution will produce
     * values uniformly distributed in the range [0, 1).  The
     * number of weights must be equal to the number of
     * interval boundaries.
     */
    template<class IntervalsRange, class WeightsRange>
    piecewise_linear_distribution(const IntervalsRange& intervals_arg,
                                  const WeightsRange& weights_arg)
      : _intervals(boost::begin(intervals_arg), boost::end(intervals_arg)),
        _weights(boost::begin(weights_arg), boost::end(weights_arg))
    {
        if(_intervals.size() < 2) {
            default_init();
        } else {
            init();
        }
    }
    /**
     * Constructs a piecewise_linear_distribution that approximates a
     * function.  The range of the distribution is [xmin, xmax).  This
     * range is divided into nw equally sized intervals and the weights
     * are found by calling the unary function f on the interval boundaries.
     */
    template<class F>
    piecewise_linear_distribution(std::size_t nw,
                                  RealType xmin,
                                  RealType xmax,
                                  F f)
    {
        if(nw == 0) { nw = 1; }
        RealType delta = (xmax - xmin) / nw;
        _intervals.reserve(nw + 1);
        for(std::size_t i = 0; i < nw; ++i) {
            RealType x = xmin + i * delta;
            _intervals.push_back(x);
            _weights.push_back(f(x));
        }
        _intervals.push_back(xmax);
        _weights.push_back(f(xmax));
        init();
    }
    /**
     * Constructs a piecewise_linear_distribution from its parameters.
     */
    explicit piecewise_linear_distribution(const param_type& parm)
      : _intervals(parm._intervals),
        _weights(parm._weights)
    {
        init();
    }

    /**
     * Returns a value distributed according to the parameters of the
     * piecewise_linear_distribution.
     */
    template<class URNG>
    RealType operator()(URNG& urng) const
    {
        std::size_t i = _bins(urng);
        bool is_in_rectangle = (i % 2 == 0);
        i = i / 2;
        uniform_real<RealType> dist(_intervals[i], _intervals[i+1]);
        if(is_in_rectangle) {
            return dist(urng);
        } else if(_weights[i] < _weights[i+1]) {
            return (std::max)(dist(urng), dist(urng));
        } else {
            return (std::min)(dist(urng), dist(urng));
        }
    }
    
    /**
     * Returns a value distributed according to the parameters
     * specified by param.
     */
    template<class URNG>
    RealType operator()(URNG& urng, const param_type& parm) const
    {
        return piecewise_linear_distribution(parm)(urng);
    }
    
    /** Returns the smallest value that the distribution can produce. */
    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return _intervals.front(); }
    /** Returns the largest value that the distribution can produce. */
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return _intervals.back(); }

    /**
     * Returns a vector containing the probability densities
     * at the interval boundaries.
     */
    std::vector<RealType> densities() const
    {
        RealType sum = static_cast<RealType>(0);
        for(std::size_t i = 0; i < _intervals.size() - 1; ++i) {
            RealType width = _intervals[i + 1] - _intervals[i];
            sum += (_weights[i] + _weights[i + 1]) * width / 2;
        }
        std::vector<RealType> result;
        result.reserve(_weights.size());
        for(typename std::vector<RealType>::const_iterator
            iter = _weights.begin(), end = _weights.end();
            iter != end; ++iter)
        {
            result.push_back(*iter / sum);
        }
        return result;
    }
    /**  Returns a vector containing the interval boundaries. */
    std::vector<RealType> intervals() const { return _intervals; }

    /** Returns the parameters of the distribution. */
    param_type param() const
    {
        return param_type(_intervals, _weights);
    }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        std::vector<RealType> new_intervals(parm._intervals);
        std::vector<RealType> new_weights(parm._weights);
        init(new_intervals, new_weights);
        _intervals.swap(new_intervals);
        _weights.swap(new_weights);
    }
    
    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { _bins.reset(); }

    /** Writes a distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(
        os, piecewise_linear_distribution, pld)
    {
        os << pld.param();
        return os;
    }

    /** Reads a distribution from a @c std::istream */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(
        is, piecewise_linear_distribution, pld)
    {
        param_type parm;
        if(is >> parm) {
            pld.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two distributions will return the
     * same sequence of values, when passed equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(
        piecewise_linear_distribution, lhs,  rhs)
    {
        return lhs._intervals == rhs._intervals && lhs._weights == rhs._weights;
    }
    /**
     * Returns true if the two distributions may return different
     * sequences of values, when passed equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(piecewise_linear_distribution)

private:

    /// @cond \show_private

    void init(const std::vector<RealType>& intervals_arg,
              const std::vector<RealType>& weights_arg)
    {
        using std::abs;
        std::vector<RealType> bin_weights;
        bin_weights.reserve((intervals_arg.size() - 1) * 2);
        for(std::size_t i = 0; i < intervals_arg.size() - 1; ++i) {
            RealType width = intervals_arg[i + 1] - intervals_arg[i];
            RealType w1 = weights_arg[i];
            RealType w2 = weights_arg[i + 1];
            bin_weights.push_back((std::min)(w1, w2) * width);
            bin_weights.push_back(abs(w1 - w2) * width / 2);
        }
        typedef discrete_distribution<std::size_t, RealType> bins_type;
        typename bins_type::param_type bins_param(bin_weights);
        _bins.param(bins_param);
    }

    void init()
    {
        init(_intervals, _weights);
    }

    void default_init()
    {
        _intervals.clear();
        _intervals.push_back(RealType(0));
        _intervals.push_back(RealType(1));
        _weights.clear();
        _weights.push_back(RealType(1));
        _weights.push_back(RealType(1));
        init();
    }

    discrete_distribution<std::size_t, RealType> _bins;
    std::vector<RealType> _intervals;
    std::vector<RealType> _weights;

    /// @endcond
};

}
}

#endif
