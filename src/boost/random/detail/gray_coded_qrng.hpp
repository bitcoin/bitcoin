/* boost random/detail/gray_coded_qrng.hpp header file
 *
 * Copyright Justinas Vygintas Daugmaudis 2010-2018
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_RANDOM_DETAIL_GRAY_CODED_QRNG_HPP
#define BOOST_RANDOM_DETAIL_GRAY_CODED_QRNG_HPP

#include <boost/random/detail/qrng_base.hpp>

#include <boost/core/bit.hpp> // lsb
#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <functional> // bit_xor
#include <algorithm>

#include <boost/type_traits/conditional.hpp>

#include <boost/integer/integer_mask.hpp>

//!\file
//!Describes the gray-coded quasi-random number generator base class template.

namespace boost {
namespace random {

namespace qrng_detail {

template<class T> static int lsb( T x )
{
  if( x == 0 )
  {
    BOOST_THROW_EXCEPTION( std::range_error( "qrng_detail::lsb: argument is 0" ) );
  }

  return boost::core::countr_zero( x );
}

template<class T> static int msb( T x )
{
  if( x == 0 )
  {
    BOOST_THROW_EXCEPTION( std::range_error( "qrng_detail::msb: argument is 0" ) );
  }

  return std::numeric_limits<T>::digits - 1 - boost::core::countl_zero( x );
}

template<typename LatticeT>
class gray_coded_qrng
  : public qrng_base<
      gray_coded_qrng<LatticeT>
    , LatticeT
    , typename LatticeT::value_type
    >
{
public:
  typedef typename LatticeT::value_type result_type;
  typedef result_type size_type;

private:
  typedef gray_coded_qrng<LatticeT> self_t;
  typedef qrng_base<self_t, LatticeT, size_type> base_t;

  // The base needs to access modifying member f-ns, and we
  // don't want these functions to be available for the public use
  friend class qrng_base<self_t, LatticeT, size_type>;

  // Respect lattice bit_count here
  struct check_nothing {
    inline static void bit_pos(unsigned) {}
    inline static void code_size(size_type) {}
  };
  struct check_bit_range {
    static void raise_bit_count() {
      boost::throw_exception( std::range_error("gray_coded_qrng: bit_count") );
    }
    inline static void bit_pos(unsigned bit_pos) {
      if (bit_pos >= LatticeT::bit_count)
        raise_bit_count();
    }
    inline static void code_size(size_type code) {
      if (code > (self_t::max)())
        raise_bit_count();
    }
  };

  // We only want to check whether bit pos is outside the range if given bit_count
  // is narrower than the size_type, otherwise checks compile to nothing.
  BOOST_STATIC_ASSERT(LatticeT::bit_count <= std::numeric_limits<size_type>::digits);

  typedef typename conditional<
      ((LatticeT::bit_count) < std::numeric_limits<size_type>::digits)
    , check_bit_range
    , check_nothing
  >::type check_bit_range_t;

public:
  //!Returns: Tight lower bound on the set of values returned by operator().
  //!
  //!Throws: nothing.
  static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
  { return 0; }

  //!Returns: Tight upper bound on the set of values returned by operator().
  //!
  //!Throws: nothing.
  static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
  { return low_bits_mask_t<LatticeT::bit_count>::sig_bits; }

  explicit gray_coded_qrng(std::size_t dimension)
    : base_t(dimension)
  {}

  // default copy c-tor is fine

  // default assignment operator is fine

  void seed()
  {
    set_zero_state();
    update_quasi(0);
    base_t::reset_seq(0);
  }

  void seed(const size_type init)
  {
    if (init != this->curr_seq())
    {
      // We don't want negative seeds.
      check_seed_sign(init);

      size_type seq_code = init + 1;
      if (BOOST_UNLIKELY(!(init < seq_code)))
        boost::throw_exception( std::range_error("gray_coded_qrng: seed") );

      seq_code ^= (seq_code >> 1);
      // Fail if we see that seq_code is outside bit range.
      // We do that before we even touch engine state.
      check_bit_range_t::code_size(seq_code);

      set_zero_state();
      for (unsigned r = 0; seq_code != 0; ++r, seq_code >>= 1)
      {
        if (seq_code & static_cast<size_type>(1))
          update_quasi(r);
      }
    }
    // Everything went well, set the new seq count
    base_t::reset_seq(init);
  }

private:

  void compute_seq(size_type seq)
  {
    // Find the position of the least-significant zero in sequence count.
    // This is the bit that changes in the Gray-code representation as
    // the count is advanced.
    // Xor'ing with max() has the effect of flipping all the bits in seq,
    // except for the sign bit.
    unsigned r = qrng_detail::lsb(static_cast<size_type>(seq ^ (self_t::max)()));
    check_bit_range_t::bit_pos(r);
    update_quasi(r);
  }

  void update_quasi(unsigned r)
  {
    // Calculate the next state.
    std::transform(this->state_begin(), this->state_end(),
      this->lattice.iter_at(r * this->dimension()), this->state_begin(),
      std::bit_xor<result_type>());
  }

  void set_zero_state()
  {
    std::fill(this->state_begin(), this->state_end(), result_type /*zero*/ ());
  }
};

} // namespace qrng_detail

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_DETAIL_GRAY_CODED_QRNG_HPP
