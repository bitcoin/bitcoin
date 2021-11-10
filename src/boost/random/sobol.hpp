/* boost random/sobol.hpp header file
 *
 * Copyright Justinas Vygintas Daugmaudis 2010-2018
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_RANDOM_SOBOL_HPP
#define BOOST_RANDOM_SOBOL_HPP

#include <boost/random/detail/sobol_table.hpp>
#include <boost/random/detail/gray_coded_qrng.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace random {

/** @cond */
namespace qrng_detail {

// sobol_lattice sets up the random-number generator to produce a Sobol
// sequence of at most max dims-dimensional quasi-random vectors.
// Adapted from ACM TOMS algorithm 659, see

// http://doi.acm.org/10.1145/42288.214372

template<typename UIntType, unsigned w, typename SobolTables>
struct sobol_lattice
{
  typedef UIntType value_type;

  BOOST_STATIC_ASSERT(w > 0u);
  BOOST_STATIC_CONSTANT(unsigned, bit_count = w);

private:
  typedef std::vector<value_type> container_type;

public:
  explicit sobol_lattice(std::size_t dimension)
  {
    resize(dimension);
  }

  // default copy c-tor is fine

  void resize(std::size_t dimension)
  {
    dimension_assert("Sobol", dimension, SobolTables::max_dimension);

    // Initialize the bit array
    container_type cj(bit_count * dimension);

    // Initialize direction table in dimension 0
    for (unsigned k = 0; k != bit_count; ++k)
      cj[dimension*k] = static_cast<value_type>(1);

    // Initialize in remaining dimensions.
    for (std::size_t dim = 1; dim < dimension; ++dim)
    {
      const typename SobolTables::value_type poly = SobolTables::polynomial(dim-1);
      if (poly > (std::numeric_limits<value_type>::max)()) {
        boost::throw_exception( std::range_error("sobol: polynomial value outside the given value type range") );
      }
      const unsigned degree = qrng_detail::msb(poly); // integer log2(poly)

      // set initial values of m from table
      for (unsigned k = 0; k != degree; ++k)
        cj[dimension*k + dim] = SobolTables::minit(dim-1, k);

      // Calculate remaining elements for this dimension,
      // as explained in Bratley+Fox, section 2.
      for (unsigned j = degree; j < bit_count; ++j)
      {
        typename SobolTables::value_type p_i = poly;
        const std::size_t bit_offset = dimension*j + dim;

        cj[bit_offset] = cj[dimension*(j-degree) + dim];
        for (unsigned k = 0; k != degree; ++k, p_i >>= 1)
        {
          int rem = degree - k;
          cj[bit_offset] ^= ((p_i & 1) * cj[dimension*(j-rem) + dim]) << rem;
        }
      }
    }

    // Shift columns by appropriate power of 2.
    unsigned p = 1u;
    for (int j = bit_count-1-1; j >= 0; --j, ++p)
    {
      const std::size_t bit_offset = dimension * j;
      for (std::size_t dim = 0; dim != dimension; ++dim)
        cj[bit_offset + dim] <<= p;
    }

    bits.swap(cj);
  }

  typename container_type::const_iterator iter_at(std::size_t n) const
  {
    BOOST_ASSERT(!(n > bits.size()));
    return bits.begin() + n;
  }

private:
  container_type bits;
};

} // namespace qrng_detail

typedef detail::qrng_tables::sobol default_sobol_table;

/** @endcond */

//!Instantiations of class template sobol_engine model a \quasi_random_number_generator.
//!The sobol_engine uses the algorithm described in
//! \blockquote
//![Bratley+Fox, TOMS 14, 88 (1988)]
//!and [Antonov+Saleev, USSR Comput. Maths. Math. Phys. 19, 252 (1980)]
//! \endblockquote
//!
//!\attention sobol_engine skips trivial zeroes at the start of the sequence. For example, the beginning
//!of the 2-dimensional Sobol sequence in @c uniform_01 distribution will look like this:
//!\code{.cpp}
//!0.5, 0.5,
//!0.75, 0.25,
//!0.25, 0.75,
//!0.375, 0.375,
//!0.875, 0.875,
//!...
//!\endcode
//!
//!In the following documentation @c X denotes the concrete class of the template
//!sobol_engine returning objects of type @c UIntType, u and v are the values of @c X.
//!
//!Some member functions may throw exceptions of type @c std::range_error. This
//!happens when the quasi-random domain is exhausted and the generator cannot produce
//!any more values. The length of the low discrepancy sequence is given by \f$L=Dimension \times (2^{w} - 1)\f$.
template<typename UIntType, unsigned w, typename SobolTables = default_sobol_table>
class sobol_engine
  : public qrng_detail::gray_coded_qrng<
      qrng_detail::sobol_lattice<UIntType, w, SobolTables>
    >
{
  typedef qrng_detail::sobol_lattice<UIntType, w, SobolTables> lattice_t;
  typedef qrng_detail::gray_coded_qrng<lattice_t> base_t;

public:
  //!Effects: Constructs the default `s`-dimensional Sobol quasi-random number generator.
  //!
  //!Throws: bad_alloc, invalid_argument, range_error.
  explicit sobol_engine(std::size_t s)
    : base_t(s)
  {}

  // default copy c-tor is fine

#ifdef BOOST_RANDOM_DOXYGEN
  //=========================Doxygen needs this!==============================
  typedef UIntType result_type;

  /** @copydoc boost::random::niederreiter_base2_engine::min() */
  static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
  { return (base_t::min)(); }

  /** @copydoc boost::random::niederreiter_base2_engine::max() */
  static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
  { return (base_t::max)(); }

  /** @copydoc boost::random::niederreiter_base2_engine::dimension() */
  std::size_t dimension() const { return base_t::dimension(); }

  /** @copydoc boost::random::niederreiter_base2_engine::seed() */
  void seed()
  {
    base_t::seed();
  }

  /** @copydoc boost::random::niederreiter_base2_engine::seed(UIntType) */
  void seed(UIntType init)
  {
    base_t::seed(init);
  }

  /** @copydoc boost::random::niederreiter_base2_engine::operator()() */
  result_type operator()()
  {
    return base_t::operator()();
  }

  /** @copydoc boost::random::niederreiter_base2_engine::discard(boost::uintmax_t) */
  void discard(boost::uintmax_t z)
  {
    base_t::discard(z);
  }

  /** Returns true if the two generators will produce identical sequences of outputs. */
  BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(sobol_engine, x, y)
  { return static_cast<const base_t&>(x) == y; }

  /** Returns true if the two generators will produce different sequences of outputs. */
  BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(sobol_engine)

  /** Writes the textual representation of the generator to a @c std::ostream. */
  BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, sobol_engine, s)
  { return os << static_cast<const base_t&>(s); }

  /** Reads the textual representation of the generator from a @c std::istream. */
  BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, sobol_engine, s)
  { return is >> static_cast<base_t&>(s); }

#endif // BOOST_RANDOM_DOXYGEN
};

/**
 * @attention This specialization of \sobol_engine supports up to 3667 dimensions.
 *
 * Data on the primitive binary polynomials `a` and the corresponding starting values `m`
 * for Sobol sequences in up to 21201 dimensions was taken from
 *
 *  @blockquote
 *  S. Joe and F. Y. Kuo, Constructing Sobol sequences with better two-dimensional projections,
 *  SIAM J. Sci. Comput. 30, 2635-2654 (2008).
 *  @endblockquote
 *
 * See the original tables up to dimension 21201: https://web.archive.org/web/20170802022909/http://web.maths.unsw.edu.au/~fkuo/sobol/new-joe-kuo-6.21201
 *
 * For practical reasons the default table uses only the subset of binary polynomials `a` < 2<sup>16</sup>.
 *
 * However, it is possible to provide your own table to \sobol_engine should the default one be insufficient.
 */
typedef sobol_engine<boost::uint_least64_t, 64u, default_sobol_table> sobol;

} // namespace random

} // namespace boost

#endif // BOOST_RANDOM_SOBOL_HPP
