/* boost random/detail/polynomial.hpp header file
 *
 * Copyright Steven Watanabe 2014
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_DETAIL_POLYNOMIAL_HPP
#define BOOST_RANDOM_DETAIL_POLYNOMIAL_HPP

#include <cstddef>
#include <limits>
#include <vector>
#include <algorithm>
#include <boost/assert.hpp>
#include <boost/cstdint.hpp>

namespace boost {
namespace random {
namespace detail {

class polynomial_ops {
public:
    typedef unsigned long digit_t;

    static void add(std::size_t size, const digit_t * lhs,
                       const digit_t * rhs, digit_t * output)
    {
        for(std::size_t i = 0; i < size; ++i) {
            output[i] = lhs[i] ^ rhs[i];
        }
    }

    static void add_shifted_inplace(std::size_t size, const digit_t * lhs,
                                    digit_t * output, std::size_t shift)
    {
        if(shift == 0) {
            add(size, lhs, output, output);
            return;
        }
        std::size_t bits = std::numeric_limits<digit_t>::digits;
        digit_t prev = 0;
        for(std::size_t i = 0; i < size; ++i) {
            digit_t tmp = lhs[i];
            output[i] ^= (tmp << shift) | (prev >> (bits-shift));
            prev = tmp;
        }
        output[size] ^= (prev >> (bits-shift));
    }

    static void multiply_simple(std::size_t size, const digit_t * lhs,
                                   const digit_t * rhs, digit_t * output)
    {
        std::size_t bits = std::numeric_limits<digit_t>::digits;
        for(std::size_t i = 0; i < 2*size; ++i) {
            output[i] = 0;
        }
        for(std::size_t i = 0; i < size; ++i) {
            for(std::size_t j = 0; j < bits; ++j) {
                if((lhs[i] & (digit_t(1) << j)) != 0) {
                    add_shifted_inplace(size, rhs, output + i, j);
                }
            }
        }
    }

    // memory requirements: (size - cutoff) * 4 + next_smaller
    static void multiply_karatsuba(std::size_t size,
                               const digit_t * lhs, const digit_t * rhs,
                               digit_t * output)
    {
        if(size < 64) {
            multiply_simple(size, lhs, rhs, output);
            return;
        }
        // split in half
        std::size_t cutoff = size/2;
        multiply_karatsuba(cutoff, lhs, rhs, output);
        multiply_karatsuba(size - cutoff, lhs + cutoff, rhs + cutoff,
                              output + cutoff*2);
        std::vector<digit_t> local1(size - cutoff);
        std::vector<digit_t> local2(size - cutoff);
        // combine the digits for the inner multiply
        add(cutoff, lhs, lhs + cutoff, &local1[0]);
        if(size & 1) local1[cutoff] = lhs[size - 1];
        add(cutoff, rhs + cutoff, rhs, &local2[0]);
        if(size & 1) local2[cutoff] = rhs[size - 1];
        std::vector<digit_t> local3((size - cutoff) * 2);
        multiply_karatsuba(size - cutoff, &local1[0], &local2[0], &local3[0]);
        add(cutoff * 2, output, &local3[0], &local3[0]);
        add((size - cutoff) * 2, output + cutoff*2, &local3[0], &local3[0]);
        // Finally, add the inner result
        add((size - cutoff) * 2, output + cutoff, &local3[0], output + cutoff);
    }
    
    static void multiply_add_karatsuba(std::size_t size,
                                       const digit_t * lhs, const digit_t * rhs,
                                       digit_t * output)
    {
        std::vector<digit_t> buf(size * 2);
        multiply_karatsuba(size, lhs, rhs, &buf[0]);
        add(size * 2, &buf[0], output, output);
    }

    static void multiply(const digit_t * lhs, std::size_t lhs_size,
                         const digit_t * rhs, std::size_t rhs_size,
                         digit_t * output)
    {
        std::fill_n(output, lhs_size + rhs_size, digit_t(0));
        multiply_add(lhs, lhs_size, rhs, rhs_size, output);
    }

    static void multiply_add(const digit_t * lhs, std::size_t lhs_size,
                             const digit_t * rhs, std::size_t rhs_size,
                             digit_t * output)
    {
        // split into pieces that can be passed to
        // karatsuba multiply.
        while(lhs_size != 0) {
            if(lhs_size < rhs_size) {
                std::swap(lhs, rhs);
                std::swap(lhs_size, rhs_size);
            }
            
            multiply_add_karatsuba(rhs_size, lhs, rhs, output);
            
            lhs += rhs_size;
            lhs_size -= rhs_size;
            output += rhs_size;
        }
    }

    static void copy_bits(const digit_t * x, std::size_t low, std::size_t high,
                   digit_t * out)
    {
        const std::size_t bits = std::numeric_limits<digit_t>::digits;
        std::size_t offset = low/bits;
        x += offset;
        low -= offset*bits;
        high -= offset*bits;
        std::size_t n = (high-low)/bits;
        if(low == 0) {
            for(std::size_t i = 0; i < n; ++i) {
                out[i] = x[i];
            }
        } else {
            for(std::size_t i = 0; i < n; ++i) {
                out[i] = (x[i] >> low) | (x[i+1] << (bits-low));
            }
        }
        if((high-low)%bits) {
            digit_t low_mask = (digit_t(1) << ((high-low)%bits)) - 1;
            digit_t result = (x[n] >> low);
            if(low != 0 && (n+1)*bits < high) {
                result |= (x[n+1] << (bits-low));
            }
            out[n] = (result & low_mask);
        }
    }

    static void shift_left(digit_t * val, std::size_t size, std::size_t shift)
    {
        const std::size_t bits = std::numeric_limits<digit_t>::digits;
        BOOST_ASSERT(shift > 0);
        BOOST_ASSERT(shift < bits);
        digit_t prev = 0;
        for(std::size_t i = 0; i < size; ++i) {
            digit_t tmp = val[i];
            val[i] = (prev >> (bits - shift)) | (val[i] << shift);
            prev = tmp;
        }
    }

    static digit_t sqr(digit_t val) {
        const std::size_t bits = std::numeric_limits<digit_t>::digits;
        digit_t mask = (digit_t(1) << bits/2) - 1;
        for(std::size_t i = bits; i > 1; i /= 2) {
            val = ((val & ~mask) << i/2) | (val & mask);
            mask = mask & (mask >> i/4);
            mask = mask | (mask << i/2);
        }
        return val;
    }

    static void sqr(digit_t * val, std::size_t size)
    {
        const std::size_t bits = std::numeric_limits<digit_t>::digits;
        digit_t mask = (digit_t(1) << bits/2) - 1;
        for(std::size_t i = 0; i < size; ++i) {
            digit_t x = val[size - i - 1];
            val[(size - i - 1) * 2] = sqr(x & mask);
            val[(size - i - 1) * 2 + 1] = sqr(x >> bits/2);
        }
    }

    // optimized for the case when the modulus has few bits set.
    struct sparse_mod {
        sparse_mod(const digit_t * divisor, std::size_t divisor_bits)
        {
            const std::size_t bits = std::numeric_limits<digit_t>::digits;
            _remainder_bits = divisor_bits - 1;
            for(std::size_t i = 0; i < divisor_bits; ++i) {
                if(divisor[i/bits] & (digit_t(1) << i%bits)) {
                    _bit_indices.push_back(i);
                }
            }
            BOOST_ASSERT(_bit_indices.back() == divisor_bits - 1);
            _bit_indices.pop_back();
            if(_bit_indices.empty()) {
                _block_bits = divisor_bits;
                _lower_bits = 0;
            } else {
                _block_bits = divisor_bits - _bit_indices.back() - 1;
                _lower_bits = _bit_indices.back() + 1;
            }
            
            _partial_quotient.resize((_block_bits + bits - 1)/bits);
        }
        void operator()(digit_t * dividend, std::size_t dividend_bits)
        {
            const std::size_t bits = std::numeric_limits<digit_t>::digits;
            while(dividend_bits > _remainder_bits) {
                std::size_t block_start = (std::max)(dividend_bits - _block_bits, _remainder_bits);
                std::size_t block_size = (dividend_bits - block_start + bits - 1) / bits;
                copy_bits(dividend, block_start, dividend_bits, &_partial_quotient[0]);
                for(std::size_t i = 0; i < _bit_indices.size(); ++i) {
                    std::size_t pos = _bit_indices[i] + block_start - _remainder_bits;
                    add_shifted_inplace(block_size, &_partial_quotient[0], dividend + pos/bits, pos%bits);
                }
                add_shifted_inplace(block_size, &_partial_quotient[0], dividend + block_start/bits, block_start%bits);
                dividend_bits = block_start;
            }
        }
        std::vector<digit_t> _partial_quotient;
        std::size_t _remainder_bits;
        std::size_t _block_bits;
        std::size_t _lower_bits;
        std::vector<std::size_t> _bit_indices;
    };

    // base should have the same number of bits as mod
    // base, and mod should both be able to hold a power
    // of 2 >= mod_bits.  out needs to be twice as large.
    static void mod_pow_x(boost::uintmax_t exponent, const digit_t * mod, std::size_t mod_bits, digit_t * out)
    {
        const std::size_t bits = std::numeric_limits<digit_t>::digits;
        const std::size_t n = (mod_bits + bits - 1) / bits;
        const std::size_t highbit = mod_bits - 1;
        if(exponent == 0) {
            out[0] = 1;
            std::fill_n(out + 1, n - 1, digit_t(0));
            return;
        }
        boost::uintmax_t i = std::numeric_limits<boost::uintmax_t>::digits - 1;
        while(((boost::uintmax_t(1) << i) & exponent) == 0) {
            --i;
        }
        out[0] = 2;
        std::fill_n(out + 1, n - 1, digit_t(0));
        sparse_mod m(mod, mod_bits);
        while(i--) {
            sqr(out, n);
            m(out, 2 * mod_bits - 1);
            if((boost::uintmax_t(1) << i) & exponent) {
                shift_left(out, n, 1);
                if(out[highbit / bits] & (digit_t(1) << highbit%bits))
                    add(n, out, mod, out);
            }
        }
    }
};

class polynomial
{
    typedef polynomial_ops::digit_t digit_t;
public:
    polynomial() : _size(0) {}
    class reference {
    public:
        reference(digit_t &value, int idx)
            : _value(value), _idx(idx) {}
        operator bool() const { return (_value & (digit_t(1) << _idx)) != 0; }
        reference& operator=(bool b)
        {
            if(b) {
                _value |= (digit_t(1) << _idx);
            } else {
                _value &= ~(digit_t(1) << _idx);
            }
            return *this;
        }
        reference &operator^=(bool b)
        {
            _value ^= (digit_t(b) << _idx);
            return *this;
        }

        reference &operator=(const reference &other)
        {
            return *this = static_cast<bool>(other);
        }
    private:
        digit_t &_value;
        int _idx;
    };
    reference operator[](std::size_t i)
    {
        static const std::size_t bits = std::numeric_limits<digit_t>::digits;
        ensure_bit(i);
        return reference(_storage[i/bits], i%bits);
    }
    bool operator[](std::size_t i) const
    {
        static const std::size_t bits = std::numeric_limits<digit_t>::digits;
        if(i < size())
            return (_storage[i/bits] & (digit_t(1) << (i%bits))) != 0;
        else
            return false;
    }
    std::size_t size() const
    {
        return _size;
    }
    void resize(std::size_t n)
    {
        static const std::size_t bits = std::numeric_limits<digit_t>::digits;
        _storage.resize((n + bits - 1)/bits);
        // clear the high order bits in case we're shrinking.
        if(n%bits) {
            _storage.back() &= ((digit_t(1) << (n%bits)) - 1);
        }
        _size = n;
    }
    friend polynomial operator*(const polynomial &lhs, const polynomial &rhs);
    friend polynomial mod_pow_x(boost::uintmax_t exponent, polynomial mod);
private:
    std::vector<polynomial_ops::digit_t> _storage;
    std::size_t _size;
    void ensure_bit(std::size_t i)
    {
        if(i >= size()) {
            resize(i + 1);
        }
    }
    void normalize()
    {
        while(size() && (*this)[size() - 1] == 0)
            resize(size() - 1);
    }
};

inline polynomial operator*(const polynomial &lhs, const polynomial &rhs)
{
    polynomial result;
    result._storage.resize(lhs._storage.size() + rhs._storage.size());
    polynomial_ops::multiply(&lhs._storage[0], lhs._storage.size(),
                             &rhs._storage[0], rhs._storage.size(),
                             &result._storage[0]);
    result._size = lhs._size + rhs._size;
    return result;
}

inline polynomial mod_pow_x(boost::uintmax_t exponent, polynomial mod)
{
    polynomial result;
    mod.normalize();
    std::size_t mod_size = mod.size();
    result._storage.resize(mod._storage.size() * 2);
    result._size = mod.size() * 2;
    polynomial_ops::mod_pow_x(exponent, &mod._storage[0], mod_size, &result._storage[0]);
    result.resize(mod.size() - 1);
    return result;
}

}
}
}

#endif // BOOST_RANDOM_DETAIL_POLYNOMIAL_HPP
