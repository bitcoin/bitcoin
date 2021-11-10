/* boost random/subtract_with_carry.hpp header file
 *
 * Copyright Jens Maurer 2002
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 * Revision history
 *  2002-03-02  created
 */

#ifndef BOOST_RANDOM_SUBTRACT_WITH_CARRY_HPP
#define BOOST_RANDOM_SUBTRACT_WITH_CARRY_HPP

#include <boost/config/no_tr1/cmath.hpp>         // std::pow
#include <iostream>
#include <algorithm>     // std::equal
#include <stdexcept>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <boost/integer/static_log2.hpp>
#include <boost/integer/integer_mask.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/seed_impl.hpp>
#include <boost/random/detail/generator_seed_seq.hpp>
#include <boost/random/linear_congruential.hpp>


namespace boost {
namespace random {

namespace detail {
   
struct subtract_with_carry_discard
{
    template<class Engine>
    static void apply(Engine& eng, boost::uintmax_t z)
    {
        typedef typename Engine::result_type IntType;
        const std::size_t short_lag = Engine::short_lag;
        const std::size_t long_lag = Engine::long_lag;
        std::size_t k = eng.k;
        IntType carry = eng.carry;
        if(k != 0) {
            // increment k until it becomes 0.
            if(k < short_lag) {
                std::size_t limit = (short_lag - k) < z?
                    short_lag : (k + static_cast<std::size_t>(z));
                for(std::size_t j = k; j < limit; ++j) {
                    carry = eng.do_update(j, j + long_lag - short_lag, carry);
                }
            }
            std::size_t limit = (long_lag - k) < z?
                long_lag : (k + static_cast<std::size_t>(z));
            std::size_t start = (k < short_lag ? short_lag : k);
            for(std::size_t j = start; j < limit; ++j) {
                carry = eng.do_update(j, j - short_lag, carry);
            }
        }

        k = ((z % long_lag) + k) % long_lag;

        if(k < z) {
            // main loop: update full blocks from k = 0 to long_lag
            for(std::size_t i = 0; i < (z - k) / long_lag; ++i) {
                for(std::size_t j = 0; j < short_lag; ++j) {
                    carry = eng.do_update(j, j + long_lag - short_lag, carry);
                }
                for(std::size_t j = short_lag; j < long_lag; ++j) {
                    carry = eng.do_update(j, j - short_lag, carry);
                }
            }

            // Update the last partial block
            std::size_t limit = short_lag < k? short_lag : k; 
            for(std::size_t j = 0; j < limit; ++j) {
                carry = eng.do_update(j, j + long_lag - short_lag, carry);
            }
            for(std::size_t j = short_lag; j < k; ++j) {
                carry = eng.do_update(j, j - short_lag, carry);
            }
        }
        eng.carry = carry;
        eng.k = k;
    }
};

}

/**
 * Instantiations of @c subtract_with_carry_engine model a
 * \pseudo_random_number_generator.  The algorithm is
 * described in
 *
 *  @blockquote
 *  "A New Class of Random Number Generators", George
 *  Marsaglia and Arif Zaman, Annals of Applied Probability,
 *  Volume 1, Number 3 (1991), 462-480.
 *  @endblockquote
 */
template<class IntType, std::size_t w, std::size_t s, std::size_t r>
class subtract_with_carry_engine
{
public:
    typedef IntType result_type;
    BOOST_STATIC_CONSTANT(std::size_t, word_size = w);
    BOOST_STATIC_CONSTANT(std::size_t, long_lag = r);
    BOOST_STATIC_CONSTANT(std::size_t, short_lag = s);
    BOOST_STATIC_CONSTANT(uint32_t, default_seed = 19780503u);

    // Required by the old Boost.Random concepts
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    // Backwards compatibility
    BOOST_STATIC_CONSTANT(result_type, modulus = (result_type(1) << w));
    
    BOOST_STATIC_ASSERT(std::numeric_limits<result_type>::is_integer);

    /**
     * Constructs a new @c subtract_with_carry_engine and seeds
     * it with the default seed.
     */
    subtract_with_carry_engine() { seed(); }
    /**
     * Constructs a new @c subtract_with_carry_engine and seeds
     * it with @c value.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(subtract_with_carry_engine,
                                               IntType, value)
    { seed(value); }
    /**
     * Constructs a new @c subtract_with_carry_engine and seeds
     * it with values produced by @c seq.generate().
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(subtract_with_carry_engine,
                                             SeedSeq, seq)
    { seed(seq); }
    /**
     * Constructs a new @c subtract_with_carry_engine and seeds
     * it with values from a range.  first is updated to point
     * one past the last value consumed.  If there are not
     * enough elements in the range to fill the entire state of
     * the generator, throws @c std::invalid_argument.
     */
    template<class It> subtract_with_carry_engine(It& first, It last)
    { seed(first,last); }

    // compiler-generated copy ctor and assignment operator are fine

    /** Seeds the generator with the default seed. */
    void seed() { seed(default_seed); }
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(subtract_with_carry_engine,
                                        IntType, value)
    {
        typedef linear_congruential_engine<uint32_t,40014,0,2147483563> gen_t;
        gen_t intgen(static_cast<boost::uint32_t>(value == 0 ? default_seed : value));
        detail::generator_seed_seq<gen_t> gen(intgen);
        seed(gen);
    }

    /** Seeds the generator with values produced by @c seq.generate(). */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(subtract_with_carry, SeedSeq, seq)
    {
        detail::seed_array_int<w>(seq, x);
        carry = (x[long_lag-1] == 0);
        k = 0;
    }

    /**
     * Seeds the generator with values from a range.  Updates @c first to
     * point one past the last consumed value.  If the range does not
     * contain enough elements to fill the entire state of the generator,
     * throws @c std::invalid_argument.
     */
    template<class It>
    void seed(It& first, It last)
    {
        detail::fill_array_int<w>(first, last, x);
        carry = (x[long_lag-1] == 0);
        k = 0;
    }

    /** Returns the smallest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return 0; }
    /** Returns the largest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return boost::low_bits_mask_t<w>::sig_bits; }

    /** Returns the next value of the generator. */
    result_type operator()()
    {
        std::size_t short_index =
            (k < short_lag)?
                (k + long_lag - short_lag) :
                (k - short_lag);
        carry = do_update(k, short_index, carry);
        IntType result = x[k];
        ++k;
        if(k >= long_lag)
            k = 0;
        return result;
    }

    /** Advances the state of the generator by @c z. */
    void discard(boost::uintmax_t z)
    {
        detail::subtract_with_carry_discard::apply(*this, z);
    }

    /** Fills a range with random values. */
    template<class It>
    void generate(It first, It last)
    { detail::generate_from_int(*this, first, last); }
 
    /** Writes a @c subtract_with_carry_engine to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, subtract_with_carry_engine, f)
    {
        for(unsigned int j = 0; j < f.long_lag; ++j)
            os << f.compute(j) << ' ';
        os << f.carry;
        return os;
    }

    /** Reads a @c subtract_with_carry_engine from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, subtract_with_carry_engine, f)
    {
        for(unsigned int j = 0; j < f.long_lag; ++j)
            is >> f.x[j] >> std::ws;
        is >> f.carry;
        f.k = 0;
        return is;
    }

    /**
     * Returns true if the two generators will produce identical
     * sequences of values.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(subtract_with_carry_engine, x, y)
    {
        for(unsigned int j = 0; j < r; ++j)
            if(x.compute(j) != y.compute(j))
                return false;
        return true;
    }

    /**
     * Returns true if the two generators will produce different
     * sequences of values.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(subtract_with_carry_engine)

private:
    /// \cond show_private
    // returns x(i-r+index), where index is in 0..r-1
    IntType compute(unsigned int index) const
    {
        return x[(k+index) % long_lag];
    }

    friend struct detail::subtract_with_carry_discard;

    IntType do_update(std::size_t current, std::size_t short_index, IntType carry)
    {
        IntType delta;
        IntType temp = x[current] + carry;
        if (x[short_index] >= temp) {
            // x(n) >= 0
            delta =  x[short_index] - temp;
            carry = 0;
        } else {
            // x(n) < 0
            delta = modulus - temp + x[short_index];
            carry = 1;
        }
        x[current] = delta;
        return carry;
    }
    /// \endcond

    // state representation; next output (state) is x(i)
    //   x[0]  ... x[k] x[k+1] ... x[long_lag-1]     represents
    //  x(i-k) ... x(i) x(i+1) ... x(i-k+long_lag-1)
    // speed: base: 20-25 nsec
    // ranlux_4: 230 nsec, ranlux_7: 430 nsec, ranlux_14: 810 nsec
    // This state representation makes operator== and save/restore more
    // difficult, because we've already computed "too much" and thus
    // have to undo some steps to get at x(i-r) etc.

    // state representation: next output (state) is x(i)
    //   x[0]  ... x[k] x[k+1]          ... x[long_lag-1]     represents
    //  x(i-k) ... x(i) x(i-long_lag+1) ... x(i-k-1)
    // speed: base 28 nsec
    // ranlux_4: 370 nsec, ranlux_7: 688 nsec, ranlux_14: 1343 nsec
    IntType x[long_lag];
    std::size_t k;
    IntType carry;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class IntType, std::size_t w, std::size_t s, std::size_t r>
const bool subtract_with_carry_engine<IntType, w, s, r>::has_fixed_range;
template<class IntType, std::size_t w, std::size_t s, std::size_t r>
const IntType subtract_with_carry_engine<IntType, w, s, r>::modulus;
template<class IntType, std::size_t w, std::size_t s, std::size_t r>
const std::size_t subtract_with_carry_engine<IntType, w, s, r>::word_size;
template<class IntType, std::size_t w, std::size_t s, std::size_t r>
const std::size_t subtract_with_carry_engine<IntType, w, s, r>::long_lag;
template<class IntType, std::size_t w, std::size_t s, std::size_t r>
const std::size_t subtract_with_carry_engine<IntType, w, s, r>::short_lag;
template<class IntType, std::size_t w, std::size_t s, std::size_t r>
const uint32_t subtract_with_carry_engine<IntType, w, s, r>::default_seed;
#endif


// use a floating-point representation to produce values in [0..1)
/**
 * Instantiations of \subtract_with_carry_01_engine model a
 * \pseudo_random_number_generator.  The algorithm is
 * described in
 *
 *  @blockquote
 *  "A New Class of Random Number Generators", George
 *  Marsaglia and Arif Zaman, Annals of Applied Probability,
 *  Volume 1, Number 3 (1991), 462-480.
 *  @endblockquote
 */
template<class RealType, std::size_t w, std::size_t s, std::size_t r>
class subtract_with_carry_01_engine
{
public:
    typedef RealType result_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    BOOST_STATIC_CONSTANT(std::size_t, word_size = w);
    BOOST_STATIC_CONSTANT(std::size_t, long_lag = r);
    BOOST_STATIC_CONSTANT(std::size_t, short_lag = s);
    BOOST_STATIC_CONSTANT(boost::uint32_t, default_seed = 19780503u);

    BOOST_STATIC_ASSERT(!std::numeric_limits<result_type>::is_integer);

    /** Creates a new \subtract_with_carry_01_engine using the default seed. */
    subtract_with_carry_01_engine() { init_modulus(); seed(); }
    /** Creates a new subtract_with_carry_01_engine and seeds it with value. */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(subtract_with_carry_01_engine,
                                               boost::uint32_t, value)
    { init_modulus(); seed(value); }
    /**
     * Creates a new \subtract_with_carry_01_engine and seeds with values
     * produced by seq.generate().
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(subtract_with_carry_01_engine,
                                             SeedSeq, seq)
    { init_modulus(); seed(seq); }
    /**
     * Creates a new \subtract_with_carry_01_engine and seeds it with values
     * from a range.  Advances first to point one past the last consumed
     * value.  If the range does not contain enough elements to fill the
     * entire state, throws @c std::invalid_argument.
     */
    template<class It> subtract_with_carry_01_engine(It& first, It last)
    { init_modulus(); seed(first,last); }

private:
    /// \cond show_private
    void init_modulus()
    {
#ifndef BOOST_NO_STDC_NAMESPACE
        // allow for Koenig lookup
        using std::pow;
#endif
        _modulus = pow(RealType(2), RealType(word_size));
    }
    /// \endcond

public:
    // compiler-generated copy ctor and assignment operator are fine

    /** Seeds the generator with the default seed. */
    void seed() { seed(default_seed); }

    /** Seeds the generator with @c value. */
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(subtract_with_carry_01_engine,
                                        boost::uint32_t, value)
    {
        typedef linear_congruential_engine<uint32_t, 40014, 0, 2147483563> gen_t;
        gen_t intgen(value == 0 ? default_seed : value);
        detail::generator_seed_seq<gen_t> gen(intgen);
        seed(gen);
    }

    /** Seeds the generator with values produced by @c seq.generate(). */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(subtract_with_carry_01_engine,
                                      SeedSeq, seq)
    {
        detail::seed_array_real<w>(seq, x);
        carry = (x[long_lag-1] ? result_type(0) : result_type(1 / _modulus));
        k = 0;
    }

    /**
     * Seeds the generator with values from a range.  Updates first to
     * point one past the last consumed element.  If there are not
     * enough elements in the range to fill the entire state, throws
     * @c std::invalid_argument.
     */
    template<class It>
    void seed(It& first, It last)
    {
        detail::fill_array_real<w>(first, last, x);
        carry = (x[long_lag-1] ? result_type(0) : result_type(1 / _modulus));
        k = 0;
    }

    /** Returns the smallest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return result_type(0); }
    /** Returns the largest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return result_type(1); }

    /** Returns the next value of the generator. */
    result_type operator()()
    {
        std::size_t short_index =
            (k < short_lag) ?
                (k + long_lag - short_lag) :
                (k - short_lag);
        carry = do_update(k, short_index, carry);
        RealType result = x[k];
        ++k;
        if(k >= long_lag)
            k = 0;
        return result;
    }

    /** Advances the state of the generator by @c z. */
    void discard(boost::uintmax_t z)
    { detail::subtract_with_carry_discard::apply(*this, z); }

    /** Fills a range with random values. */
    template<class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_real(*this, first, last); }

    /** Writes a \subtract_with_carry_01_engine to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, subtract_with_carry_01_engine, f)
    {
        std::ios_base::fmtflags oldflags =
            os.flags(os.dec | os.fixed | os.left); 
        for(unsigned int j = 0; j < f.long_lag; ++j)
            os << (f.compute(j) * f._modulus) << ' ';
        os << (f.carry * f._modulus);
        os.flags(oldflags);
        return os;
    }
    
    /** Reads a \subtract_with_carry_01_engine from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, subtract_with_carry_01_engine, f)
    {
        RealType value;
        for(unsigned int j = 0; j < long_lag; ++j) {
            is >> value >> std::ws;
            f.x[j] = value / f._modulus;
        }
        is >> value;
        f.carry = value / f._modulus;
        f.k = 0;
        return is;
    }

    /** Returns true if the two generators will produce identical sequences. */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(subtract_with_carry_01_engine, x, y)
    {
        for(unsigned int j = 0; j < r; ++j)
            if(x.compute(j) != y.compute(j))
                return false;
        return true;
    }

    /** Returns true if the two generators will produce different sequences. */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(subtract_with_carry_01_engine)

private:
    /// \cond show_private
    RealType compute(unsigned int index) const
    {
        return x[(k+index) % long_lag];
    }

    friend struct detail::subtract_with_carry_discard;

    RealType do_update(std::size_t current, std::size_t short_index, RealType carry)
    {
        RealType delta = x[short_index] - x[current] - carry;
        if(delta < 0) {
            delta += RealType(1);
            carry = RealType(1)/_modulus;
        } else {
            carry = 0;
        }
        x[current] = delta;
        return carry;
    }
    /// \endcond
    std::size_t k;
    RealType carry;
    RealType x[long_lag];
    RealType _modulus;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class RealType, std::size_t w, std::size_t s, std::size_t r>
const bool subtract_with_carry_01_engine<RealType, w, s, r>::has_fixed_range;
template<class RealType, std::size_t w, std::size_t s, std::size_t r>
const std::size_t subtract_with_carry_01_engine<RealType, w, s, r>::word_size;
template<class RealType, std::size_t w, std::size_t s, std::size_t r>
const std::size_t subtract_with_carry_01_engine<RealType, w, s, r>::long_lag;
template<class RealType, std::size_t w, std::size_t s, std::size_t r>
const std::size_t subtract_with_carry_01_engine<RealType, w, s, r>::short_lag;
template<class RealType, std::size_t w, std::size_t s, std::size_t r>
const uint32_t subtract_with_carry_01_engine<RealType, w, s, r>::default_seed;
#endif


/// \cond show_deprecated

template<class IntType, IntType m, unsigned s, unsigned r, IntType v>
class subtract_with_carry :
    public subtract_with_carry_engine<IntType,
        boost::static_log2<m>::value, s, r>
{
    typedef subtract_with_carry_engine<IntType,
        boost::static_log2<m>::value, s, r> base_type;
public:
    subtract_with_carry() {}
    BOOST_RANDOM_DETAIL_GENERATOR_CONSTRUCTOR(subtract_with_carry, Gen, gen)
    { seed(gen); }
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(subtract_with_carry,
                                               IntType, val)
    { seed(val); }
    template<class It>
    subtract_with_carry(It& first, It last) : base_type(first, last) {}
    void seed() { base_type::seed(); }
    BOOST_RANDOM_DETAIL_GENERATOR_SEED(subtract_with_carry, Gen, gen)
    {
        detail::generator_seed_seq<Gen> seq(gen);
        base_type::seed(seq);
    }
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(subtract_with_carry, IntType, val)
    { base_type::seed(val); }
    template<class It>
    void seed(It& first, It last) { base_type::seed(first, last); }
};

template<class RealType, int w, unsigned s, unsigned r, int v = 0>
class subtract_with_carry_01 :
    public subtract_with_carry_01_engine<RealType, w, s, r>
{
    typedef subtract_with_carry_01_engine<RealType, w, s, r> base_type;
public:
    subtract_with_carry_01() {}
    BOOST_RANDOM_DETAIL_GENERATOR_CONSTRUCTOR(subtract_with_carry_01, Gen, gen)
    { seed(gen); }
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(subtract_with_carry_01,
                                               uint32_t, val)
    { seed(val); }
    template<class It>
    subtract_with_carry_01(It& first, It last) : base_type(first, last) {}
    void seed() { base_type::seed(); }
    BOOST_RANDOM_DETAIL_GENERATOR_SEED(subtract_with_carry_01, Gen, gen)
    {
        detail::generator_seed_seq<Gen> seq(gen);
        base_type::seed(seq);
    }
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(subtract_with_carry_01, uint32_t, val)
    { base_type::seed(val); }
    template<class It>
    void seed(It& first, It last) { base_type::seed(first, last); }
};

/// \endcond

namespace detail {

template<class Engine>
struct generator_bits;

template<class RealType, std::size_t w, std::size_t s, std::size_t r>
struct generator_bits<subtract_with_carry_01_engine<RealType, w, s, r> > {
    static std::size_t value() { return w; }
};

template<class RealType, int w, unsigned s, unsigned r, int v>
struct generator_bits<subtract_with_carry_01<RealType, w, s, r, v> > {
    static std::size_t value() { return w; }
};

}

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_SUBTRACT_WITH_CARRY_HPP
