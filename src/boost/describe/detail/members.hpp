#ifndef BOOST_DESCRIBE_DETAIL_MEMBERS_HPP_INCLUDED
#define BOOST_DESCRIBE_DETAIL_MEMBERS_HPP_INCLUDED

// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/describe/modifiers.hpp>
#include <boost/describe/detail/pp_for_each.hpp>
#include <boost/describe/detail/pp_utilities.hpp>
#include <boost/describe/detail/list.hpp>
#include <type_traits>

namespace boost
{
namespace describe
{
namespace detail
{

template<class Pm> constexpr unsigned add_static_modifier( Pm )
{
    return std::is_member_pointer<Pm>::value? 0: mod_static;
}

template<class Pm> constexpr unsigned add_function_modifier( Pm )
{
    return std::is_member_function_pointer<Pm>::value || std::is_function< std::remove_pointer_t<Pm> >::value? mod_function: 0;
}

template<class D, unsigned M> struct member_descriptor
{
    static constexpr decltype(D::pointer()) pointer = D::pointer();
    static constexpr decltype(D::name()) name = D::name();
    static constexpr unsigned modifiers = M | add_static_modifier( D::pointer() ) | add_function_modifier( D::pointer() );
};

template<class D, unsigned M> constexpr decltype(D::pointer()) member_descriptor<D, M>::pointer;
template<class D, unsigned M> constexpr decltype(D::name()) member_descriptor<D, M>::name;
template<class D, unsigned M> constexpr unsigned member_descriptor<D, M>::modifiers;

template<unsigned M, class... T> auto member_descriptor_fn_impl( int, T... )
{
    return list<member_descriptor<T, M>...>();
}

template<class C, class F> constexpr auto mfn( F C::* p ) { return p; }
template<class C, class F> constexpr auto mfn( F * p ) { return p; }

#define BOOST_DESCRIBE_MEMBER_IMPL(C, m) , []{ struct _boost_desc { \
    static constexpr auto pointer() noexcept { return BOOST_DESCRIBE_PP_POINTER(C, m); } \
    static constexpr auto name() noexcept { return BOOST_DESCRIBE_PP_NAME(m); } }; return _boost_desc(); }()

#if defined(_MSC_VER) && !defined(__clang__)

#define BOOST_DESCRIBE_PUBLIC_MEMBERS(C, ...) inline auto boost_public_member_descriptor_fn( C * ) \
{ return boost::describe::detail::member_descriptor_fn_impl<boost::describe::mod_public>( 0 BOOST_DESCRIBE_PP_FOR_EACH(BOOST_DESCRIBE_MEMBER_IMPL, C, __VA_ARGS__) ); }

#define BOOST_DESCRIBE_PROTECTED_MEMBERS(C, ...) inline auto boost_protected_member_descriptor_fn( C * ) \
{ return boost::describe::detail::member_descriptor_fn_impl<boost::describe::mod_protected>( 0 BOOST_DESCRIBE_PP_FOR_EACH(BOOST_DESCRIBE_MEMBER_IMPL, C, __VA_ARGS__) ); }

#define BOOST_DESCRIBE_PRIVATE_MEMBERS(C, ...) inline auto boost_private_member_descriptor_fn( C * ) \
{ return boost::describe::detail::member_descriptor_fn_impl<boost::describe::mod_private>( 0 BOOST_DESCRIBE_PP_FOR_EACH(BOOST_DESCRIBE_MEMBER_IMPL, C, __VA_ARGS__) ); }

#else

#define BOOST_DESCRIBE_PUBLIC_MEMBERS(C, ...) inline auto boost_public_member_descriptor_fn( C * ) \
{ return boost::describe::detail::member_descriptor_fn_impl<boost::describe::mod_public>( 0 BOOST_DESCRIBE_PP_FOR_EACH(BOOST_DESCRIBE_MEMBER_IMPL, C, ##__VA_ARGS__) ); }

#define BOOST_DESCRIBE_PROTECTED_MEMBERS(C, ...) inline auto boost_protected_member_descriptor_fn( C * ) \
{ return boost::describe::detail::member_descriptor_fn_impl<boost::describe::mod_protected>( 0 BOOST_DESCRIBE_PP_FOR_EACH(BOOST_DESCRIBE_MEMBER_IMPL, C, ##__VA_ARGS__) ); }

#define BOOST_DESCRIBE_PRIVATE_MEMBERS(C, ...) inline auto boost_private_member_descriptor_fn( C * ) \
{ return boost::describe::detail::member_descriptor_fn_impl<boost::describe::mod_private>( 0 BOOST_DESCRIBE_PP_FOR_EACH(BOOST_DESCRIBE_MEMBER_IMPL, C, ##__VA_ARGS__) ); }

#endif

} // namespace detail
} // namespace describe
} // namespace boost

#endif // #ifndef BOOST_DESCRIBE_DETAIL_MEMBERS_HPP_INCLUDED
