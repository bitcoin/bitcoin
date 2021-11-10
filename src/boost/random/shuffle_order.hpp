/* boost random/shuffle_order.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 */

#ifndef BOOST_RANDOM_SHUFFLE_ORDER_HPP
#define BOOST_RANDOM_SHUFFLE_ORDER_HPP

#include <iostream>
#include <algorithm>     // std::copy
#include <cassert>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/signed_unsigned_tools.hpp>
#include <boost/random/linear_congruential.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {
namespace random {

/**
 * Instatiations of class template @c shuffle_order_engine model a
 * \pseudo_random_number_generator. It mixes the output
 * of some (usually \linear_congruential_engine)
 * \uniform_random_number_generator to get better statistical properties.
 * The algorithm is described in
 *
 *  @blockquote
 *  "Improving a poor random number generator", Carter Bays
 *  and S.D. Durham, ACM Transactions on Mathematical Software,
 *  Vol 2, No. 1, March 1976, pp. 59-64.
 *  http://doi.acm.org/10.1145/355666.355670
 *  @endblockquote
 *
 * The output of the base generator is buffered in an array of
 * length k. Every output X(n) has a second role: It gives an
 * index into the array where X(n+1) will be retrieved. Used
 * array elements are replaced with fresh output from the base
 * generator.
 *
 * Template parameters are the base generator and the array
 * length k, which should be around 100.
 */
template<class UniformRandomNumberGenerator, std::size_t k>
class shuffle_order_engine
{
public:
    typedef UniformRandomNumberGenerator base_type;
    typedef typename base_type::result_type result_type;

    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    BOOST_STATIC_CONSTANT(std::size_t, buffer_size = k);
    BOOST_STATIC_CONSTANT(std::size_t, table_size = k);

    BOOST_STATIC_ASSERT(std::numeric_limits<result_type>::is_integer);
    BOOST_STATIC_ASSERT(k > 0);

    /**
     * Constructs a @c shuffle_order_engine by invoking the
     * default constructor of the base generator.
     *
     * Complexity: Exactly k+1 invocations of the base generator.
     */
    shuffle_order_engine() : _rng() { init(); }
    /**
     * Constructs a @c shuffle_output_engine by invoking the one-argument
     * constructor of the base generator with the parameter seed.
     *
     * Complexity: Exactly k+1 invocations of the base generator.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(shuffle_order_engine,
                                               result_type, s)
    { _rng.seed(s); init(); }
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(shuffle_order_engine, SeedSeq, seq)
    { _rng.seed(seq); init(); }
    /**
     * Constructs a @c shuffle_output_engine by using a copy
     * of the provided generator.
     *
     * Precondition: The template argument UniformRandomNumberGenerator
     * shall denote a CopyConstructible type.
     *
     * Complexity: Exactly k+1 invocations of the base generator.
     */
    explicit shuffle_order_engine(const base_type & rng) : _rng(rng) { init(); }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    explicit shuffle_order_engine(base_type&& rng) : _rng(rng) { init(); }
#endif

    template<class It> shuffle_order_engine(It& first, It last)
      : _rng(first, last) { init(); }
    void seed() { _rng.seed(); init(); }
    /**
     * Invokes the one-argument seed method of the base generator
     * with the parameter seed and re-initializes the internal buffer array.
     *
     * Complexity: Exactly k+1 invocations of the base generator.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(shuffle_order_engine,
        result_type, seed_arg)
    { _rng.seed(seed_arg); init(); }
    /**
     * Invokes the one-argument seed method of the base generator
     * with the parameter seq and re-initializes the internal buffer array.
     *
     * Complexity: Exactly k+1 invocations of the base generator.
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(shuffle_order_engine, SeedSeq, seq)
    { _rng.seed(seq); init(); }
    template<class It> void seed(It& first, It last)
    { _rng.seed(first, last); init(); }

    const base_type& base() const { return _rng; }

    result_type operator()() {
        // calculating the range every time may seem wasteful.  However, this
        // makes the information locally available for the optimizer.
        typedef typename boost::random::traits::make_unsigned<result_type>::type base_unsigned;
        const base_unsigned brange =
            detail::subtract<result_type>()((max)(), (min)());
        const base_unsigned off =
            detail::subtract<result_type>()(y, (min)());

        base_unsigned j;
        if(k == 1) {
            j = 0;
        } else if(brange < (std::numeric_limits<base_unsigned>::max)() / k) {
            // try to do it in the native type if we know that it won't
            // overflow
            j = k * off / (brange + 1);
        } else if(brange < (std::numeric_limits<uintmax_t>::max)() / k) {
            // Otherwise try to use uint64_t
            j = static_cast<base_unsigned>(
                static_cast<uintmax_t>(off) * k /
                (static_cast<uintmax_t>(brange) + 1));
        } else {
            boost::uintmax_t divisor =
                static_cast<boost::uintmax_t>(brange) + 1;
            j = static_cast<base_unsigned>(detail::muldiv(off, k, divisor));
        }
        // assert(0 <= j && j < k);
        y = v[j];
        v[j] = _rng();
        return y;
    }

    /** Advances the generator by z steps. */
    void discard(boost::uintmax_t z)
    {
        for(boost::uintmax_t j = 0; j < z; ++j) {
            (*this)();
        }
    }

    /** Fills a range with pseudo-random values. */
    template<class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    /** Returns the smallest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (base_type::min)(); }
    /** Returns the largest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (base_type::max)(); }

    /** Writes a @c shuffle_order_engine to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, shuffle_order_engine, s)
    {
        os << s._rng;
        for(std::size_t i = 0; i < k; ++i)
            os << ' ' << s.v[i];
        os << ' ' << s.y;
        return os;
    }

    /** Reads a @c shuffle_order_engine from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, shuffle_order_engine, s)
    {
        is >> s._rng;
        for(std::size_t i = 0; i < k; ++i)
            is >> std::ws >> s.v[i];
        is >> std::ws >> s.y;
        return is;
    }

    /** Returns true if the two generators will produce identical sequences. */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(shuffle_order_engine, x, y)
    { return x._rng == y._rng && x.y == y.y && std::equal(x.v, x.v+k, y.v); }
    /** Returns true if the two generators will produce different sequences. */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(shuffle_order_engine)

private:

    /// \cond show_private

    void init()
    {
        // we cannot use std::generate, because it uses pass-by-value for _rng
        for(result_type * p = v; p != v+k; ++p)
            *p = _rng();
        y = _rng();
    }

    /// \endcond

    base_type _rng;
    result_type v[k];
    result_type y;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class URNG, std::size_t k>
const bool shuffle_order_engine<URNG, k>::has_fixed_range;
template<class URNG, std::size_t k>
const std::size_t shuffle_order_engine<URNG, k>::table_size;
template<class URNG, std::size_t k>
const std::size_t shuffle_order_engine<URNG, k>::buffer_size;
#endif

/**
 * According to Harry Erwin (private e-mail), the specialization
 * @c kreutzer1986 was suggested in:
 *
 * @blockquote
 * "System Simulation: Programming Styles and Languages (International
 * Computer Science Series)", Wolfgang Kreutzer, Addison-Wesley, December 1986.
 * @endblockquote
 */
typedef shuffle_order_engine<
    linear_congruential_engine<uint32_t, 1366, 150889, 714025>,
    97> kreutzer1986;

/**
 * The specialization @c knuth_b is specified by the C++ standard.
 * It is described in
 *
 * @blockquote
 * "The Art of Computer Programming, Second Edition, Volume 2,
 * Seminumerical Algorithms", Donald Knuth, Addison-Wesley, 1981.
 * @endblockquote
 */
typedef shuffle_order_engine<minstd_rand0, 256> knuth_b;

} // namespace random

using random::kreutzer1986;

} // namespace boost

#include <boost/random/detail/enable_warnings.hpp>

#endif // BOOST_RANDOM_SHUFFLE_OUTPUT_HPP
