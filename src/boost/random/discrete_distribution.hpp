/* boost random/discrete_distribution.hpp header file
 *
 * Copyright Steven Watanabe 2009-2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_DISCRETE_DISTRIBUTION_HPP_INCLUDED
#define BOOST_RANDOM_DISCRETE_DISTRIBUTION_HPP_INCLUDED

#include <vector>
#include <limits>
#include <numeric>
#include <utility>
#include <iterator>
#include <boost/assert.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/vector_io.hpp>

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#include <initializer_list>
#endif

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {
namespace random {
namespace detail {

template<class IntType, class WeightType>
struct integer_alias_table {
    WeightType get_weight(IntType bin) const {
        WeightType result = _average;
        if(bin < _excess) ++result;
        return result;
    }
    template<class Iter>
    WeightType init_average(Iter begin, Iter end) {
        WeightType weight_average = 0;
        IntType excess = 0;
        IntType n = 0;
        // weight_average * n + excess == current partial sum
        // This is a bit messy, but it's guaranteed not to overflow
        for(Iter iter = begin; iter != end; ++iter) {
            ++n;
            if(*iter < weight_average) {
                WeightType diff = weight_average - *iter;
                weight_average -= diff / n;
                if(diff % n > excess) {
                    --weight_average;
                    excess += n - diff % n;
                } else {
                    excess -= diff % n;
                }
            } else {
                WeightType diff = *iter - weight_average;
                weight_average += diff / n;
                if(diff % n < n - excess) {
                    excess += diff % n;
                } else {
                    ++weight_average;
                    excess -= n - diff % n;
                }
            }
        }
        _alias_table.resize(static_cast<std::size_t>(n));
        _average = weight_average;
        _excess = excess;
        return weight_average;
    }
    void init_empty()
    {
        _alias_table.clear();
        _alias_table.push_back(std::make_pair(static_cast<WeightType>(1),
                                              static_cast<IntType>(0)));
        _average = static_cast<WeightType>(1);
        _excess = static_cast<IntType>(0);
    }
    bool operator==(const integer_alias_table& other) const
    {
        return _alias_table == other._alias_table &&
            _average == other._average && _excess == other._excess;
    }
    static WeightType normalize(WeightType val, WeightType /* average */)
    {
        return val;
    }
    static void normalize(std::vector<WeightType>&) {}
    template<class URNG>
    WeightType test(URNG &urng) const
    {
        return uniform_int_distribution<WeightType>(0, _average)(urng);
    }
    bool accept(IntType result, WeightType val) const
    {
        return result < _excess || val < _average;
    }
    static WeightType try_get_sum(const std::vector<WeightType>& weights)
    {
        WeightType result = static_cast<WeightType>(0);
        for(typename std::vector<WeightType>::const_iterator
                iter = weights.begin(), end = weights.end();
            iter != end; ++iter)
        {
            if((std::numeric_limits<WeightType>::max)() - result > *iter) {
                return static_cast<WeightType>(0);
            }
            result += *iter;
        }
        return result;
    }
    template<class URNG>
    static WeightType generate_in_range(URNG &urng, WeightType max)
    {
        return uniform_int_distribution<WeightType>(
            static_cast<WeightType>(0), max-1)(urng);
    }
    typedef std::vector<std::pair<WeightType, IntType> > alias_table_t;
    alias_table_t _alias_table;
    WeightType _average;
    IntType _excess;
};

template<class IntType, class WeightType>
struct real_alias_table {
    WeightType get_weight(IntType) const
    {
        return WeightType(1.0);
    }
    template<class Iter>
    WeightType init_average(Iter first, Iter last)
    {
        std::size_t size = std::distance(first, last);
        WeightType weight_sum =
            std::accumulate(first, last, static_cast<WeightType>(0));
        _alias_table.resize(size);
        return weight_sum / size;
    }
    void init_empty()
    {
        _alias_table.clear();
        _alias_table.push_back(std::make_pair(static_cast<WeightType>(1),
                                              static_cast<IntType>(0)));
    }
    bool operator==(const real_alias_table& other) const
    {
        return _alias_table == other._alias_table;
    }
    static WeightType normalize(WeightType val, WeightType average)
    {
        return val / average;
    }
    static void normalize(std::vector<WeightType>& weights)
    {
        WeightType sum =
            std::accumulate(weights.begin(), weights.end(),
                            static_cast<WeightType>(0));
        for(typename std::vector<WeightType>::iterator
                iter = weights.begin(),
                end = weights.end();
            iter != end; ++iter)
        {
            *iter /= sum;
        }
    }
    template<class URNG>
    WeightType test(URNG &urng) const
    {
        return uniform_01<WeightType>()(urng);
    }
    bool accept(IntType, WeightType) const
    {
        return true;
    }
    static WeightType try_get_sum(const std::vector<WeightType>& /* weights */)
    {
        return static_cast<WeightType>(1);
    }
    template<class URNG>
    static WeightType generate_in_range(URNG &urng, WeightType)
    {
        return uniform_01<WeightType>()(urng);
    }
    typedef std::vector<std::pair<WeightType, IntType> > alias_table_t;
    alias_table_t _alias_table;
};

template<bool IsIntegral>
struct select_alias_table;

template<>
struct select_alias_table<true> {
    template<class IntType, class WeightType>
    struct apply {
        typedef integer_alias_table<IntType, WeightType> type;
    };
};

template<>
struct select_alias_table<false> {
    template<class IntType, class WeightType>
    struct apply {
        typedef real_alias_table<IntType, WeightType> type;
    };
};

}

/**
 * The class @c discrete_distribution models a \random_distribution.
 * It produces integers in the range [0, n) with the probability
 * of producing each value is specified by the parameters of the
 * distribution.
 */
template<class IntType = int, class WeightType = double>
class discrete_distribution {
public:
    typedef WeightType input_type;
    typedef IntType result_type;

    class param_type {
    public:

        typedef discrete_distribution distribution_type;

        /**
         * Constructs a @c param_type object, representing a distribution
         * with \f$p(0) = 1\f$ and \f$p(k|k>0) = 0\f$.
         */
        param_type() : _probabilities(1, static_cast<WeightType>(1)) {}
        /**
         * If @c first == @c last, equivalent to the default constructor.
         * Otherwise, the values of the range represent weights for the
         * possible values of the distribution.
         */
        template<class Iter>
        param_type(Iter first, Iter last) : _probabilities(first, last)
        {
            normalize();
        }
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
        /**
         * If wl.size() == 0, equivalent to the default constructor.
         * Otherwise, the values of the @c initializer_list represent
         * weights for the possible values of the distribution.
         */
        param_type(const std::initializer_list<WeightType>& wl)
          : _probabilities(wl)
        {
            normalize();
        }
#endif
        /**
         * If the range is empty, equivalent to the default constructor.
         * Otherwise, the elements of the range represent
         * weights for the possible values of the distribution.
         */
        template<class Range>
        explicit param_type(const Range& range)
          : _probabilities(boost::begin(range), boost::end(range))
        {
            normalize();
        }

        /**
         * If nw is zero, equivalent to the default constructor.
         * Otherwise, the range of the distribution is [0, nw),
         * and the weights are found by  calling fw with values
         * evenly distributed between \f$\mbox{xmin} + \delta/2\f$ and
         * \f$\mbox{xmax} - \delta/2\f$, where
         * \f$\delta = (\mbox{xmax} - \mbox{xmin})/\mbox{nw}\f$.
         */
        template<class Func>
        param_type(std::size_t nw, double xmin, double xmax, Func fw)
        {
            std::size_t n = (nw == 0) ? 1 : nw;
            double delta = (xmax - xmin) / n;
            BOOST_ASSERT(delta > 0);
            for(std::size_t k = 0; k < n; ++k) {
                _probabilities.push_back(fw(xmin + k*delta + delta/2));
            }
            normalize();
        }

        /**
         * Returns a vector containing the probabilities of each possible
         * value of the distribution.
         */
        std::vector<WeightType> probabilities() const
        {
            return _probabilities;
        }

        /** Writes the parameters to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            detail::print_vector(os, parm._probabilities);
            return os;
        }
        
        /** Reads the parameters from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            std::vector<WeightType> temp;
            detail::read_vector(is, temp);
            if(is) {
                parm._probabilities.swap(temp);
            }
            return is;
        }

        /** Returns true if the two sets of parameters are the same. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        {
            return lhs._probabilities == rhs._probabilities;
        }
        /** Returns true if the two sets of parameters are different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)
    private:
        /// @cond show_private
        friend class discrete_distribution;
        explicit param_type(const discrete_distribution& dist)
          : _probabilities(dist.probabilities())
        {}
        void normalize()
        {
            impl_type::normalize(_probabilities);
        }
        std::vector<WeightType> _probabilities;
        /// @endcond
    };

    /**
     * Creates a new @c discrete_distribution object that has
     * \f$p(0) = 1\f$ and \f$p(i|i>0) = 0\f$.
     */
    discrete_distribution()
    {
        _impl.init_empty();
    }
    /**
     * Constructs a discrete_distribution from an iterator range.
     * If @c first == @c last, equivalent to the default constructor.
     * Otherwise, the values of the range represent weights for the
     * possible values of the distribution.
     */
    template<class Iter>
    discrete_distribution(Iter first, Iter last)
    {
        init(first, last);
    }
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    /**
     * Constructs a @c discrete_distribution from a @c std::initializer_list.
     * If the @c initializer_list is empty, equivalent to the default
     * constructor.  Otherwise, the values of the @c initializer_list
     * represent weights for the possible values of the distribution.
     * For example, given the distribution
     *
     * @code
     * discrete_distribution<> dist{1, 4, 5};
     * @endcode
     *
     * The probability of a 0 is 1/10, the probability of a 1 is 2/5,
     * the probability of a 2 is 1/2, and no other values are possible.
     */
    discrete_distribution(std::initializer_list<WeightType> wl)
    {
        init(wl.begin(), wl.end());
    }
#endif
    /**
     * Constructs a discrete_distribution from a Boost.Range range.
     * If the range is empty, equivalent to the default constructor.
     * Otherwise, the values of the range represent weights for the
     * possible values of the distribution.
     */
    template<class Range>
    explicit discrete_distribution(const Range& range)
    {
        init(boost::begin(range), boost::end(range));
    }
    /**
     * Constructs a discrete_distribution that approximates a function.
     * If nw is zero, equivalent to the default constructor.
     * Otherwise, the range of the distribution is [0, nw),
     * and the weights are found by  calling fw with values
     * evenly distributed between \f$\mbox{xmin} + \delta/2\f$ and
     * \f$\mbox{xmax} - \delta/2\f$, where
     * \f$\delta = (\mbox{xmax} - \mbox{xmin})/\mbox{nw}\f$.
     */
    template<class Func>
    discrete_distribution(std::size_t nw, double xmin, double xmax, Func fw)
    {
        std::size_t n = (nw == 0) ? 1 : nw;
        double delta = (xmax - xmin) / n;
        BOOST_ASSERT(delta > 0);
        std::vector<WeightType> weights;
        for(std::size_t k = 0; k < n; ++k) {
            weights.push_back(fw(xmin + k*delta + delta/2));
        }
        init(weights.begin(), weights.end());
    }
    /**
     * Constructs a discrete_distribution from its parameters.
     */
    explicit discrete_distribution(const param_type& parm)
    {
        param(parm);
    }

    /**
     * Returns a value distributed according to the parameters of the
     * discrete_distribution.
     */
    template<class URNG>
    IntType operator()(URNG& urng) const
    {
        BOOST_ASSERT(!_impl._alias_table.empty());
        IntType result;
        WeightType test;
        do {
            result = uniform_int_distribution<IntType>((min)(), (max)())(urng);
            test = _impl.test(urng);
        } while(!_impl.accept(result, test));
        if(test < _impl._alias_table[static_cast<std::size_t>(result)].first) {
            return result;
        } else {
            return(_impl._alias_table[static_cast<std::size_t>(result)].second);
        }
    }
    
    /**
     * Returns a value distributed according to the parameters
     * specified by param.
     */
    template<class URNG>
    IntType operator()(URNG& urng, const param_type& parm) const
    {
        if(WeightType limit = impl_type::try_get_sum(parm._probabilities)) {
            WeightType val = impl_type::generate_in_range(urng, limit);
            WeightType sum = 0;
            std::size_t result = 0;
            for(typename std::vector<WeightType>::const_iterator
                    iter = parm._probabilities.begin(),
                    end = parm._probabilities.end();
                iter != end; ++iter, ++result)
            {
                sum += *iter;
                if(sum > val) {
                    return result;
                }
            }
            // This shouldn't be reachable, but round-off error
            // can prevent any match from being found when val is
            // very close to 1.
            return static_cast<IntType>(parm._probabilities.size() - 1);
        } else {
            // WeightType is integral and sum(parm._probabilities)
            // would overflow.  Just use the easy solution.
            return discrete_distribution(parm)(urng);
        }
    }
    
    /** Returns the smallest value that the distribution can produce. */
    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return 0; }
    /** Returns the largest value that the distribution can produce. */
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    { return static_cast<result_type>(_impl._alias_table.size() - 1); }

    /**
     * Returns a vector containing the probabilities of each
     * value of the distribution.  For example, given
     *
     * @code
     * discrete_distribution<> dist = { 1, 4, 5 };
     * std::vector<double> p = dist.param();
     * @endcode
     *
     * the vector, p will contain {0.1, 0.4, 0.5}.
     *
     * If @c WeightType is integral, then the weights
     * will be returned unchanged.
     */
    std::vector<WeightType> probabilities() const
    {
        std::vector<WeightType> result(_impl._alias_table.size(), static_cast<WeightType>(0));
        std::size_t i = 0;
        for(typename impl_type::alias_table_t::const_iterator
                iter = _impl._alias_table.begin(),
                end = _impl._alias_table.end();
                iter != end; ++iter, ++i)
        {
            WeightType val = iter->first;
            result[i] += val;
            result[static_cast<std::size_t>(iter->second)] += _impl.get_weight(i) - val;
        }
        impl_type::normalize(result);
        return(result);
    }

    /** Returns the parameters of the distribution. */
    param_type param() const
    {
        return param_type(*this);
    }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        init(parm._probabilities.begin(), parm._probabilities.end());
    }
    
    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() {}

    /** Writes a distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, discrete_distribution, dd)
    {
        os << dd.param();
        return os;
    }

    /** Reads a distribution from a @c std::istream */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, discrete_distribution, dd)
    {
        param_type parm;
        if(is >> parm) {
            dd.param(parm);
        }
        return is;
    }

    /**
     * Returns true if the two distributions will return the
     * same sequence of values, when passed equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(discrete_distribution, lhs, rhs)
    {
        return lhs._impl == rhs._impl;
    }
    /**
     * Returns true if the two distributions may return different
     * sequences of values, when passed equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(discrete_distribution)

private:

    /// @cond show_private

    template<class Iter>
    void init(Iter first, Iter last, std::input_iterator_tag)
    {
        std::vector<WeightType> temp(first, last);
        init(temp.begin(), temp.end());
    }
    template<class Iter>
    void init(Iter first, Iter last, std::forward_iterator_tag)
    {
        size_t input_size = std::distance(first, last);
        std::vector<std::pair<WeightType, IntType> > below_average;
        std::vector<std::pair<WeightType, IntType> > above_average;
        below_average.reserve(input_size);
        above_average.reserve(input_size);
        
        WeightType weight_average = _impl.init_average(first, last);
        WeightType normalized_average = _impl.get_weight(0);
        std::size_t i = 0;
        for(; first != last; ++first, ++i) {
            WeightType val = impl_type::normalize(*first, weight_average);
            std::pair<WeightType, IntType> elem(val, static_cast<IntType>(i));
            if(val < normalized_average) {
                below_average.push_back(elem);
            } else {
                above_average.push_back(elem);
            }
        }

        typename impl_type::alias_table_t::iterator
            b_iter = below_average.begin(),
            b_end = below_average.end(),
            a_iter = above_average.begin(),
            a_end = above_average.end()
            ;
        while(b_iter != b_end && a_iter != a_end) {
            _impl._alias_table[static_cast<std::size_t>(b_iter->second)] =
                std::make_pair(b_iter->first, a_iter->second);
            a_iter->first -= (_impl.get_weight(b_iter->second) - b_iter->first);
            if(a_iter->first < normalized_average) {
                *b_iter = *a_iter++;
            } else {
                ++b_iter;
            }
        }
        for(; b_iter != b_end; ++b_iter) {
            _impl._alias_table[static_cast<std::size_t>(b_iter->second)].first =
                _impl.get_weight(b_iter->second);
        }
        for(; a_iter != a_end; ++a_iter) {
            _impl._alias_table[static_cast<std::size_t>(a_iter->second)].first =
                _impl.get_weight(a_iter->second);
        }
    }
    template<class Iter>
    void init(Iter first, Iter last)
    {
        if(first == last) {
            _impl.init_empty();
        } else {
            typename std::iterator_traits<Iter>::iterator_category category;
            init(first, last, category);
        }
    }
    typedef typename detail::select_alias_table<
        (::boost::is_integral<WeightType>::value)
    >::template apply<IntType, WeightType>::type impl_type;
    impl_type _impl;
    /// @endcond
};

}
}

#include <boost/random/detail/enable_warnings.hpp>

#endif
