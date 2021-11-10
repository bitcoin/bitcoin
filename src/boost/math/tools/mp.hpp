//  Copyright Peter Dimov 2015-2021.
//  Copyright Matt Borland 2021.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  Template metaprogramming classes and functions to replace MPL
//  Source: http://www.pdimov.com/cpp2/simple_cxx11_metaprogramming.html
//  Source: https://github.com/boostorg/mp11/

#ifndef BOOST_MATH_TOOLS_MP
#define BOOST_MATH_TOOLS_MP

#include <type_traits>
#include <cstddef>
#include <utility>

namespace boost { namespace math { namespace tools { namespace meta_programming {

// Types:
// Typelist 
template<typename... T>
struct mp_list {};

// Size_t
template<std::size_t N> 
using mp_size_t = std::integral_constant<std::size_t, N>;

// Boolean
template<bool B>
using mp_bool = std::integral_constant<bool, B>;

// Identity
template<typename T>
struct mp_identity
{
    using type = T;
};

// Turns struct into quoted metafunction
template<template<typename...> class F> 
struct mp_quote_trait
{
    template<typename... T> 
    using fn = typename F<T...>::type;
};

namespace detail {
// Size
template<typename L> 
struct mp_size_impl {};

template<template<typename...> class L, typename... T> // Template template parameter must use class
struct mp_size_impl<L<T...>>
{
    using type = std::integral_constant<std::size_t, sizeof...(T)>;
};
}

template<typename T> 
using mp_size = typename detail::mp_size_impl<T>::type;

namespace detail {
// Front
template<typename L>
struct mp_front_impl {};

template<template<typename...> class L, typename T1, typename... T> 
struct mp_front_impl<L<T1, T...>>
{
    using type = T1;
};
}

template<typename T>
using mp_front = typename detail::mp_front_impl<T>::type;

namespace detail {
// At
// TODO - Use tree based lookup for larger typelists
// http://odinthenerd.blogspot.com/2017/04/tree-based-lookup-why-kvasirmpl-is.html
template<typename L, std::size_t>
struct mp_at_c {};

template<template<typename...> class L, typename T0, typename... T>
struct mp_at_c<L<T0, T...>, 0>
{
    using type = T0;
};

template<template<typename...> class L, typename T0, typename T1, typename... T>
struct mp_at_c<L<T0, T1, T...>, 1>
{
    using type = T1;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename... T>
struct mp_at_c<L<T0, T1, T2, T...>, 2>
{
    using type = T2;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T...>, 3>
{
    using type = T3;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename T4, typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T4, T...>, 4>
{
    using type = T4;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T4, T5, T...>, 5>
{
    using type = T5;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
         typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T4, T5, T6, T...>, 6>
{
    using type = T6;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
         typename T7, typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T4, T5, T6, T7, T...>, 7>
{
    using type = T7;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
         typename T7, typename T8, typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T4, T5, T6, T7, T8, T...>, 8>
{
    using type = T8;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
         typename T7, typename T8, typename T9, typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T...>, 9>
{
    using type = T9;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
         typename T7, typename T8, typename T9, typename T10, typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T...>, 10>
{
    using type = T10;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
         typename T7, typename T8, typename T9, typename T10, typename T11, typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T...>, 11>
{
    using type = T11;
};

template<template<typename...> class L, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
         typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename... T>
struct mp_at_c<L<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T...>, 12>
{
    using type = T12;
};
}

template<typename L, std::size_t I>
using mp_at_c = typename detail::mp_at_c<L, I>::type;

template<typename L, typename I>
using mp_at = typename detail::mp_at_c<L, I::value>::type;

// Back
template<typename L> 
using mp_back = mp_at_c<L, mp_size<L>::value - 1>;

namespace detail {
// Push back
template<typename L, typename... T> 
struct mp_push_back_impl {};

template<template<typename...> class L, typename... U, typename... T> 
struct mp_push_back_impl<L<U...>, T...>
{
    using type = L<U..., T...>;
};
}

template<typename L, typename... T>
using mp_push_back = typename detail::mp_push_back_impl<L, T...>::type;

namespace detail {
// Push front
template<typename L, typename... T>
struct mp_push_front_impl {};

template<template<typename...> class L, typename... U, typename... T>
struct mp_push_front_impl<L<U...>, T...>
{
    using type = L<T..., U...>;
};
}

template<typename L, typename... T> 
using mp_push_front = typename detail::mp_push_front_impl<L, T...>::type;

namespace detail{
// If
template<bool C, typename T, typename... E>
struct mp_if_c_impl{};

template<typename T, typename... E>
struct mp_if_c_impl<true, T, E...>
{
    using type = T;
};

template<typename T, typename E>
struct mp_if_c_impl<false, T, E>
{
    using type = E;
};
}

template<bool C, typename T, typename... E> 
using mp_if_c = typename detail::mp_if_c_impl<C, T, E...>::type;

template<typename C, typename T, typename... E> 
using mp_if = typename detail::mp_if_c_impl<static_cast<bool>(C::value), T, E...>::type;

namespace detail {
// Find if
template<typename L, template<typename...> class P>
struct mp_find_if_impl {};

template<template<typename...> class L, template<typename...> class P> 
struct mp_find_if_impl<L<>, P>
{
    using type = mp_size_t<0>;
};

template<typename L, template<typename...> class P> 
struct mp_find_if_impl_2
{
    using r = typename mp_find_if_impl<L, P>::type;
    using type = mp_size_t<1 + r::value>;
};

template<template<typename...> class L, typename T1, typename... T, template<typename...> class P> 
struct mp_find_if_impl<L<T1, T...>, P>
{
    using type = typename mp_if<P<T1>, mp_identity<mp_size_t<0>>, mp_find_if_impl_2<mp_list<T...>, P>>::type;
};
}

template<typename L, template<typename...> class P> 
using mp_find_if = typename detail::mp_find_if_impl<L, P>::type;

template<typename L, typename Q> 
using mp_find_if_q = mp_find_if<L, Q::template fn>;

namespace detail {
// Append
template<typename... L> 
struct mp_append_impl {};

template<> 
struct mp_append_impl<>
{
    using type = mp_list<>;
};

template<template<typename...> class L, typename... T>
struct mp_append_impl<L<T...>>
{
    using type = L<T...>;
};

template<template<typename...> class L1, typename... T1, template<typename...> class L2, typename... T2>
struct mp_append_impl<L1<T1...>, L2<T2...>>
{
    using type = L1<T1..., T2...>;
};

template<template<typename...> class L1, typename... T1, template<typename...> class L2, typename... T2, 
         template<typename...> class L3, typename... T3>
struct mp_append_impl<L1<T1...>, L2<T2...>, L3<T3...>>
{
    using type = L1<T1..., T2..., T3...>;
};

template<template<typename...> class L1, typename... T1, template<typename...> class L2, typename... T2, 
         template<typename...> class L3, typename... T3, template<typename...> class L4, typename... T4>
struct mp_append_impl<L1<T1...>, L2<T2...>, L3<T3...>, L4<T4...>>
{
    using type = L1<T1..., T2..., T3..., T4...>;
};

template<template<typename...> class L1, typename... T1, template<typename...> class L2, typename... T2,
         template<typename...> class L3, typename... T3, template<typename...> class L4, typename... T4, 
         template<typename...> class L5, typename... T5, typename... Lr> 
struct mp_append_impl<L1<T1...>, L2<T2...>, L3<T3...>, L4<T4...>, L5<T5...>, Lr...>
{
    using type = typename mp_append_impl<L1<T1..., T2..., T3..., T4..., T5...>, Lr...>::type;
};
}

template<typename... L> 
using mp_append = typename detail::mp_append_impl<L...>::type;

namespace detail {
// Remove if
template<typename L, template<typename...> class P> 
struct mp_remove_if_impl{};

template<template<typename...> class L, typename... T, template<typename...> class P>
struct mp_remove_if_impl<L<T...>, P>
{    
    template<typename U> 
    struct _f 
    { 
        using type = mp_if<P<U>, mp_list<>, mp_list<U>>; 
    };
    
    using type = mp_append<L<>, typename _f<T>::type...>;
};
}

template<typename L, template<class...> class P> 
using mp_remove_if = typename detail::mp_remove_if_impl<L, P>::type;

template<typename L, typename Q> 
using mp_remove_if_q = mp_remove_if<L, Q::template fn>;

// Index sequence
// Use C++14 index sequence if available
#if defined(__cpp_lib_integer_sequence) && (__cpp_lib_integer_sequence >= 201304)
template<std::size_t... I>
using index_sequence = std::index_sequence<I...>;

template<std::size_t N>
using make_index_sequence = std::make_index_sequence<N>;

template<typename... T>
using index_sequence_for = std::index_sequence_for<T...>;

#else

template<typename T, T... I>
struct integer_sequence {};

template<std::size_t... I>
using index_sequence = integer_sequence<std::size_t, I...>;

namespace detail {

template<bool C, typename T, typename E>
struct iseq_if_c_impl {};

template<typename T, typename F>
struct iseq_if_c_impl<true, T, F>
{
    using type = T;
};

template<typename T, typename F>
struct iseq_if_c_impl<false, T, F>
{
    using type = F;
};

template<bool C, typename T, typename F>
using iseq_if_c = typename iseq_if_c_impl<C, T, F>::type;

template<typename T>
struct iseq_identity
{
    using type = T;
};

template<typename T1, typename T2>
struct append_integer_sequence {};

template<typename T, T... I, T... J>
struct append_integer_sequence<integer_sequence<T, I...>, integer_sequence<T, J...>>
{
    using type = integer_sequence<T, I..., (J + sizeof...(I))...>;
};

template<typename T, T N>
struct make_integer_sequence_impl;

template<typename T, T N>
class make_integer_sequence_impl_
{
private:
    static_assert(N >= 0, "N must not be negative");

    static constexpr T M = N / 2;
    static constexpr T R = N % 2;

    using seq1 = typename make_integer_sequence_impl<T, M>::type;
    using seq2 = typename append_integer_sequence<seq1, seq1>::type;
    using seq3 = typename make_integer_sequence_impl<T, R>::type;
    using seq4 = typename append_integer_sequence<seq2, seq3>::type;

public:
    using type = seq4;
};

template<typename T, T N>
struct make_integer_sequence_impl
{
    using type = typename iseq_if_c<N == 0, 
                                    iseq_identity<integer_sequence<T>>, 
                                    iseq_if_c<N == 1, iseq_identity<integer_sequence<T, 0>>, 
                                    make_integer_sequence_impl_<T, N>>>::type;
};

} // namespace detail

template<typename T, T N>
using make_integer_sequence = typename detail::make_integer_sequence_impl<T, N>::type;

template<std::size_t N>
using make_index_sequence = make_integer_sequence<std::size_t, N>;

template<typename... T>
using index_sequence_for = make_integer_sequence<std::size_t, sizeof...(T)>;

#endif 

}}}} // namespaces

#endif // BOOST_MATH_TOOLS_MP
