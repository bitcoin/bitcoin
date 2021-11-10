//
// Copyright 2012-2021 Antony Polukhin.
//
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_TYPE_INDEX_DETAIL_COMPILE_TIME_TYPE_INFO_HPP
#define BOOST_TYPE_INDEX_DETAIL_COMPILE_TIME_TYPE_INFO_HPP

/// \file compile_time_type_info.hpp
/// \brief Contains helper macros and implementation details of boost::typeindex::ctti_type_index.
/// Not intended for inclusion from user's code.

#include <cstring>
#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/integral_constant.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

/// @cond
#if defined(__has_builtin)
#if __has_builtin(__builtin_constant_p)
#define BOOST_TYPE_INDEX_DETAIL_IS_CONSTANT(x) __builtin_constant_p(x)
#endif
#if __has_builtin(__builtin_strcmp)
#define BOOST_TYPE_INDEX_DETAIL_BUILTIN_STRCMP(str1, str2) __builtin_strcmp(str1, str2)
#endif
#elif defined(__GNUC__)
#define BOOST_TYPE_INDEX_DETAIL_IS_CONSTANT(x) __builtin_constant_p(x)
#define BOOST_TYPE_INDEX_DETAIL_BUILTIN_STRCMP(str1, str2) __builtin_strcmp(str1, str2)
#endif

#define BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(begin_skip, end_skip, runtime_skip, runtime_skip_until)   \
    namespace boost { namespace typeindex { namespace detail {                                                  \
        BOOST_STATIC_CONSTEXPR std::size_t ctti_skip_size_at_begin  = begin_skip;                               \
        BOOST_STATIC_CONSTEXPR std::size_t ctti_skip_size_at_end    = end_skip;                                 \
        BOOST_STATIC_CONSTEXPR bool ctti_skip_more_at_runtime       = runtime_skip;                             \
        BOOST_STATIC_CONSTEXPR char ctti_skip_until_runtime[]       = runtime_skip_until;                       \
    }}} /* namespace boost::typeindex::detail */                                                                \
    /**/
/// @endcond


#if defined(BOOST_TYPE_INDEX_DOXYGEN_INVOKED)
    /* Nothing to document. All the macro docs are moved to <boost/type_index.hpp> */
#elif defined(BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING)
#   include <boost/preprocessor/facilities/expand.hpp>
    BOOST_PP_EXPAND( BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING )
#elif defined(_MSC_VER) && !defined(__clang__) && defined (BOOST_NO_CXX11_NOEXCEPT)
    // sizeof("const char *__cdecl boost::detail::ctti<") - 1, sizeof(">::n(void)") - 1
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(40, 10, false, "")
#elif defined(_MSC_VER) && !defined(__clang__) && !defined (BOOST_NO_CXX11_NOEXCEPT)
    // sizeof("const char *__cdecl boost::detail::ctti<") - 1, sizeof(">::n(void) noexcept") - 1
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(40, 19, false, "")
#elif defined(__clang__) && defined(__APPLE__)
    // Someone made __clang_major__ equal to LLVM version rather than compiler version
    // on APPLE platform.
    //
    // Using less efficient solution because there is no good way to detect real version of Clang.
    // sizeof("static const char *boost::detail::ctti<") - 1, sizeof("]") - 1, true, "???????????>::n() [T = int"
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(39, 1, true, "T = ")
#elif defined(__clang__) && (__clang_major__ < 3 || (__clang_major__ == 3 && __clang_minor__ == 0))
    // sizeof("static const char *boost::detail::ctti<") - 1, sizeof(">::n()") - 1
    // note: checked on 3.0
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(39, 6, false, "")
#elif defined(__clang__) && (__clang_major__ >= 4 || (__clang_major__ == 3 && __clang_minor__ > 0))
    // sizeof("static const char *boost::detail::ctti<") - 1, sizeof("]") - 1, true, "int>::n() [T = int"
    // note: checked on 3.1, 3.4
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(39, 1, true, "T = ")
#elif defined(__EDG__) && !defined(BOOST_NO_CXX14_CONSTEXPR)
    // sizeof("static cha boost::detail::ctti<T>::s() [with I = 40U, T = ") - 1, sizeof("]") - 1
    // note: checked on 4.14
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(58, 1, false, "")
#elif defined(__EDG__) && defined(BOOST_NO_CXX14_CONSTEXPR)
    // sizeof("static const char *boost::detail::ctti<T>::n() [with T = ") - 1, sizeof("]") - 1
    // note: checked on 4.14
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(57, 1, false, "")
#elif defined(__GNUC__) && (__GNUC__ < 7) && !defined(BOOST_NO_CXX14_CONSTEXPR)
    // sizeof("static constexpr char boost::detail::ctti<T>::s() [with unsigned int I = 0u; T = ") - 1, sizeof("]") - 1
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(81, 1, false, "")
#elif defined(__GNUC__) && (__GNUC__ >= 7) && !defined(BOOST_NO_CXX14_CONSTEXPR)
    // sizeof("static constexpr char boost::detail::ctti<T>::s() [with unsigned int I = 0; T = ") - 1, sizeof("]") - 1
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(80, 1, false, "")
#elif defined(__GNUC__) && defined(BOOST_NO_CXX14_CONSTEXPR)
    // sizeof("static const char* boost::detail::ctti<T>::n() [with T = ") - 1, sizeof("]") - 1
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(57, 1, false, "")
#elif defined(__ghs__)
    // sizeof("static const char *boost::detail::ctti<T>::n() [with T = ") - 1, sizeof("]") - 1
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(57, 1, false, "")
#else
    // Deafult code for other platforms... Just skip nothing!
    BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS(0, 0, false, "")
#endif

#undef BOOST_TYPE_INDEX_REGISTER_CTTI_PARSING_PARAMS

namespace boost { namespace typeindex { namespace detail {
    template <bool Condition>
    BOOST_CXX14_CONSTEXPR inline void assert_compile_time_legths() BOOST_NOEXCEPT {
        BOOST_STATIC_ASSERT_MSG(
            Condition,
            "TypeIndex library is misconfigured for your compiler. "
            "Please define BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING to correct values. See section "
            "'RTTI emulation limitations' of the documentation for more information."
        );
    }

    template <class T>
    BOOST_CXX14_CONSTEXPR inline void failed_to_get_function_name() BOOST_NOEXCEPT {
        BOOST_STATIC_ASSERT_MSG(
            sizeof(T) && false,
            "TypeIndex library could not detect your compiler. "
            "Please make the BOOST_TYPE_INDEX_FUNCTION_SIGNATURE macro use "
            "correct compiler macro for getting the whole function name. "
            "Define BOOST_TYPE_INDEX_CTTI_USER_DEFINED_PARSING to correct value after that."
        );
    }

#if defined(BOOST_TYPE_INDEX_DETAIL_IS_CONSTANT)
    BOOST_CXX14_CONSTEXPR BOOST_FORCEINLINE bool is_constant_string(const char* str) BOOST_NOEXCEPT {
        while (BOOST_TYPE_INDEX_DETAIL_IS_CONSTANT(*str)) {
            if (*str == '\0')
                return true;
            ++str;
        }
        return false;
    }
#endif // defined(BOOST_TYPE_INDEX_DETAIL_IS_CONSTANT)

    template <unsigned int ArrayLength>
    BOOST_CXX14_CONSTEXPR inline const char* skip_begining_runtime(const char* begin, boost::false_type) BOOST_NOEXCEPT {
        return begin;
    }

    template<class ForwardIterator1, class ForwardIterator2>
    BOOST_CXX14_CONSTEXPR inline ForwardIterator1 constexpr_search(
        ForwardIterator1 first1,
        ForwardIterator1 last1,
        ForwardIterator2 first2,
        ForwardIterator2 last2) BOOST_NOEXCEPT
    {
        if (first2 == last2) {
            return first1;  // specified in C++11
        }

        while (first1 != last1) {
            ForwardIterator1 it1 = first1;
            ForwardIterator2 it2 = first2;

            while (*it1 == *it2) {
                ++it1;
                ++it2;
                if (it2 == last2) return first1;
                if (it1 == last1) return last1;
            }

            ++first1;
        }

        return last1;
    }

    BOOST_CXX14_CONSTEXPR inline int constexpr_strcmp_loop(const char *v1, const char *v2) BOOST_NOEXCEPT {
        while (*v1 != '\0' && *v1 == *v2) {
            ++v1;
            ++v2;
        }

        return static_cast<int>(*v1) - *v2;
    }

    BOOST_CXX14_CONSTEXPR inline int constexpr_strcmp(const char *v1, const char *v2) BOOST_NOEXCEPT {
#if !defined(BOOST_NO_CXX14_CONSTEXPR) && defined(BOOST_TYPE_INDEX_DETAIL_IS_CONSTANT) && defined(BOOST_TYPE_INDEX_DETAIL_BUILTIN_STRCMP)
        if (boost::typeindex::detail::is_constant_string(v1) && boost::typeindex::detail::is_constant_string(v2))
            return boost::typeindex::detail::constexpr_strcmp_loop(v1, v2);
        return BOOST_TYPE_INDEX_DETAIL_BUILTIN_STRCMP(v1, v2);
#elif !defined(BOOST_NO_CXX14_CONSTEXPR)
        return boost::typeindex::detail::constexpr_strcmp_loop(v1, v2);
#else
        return std::strcmp(v1, v2);
#endif
    }

    template <unsigned int ArrayLength>
    BOOST_CXX14_CONSTEXPR inline const char* skip_begining_runtime(const char* begin, boost::true_type) BOOST_NOEXCEPT {
        const char* const it = constexpr_search(
            begin, begin + ArrayLength,
            ctti_skip_until_runtime, ctti_skip_until_runtime + sizeof(ctti_skip_until_runtime) - 1
        );
        return (it == begin + ArrayLength ? begin : it + sizeof(ctti_skip_until_runtime) - 1);
    }

    template <unsigned int ArrayLength>
    BOOST_CXX14_CONSTEXPR inline const char* skip_begining(const char* begin) BOOST_NOEXCEPT {
        assert_compile_time_legths<(ArrayLength > ctti_skip_size_at_begin + ctti_skip_size_at_end)>();
        return skip_begining_runtime<ArrayLength - ctti_skip_size_at_begin>(
            begin + ctti_skip_size_at_begin,
            boost::integral_constant<bool, ctti_skip_more_at_runtime>()
        );
    }

#if !defined(__clang__) && defined(__GNUC__) && !defined(BOOST_NO_CXX14_CONSTEXPR)
    template <unsigned int... I>
    struct index_seq {};

    template <typename Left, typename Right>
    struct make_index_sequence_join;

    template <unsigned int... Left, unsigned int... Right>
    struct make_index_sequence_join<index_seq<Left...>, index_seq<Right...> > {
        typedef index_seq<Left..., Right...> type;
    };

    template <unsigned int C, unsigned int D>
    struct make_index_seq_impl {
        typedef typename make_index_sequence_join<
            typename make_index_seq_impl<C, D / 2>::type,
            typename make_index_seq_impl<C + D / 2, (D + 1) / 2>::type
        >::type type;
    };

    template <unsigned int C>
    struct make_index_seq_impl<C, 0> {
        typedef index_seq<> type;
    };

    template <unsigned int C>
    struct make_index_seq_impl<C, 1> {
        typedef index_seq<C> type;
    };

    template <char... C>
    struct cstring {
        static constexpr unsigned int size_ = sizeof...(C);
        static constexpr char data_[size_] = { C... };
    };

    template <char... C>
    constexpr char cstring<C...>::data_[];
#endif

}}} // namespace boost::typeindex::detail

namespace boost { namespace detail {

/// Noncopyable type_info that does not require RTTI.
/// CTTI == Compile Time Type Info.
/// This name must be as short as possible, to avoid code bloat
template <class T>
struct ctti {

#if !defined(__clang__) && defined(__GNUC__) && !defined(BOOST_NO_CXX14_CONSTEXPR)
    //helper functions
    template <unsigned int I>
    constexpr static char s() BOOST_NOEXCEPT { // step
        constexpr unsigned int offset =
                  (I >= 10u      ? 1u : 0u)
                + (I >= 100u     ? 1u : 0u)
                + (I >= 1000u    ? 1u : 0u)
                + (I >= 10000u   ? 1u : 0u)
                + (I >= 100000u  ? 1u : 0u)
                + (I >= 1000000u ? 1u : 0u)
        ;

    #if defined(BOOST_TYPE_INDEX_FUNCTION_SIGNATURE)
        return BOOST_TYPE_INDEX_FUNCTION_SIGNATURE[I + offset];
    #elif defined(__FUNCSIG__)
        return __FUNCSIG__[I + offset];
    #else
        return __PRETTY_FUNCTION__[I + offset];
    #endif
    }

    template <unsigned int ...Indexes>
    constexpr static const char* impl(::boost::typeindex::detail::index_seq<Indexes...> ) BOOST_NOEXCEPT {
        return ::boost::typeindex::detail::cstring<s<Indexes>()...>::data_;
    }

    template <unsigned int D = 0> // `D` means `Dummy`
    constexpr static const char* n() BOOST_NOEXCEPT {
    #if defined(BOOST_TYPE_INDEX_FUNCTION_SIGNATURE)
        constexpr unsigned int size = sizeof(BOOST_TYPE_INDEX_FUNCTION_SIGNATURE);
    #elif defined(__FUNCSIG__)
        constexpr unsigned int size = sizeof(__FUNCSIG__);
    #elif defined(__PRETTY_FUNCTION__) \
                    || defined(__GNUC__) \
                    || (defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x5130)) \
                    || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) \
                    || (defined(__ICC) && (__ICC >= 600)) \
                    || defined(__ghs__) \
                    || defined(__DMC__)
        constexpr unsigned int size = sizeof(__PRETTY_FUNCTION__);
    #else
        boost::typeindex::detail::failed_to_get_function_name<T>();
    #endif

        boost::typeindex::detail::assert_compile_time_legths<
            (size > boost::typeindex::detail::ctti_skip_size_at_begin + boost::typeindex::detail::ctti_skip_size_at_end + sizeof("const *") - 1)
        >();
        static_assert(!boost::typeindex::detail::ctti_skip_more_at_runtime, "Skipping for GCC in C++14 mode is unsupported");

        typedef typename boost::typeindex::detail::make_index_seq_impl<
            boost::typeindex::detail::ctti_skip_size_at_begin,
            size - sizeof("const *") + 1 - boost::typeindex::detail::ctti_skip_size_at_begin
        >::type idx_seq;
        return impl(idx_seq());
    }
#else
    /// Returns raw name. Must be as short, as possible, to avoid code bloat
    BOOST_CXX14_CONSTEXPR static const char* n() BOOST_NOEXCEPT {
    #if defined(BOOST_TYPE_INDEX_FUNCTION_SIGNATURE)
        return boost::typeindex::detail::skip_begining< sizeof(BOOST_TYPE_INDEX_FUNCTION_SIGNATURE) >(BOOST_TYPE_INDEX_FUNCTION_SIGNATURE);
    #elif defined(__FUNCSIG__)
        return boost::typeindex::detail::skip_begining< sizeof(__FUNCSIG__) >(__FUNCSIG__);
    #elif defined(__PRETTY_FUNCTION__) \
                || defined(__GNUC__) \
                || (defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x5130)) \
                || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) \
                || (defined(__ICC) && (__ICC >= 600)) \
                || defined(__ghs__) \
                || defined(__DMC__) \
                || defined(__clang__)
        return boost::typeindex::detail::skip_begining< sizeof(__PRETTY_FUNCTION__) >(__PRETTY_FUNCTION__);
    #else
        boost::typeindex::detail::failed_to_get_function_name<T>();
        return "";
    #endif
    }
#endif
};

}} // namespace boost::detail

#endif // BOOST_TYPE_INDEX_DETAIL_COMPILE_TIME_TYPE_INFO_HPP
