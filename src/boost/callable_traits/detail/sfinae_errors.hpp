/*
@Copyright Barrett Adair 2016-2017

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_SFINAE_ERRORS_HPP
#define BOOST_CLBL_TRTS_SFINAE_ERRORS_HPP

#include <boost/callable_traits/detail/config.hpp>

namespace boost { namespace callable_traits { namespace detail {

        struct sfinae_error{};

        template<typename T>
        struct success {
            static constexpr bool value = true;
            struct _ { using type = T; };
        };
    
        template<bool B, typename T>
        struct fail_if : T {
            static_assert(std::is_base_of<sfinae_error, T>::value,
                "incorrect usage of fail_if");

            static constexpr bool value = B;
        };

        template<typename T, typename... FailIfs>
        using sfinae_try = typename BOOST_CLBL_TRTS_DISJUNCTION(
                FailIfs..., success<T>)::_::type;

        template<typename FailMsg, typename ForceTwoPhaseLookup>
        struct fail {
            using type = typename std::conditional<std::is_same<ForceTwoPhaseLookup, std::false_type>::value,
            FailMsg, FailMsg>::type::_::type;
        };

}}} // namespace boost::callable_traits::detail

#define BOOST_CLBL_TRTS_PP_CAT_(x, y) x ## y
#define BOOST_CLBL_TRTS_PP_CAT(x, y) BOOST_CLBL_TRTS_PP_CAT_(x, y)

#define BOOST_CLBL_TRTS_DEFINE_SFINAE_ERROR_ORIGIN(origin) \
namespace error {                                          \
    template<typename ErrorMessage>                        \
    struct origin :                                        \
        ::boost::callable_traits::detail::sfinae_error     \
        { struct _ {}; };                                  \
}                                                          \
/**/

#define BOOST_CLBL_TRTS_SFINAE_MSG(origin, name) \
struct BOOST_CLBL_TRTS_PP_CAT(name, _ ){};       \
struct name : error::origin<                     \
    BOOST_CLBL_TRTS_PP_CAT(name, _ )>{};         \
/**/

namespace boost { namespace callable_traits {

    BOOST_CLBL_TRTS_DEFINE_SFINAE_ERROR_ORIGIN(parameters)
    BOOST_CLBL_TRTS_SFINAE_MSG(parameters, index_out_of_range_for_parameter_list)
    BOOST_CLBL_TRTS_SFINAE_MSG(parameters, cannot_determine_parameters_for_this_type)

    BOOST_CLBL_TRTS_DEFINE_SFINAE_ERROR_ORIGIN(varargs)
    BOOST_CLBL_TRTS_SFINAE_MSG(varargs, varargs_are_illegal_for_this_type)

    BOOST_CLBL_TRTS_DEFINE_SFINAE_ERROR_ORIGIN(member_qualifiers)
    BOOST_CLBL_TRTS_SFINAE_MSG(member_qualifiers, member_qualifiers_are_illegal_for_this_type)
    BOOST_CLBL_TRTS_SFINAE_MSG(member_qualifiers, this_compiler_doesnt_support_abominable_function_types)

    BOOST_CLBL_TRTS_DEFINE_SFINAE_ERROR_ORIGIN(transaction_safe_)
    BOOST_CLBL_TRTS_SFINAE_MSG(transaction_safe_, transaction_safe_is_not_supported_by_this_configuration)

    BOOST_CLBL_TRTS_DEFINE_SFINAE_ERROR_ORIGIN(expand_args)
    BOOST_CLBL_TRTS_SFINAE_MSG(expand_args, cannot_expand_the_parameter_list_of_first_template_argument)

    BOOST_CLBL_TRTS_DEFINE_SFINAE_ERROR_ORIGIN(member_pointer_required)
    BOOST_CLBL_TRTS_SFINAE_MSG(member_pointer_required, type_is_not_a_member_pointer)

    BOOST_CLBL_TRTS_DEFINE_SFINAE_ERROR_ORIGIN(reference_error)
    BOOST_CLBL_TRTS_SFINAE_MSG(reference_error, reference_type_not_supported_by_this_metafunction)

}} // namespace boost::callable_traits

#endif // #ifndef BOOST_CLBL_TRTS_SFINAE_ERRORS_HPP
