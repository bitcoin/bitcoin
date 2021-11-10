/* boost random/lagged_fibonacci.hpp header file
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
 *  2013-10-14  fixed some warnings with Wshadow (mgaunard)
 *  2001-02-18  moved to individual header files
 */

#ifndef BOOST_RANDOM_LAGGED_FIBONACCI_HPP
#define BOOST_RANDOM_LAGGED_FIBONACCI_HPP

#include <istream>
#include <iosfwd>
#include <algorithm>     // std::max
#include <iterator>
#include <boost/config/no_tr1/cmath.hpp>         // std::pow
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/cstdint.hpp>
#include <boost/integer/integer_mask.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/generator_seed_seq.hpp>

namespace boost {
namespace random {

/**
 * Instantiations of class template \lagged_fibonacci_engine model a
 * \pseudo_random_number_generator. It uses a lagged Fibonacci
 * algorithm with two lags @c p and @c q:
 * x(i) = x(i-p) + x(i-q) (mod 2<sup>w</sup>) with p > q.
 */
template<class UIntType, int w, unsigned int p, unsigned int q>
class lagged_fibonacci_engine
{
public:
    typedef UIntType result_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    BOOST_STATIC_CONSTANT(int, word_size = w);
    BOOST_STATIC_CONSTANT(unsigned int, long_lag = p);
    BOOST_STATIC_CONSTANT(unsigned int, short_lag = q);

    BOOST_STATIC_CONSTANT(UIntType, default_seed = 331u);

    /** Returns the smallest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return 0; }
    /** Returns the largest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return low_bits_mask_t<w>::sig_bits; }

    /** Creates a new @c lagged_fibonacci_engine and calls @c seed(). */
    lagged_fibonacci_engine() { seed(); }

    /** Creates a new @c lagged_fibonacci_engine and calls @c seed(value). */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(lagged_fibonacci_engine,
        UIntType, value)
    { seed(value); }

    /** Creates a new @c lagged_fibonacci_engine and calls @c seed(seq). */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(lagged_fibonacci_engine,
        SeedSeq, seq)
    { seed(seq); }

    /**
     * Creates a new @c lagged_fibonacci_engine and calls @c seed(first, last).
     */
    template<class It> lagged_fibonacci_engine(It& first, It last)
    { seed(first, last); }

    // compiler-generated copy ctor and assignment operator are fine

    /** Calls @c seed(default_seed). */
    void seed() { seed(default_seed); }

    /**
     * Sets the state of the generator to values produced by
     * a \minstd_rand0 generator.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(lagged_fibonacci_engine,
        UIntType, value)
    {
        minstd_rand0 intgen(static_cast<boost::uint32_t>(value));
        detail::generator_seed_seq<minstd_rand0> gen(intgen);
        seed(gen);
    }

    /**
     * Sets the state of the generator using values produced by seq.
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(lagged_fibonacci_engine, SeedSeq, seq)
    {
        detail::seed_array_int<w>(seq, x);
        i = long_lag;
    }

    /**
     * Sets the state of the generator to values from the iterator
     * range [first, last).  If there are not enough elements in the
     * range [first, last) throws @c std::invalid_argument.
     */
    template<class It>
    void seed(It& first, It last)
    {
        detail::fill_array_int<w>(first, last, x);
        i = long_lag;
    }

    /** Returns the next value of the generator. */
    result_type operator()()
    {
        if(i >= long_lag)
            fill();
        return x[i++];
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
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, lagged_fibonacci_engine, f)
    {
        os << f.i;
        for(unsigned int j = 0; j < f.long_lag; ++j)
            os << ' ' << f.x[j];
        return os;
    }

    /**
     * Reads the textual representation of the generator from a @c std::istream.
     */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, lagged_fibonacci_engine, f)
    {
        is >> f.i >> std::ws;
        for(unsigned int j = 0; j < f.long_lag; ++j)
            is >> f.x[j] >> std::ws;
        return is;
    }

    /**
     * Returns true if the two generators will produce identical
     * sequences of outputs.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(lagged_fibonacci_engine, x_, y_)
    { return x_.i == y_.i && std::equal(x_.x, x_.x+long_lag, y_.x); }

    /**
     * Returns true if the two generators will produce different
     * sequences of outputs.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(lagged_fibonacci_engine)

private:
    /// \cond show_private
    void fill();
    /// \endcond

    unsigned int i;
    UIntType x[long_lag];
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class UIntType, int w, unsigned int p, unsigned int q>
const bool lagged_fibonacci_engine<UIntType, w, p, q>::has_fixed_range;
template<class UIntType, int w, unsigned int p, unsigned int q>
const unsigned int lagged_fibonacci_engine<UIntType, w, p, q>::long_lag;
template<class UIntType, int w, unsigned int p, unsigned int q>
const unsigned int lagged_fibonacci_engine<UIntType, w, p, q>::short_lag;
template<class UIntType, int w, unsigned int p, unsigned int q>
const UIntType lagged_fibonacci_engine<UIntType, w, p, q>::default_seed;
#endif

/// \cond show_private

template<class UIntType, int w, unsigned int p, unsigned int q>
void lagged_fibonacci_engine<UIntType, w, p, q>::fill()
{
    // two loops to avoid costly modulo operations
    {  // extra scope for MSVC brokenness w.r.t. for scope
    for(unsigned int j = 0; j < short_lag; ++j)
        x[j] = (x[j] + x[j+(long_lag-short_lag)]) & low_bits_mask_t<w>::sig_bits;
    }
    for(unsigned int j = short_lag; j < long_lag; ++j)
        x[j] = (x[j] + x[j-short_lag]) & low_bits_mask_t<w>::sig_bits;
    i = 0;
}

/// \endcond

/// \cond show_deprecated

// provided for backwards compatibility
template<class UIntType, int w, unsigned int p, unsigned int q, UIntType v = 0>
class lagged_fibonacci : public lagged_fibonacci_engine<UIntType, w, p, q>
{
    typedef lagged_fibonacci_engine<UIntType, w, p, q> base_type;
public:
    lagged_fibonacci() {}
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(lagged_fibonacci, UIntType, val)
    { this->seed(val); }
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(lagged_fibonacci, SeedSeq, seq)
    { this->seed(seq); }
    template<class It>
    lagged_fibonacci(It& first, It last) : base_type(first, last) {}
};

/// \endcond

// lagged Fibonacci generator for the range [0..1)
// contributed by Matthias Troyer
// for p=55, q=24 originally by G. J. Mitchell and D. P. Moore 1958

/**
 * Instantiations of class template @c lagged_fibonacci_01 model a
 * \pseudo_random_number_generator. It uses a lagged Fibonacci
 * algorithm with two lags @c p and @c q, evaluated in floating-point
 * arithmetic: x(i) = x(i-p) + x(i-q) (mod 1) with p > q. See
 *
 *  @blockquote
 *  "Uniform random number generators for supercomputers", Richard Brent,
 *  Proc. of Fifth Australian Supercomputer Conference, Melbourne,
 *  Dec. 1992, pp. 704-706.
 *  @endblockquote
 *
 * @xmlnote
 * The quality of the generator crucially depends on the choice
 * of the parameters. User code should employ one of the sensibly
 * parameterized generators such as \lagged_fibonacci607 instead.
 * @endxmlnote
 *
 * The generator requires considerable amounts of memory for the storage
 * of its state array. For example, \lagged_fibonacci607 requires about
 * 4856 bytes and \lagged_fibonacci44497 requires about 350 KBytes.
 */
template<class RealType, int w, unsigned int p, unsigned int q>
class lagged_fibonacci_01_engine
{
public:
    typedef RealType result_type;
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    BOOST_STATIC_CONSTANT(int, word_size = w);
    BOOST_STATIC_CONSTANT(unsigned int, long_lag = p);
    BOOST_STATIC_CONSTANT(unsigned int, short_lag = q);

    BOOST_STATIC_CONSTANT(boost::uint32_t, default_seed = 331u);

    /** Constructs a @c lagged_fibonacci_01 generator and calls @c seed(). */
    lagged_fibonacci_01_engine() { seed(); }
    /** Constructs a @c lagged_fibonacci_01 generator and calls @c seed(value). */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(lagged_fibonacci_01_engine, uint32_t, value)
    { seed(value); }
    /** Constructs a @c lagged_fibonacci_01 generator and calls @c seed(gen). */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(lagged_fibonacci_01_engine, SeedSeq, seq)
    { seed(seq); }
    template<class It> lagged_fibonacci_01_engine(It& first, It last)
    { seed(first, last); }

    // compiler-generated copy ctor and assignment operator are fine

    /** Calls seed(default_seed). */
    void seed() { seed(default_seed); }

    /**
     * Constructs a \minstd_rand0 generator with the constructor parameter
     * value and calls seed with it. Distinct seeds in the range
     * [1, 2147483647) will produce generators with different states. Other
     * seeds will be equivalent to some seed within this range. See
     * \linear_congruential_engine for details.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(lagged_fibonacci_01_engine, boost::uint32_t, value)
    {
        minstd_rand0 intgen(value);
        detail::generator_seed_seq<minstd_rand0> gen(intgen);
        seed(gen);
    }

    /**
     * Seeds this @c lagged_fibonacci_01_engine using values produced by
     * @c seq.generate.
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(lagged_fibonacci_01_engine, SeedSeq, seq)
    {
        detail::seed_array_real<w>(seq, x);
        i = long_lag;
    }

    /**
     * Seeds this @c lagged_fibonacci_01_engine using values from the
     * iterator range [first, last).  If there are not enough elements
     * in the range, throws @c std::invalid_argument.
     */
    template<class It>
    void seed(It& first, It last)
    {
        detail::fill_array_real<w>(first, last, x);
        i = long_lag;
    }

    /** Returns the smallest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () { return result_type(0); }
    /** Returns the upper bound of the generators outputs. */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () { return result_type(1); }

    /** Returns the next value of the generator. */
    result_type operator()()
    {
        if(i >= long_lag)
            fill();
        return x[i++];
    }

    /** Fills a range with random values */
    template<class Iter>
    void generate(Iter first, Iter last)
    { return detail::generate_from_real(*this, first, last); }

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
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, lagged_fibonacci_01_engine, f)
    {
        // allow for Koenig lookup
        using std::pow;
        os << f.i;
        std::ios_base::fmtflags oldflags = os.flags(os.dec | os.fixed | os.left);
        for(unsigned int j = 0; j < f.long_lag; ++j)
            os << ' ' << f.x[j] * f.modulus();
        os.flags(oldflags);
        return os;
    }

    /**
     * Reads the textual representation of the generator from a @c std::istream.
     */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, lagged_fibonacci_01_engine, f)
    {
        is >> f.i;
        for(unsigned int j = 0; j < f.long_lag; ++j) {
            typename lagged_fibonacci_01_engine::result_type value;
            is >> std::ws >> value;
            f.x[j] = value / f.modulus();
        }
        return is;
    }

    /**
     * Returns true if the two generators will produce identical
     * sequences of outputs.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(lagged_fibonacci_01_engine, x_, y_)
    { return x_.i == y_.i && std::equal(x_.x, x_.x+long_lag, y_.x); }

    /**
     * Returns true if the two generators will produce different
     * sequences of outputs.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(lagged_fibonacci_01_engine)

private:
    /// \cond show_private
    void fill();
    static RealType modulus()
    {
        using std::pow;
        return pow(RealType(2), word_size);
    }
    /// \endcond
    unsigned int i;
    RealType x[long_lag];
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class RealType, int w, unsigned int p, unsigned int q>
const bool lagged_fibonacci_01_engine<RealType, w, p, q>::has_fixed_range;
template<class RealType, int w, unsigned int p, unsigned int q>
const unsigned int lagged_fibonacci_01_engine<RealType, w, p, q>::long_lag;
template<class RealType, int w, unsigned int p, unsigned int q>
const unsigned int lagged_fibonacci_01_engine<RealType, w, p, q>::short_lag;
template<class RealType, int w, unsigned int p, unsigned int q>
const int lagged_fibonacci_01_engine<RealType,w,p,q>::word_size;
template<class RealType, int w, unsigned int p, unsigned int q>
const boost::uint32_t lagged_fibonacci_01_engine<RealType,w,p,q>::default_seed;
#endif

/// \cond show_private
template<class RealType, int w, unsigned int p, unsigned int q>
void lagged_fibonacci_01_engine<RealType, w, p, q>::fill()
{
    // two loops to avoid costly modulo operations
    {  // extra scope for MSVC brokenness w.r.t. for scope
    for(unsigned int j = 0; j < short_lag; ++j) {
        RealType t = x[j] + x[j+(long_lag-short_lag)];
        if(t >= RealType(1))
            t -= RealType(1);
        x[j] = t;
    }
    }
    for(unsigned int j = short_lag; j < long_lag; ++j) {
        RealType t = x[j] + x[j-short_lag];
        if(t >= RealType(1))
            t -= RealType(1);
        x[j] = t;
    }
    i = 0;
}
/// \endcond

/// \cond show_deprecated

// provided for backwards compatibility
template<class RealType, int w, unsigned int p, unsigned int q>
class lagged_fibonacci_01 : public lagged_fibonacci_01_engine<RealType, w, p, q>
{
    typedef lagged_fibonacci_01_engine<RealType, w, p, q> base_type;
public:
    lagged_fibonacci_01() {}
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(lagged_fibonacci_01, boost::uint32_t, val)
    { this->seed(val); }
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(lagged_fibonacci_01, SeedSeq, seq)
    { this->seed(seq); }
    template<class It>
    lagged_fibonacci_01(It& first, It last) : base_type(first, last) {}
};

/// \endcond

namespace detail {

template<class Engine>
struct generator_bits;

template<class RealType, int w, unsigned int p, unsigned int q>
struct generator_bits<lagged_fibonacci_01_engine<RealType, w, p, q> >
{
    static std::size_t value() { return w; }
};

template<class RealType, int w, unsigned int p, unsigned int q>
struct generator_bits<lagged_fibonacci_01<RealType, w, p, q> >
{
    static std::size_t value() { return w; }
};

}

#ifdef BOOST_RANDOM_DOXYGEN
namespace detail {
/**
 * The specializations lagged_fibonacci607 ... lagged_fibonacci44497
 * use well tested lags.
 *
 * See
 *
 *  @blockquote
 *  "On the Periods of Generalized Fibonacci Recurrences", Richard P. Brent
 *  Computer Sciences Laboratory Australian National University, December 1992
 *  @endblockquote
 *
 * The lags used here can be found in
 *
 *  @blockquote
 *  "Uniform random number generators for supercomputers", Richard Brent,
 *  Proc. of Fifth Australian Supercomputer Conference, Melbourne,
 *  Dec. 1992, pp. 704-706.
 *  @endblockquote
 */
struct lagged_fibonacci_doc {};
}
#endif

/** @copydoc boost::random::detail::lagged_fibonacci_doc */
typedef lagged_fibonacci_01_engine<double, 48, 607, 273> lagged_fibonacci607;
/** @copydoc boost::random::detail::lagged_fibonacci_doc */
typedef lagged_fibonacci_01_engine<double, 48, 1279, 418> lagged_fibonacci1279;
/** @copydoc boost::random::detail::lagged_fibonacci_doc */
typedef lagged_fibonacci_01_engine<double, 48, 2281, 1252> lagged_fibonacci2281;
/** @copydoc boost::random::detail::lagged_fibonacci_doc */
typedef lagged_fibonacci_01_engine<double, 48, 3217, 576> lagged_fibonacci3217;
/** @copydoc boost::random::detail::lagged_fibonacci_doc */
typedef lagged_fibonacci_01_engine<double, 48, 4423, 2098> lagged_fibonacci4423;
/** @copydoc boost::random::detail::lagged_fibonacci_doc */
typedef lagged_fibonacci_01_engine<double, 48, 9689, 5502> lagged_fibonacci9689;
/** @copydoc boost::random::detail::lagged_fibonacci_doc */
typedef lagged_fibonacci_01_engine<double, 48, 19937, 9842> lagged_fibonacci19937;
/** @copydoc boost::random::detail::lagged_fibonacci_doc */
typedef lagged_fibonacci_01_engine<double, 48, 23209, 13470> lagged_fibonacci23209;
/** @copydoc boost::random::detail::lagged_fibonacci_doc */
typedef lagged_fibonacci_01_engine<double, 48, 44497, 21034> lagged_fibonacci44497;

} // namespace random

using random::lagged_fibonacci607;
using random::lagged_fibonacci1279;
using random::lagged_fibonacci2281;
using random::lagged_fibonacci3217;
using random::lagged_fibonacci4423;
using random::lagged_fibonacci9689;
using random::lagged_fibonacci19937;
using random::lagged_fibonacci23209;
using random::lagged_fibonacci44497;

} // namespace boost

#endif // BOOST_RANDOM_LAGGED_FIBONACCI_HPP
