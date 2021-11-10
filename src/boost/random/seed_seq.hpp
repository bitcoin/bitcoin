/* boost random/seed_seq.hpp header file
 *
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 */

#ifndef BOOST_RANDOM_SEED_SEQ_HPP
#define BOOST_RANDOM_SEED_SEQ_HPP

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <cstddef>
#include <vector>
#include <algorithm>
#include <iterator>

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#include <initializer_list>
#endif

namespace boost {
namespace random {

/**
 * The class @c seed_seq stores a sequence of 32-bit words
 * for seeding a \pseudo_random_number_generator.  These
 * words will be combined to fill the entire state of the
 * generator.
 */
class seed_seq {
public:
    typedef boost::uint_least32_t result_type;

    /** Initializes a seed_seq to hold an empty sequence. */
    seed_seq() {}
#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    /** Initializes the sequence from an initializer_list. */
    template<class T>
    seed_seq(const std::initializer_list<T>& il) : v(il.begin(), il.end()) {}
#endif
    /** Initializes the sequence from an iterator range. */
    template<class Iter>
    seed_seq(Iter first, Iter last) : v(first, last) {}
    /** Initializes the sequence from Boost.Range range. */
    template<class Range>
    explicit seed_seq(const Range& range)
      : v(boost::begin(range), boost::end(range)) {}

    /**
     * Fills a range with 32-bit values based on the stored sequence.
     *
     * Requires: Iter must be a Random Access Iterator whose value type
     * is an unsigned integral type at least 32 bits wide.
     */
    template<class Iter>
    void generate(Iter first, Iter last) const
    {
        typedef typename std::iterator_traits<Iter>::value_type value_type;
        std::fill(first, last, static_cast<value_type>(0x8b8b8b8bu));
        std::size_t s = v.size();
        std::size_t n = last - first;
        std::size_t t =
            (n >= 623) ? 11 :
            (n >=  68) ?  7 :
            (n >=  39) ?  5 :
            (n >=   7) ?  3 :
            (n - 1)/2;
        std::size_t p = (n - t) / 2;
        std::size_t q = p + t;
        std::size_t m = (std::max)(s+1, n);
        value_type mask = 0xffffffffu;
        for(std::size_t k = 0; k < m; ++k) {
            value_type r1 = static_cast<value_type>
                (*(first + k%n) ^ *(first + (k+p)%n) ^ *(first + (k+n-1)%n));
            r1 = r1 ^ (r1 >> 27);
            r1 = (r1 * 1664525u) & mask;
            value_type r2 = static_cast<value_type>(r1 +
                ((k == 0) ? s :
                 (k <= s) ? k % n + v[k - 1] :
                 (k % n)));
            *(first + (k+p)%n) = (*(first + (k+p)%n) + r1) & mask;
            *(first + (k+q)%n) = (*(first + (k+q)%n) + r2) & mask;
            *(first + k%n) = r2;
        }
        for(std::size_t k = m; k < m + n; ++k) {
            value_type r3 = static_cast<value_type>
                ((*(first + k%n) + *(first + (k+p)%n) + *(first + (k+n-1)%n))
                & mask);
            r3 = r3 ^ (r3 >> 27);
            r3 = (r3 * 1566083941u) & mask;
            value_type r4 = static_cast<value_type>(r3 - k%n);
            *(first + (k+p)%n) ^= r3;
            *(first + (k+q)%n) ^= r4;
            *(first + k%n) = r4;
        }
    }
    /** Returns the size of the sequence. */
    std::size_t size() const { return v.size(); }
    /** Writes the stored sequence to iter. */
    template<class Iter>
    void param(Iter out) { std::copy(v.begin(), v.end(), out); }
private:
    std::vector<result_type> v;
};

}
}

#endif
