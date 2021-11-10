#ifndef BOOST_VARIANT2_VARIANT_HPP_INCLUDED
#define BOOST_VARIANT2_VARIANT_HPP_INCLUDED

// Copyright 2017-2019 Peter Dimov.
//
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#if defined(_MSC_VER) && _MSC_VER < 1910
# pragma warning( push )
# pragma warning( disable: 4521 4522 ) // multiple copy operators
#endif

#ifndef BOOST_MP11_HPP_INCLUDED
#include <boost/mp11.hpp>
#endif
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/cstdint.hpp>
#include <cstddef>
#include <type_traits>
#include <exception>
#include <cassert>
#include <initializer_list>
#include <utility>
#include <functional> // std::hash
#include <cstdint>

//

namespace boost
{

#ifdef BOOST_NO_EXCEPTIONS

BOOST_NORETURN void throw_exception( std::exception const & e ); // user defined

#endif

template<class T> struct hash;

namespace variant2
{

// bad_variant_access

class bad_variant_access: public std::exception
{
public:

    bad_variant_access() noexcept
    {
    }

    char const * what() const noexcept
    {
        return "bad_variant_access";
    }
};

namespace detail
{

BOOST_NORETURN inline void throw_bad_variant_access()
{
#ifdef BOOST_NO_EXCEPTIONS

    boost::throw_exception( bad_variant_access() );

#else

    throw bad_variant_access();

#endif
}

} // namespace detail

// monostate

struct monostate
{
};

#if !BOOST_WORKAROUND(BOOST_MSVC, < 1940)

constexpr bool operator<(monostate, monostate) noexcept { return false; }
constexpr bool operator>(monostate, monostate) noexcept { return false; }
constexpr bool operator<=(monostate, monostate) noexcept { return true; }
constexpr bool operator>=(monostate, monostate) noexcept { return true; }
constexpr bool operator==(monostate, monostate) noexcept { return true; }
constexpr bool operator!=(monostate, monostate) noexcept { return false; }

#else

constexpr bool operator<(monostate const&, monostate const&) noexcept { return false; }
constexpr bool operator>(monostate const&, monostate const&) noexcept { return false; }
constexpr bool operator<=(monostate const&, monostate const&) noexcept { return true; }
constexpr bool operator>=(monostate const&, monostate const&) noexcept { return true; }
constexpr bool operator==(monostate const&, monostate const&) noexcept { return true; }
constexpr bool operator!=(monostate const&, monostate const&) noexcept { return false; }

#endif

// variant forward declaration

template<class... T> class variant;

// variant_size

template<class T> struct variant_size
{
};

template<class T> struct variant_size<T const>: variant_size<T>
{
};

template<class T> struct variant_size<T volatile>: variant_size<T>
{
};

template<class T> struct variant_size<T const volatile>: variant_size<T>
{
};

template<class T> struct variant_size<T&>: variant_size<T>
{
};

template<class T> struct variant_size<T&&>: variant_size<T>
{
};

#if !defined(BOOST_NO_CXX14_VARIABLE_TEMPLATES)

template <class T> /*inline*/ constexpr std::size_t variant_size_v = variant_size<T>::value;

#endif

template <class... T> struct variant_size<variant<T...>>: mp11::mp_size<variant<T...>>
{
};

// variant_alternative

template<std::size_t I, class T> struct variant_alternative;

template<std::size_t I, class T> using variant_alternative_t = typename variant_alternative<I, T>::type;

#if BOOST_WORKAROUND(BOOST_GCC, < 40900)

namespace detail
{

template<std::size_t I, class T, bool E> struct variant_alternative_impl
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...>, true>
{
    using type = mp11::mp_at_c<variant<T...>, I>;
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...> const, true>: std::add_const< mp11::mp_at_c<variant<T...>, I> >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...> volatile, true>: std::add_volatile< mp11::mp_at_c<variant<T...>, I> >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...> const volatile, true>: std::add_cv< mp11::mp_at_c<variant<T...>, I> >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...>&, true>: std::add_lvalue_reference< mp11::mp_at_c<variant<T...>, I> >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...> const&, true>: std::add_lvalue_reference< mp11::mp_at_c<variant<T...>, I> const >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...> volatile&, true>: std::add_lvalue_reference< mp11::mp_at_c<variant<T...>, I> volatile >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...> const volatile&, true>: std::add_lvalue_reference< mp11::mp_at_c<variant<T...>, I> const volatile >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...>&&, true>: std::add_rvalue_reference< mp11::mp_at_c<variant<T...>, I> >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...> const&&, true>: std::add_rvalue_reference< mp11::mp_at_c<variant<T...>, I> const >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...> volatile&&, true>: std::add_rvalue_reference< mp11::mp_at_c<variant<T...>, I> volatile >
{
};

template<std::size_t I, class... T> struct variant_alternative_impl<I, variant<T...> const volatile&&, true>: std::add_rvalue_reference< mp11::mp_at_c<variant<T...>, I> const volatile >
{
};

} // namespace detail

template<std::size_t I, class T> struct variant_alternative
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...>>: public detail::variant_alternative_impl<I, variant<T...>, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...> const>: public detail::variant_alternative_impl<I, variant<T...> const, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...> volatile>: public detail::variant_alternative_impl<I, variant<T...> volatile, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...> const volatile>: public detail::variant_alternative_impl<I, variant<T...> const volatile, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...>&>: public detail::variant_alternative_impl<I, variant<T...>&, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...> const&>: public detail::variant_alternative_impl<I, variant<T...> const&, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...> volatile&>: public detail::variant_alternative_impl<I, variant<T...> volatile&, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...> const volatile&>: public detail::variant_alternative_impl<I, variant<T...> const volatile&, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...>&&>: public detail::variant_alternative_impl<I, variant<T...>&&, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...> const&&>: public detail::variant_alternative_impl<I, variant<T...> const&&, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...> volatile&&>: public detail::variant_alternative_impl<I, variant<T...> volatile&&, (I < sizeof...(T))>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...> const volatile&&>: public detail::variant_alternative_impl<I, variant<T...> const volatile&&, (I < sizeof...(T))>
{
};

#else

namespace detail
{

#if defined( BOOST_MP11_VERSION ) && BOOST_MP11_VERSION >= 107000

template<class I, class T, class Q> using var_alt_impl = mp11::mp_invoke_q<Q, variant_alternative_t<I::value, T>>;

#else

template<class I, class T, class Q> using var_alt_impl = mp11::mp_invoke<Q, variant_alternative_t<I::value, T>>;

#endif

} // namespace detail

template<std::size_t I, class T> struct variant_alternative
{
};

template<std::size_t I, class T> struct variant_alternative<I, T const>: mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_const>>
{
};

template<std::size_t I, class T> struct variant_alternative<I, T volatile>: mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_volatile>>
{
};

template<std::size_t I, class T> struct variant_alternative<I, T const volatile>: mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_cv>>
{
};

template<std::size_t I, class T> struct variant_alternative<I, T&>: mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_lvalue_reference>>
{
};

template<std::size_t I, class T> struct variant_alternative<I, T&&>: mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_rvalue_reference>>
{
};

template<std::size_t I, class... T> struct variant_alternative<I, variant<T...>>: mp11::mp_defer<mp11::mp_at, variant<T...>, mp11::mp_size_t<I>>
{
};

#endif

// variant_npos

constexpr std::size_t variant_npos = ~static_cast<std::size_t>( 0 );

// holds_alternative

template<class U, class... T> constexpr bool holds_alternative( variant<T...> const& v ) noexcept
{
    static_assert( mp11::mp_count<variant<T...>, U>::value == 1, "The type must occur exactly once in the list of variant alternatives" );
    return v.index() == mp11::mp_find<variant<T...>, U>::value;
}

// get (index)

template<std::size_t I, class... T> constexpr variant_alternative_t<I, variant<T...>>& get(variant<T...>& v)
{
    static_assert( I < sizeof...(T), "Index out of bounds" );
    return ( v.index() != I? detail::throw_bad_variant_access(): (void)0 ), v._get_impl( mp11::mp_size_t<I>() );
}

template<std::size_t I, class... T> constexpr variant_alternative_t<I, variant<T...>>&& get(variant<T...>&& v)
{
    static_assert( I < sizeof...(T), "Index out of bounds" );

#if !BOOST_WORKAROUND(BOOST_MSVC, < 1930)

    return ( v.index() != I? detail::throw_bad_variant_access(): (void)0 ), std::move( v._get_impl( mp11::mp_size_t<I>() ) );

#else

    if( v.index() != I ) detail::throw_bad_variant_access();
    return std::move( v._get_impl( mp11::mp_size_t<I>() ) );

#endif
}

template<std::size_t I, class... T> constexpr variant_alternative_t<I, variant<T...>> const& get(variant<T...> const& v)
{
    static_assert( I < sizeof...(T), "Index out of bounds" );
    return ( v.index() != I? detail::throw_bad_variant_access(): (void)0 ), v._get_impl( mp11::mp_size_t<I>() );
}

template<std::size_t I, class... T> constexpr variant_alternative_t<I, variant<T...>> const&& get(variant<T...> const&& v)
{
    static_assert( I < sizeof...(T), "Index out of bounds" );

#if !BOOST_WORKAROUND(BOOST_MSVC, < 1930)

    return ( v.index() != I? detail::throw_bad_variant_access(): (void)0 ), std::move( v._get_impl( mp11::mp_size_t<I>() ) );

#else

    if( v.index() != I ) detail::throw_bad_variant_access();
    return std::move( v._get_impl( mp11::mp_size_t<I>() ) );

#endif
}

// detail::unsafe_get (for visit)

namespace detail
{

template<std::size_t I, class... T> constexpr variant_alternative_t<I, variant<T...>>& unsafe_get(variant<T...>& v)
{
    static_assert( I < sizeof...(T), "Index out of bounds" );
    return v._get_impl( mp11::mp_size_t<I>() );
}

template<std::size_t I, class... T> constexpr variant_alternative_t<I, variant<T...>>&& unsafe_get(variant<T...>&& v)
{
    static_assert( I < sizeof...(T), "Index out of bounds" );
    return std::move( v._get_impl( mp11::mp_size_t<I>() ) );
}

template<std::size_t I, class... T> constexpr variant_alternative_t<I, variant<T...>> const& unsafe_get(variant<T...> const& v)
{
    static_assert( I < sizeof...(T), "Index out of bounds" );
    return v._get_impl( mp11::mp_size_t<I>() );
}

template<std::size_t I, class... T> constexpr variant_alternative_t<I, variant<T...>> const&& unsafe_get(variant<T...> const&& v)
{
    static_assert( I < sizeof...(T), "Index out of bounds" );
    return std::move( v._get_impl( mp11::mp_size_t<I>() ) );
}

} // namespace detail

// get (type)

template<class U, class... T> constexpr U& get(variant<T...>& v)
{
    static_assert( mp11::mp_count<variant<T...>, U>::value == 1, "The type must occur exactly once in the list of variant alternatives" );

    using I = mp11::mp_find<variant<T...>, U>;

    return ( v.index() != I::value? detail::throw_bad_variant_access(): (void)0 ), v._get_impl( I() );
}

template<class U, class... T> constexpr U&& get(variant<T...>&& v)
{
    static_assert( mp11::mp_count<variant<T...>, U>::value == 1, "The type must occur exactly once in the list of variant alternatives" );

    using I = mp11::mp_find<variant<T...>, U>;

#if !BOOST_WORKAROUND(BOOST_MSVC, < 1930)

    return ( v.index() != I::value? detail::throw_bad_variant_access(): (void)0 ), std::move( v._get_impl( I() ) );

#else

    if( v.index() != I::value ) detail::throw_bad_variant_access();
    return std::move( v._get_impl( I() ) );

#endif
}

template<class U, class... T> constexpr U const& get(variant<T...> const& v)
{
    static_assert( mp11::mp_count<variant<T...>, U>::value == 1, "The type must occur exactly once in the list of variant alternatives" );

    using I = mp11::mp_find<variant<T...>, U>;

    return ( v.index() != I::value? detail::throw_bad_variant_access(): (void)0 ), v._get_impl( I() );
}

template<class U, class... T> constexpr U const&& get(variant<T...> const&& v)
{
    static_assert( mp11::mp_count<variant<T...>, U>::value == 1, "The type must occur exactly once in the list of variant alternatives" );

    using I = mp11::mp_find<variant<T...>, U>;

#if !BOOST_WORKAROUND(BOOST_MSVC, < 1930)

    return ( v.index() != I::value? detail::throw_bad_variant_access(): (void)0 ), std::move( v._get_impl( I() ) );

#else

    if( v.index() != I::value ) detail::throw_bad_variant_access();
    return std::move( v._get_impl( I() ) );

#endif
}

// get_if

template<std::size_t I, class... T> constexpr typename std::add_pointer<variant_alternative_t<I, variant<T...>>>::type get_if(variant<T...>* v) noexcept
{
    static_assert( I < sizeof...(T), "Index out of bounds" );
    return v && v->index() == I? &v->_get_impl( mp11::mp_size_t<I>() ): 0;
}

template<std::size_t I, class... T> constexpr typename std::add_pointer<const variant_alternative_t<I, variant<T...>>>::type get_if(variant<T...> const * v) noexcept
{
    static_assert( I < sizeof...(T), "Index out of bounds" );
    return v && v->index() == I? &v->_get_impl( mp11::mp_size_t<I>() ): 0;
}

template<class U, class... T> constexpr typename std::add_pointer<U>::type get_if(variant<T...>* v) noexcept
{
    static_assert( mp11::mp_count<variant<T...>, U>::value == 1, "The type must occur exactly once in the list of variant alternatives" );

    using I = mp11::mp_find<variant<T...>, U>;

    return v && v->index() == I::value? &v->_get_impl( I() ): 0;
}

template<class U, class... T> constexpr typename std::add_pointer<U const>::type get_if(variant<T...> const * v) noexcept
{
    static_assert( mp11::mp_count<variant<T...>, U>::value == 1, "The type must occur exactly once in the list of variant alternatives" );

    using I = mp11::mp_find<variant<T...>, U>;

    return v && v->index() == I::value? &v->_get_impl( I() ): 0;
}

//

namespace detail
{

// trivially_*

#if defined( BOOST_LIBSTDCXX_VERSION ) && BOOST_LIBSTDCXX_VERSION < 50000

template<class T> struct is_trivially_copy_constructible: mp11::mp_bool<std::is_copy_constructible<T>::value && std::has_trivial_copy_constructor<T>::value>
{
};

template<class T> struct is_trivially_copy_assignable: mp11::mp_bool<std::is_copy_assignable<T>::value && std::has_trivial_copy_assign<T>::value>
{
};

template<class T> struct is_trivially_move_constructible: mp11::mp_bool<std::is_move_constructible<T>::value && std::is_trivial<T>::value>
{
};

template<class T> struct is_trivially_move_assignable: mp11::mp_bool<std::is_move_assignable<T>::value && std::is_trivial<T>::value>
{
};

#else

using std::is_trivially_copy_constructible;
using std::is_trivially_copy_assignable;
using std::is_trivially_move_constructible;
using std::is_trivially_move_assignable;

#endif

// variant_storage

template<class D, class... T> union variant_storage_impl;

template<class... T> using variant_storage = variant_storage_impl<mp11::mp_all<std::is_trivially_destructible<T>...>, T...>;

template<class D> union variant_storage_impl<D>
{
};

// not all trivially destructible
template<class T1, class... T> union variant_storage_impl<mp11::mp_false, T1, T...>
{
    T1 first_;
    variant_storage<T...> rest_;

    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<0>, A&&... a ): first_( std::forward<A>(a)... )
    {
    }

    template<std::size_t I, class... A> constexpr variant_storage_impl( mp11::mp_size_t<I>, A&&... a ): rest_( mp11::mp_size_t<I-1>(), std::forward<A>(a)... )
    {
    }

    ~variant_storage_impl()
    {
    }

    template<class... A> void emplace( mp11::mp_size_t<0>, A&&... a )
    {
        ::new( &first_ ) T1( std::forward<A>(a)... );
    }

    template<std::size_t I, class... A> void emplace( mp11::mp_size_t<I>, A&&... a )
    {
        rest_.emplace( mp11::mp_size_t<I-1>(), std::forward<A>(a)... );
    }

    BOOST_CXX14_CONSTEXPR T1& get( mp11::mp_size_t<0> ) noexcept { return first_; }
    constexpr T1 const& get( mp11::mp_size_t<0> ) const noexcept { return first_; }

    template<std::size_t I> BOOST_CXX14_CONSTEXPR mp11::mp_at_c<mp11::mp_list<T...>, I-1>& get( mp11::mp_size_t<I> ) noexcept { return rest_.get( mp11::mp_size_t<I-1>() ); }
    template<std::size_t I> constexpr mp11::mp_at_c<mp11::mp_list<T...>, I-1> const& get( mp11::mp_size_t<I> ) const noexcept { return rest_.get( mp11::mp_size_t<I-1>() ); }
};

template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class... T> union variant_storage_impl<mp11::mp_false, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T...>
{
    T0 t0_;
    T1 t1_;
    T2 t2_;
    T3 t3_;
    T4 t4_;
    T5 t5_;
    T6 t6_;
    T7 t7_;
    T8 t8_;
    T9 t9_;

    variant_storage<T...> rest_;

    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<0>, A&&... a ): t0_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<1>, A&&... a ): t1_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<2>, A&&... a ): t2_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<3>, A&&... a ): t3_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<4>, A&&... a ): t4_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<5>, A&&... a ): t5_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<6>, A&&... a ): t6_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<7>, A&&... a ): t7_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<8>, A&&... a ): t8_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<9>, A&&... a ): t9_( std::forward<A>(a)... ) {}

    template<std::size_t I, class... A> constexpr variant_storage_impl( mp11::mp_size_t<I>, A&&... a ): rest_( mp11::mp_size_t<I-10>(), std::forward<A>(a)... ) {}

    ~variant_storage_impl()
    {
    }

    template<class... A> void emplace( mp11::mp_size_t<0>, A&&... a ) { ::new( &t0_ ) T0( std::forward<A>(a)... ); }
    template<class... A> void emplace( mp11::mp_size_t<1>, A&&... a ) { ::new( &t1_ ) T1( std::forward<A>(a)... ); }
    template<class... A> void emplace( mp11::mp_size_t<2>, A&&... a ) { ::new( &t2_ ) T2( std::forward<A>(a)... ); }
    template<class... A> void emplace( mp11::mp_size_t<3>, A&&... a ) { ::new( &t3_ ) T3( std::forward<A>(a)... ); }
    template<class... A> void emplace( mp11::mp_size_t<4>, A&&... a ) { ::new( &t4_ ) T4( std::forward<A>(a)... ); }
    template<class... A> void emplace( mp11::mp_size_t<5>, A&&... a ) { ::new( &t5_ ) T5( std::forward<A>(a)... ); }
    template<class... A> void emplace( mp11::mp_size_t<6>, A&&... a ) { ::new( &t6_ ) T6( std::forward<A>(a)... ); }
    template<class... A> void emplace( mp11::mp_size_t<7>, A&&... a ) { ::new( &t7_ ) T7( std::forward<A>(a)... ); }
    template<class... A> void emplace( mp11::mp_size_t<8>, A&&... a ) { ::new( &t8_ ) T8( std::forward<A>(a)... ); }
    template<class... A> void emplace( mp11::mp_size_t<9>, A&&... a ) { ::new( &t9_ ) T9( std::forward<A>(a)... ); }

    template<std::size_t I, class... A> void emplace( mp11::mp_size_t<I>, A&&... a )
    {
        rest_.emplace( mp11::mp_size_t<I-10>(), std::forward<A>(a)... );
    }

    BOOST_CXX14_CONSTEXPR T0& get( mp11::mp_size_t<0> ) noexcept { return t0_; }
    constexpr T0 const& get( mp11::mp_size_t<0> ) const noexcept { return t0_; }

    BOOST_CXX14_CONSTEXPR T1& get( mp11::mp_size_t<1> ) noexcept { return t1_; }
    constexpr T1 const& get( mp11::mp_size_t<1> ) const noexcept { return t1_; }

    BOOST_CXX14_CONSTEXPR T2& get( mp11::mp_size_t<2> ) noexcept { return t2_; }
    constexpr T2 const& get( mp11::mp_size_t<2> ) const noexcept { return t2_; }

    BOOST_CXX14_CONSTEXPR T3& get( mp11::mp_size_t<3> ) noexcept { return t3_; }
    constexpr T3 const& get( mp11::mp_size_t<3> ) const noexcept { return t3_; }

    BOOST_CXX14_CONSTEXPR T4& get( mp11::mp_size_t<4> ) noexcept { return t4_; }
    constexpr T4 const& get( mp11::mp_size_t<4> ) const noexcept { return t4_; }

    BOOST_CXX14_CONSTEXPR T5& get( mp11::mp_size_t<5> ) noexcept { return t5_; }
    constexpr T5 const& get( mp11::mp_size_t<5> ) const noexcept { return t5_; }

    BOOST_CXX14_CONSTEXPR T6& get( mp11::mp_size_t<6> ) noexcept { return t6_; }
    constexpr T6 const& get( mp11::mp_size_t<6> ) const noexcept { return t6_; }

    BOOST_CXX14_CONSTEXPR T7& get( mp11::mp_size_t<7> ) noexcept { return t7_; }
    constexpr T7 const& get( mp11::mp_size_t<7> ) const noexcept { return t7_; }

    BOOST_CXX14_CONSTEXPR T8& get( mp11::mp_size_t<8> ) noexcept { return t8_; }
    constexpr T8 const& get( mp11::mp_size_t<8> ) const noexcept { return t8_; }

    BOOST_CXX14_CONSTEXPR T9& get( mp11::mp_size_t<9> ) noexcept { return t9_; }
    constexpr T9 const& get( mp11::mp_size_t<9> ) const noexcept { return t9_; }

    template<std::size_t I> BOOST_CXX14_CONSTEXPR mp11::mp_at_c<mp11::mp_list<T...>, I-10>& get( mp11::mp_size_t<I> ) noexcept { return rest_.get( mp11::mp_size_t<I-10>() ); }
    template<std::size_t I> constexpr mp11::mp_at_c<mp11::mp_list<T...>, I-10> const& get( mp11::mp_size_t<I> ) const noexcept { return rest_.get( mp11::mp_size_t<I-10>() ); }
};

// all trivially destructible
template<class T1, class... T> union variant_storage_impl<mp11::mp_true, T1, T...>
{
    T1 first_;
    variant_storage<T...> rest_;

    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<0>, A&&... a ): first_( std::forward<A>(a)... )
    {
    }

    template<std::size_t I, class... A> constexpr variant_storage_impl( mp11::mp_size_t<I>, A&&... a ): rest_( mp11::mp_size_t<I-1>(), std::forward<A>(a)... )
    {
    }

    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<0>, A&&... a )
    {
        ::new( &first_ ) T1( std::forward<A>(a)... );
    }

    template<std::size_t I, class... A> BOOST_CXX14_CONSTEXPR void emplace_impl( mp11::mp_false, mp11::mp_size_t<I>, A&&... a )
    {
        rest_.emplace( mp11::mp_size_t<I-1>(), std::forward<A>(a)... );
    }

    template<std::size_t I, class... A> BOOST_CXX14_CONSTEXPR void emplace_impl( mp11::mp_true, mp11::mp_size_t<I>, A&&... a )
    {
        *this = variant_storage_impl( mp11::mp_size_t<I>(), std::forward<A>(a)... );
    }

    template<std::size_t I, class... A> BOOST_CXX14_CONSTEXPR void emplace( mp11::mp_size_t<I>, A&&... a )
    {
        this->emplace_impl( mp11::mp_all<detail::is_trivially_move_assignable<T1>, detail::is_trivially_move_assignable<T>...>(), mp11::mp_size_t<I>(), std::forward<A>(a)... );
    }

    BOOST_CXX14_CONSTEXPR T1& get( mp11::mp_size_t<0> ) noexcept { return first_; }
    constexpr T1 const& get( mp11::mp_size_t<0> ) const noexcept { return first_; }

    template<std::size_t I> BOOST_CXX14_CONSTEXPR mp11::mp_at_c<mp11::mp_list<T...>, I-1>& get( mp11::mp_size_t<I> ) noexcept { return rest_.get( mp11::mp_size_t<I-1>() ); }
    template<std::size_t I> constexpr mp11::mp_at_c<mp11::mp_list<T...>, I-1> const& get( mp11::mp_size_t<I> ) const noexcept { return rest_.get( mp11::mp_size_t<I-1>() ); }
};

template<class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class... T> union variant_storage_impl<mp11::mp_true, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T...>
{
    T0 t0_;
    T1 t1_;
    T2 t2_;
    T3 t3_;
    T4 t4_;
    T5 t5_;
    T6 t6_;
    T7 t7_;
    T8 t8_;
    T9 t9_;

    variant_storage<T...> rest_;

    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<0>, A&&... a ): t0_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<1>, A&&... a ): t1_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<2>, A&&... a ): t2_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<3>, A&&... a ): t3_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<4>, A&&... a ): t4_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<5>, A&&... a ): t5_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<6>, A&&... a ): t6_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<7>, A&&... a ): t7_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<8>, A&&... a ): t8_( std::forward<A>(a)... ) {}
    template<class... A> constexpr variant_storage_impl( mp11::mp_size_t<9>, A&&... a ): t9_( std::forward<A>(a)... ) {}

    template<std::size_t I, class... A> constexpr variant_storage_impl( mp11::mp_size_t<I>, A&&... a ): rest_( mp11::mp_size_t<I-10>(), std::forward<A>(a)... ) {}

    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<0>, A&&... a ) { ::new( &t0_ ) T0( std::forward<A>(a)... ); }
    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<1>, A&&... a ) { ::new( &t1_ ) T1( std::forward<A>(a)... ); }
    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<2>, A&&... a ) { ::new( &t2_ ) T2( std::forward<A>(a)... ); }
    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<3>, A&&... a ) { ::new( &t3_ ) T3( std::forward<A>(a)... ); }
    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<4>, A&&... a ) { ::new( &t4_ ) T4( std::forward<A>(a)... ); }
    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<5>, A&&... a ) { ::new( &t5_ ) T5( std::forward<A>(a)... ); }
    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<6>, A&&... a ) { ::new( &t6_ ) T6( std::forward<A>(a)... ); }
    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<7>, A&&... a ) { ::new( &t7_ ) T7( std::forward<A>(a)... ); }
    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<8>, A&&... a ) { ::new( &t8_ ) T8( std::forward<A>(a)... ); }
    template<class... A> void emplace_impl( mp11::mp_false, mp11::mp_size_t<9>, A&&... a ) { ::new( &t9_ ) T9( std::forward<A>(a)... ); }

    template<std::size_t I, class... A> BOOST_CXX14_CONSTEXPR void emplace_impl( mp11::mp_false, mp11::mp_size_t<I>, A&&... a )
    {
        rest_.emplace( mp11::mp_size_t<I-10>(), std::forward<A>(a)... );
    }

    template<std::size_t I, class... A> BOOST_CXX14_CONSTEXPR void emplace_impl( mp11::mp_true, mp11::mp_size_t<I>, A&&... a )
    {
        *this = variant_storage_impl( mp11::mp_size_t<I>(), std::forward<A>(a)... );
    }

    template<std::size_t I, class... A> BOOST_CXX14_CONSTEXPR void emplace( mp11::mp_size_t<I>, A&&... a )
    {
        this->emplace_impl( mp11::mp_all<
            detail::is_trivially_move_assignable<T0>,
            detail::is_trivially_move_assignable<T1>,
            detail::is_trivially_move_assignable<T2>,
            detail::is_trivially_move_assignable<T3>,
            detail::is_trivially_move_assignable<T4>,
            detail::is_trivially_move_assignable<T5>,
            detail::is_trivially_move_assignable<T6>,
            detail::is_trivially_move_assignable<T7>,
            detail::is_trivially_move_assignable<T8>,
            detail::is_trivially_move_assignable<T9>,
            detail::is_trivially_move_assignable<T>...>(), mp11::mp_size_t<I>(), std::forward<A>(a)... );
    }

    BOOST_CXX14_CONSTEXPR T0& get( mp11::mp_size_t<0> ) noexcept { return t0_; }
    constexpr T0 const& get( mp11::mp_size_t<0> ) const noexcept { return t0_; }

    BOOST_CXX14_CONSTEXPR T1& get( mp11::mp_size_t<1> ) noexcept { return t1_; }
    constexpr T1 const& get( mp11::mp_size_t<1> ) const noexcept { return t1_; }

    BOOST_CXX14_CONSTEXPR T2& get( mp11::mp_size_t<2> ) noexcept { return t2_; }
    constexpr T2 const& get( mp11::mp_size_t<2> ) const noexcept { return t2_; }

    BOOST_CXX14_CONSTEXPR T3& get( mp11::mp_size_t<3> ) noexcept { return t3_; }
    constexpr T3 const& get( mp11::mp_size_t<3> ) const noexcept { return t3_; }

    BOOST_CXX14_CONSTEXPR T4& get( mp11::mp_size_t<4> ) noexcept { return t4_; }
    constexpr T4 const& get( mp11::mp_size_t<4> ) const noexcept { return t4_; }

    BOOST_CXX14_CONSTEXPR T5& get( mp11::mp_size_t<5> ) noexcept { return t5_; }
    constexpr T5 const& get( mp11::mp_size_t<5> ) const noexcept { return t5_; }

    BOOST_CXX14_CONSTEXPR T6& get( mp11::mp_size_t<6> ) noexcept { return t6_; }
    constexpr T6 const& get( mp11::mp_size_t<6> ) const noexcept { return t6_; }

    BOOST_CXX14_CONSTEXPR T7& get( mp11::mp_size_t<7> ) noexcept { return t7_; }
    constexpr T7 const& get( mp11::mp_size_t<7> ) const noexcept { return t7_; }

    BOOST_CXX14_CONSTEXPR T8& get( mp11::mp_size_t<8> ) noexcept { return t8_; }
    constexpr T8 const& get( mp11::mp_size_t<8> ) const noexcept { return t8_; }

    BOOST_CXX14_CONSTEXPR T9& get( mp11::mp_size_t<9> ) noexcept { return t9_; }
    constexpr T9 const& get( mp11::mp_size_t<9> ) const noexcept { return t9_; }

    template<std::size_t I> BOOST_CXX14_CONSTEXPR mp11::mp_at_c<mp11::mp_list<T...>, I-10>& get( mp11::mp_size_t<I> ) noexcept { return rest_.get( mp11::mp_size_t<I-10>() ); }
    template<std::size_t I> constexpr mp11::mp_at_c<mp11::mp_list<T...>, I-10> const& get( mp11::mp_size_t<I> ) const noexcept { return rest_.get( mp11::mp_size_t<I-10>() ); }
};

// resolve_overload_*

template<class... T> struct overload;

template<> struct overload<>
{
    void operator()() const;
};

template<class T1, class... T> struct overload<T1, T...>: overload<T...>
{
    using overload<T...>::operator();
    mp11::mp_identity<T1> operator()(T1) const;
};

#if BOOST_WORKAROUND( BOOST_MSVC, < 1930 )

template<class U, class... T> using resolve_overload_type_ = decltype( overload<T...>()(std::declval<U>()) );

template<class U, class... T> struct resolve_overload_type_impl: mp11::mp_defer< resolve_overload_type_, U, T... >
{
};

template<class U, class... T> using resolve_overload_type = typename resolve_overload_type_impl<U, T...>::type::type;

#else

template<class U, class... T> using resolve_overload_type = typename decltype( overload<T...>()(std::declval<U>()) )::type;

#endif

template<class U, class... T> using resolve_overload_index = mp11::mp_find<mp11::mp_list<T...>, resolve_overload_type<U, T...>>;

// variant_base

template<bool is_trivially_destructible, bool is_single_buffered, class... T> struct variant_base_impl;
template<class... T> using variant_base = variant_base_impl<mp11::mp_all<std::is_trivially_destructible<T>...>::value, mp11::mp_all<std::is_nothrow_move_constructible<T>...>::value, T...>;

struct none {};

// trivially destructible, single buffered
template<class... T> struct variant_base_impl<true, true, T...>
{
    unsigned ix_;
    variant_storage<none, T...> st_;

    constexpr variant_base_impl(): ix_( 0 ), st_( mp11::mp_size_t<0>() )
    {
    }

    template<class I, class... A> constexpr explicit variant_base_impl( I, A&&... a ): ix_( I::value + 1 ), st_( mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... )
    {
    }

    // requires: ix_ == 0
    template<class I, class... A> void _replace( I, A&&... a )
    {
        ::new( &st_ ) variant_storage<none, T...>( mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... );
        ix_ = I::value + 1;
    }

    constexpr std::size_t index() const noexcept
    {
        return ix_ - 1;
    }

    template<std::size_t I> BOOST_CXX14_CONSTEXPR mp11::mp_at_c<variant<T...>, I>& _get_impl( mp11::mp_size_t<I> ) noexcept
    {
        size_t const J = I+1;

        assert( ix_ == J );

        return st_.get( mp11::mp_size_t<J>() );
    }

    template<std::size_t I> constexpr mp11::mp_at_c<variant<T...>, I> const& _get_impl( mp11::mp_size_t<I> ) const noexcept
    {
        // size_t const J = I+1;
        // assert( ix_ == I+1 );

        return st_.get( mp11::mp_size_t<I+1>() );
    }

    template<std::size_t J, class U, class... A> BOOST_CXX14_CONSTEXPR void emplace_impl( mp11::mp_true, A&&... a )
    {
        static_assert( std::is_nothrow_constructible<U, A&&...>::value, "Logic error: U must be nothrow constructible from A&&..." );

        st_.emplace( mp11::mp_size_t<J>(), std::forward<A>(a)... );
        ix_ = J;
    }

    template<std::size_t J, class U, class... A> BOOST_CXX14_CONSTEXPR void emplace_impl( mp11::mp_false, A&&... a )
    {
        static_assert( std::is_nothrow_move_constructible<U>::value, "Logic error: U must be nothrow move constructible" );

        U tmp( std::forward<A>(a)... );

        st_.emplace( mp11::mp_size_t<J>(), std::move(tmp) );
        ix_ = J;
    }

    template<std::size_t I, class... A> BOOST_CXX14_CONSTEXPR void emplace( A&&... a )
    {
        std::size_t const J = I+1;
        using U = mp11::mp_at_c<variant<T...>, I>;

        this->emplace_impl<J, U>( std::is_nothrow_constructible<U, A&&...>(), std::forward<A>(a)... );
    }
};

// trivially destructible, double buffered
template<class... T> struct variant_base_impl<true, false, T...>
{
    unsigned ix_;
    variant_storage<none, T...> st_[ 2 ];

    constexpr variant_base_impl(): ix_( 0 ), st_{ { mp11::mp_size_t<0>() }, { mp11::mp_size_t<0>() } }
    {
    }

    template<class I, class... A> constexpr explicit variant_base_impl( I, A&&... a ): ix_( ( I::value + 1 ) * 2 ), st_{ { mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... }, { mp11::mp_size_t<0>() } }
    {
    }

    // requires: ix_ == 0
    template<class I, class... A> void _replace( I, A&&... a )
    {
        ::new( &st_[ 0 ] ) variant_storage<none, T...>( mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... );
        ix_ = ( I::value + 1 ) * 2;
    }

    constexpr std::size_t index() const noexcept
    {
        return ix_ / 2 - 1;
    }

    template<std::size_t I> BOOST_CXX14_CONSTEXPR mp11::mp_at_c<variant<T...>, I>& _get_impl( mp11::mp_size_t<I> ) noexcept
    {
        assert( index() == I );

        size_t const J = I+1;

        constexpr mp11::mp_size_t<J> j{};
        return st_[ ix_ & 1 ].get( j );
    }

    template<std::size_t I> constexpr mp11::mp_at_c<variant<T...>, I> const& _get_impl( mp11::mp_size_t<I> ) const noexcept
    {
        // assert( index() == I );
        // size_t const J = I+1;
        // constexpr mp_size_t<J> j{};

        return st_[ ix_ & 1 ].get( mp11::mp_size_t<I+1>() );
    }

    template<std::size_t I, class... A> BOOST_CXX14_CONSTEXPR void emplace( A&&... a )
    {
        size_t const J = I+1;

        unsigned i2 = 1 - ( ix_ & 1 );

        st_[ i2 ].emplace( mp11::mp_size_t<J>(), std::forward<A>(a)... );

        ix_ = J * 2 + i2;
    }
};

// not trivially destructible, single buffered
template<class... T> struct variant_base_impl<false, true, T...>
{
    unsigned ix_;
    variant_storage<none, T...> st_;

    constexpr variant_base_impl(): ix_( 0 ), st_( mp11::mp_size_t<0>() )
    {
    }

    template<class I, class... A> constexpr explicit variant_base_impl( I, A&&... a ): ix_( I::value + 1 ), st_( mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... )
    {
    }

    // requires: ix_ == 0
    template<class I, class... A> void _replace( I, A&&... a )
    {
        ::new( &st_ ) variant_storage<none, T...>( mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... );
        ix_ = I::value + 1;
    }

    //[&]( auto I ){
    //    using U = mp_at_c<mp_list<none, T...>, I>;
    //    st1_.get( I ).~U();
    //}

    struct _destroy_L1
    {
        variant_base_impl * this_;

        template<class I> void operator()( I ) const noexcept
        {
            using U = mp11::mp_at<mp11::mp_list<none, T...>, I>;
            this_->st_.get( I() ).~U();
        }
    };

    void _destroy() noexcept
    {
        if( ix_ > 0 )
        {
            mp11::mp_with_index<1 + sizeof...(T)>( ix_, _destroy_L1{ this } );
        }
    }

    ~variant_base_impl() noexcept
    {
        _destroy();
    }

    constexpr std::size_t index() const noexcept
    {
        return ix_ - 1;
    }

    template<std::size_t I> BOOST_CXX14_CONSTEXPR mp11::mp_at_c<variant<T...>, I>& _get_impl( mp11::mp_size_t<I> ) noexcept
    {
        size_t const J = I+1;

        assert( ix_ == J );

        return st_.get( mp11::mp_size_t<J>() );
    }

    template<std::size_t I> constexpr mp11::mp_at_c<variant<T...>, I> const& _get_impl( mp11::mp_size_t<I> ) const noexcept
    {
        // size_t const J = I+1;
        // assert( ix_ == J );

        return st_.get( mp11::mp_size_t<I+1>() );
    }

    template<std::size_t I, class... A> void emplace( A&&... a )
    {
        size_t const J = I+1;

        using U = mp11::mp_at_c<variant<T...>, I>;

        static_assert( std::is_nothrow_move_constructible<U>::value, "Logic error: U must be nothrow move constructible" );

        U tmp( std::forward<A>(a)... );

        _destroy();

        st_.emplace( mp11::mp_size_t<J>(), std::move(tmp) );
        ix_ = J;
    }
};

// not trivially destructible, double buffered
template<class... T> struct variant_base_impl<false, false, T...>
{
    unsigned ix_;

#if defined(__GNUC__) && __GNUC__ < 11 && !defined(__clang__) && !defined(__INTEL_COMPILER)

    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63707 :-(

    variant_storage<none, T...> st1_, st2_;

    constexpr variant_base_impl(): ix_( 0 ), st1_( mp11::mp_size_t<0>() ), st2_( mp11::mp_size_t<0>() )
    {
    }

    template<class I, class... A> constexpr explicit variant_base_impl( I, A&&... a ): ix_( ( I::value + 1 ) * 2 ), st1_( mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... ), st2_( mp11::mp_size_t<0>() )
    {
    }

    BOOST_CXX14_CONSTEXPR variant_storage<none, T...>& storage( unsigned i2 ) noexcept
    {
        return i2 == 0? st1_: st2_;
    }

    constexpr variant_storage<none, T...> const& storage( unsigned i2 ) const noexcept
    {
        return i2 == 0? st1_: st2_;
    }

#else

    variant_storage<none, T...> st_[ 2 ];

    constexpr variant_base_impl(): ix_( 0 ), st_{ { mp11::mp_size_t<0>() }, { mp11::mp_size_t<0>() } }
    {
    }

    template<class I, class... A> constexpr explicit variant_base_impl( I, A&&... a ): ix_( ( I::value + 1 ) * 2 ), st_{ { mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... }, { mp11::mp_size_t<0>() } }
    {
    }

    BOOST_CXX14_CONSTEXPR variant_storage<none, T...>& storage( unsigned i2 ) noexcept
    {
        return st_[ i2 ];
    }

    constexpr variant_storage<none, T...> const& storage( unsigned i2 ) const noexcept
    {
        return st_[ i2 ];
    }

#endif

    // requires: ix_ == 0
    template<class I, class... A> void _replace( I, A&&... a )
    {
        ::new( &storage( 0 ) ) variant_storage<none, T...>( mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... );
        ix_ = ( I::value + 1 ) * 2;
    }

    //[&]( auto I ){
    //    using U = mp_at_c<mp_list<none, T...>, I>;
    //    st1_.get( I ).~U();
    //}

    struct _destroy_L1
    {
        variant_base_impl * this_;
        unsigned i2_;

        template<class I> void operator()( I ) const noexcept
        {
            using U = mp11::mp_at<mp11::mp_list<none, T...>, I>;
            this_->storage( i2_ ).get( I() ).~U();
        }
    };

    void _destroy() noexcept
    {
        mp11::mp_with_index<1 + sizeof...(T)>( ix_ / 2, _destroy_L1{ this, ix_ & 1 } );
    }

    ~variant_base_impl() noexcept
    {
        _destroy();
    }

    constexpr std::size_t index() const noexcept
    {
        return ix_ / 2 - 1;
    }

    template<std::size_t I> BOOST_CXX14_CONSTEXPR mp11::mp_at_c<variant<T...>, I>& _get_impl( mp11::mp_size_t<I> ) noexcept
    {
        assert( index() == I );

        size_t const J = I+1;

        constexpr mp11::mp_size_t<J> j{};
        return storage( ix_ & 1 ).get( j );
    }

    template<std::size_t I> constexpr mp11::mp_at_c<variant<T...>, I> const& _get_impl( mp11::mp_size_t<I> ) const noexcept
    {
        // assert( index() == I );
        // size_t const J = I+1;
        // constexpr mp_size_t<J> j{};

        return storage( ix_ & 1 ).get( mp11::mp_size_t<I+1>() );
    }

    template<std::size_t I, class... A> void emplace( A&&... a )
    {
        size_t const J = I+1;

        unsigned i2 = 1 - ( ix_ & 1 );

        storage( i2 ).emplace( mp11::mp_size_t<J>(), std::forward<A>(a)... );
        _destroy();

        ix_ = J * 2 + i2;
    }
};

} // namespace detail

// in_place_type_t

template<class T> struct in_place_type_t
{
};

#if !defined(BOOST_NO_CXX14_VARIABLE_TEMPLATES)

template<class T> constexpr in_place_type_t<T> in_place_type{};

#endif

namespace detail
{

template<class T> struct is_in_place_type: std::false_type {};
template<class T> struct is_in_place_type<in_place_type_t<T>>: std::true_type {};

} // namespace detail

// in_place_index_t

template<std::size_t I> struct in_place_index_t
{
};

#if !defined(BOOST_NO_CXX14_VARIABLE_TEMPLATES)

template<std::size_t I> constexpr in_place_index_t<I> in_place_index{};

#endif

namespace detail
{

template<class T> struct is_in_place_index: std::false_type {};
template<std::size_t I> struct is_in_place_index<in_place_index_t<I>>: std::true_type {};

} // namespace detail

// is_nothrow_swappable

namespace detail
{

namespace det2
{

using std::swap;

template<class T> using is_swappable_impl = decltype(swap(std::declval<T&>(), std::declval<T&>()));

#if BOOST_WORKAROUND( BOOST_MSVC, < 1920 )

template<class T> struct is_nothrow_swappable_impl_
{
    static constexpr bool value = noexcept(swap(std::declval<T&>(), std::declval<T&>()));
};

template<class T> using is_nothrow_swappable_impl = mp11::mp_bool< is_nothrow_swappable_impl_<T>::value >;

#else

template<class T> using is_nothrow_swappable_impl = typename std::enable_if<noexcept(swap(std::declval<T&>(), std::declval<T&>()))>::type;

#endif

} // namespace det2

template<class T> struct is_swappable: mp11::mp_valid<det2::is_swappable_impl, T>
{
};

#if BOOST_WORKAROUND( BOOST_MSVC, < 1920 )

template<class T> struct is_nothrow_swappable: mp11::mp_eval_if<mp11::mp_not<is_swappable<T>>, mp11::mp_false, det2::is_nothrow_swappable_impl, T>
{
};

#else

template<class T> struct is_nothrow_swappable: mp11::mp_valid<det2::is_nothrow_swappable_impl, T>
{
};

#endif

// variant_cc_base

template<bool CopyConstructible, bool TriviallyCopyConstructible, class... T> struct variant_cc_base_impl;

template<class... T> using variant_cc_base = variant_cc_base_impl<
    mp11::mp_all<std::is_copy_constructible<T>...>::value,
    mp11::mp_all<detail::is_trivially_copy_constructible<T>...>::value,
    T...>;

template<class... T> struct variant_cc_base_impl<true, true, T...>: public variant_base<T...>
{
    using variant_base = detail::variant_base<T...>;
    using variant_base::variant_base;

    variant_cc_base_impl() = default;
    variant_cc_base_impl( variant_cc_base_impl const& ) = default;
    variant_cc_base_impl( variant_cc_base_impl && ) = default;
    variant_cc_base_impl& operator=( variant_cc_base_impl const& ) = default;
    variant_cc_base_impl& operator=( variant_cc_base_impl && ) = default;
};

template<bool B, class... T> struct variant_cc_base_impl<false, B, T...>: public variant_base<T...>
{
    using variant_base = detail::variant_base<T...>;
    using variant_base::variant_base;

    variant_cc_base_impl() = default;
    variant_cc_base_impl( variant_cc_base_impl const& ) = delete;
    variant_cc_base_impl( variant_cc_base_impl && ) = default;
    variant_cc_base_impl& operator=( variant_cc_base_impl const& ) = default;
    variant_cc_base_impl& operator=( variant_cc_base_impl && ) = default;
};

template<class... T> struct variant_cc_base_impl<true, false, T...>: public variant_base<T...>
{
    using variant_base = detail::variant_base<T...>;
    using variant_base::variant_base;

public:

    // constructors

    variant_cc_base_impl() = default;

    // copy constructor

private:

    struct L1
    {
        variant_base * this_;
        variant_base const & r;

        template<class I> void operator()( I i ) const
        {
            this_->_replace( i, r._get_impl( i ) );
        }
    };

public:

    variant_cc_base_impl( variant_cc_base_impl const& r )
        noexcept( mp11::mp_all<std::is_nothrow_copy_constructible<T>...>::value )
        : variant_base()
    {
        mp11::mp_with_index<sizeof...(T)>( r.index(), L1{ this, r } );
    }

    // move constructor

    variant_cc_base_impl( variant_cc_base_impl && ) = default;

    // assignment

    variant_cc_base_impl& operator=( variant_cc_base_impl const & ) = default;
    variant_cc_base_impl& operator=( variant_cc_base_impl && ) = default;
};

// variant_ca_base

template<bool CopyAssignable, bool TriviallyCopyAssignable, class... T> struct variant_ca_base_impl;

template<class... T> using variant_ca_base = variant_ca_base_impl<
    mp11::mp_all<std::is_copy_constructible<T>..., std::is_copy_assignable<T>...>::value,
    mp11::mp_all<std::is_trivially_destructible<T>..., detail::is_trivially_copy_constructible<T>..., detail::is_trivially_copy_assignable<T>...>::value,
    T...>;

template<class... T> struct variant_ca_base_impl<true, true, T...>: public variant_cc_base<T...>
{
    using variant_base = detail::variant_cc_base<T...>;
    using variant_base::variant_base;

    variant_ca_base_impl() = default;
    variant_ca_base_impl( variant_ca_base_impl const& ) = default;
    variant_ca_base_impl( variant_ca_base_impl && ) = default;
    variant_ca_base_impl& operator=( variant_ca_base_impl const& ) = default;
    variant_ca_base_impl& operator=( variant_ca_base_impl && ) = default;
};

template<bool B, class... T> struct variant_ca_base_impl<false, B, T...>: public variant_cc_base<T...>
{
    using variant_base = detail::variant_cc_base<T...>;
    using variant_base::variant_base;

    variant_ca_base_impl() = default;
    variant_ca_base_impl( variant_ca_base_impl const& ) = default;
    variant_ca_base_impl( variant_ca_base_impl && ) = default;
    variant_ca_base_impl& operator=( variant_ca_base_impl const& ) = delete;
    variant_ca_base_impl& operator=( variant_ca_base_impl && ) = default;
};

template<class... T> struct variant_ca_base_impl<true, false, T...>: public variant_cc_base<T...>
{
    using variant_base = detail::variant_cc_base<T...>;
    using variant_base::variant_base;

public:

    // constructors

    variant_ca_base_impl() = default;
    variant_ca_base_impl( variant_ca_base_impl const& ) = default;
    variant_ca_base_impl( variant_ca_base_impl && ) = default;

    // copy assignment

private:

    struct L3
    {
        variant_base * this_;
        variant_base const & r;

        template<class I> void operator()( I i ) const
        {
            this_->template emplace<I::value>( r._get_impl( i ) );
        }
    };

public:

    BOOST_CXX14_CONSTEXPR variant_ca_base_impl& operator=( variant_ca_base_impl const & r )
        noexcept( mp11::mp_all<std::is_nothrow_copy_constructible<T>...>::value )
    {
        mp11::mp_with_index<sizeof...(T)>( r.index(), L3{ this, r } );
        return *this;
    }

    // move assignment

    variant_ca_base_impl& operator=( variant_ca_base_impl && ) = default;
};

// variant_mc_base

template<bool MoveConstructible, bool TriviallyMoveConstructible, class... T> struct variant_mc_base_impl;

template<class... T> using variant_mc_base = variant_mc_base_impl<
    mp11::mp_all<std::is_move_constructible<T>...>::value,
    mp11::mp_all<detail::is_trivially_move_constructible<T>...>::value,
    T...>;

template<class... T> struct variant_mc_base_impl<true, true, T...>: public variant_ca_base<T...>
{
    using variant_base = detail::variant_ca_base<T...>;
    using variant_base::variant_base;

    variant_mc_base_impl() = default;
    variant_mc_base_impl( variant_mc_base_impl const& ) = default;
    variant_mc_base_impl( variant_mc_base_impl && ) = default;
    variant_mc_base_impl& operator=( variant_mc_base_impl const& ) = default;
    variant_mc_base_impl& operator=( variant_mc_base_impl && ) = default;
};

template<bool B, class... T> struct variant_mc_base_impl<false, B, T...>: public variant_ca_base<T...>
{
    using variant_base = detail::variant_ca_base<T...>;
    using variant_base::variant_base;

    variant_mc_base_impl() = default;
    variant_mc_base_impl( variant_mc_base_impl const& ) = default;
    variant_mc_base_impl( variant_mc_base_impl && ) = delete;
    variant_mc_base_impl& operator=( variant_mc_base_impl const& ) = default;
    variant_mc_base_impl& operator=( variant_mc_base_impl && ) = default;
};

template<class... T> struct variant_mc_base_impl<true, false, T...>: public variant_ca_base<T...>
{
    using variant_base = detail::variant_ca_base<T...>;
    using variant_base::variant_base;

public:

    // constructors

    variant_mc_base_impl() = default;
    variant_mc_base_impl( variant_mc_base_impl const& ) = default;

    // move constructor

private:

    struct L2
    {
        variant_base * this_;
        variant_base & r;

        template<class I> void operator()( I i ) const
        {
            this_->_replace( i, std::move( r._get_impl( i ) ) );
        }
    };

public:

    variant_mc_base_impl( variant_mc_base_impl && r )
        noexcept( mp11::mp_all<std::is_nothrow_move_constructible<T>...>::value )
    {
        mp11::mp_with_index<sizeof...(T)>( r.index(), L2{ this, r } );
    }

    // assignment

    variant_mc_base_impl& operator=( variant_mc_base_impl const & ) = default;
    variant_mc_base_impl& operator=( variant_mc_base_impl && ) = default;
};

// variant_ma_base

template<bool MoveAssignable, bool TriviallyMoveAssignable, class... T> struct variant_ma_base_impl;

template<class... T> using variant_ma_base = variant_ma_base_impl<
    mp11::mp_all<std::is_move_constructible<T>..., std::is_move_assignable<T>...>::value,
    mp11::mp_all<std::is_trivially_destructible<T>..., detail::is_trivially_move_constructible<T>..., detail::is_trivially_move_assignable<T>...>::value,
    T...>;

template<class... T> struct variant_ma_base_impl<true, true, T...>: public variant_mc_base<T...>
{
    using variant_base = detail::variant_mc_base<T...>;
    using variant_base::variant_base;

    variant_ma_base_impl() = default;
    variant_ma_base_impl( variant_ma_base_impl const& ) = default;
    variant_ma_base_impl( variant_ma_base_impl && ) = default;
    variant_ma_base_impl& operator=( variant_ma_base_impl const& ) = default;
    variant_ma_base_impl& operator=( variant_ma_base_impl && ) = default;
};

template<bool B, class... T> struct variant_ma_base_impl<false, B, T...>: public variant_mc_base<T...>
{
    using variant_base = detail::variant_mc_base<T...>;
    using variant_base::variant_base;

    variant_ma_base_impl() = default;
    variant_ma_base_impl( variant_ma_base_impl const& ) = default;
    variant_ma_base_impl( variant_ma_base_impl && ) = default;
    variant_ma_base_impl& operator=( variant_ma_base_impl const& ) = default;
    variant_ma_base_impl& operator=( variant_ma_base_impl && ) = delete;
};

template<class... T> struct variant_ma_base_impl<true, false, T...>: public variant_mc_base<T...>
{
    using variant_base = detail::variant_mc_base<T...>;
    using variant_base::variant_base;

public:

    // constructors

    variant_ma_base_impl() = default;
    variant_ma_base_impl( variant_ma_base_impl const& ) = default;
    variant_ma_base_impl( variant_ma_base_impl && ) = default;

    // copy assignment

    variant_ma_base_impl& operator=( variant_ma_base_impl const & ) = default;

    // move assignment

private:

    struct L4
    {
        variant_base * this_;
        variant_base & r;

        template<class I> void operator()( I i ) const
        {
            this_->template emplace<I::value>( std::move( r._get_impl( i ) ) );
        }
    };

public:

    variant_ma_base_impl& operator=( variant_ma_base_impl && r )
        noexcept( mp11::mp_all<std::is_nothrow_move_constructible<T>...>::value )
    {
        mp11::mp_with_index<sizeof...(T)>( r.index(), L4{ this, r } );
        return *this;
    }
};

} // namespace detail

// variant

template<class... T> class variant: private detail::variant_ma_base<T...>
{
private:

    using variant_base = detail::variant_ma_base<T...>;

public:

    // constructors

    template<class E1 = void, class E2 = mp11::mp_if<std::is_default_constructible< mp11::mp_first<variant<T...>> >, E1>>
    constexpr variant()
        noexcept( std::is_nothrow_default_constructible< mp11::mp_first<variant<T...>> >::value )
        : variant_base( mp11::mp_size_t<0>() )
    {
    }

    // variant( variant const& ) = default;
    // variant( variant && ) = default;

    template<class U,
        class Ud = typename std::decay<U>::type,
        class E1 = typename std::enable_if< !std::is_same<Ud, variant>::value && !std::is_base_of<variant, Ud>::value && !detail::is_in_place_index<Ud>::value && !detail::is_in_place_type<Ud>::value >::type,
        class V = detail::resolve_overload_type<U&&, T...>,
        class E2 = typename std::enable_if<std::is_constructible<V, U&&>::value>::type
        >
    constexpr variant( U&& u )
        noexcept( std::is_nothrow_constructible<V, U&&>::value )
        : variant_base( detail::resolve_overload_index<U&&, T...>(), std::forward<U>(u) )
    {
    }

    template<class U, class... A, class I = mp11::mp_find<variant<T...>, U>, class E = typename std::enable_if<std::is_constructible<U, A&&...>::value>::type>
    constexpr explicit variant( in_place_type_t<U>, A&&... a ): variant_base( I(), std::forward<A>(a)... )
    {
    }

    template<class U, class V, class... A, class I = mp11::mp_find<variant<T...>, U>, class E = typename std::enable_if<std::is_constructible<U, std::initializer_list<V>&, A&&...>::value>::type>
    constexpr explicit variant( in_place_type_t<U>, std::initializer_list<V> il, A&&... a ): variant_base( I(), il, std::forward<A>(a)... )
    {
    }

    template<std::size_t I, class... A, class E = typename std::enable_if<std::is_constructible<mp11::mp_at_c<variant<T...>, I>, A&&...>::value>::type>
    constexpr explicit variant( in_place_index_t<I>, A&&... a ): variant_base( mp11::mp_size_t<I>(), std::forward<A>(a)... )
    {
    }

    template<std::size_t I, class V, class... A, class E = typename std::enable_if<std::is_constructible<mp11::mp_at_c<variant<T...>, I>, std::initializer_list<V>&, A&&...>::value>::type>
    constexpr explicit variant( in_place_index_t<I>, std::initializer_list<V> il, A&&... a ): variant_base( mp11::mp_size_t<I>(), il, std::forward<A>(a)... )
    {
    }

    // assignment

    // variant& operator=( variant const& ) = default;
    // variant& operator=( variant && ) = default;

    template<class U,
        class E1 = typename std::enable_if<!std::is_same<typename std::decay<U>::type, variant>::value>::type,
        class V = detail::resolve_overload_type<U, T...>,
        class E2 = typename std::enable_if<std::is_assignable<V&, U&&>::value && std::is_constructible<V, U&&>::value>::type
    >
    BOOST_CXX14_CONSTEXPR variant& operator=( U&& u )
        noexcept( std::is_nothrow_constructible<V, U&&>::value )
    {
        std::size_t const I = detail::resolve_overload_index<U, T...>::value;
        this->template emplace<I>( std::forward<U>(u) );
        return *this;
    }

    // modifiers

    template<class U, class... A,
        class E = typename std::enable_if< mp11::mp_count<variant<T...>, U>::value == 1 && std::is_constructible<U, A&&...>::value >::type>
    BOOST_CXX14_CONSTEXPR U& emplace( A&&... a )
    {
        using I = mp11::mp_find<variant<T...>, U>;
        variant_base::template emplace<I::value>( std::forward<A>(a)... );
        return _get_impl( I() );
    }

    template<class U, class V, class... A,
        class E = typename std::enable_if< mp11::mp_count<variant<T...>, U>::value == 1 && std::is_constructible<U, std::initializer_list<V>&, A&&...>::value >::type>
    BOOST_CXX14_CONSTEXPR U& emplace( std::initializer_list<V> il, A&&... a )
    {
        using I = mp11::mp_find<variant<T...>, U>;
        variant_base::template emplace<I::value>( il, std::forward<A>(a)... );
        return _get_impl( I() );
    }

    template<std::size_t I, class... A, class E = typename std::enable_if<std::is_constructible<mp11::mp_at_c<variant<T...>, I>, A&&...>::value>::type>
    BOOST_CXX14_CONSTEXPR variant_alternative_t<I, variant<T...>>& emplace( A&&... a )
    {
        variant_base::template emplace<I>( std::forward<A>(a)... );
        return _get_impl( mp11::mp_size_t<I>() );
    }

    template<std::size_t I, class V, class... A, class E = typename std::enable_if<std::is_constructible<mp11::mp_at_c<variant<T...>, I>, std::initializer_list<V>&, A&&...>::value>::type>
    BOOST_CXX14_CONSTEXPR variant_alternative_t<I, variant<T...>>& emplace( std::initializer_list<V> il, A&&... a )
    {
        variant_base::template emplace<I>( il, std::forward<A>(a)... );
        return _get_impl( mp11::mp_size_t<I>() );
    }

    // value status

    constexpr bool valueless_by_exception() const noexcept
    {
        return false;
    }

    using variant_base::index;

    // swap

private:

    struct L5
    {
        variant * this_;
        variant & r;

        template<class I> void operator()( I i ) const
        {
            using std::swap;
            swap( this_->_get_impl( i ), r._get_impl( i ) );
        }
    };

public:

    void swap( variant& r ) noexcept( mp11::mp_all<std::is_nothrow_move_constructible<T>..., detail::is_nothrow_swappable<T>...>::value )
    {
        if( index() == r.index() )
        {
            mp11::mp_with_index<sizeof...(T)>( index(), L5{ this, r } );
        }
        else
        {
            variant tmp( std::move(*this) );
            *this = std::move( r );
            r = std::move( tmp );
        }
    }

    // private accessors

    using variant_base::_get_impl;

    // converting constructors (extension)

private:

    template<class... U> struct L6
    {
        variant_base * this_;
        variant<U...> const & r;

        template<class I> void operator()( I i ) const
        {
            using J = mp11::mp_find<mp11::mp_list<T...>, mp11::mp_at<mp11::mp_list<U...>, I>>;
            this_->_replace( J{}, r._get_impl( i ) );
        }
    };

public:

    template<class... U,
        class E2 = mp11::mp_if<mp11::mp_all<std::is_copy_constructible<U>..., mp11::mp_contains<mp11::mp_list<T...>, U>...>, void> >
    variant( variant<U...> const& r )
        noexcept( mp11::mp_all<std::is_nothrow_copy_constructible<U>...>::value )
    {
        mp11::mp_with_index<sizeof...(U)>( r.index(), L6<U...>{ this, r } );
    }

private:

    template<class... U> struct L7
    {
        variant_base * this_;
        variant<U...> & r;

        template<class I> void operator()( I i ) const
        {
            using J = mp11::mp_find<mp11::mp_list<T...>, mp11::mp_at<mp11::mp_list<U...>, I>>;
            this_->_replace( J{}, std::move( r._get_impl( i ) ) );
        }
    };

public:

    template<class... U,
        class E2 = mp11::mp_if<mp11::mp_all<std::is_move_constructible<U>..., mp11::mp_contains<mp11::mp_list<T...>, U>...>, void> >
    variant( variant<U...> && r )
        noexcept( mp11::mp_all<std::is_nothrow_move_constructible<U>...>::value )
    {
        mp11::mp_with_index<sizeof...(U)>( r.index(), L7<U...>{ this, r } );
    }

    // subset (extension)

private:

    template<class... U, class V, std::size_t J, class E = typename std::enable_if<J != sizeof...(U)>::type> static constexpr variant<U...> _subset_impl( mp11::mp_size_t<J>, V && v )
    {
        return variant<U...>( in_place_index_t<J>(), std::forward<V>(v) );
    }

    template<class... U, class V> static variant<U...> _subset_impl( mp11::mp_size_t<sizeof...(U)>, V && /*v*/ )
    {
        detail::throw_bad_variant_access();
    }

private:

    template<class... U> struct L8
    {
        variant * this_;

        template<class I> variant<U...> operator()( I i ) const
        {
            using J = mp11::mp_find<mp11::mp_list<U...>, mp11::mp_at<mp11::mp_list<T...>, I>>;
            return this_->_subset_impl<U...>( J{}, this_->_get_impl( i ) );
        }
    };

public:

    template<class... U,
        class E2 = mp11::mp_if<mp11::mp_all<std::is_copy_constructible<U>..., mp11::mp_contains<mp11::mp_list<T...>, U>...>, void> >
    BOOST_CXX14_CONSTEXPR variant<U...> subset() &
    {
        return mp11::mp_with_index<sizeof...(T)>( index(), L8<U...>{ this } );
    }

private:

    template<class... U> struct L9
    {
        variant const * this_;

        template<class I> variant<U...> operator()( I i ) const
        {
            using J = mp11::mp_find<mp11::mp_list<U...>, mp11::mp_at<mp11::mp_list<T...>, I>>;
            return this_->_subset_impl<U...>( J{}, this_->_get_impl( i ) );
        }
    };

public:

    template<class... U,
        class E2 = mp11::mp_if<mp11::mp_all<std::is_copy_constructible<U>..., mp11::mp_contains<mp11::mp_list<T...>, U>...>, void> >
    constexpr variant<U...> subset() const&
    {
        return mp11::mp_with_index<sizeof...(T)>( index(), L9<U...>{ this } );
    }

private:

    template<class... U> struct L10
    {
        variant * this_;

        template<class I> variant<U...> operator()( I i ) const
        {
            using J = mp11::mp_find<mp11::mp_list<U...>, mp11::mp_at<mp11::mp_list<T...>, I>>;
            return this_->_subset_impl<U...>( J{}, std::move( this_->_get_impl( i ) ) );
        }
    };

public:

    template<class... U,
        class E2 = mp11::mp_if<mp11::mp_all<std::is_copy_constructible<U>..., mp11::mp_contains<mp11::mp_list<T...>, U>...>, void> >
    BOOST_CXX14_CONSTEXPR variant<U...> subset() &&
    {
        return mp11::mp_with_index<sizeof...(T)>( index(), L10<U...>{ this } );
    }

#if !BOOST_WORKAROUND(BOOST_GCC, < 40900)

    // g++ 4.8 doesn't handle const&& particularly well

private:

    template<class... U> struct L11
    {
        variant const * this_;

        template<class I> variant<U...> operator()( I i ) const
        {
            using J = mp11::mp_find<mp11::mp_list<U...>, mp11::mp_at<mp11::mp_list<T...>, I>>;
            return this_->_subset_impl<U...>( J{}, std::move( this_->_get_impl( i ) ) );
        }
    };

public:

    template<class... U,
        class E2 = mp11::mp_if<mp11::mp_all<std::is_copy_constructible<U>..., mp11::mp_contains<mp11::mp_list<T...>, U>...>, void> >
    constexpr variant<U...> subset() const&&
    {
        return mp11::mp_with_index<sizeof...(T)>( index(), L11<U...>{ this } );
    }

#endif
};

// relational operators

namespace detail
{

template<class... T> struct eq_L
{
    variant<T...> const & v;
    variant<T...> const & w;

    template<class I> constexpr bool operator()( I i ) const
    {
        return v._get_impl( i ) == w._get_impl( i );
    }
};

} // namespace detail

template<class... T> constexpr bool operator==( variant<T...> const & v, variant<T...> const & w )
{
    return v.index() == w.index() && mp11::mp_with_index<sizeof...(T)>( v.index(), detail::eq_L<T...>{ v, w } );
}

namespace detail
{

template<class... T> struct ne_L
{
    variant<T...> const & v;
    variant<T...> const & w;

    template<class I> constexpr bool operator()( I i ) const
    {
        return v._get_impl( i ) != w._get_impl( i );
    }
};

} // namespace detail

template<class... T> constexpr bool operator!=( variant<T...> const & v, variant<T...> const & w )
{
    return v.index() != w.index() || mp11::mp_with_index<sizeof...(T)>( v.index(), detail::ne_L<T...>{ v, w } );
}

namespace detail
{

template<class... T> struct lt_L
{
    variant<T...> const & v;
    variant<T...> const & w;

    template<class I> constexpr bool operator()( I i ) const
    {
        return v._get_impl( i ) < w._get_impl( i );
    }
};

} // namespace detail

template<class... T> constexpr bool operator<( variant<T...> const & v, variant<T...> const & w )
{
    return v.index() < w.index() || ( v.index() == w.index() && mp11::mp_with_index<sizeof...(T)>( v.index(), detail::lt_L<T...>{ v, w } ) );
}

template<class... T> constexpr bool operator>(  variant<T...> const & v, variant<T...> const & w )
{
    return w < v;
}

namespace detail
{

template<class... T> struct le_L
{
    variant<T...> const & v;
    variant<T...> const & w;

    template<class I> constexpr bool operator()( I i ) const
    {
        return v._get_impl( i ) <= w._get_impl( i );
    }
};

} // namespace detail

template<class... T> constexpr bool operator<=( variant<T...> const & v, variant<T...> const & w )
{
    return v.index() < w.index() || ( v.index() == w.index() && mp11::mp_with_index<sizeof...(T)>( v.index(), detail::le_L<T...>{ v, w } ) );
}

template<class... T> constexpr bool operator>=( variant<T...> const & v, variant<T...> const & w )
{
    return w <= v;
}

// visitation
namespace detail
{

template<class T> using remove_cv_ref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template<class... T> variant<T...> const & extract_variant_base_( variant<T...> const & );

#if BOOST_WORKAROUND( BOOST_MSVC, < 1930 )

template<class V> struct extract_vbase_impl
{
    using type = decltype( extract_variant_base_( std::declval<V>() ) );
};

template<class V> using extract_variant_base = remove_cv_ref_t< typename extract_vbase_impl<V>::type >;

#else

template<class V> using extract_variant_base = remove_cv_ref_t< decltype( extract_variant_base_( std::declval<V>() ) ) >;

#endif

template<class V> using variant_base_size = variant_size< extract_variant_base<V> >;

template<class T, class U> struct copy_cv_ref
{
    using type = T;
};

template<class T, class U> struct copy_cv_ref<T, U const>
{
    using type = T const;
};

template<class T, class U> struct copy_cv_ref<T, U volatile>
{
    using type = T volatile;
};

template<class T, class U> struct copy_cv_ref<T, U const volatile>
{
    using type = T const volatile;
};

template<class T, class U> struct copy_cv_ref<T, U&>
{
    using type = typename copy_cv_ref<T, U>::type&;
};

template<class T, class U> struct copy_cv_ref<T, U&&>
{
    using type = typename copy_cv_ref<T, U>::type&&;
};

template<class T, class U> using copy_cv_ref_t = typename copy_cv_ref<T, U>::type;

template<class F> struct Qret
{
    template<class... T> using fn = decltype( std::declval<F>()( std::declval<T>()... ) );
};

template<class L> using front_if_same = mp11::mp_if<mp11::mp_apply<mp11::mp_same, L>, mp11::mp_front<L>>;

template<class V> using apply_cv_ref = mp11::mp_product<copy_cv_ref_t, extract_variant_base<V>, mp11::mp_list<V>>;

struct deduced {};

#if !BOOST_WORKAROUND( BOOST_MSVC, < 1920 )

template<class R, class F, class... V> using Vret = mp11::mp_eval_if_not< std::is_same<R, deduced>, R, front_if_same, mp11::mp_product_q<Qret<F>, apply_cv_ref<V>...> >;

#else

template<class R, class F, class... V> struct Vret_impl
{
    using type = mp11::mp_eval_if_not< std::is_same<R, deduced>, R, front_if_same, mp11::mp_product_q<Qret<F>, apply_cv_ref<V>...> >;
};

template<class R, class F, class... V> using Vret = typename Vret_impl<R, F, V...>::type;

#endif

} // namespace detail

template<class R = detail::deduced, class F> constexpr auto visit( F&& f ) -> detail::Vret<R, F>
{
    return std::forward<F>(f)();
}

namespace detail
{

template<class R, class F, class V1> struct visit_L1
{
    F&& f;
    V1&& v1;

    template<class I> auto operator()( I ) const -> Vret<R, F, V1>
    {
        return std::forward<F>(f)( unsafe_get<I::value>( std::forward<V1>(v1) ) );
    }
};

} // namespace detail

template<class R = detail::deduced, class F, class V1> constexpr auto visit( F&& f, V1&& v1 ) -> detail::Vret<R, F, V1>
{
    return mp11::mp_with_index<detail::variant_base_size<V1>>( v1.index(), detail::visit_L1<R, F, V1>{ std::forward<F>(f), std::forward<V1>(v1) } );
}

#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS) || BOOST_WORKAROUND( BOOST_MSVC, < 1920 )

namespace detail
{

template<class F, class A> struct bind_front_
{
    F&& f;
    A&& a;

    template<class... T> auto operator()( T&&... t ) -> decltype( std::forward<F>(f)( std::forward<A>(a), std::forward<T>(t)... ) )
    {
        return std::forward<F>(f)( std::forward<A>(a), std::forward<T>(t)... );
    }
};

template<class F, class A> bind_front_<F, A> bind_front( F&& f, A&& a )
{
    return bind_front_<F, A>{ std::forward<F>(f), std::forward<A>(a) };
}

template<class R, class F, class V1, class V2> struct visit_L2
{
    F&& f;

    V1&& v1;
    V2&& v2;

    template<class I> auto operator()( I ) const -> Vret<R, F, V1, V2>
    {
        auto f2 = bind_front( std::forward<F>(f), unsafe_get<I::value>( std::forward<V1>(v1) ) );
        return visit<R>( f2, std::forward<V2>(v2) );
    }
};

} // namespace detail

template<class R = detail::deduced, class F, class V1, class V2> constexpr auto visit( F&& f, V1&& v1, V2&& v2 ) -> detail::Vret<R, F, V1, V2>
{
    return mp11::mp_with_index<detail::variant_base_size<V1>>( v1.index(), detail::visit_L2<R, F, V1, V2>{ std::forward<F>(f), std::forward<V1>(v1), std::forward<V2>(v2) } );
}

namespace detail
{

template<class R, class F, class V1, class V2, class V3> struct visit_L3
{
    F&& f;

    V1&& v1;
    V2&& v2;
    V3&& v3;

    template<class I> auto operator()( I ) const -> Vret<R, F, V1, V2, V3>
    {
        auto f2 = bind_front( std::forward<F>(f), unsafe_get<I::value>( std::forward<V1>(v1) ) );
        return visit<R>( f2, std::forward<V2>(v2), std::forward<V3>(v3) );
    }
};

} // namespace detail

template<class R = detail::deduced, class F, class V1, class V2, class V3> constexpr auto visit( F&& f, V1&& v1, V2&& v2, V3&& v3 ) -> detail::Vret<R, F, V1, V2, V3>
{
    return mp11::mp_with_index<detail::variant_base_size<V1>>( v1.index(), detail::visit_L3<R, F, V1, V2, V3>{ std::forward<F>(f), std::forward<V1>(v1), std::forward<V2>(v2), std::forward<V3>(v3) } );
}

namespace detail
{

template<class R, class F, class V1, class V2, class V3, class V4> struct visit_L4
{
    F&& f;

    V1&& v1;
    V2&& v2;
    V3&& v3;
    V4&& v4;

    template<class I> auto operator()( I ) const -> Vret<R, F, V1, V2, V3, V4>
    {
        auto f2 = bind_front( std::forward<F>(f), unsafe_get<I::value>( std::forward<V1>(v1) ) );
        return visit<R>( f2, std::forward<V2>(v2), std::forward<V3>(v3), std::forward<V4>(v4) );
    }
};

} // namespace detail

template<class R = detail::deduced, class F, class V1, class V2, class V3, class V4> constexpr auto visit( F&& f, V1&& v1, V2&& v2, V3&& v3, V4&& v4 ) -> detail::Vret<R, F, V1, V2, V3, V4>
{
    return mp11::mp_with_index<detail::variant_base_size<V1>>( v1.index(), detail::visit_L4<R, F, V1, V2, V3, V4>{ std::forward<F>(f), std::forward<V1>(v1), std::forward<V2>(v2), std::forward<V3>(v3), std::forward<V4>(v4) } );
}

#else

template<class R = detail::deduced, class F, class V1, class V2, class... V> constexpr auto visit( F&& f, V1&& v1, V2&& v2, V&&... v ) -> detail::Vret<R, F, V1, V2, V...>
{
    return mp11::mp_with_index<detail::variant_base_size<V1>>( v1.index(), [&]( auto I ){

        auto f2 = [&]( auto&&... a ){ return std::forward<F>(f)( detail::unsafe_get<I.value>( std::forward<V1>(v1) ), std::forward<decltype(a)>(a)... ); };
        return visit<R>( f2, std::forward<V2>(v2), std::forward<V>(v)... );

    });
}

#endif

// specialized algorithms
template<class... T,
    class E = typename std::enable_if<mp11::mp_all<std::is_move_constructible<T>..., detail::is_swappable<T>...>::value>::type>
void swap( variant<T...> & v, variant<T...> & w )
    noexcept( noexcept(v.swap(w)) )
{
    v.swap( w );
}

// hashing support

namespace detail
{

inline std::size_t hash_value_impl_( mp11::mp_true, std::size_t index, std::size_t value )
{
    boost::ulong_long_type hv = ( boost::ulong_long_type( 0xCBF29CE4 ) << 32 ) + 0x84222325;
    boost::ulong_long_type const prime = ( boost::ulong_long_type( 0x00000100 ) << 32 ) + 0x000001B3;

    hv ^= index;
    hv *= prime;

    hv ^= value;
    hv *= prime;

    return static_cast<std::size_t>( hv );
}

inline std::size_t hash_value_impl_( mp11::mp_false, std::size_t index, std::size_t value )
{
    std::size_t hv = 0x811C9DC5;
    std::size_t const prime = 0x01000193;

    hv ^= index;
    hv *= prime;

    hv ^= value;
    hv *= prime;

    return hv;
}

inline std::size_t hash_value_impl( std::size_t index, std::size_t value )
{
    return hash_value_impl_( mp11::mp_bool< (SIZE_MAX > UINT32_MAX) >(), index, value );
}

template<template<class> class H, class V> struct hash_value_L
{
    V const & v;

    template<class I> std::size_t operator()( I ) const
    {
        auto const & t = unsafe_get<I::value>( v );

        std::size_t index = I::value;
        std::size_t value = H<remove_cv_ref_t<decltype(t)>>()( t );

        return hash_value_impl( index, value );
    }
};

template<class... T> std::size_t hash_value_std( variant<T...> const & v )
{
    return mp11::mp_with_index<sizeof...(T)>( v.index(), detail::hash_value_L< std::hash, variant<T...> >{ v } );
}

} // namespace detail

inline std::size_t hash_value( monostate const & )
{
    return 0xA7EE4757u;
}

template<class... T> std::size_t hash_value( variant<T...> const & v )
{
    return mp11::mp_with_index<sizeof...(T)>( v.index(), detail::hash_value_L< boost::hash, variant<T...> >{ v } );
}

namespace detail
{

template<class T> using is_hash_enabled = std::is_default_constructible< std::hash<typename std::remove_const<T>::type> >;

template<class V, bool E = mp11::mp_all_of<V, is_hash_enabled>::value> struct std_hash_impl;

template<class V> struct std_hash_impl<V, false>
{
    std_hash_impl() = delete;
    std_hash_impl( std_hash_impl const& ) = delete;
    std_hash_impl& operator=( std_hash_impl const& ) = delete;
};

template<class V> struct std_hash_impl<V, true>
{
    std::size_t operator()( V const & v ) const
    {
        return detail::hash_value_std( v );
    }
};

} // namespace detail

} // namespace variant2
} // namespace boost

namespace std
{

template<class... T> struct hash< ::boost::variant2::variant<T...> >: public ::boost::variant2::detail::std_hash_impl< ::boost::variant2::variant<T...> >
{
};

template<> struct hash< ::boost::variant2::monostate >
{
    std::size_t operator()( ::boost::variant2::monostate const & v ) const
    {
        return hash_value( v );
    }
};

} // namespace std

#if defined(_MSC_VER) && _MSC_VER < 1910
# pragma warning( pop )
#endif

#endif // #ifndef BOOST_VARIANT2_VARIANT_HPP_INCLUDED
