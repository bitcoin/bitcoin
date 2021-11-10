// Copyright John Maddock 2008-11.
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_C_MACROS_IPP
#define BOOST_MATH_C_MACROS_IPP

// C99 Functions:
#ifdef acosh
#undef acosh
#endif
#define acosh boost_acosh
#ifdef acoshf
#undef acoshf
#endif
#define acoshf boost_acoshf
#ifdef acoshl
#undef acoshl
#endif
#define acoshl boost_acoshl

#ifdef asinh
#undef asinh
#endif
#define asinh boost_asinh
#ifdef asinhf
#undef asinhf
#endif
#define asinhf boost_asinhf
#ifdef asinhl
#undef asinhl
#endif
#define asinhl boost_asinhl

#ifdef atanh
#undef atanh
#endif
#define atanh boost_atanh
#ifdef atanhf
#undef atanhf
#endif
#define atanhf boost_atanhf
#ifdef atanhl
#undef atanhl
#endif
#define atanhl boost_atanhl

#ifdef cbrt
#undef cbrt
#endif
#define cbrt boost_cbrt
#ifdef cbrtf
#undef cbrtf
#endif
#define cbrtf boost_cbrtf
#ifdef cbrtl
#undef cbrtl
#endif
#define cbrtl boost_cbrtl

#ifdef copysign
#undef copysign
#endif
#define copysign boost_copysign
#ifdef copysignf
#undef copysignf
#endif
#define copysignf boost_copysignf
#ifdef copysignl
#undef copysignl
#endif
#define copysignl boost_copysignl

#ifdef erf
#undef erf
#endif
#define erf boost_erf
#ifdef erff
#undef erff
#endif
#define erff boost_erff
#ifdef erfl
#undef erfl
#endif
#define erfl boost_erfl

#ifdef erfc
#undef erfc
#endif
#define erfc boost_erfc
#ifdef erfcf
#undef erfcf
#endif
#define erfcf boost_erfcf
#ifdef erfcl
#undef erfcl
#endif
#define erfcl boost_erfcl

#if 0
#ifdef exp2
#undef exp2
#endif
#define exp2 boost_exp2
#ifdef exp2f
#undef exp2f
#endif
#define exp2f boost_exp2f
#ifdef exp2l
#undef exp2l
#endif
#define exp2l boost_exp2l
#endif

#ifdef expm1
#undef expm1
#endif
#define expm1 boost_expm1
#ifdef expm1f
#undef expm1f
#endif
#define expm1f boost_expm1f
#ifdef expm1l
#undef expm1l
#endif
#define expm1l boost_expm1l

#if 0
#ifdef fdim
#undef fdim
#endif
#define fdim boost_fdim
#ifdef fdimf
#undef fdimf
#endif
#define fdimf boost_fdimf
#ifdef fdiml
#undef fdiml
#endif
#define fdiml boost_fdiml
#ifdef acosh
#undef acosh
#endif
#define fma boost_fma
#ifdef fmaf
#undef fmaf
#endif
#define fmaf boost_fmaf
#ifdef fmal
#undef fmal
#endif
#define fmal boost_fmal
#endif

#ifdef fmax
#undef fmax
#endif
#define fmax boost_fmax
#ifdef fmaxf
#undef fmaxf
#endif
#define fmaxf boost_fmaxf
#ifdef fmaxl
#undef fmaxl
#endif
#define fmaxl boost_fmaxl

#ifdef fmin
#undef fmin
#endif
#define fmin boost_fmin
#ifdef fminf
#undef fminf
#endif
#define fminf boost_fminf
#ifdef fminl
#undef fminl
#endif
#define fminl boost_fminl

#ifdef hypot
#undef hypot
#endif
#define hypot boost_hypot
#ifdef hypotf
#undef hypotf
#endif
#define hypotf boost_hypotf
#ifdef hypotl
#undef hypotl
#endif
#define hypotl boost_hypotl

#if 0
#ifdef ilogb
#undef ilogb
#endif
#define ilogb boost_ilogb
#ifdef ilogbf
#undef ilogbf
#endif
#define ilogbf boost_ilogbf
#ifdef ilogbl
#undef ilogbl
#endif
#define ilogbl boost_ilogbl
#endif

#ifdef lgamma
#undef lgamma
#endif
#define lgamma boost_lgamma
#ifdef lgammaf
#undef lgammaf
#endif
#define lgammaf boost_lgammaf
#ifdef lgammal
#undef lgammal
#endif
#define lgammal boost_lgammal

#ifdef BOOST_HAS_LONG_LONG
#if 0
#ifdef llrint
#undef llrint
#endif
#define llrint boost_llrint
#ifdef llrintf
#undef llrintf
#endif
#define llrintf boost_llrintf
#ifdef llrintl
#undef llrintl
#endif
#define llrintl boost_llrintl
#endif
#ifdef llround
#undef llround
#endif
#define llround boost_llround
#ifdef llroundf
#undef llroundf
#endif
#define llroundf boost_llroundf
#ifdef llroundl
#undef llroundl
#endif
#define llroundl boost_llroundl
#endif

#ifdef log1p
#undef log1p
#endif
#define log1p boost_log1p
#ifdef log1pf
#undef log1pf
#endif
#define log1pf boost_log1pf
#ifdef log1pl
#undef log1pl
#endif
#define log1pl boost_log1pl

#if 0
#ifdef log2
#undef log2
#endif
#define log2 boost_log2
#ifdef log2f
#undef log2f
#endif
#define log2f boost_log2f
#ifdef log2l
#undef log2l
#endif
#define log2l boost_log2l

#ifdef logb
#undef logb
#endif
#define logb boost_logb
#ifdef logbf
#undef logbf
#endif
#define logbf boost_logbf
#ifdef logbl
#undef logbl
#endif
#define logbl boost_logbl

#ifdef lrint
#undef lrint
#endif
#define lrint boost_lrint
#ifdef lrintf
#undef lrintf
#endif
#define lrintf boost_lrintf
#ifdef lrintl
#undef lrintl
#endif
#define lrintl boost_lrintl
#endif

#ifdef lround
#undef lround
#endif
#define lround boost_lround
#ifdef lroundf
#undef lroundf
#endif
#define lroundf boost_lroundf
#ifdef lroundl
#undef lroundl
#endif
#define lroundl boost_lroundl

#if 0
#ifdef nan
#undef nan
#endif
#define nan boost_nan
#ifdef nanf
#undef nanf
#endif
#define nanf boost_nanf
#ifdef nanl
#undef nanl
#endif
#define nanl boost_nanl

#ifdef nearbyint
#undef nearbyint
#endif
#define nearbyint boost_nearbyint
#ifdef nearbyintf
#undef nearbyintf
#endif
#define nearbyintf boost_nearbyintf
#ifdef nearbyintl
#undef nearbyintl
#endif
#define nearbyintl boost_nearbyintl
#endif

#ifdef nextafter
#undef nextafter
#endif
#define nextafter boost_nextafter
#ifdef nextafterf
#undef nextafterf
#endif
#define nextafterf boost_nextafterf
#ifdef nextafterl
#undef nextafterl
#endif
#define nextafterl boost_nextafterl

#ifdef nexttoward
#undef nexttoward
#endif
#define nexttoward boost_nexttoward
#ifdef nexttowardf
#undef nexttowardf
#endif
#define nexttowardf boost_nexttowardf
#ifdef nexttowardl
#undef nexttowardl
#endif
#define nexttowardl boost_nexttowardl

#if 0
#ifdef remainder
#undef remainder
#endif
#define remainder boost_remainder
#ifdef remainderf
#undef remainderf
#endif
#define remainderf boost_remainderf
#ifdef remainderl
#undef remainderl
#endif
#define remainderl boost_remainderl

#ifdef remquo
#undef remquo
#endif
#define remquo boost_remquo
#ifdef remquof
#undef remquof
#endif
#define remquof boost_remquof
#ifdef remquol
#undef remquol
#endif
#define remquol boost_remquol

#ifdef rint
#undef rint
#endif
#define rint boost_rint
#ifdef rintf
#undef rintf
#endif
#define rintf boost_rintf
#ifdef rintl
#undef rintl
#endif
#define rintl boost_rintl
#endif

#ifdef round
#undef round
#endif
#define round boost_round
#ifdef roundf
#undef roundf
#endif
#define roundf boost_roundf
#ifdef roundl
#undef roundl
#endif
#define roundl boost_roundl

#if 0
#ifdef scalbln
#undef scalbln
#endif
#define scalbln boost_scalbln
#ifdef scalblnf
#undef scalblnf
#endif
#define scalblnf boost_scalblnf
#ifdef scalblnl
#undef scalblnl
#endif
#define scalblnl boost_scalblnl

#ifdef scalbn
#undef scalbn
#endif
#define scalbn boost_scalbn
#ifdef scalbnf
#undef scalbnf
#endif
#define scalbnf boost_scalbnf
#ifdef scalbnl
#undef scalbnl
#endif
#define scalbnl boost_scalbnl
#endif

#ifdef tgamma
#undef tgamma
#endif
#define tgamma boost_tgamma
#ifdef tgammaf
#undef tgammaf
#endif
#define tgammaf boost_tgammaf
#ifdef tgammal
#undef tgammal
#endif
#define tgammal boost_tgammal

#ifdef trunc
#undef trunc
#endif
#define trunc boost_trunc
#ifdef truncf
#undef truncf
#endif
#define truncf boost_truncf
#ifdef truncl
#undef truncl
#endif
#define truncl boost_truncl

// [5.2.1.1] associated Laguerre polynomials:
#ifdef assoc_laguerre
#undef assoc_laguerre
#endif
#define assoc_laguerre boost_assoc_laguerre
#ifdef assoc_laguerref
#undef assoc_laguerref
#endif
#define assoc_laguerref boost_assoc_laguerref
#ifdef assoc_laguerrel
#undef assoc_laguerrel
#endif
#define assoc_laguerrel boost_assoc_laguerrel

// [5.2.1.2] associated Legendre functions:
#ifdef assoc_legendre
#undef assoc_legendre
#endif
#define assoc_legendre boost_assoc_legendre
#ifdef assoc_legendref
#undef assoc_legendref
#endif
#define assoc_legendref boost_assoc_legendref
#ifdef assoc_legendrel
#undef assoc_legendrel
#endif
#define assoc_legendrel boost_assoc_legendrel

// [5.2.1.3] beta function:
#ifdef beta
#undef beta
#endif
#define beta boost_beta
#ifdef betaf
#undef betaf
#endif
#define betaf boost_betaf
#ifdef betal
#undef betal
#endif
#define betal boost_betal

// [5.2.1.4] (complete) elliptic integral of the first kind:
#ifdef comp_ellint_1
#undef comp_ellint_1
#endif
#define comp_ellint_1 boost_comp_ellint_1
#ifdef comp_ellint_1f
#undef comp_ellint_1f
#endif
#define comp_ellint_1f boost_comp_ellint_1f
#ifdef comp_ellint_1l
#undef comp_ellint_1l
#endif
#define comp_ellint_1l boost_comp_ellint_1l

// [5.2.1.5] (complete) elliptic integral of the second kind:
#ifdef comp_ellint_2
#undef comp_ellint_2
#endif
#define comp_ellint_2 boost_comp_ellint_2
#ifdef comp_ellint_2f
#undef comp_ellint_2f
#endif
#define comp_ellint_2f boost_comp_ellint_2f
#ifdef comp_ellint_2l
#undef comp_ellint_2l
#endif
#define comp_ellint_2l boost_comp_ellint_2l

// [5.2.1.6] (complete) elliptic integral of the third kind:
#ifdef comp_ellint_3
#undef comp_ellint_3
#endif
#define comp_ellint_3 boost_comp_ellint_3
#ifdef comp_ellint_3f
#undef comp_ellint_3f
#endif
#define comp_ellint_3f boost_comp_ellint_3f
#ifdef comp_ellint_3l
#undef comp_ellint_3l
#endif
#define comp_ellint_3l boost_comp_ellint_3l

#if 0
// [5.2.1.7] confluent hypergeometric functions:
#ifdef conf_hyper
#undef conf_hyper
#endif
#define conf_hyper boost_conf_hyper
#ifdef conf_hyperf
#undef conf_hyperf
#endif
#define conf_hyperf boost_conf_hyperf
#ifdef conf_hyperl
#undef conf_hyperl
#endif
#define conf_hyperl boost_conf_hyperl
#endif

// [5.2.1.8] regular modified cylindrical Bessel functions:
#ifdef cyl_bessel_i
#undef cyl_bessel_i
#endif
#define cyl_bessel_i boost_cyl_bessel_i
#ifdef cyl_bessel_if
#undef cyl_bessel_if
#endif
#define cyl_bessel_if boost_cyl_bessel_if
#ifdef cyl_bessel_il
#undef cyl_bessel_il
#endif
#define cyl_bessel_il boost_cyl_bessel_il

// [5.2.1.9] cylindrical Bessel functions (of the first kind):
#ifdef cyl_bessel_j
#undef cyl_bessel_j
#endif
#define cyl_bessel_j boost_cyl_bessel_j
#ifdef cyl_bessel_jf
#undef cyl_bessel_jf
#endif
#define cyl_bessel_jf boost_cyl_bessel_jf
#ifdef cyl_bessel_jl
#undef cyl_bessel_jl
#endif
#define cyl_bessel_jl boost_cyl_bessel_jl

// [5.2.1.10] irregular modified cylindrical Bessel functions:
#ifdef cyl_bessel_k
#undef cyl_bessel_k
#endif
#define cyl_bessel_k boost_cyl_bessel_k
#ifdef cyl_bessel_kf
#undef cyl_bessel_kf
#endif
#define cyl_bessel_kf boost_cyl_bessel_kf
#ifdef cyl_bessel_kl
#undef cyl_bessel_kl
#endif
#define cyl_bessel_kl boost_cyl_bessel_kl

// [5.2.1.11] cylindrical Neumann functions BOOST_MATH_C99_THROW_SPEC;
// cylindrical Bessel functions (of the second kind):
#ifdef cyl_neumann
#undef cyl_neumann
#endif
#define cyl_neumann boost_cyl_neumann
#ifdef cyl_neumannf
#undef cyl_neumannf
#endif
#define cyl_neumannf boost_cyl_neumannf
#ifdef cyl_neumannl
#undef cyl_neumannl
#endif
#define cyl_neumannl boost_cyl_neumannl

// [5.2.1.12] (incomplete) elliptic integral of the first kind:
#ifdef ellint_1
#undef ellint_1
#endif
#define ellint_1 boost_ellint_1
#ifdef ellint_1f
#undef ellint_1f
#endif
#define ellint_1f boost_ellint_1f
#ifdef ellint_1l
#undef ellint_1l
#endif
#define ellint_1l boost_ellint_1l

// [5.2.1.13] (incomplete) elliptic integral of the second kind:
#ifdef ellint_2
#undef ellint_2
#endif
#define ellint_2 boost_ellint_2
#ifdef ellint_2f
#undef ellint_2f
#endif
#define ellint_2f boost_ellint_2f
#ifdef ellint_2l
#undef ellint_2l
#endif
#define ellint_2l boost_ellint_2l

// [5.2.1.14] (incomplete) elliptic integral of the third kind:
#ifdef ellint_3
#undef ellint_3
#endif
#define ellint_3 boost_ellint_3
#ifdef ellint_3f
#undef ellint_3f
#endif
#define ellint_3f boost_ellint_3f
#ifdef ellint_3l
#undef ellint_3l
#endif
#define ellint_3l boost_ellint_3l

// [5.2.1.15] exponential integral:
#ifdef expint
#undef expint
#endif
#define expint boost_expint
#ifdef expintf
#undef expintf
#endif
#define expintf boost_expintf
#ifdef expintl
#undef expintl
#endif
#define expintl boost_expintl

// [5.2.1.16] Hermite polynomials:
#ifdef hermite
#undef hermite
#endif
#define hermite boost_hermite
#ifdef hermitef
#undef hermitef
#endif
#define hermitef boost_hermitef
#ifdef hermitel
#undef hermitel
#endif
#define hermitel boost_hermitel

#if 0
// [5.2.1.17] hypergeometric functions:
#ifdef hyperg
#undef hyperg
#endif
#define hyperg boost_hyperg
#ifdef hypergf
#undef hypergf
#endif
#define hypergf boost_hypergf
#ifdef hypergl
#undef hypergl
#endif
#define hypergl boost_hypergl
#endif

// [5.2.1.18] Laguerre polynomials:
#ifdef laguerre
#undef laguerre
#endif
#define laguerre boost_laguerre
#ifdef laguerref
#undef laguerref
#endif
#define laguerref boost_laguerref
#ifdef laguerrel
#undef laguerrel
#endif
#define laguerrel boost_laguerrel

// [5.2.1.19] Legendre polynomials:
#ifdef legendre
#undef legendre
#endif
#define legendre boost_legendre
#ifdef legendref
#undef legendref
#endif
#define legendref boost_legendref
#ifdef legendrel
#undef legendrel
#endif
#define legendrel boost_legendrel

// [5.2.1.20] Riemann zeta function:
#ifdef riemann_zeta
#undef riemann_zeta
#endif
#define riemann_zeta boost_riemann_zeta
#ifdef riemann_zetaf
#undef riemann_zetaf
#endif
#define riemann_zetaf boost_riemann_zetaf
#ifdef riemann_zetal
#undef riemann_zetal
#endif
#define riemann_zetal boost_riemann_zetal

// [5.2.1.21] spherical Bessel functions (of the first kind):
#ifdef sph_bessel
#undef sph_bessel
#endif
#define sph_bessel boost_sph_bessel
#ifdef sph_besself
#undef sph_besself
#endif
#define sph_besself boost_sph_besself
#ifdef sph_bessell
#undef sph_bessell
#endif
#define sph_bessell boost_sph_bessell

// [5.2.1.22] spherical associated Legendre functions:
#ifdef sph_legendre
#undef sph_legendre
#endif
#define sph_legendre boost_sph_legendre
#ifdef sph_legendref
#undef sph_legendref
#endif
#define sph_legendref boost_sph_legendref
#ifdef sph_legendrel
#undef sph_legendrel
#endif
#define sph_legendrel boost_sph_legendrel

// [5.2.1.23] spherical Neumann functions BOOST_MATH_C99_THROW_SPEC;
// spherical Bessel functions (of the second kind):
#ifdef sph_neumann
#undef sph_neumann
#endif
#define sph_neumann boost_sph_neumann
#ifdef sph_neumannf
#undef sph_neumannf
#endif
#define sph_neumannf boost_sph_neumannf
#ifdef sph_neumannl
#undef sph_neumannl
#endif
#define sph_neumannl boost_sph_neumannl

#endif // BOOST_MATH_C_MACROS_IPP
