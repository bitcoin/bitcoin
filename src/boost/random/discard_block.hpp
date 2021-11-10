/* boost random/discard_block.hpp header file
 *
 * Copyright Jens Maurer 2002
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 * Revision history
 *  2001-03-02  created
 */

#ifndef BOOST_RANDOM_DISCARD_BLOCK_HPP
#define BOOST_RANDOM_DISCARD_BLOCK_HPP

#include <iostream>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/detail/seed_impl.hpp>


namespace boost {
namespace random {

/**
 * The class template \discard_block_engine is a model of
 * \pseudo_random_number_generator.  It modifies
 * another generator by discarding parts of its output.
 * Out of every block of @c p results, the first @c r
 * will be returned and the rest discarded.
 *
 * Requires: 0 < p <= r
 */
template<class UniformRandomNumberGenerator, std::size_t p, std::size_t r>
class discard_block_engine
{
    typedef typename detail::seed_type<
        typename UniformRandomNumberGenerator::result_type>::type seed_type;
public:
    typedef UniformRandomNumberGenerator base_type;
    typedef typename base_type::result_type result_type;

    BOOST_STATIC_CONSTANT(std::size_t, block_size = p);
    BOOST_STATIC_CONSTANT(std::size_t, used_block = r);

    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    BOOST_STATIC_CONSTANT(std::size_t, total_block = p);
    BOOST_STATIC_CONSTANT(std::size_t, returned_block = r);

    BOOST_STATIC_ASSERT(total_block >= returned_block);

    /** Uses the default seed for the base generator. */
    discard_block_engine() : _rng(), _n(0) { }
    /** Constructs a new \discard_block_engine with a copy of rng. */
    explicit discard_block_engine(const base_type & rng) : _rng(rng), _n(0) { }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    /** Constructs a new \discard_block_engine with rng. */
    explicit discard_block_engine(base_type && rng) : _rng(rng), _n(0) { }
#endif

    /**
     * Creates a new \discard_block_engine and seeds the underlying
     * generator with @c value
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(discard_block_engine,
                                               seed_type, value)
    { _rng.seed(value); _n = 0; }
    
    /**
     * Creates a new \discard_block_engine and seeds the underlying
     * generator with @c seq
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(discard_block_engine, SeedSeq, seq)
    { _rng.seed(seq); _n = 0; }
    
    /**
     * Creates a new \discard_block_engine and seeds the underlying
     * generator with first and last.
     */
    template<class It> discard_block_engine(It& first, It last)
      : _rng(first, last), _n(0) { }
    
    /** default seeds the underlying generator. */
    void seed() { _rng.seed(); _n = 0; }
    /** Seeds the underlying generator with s. */
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(discard_block_engine, seed_type, s)
    { _rng.seed(s); _n = 0; }
    /** Seeds the underlying generator with seq. */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(discard_block_engine, SeedSeq, seq)
    { _rng.seed(seq); _n = 0; }
    /** Seeds the underlying generator with first and last. */
    template<class It> void seed(It& first, It last)
    { _rng.seed(first, last); _n = 0; }

    /** Returns the underlying engine. */
    const base_type& base() const { return _rng; }

    /** Returns the next value of the generator. */
    result_type operator()()
    {
        if(_n >= returned_block) {
            // discard values of random number generator
            // Don't use discard, since we still need to
            // be somewhat compatible with TR1.
            // _rng.discard(total_block - _n);
            for(std::size_t i = 0; i < total_block - _n; ++i) {
                _rng();
            }
            _n = 0;
        }
        ++_n;
        return _rng();
    }

    void discard(boost::uintmax_t z)
    {
        for(boost::uintmax_t j = 0; j < z; ++j) {
            (*this)();
        }
    }

    template<class It>
    void generate(It first, It last)
    { detail::generate(*this, first, last); }

    /**
     * Returns the smallest value that the generator can produce.
     * This is the same as the minimum of the underlying generator.
     */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (base_type::min)(); }
    /**
     * Returns the largest value that the generator can produce.
     * This is the same as the maximum of the underlying generator.
     */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (base_type::max)(); }

#ifndef BOOST_RANDOM_NO_STREAM_OPERATORS
    /** Writes a \discard_block_engine to a @c std::ostream. */
    template<class CharT, class Traits>
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& os,
               const discard_block_engine& s)
    {
        os << s._rng << ' ' << s._n;
        return os;
    }

    /** Reads a \discard_block_engine from a @c std::istream. */
    template<class CharT, class Traits>
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& is, discard_block_engine& s)
    {
        is >> s._rng >> std::ws >> s._n;
        return is;
    }
#endif

    /** Returns true if the two generators will produce identical sequences. */
    friend bool operator==(const discard_block_engine& x,
                           const discard_block_engine& y)
    { return x._rng == y._rng && x._n == y._n; }
    /** Returns true if the two generators will produce different sequences. */
    friend bool operator!=(const discard_block_engine& x,
                           const discard_block_engine& y)
    { return !(x == y); }

private:
    base_type _rng;
    std::size_t _n;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
//  A definition is required even for integral static constants
template<class URNG, std::size_t p, std::size_t r>
const bool discard_block_engine<URNG, p, r>::has_fixed_range;
template<class URNG, std::size_t p, std::size_t r>
const std::size_t discard_block_engine<URNG, p, r>::total_block;
template<class URNG, std::size_t p, std::size_t r>
const std::size_t discard_block_engine<URNG, p, r>::returned_block;
template<class URNG, std::size_t p, std::size_t r>
const std::size_t discard_block_engine<URNG, p, r>::block_size;
template<class URNG, std::size_t p, std::size_t r>
const std::size_t discard_block_engine<URNG, p, r>::used_block;
#endif

/// \cond \show_deprecated

template<class URNG, int p, int r>
class discard_block : public discard_block_engine<URNG, p, r>
{
    typedef discard_block_engine<URNG, p, r> base_t;
public:
    typedef typename base_t::result_type result_type;
    discard_block() {}
    template<class T>
    discard_block(T& arg) : base_t(arg) {}
    template<class T>
    discard_block(const T& arg) : base_t(arg) {}
    template<class It>
    discard_block(It& first, It last) : base_t(first, last) {}
    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (this->base().min)(); }
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return (this->base().max)(); }
};

/// \endcond

namespace detail {

    template<class Engine>
    struct generator_bits;
    
    template<class URNG, std::size_t p, std::size_t r>
    struct generator_bits<discard_block_engine<URNG, p, r> > {
        static std::size_t value() { return generator_bits<URNG>::value(); }
    };

    template<class URNG, int p, int r>
    struct generator_bits<discard_block<URNG, p, r> > {
        static std::size_t value() { return generator_bits<URNG>::value(); }
    };

}

} // namespace random

} // namespace boost

#endif // BOOST_RANDOM_DISCARD_BLOCK_HPP
