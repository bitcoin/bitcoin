/* boost random/detail/seed.hpp header file
 *
 * Copyright Steven Watanabe 2009
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_DETAIL_SEED_IMPL_HPP
#define BOOST_RANDOM_DETAIL_SEED_IMPL_HPP

#include <stdexcept>
#include <boost/cstdint.hpp>
#include <boost/throw_exception.hpp>
#include <boost/config/no_tr1/cmath.hpp>
#include <boost/integer/integer_mask.hpp>
#include <boost/integer/static_log2.hpp>
#include <boost/random/traits.hpp>
#include <boost/random/detail/const_mod.hpp>
#include <boost/random/detail/integer_log2.hpp>
#include <boost/random/detail/signed_unsigned_tools.hpp>
#include <boost/random/detail/generator_bits.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/integral_constant.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {
namespace random {
namespace detail {

// finds the seed type of an engine, given its
// result_type.  If the result_type is integral
// the seed type is the same.  If the result_type
// is floating point, the seed type is uint32_t
template<class T>
struct seed_type
{
    typedef typename boost::conditional<boost::is_integral<T>::value,
        T,
        boost::uint32_t
    >::type type;
};

template<int N>
struct const_pow_impl
{
    template<class T>
    static T call(T arg, int n, T result)
    {
        return const_pow_impl<N / 2>::call(T(arg * arg), n / 2,
            n%2 == 0? result : T(result * arg));
    }
};

template<>
struct const_pow_impl<0>
{
    template<class T>
    static T call(T, int, T result)
    {
        return result;
    }
};

// requires N is an upper bound on n
template<int N, class T>
inline T const_pow(T arg, int n) { return const_pow_impl<N>::call(arg, n, T(1)); }

template<class T>
inline T pow2(int n)
{
    typedef unsigned int_type;
    const int max_bits = std::numeric_limits<int_type>::digits;
    T multiplier = T(int_type(1) << (max_bits - 1)) * 2;
    return (int_type(1) << (n % max_bits)) *
        const_pow<std::numeric_limits<T>::digits / max_bits>(multiplier, n / max_bits);
}

template<class Engine, class Iter>
void generate_from_real(Engine& eng, Iter begin, Iter end)
{
    using std::fmod;
    typedef typename Engine::result_type RealType;
    const int Bits = detail::generator_bits<Engine>::value();
    int remaining_bits = 0;
    boost::uint_least32_t saved_bits = 0;
    RealType multiplier = pow2<RealType>( Bits);
    RealType mult32 = RealType(4294967296.0); // 2^32
    while(true) {
        RealType val = eng() * multiplier;
        int available_bits = Bits;
        // Make sure the compiler can optimize this out
        // if it isn't possible.
        if(Bits < 32 && available_bits < 32 - remaining_bits) {
            saved_bits |= boost::uint_least32_t(val) << remaining_bits;
            remaining_bits += Bits;
        } else {
            // If Bits < 32, then remaining_bits != 0, since
            // if remaining_bits == 0, available_bits < 32 - 0,
            // and we won't get here to begin with.
            if(Bits < 32 || remaining_bits != 0) {
                boost::uint_least32_t divisor =
                    (boost::uint_least32_t(1) << (32 - remaining_bits));
                boost::uint_least32_t extra_bits = boost::uint_least32_t(fmod(val, mult32)) & (divisor - 1);
                val = val / divisor;
                *begin++ = saved_bits | (extra_bits << remaining_bits);
                if(begin == end) return;
                available_bits -= 32 - remaining_bits;
                remaining_bits = 0;
            }
            // If Bits < 32 we should never enter this loop
            if(Bits >= 32) {
                for(; available_bits >= 32; available_bits -= 32) {
                    boost::uint_least32_t word = boost::uint_least32_t(fmod(val, mult32));
                    val /= mult32;
                    *begin++ = word;
                    if(begin == end) return;
                }
            }
            remaining_bits = available_bits;
            saved_bits = static_cast<boost::uint_least32_t>(val);
        }
    }
}

template<class Engine, class Iter>
void generate_from_int(Engine& eng, Iter begin, Iter end)
{
    typedef typename Engine::result_type IntType;
    typedef typename boost::random::traits::make_unsigned<IntType>::type unsigned_type;
    int remaining_bits = 0;
    boost::uint_least32_t saved_bits = 0;
    unsigned_type range = boost::random::detail::subtract<IntType>()((eng.max)(), (eng.min)());

    int bits =
        (range == (std::numeric_limits<unsigned_type>::max)()) ?
            std::numeric_limits<unsigned_type>::digits :
            detail::integer_log2(range + 1);

    {
        int discarded_bits = detail::integer_log2(bits);
        unsigned_type excess = (range + 1) >> (bits - discarded_bits);
        if(excess != 0) {
            int extra_bits = detail::integer_log2((excess - 1) ^ excess);
            bits = bits - discarded_bits + extra_bits;
        }
    }

    unsigned_type mask = (static_cast<unsigned_type>(2) << (bits - 1)) - 1;
    unsigned_type limit = ((range + 1) & ~mask) - 1;

    while(true) {
        unsigned_type val;
        do {
            val = boost::random::detail::subtract<IntType>()(eng(), (eng.min)());
        } while(limit != range && val > limit);
        val &= mask;
        int available_bits = bits;
        if(available_bits == 32) {
            *begin++ = static_cast<boost::uint_least32_t>(val) & 0xFFFFFFFFu;
            if(begin == end) return;
        } else if(available_bits % 32 == 0) {
            for(int i = 0; i < available_bits / 32; ++i) {
                boost::uint_least32_t word = boost::uint_least32_t(val) & 0xFFFFFFFFu;
                int suppress_warning = (bits >= 32);
                BOOST_ASSERT(suppress_warning == 1);
                val >>= (32 * suppress_warning);
                *begin++ = word;
                if(begin == end) return;
            }
        } else if(bits < 32 && available_bits < 32 - remaining_bits) {
            saved_bits |= boost::uint_least32_t(val) << remaining_bits;
            remaining_bits += bits;
        } else {
            if(bits < 32 || remaining_bits != 0) {
                boost::uint_least32_t extra_bits = boost::uint_least32_t(val) & ((boost::uint_least32_t(1) << (32 - remaining_bits)) - 1);
                val >>= 32 - remaining_bits;
                *begin++ = saved_bits | (extra_bits << remaining_bits);
                if(begin == end) return;
                available_bits -= 32 - remaining_bits;
                remaining_bits = 0;
            }
            if(bits >= 32) {
                for(; available_bits >= 32; available_bits -= 32) {
                    boost::uint_least32_t word = boost::uint_least32_t(val) & 0xFFFFFFFFu;
                    int suppress_warning = (bits >= 32);
                    BOOST_ASSERT(suppress_warning == 1);
                    val >>= (32 * suppress_warning);
                    *begin++ = word;
                    if(begin == end) return;
                }
            }
            remaining_bits = available_bits;
            saved_bits = static_cast<boost::uint_least32_t>(val);
        }
    }
}

template<class Engine, class Iter>
void generate_impl(Engine& eng, Iter first, Iter last, boost::true_type)
{
    return detail::generate_from_int(eng, first, last);
}

template<class Engine, class Iter>
void generate_impl(Engine& eng, Iter first, Iter last, boost::false_type)
{
    return detail::generate_from_real(eng, first, last);
}

template<class Engine, class Iter>
void generate(Engine& eng, Iter first, Iter last)
{
    return detail::generate_impl(eng, first, last, boost::random::traits::is_integral<typename Engine::result_type>());
}



template<class IntType, IntType m, class SeedSeq>
IntType seed_one_int(SeedSeq& seq)
{
    static const int log = ::boost::conditional<(m == 0),
        ::boost::integral_constant<int, (::std::numeric_limits<IntType>::digits)>,
        ::boost::static_log2<m> >::type::value;
    static const int k =
        (log + ((~(static_cast<IntType>(2) << (log - 1)) & m)? 32 : 31)) / 32;
    ::boost::uint_least32_t array[log / 32 + 4];
    seq.generate(&array[0], &array[0] + k + 3);
    IntType s = 0;
    for(int j = 0; j < k; ++j) {
        IntType digit = const_mod<IntType, m>::apply(IntType(array[j+3]));
        IntType mult = IntType(1) << 32*j;
        s = const_mod<IntType, m>::mult_add(mult, digit, s);
    }
    return s;
}

template<class IntType, IntType m, class Iter>
IntType get_one_int(Iter& first, Iter last)
{
    static const int log = ::boost::conditional<(m == 0),
        ::boost::integral_constant<int, (::std::numeric_limits<IntType>::digits)>,
        ::boost::static_log2<m> >::type::value;
    static const int k =
        (log + ((~(static_cast<IntType>(2) << (log - 1)) & m)? 32 : 31)) / 32;
    IntType s = 0;
    for(int j = 0; j < k; ++j) {
        if(first == last) {
            boost::throw_exception(::std::invalid_argument("Not enough elements in call to seed."));
        }
        IntType digit = const_mod<IntType, m>::apply(IntType(*first++));
        IntType mult = IntType(1) << 32*j;
        s = const_mod<IntType, m>::mult_add(mult, digit, s);
    }
    return s;
}

// TODO: work in-place whenever possible
template<int w, std::size_t n, class SeedSeq, class UIntType>
void seed_array_int_impl(SeedSeq& seq, UIntType (&x)[n])
{
    boost::uint_least32_t storage[((w+31)/32) * n];
    seq.generate(&storage[0], &storage[0] + ((w+31)/32) * n);
    for(std::size_t j = 0; j < n; j++) {
        UIntType val = 0;
        for(std::size_t k = 0; k < (w+31)/32; ++k) {
            val += static_cast<UIntType>(storage[(w+31)/32*j + k]) << 32*k;
        }
        x[j] = val & ::boost::low_bits_mask_t<w>::sig_bits;
    }
}

template<int w, std::size_t n, class SeedSeq, class IntType>
inline void seed_array_int_impl(SeedSeq& seq, IntType (&x)[n], boost::true_type)
{
    BOOST_STATIC_ASSERT_MSG(boost::is_integral<IntType>::value, "Sorry but this routine has not been ported to non built-in integers as it relies on a reinterpret_cast.");
    typedef typename boost::make_unsigned<IntType>::type unsigned_array[n];
    seed_array_int_impl<w>(seq, reinterpret_cast<unsigned_array&>(x));
}

template<int w, std::size_t n, class SeedSeq, class IntType>
inline void seed_array_int_impl(SeedSeq& seq, IntType (&x)[n], boost::false_type)
{
    seed_array_int_impl<w>(seq, x);
}

template<int w, std::size_t n, class SeedSeq, class IntType>
inline void seed_array_int(SeedSeq& seq, IntType (&x)[n])
{
    seed_array_int_impl<w>(seq, x, boost::random::traits::is_signed<IntType>());
}

template<int w, std::size_t n, class Iter, class UIntType>
void fill_array_int_impl(Iter& first, Iter last, UIntType (&x)[n])
{
    for(std::size_t j = 0; j < n; j++) {
        UIntType val = 0;
        for(std::size_t k = 0; k < (w+31)/32; ++k) {
            if(first == last) {
                boost::throw_exception(std::invalid_argument("Not enough elements in call to seed."));
            }
            val += static_cast<UIntType>(*first++) << 32*k;
        }
        x[j] = val & ::boost::low_bits_mask_t<w>::sig_bits;
    }
}

template<int w, std::size_t n, class Iter, class IntType>
inline void fill_array_int_impl(Iter& first, Iter last, IntType (&x)[n], boost::true_type)
{
    BOOST_STATIC_ASSERT_MSG(boost::is_integral<IntType>::value, "Sorry but this routine has not been ported to non built-in integers as it relies on a reinterpret_cast.");
    typedef typename boost::make_unsigned<IntType>::type unsigned_array[n];
    fill_array_int_impl<w>(first, last, reinterpret_cast<unsigned_array&>(x));
}

template<int w, std::size_t n, class Iter, class IntType>
inline void fill_array_int_impl(Iter& first, Iter last, IntType (&x)[n], boost::false_type)
{
    fill_array_int_impl<w>(first, last, x);
}

template<int w, std::size_t n, class Iter, class IntType>
inline void fill_array_int(Iter& first, Iter last, IntType (&x)[n])
{
    fill_array_int_impl<w>(first, last, x, boost::random::traits::is_signed<IntType>());
}

template<int w, std::size_t n, class RealType>
void seed_array_real_impl(const boost::uint_least32_t* storage, RealType (&x)[n])
{
    boost::uint_least32_t mask = ~((~boost::uint_least32_t(0)) << (w%32));
    RealType two32 = 4294967296.0;
    const RealType divisor = RealType(1)/detail::pow2<RealType>(w);
    unsigned int j;
    for(j = 0; j < n; ++j) {
        RealType val = RealType(0);
        RealType mult = divisor;
        for(int k = 0; k < w/32; ++k) {
            val += *storage++ * mult;
            mult *= two32;
        }
        if(mask != 0) {
            val += (*storage++ & mask) * mult;
        }
        BOOST_ASSERT(val >= 0);
        BOOST_ASSERT(val < 1);
        x[j] = val;
    }
}

template<int w, std::size_t n, class SeedSeq, class RealType>
void seed_array_real(SeedSeq& seq, RealType (&x)[n])
{
    using std::pow;
    boost::uint_least32_t storage[((w+31)/32) * n];
    seq.generate(&storage[0], &storage[0] + ((w+31)/32) * n);
    seed_array_real_impl<w>(storage, x);
}

template<int w, std::size_t n, class Iter, class RealType>
void fill_array_real(Iter& first, Iter last, RealType (&x)[n])
{
    boost::uint_least32_t mask = ~((~boost::uint_least32_t(0)) << (w%32));
    RealType two32 = 4294967296.0;
    const RealType divisor = RealType(1)/detail::pow2<RealType>(w);
    unsigned int j;
    for(j = 0; j < n; ++j) {
        RealType val = RealType(0);
        RealType mult = divisor;
        for(int k = 0; k < w/32; ++k, ++first) {
            if(first == last) boost::throw_exception(std::invalid_argument("Not enough elements in call to seed."));
            val += *first * mult;
            mult *= two32;
        }
        if(mask != 0) {
            if(first == last) boost::throw_exception(std::invalid_argument("Not enough elements in call to seed."));
            val += (*first & mask) * mult;
            ++first;
        }
        BOOST_ASSERT(val >= 0);
        BOOST_ASSERT(val < 1);
        x[j] = val;
    }
}

}
}
}

#include <boost/random/detail/enable_warnings.hpp>

#endif
