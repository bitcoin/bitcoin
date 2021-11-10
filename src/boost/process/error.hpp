// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_ERROR_HPP
#define BOOST_PROCESS_DETAIL_ERROR_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/traits.hpp>


#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/handler.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/handler.hpp>
#endif

#include <system_error>

#include <type_traits>
#include <boost/fusion/algorithm/query/find_if.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/container/set/convert.hpp>
#include <boost/type_index.hpp>

/** \file boost/process/error.hpp
 *
 *    Header which provides the error properties. It allows to explicitly set the error handling, the properties are:
 *
\xmlonly
<programlisting>
namespace boost {
  namespace process {
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::ignore_error">ignore_error</globalname>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::throw_on_error">throw_on_error</globalname>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::error">error</globalname>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::error_ref">error_ref</globalname>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::error_code">error_code</globalname>;
  }
}
</programlisting>
\endxmlonly
 *     For error there are two aliases: error_ref and error_code
 */

namespace boost { namespace process {

namespace detail {

struct throw_on_error_ : ::boost::process::detail::api::handler_base_ext
{
    constexpr throw_on_error_() = default;

    template <class Executor>
    void on_error(Executor&, const std::error_code & ec) const
    {
        throw process_error(ec, "process creation failed");
    }

    const throw_on_error_ &operator()() const {return *this;}
};

struct ignore_error_ : ::boost::process::detail::api::handler_base_ext
{
    constexpr ignore_error_() = default;
};

struct set_on_error : ::boost::process::detail::api::handler_base_ext
{
    set_on_error(const set_on_error&) = default;
    explicit set_on_error(std::error_code &ec) : ec_(ec) {}

    template <class Executor>
    void on_error(Executor&, const std::error_code & ec) const noexcept
    {
        ec_ = ec;
    }

private:
    std::error_code &ec_;
};

struct error_
{
    constexpr error_() = default;
    set_on_error operator()(std::error_code &ec) const {return set_on_error(ec);}
    set_on_error operator= (std::error_code &ec) const {return set_on_error(ec);}

};


template<typename T>
struct is_error_handler : std::false_type {};

template<> struct is_error_handler<set_on_error>    : std::true_type {};
template<> struct is_error_handler<throw_on_error_> : std::true_type {};
template<> struct is_error_handler<ignore_error_>   : std::true_type {};



template<typename Iterator, typename End>
struct has_error_handler_impl
{
    typedef typename boost::fusion::result_of::deref<Iterator>::type ref_type;
    typedef typename std::remove_reference<ref_type>::type res_type_;
    typedef typename std::remove_cv<res_type_>::type res_type;
    typedef typename is_error_handler<res_type>::type cond;

    typedef typename boost::fusion::result_of::next<Iterator>::type next_itr;
    typedef typename has_error_handler_impl<next_itr, End>::type next;

    typedef typename boost::mpl::or_<cond, next>::type type;
};

template<typename Iterator>
struct has_error_handler_impl<Iterator, Iterator>
{
    typedef boost::mpl::false_ type;
};


template<typename Sequence>
struct has_error_handler
{
    typedef typename boost::fusion::result_of::as_vector<Sequence>::type vector_type;

    typedef typename has_error_handler_impl<
            typename boost::fusion::result_of::begin<vector_type>::type,
            typename boost::fusion::result_of::end<  vector_type>::type
            >::type type;
};

template<typename Sequence>
struct has_ignore_error
{
    typedef typename boost::fusion::result_of::as_set<Sequence>::type set_type;
    typedef typename boost::fusion::result_of::has_key<set_type, ignore_error_>::type  type1;
    typedef typename boost::fusion::result_of::has_key<set_type, ignore_error_&>::type type2;
    typedef typename boost::fusion::result_of::has_key<set_type, const ignore_error_&>::type type3;
    typedef typename boost::mpl::or_<type1,type2, type3>::type type;
};

struct error_builder
{
    std::error_code *err;
    typedef set_on_error result_type;
    set_on_error get_initializer() {return set_on_error(*err);};
    void operator()(std::error_code & ec) {err = &ec;};
};

template<>
struct initializer_tag<std::error_code>
{
    typedef error_tag type;
};


template<>
struct initializer_builder<error_tag>
{
    typedef error_builder type;
};

}
/**The ignore_error property will disable any error handling. This can be useful
on linux, where error handling will require a pipe.*/
constexpr boost::process::detail::ignore_error_ ignore_error;
/**The throw_on_error property will enable the exception when launching a process.
It is unnecessary by default, but may be used, when an additional error_code is provided.*/
constexpr boost::process::detail::throw_on_error_ throw_on_error;
/**
The error property will set the executor to handle any errors by setting an
[std::error_code](http://en.cppreference.com/w/cpp/error/error_code).

\code{.cpp}
std::error_code ec;
system("gcc", error(ec));
\endcode

The following syntax is valid:

\code{.cpp}
error(ec);
error=ec;
\endcode

The overload version is achieved by just passing an object of
 [std::error_code](http://en.cppreference.com/w/cpp/error/error_code) to the function.


 */
constexpr boost::process::detail::error_ error;
///Alias for \xmlonly <globalname alt="boost::process::error">error</globalname> \endxmlonly .
constexpr boost::process::detail::error_ error_ref;
///Alias for \xmlonly <globalname alt="boost::process::error">error</globalname> \endxmlonly .
constexpr boost::process::detail::error_ error_code;


}}

#endif
