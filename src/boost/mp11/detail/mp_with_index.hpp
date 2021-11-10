#ifndef BOOST_MP11_DETAIL_MP_WITH_INDEX_HPP_INCLUDED
#define BOOST_MP11_DETAIL_MP_WITH_INDEX_HPP_INCLUDED

//  Copyright 2017 Peter Dimov.
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/mp11/integral.hpp>
#include <boost/mp11/detail/config.hpp>
#include <type_traits>
#include <utility>
#include <cassert>

#if defined( BOOST_MP11_HAS_CXX14_CONSTEXPR )
# define BOOST_MP11_CONSTEXPR14 constexpr
#else
# define BOOST_MP11_CONSTEXPR14
#endif

#if defined( __GNUC__ ) || defined( __clang__ )
# define BOOST_MP11_UNREACHABLE_DEFAULT default: __builtin_unreachable();
#elif defined( _MSC_VER )
# define BOOST_MP11_UNREACHABLE_DEFAULT default: __assume(false);
#else
# define BOOST_MP11_UNREACHABLE_DEFAULT
#endif

namespace boost
{
namespace mp11
{

namespace detail
{

template<std::size_t N> struct mp_with_index_impl_
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        if( i < N / 2 )
        {
            return mp_with_index_impl_<N/2>::template call<K>( i, std::forward<F>(f) );
        }
        else
        {
            return mp_with_index_impl_<N-N/2>::template call<K+N/2>( i - N/2, std::forward<F>(f) );
        }
    }
};

template<> struct mp_with_index_impl_<0>
{
};

template<> struct mp_with_index_impl_<1>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t /*i*/, F && f )
    {
        return std::forward<F>(f)( mp_size_t<K+0>() );
    }
};

template<> struct mp_with_index_impl_<2>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        }
    }
};

template<> struct mp_with_index_impl_<3>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        }
    }
};

template<> struct mp_with_index_impl_<4>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        }
    }
};

template<> struct mp_with_index_impl_<5>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        }
    }
};

template<> struct mp_with_index_impl_<6>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        }
    }
};

template<> struct mp_with_index_impl_<7>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        }
    }
};

template<> struct mp_with_index_impl_<8>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        case 7: return std::forward<F>(f)( mp_size_t<K+7>() );
        }
    }
};

template<> struct mp_with_index_impl_<9>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        case 7: return std::forward<F>(f)( mp_size_t<K+7>() );
        case 8: return std::forward<F>(f)( mp_size_t<K+8>() );
        }
    }
};

template<> struct mp_with_index_impl_<10>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        case 7: return std::forward<F>(f)( mp_size_t<K+7>() );
        case 8: return std::forward<F>(f)( mp_size_t<K+8>() );
        case 9: return std::forward<F>(f)( mp_size_t<K+9>() );
        }
    }
};

template<> struct mp_with_index_impl_<11>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        case 7: return std::forward<F>(f)( mp_size_t<K+7>() );
        case 8: return std::forward<F>(f)( mp_size_t<K+8>() );
        case 9: return std::forward<F>(f)( mp_size_t<K+9>() );
        case 10: return std::forward<F>(f)( mp_size_t<K+10>() );
        }
    }
};

template<> struct mp_with_index_impl_<12>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        case 7: return std::forward<F>(f)( mp_size_t<K+7>() );
        case 8: return std::forward<F>(f)( mp_size_t<K+8>() );
        case 9: return std::forward<F>(f)( mp_size_t<K+9>() );
        case 10: return std::forward<F>(f)( mp_size_t<K+10>() );
        case 11: return std::forward<F>(f)( mp_size_t<K+11>() );
        }
    }
};

template<> struct mp_with_index_impl_<13>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        case 7: return std::forward<F>(f)( mp_size_t<K+7>() );
        case 8: return std::forward<F>(f)( mp_size_t<K+8>() );
        case 9: return std::forward<F>(f)( mp_size_t<K+9>() );
        case 10: return std::forward<F>(f)( mp_size_t<K+10>() );
        case 11: return std::forward<F>(f)( mp_size_t<K+11>() );
        case 12: return std::forward<F>(f)( mp_size_t<K+12>() );
        }
    }
};

template<> struct mp_with_index_impl_<14>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        case 7: return std::forward<F>(f)( mp_size_t<K+7>() );
        case 8: return std::forward<F>(f)( mp_size_t<K+8>() );
        case 9: return std::forward<F>(f)( mp_size_t<K+9>() );
        case 10: return std::forward<F>(f)( mp_size_t<K+10>() );
        case 11: return std::forward<F>(f)( mp_size_t<K+11>() );
        case 12: return std::forward<F>(f)( mp_size_t<K+12>() );
        case 13: return std::forward<F>(f)( mp_size_t<K+13>() );
        }
    }
};

template<> struct mp_with_index_impl_<15>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        case 7: return std::forward<F>(f)( mp_size_t<K+7>() );
        case 8: return std::forward<F>(f)( mp_size_t<K+8>() );
        case 9: return std::forward<F>(f)( mp_size_t<K+9>() );
        case 10: return std::forward<F>(f)( mp_size_t<K+10>() );
        case 11: return std::forward<F>(f)( mp_size_t<K+11>() );
        case 12: return std::forward<F>(f)( mp_size_t<K+12>() );
        case 13: return std::forward<F>(f)( mp_size_t<K+13>() );
        case 14: return std::forward<F>(f)( mp_size_t<K+14>() );
        }
    }
};

template<> struct mp_with_index_impl_<16>
{
    template<std::size_t K, class F> static BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) call( std::size_t i, F && f )
    {
        switch( i )
        {
        BOOST_MP11_UNREACHABLE_DEFAULT
        case 0: return std::forward<F>(f)( mp_size_t<K+0>() );
        case 1: return std::forward<F>(f)( mp_size_t<K+1>() );
        case 2: return std::forward<F>(f)( mp_size_t<K+2>() );
        case 3: return std::forward<F>(f)( mp_size_t<K+3>() );
        case 4: return std::forward<F>(f)( mp_size_t<K+4>() );
        case 5: return std::forward<F>(f)( mp_size_t<K+5>() );
        case 6: return std::forward<F>(f)( mp_size_t<K+6>() );
        case 7: return std::forward<F>(f)( mp_size_t<K+7>() );
        case 8: return std::forward<F>(f)( mp_size_t<K+8>() );
        case 9: return std::forward<F>(f)( mp_size_t<K+9>() );
        case 10: return std::forward<F>(f)( mp_size_t<K+10>() );
        case 11: return std::forward<F>(f)( mp_size_t<K+11>() );
        case 12: return std::forward<F>(f)( mp_size_t<K+12>() );
        case 13: return std::forward<F>(f)( mp_size_t<K+13>() );
        case 14: return std::forward<F>(f)( mp_size_t<K+14>() );
        case 15: return std::forward<F>(f)( mp_size_t<K+15>() );
        }
    }
};

} // namespace detail

template<std::size_t N, class F> inline BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) mp_with_index( std::size_t i, F && f )
{
    assert( i < N );
    return detail::mp_with_index_impl_<N>::template call<0>( i, std::forward<F>(f) );
}

template<class N, class F> inline BOOST_MP11_CONSTEXPR14 decltype(std::declval<F>()(std::declval<mp_size_t<0>>())) mp_with_index( std::size_t i, F && f )
{
    return mp_with_index<std::size_t{N::value}>( i, std::forward<F>(f) );
}

#undef BOOST_MP11_CONSTEXPR14
#undef BOOST_MP11_UNREACHABLE_DEFAULT

} // namespace mp11
} // namespace boost

#endif // #ifndef BOOST_MP11_DETAIL_MP_WITH_INDEX_HPP_INCLUDED
