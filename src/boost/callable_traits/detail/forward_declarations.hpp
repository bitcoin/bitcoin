#ifndef BOOST_CLBL_TRTS_DETAIL_FORWARD_DECLARATIONS
#define BOOST_CLBL_TRTS_DETAIL_FORWARD_DECLARATIONS
#include <boost/callable_traits/detail/config.hpp>
#include <boost/callable_traits/detail/default_callable_traits.hpp>

namespace boost { namespace callable_traits { namespace detail {

template<typename T>
struct function;

template<typename T>
struct has_normal_call_operator
{
    template<typename N, N Value>
    struct check { check(std::nullptr_t) {} };

    template<typename U>
    static std::int8_t test(
        check<decltype(&U::operator()), &U::operator()>);

    template<typename>
    static std::int16_t test(...);

    static constexpr bool value =
        sizeof(test<T>(nullptr)) == sizeof(std::int8_t);
};

struct callable_dummy {
    void operator()() {}
};

template<typename T>
using default_to_function_object = typename std::conditional<
    has_normal_call_operator<T>::value,
    T, callable_dummy>::type;

template<typename T>
struct pmf;

template<typename T>
struct pmd;

template<typename F, typename T = typename std::remove_reference<F>::type>
using function_object_base = typename std::conditional<
    has_normal_call_operator<T>::value,
    pmf<decltype(&default_to_function_object<T>::operator())>,
    default_callable_traits<T>>::type;

template<typename T, typename Base = function_object_base<T>>
struct function_object;

}}} // namespace boost::callable_traits::detail

#endif // #ifndef BOOST_CLBL_TRTS_DETAIL_FORWARD_DECLARATIONS
