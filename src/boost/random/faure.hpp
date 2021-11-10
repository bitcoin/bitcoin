/* boost random/faure.hpp header file
 *
 * Copyright Justinas Vygintas Daugmaudis 2010-2018
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_RANDOM_FAURE_HPP
#define BOOST_RANDOM_FAURE_HPP

#include <boost/random/detail/qrng_base.hpp>

#include <cmath>
#include <vector>
#include <algorithm>

#include <boost/assert.hpp>

namespace boost {
namespace random {

/** @cond */
namespace detail {

namespace qrng_tables {

// There is no particular reason why 187 first primes were chosen
// to be put into this table. The only reason was, perhaps, that
// the number of dimensions for Faure generator would be around
// the same order of magnitude as the number of dimensions supported
// by the Sobol qrng.
struct primes
{
  typedef unsigned short value_type;

  BOOST_STATIC_CONSTANT(int, number_of_primes = 187);

  // A function that returns lower bound prime for a given n
  static value_type lower_bound(std::size_t n)
  {
    static const value_type prim_a[number_of_primes] = {
      2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
      59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
      127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
      191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251,
      257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317,
      331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397,
      401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
      467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557,
      563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619,
      631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701,
      709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787,
      797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863,
      877, 881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953,
      967, 971, 977, 983, 991, 997, 1009, 1013, 1019, 1021, 1031,
      1033, 1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093,
      1097, 1103, 1109, 1117 };

    qrng_detail::dimension_assert("Faure", n, prim_a[number_of_primes - 1]);

    return *std::lower_bound(prim_a, prim_a + number_of_primes, n);
  }
};

} // namespace qrng_tables
} // namespace detail

namespace qrng_detail {
namespace fr {

// Returns the integer part of the logarithm base Base of arg.
// In erroneous situations, e.g., integer_log(base, 0) the function
// returns 0 and does not report the error. This is the intended
// behavior.
template <typename T>
inline T integer_log(T base, T arg)
{
  T ilog = T();
  while (base <= arg)
  {
    arg /= base; ++ilog;
  }
  return ilog;
}

// Perform exponentiation by squaring (potential for code reuse in multiprecision::powm)
template <typename T>
inline T integer_pow(T base, T e)
{
  T result = static_cast<T>(1);
  while (e)
  {
    if (e & static_cast<T>(1))
      result *= base;
    e >>= 1;
    base *= base;
  }
  return result;
}

} // namespace fr

// Computes a table of binomial coefficients modulo qs.
template<typename RealType, typename SeqSizeT, typename PrimeTable>
struct binomial_coefficients
{
  typedef RealType value_type;
  typedef SeqSizeT size_type;

  // Binomial values modulo qs_base will never be bigger than qs_base.
  // We can choose an appropriate integer type to hold modulo values and
  // shave off memory footprint.
  typedef typename PrimeTable::value_type packed_uint_t;

  // default copy c-tor is fine

  explicit binomial_coefficients(std::size_t dimension)
  {
    resize(dimension);
  }

  void resize(std::size_t dimension)
  {
    qs_base = PrimeTable::lower_bound(dimension);

    // Throw away previously computed coefficients.
    // This will trigger recomputation on next update
    coeff.clear();
  }

  template <typename Iterator>
  void update(size_type seq, Iterator first, Iterator last)
  {
    if (first != last)
    {
      const size_type ilog = fr::integer_log(static_cast<size_type>(qs_base), seq);
      const size_type hisum = ilog + 1;
      if (coeff.size() != size_hint(hisum)) {
        ytemp.resize(static_cast<std::size_t>(hisum)); // cast safe because log is small
        compute_coefficients(hisum);
        qs_pow = fr::integer_pow(static_cast<size_type>(qs_base), ilog);
      }

      *first = compute_recip(seq, ytemp.rbegin());

      // Find other components using the Faure method.
      ++first;
      for ( ; first != last; ++first)
      {
        *first = RealType();
        RealType r = static_cast<RealType>(1);

        for (size_type i = 0; i != hisum; ++i)
        {
          RealType ztemp = ytemp[static_cast<std::size_t>(i)] * upper_element(i, i, hisum);
          for (size_type j = i + 1; j != hisum; ++j)
            ztemp += ytemp[static_cast<std::size_t>(j)] * upper_element(i, j, hisum);

          // Sum ( J <= I <= HISUM ) ( old ytemp(i) * binom(i,j) ) mod QS.
          ytemp[static_cast<std::size_t>(i)] = std::fmod(ztemp, static_cast<RealType>(qs_base));
          r *= static_cast<RealType>(qs_base);
          *first += ytemp[static_cast<std::size_t>(i)] / r;
        }
      }
    }
  }

private:
  inline static size_type size_hint(size_type n)
  {
    return n * (n + 1) / 2;
  }

  packed_uint_t& upper_element(size_type i, size_type j, size_type dim)
  {
    BOOST_ASSERT( i < dim );
    BOOST_ASSERT( j < dim );
    BOOST_ASSERT( i <= j );
    return coeff[static_cast<std::size_t>((i * (2 * dim - i + 1)) / 2 + j - i)];
  }

  template<typename Iterator>
  RealType compute_recip(size_type seq, Iterator out) const
  {
    // Here we do
    //   Sum ( 0 <= J <= HISUM ) YTEMP(J) * QS**J
    //   Sum ( 0 <= J <= HISUM ) YTEMP(J) / QS**(J+1)
    // in one go
    RealType r = RealType();
    size_type m, k = qs_pow;
    for( ; k != 0; ++out, seq = m, k /= qs_base )
    {
      m  = seq % k;
      RealType v  = static_cast<RealType>((seq - m) / k); // RealType <- size type
      r += v;
      r /= static_cast<RealType>(qs_base);
      *out = v; // saves double dereference
    }
    return r;
  }

  void compute_coefficients(const size_type n)
  {
    // Resize and initialize to zero
    coeff.resize(static_cast<std::size_t>(size_hint(n)));
    std::fill(coeff.begin(), coeff.end(), packed_uint_t());

    // The first row and the diagonal is assigned to 1
    upper_element(0, 0, n) = 1;
    for (size_type i = 1; i < n; ++i)
    {
      upper_element(0, i, n) = 1;
      upper_element(i, i, n) = 1;
    }

    // Computes binomial coefficients MOD qs_base
    for (size_type i = 1; i < n; ++i)
    {
      for (size_type j = i + 1; j < n; ++j)
      {
        upper_element(i, j, n) = ( upper_element(i, j-1, n) +
                                   upper_element(i-1, j-1, n) ) % qs_base;
      }
    }
  }

private:
  packed_uint_t qs_base;

  // here we cache precomputed data; note that binomial coefficients have
  // to be recomputed iff the integer part of the logarithm of seq changes,
  // which happens relatively rarely.
  std::vector<packed_uint_t> coeff; // packed upper (!) triangular matrix
  std::vector<RealType> ytemp;
  size_type qs_pow;
};

} // namespace qrng_detail

typedef detail::qrng_tables::primes default_faure_prime_table;

/** @endcond */

//!Instantiations of class template faure_engine model a \quasi_random_number_generator.
//!The faure_engine uses the algorithm described in
//! \blockquote
//!Henri Faure,
//!Discrepance de suites associees a un systeme de numeration (en dimension s),
//!Acta Arithmetica,
//!Volume 41, 1982, pages 337-351.
//! \endblockquote
//
//! \blockquote
//!Bennett Fox,
//!Algorithm 647:
//!Implementation and Relative Efficiency of Quasirandom
//!Sequence Generators,
//!ACM Transactions on Mathematical Software,
//!Volume 12, Number 4, December 1986, pages 362-376.
//! \endblockquote
//!
//!In the following documentation @c X denotes the concrete class of the template
//!faure_engine returning objects of type @c RealType, u and v are the values of @c X.
//!
//!Some member functions may throw exceptions of type @c std::bad_alloc.
template<typename RealType, typename SeqSizeT, typename PrimeTable = default_faure_prime_table>
class faure_engine
  : public qrng_detail::qrng_base<
      faure_engine<RealType, SeqSizeT, PrimeTable>
    , qrng_detail::binomial_coefficients<RealType, SeqSizeT, PrimeTable>
    , SeqSizeT
    >
{
  typedef faure_engine<RealType, SeqSizeT, PrimeTable> self_t;

  typedef qrng_detail::binomial_coefficients<RealType, SeqSizeT, PrimeTable> lattice_t;
  typedef qrng_detail::qrng_base<self_t, lattice_t, SeqSizeT> base_t;

  friend class qrng_detail::qrng_base<self_t, lattice_t, SeqSizeT>;

public:
  typedef RealType result_type;

  /** @copydoc boost::random::niederreiter_base2_engine::min() */
  static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
  { return static_cast<result_type>(0); }

  /** @copydoc boost::random::niederreiter_base2_engine::max() */
  static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
  { return static_cast<result_type>(1); }

  //!Effects: Constructs the `s`-dimensional default Faure quasi-random number generator.
  //!
  //!Throws: bad_alloc, invalid_argument.
  explicit faure_engine(std::size_t s)
    : base_t(s) // initialize the binomial table here
  {}

  /** @copydetails boost::random::niederreiter_base2_engine::seed(UIntType)
   * Throws: bad_alloc.
   */
  void seed(SeqSizeT init = 0)
  {
    compute_seq(init);
    base_t::reset_seq(init);
  }

#ifdef BOOST_RANDOM_DOXYGEN
  //=========================Doxygen needs this!==============================

  /** @copydoc boost::random::niederreiter_base2_engine::dimension() */
  std::size_t dimension() const { return base_t::dimension(); }

  /** @copydoc boost::random::niederreiter_base2_engine::operator()() */
  result_type operator()()
  {
    return base_t::operator()();
  }

  /** @copydoc boost::random::niederreiter_base2_engine::discard(boost::uintmax_t)
   * Throws: bad_alloc.
   */
  void discard(boost::uintmax_t z)
  {
    base_t::discard(z);
  }

  /** Returns true if the two generators will produce identical sequences of outputs. */
  BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(faure_engine, x, y)
  { return static_cast<const base_t&>(x) == y; }

  /** Returns true if the two generators will produce different sequences of outputs. */
  BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(faure_engine)

  /** Writes the textual representation of the generator to a @c std::ostream. */
  BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, faure_engine, s)
  { return os << static_cast<const base_t&>(s); }

  /** Reads the textual representation of the generator from a @c std::istream. */
  BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, faure_engine, s)
  { return is >> static_cast<base_t&>(s); }

#endif // BOOST_RANDOM_DOXYGEN

private:
/** @cond hide_private_members */
  void compute_seq(SeqSizeT seq)
  {
    qrng_detail::check_seed_sign(seq);
    this->lattice.update(seq, this->state_begin(), this->state_end());
  }
/** @endcond */
};

/**
 * @attention This specialization of \faure_engine supports up to 1117 dimensions.
 *
 * However, it is possible to provide your own prime table to \faure_engine should the default one be insufficient.
 */
typedef faure_engine<double, boost::uint_least64_t, default_faure_prime_table> faure;

} // namespace random

} // namespace boost

#endif // BOOST_RANDOM_FAURE_HPP
