/* boost random/additive_combine.hpp header file
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

#ifndef BOOST_RANDOM_ADDITIVE_COMBINE_HPP
#define BOOST_RANDOM_ADDITIVE_COMBINE_HPP

#include <istream>
#include <iosfwd>
#include <algorithm> // for std::min and std::max
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/random/detail/config.hpp>
#include <boost/random/detail/operators.hpp>
#include <boost/random/detail/seed.hpp>
#include <boost/random/linear_congruential.hpp>

namespace boost {
namespace random {

/**
 * An instantiation of class template @c additive_combine_engine models a
 * \pseudo_random_number_generator. It combines two multiplicative
 * \linear_congruential_engine number generators, i.e. those with @c c = 0.
 * It is described in
 *
 *  @blockquote
 *  "Efficient and Portable Combined Random Number Generators", Pierre L'Ecuyer,
 *  Communications of the ACM, Vol. 31, No. 6, June 1988, pp. 742-749, 774
 *  @endblockquote
 *
 * The template parameters MLCG1 and MLCG2 shall denote two different
 * \linear_congruential_engine number generators, each with c = 0. Each
 * invocation returns a random number
 * X(n) := (MLCG1(n) - MLCG2(n)) mod (m1 - 1),
 * where m1 denotes the modulus of MLCG1. 
 */
template<class MLCG1, class MLCG2>
class additive_combine_engine
{
public:
    typedef MLCG1 first_base;
    typedef MLCG2 second_base;
    typedef typename MLCG1::result_type result_type;

    // Required by old Boost.Random concept
    BOOST_STATIC_CONSTANT(bool, has_fixed_range = false);
    /**
     * Returns the smallest value that the generator can produce
     */
    static BOOST_CONSTEXPR result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return 1; }
    /**
     * Returns the largest value that the generator can produce
     */
    static BOOST_CONSTEXPR result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return MLCG1::modulus-1; }

    /**
     * Constructs an @c additive_combine_engine using the
     * default constructors of the two base generators.
     */
    additive_combine_engine() : _mlcg1(), _mlcg2() { }
    /**
     * Constructs an @c additive_combine_engine, using seed as
     * the constructor argument for both base generators.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(additive_combine_engine,
        result_type, seed_arg)
    {
        _mlcg1.seed(seed_arg);
        _mlcg2.seed(seed_arg);
    }
    /**
     * Constructs an @c additive_combine_engine, using seq as
     * the constructor argument for both base generators.
     *
     * @xmlwarning
     * The semantics of this function are liable to change.
     * A @c seed_seq is designed to generate all the seeds
     * in one shot, but this seeds the two base engines
     * independantly and probably ends up giving the same
     * sequence to both.
     * @endxmlwarning
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(additive_combine_engine,
        SeedSeq, seq)
    {
        _mlcg1.seed(seq);
        _mlcg2.seed(seq);
    }
    /**
     * Constructs an @c additive_combine_engine, using
     * @c seed1 and @c seed2 as the constructor argument to
     * the first and second base generators, respectively.
     */
    additive_combine_engine(typename MLCG1::result_type seed1, 
                            typename MLCG2::result_type seed2)
      : _mlcg1(seed1), _mlcg2(seed2) { }
    /**
     * Contructs an @c additive_combine_engine with
     * values from the range defined by the input iterators first
     * and last.  first will be modified to point to the element
     * after the last one used.
     *
     * Throws: @c std::invalid_argument if the input range is too small.
     *
     * Exception Safety: Basic
     */
    template<class It> additive_combine_engine(It& first, It last)
      : _mlcg1(first, last), _mlcg2(first, last) { }

    /**
     * Seeds an @c additive_combine_engine using the default
     * seeds of the two base generators.
     */
    void seed()
    {
        _mlcg1.seed();
        _mlcg2.seed();
    }

    /**
     * Seeds an @c additive_combine_engine, using @c seed as the
     * seed for both base generators.
     */
    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(additive_combine_engine,
        result_type, seed_arg)
    {
        _mlcg1.seed(seed_arg);
        _mlcg2.seed(seed_arg);
    }

    /**
     * Seeds an @c additive_combine_engine, using @c seq to
     * seed both base generators.
     *
     * See the warning on the corresponding constructor.
     */
    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(additive_combine_engine,
        SeedSeq, seq)
    {
        _mlcg1.seed(seq);
        _mlcg2.seed(seq);
    }

    /**
     * Seeds an @c additive_combine generator, using @c seed1 and @c seed2 as
     * the seeds to the first and second base generators, respectively.
     */
    void seed(typename MLCG1::result_type seed1,
              typename MLCG2::result_type seed2)
    {
        _mlcg1.seed(seed1);
        _mlcg2.seed(seed2);
    }

    /**
     * Seeds an @c additive_combine_engine with
     * values from the range defined by the input iterators first
     * and last.  first will be modified to point to the element
     * after the last one used.
     *
     * Throws: @c std::invalid_argument if the input range is too small.
     *
     * Exception Safety: Basic
     */
    template<class It> void seed(It& first, It last)
    {
        _mlcg1.seed(first, last);
        _mlcg2.seed(first, last);
    }

    /** Returns the next value of the generator. */
    result_type operator()() {
        result_type val1 = _mlcg1();
        result_type val2 = _mlcg2();
        if(val2 < val1) return val1 - val2;
        else return val1 - val2 + MLCG1::modulus - 1;
    }
  
    /** Fills a range with random values */
    template<class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    /** Advances the state of the generator by @c z. */
    void discard(boost::uintmax_t z)
    {
        _mlcg1.discard(z);
        _mlcg2.discard(z);
    }

    /**
     * Writes the state of an @c additive_combine_engine to a @c
     * std::ostream.  The textual representation of an @c
     * additive_combine_engine is the textual representation of
     * the first base generator followed by the textual representation
     * of the second base generator.
     */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, additive_combine_engine, r)
    { os << r._mlcg1 << ' ' << r._mlcg2; return os; }

    /**
     * Reads the state of an @c additive_combine_engine from a
     * @c std::istream.
     */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, additive_combine_engine, r)
    { is >> r._mlcg1 >> std::ws >> r._mlcg2; return is; }

    /**
     * Returns: true iff the two @c additive_combine_engines will
     * produce the same sequence of values.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(additive_combine_engine, x, y)
    { return x._mlcg1 == y._mlcg1 && x._mlcg2 == y._mlcg2; }
    /**
     * Returns: true iff the two @c additive_combine_engines will
     * produce different sequences of values.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(additive_combine_engine)

private:
    MLCG1 _mlcg1;
    MLCG2 _mlcg2;
};

#ifndef BOOST_NO_INCLASS_MEMBER_INITIALIZATION
template<class MLCG1, class MLCG2>
const bool additive_combine_engine<MLCG1, MLCG2>::has_fixed_range;
#endif

/// \cond show_deprecated

/** Provided for backwards compatibility. */
template<class MLCG1, class MLCG2, typename MLCG1::result_type val = 0>
class additive_combine : public additive_combine_engine<MLCG1, MLCG2>
{
    typedef additive_combine_engine<MLCG1, MLCG2> base_t;
public:
    typedef typename base_t::result_type result_type;
    additive_combine() {}
    template<class T>
    additive_combine(T& arg) : base_t(arg) {}
    template<class T>
    additive_combine(const T& arg) : base_t(arg) {}
    template<class It>
    additive_combine(It& first, It last) : base_t(first, last) {}
};

/// \endcond

/**
 * The specialization \ecuyer1988 was suggested in
 *
 *  @blockquote
 *  "Efficient and Portable Combined Random Number Generators", Pierre L'Ecuyer,
 *  Communications of the ACM, Vol. 31, No. 6, June 1988, pp. 742-749, 774
 *  @endblockquote
 */
typedef additive_combine_engine<
    linear_congruential_engine<uint32_t, 40014, 0, 2147483563>,
    linear_congruential_engine<uint32_t, 40692, 0, 2147483399>
> ecuyer1988;

} // namespace random

using random::ecuyer1988;

} // namespace boost

#endif // BOOST_RANDOM_ADDITIVE_COMBINE_HPP
