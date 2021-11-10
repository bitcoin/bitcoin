#ifndef BOOST_MP11_DETAIL_MP_APPEND_HPP_INCLUDED
#define BOOST_MP11_DETAIL_MP_APPEND_HPP_INCLUDED

//  Copyright 2015-2017 Peter Dimov.
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/mp11/detail/mp_list.hpp>
#include <boost/mp11/utility.hpp>
#include <boost/mp11/detail/config.hpp>

namespace boost
{
namespace mp11
{

// mp_append<L...>

namespace detail
{

template<class... L> struct mp_append_impl;

#if BOOST_MP11_WORKAROUND( BOOST_MP11_MSVC, <= 1800 )

template<class... L> struct mp_append_impl
{
};

template<> struct mp_append_impl<>
{
    using type = mp_list<>;
};

template<template<class...> class L, class... T> struct mp_append_impl<L<T...>>
{
    using type = L<T...>;
};

template<template<class...> class L1, class... T1, template<class...> class L2, class... T2> struct mp_append_impl<L1<T1...>, L2<T2...>>
{
    using type = L1<T1..., T2...>;
};

template<template<class...> class L1, class... T1, template<class...> class L2, class... T2, template<class...> class L3, class... T3> struct mp_append_impl<L1<T1...>, L2<T2...>, L3<T3...>>
{
    using type = L1<T1..., T2..., T3...>;
};

template<template<class...> class L1, class... T1, template<class...> class L2, class... T2, template<class...> class L3, class... T3, template<class...> class L4, class... T4> struct mp_append_impl<L1<T1...>, L2<T2...>, L3<T3...>, L4<T4...>>
{
    using type = L1<T1..., T2..., T3..., T4...>;
};

template<template<class...> class L1, class... T1, template<class...> class L2, class... T2, template<class...> class L3, class... T3, template<class...> class L4, class... T4, template<class...> class L5, class... T5, class... Lr> struct mp_append_impl<L1<T1...>, L2<T2...>, L3<T3...>, L4<T4...>, L5<T5...>, Lr...>
{
    using type = typename mp_append_impl<L1<T1..., T2..., T3..., T4..., T5...>, Lr...>::type;
};

#else

template<class L1 = mp_list<>, class L2 = mp_list<>, class L3 = mp_list<>, class L4 = mp_list<>, class L5 = mp_list<>, class L6 = mp_list<>, class L7 = mp_list<>, class L8 = mp_list<>, class L9 = mp_list<>, class L10 = mp_list<>, class L11 = mp_list<>> struct append_11_impl
{
};

template<
    template<class...> class L1, class... T1,
    template<class...> class L2, class... T2,
    template<class...> class L3, class... T3,
    template<class...> class L4, class... T4,
    template<class...> class L5, class... T5,
    template<class...> class L6, class... T6,
    template<class...> class L7, class... T7,
    template<class...> class L8, class... T8,
    template<class...> class L9, class... T9,
    template<class...> class L10, class... T10,
    template<class...> class L11, class... T11>

struct append_11_impl<L1<T1...>, L2<T2...>, L3<T3...>, L4<T4...>, L5<T5...>, L6<T6...>, L7<T7...>, L8<T8...>, L9<T9...>, L10<T10...>, L11<T11...>>
{
    using type = L1<T1..., T2..., T3..., T4..., T5..., T6..., T7..., T8..., T9..., T10..., T11...>;
};

template<

    class L00 = mp_list<>, class L01 = mp_list<>, class L02 = mp_list<>, class L03 = mp_list<>, class L04 = mp_list<>, class L05 = mp_list<>, class L06 = mp_list<>, class L07 = mp_list<>, class L08 = mp_list<>, class L09 = mp_list<>, class L0A = mp_list<>,
    class L10 = mp_list<>, class L11 = mp_list<>, class L12 = mp_list<>, class L13 = mp_list<>, class L14 = mp_list<>, class L15 = mp_list<>, class L16 = mp_list<>, class L17 = mp_list<>, class L18 = mp_list<>, class L19 = mp_list<>,
    class L20 = mp_list<>, class L21 = mp_list<>, class L22 = mp_list<>, class L23 = mp_list<>, class L24 = mp_list<>, class L25 = mp_list<>, class L26 = mp_list<>, class L27 = mp_list<>, class L28 = mp_list<>, class L29 = mp_list<>,
    class L30 = mp_list<>, class L31 = mp_list<>, class L32 = mp_list<>, class L33 = mp_list<>, class L34 = mp_list<>, class L35 = mp_list<>, class L36 = mp_list<>, class L37 = mp_list<>, class L38 = mp_list<>, class L39 = mp_list<>,
    class L40 = mp_list<>, class L41 = mp_list<>, class L42 = mp_list<>, class L43 = mp_list<>, class L44 = mp_list<>, class L45 = mp_list<>, class L46 = mp_list<>, class L47 = mp_list<>, class L48 = mp_list<>, class L49 = mp_list<>,
    class L50 = mp_list<>, class L51 = mp_list<>, class L52 = mp_list<>, class L53 = mp_list<>, class L54 = mp_list<>, class L55 = mp_list<>, class L56 = mp_list<>, class L57 = mp_list<>, class L58 = mp_list<>, class L59 = mp_list<>,
    class L60 = mp_list<>, class L61 = mp_list<>, class L62 = mp_list<>, class L63 = mp_list<>, class L64 = mp_list<>, class L65 = mp_list<>, class L66 = mp_list<>, class L67 = mp_list<>, class L68 = mp_list<>, class L69 = mp_list<>,
    class L70 = mp_list<>, class L71 = mp_list<>, class L72 = mp_list<>, class L73 = mp_list<>, class L74 = mp_list<>, class L75 = mp_list<>, class L76 = mp_list<>, class L77 = mp_list<>, class L78 = mp_list<>, class L79 = mp_list<>,
    class L80 = mp_list<>, class L81 = mp_list<>, class L82 = mp_list<>, class L83 = mp_list<>, class L84 = mp_list<>, class L85 = mp_list<>, class L86 = mp_list<>, class L87 = mp_list<>, class L88 = mp_list<>, class L89 = mp_list<>,
    class L90 = mp_list<>, class L91 = mp_list<>, class L92 = mp_list<>, class L93 = mp_list<>, class L94 = mp_list<>, class L95 = mp_list<>, class L96 = mp_list<>, class L97 = mp_list<>, class L98 = mp_list<>, class L99 = mp_list<>,
    class LA0 = mp_list<>, class LA1 = mp_list<>, class LA2 = mp_list<>, class LA3 = mp_list<>, class LA4 = mp_list<>, class LA5 = mp_list<>, class LA6 = mp_list<>, class LA7 = mp_list<>, class LA8 = mp_list<>, class LA9 = mp_list<>

> struct append_111_impl
{
    using type = typename append_11_impl<

        typename append_11_impl<L00, L01, L02, L03, L04, L05, L06, L07, L08, L09, L0A>::type,
        typename append_11_impl<mp_list<>, L10, L11, L12, L13, L14, L15, L16, L17, L18, L19>::type,
        typename append_11_impl<mp_list<>, L20, L21, L22, L23, L24, L25, L26, L27, L28, L29>::type,
        typename append_11_impl<mp_list<>, L30, L31, L32, L33, L34, L35, L36, L37, L38, L39>::type,
        typename append_11_impl<mp_list<>, L40, L41, L42, L43, L44, L45, L46, L47, L48, L49>::type,
        typename append_11_impl<mp_list<>, L50, L51, L52, L53, L54, L55, L56, L57, L58, L59>::type,
        typename append_11_impl<mp_list<>, L60, L61, L62, L63, L64, L65, L66, L67, L68, L69>::type,
        typename append_11_impl<mp_list<>, L70, L71, L72, L73, L74, L75, L76, L77, L78, L79>::type,
        typename append_11_impl<mp_list<>, L80, L81, L82, L83, L84, L85, L86, L87, L88, L89>::type,
        typename append_11_impl<mp_list<>, L90, L91, L92, L93, L94, L95, L96, L97, L98, L99>::type,
        typename append_11_impl<mp_list<>, LA0, LA1, LA2, LA3, LA4, LA5, LA6, LA7, LA8, LA9>::type

    >::type;
};

template<

    class L00, class L01, class L02, class L03, class L04, class L05, class L06, class L07, class L08, class L09, class L0A,
    class L10, class L11, class L12, class L13, class L14, class L15, class L16, class L17, class L18, class L19,
    class L20, class L21, class L22, class L23, class L24, class L25, class L26, class L27, class L28, class L29,
    class L30, class L31, class L32, class L33, class L34, class L35, class L36, class L37, class L38, class L39,
    class L40, class L41, class L42, class L43, class L44, class L45, class L46, class L47, class L48, class L49,
    class L50, class L51, class L52, class L53, class L54, class L55, class L56, class L57, class L58, class L59,
    class L60, class L61, class L62, class L63, class L64, class L65, class L66, class L67, class L68, class L69,
    class L70, class L71, class L72, class L73, class L74, class L75, class L76, class L77, class L78, class L79,
    class L80, class L81, class L82, class L83, class L84, class L85, class L86, class L87, class L88, class L89,
    class L90, class L91, class L92, class L93, class L94, class L95, class L96, class L97, class L98, class L99,
    class LA0, class LA1, class LA2, class LA3, class LA4, class LA5, class LA6, class LA7, class LA8, class LA9,
    class... Lr

> struct append_inf_impl
{
    using prefix = typename append_111_impl<

        L00, L01, L02, L03, L04, L05, L06, L07, L08, L09, L0A,
        L10, L11, L12, L13, L14, L15, L16, L17, L18, L19,
        L20, L21, L22, L23, L24, L25, L26, L27, L28, L29,
        L30, L31, L32, L33, L34, L35, L36, L37, L38, L39,
        L40, L41, L42, L43, L44, L45, L46, L47, L48, L49,
        L50, L51, L52, L53, L54, L55, L56, L57, L58, L59,
        L60, L61, L62, L63, L64, L65, L66, L67, L68, L69,
        L70, L71, L72, L73, L74, L75, L76, L77, L78, L79,
        L80, L81, L82, L83, L84, L85, L86, L87, L88, L89,
        L90, L91, L92, L93, L94, L95, L96, L97, L98, L99,
        LA0, LA1, LA2, LA3, LA4, LA5, LA6, LA7, LA8, LA9

    >::type;

    using type = typename mp_append_impl<prefix, Lr...>::type;
};

#if BOOST_MP11_WORKAROUND( BOOST_MP11_CUDA, >= 9000000 && BOOST_MP11_CUDA < 10000000 )

template<class... L>
struct mp_append_impl_cuda_workaround
{
    using type = mp_if_c<(sizeof...(L) > 111), mp_quote<append_inf_impl>, mp_if_c<(sizeof...(L) > 11), mp_quote<append_111_impl>, mp_quote<append_11_impl> > >;
};

template<class... L> struct mp_append_impl: mp_append_impl_cuda_workaround<L...>::type::template fn<L...>
{
};

#else

template<class... L> struct mp_append_impl: mp_if_c<(sizeof...(L) > 111), mp_quote<append_inf_impl>, mp_if_c<(sizeof...(L) > 11), mp_quote<append_111_impl>, mp_quote<append_11_impl> > >::template fn<L...>
{
};

#endif

#endif

} // namespace detail

template<class... L> using mp_append = typename detail::mp_append_impl<L...>::type;

} // namespace mp11
} // namespace boost

#endif // #ifndef BOOST_MP11_DETAIL_MP_APPEND_HPP_INCLUDED
