/* boost random/inversive_congruential.hpp header file
 *
 * Copyright Jens Maurer 2000-2001
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 * Revision history
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_INVERSIVE_CONGRUENTIAL_HPP
#define BOOST_RANDOM_INVERSIVE_CONGRUENTIAL_HPP

#include <iosfwd>
#include <stdexcept>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/const_mod.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/seed_impl.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {
namespace random {

// Eichenauer and Lehn 1986
/**
 * Instantiations of class template @c inversive_congruential_engine model a
 * \pseudo_random_number_generator. It uses the inversive congruential
 * algorithm (ICG) described in
 *
 *  @blockquote
 *  "Inversive pseudorandom number generators: concepts, results and links",
 *  Peter Hellekalek, In: "Proceedings of the 1995 Winter Simulation
 *  Conference", C. Alexopoulos, K. Kang, W.R. Lilegdon, and D. Goldsman
 *  (editors), 1995, pp. 255-262. ftp://random.mat.sbg.ac.at/pub/data/wsc95.ps
 *  @endblockquote
 *
 * The output sequence is defined by x(n+1) = (a*inv(x(n)) - b) (mod p),
 * where x(0), a, b, and the prime number p are parameters of the generator.
 * The expression inv(k) denotes the multiplicative inverse of k in the
 * field of integer numbers modulo p, with inv(0) := 0.
 *
 * The template parameter IntType shall denote a signed integral type large
 * enough to hold p; a, b, and p are the parameters of the generators. The
 * template parameter val is the validation value checked by validation.
 *
 * @xmlnote
 * The implementation currently uses the Euclidian Algorithm to compute
 * the multiplicative inverse. Therefore, the inversive generators are about
 * 10-20 times slower than the others (see section"performance"). However,
 * the paper talks of only 3x slowdown, so the Euclidian Algorithm is probably
 * not optimal for calculating the multiplicative inverse.
 * @endxmlnote
 */
template<class IntType, IntType a, IntType b, IntType p>
class inversive_congruential_engine
{
public:
    typedef IntType result_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

    BOOST_STATIC_CONSTANT(result_type, multiplier = a);
    BOOST_STATIC_CONSTANT(result_type, increment = b);
    BOOST_STATIC_CONSTANT(result_type, modulus = p);
    BOOST_STATIC_CONSTANT(IntType, default_seed = 1);

    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return b == 0 ? 1 : 0; }
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () { return p-1; }
    
    /**
     * Constructs an @c inversive_congruential_engine, seeding it with
     * the default seed.
     */
    inversive_congruential_engine() { seed(); }

    /**
     * Constructs an @c inversive_congruential_engine, seeding it with @c x0.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(inversive_congruential_engine,
                                               IntType, x0)
    { seed(x0); }
    
    /**
     * Constructs an @c inversive_congruential_engine, seeding it with values
     * produced by a call to @c seq.generate().
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(inversive_congruential_engine,
                                             SeedSeq, seq)
    { seed(seq); }
    
    /**
     * Constructs an @c inversive_congruential_engine, seeds it
     * with values taken from the itrator range [first, last),
     * and adjusts first to point to the element after the last one
     * used.  If there are not enough elements, throws @c std::invalid_argument.
     *
     * first and last must be input iterators.
     */
    template<class It> inversive_congruential_engine(It& first, It last)
    { seed(first, last); }

    /**
     * Calls seed(default_seed)
     */
    void seed() { seed(default_seed); }
  
    /**
     * If c mod m is zero and x0 mod m is zero, changes the current value of
     * the generator to 1. Otherwise, changes it to x0 mod m. If c is zero,
     * distinct seeds in the range [1,m) will leave the generator in distinct
     * states. If c is not zero, the range is [0,m).
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(inversive_congruential_engine, IntType, x0)
    {
        // wrap _x if it doesn't fit in the destination
        if(modulus == 0) {
            _value = x0;
        } else {
            _value = x0 % modulus;
        }
        // handle negative seeds
        if(_value <= 0 && _value != 0) {
            _value += modulus;
        }
        // adjust to the correct range
        if(increment == 0 && _value == 0) {
            _value = 1;
        }
        BOOST_ASSERT(_value >= (min)());
        BOOST_ASSERT(_value <= (max)());
    }

    /**
     * Seeds an @c inversive_congruential_engine using values from a SeedSeq.
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(inversive_congruential_engine, SeedSeq, seq)
    { seed(detail::seed_one_int<IntType, modulus>(seq)); }
    
    /**
     * seeds an @c inversive_congruential_engine with values taken
     * from the itrator range [first, last) and adjusts @c first to
     * point to the element after the last one used.  If there are
     * not enough elements, throws @c std::invalid_argument.
     *
     * @c first and @c last must be input iterators.
     */
    template<class It> void seed(It& first, It last)
    { seed(detail::get_one_int<IntType, modulus>(first, last)); }

    /** Returns the next output of the generator. */
    IntType operator()()
    {
        typedef const_mod<IntType, p> do_mod;
        _value = do_mod::mult_add(a, do_mod::invert(_value), b);
        return _value;
    }
  
    /** Fills a range with random values */
    template<class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    /** Advances the state of the generator by @c z. */
    void discard(boost::uintmax_t z)
    {
        for(boost::uintmax_t j = 0; j < z; ++j) {
            (*this)();
        }
    }

    /**
     * Writes the textual representation of the generator to a @c std::ostream.
     */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, inversive_congruential_engine, x)
    {
        os << x._value;
        return os;
    }

    /**
     * Reads the textual representation of the generator from a @c std::istream.
     */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, inversive_congruential_engine, x)
    {
        is >> x._value;
        return is;
    }

    /**
     * Returns true if the two generators will produce identical
     * sequences of outputs.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(inversive_congruential_engine, x, y)
    { return x._value == y._value; }

    /**
     * Returns true if the two generators will produce different
     * sequences of outputs.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(inversive_congruential_engine)

private:
    IntType _value;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class IntType, IntType a, IntType b, IntType p>
const bool inversive_congruential_engine<IntType, a, b, p>::has_fixed_range;
template<class IntType, IntType a, IntType b, IntType p>
const typename inversive_congruential_engine<IntType, a, b, p>::result_type inversive_congruential_engine<IntType, a, b, p>::multiplier;
template<class IntType, IntType a, IntType b, IntType p>
const typename inversive_congruential_engine<IntType, a, b, p>::result_type inversive_congruential_engine<IntType, a, b, p>::increment;
template<class IntType, IntType a, IntType b, IntType p>
const typename inversive_congruential_engine<IntType, a, b, p>::result_type inversive_congruential_engine<IntType, a, b, p>::modulus;
template<class IntType, IntType a, IntType b, IntType p>
const typename inversive_congruential_engine<IntType, a, b, p>::result_type inversive_congruential_engine<IntType, a, b, p>::default_seed;
#endif

/// \cond show_deprecated

// provided for backwards compatibility
template<class IntType, IntType a, IntType b, IntType p, IntType val = 0>
class inversive_congruential : public inversive_congruential_engine<IntType, a, b, p>
{
    typedef inversive_congruential_engine<IntType, a, b, p> base_type;
public:
    inversive_congruential(IntType x0 = 1) : base_type(x0) {}
    template<class It>
    inversive_congruential(It& first, It last) : base_type(first, last) {}
};

/// \endcond

/**
 * The specialization hellekalek1995 was suggested in
 *
 *  @blockquote
 *  "Inversive pseudorandom number generators: concepts, results and links",
 *  Peter Hellekalek, In: "Proceedings of the 1995 Winter Simulation
 *  Conference", C. Alexopoulos, K. Kang, W.R. Lilegdon, and D. Goldsman
 *  (editors), 1995, pp. 255-262. ftp://random.mat.sbg.ac.at/pub/data/wsc95.ps
 *  @endblockquote
 */
typedef inversive_congruential_engine<uint32_t, 9102, 2147483647-36884165,
  2147483647> hellekalek1995;

} // namespace random

using random::hellekalek1995;

} // namespace boost

#include <boost/random/detail/enable_warnings.hpp>

#endif // BOOST_RANDOM_INVERSIVE_CONGRUENTIAL_HPP
