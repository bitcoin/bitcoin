/* boost random/detail/qrng_base.hpp header file
 *
 * Copyright Justinas Vygintas Daugmaudis 2010-2018
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_RANDOM_DETAIL_QRNG_BASE_HPP
#define BOOST_RANDOM_DETAIL_QRNG_BASE_HPP

#include <stdexcept>
#include <vector>
#include <limits>

#include <istream>
#include <ostream>
#include <sstream>

#include <boost/cstdint.hpp>
#include <boost/random/detail/operators.hpp>

#include <boost/throw_exception.hpp>

#include <boost/type_traits/integral_constant.hpp>

#include <boost/random/detail/disable_warnings.hpp>

//!\file
//!Describes the quasi-random number generator base class template.

namespace boost {
namespace random {

namespace qrng_detail {

// If the seed is a signed integer type, then we need to
// check that the value is positive:
template <typename Integer>
inline void check_seed_sign(const Integer& v, const boost::true_type)
{
  if (v < 0)
  {
    boost::throw_exception( std::range_error("seed must be a positive integer") );
  }
}
template <typename Integer>
inline void check_seed_sign(const Integer&, const boost::false_type) {}

template <typename Integer>
inline void check_seed_sign(const Integer& v)
{
  check_seed_sign(v, integral_constant<bool, std::numeric_limits<Integer>::is_signed>());
}


template<typename DerivedT, typename LatticeT, typename SizeT>
class qrng_base
{
public:
  typedef SizeT size_type;
  typedef typename LatticeT::value_type result_type;

  explicit qrng_base(std::size_t dimension)
    // Guard against invalid dimensions before creating the lattice
    : lattice(prevent_zero_dimension(dimension))
    , quasi_state(dimension)
  {
    derived().seed();
  }

  // default copy c-tor is fine

  // default assignment operator is fine

  //!Returns: The dimension of of the quasi-random domain.
  //!
  //!Throws: nothing.
  std::size_t dimension() const { return quasi_state.size(); }

  //!Returns: Returns a successive element of an s-dimensional
  //!(s = X::dimension()) vector at each invocation. When all elements are
  //!exhausted, X::operator() begins anew with the starting element of a
  //!subsequent s-dimensional vector.
  //!
  //!Throws: range_error.
  result_type operator()()
  {
    return curr_elem != dimension() ? load_cached(): next_state();
  }

  //!Fills a range with quasi-random values.
  template<typename Iter> void generate(Iter first, Iter last)
  {
    for (; first != last; ++first)
      *first = this->operator()();
  }

  //!Effects: Advances *this state as if z consecutive
  //!X::operator() invocations were executed.
  //!
  //!Throws: range_error.
  void discard(boost::uintmax_t z)
  {
    const std::size_t dimension_value = dimension();

    // Compiler knows how to optimize subsequent x / y and x % y
    // statements. In fact, gcc does this even at -O1, so don't
    // be tempted to "optimize" % via subtraction and multiplication.

    boost::uintmax_t vec_n = z / dimension_value;
    std::size_t carry = curr_elem + (z % dimension_value);

    vec_n += carry / dimension_value;
    carry  = carry % dimension_value;

    // Avoid overdiscarding by branchlessly correcting the triple
    // (D, S + 1, 0) to (D, S, D) (see equality operator)
    const bool corr = (!carry) & static_cast<bool>(vec_n);

    // Discards vec_n (with correction) consecutive s-dimensional vectors
    discard_vector(vec_n - static_cast<boost::uintmax_t>(corr));

#ifdef BOOST_MSVC
#pragma warning(push)
// disable unary minus operator applied to an unsigned type,
// result still unsigned.
#pragma warning(disable:4146)
#endif

    // Sets up the proper position of the element-to-read
    // curr_elem = carry + corr*dimension_value
    curr_elem = carry ^ (-static_cast<std::size_t>(corr) & dimension_value);

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
  }

  //!Writes the textual representation of the generator to a @c std::ostream.
  BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, qrng_base, s)
  {
    os << s.dimension() << " " << s.seq_count << " " << s.curr_elem;
    return os;
  }

  //!Reads the textual representation of the generator from a @c std::istream.
  BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, qrng_base, s)
  {
    std::size_t dim;
    size_type seed;
    boost::uintmax_t z;
    if (is >> dim >> std::ws >> seed >> std::ws >> z) // initialize iff success!
    {
      // Check seed sign before resizing the lattice and/or recomputing state
      check_seed_sign(seed);

      if (s.dimension() != prevent_zero_dimension(dim))
      {
        s.lattice.resize(dim);
        s.quasi_state.resize(dim);
      }
      // Fast-forward to the correct state
      s.derived().seed(seed);
      if (z != 0) s.discard(z);
    }
    return is;
  }

  //!Returns true if the two generators will produce identical sequences of outputs.
  BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(qrng_base, x, y)
  {
    const std::size_t dimension_value = x.dimension();

    // Note that two generators with different seq_counts and curr_elems can
    // produce the same sequence because the generator triple
    // (D, S, D) is equivalent to (D, S + 1, 0), where D is dimension, S -- seq_count,
    // and the last one is curr_elem.

    return (dimension_value == y.dimension()) &&
      // |x.seq_count - y.seq_count| <= 1
      !((x.seq_count < y.seq_count ? y.seq_count - x.seq_count : x.seq_count - y.seq_count)
          > static_cast<size_type>(1)) &&
      // Potential overflows don't matter here, since we've already ascertained
      // that sequence counts differ by no more than 1, so if they overflow, they
      // can overflow together.
      (x.seq_count + (x.curr_elem / dimension_value) == y.seq_count + (y.curr_elem / dimension_value)) &&
      (x.curr_elem % dimension_value == y.curr_elem % dimension_value);
  }

  //!Returns true if the two generators will produce different sequences of outputs.
  BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(qrng_base)

protected:
  typedef std::vector<result_type> state_type;
  typedef typename state_type::iterator state_iterator;

  // Getters
  size_type curr_seq() const { return seq_count; }

  state_iterator state_begin() { return quasi_state.begin(); }
  state_iterator state_end() { return quasi_state.end(); }

  // Setters
  void reset_seq(size_type seq)
  {
    seq_count = seq;
    curr_elem = 0u;
  }

private:
  DerivedT& derived() throw()
  {
    return *static_cast<DerivedT * const>(this);
  }

  // Load the result from the saved state.
  result_type load_cached()
  {
    return quasi_state[curr_elem++];
  }

  result_type next_state()
  {
    size_type new_seq = seq_count;
    if (BOOST_LIKELY(++new_seq > seq_count))
    {
      derived().compute_seq(new_seq);
      reset_seq(new_seq);
      return load_cached();
    }
    boost::throw_exception( std::range_error("qrng_base: next_state") );
  }

  // Discards z consecutive s-dimensional vectors,
  // and preserves the position of the element-to-read
  void discard_vector(boost::uintmax_t z)
  {
    const boost::uintmax_t max_z = (std::numeric_limits<size_type>::max)() - seq_count;

    // Don't allow seq_count + z overflows here
    if (max_z < z)
      boost::throw_exception( std::range_error("qrng_base: discard_vector") );

    std::size_t tmp = curr_elem;
    derived().seed(static_cast<size_type>(seq_count + z));
    curr_elem = tmp;
  }

  static std::size_t prevent_zero_dimension(std::size_t dimension)
  {
    if (dimension == 0)
      boost::throw_exception( std::invalid_argument("qrng_base: zero dimension") );
    return dimension;
  }

  // Member variables are so ordered with the intention
  // that the typical memory access pattern would be
  // incremental. Moreover, lattice is put before quasi_state
  // because we want to construct lattice first. Lattices
  // can do some kind of dimension sanity check (as in
  // dimension_assert below), and if that fails then we don't
  // need to do any more work.
private:
  std::size_t curr_elem;
  size_type seq_count;
protected:
  LatticeT lattice;
private:
  state_type quasi_state;
};

inline void dimension_assert(const char* generator, std::size_t dim, std::size_t maxdim)
{
  if (!dim || dim > maxdim)
  {
    std::ostringstream os;
    os << "The " << generator << " quasi-random number generator only supports dimensions in range [1; "
      << maxdim << "], but dimension " << dim << " was supplied.";
    boost::throw_exception( std::invalid_argument(os.str()) );
  }
}

} // namespace qrng_detail

} // namespace random
} // namespace boost

#include <boost/random/detail/enable_warnings.hpp>

#endif // BOOST_RANDOM_DETAIL_QRNG_BASE_HPP
