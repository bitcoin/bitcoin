/* boost random/linear_congruential.hpp header file
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

#ifndef BOOST_RANDOM_LINEAR_CONGRUENTIAL_HPP
#define BOOST_RANDOM_LINEAR_CONGRUENTIAL_HPP

#include <iostream>
#include <stdexcept>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/const_mod.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/seed_impl.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {
namespace random {

/**
 * Instantiations of class template linear_congruential_engine model a
 * \pseudo_random_number_generator. Linear congruential pseudo-random
 * number generators are described in:
 *
 *  @blockquote
 *  "Mathematical methods in large-scale computing units", D. H. Lehmer,
 *  Proc. 2nd Symposium on Large-Scale Digital Calculating Machines,
 *  Harvard University Press, 1951, pp. 141-146
 *  @endblockquote
 *
 * Let x(n) denote the sequence of numbers returned by some pseudo-random
 * number generator. Then for the linear congruential generator,
 * x(n+1) := (a * x(n) + c) mod m. Parameters for the generator are
 * x(0), a, c, m. The template parameter IntType shall denote an integral
 * type. It must be large enough to hold values a, c, and m. The template
 * parameters a and c must be smaller than m.
 *
 * Note: The quality of the generator crucially depends on the choice of
 * the parameters. User code should use one of the sensibly parameterized
 * generators such as minstd_rand instead.
 */
template<class IntType, IntType a, IntType c, IntType m>
class linear_congruential_engine
{
public:
    typedef IntType result_type;

    // Required for old Boost.Random concept
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);

    BOOST_STATIC_CONSTANT(IntType, multiplier = a);
    BOOST_STATIC_CONSTANT(IntType, increment = c);
    BOOST_STATIC_CONSTANT(IntType, modulus = m);
    BOOST_STATIC_CONSTANT(IntType, default_seed = 1);
    
    BOOST_STATIC_ASSERT(std::numeric_limits<IntType>::is_integer);
    BOOST_STATIC_ASSERT(m == 0 || a < m);
    BOOST_STATIC_ASSERT(m == 0 || c < m);
    
    /**
     * Constructs a @c linear_congruential_engine, using the default seed
     */
    linear_congruential_engine() { seed(); }

    /**
     * Constructs a @c linear_congruential_engine, seeding it with @c x0.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(linear_congruential_engine,
                                               IntType, x0)
    { seed(x0); }
    
    /**
     * Constructs a @c linear_congruential_engine, seeding it with values
     * produced by a call to @c seq.generate().
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(linear_congruential_engine,
                                             SeedSeq, seq)
    { seed(seq); }

    /**
     * Constructs a @c linear_congruential_engine  and seeds it
     * with values taken from the itrator range [first, last)
     * and adjusts first to point to the element after the last one
     * used.  If there are not enough elements, throws @c std::invalid_argument.
     *
     * first and last must be input iterators.
     */
    template<class It>
    linear_congruential_engine(It& first, It last)
    {
        seed(first, last);
    }

    // compiler-generated copy constructor and assignment operator are fine

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
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(linear_congruential_engine, IntType, x0_)
    {
        // Work around a msvc 12/14 optimizer bug, which causes
        // the line _x = 1 to run unconditionally sometimes.
        // Creating a local copy seems to make it work.
        IntType x0 = x0_;
        // wrap _x if it doesn't fit in the destination
        if(modulus == 0) {
            _x = x0;
        } else {
            _x = x0 % modulus;
        }
        // handle negative seeds
        if(_x < 0) {
            _x += modulus;
        }
        // adjust to the correct range
        if(increment == 0 && _x == 0) {
            _x = 1;
        }
        BOOST_ASSERT(_x >= (min)());
        BOOST_ASSERT(_x <= (max)());
    }

    /**
     * Seeds a @c linear_congruential_engine using values from a SeedSeq.
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(linear_congruential_engine, SeedSeq, seq)
    { seed(detail::seed_one_int<IntType, m>(seq)); }

    /**
     * seeds a @c linear_congruential_engine with values taken
     * from the itrator range [first, last) and adjusts @c first to
     * point to the element after the last one used.  If there are
     * not enough elements, throws @c std::invalid_argument.
     *
     * @c first and @c last must be input iterators.
     */
    template<class It>
    void seed(It& first, It last)
    { seed(detail::get_one_int<IntType, m>(first, last)); }

    /**
     * Returns the smallest value that the @c linear_congruential_engine
     * can produce.
     */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return c == 0 ? 1 : 0; }
    /**
     * Returns the largest value that the @c linear_congruential_engine
     * can produce.
     */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return modulus-1; }

    /** Returns the next value of the @c linear_congruential_engine. */
    IntType operator()()
    {
        _x = const_mod<IntType, m>::mult_add(a, _x, c);
        return _x;
    }
  
    /** Fills a range with random values */
    template<class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    /** Advances the state of the generator by @c z. */
    void discard(boost::uintmax_t z)
    {
        typedef const_mod<IntType, m> mod_type;
        IntType b_inv = mod_type::invert(a-1);
        IntType b_gcd = mod_type::mult(a-1, b_inv);
        if(b_gcd == 1) {
            IntType a_z = mod_type::pow(a, z);
            _x = mod_type::mult_add(a_z, _x, 
                mod_type::mult(mod_type::mult(c, b_inv), a_z - 1));
        } else {
            // compute (a^z - 1)*c % (b_gcd * m) / (b / b_gcd) * inv(b / b_gcd)
            // we're storing the intermediate result / b_gcd
            IntType a_zm1_over_gcd = 0;
            IntType a_km1_over_gcd = (a - 1) / b_gcd;
            boost::uintmax_t exponent = z;
            while(exponent != 0) {
                if(exponent % 2 == 1) {
                    a_zm1_over_gcd =
                        mod_type::mult_add(
                            b_gcd,
                            mod_type::mult(a_zm1_over_gcd, a_km1_over_gcd),
                            mod_type::add(a_zm1_over_gcd, a_km1_over_gcd));
                }
                a_km1_over_gcd = mod_type::mult_add(
                    b_gcd,
                    mod_type::mult(a_km1_over_gcd, a_km1_over_gcd),
                    mod_type::add(a_km1_over_gcd, a_km1_over_gcd));
                exponent /= 2;
            }
            
            IntType a_z = mod_type::mult_add(b_gcd, a_zm1_over_gcd, 1);
            IntType num = mod_type::mult(c, a_zm1_over_gcd);
            b_inv = mod_type::invert((a-1)/b_gcd);
            _x = mod_type::mult_add(a_z, _x, mod_type::mult(b_inv, num));
        }
    }

    friend bool operator==(const linear_congruential_engine& x,
                           const linear_congruential_engine& y)
    { return x._x == y._x; }
    friend bool operator!=(const linear_congruential_engine& x,
                           const linear_congruential_engine& y)
    { return !(x == y); }
    
#if !defined(BOOST_RANDOM_NO_STREAM_OPERATORS)
    /** Writes a @c linear_congruential_engine to a @c std::ostream. */
    template<class CharT, class Traits>
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& os,
               const linear_congruential_engine& lcg)
    {
        return os << lcg._x;
    }

    /** Reads a @c linear_congruential_engine from a @c std::istream. */
    template<class CharT, class Traits>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& is,
               linear_congruential_engine& lcg)
    {
        lcg.read(is);
        return is;
    }
#endif

private:

    /// \cond show_private

    template<class CharT, class Traits>
    void read(std::basic_istream<CharT, Traits>& is) {
        IntType x;
        if(is >> x) {
            if(x >= (min)() && x <= (max)()) {
                _x = x;
            } else {
                is.setstate(std::ios_base::failbit);
            }
        }
    }

    /// \endcond

    IntType _x;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class IntType, IntType a, IntType c, IntType m>
const bool linear_congruential_engine<IntType, a, c, m>::has_fixed_range;
template<class IntType, IntType a, IntType c, IntType m>
const IntType linear_congruential_engine<IntType,a,c,m>::multiplier;
template<class IntType, IntType a, IntType c, IntType m>
const IntType linear_congruential_engine<IntType,a,c,m>::increment;
template<class IntType, IntType a, IntType c, IntType m>
const IntType linear_congruential_engine<IntType,a,c,m>::modulus;
template<class IntType, IntType a, IntType c, IntType m>
const IntType linear_congruential_engine<IntType,a,c,m>::default_seed;
#endif

/// \cond show_deprecated

// provided for backwards compatibility
template<class IntType, IntType a, IntType c, IntType m, IntType val = 0>
class linear_congruential : public linear_congruential_engine<IntType, a, c, m>
{
    typedef linear_congruential_engine<IntType, a, c, m> base_type;
public:
    linear_congruential(IntType x0 = 1) : base_type(x0) {}
    template<class It>
    linear_congruential(It& first, It last) : base_type(first, last) {}
};

/// \endcond

/**
 * The specialization \minstd_rand0 was originally suggested in
 *
 *  @blockquote
 *  A pseudo-random number generator for the System/360, P.A. Lewis,
 *  A.S. Goodman, J.M. Miller, IBM Systems Journal, Vol. 8, No. 2,
 *  1969, pp. 136-146
 *  @endblockquote
 *
 * It is examined more closely together with \minstd_rand in
 *
 *  @blockquote
 *  "Random Number Generators: Good ones are hard to find",
 *  Stephen K. Park and Keith W. Miller, Communications of
 *  the ACM, Vol. 31, No. 10, October 1988, pp. 1192-1201 
 *  @endblockquote
 */
typedef linear_congruential_engine<uint32_t, 16807, 0, 2147483647> minstd_rand0;

/** The specialization \minstd_rand was suggested in
 *
 *  @blockquote
 *  "Random Number Generators: Good ones are hard to find",
 *  Stephen K. Park and Keith W. Miller, Communications of
 *  the ACM, Vol. 31, No. 10, October 1988, pp. 1192-1201
 *  @endblockquote
 */
typedef linear_congruential_engine<uint32_t, 48271, 0, 2147483647> minstd_rand;


#if !defined(BOOST_NO_INT64_T) && !defined(BOOST_NO_INTEGRAL_INT64_T)
/**
 * Class @c rand48 models a \pseudo_random_number_generator. It uses
 * the linear congruential algorithm with the parameters a = 0x5DEECE66D,
 * c = 0xB, m = 2**48. It delivers identical results to the @c lrand48()
 * function available on some systems (assuming lcong48 has not been called).
 *
 * It is only available on systems where @c uint64_t is provided as an
 * integral type, so that for example static in-class constants and/or
 * enum definitions with large @c uint64_t numbers work.
 */
class rand48 
{
public:
    typedef boost::uint32_t result_type;

    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    /**
     * Returns the smallest value that the generator can produce
     */
    static BOOST_CONSTEXPR uint32_t min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    /**
     * Returns the largest value that the generator can produce
     */
    static BOOST_CONSTEXPR uint32_t max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return 0x7FFFFFFF; }
  
    /** Seeds the generator with the default seed. */
    rand48() : lcf(cnv(static_cast<uint32_t>(1))) {}
    /**
     * Constructs a \rand48 generator with x(0) := (x0 << 16) | 0x330e.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(rand48, result_type, x0)
    { seed(x0); }
    /**
     * Seeds the generator with values produced by @c seq.generate().
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(rand48, SeedSeq, seq)
    { seed(seq); }
    /**
     * Seeds the generator using values from an iterator range,
     * and updates first to point one past the last value consumed.
     */
    template<class It> rand48(It& first, It last) : lcf(first, last) { }

    // compiler-generated copy ctor and assignment operator are fine

    /** Seeds the generator with the default seed. */
    void seed() { seed(static_cast<uint32_t>(1)); }
    /**
     * Changes the current value x(n) of the generator to (x0 << 16) | 0x330e.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(rand48, result_type, x0)
    { lcf.seed(cnv(x0)); }
    /**
     * Seeds the generator using values from an iterator range,
     * and updates first to point one past the last value consumed.
     */
    template<class It> void seed(It& first, It last) { lcf.seed(first,last); }
    /**
     * Seeds the generator with values produced by @c seq.generate().
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(rand48, SeedSeq, seq)
    { lcf.seed(seq); }

    /**  Returns the next value of the generator. */
    uint32_t operator()() { return static_cast<uint32_t>(lcf() >> 17); }
    
    /** Advances the state of the generator by @c z. */
    void discard(boost::uintmax_t z) { lcf.discard(z); }
  
    /** Fills a range with random values */
    template<class Iter>
    void generate(Iter first, Iter last)
    {
        for(; first != last; ++first) {
            *first = (*this)();
        }
    }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
    /**  Writes a @c rand48 to a @c std::ostream. */
    template<class CharT,class Traits>
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& os, const rand48& r)
    { os << r.lcf; return os; }

    /** Reads a @c rand48 from a @c std::istream. */
    template<class CharT,class Traits>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& is, rand48& r)
    { is >> r.lcf; return is; }
#endif

    /**
     * Returns true if the two generators will produce identical
     * sequences of values.
     */
    friend bool operator==(const rand48& x, const rand48& y)
    { return x.lcf == y.lcf; }
    /**
     * Returns true if the two generators will produce different
     * sequences of values.
     */
    friend bool operator!=(const rand48& x, const rand48& y)
    { return !(x == y); }
private:
    /// \cond show_private
    typedef random::linear_congruential_engine<uint64_t,
        // xxxxULL is not portable
        uint64_t(0xDEECE66DUL) | (uint64_t(0x5) << 32),
        0xB, uint64_t(1)<<48> lcf_t;
    lcf_t lcf;

    static boost::uint64_t cnv(boost::uint32_t x)
    { return (static_cast<uint64_t>(x) << 16) | 0x330e; }
    /// \endcond
};
#endif /* !BOOST_NO_INT64_T && !BOOST_NO_INTEGRAL_INT64_T */

} // namespace random

using random::minstd_rand0;
using random::minstd_rand;
using random::rand48;

} // namespace boost

#include <boost/random/detail/enable_warnings.hpp>

#endif // BOOST_RANDOM_LINEAR_CONGRUENTIAL_HPP
