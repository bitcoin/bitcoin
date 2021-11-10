// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/**
 * \file boost/process/execute.hpp
 *
 * Defines a function to execute a program.
 */

#ifndef BOOST_PROCESS_EXECUTE_HPP
#define BOOST_PROCESS_EXECUTE_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/traits.hpp>

#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/executor.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/executor.hpp>
#endif

#include <boost/process/detail/basic_cmd.hpp>
#include <boost/process/detail/handler.hpp>

#include <boost/fusion/view.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/fusion/algorithm/transformation/filter_if.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/container/vector/convert.hpp>

#include <type_traits>
#include <utility>

namespace boost { namespace process {

class child;

namespace detail {


template<typename ...Args>
struct has_wchar;

template<typename First, typename ...Args>
struct has_wchar<First, Args...>
{
    typedef has_wchar<Args...> next;
    typedef typename std::remove_cv<
                typename std::remove_reference<First>::type>::type res_type;

    constexpr static bool my_value = is_wchar_t<res_type>::value;
    constexpr static bool value = my_value || next::value;

    typedef std::integral_constant<bool, value> type;
};

template<typename First>
struct has_wchar<First>
{
    typedef typename std::remove_cv<
                typename std::remove_reference<First>::type>::type res_type;

    constexpr static bool value = is_wchar_t<res_type>::value;

    typedef std::integral_constant<bool, value> type;
};


#if defined(BOOST_WINDOWS_API)
//everything needs to be wchar_t
#if defined(BOOST_NO_ANSI_APIS)
template<bool has_wchar>
struct required_char_type
{
    typedef wchar_t type;
};
#else
template<bool has_wchar> struct required_char_type;
template<> struct required_char_type<true>
{
    typedef wchar_t type;
};
template<> struct required_char_type<false>
{
    typedef char type;
};
#endif

#elif defined(BOOST_POSIX_API)
template<bool has_wchar>
struct required_char_type
{
    typedef char type;
};
#endif

template<typename ... Args>
using required_char_type_t = typename required_char_type<
                    has_wchar<Args...>::value>::type;


template<typename Iterator, typename End, typename ...Args>
struct make_builders_from_view
{
    typedef boost::fusion::set<Args...> set;
    typedef typename boost::fusion::result_of::deref<Iterator>::type ref_type;
    typedef typename std::remove_reference<ref_type>::type res_type;
    typedef typename initializer_tag<res_type>::type tag;
    typedef typename initializer_builder<tag>::type builder_type;
    typedef typename boost::fusion::result_of::has_key<set, builder_type> has_key;

    typedef typename boost::fusion::result_of::next<Iterator>::type next_itr;
    typedef typename make_builders_from_view<next_itr, End>::type next;

    typedef typename
            std::conditional<has_key::value,
                typename make_builders_from_view<next_itr, End, Args...>::type,
                typename make_builders_from_view<next_itr, End, Args..., builder_type>::type
            >::type type;

};

template<typename Iterator, typename ...Args>
struct make_builders_from_view<Iterator, Iterator, Args...>
{
    typedef boost::fusion::set<Args...> type;
};

template<typename Builders>
struct builder_ref
{
    Builders &builders;
    builder_ref(Builders & builders) : builders(builders) {};

    template<typename T>
    void operator()(T && value) const
    {
        typedef typename initializer_tag<typename std::remove_reference<T>::type>::type tag;
        typedef typename initializer_builder<tag>::type builder_type;
        boost::fusion::at_key<builder_type>(builders)(std::forward<T>(value));
    }
};

template<typename T>
struct get_initializers_result
{
    typedef typename T::result_type type;
};

template<>
struct get_initializers_result<boost::fusion::void_>
{
    typedef boost::fusion::void_ type;
};

template<typename ...Args>
struct helper_vector
{

};

template<typename T, typename ...Stack>
struct invoke_get_initializer_collect_keys;

template<typename ...Stack>
struct invoke_get_initializer_collect_keys<boost::fusion::vector<>, Stack...>
{
    typedef helper_vector<Stack...> type;
};


template<typename First, typename ...Args, typename ...Stack>
struct invoke_get_initializer_collect_keys<boost::fusion::vector<First, Args...>, Stack...>
{
    typedef typename invoke_get_initializer_collect_keys<boost::fusion::vector<Args...>, Stack..., First>::type next;
    typedef helper_vector<Stack...> stack_t;

    typedef typename std::conditional<std::is_same<boost::fusion::void_, First>::value,
            stack_t, next>::type type;


};


template<typename Keys>
struct invoke_get_initializer;

template<typename ...Args>
struct invoke_get_initializer<helper_vector<Args...>>

{
    typedef boost::fusion::tuple<typename get_initializers_result<Args>::type...> result_type;

    template<typename Sequence>
    static result_type call(Sequence & seq)
    {
        return result_type(boost::fusion::at_key<Args>(seq).get_initializer()...);;
    }
};





template<typename ...Args>
inline boost::fusion::tuple<typename get_initializers_result<Args>::type...>
        get_initializers(boost::fusion::set<Args...> & builders)
{
    //typedef boost::fusion::tuple<typename get_initializers_result<Args>::type...> return_type;
    typedef typename invoke_get_initializer_collect_keys<boost::fusion::vector<Args...>>::type keys;
    return invoke_get_initializer<keys>::call(builders);
}


template<typename Char, typename ... Args>
inline child basic_execute_impl(Args && ... args)
{
    //create a tuple from the argument list
    boost::fusion::tuple<typename std::remove_reference<Args>::type&...> tup(args...);

    auto inits = boost::fusion::filter_if<
                boost::process::detail::is_initializer<
                    typename std::remove_reference<
                        boost::mpl::_
                        >::type
                    >
                >(tup);

    auto others = boost::fusion::filter_if<
                boost::mpl::not_<
                    boost::process::detail::is_initializer<
                     typename std::remove_reference<
                            boost::mpl::_
                            >::type
                        >
                    >
                >(tup);

   // typename detail::make_builders_from_view<decltype(others)>::type builders;

    //typedef typename boost::fusion::result_of::as_vector<decltype(inits)>::type  inits_t;
    typedef typename boost::fusion::result_of::as_vector<decltype(others)>::type others_t;
    //  typedef decltype(others) others_t;
    typedef typename ::boost::process::detail::make_builders_from_view<
            typename boost::fusion::result_of::begin<others_t>::type,
            typename boost::fusion::result_of::end  <others_t>::type>::type builder_t;

    builder_t builders;
    ::boost::process::detail::builder_ref<builder_t> builder_ref(builders);

    boost::fusion::for_each(others, builder_ref);
    auto other_inits = ::boost::process::detail::get_initializers(builders);


    boost::fusion::joint_view<decltype(other_inits), decltype(inits)> complete_inits(other_inits, inits);

    auto exec = boost::process::detail::api::make_executor<Char>(complete_inits);
    return exec();
}

template<typename ...Args>
inline child execute_impl(Args&& ... args)
{
    typedef required_char_type_t<Args...> req_char_type;

    return basic_execute_impl<req_char_type>(
        boost::process::detail::char_converter_t<req_char_type, Args>::conv(
                std::forward<Args>(args))...
            );
}

}}}


#endif
