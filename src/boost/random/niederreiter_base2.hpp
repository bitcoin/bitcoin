/* boost random/nierderreiter_base2.hpp header file
 *
 * Copyright Justinas Vygintas Daugmaudis 2010-2018
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_RANDOM_NIEDERREITER_BASE2_HPP
#define BOOST_RANDOM_NIEDERREITER_BASE2_HPP

#include <boost/random/detail/niederreiter_base2_table.hpp>
#include <boost/random/detail/gray_coded_qrng.hpp>

#include <boost/dynamic_bitset.hpp>

namespace boost {
namespace random {

/** @cond */
namespace qrng_detail {
namespace nb2 {

// Return the base 2 logarithm for a given bitset v
template <typename DynamicBitset>
inline typename DynamicBitset::size_type
bitset_log2(const DynamicBitset& v)
{
  if (v.none())
    boost::throw_exception( std::invalid_argument("bitset_log2") );

  typename DynamicBitset::size_type hibit = v.size() - 1;
  while (!v.test(hibit))
    --hibit;
  return hibit;
}


// Multiply polynomials over Z_2.
template <typename PolynomialT, typename DynamicBitset>
inline void modulo2_multiply(PolynomialT P, DynamicBitset& v, DynamicBitset& pt)
{
  pt.reset(); // pt == 0
  for (; P; P >>= 1, v <<= 1)
    if (P & 1) pt ^= v;
  pt.swap(v);
}


// Calculate the values of the constants V(J,R) as
// described in BFN section 3.3.
//
// pb = polynomial defined in section 2.3 of BFN.
template <typename DynamicBitset>
inline void calculate_v(const DynamicBitset& pb,
  typename DynamicBitset::size_type kj,
  typename DynamicBitset::size_type pb_degree,
  DynamicBitset& v)
{
  typedef typename DynamicBitset::size_type size_type;

  // Now choose values of V in accordance with
  // the conditions in section 3.3.
  size_type r = 0;
  for ( ; r != kj; ++r)
    v.reset(r);

  // Quoting from BFN: "Our program currently sets each K_q
  // equal to eq. This has the effect of setting all unrestricted
  // values of v to 1."
  for ( ; r < pb_degree; ++r)
    v.set(r);

  // Calculate the remaining V's using the recursion of section 2.3,
  // remembering that the B's have the opposite sign.
  for ( ; r != v.size(); ++r)
  {
    bool term = false;
    for (typename DynamicBitset::size_type k = 0; k < pb_degree; ++k)
    {
      term ^= pb.test(k) & v[r + k - pb_degree];
    }
    v[r] = term;
  }
}

} // namespace nb2

template<typename UIntType, unsigned w, typename Nb2Table>
struct niederreiter_base2_lattice
{
  typedef UIntType value_type;

  BOOST_STATIC_ASSERT(w > 0u);
  BOOST_STATIC_CONSTANT(unsigned, bit_count = w);

private:
  typedef std::vector<value_type> container_type;

public:
  explicit niederreiter_base2_lattice(std::size_t dimension)
  {
    resize(dimension);
  }

  void resize(std::size_t dimension)
  {
    typedef boost::dynamic_bitset<> bitset_type;

    dimension_assert("Niederreiter base 2", dimension, Nb2Table::max_dimension);

    // Initialize the bit array
    container_type cj(bit_count * dimension);

    // Reserve temporary space for lattice computation
    bitset_type v, pb, tmp;

    // Compute Niedderreiter base 2 lattice
    for (std::size_t dim = 0; dim != dimension; ++dim)
    {
      const typename Nb2Table::value_type poly = Nb2Table::polynomial(dim);
      if (poly > (std::numeric_limits<value_type>::max)()) {
        boost::throw_exception( std::range_error("niederreiter_base2: polynomial value outside the given value type range") );
      }

      const unsigned degree = qrng_detail::msb(poly); // integer log2(poly)
      const unsigned space_required = degree * ((bit_count / degree) + 1); // ~ degree + bit_count

      v.resize(degree + bit_count - 1);

      // For each dimension, we need to calculate powers of an
      // appropriate irreducible polynomial, see Niederreiter
      // page 65, just below equation (19).
      // Copy the appropriate irreducible polynomial into PX,
      // and its degree into E. Set polynomial B = PX ** 0 = 1.
      // M is the degree of B. Subsequently B will hold higher
      // powers of PX.
      pb.resize(space_required); tmp.resize(space_required);

      typename bitset_type::size_type kj, pb_degree = 0;
      pb.reset(); // pb == 0
      pb.set(pb_degree); // set the proper bit for the pb_degree

      value_type j = high_bit_mask_t<bit_count - 1>::high_bit;
      do
      {
        // Now choose a value of Kj as defined in section 3.3.
        // We must have 0 <= Kj < E*J = M.
        // The limit condition on Kj does not seem to be very relevant
        // in this program.
        kj = pb_degree;

        // Now multiply B by PX so B becomes PX**J.
        // In section 2.3, the values of Bi are defined with a minus sign :
        // don't forget this if you use them later!
        nb2::modulo2_multiply(poly, pb, tmp);
        pb_degree += degree;
        if (pb_degree >= pb.size()) {
          // Note that it is quite possible for kj to become bigger than
          // the new computed value of pb_degree.
          pb_degree = nb2::bitset_log2(pb);
        }

        // If U = 0, we need to set B to the next power of PX
        // and recalculate V.
        nb2::calculate_v(pb, kj, pb_degree, v);

        // Niederreiter (page 56, after equation (7), defines two
        // variables Q and U.  We do not need Q explicitly, but we
        // do need U.

        // Advance Niederreiter's state variables.
        for (unsigned u = 0; j && u != degree; ++u, j >>= 1)
        {
          // Now C is obtained from V. Niederreiter
          // obtains A from V (page 65, near the bottom), and then gets
          // C from A (page 56, equation (7)).  However this can be done
          // in one step.  Here CI(J,R) corresponds to
          // Niederreiter's C(I,J,R), whose values we pack into array
          // CJ so that CJ(I,R) holds all the values of C(I,J,R) for J from 1 to NBITS.
          for (unsigned r = 0; r != bit_count; ++r) {
            value_type& num = cj[dimension * r + dim];
            // set the jth bit in num
            num = (num & ~j) | (-v[r + u] & j);
          }
        }
      } while (j != 0);
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

typedef detail::qrng_tables::niederreiter_base2 default_niederreiter_base2_table;

/** @endcond */

//!Instantiations of class template niederreiter_base2_engine model a \quasi_random_number_generator.
//!The niederreiter_base2_engine uses the algorithm described in
//! \blockquote
//!Bratley, Fox, Niederreiter, ACM Trans. Model. Comp. Sim. 2, 195 (1992).
//! \endblockquote
//!
//!\attention niederreiter_base2_engine skips trivial zeroes at the start of the sequence. For example,
//!the beginning of the 2-dimensional Niederreiter base 2 sequence in @c uniform_01 distribution will look
//!like this:
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
//!niederreiter_base2_engine returning objects of type @c UIntType, u and v are the values of @c X.
//!
//!Some member functions may throw exceptions of type std::range_error. This
//!happens when the quasi-random domain is exhausted and the generator cannot produce
//!any more values. The length of the low discrepancy sequence is given by
//! \f$L=Dimension \times (2^{w} - 1)\f$.
template<typename UIntType, unsigned w, typename Nb2Table = default_niederreiter_base2_table>
class niederreiter_base2_engine
  : public qrng_detail::gray_coded_qrng<
      qrng_detail::niederreiter_base2_lattice<UIntType, w, Nb2Table>
    >
{
  typedef qrng_detail::niederreiter_base2_lattice<UIntType, w, Nb2Table> lattice_t;
  typedef qrng_detail::gray_coded_qrng<lattice_t> base_t;

public:
  //!Effects: Constructs the default `s`-dimensional Niederreiter base 2 quasi-random number generator.
  //!
  //!Throws: bad_alloc, invalid_argument, range_error.
  explicit niederreiter_base2_engine(std::size_t s)
    : base_t(s) // initialize lattice here
  {}

#ifdef BOOST_RANDOM_DOXYGEN
  //=========================Doxygen needs this!==============================
  typedef UIntType result_type;

  //!Returns: Tight lower bound on the set of values returned by operator().
  //!
  //!Throws: nothing.
  static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
  { return (base_t::min)(); }

  //!Returns: Tight upper bound on the set of values returned by operator().
  //!
  //!Throws: nothing.
  static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
  { return (base_t::max)(); }

  //!Returns: The dimension of of the quasi-random domain.
  //!
  //!Throws: nothing.
  std::size_t dimension() const { return base_t::dimension(); }

  //!Effects: Resets the quasi-random number generator state to
  //!the one given by the default construction. Equivalent to u.seed(0).
  //!
  //!\brief Throws: nothing.
  void seed()
  {
    base_t::seed();
  }

  //!Effects: Effectively sets the quasi-random number generator state to the `init`-th
  //!vector in the `s`-dimensional quasi-random domain, where `s` == X::dimension().
  //!\code
  //!X u, v;
  //!for(int i = 0; i < N; ++i)
  //!    for( std::size_t j = 0; j < u.dimension(); ++j )
  //!        u();
  //!v.seed(N);
  //!assert(u() == v());
  //!\endcode
  //!
  //!\brief Throws: range_error.
  void seed(UIntType init)
  {
    base_t::seed(init);
  }

  //!Returns: Returns a successive element of an `s`-dimensional
  //!(s = X::dimension()) vector at each invocation. When all elements are
  //!exhausted, X::operator() begins anew with the starting element of a
  //!subsequent `s`-dimensional vector.
  //!
  //!Throws: range_error.
  result_type operator()()
  {
    return base_t::operator()();
  }

  //!Effects: Advances *this state as if `z` consecutive
  //!X::operator() invocations were executed.
  //!\code
  //!X u = v;
  //!for(int i = 0; i < N; ++i)
  //!    u();
  //!v.discard(N);
  //!assert(u() == v());
  //!\endcode
  //!
  //!Throws: range_error.
  void discard(boost::uintmax_t z)
  {
    base_t::discard(z);
  }

  //!Returns true if the two generators will produce identical sequences of outputs.
  BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(niederreiter_base2_engine, x, y)
  { return static_cast<const base_t&>(x) == y; }

  //!Returns true if the two generators will produce different sequences of outputs.
  BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(niederreiter_base2_engine)

  //!Writes the textual representation of the generator to a @c std::ostream.
  BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, niederreiter_base2_engine, s)
  { return os << static_cast<const base_t&>(s); }

  //!Reads the textual representation of the generator from a @c std::istream.
  BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, niederreiter_base2_engine, s)
  { return is >> static_cast<base_t&>(s); }

#endif // BOOST_RANDOM_DOXYGEN
};


/**
 * @attention This specialization of \niederreiter_base2_engine supports up to 4720 dimensions.
 *
 * Binary irreducible polynomials (primes in the ring `GF(2)[X]`, evaluated at `X=2`) were generated
 * while condition `max(prime)` < 2<sup>16</sup> was satisfied.
 *
 * There are exactly 4720 such primes, which yields a Niederreiter base 2 table for 4720 dimensions.
 *
 * However, it is possible to provide your own table to \niederreiter_base2_engine should the default one be insufficient.
 */
typedef niederreiter_base2_engine<boost::uint_least64_t, 64u, default_niederreiter_base2_table> niederreiter_base2;

} // namespace random

} // namespace boost

#endif // BOOST_RANDOM_NIEDERREITER_BASE2_HPP
