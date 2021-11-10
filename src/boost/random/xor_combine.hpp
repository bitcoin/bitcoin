/* boost random/xor_combine.hpp header file
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
 */

#ifndef BOOST_RANDOM_XOR_COMBINE_HPP
#define BOOST_RANDOM_XOR_COMBINE_HPP

#include <istream>
#include <iosfwd>
#include <cassert>
#include <algorithm> // for std::min and std::max
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/cstdint.hpp>     // uint32_t
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/seed_impl.hpp>
#include <boost/random/detail/operators.hpp>

namespace boost {
namespace random {

/**
 * Instantiations of @c xor_combine_engine model a
 * \pseudo_random_number_generator.  To produce its output it
 * invokes each of the base generators, shifts their results
 * and xors them together.
 */
template<class URNG1, int s1, class URNG2, int s2>
class xor_combine_engine
{
public:
    typedef URNG1 base1_type;
    typedef URNG2 base2_type;
    typedef typename base1_type::result_type result_type;

    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    BOOST_STATIC_CONSTANT(int, shift1 = s1);
    BOOST_STATIC_CONSTANT(int, shift2 = s2);

    /**
     * Constructors a @c xor_combine_engine by default constructing
     * both base generators.
     */
    xor_combine_engine() : _rng1(), _rng2() { }

    /** Constructs a @c xor_combine by copying two base generators. */
    xor_combine_engine(const base1_type & rng1, const base2_type & rng2)
      : _rng1(rng1), _rng2(rng2) { }

    /**
     * Constructs a @c xor_combine_engine, seeding both base generators
     * with @c v.
     *
     * @xmlwarning
     * The exact algorithm used by this function may change in the future.
     * @endxmlwarning
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(xor_combine_engine,
        result_type, v)
    { seed(v); }

    /**
     * Constructs a @c xor_combine_engine, seeding both base generators
     * with values produced by @c seq.
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(xor_combine_engine,
        SeedSeq, seq)
    { seed(seq); }

    /**
     * Constructs a @c xor_combine_engine, seeding both base generators
     * with values from the iterator range [first, last) and changes
     * first to point to the element after the last one used.  If there
     * are not enough elements in the range to seed both generators,
     * throws @c std::invalid_argument.
     */
    template<class It> xor_combine_engine(It& first, It last)
      : _rng1(first, last), _rng2( /* advanced by other call */ first, last) { }

    /** Calls @c seed() for both base generators. */
    void seed() { _rng1.seed(); _rng2.seed(); }

    /** @c seeds both base generators with @c v. */
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(xor_combine_engine, result_type, v)
    { _rng1.seed(v); _rng2.seed(v); }

    /** @c seeds both base generators with values produced by @c seq. */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(xor_combine_engine, SeedSeq, seq)
    { _rng1.seed(seq); _rng2.seed(seq); }

    /**
     * seeds both base generators with values from the iterator
     * range [first, last) and changes first to point to the element
     * after the last one used.  If there are not enough elements in
     * the range to seed both generators, throws @c std::invalid_argument.
     */
    template<class It> void seed(It& first, It last)
    {
        _rng1.seed(first, last);
        _rng2.seed(first, last);
    }

    /** Returns the first base generator. */
    const base1_type& base1() const { return _rng1; }

    /** Returns the second base generator. */
    const base2_type& base2() const { return _rng2; }

    /** Returns the next value of the generator. */
    result_type operator()()
    {
        return (_rng1() << s1) ^ (_rng2() << s2);
    }
  
    /** Fills a range with random values */
    template<class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    /** Advances the state of the generator by @c z. */
    void discard(boost::uintmax_t z)
    {
        _rng1.discard(z);
        _rng2.discard(z);
    }

    /** Returns the smallest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (URNG1::min)()<(URNG2::min)()?(URNG1::min)():(URNG2::min)(); }
    /** Returns the largest value that the generator can produce. */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (URNG1::max)()>(URNG2::max)()?(URNG1::max)():(URNG2::max)(); }

    /**
     * Writes the textual representation of the generator to a @c std::ostream.
     */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, xor_combine_engine, s)
    {
        os << s._rng1 << ' ' << s._rng2;
        return os;
    }
    
    /**
     * Reads the textual representation of the generator from a @c std::istream.
     */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, xor_combine_engine, s)
    {
        is >> s._rng1 >> std::ws >> s._rng2;
        return is;
    }
    
    /** Returns true if the two generators will produce identical sequences. */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(xor_combine_engine, x, y)
    { return x._rng1 == y._rng1 && x._rng2 == y._rng2; }
    
    /** Returns true if the two generators will produce different sequences. */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(xor_combine_engine)

private:
    base1_type _rng1;
    base2_type _rng2;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class URNG1, int s1, class URNG2, int s2>
const bool xor_combine_engine<URNG1, s1, URNG2, s2>::has_fixed_range;
template<class URNG1, int s1, class URNG2, int s2>
const int xor_combine_engine<URNG1, s1, URNG2, s2>::shift1;
template<class URNG1, int s1, class URNG2, int s2>
const int xor_combine_engine<URNG1, s1, URNG2, s2>::shift2;
#endif

/// \cond show_private

/** Provided for backwards compatibility. */
template<class URNG1, int s1, class URNG2, int s2,
    typename URNG1::result_type v = 0>
class xor_combine : public xor_combine_engine<URNG1, s1, URNG2, s2>
{
    typedef xor_combine_engine<URNG1, s1, URNG2, s2> base_type;
public:
    typedef typename base_type::result_type result_type;
    xor_combine() {}
    xor_combine(result_type val) : base_type(val) {}
    template<class It>
    xor_combine(It& first, It last) : base_type(first, last) {}
    xor_combine(const URNG1 & rng1, const URNG2 & rng2)
      : base_type(rng1, rng2) { }

    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return (std::min)((this->base1().min)(), (this->base2().min)()); }
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return (std::max)((this->base1().min)(), (this->base2().max)()); }
};

/// \endcond

} // namespace random
} // namespace boost

#endif // BOOST_RANDOM_XOR_COMBINE_HPP
