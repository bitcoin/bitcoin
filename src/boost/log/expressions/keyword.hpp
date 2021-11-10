/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keyword.hpp
 * \author Andrey Semashev
 * \date   29.01.2012
 *
 * The header contains attribute keyword declaration.
 */

#ifndef BOOST_LOG_EXPRESSIONS_KEYWORD_HPP_INCLUDED_
#define BOOST_LOG_EXPRESSIONS_KEYWORD_HPP_INCLUDED_

#include <boost/ref.hpp>
#include <boost/proto/extends.hpp>
#include <boost/proto/make_expr.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/domain.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/custom_terminal_spec.hpp>
#include <boost/log/expressions/keyword_fwd.hpp>
#include <boost/log/expressions/is_keyword_descriptor.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/attributes/fallback_policy.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

/*!
 * This class implements an expression template keyword. It is used to start template expressions involving attribute values.
 */
template< typename DescriptorT, template< typename > class ActorT >
struct attribute_keyword
{
    //! Self type
    typedef attribute_keyword this_type;
    //! Attribute descriptor type
    typedef DescriptorT descriptor_type;

    BOOST_PROTO_BASIC_EXTENDS(typename proto::terminal< descriptor_type >::type, this_type, phoenix::phoenix_domain)

    //! Attribute value type
    typedef typename descriptor_type::value_type value_type;

    //! Returns attribute name
    static attribute_name get_name() { return descriptor_type::get_name(); }

    //! Expression with cached attribute name
    typedef attribute_actor<
        value_type,
        fallback_to_none,
        descriptor_type,
        ActorT
    > or_none_result_type;

    //! Generates an expression that extracts the attribute value or a default value
    static or_none_result_type or_none()
    {
        typedef typename or_none_result_type::terminal_type result_terminal;
        typename or_none_result_type::base_type act = {{ result_terminal(get_name()) }};
        return or_none_result_type(act);
    }

    //! Expression with cached attribute name
    typedef attribute_actor<
        value_type,
        fallback_to_throw,
        descriptor_type,
        ActorT
    > or_throw_result_type;

    //! Generates an expression that extracts the attribute value or throws an exception
    static or_throw_result_type or_throw()
    {
        typedef typename or_throw_result_type::terminal_type result_terminal;
        typename or_throw_result_type::base_type act = {{ result_terminal(get_name()) }};
        return or_throw_result_type(act);
    }

    //! Generates an expression that extracts the attribute value or a default value
    template< typename DefaultT >
    static attribute_actor<
        value_type,
        fallback_to_default< DefaultT >,
        descriptor_type,
        ActorT
    > or_default(DefaultT const& def_val)
    {
        typedef attribute_actor<
            value_type,
            fallback_to_default< DefaultT >,
            descriptor_type,
            ActorT
        > or_default_result_type;
        typedef typename or_default_result_type::terminal_type result_terminal;
        typename or_default_result_type::base_type act = {{ result_terminal(get_name(), def_val) }};
        return or_default_result_type(act);
    }
};

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace proto {

namespace detail {

// This hack is needed in order to cache attribute name into the expression terminal when the template
// expression is constructed. The standard way through a custom domain doesn't work because phoenix::actor
// is bound to phoenix_domain.
template< typename DescriptorT, template< typename > class ActorT, typename DomainT >
struct protoify< boost::log::expressions::attribute_keyword< DescriptorT, ActorT >, DomainT >
{
    typedef boost::log::expressions::attribute_keyword< DescriptorT, ActorT > keyword_type;
    typedef typename keyword_type::or_none_result_type result_type;

    result_type operator() (keyword_type const& keyword) const
    {
        return keyword.or_none();
    }
};

template< typename DescriptorT, template< typename > class ActorT, typename DomainT >
struct protoify< boost::log::expressions::attribute_keyword< DescriptorT, ActorT >&, DomainT > :
    public protoify< boost::log::expressions::attribute_keyword< DescriptorT, ActorT >, DomainT >
{
};

template< typename DescriptorT, template< typename > class ActorT, typename DomainT >
struct protoify< boost::log::expressions::attribute_keyword< DescriptorT, ActorT > const&, DomainT > :
    public protoify< boost::log::expressions::attribute_keyword< DescriptorT, ActorT >, DomainT >
{
};

template< typename DescriptorT, template< typename > class ActorT, typename DomainT >
struct protoify< boost::reference_wrapper< boost::log::expressions::attribute_keyword< DescriptorT, ActorT > >, DomainT > :
    public protoify< boost::log::expressions::attribute_keyword< DescriptorT, ActorT >, DomainT >
{
};

template< typename DescriptorT, template< typename > class ActorT, typename DomainT >
struct protoify< boost::reference_wrapper< boost::log::expressions::attribute_keyword< DescriptorT, ActorT > > const, DomainT > :
    public protoify< boost::log::expressions::attribute_keyword< DescriptorT, ActorT >, DomainT >
{
};

template< typename DescriptorT, template< typename > class ActorT, typename DomainT >
struct protoify< boost::reference_wrapper< const boost::log::expressions::attribute_keyword< DescriptorT, ActorT > >, DomainT > :
    public protoify< boost::log::expressions::attribute_keyword< DescriptorT, ActorT >, DomainT >
{
};

template< typename DescriptorT, template< typename > class ActorT, typename DomainT >
struct protoify< boost::reference_wrapper< const boost::log::expressions::attribute_keyword< DescriptorT, ActorT > > const, DomainT > :
    public protoify< boost::log::expressions::attribute_keyword< DescriptorT, ActorT >, DomainT >
{
};

} // namespace detail

} // namespace proto

#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

} // namespace boost

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_ATTRIBUTE_KEYWORD_TYPE_IMPL(keyword_, name_, value_type_, tag_ns_)\
    namespace tag_ns_\
    {\
        struct keyword_ :\
            public ::boost::log::expressions::keyword_descriptor\
        {\
            typedef value_type_ value_type;\
            static ::boost::log::attribute_name get_name() { return ::boost::log::attribute_name(name_); }\
        };\
    }\
    typedef ::boost::log::expressions::attribute_keyword< tag_ns_::keyword_ > BOOST_PP_CAT(keyword_, _type);

#define BOOST_LOG_ATTRIBUTE_KEYWORD_IMPL(keyword_, name_, value_type_, tag_ns_)\
    BOOST_LOG_ATTRIBUTE_KEYWORD_TYPE_IMPL(keyword_, name_, value_type_, tag_ns_)\
    BOOST_INLINE_VARIABLE const BOOST_PP_CAT(keyword_, _type) keyword_ = {};

#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief The macro declares an attribute keyword type
 *
 * The macro should be used at a namespace scope. It expands into an attribute keyword type definition, including the
 * \c tag namespace and the keyword tag type within which has the following layout:
 *
 * \code
 * namespace tag
 * {
 *   struct keyword_ :
 *     public boost::log::expressions::keyword_descriptor
 *   {
 *     typedef value_type_ value_type;
 *     static boost::log::attribute_name get_name();
 *   };
 * }
 *
 * typedef boost::log::expressions::attribute_keyword< tag::keyword_ > keyword_type;
 * \endcode
 *
 * The \c get_name method returns the attribute name.
 *
 * \note This macro only defines the type of the keyword. To also define the keyword object, use
 *       the \c BOOST_LOG_ATTRIBUTE_KEYWORD macro instead.
 *
 * \param keyword_ Keyword name
 * \param name_ Attribute name string
 * \param value_type_ Attribute value type
 */
#define BOOST_LOG_ATTRIBUTE_KEYWORD_TYPE(keyword_, name_, value_type_)\
    BOOST_LOG_ATTRIBUTE_KEYWORD_TYPE_IMPL(keyword_, name_, value_type_, tag)

/*!
 * \brief The macro declares an attribute keyword
 *
 * The macro provides definitions similar to \c BOOST_LOG_ATTRIBUTE_KEYWORD_TYPE and additionally
 * defines the keyword object.
 *
 * \param keyword_ Keyword name
 * \param name_ Attribute name string
 * \param value_type_ Attribute value type
 */
#define BOOST_LOG_ATTRIBUTE_KEYWORD(keyword_, name_, value_type_)\
    BOOST_LOG_ATTRIBUTE_KEYWORD_IMPL(keyword_, name_, value_type_, tag)

#include <boost/log/detail/footer.hpp>

#if defined(BOOST_LOG_TRIVIAL_HPP_INCLUDED_)
#include <boost/log/detail/trivial_keyword.hpp>
#endif

#endif // BOOST_LOG_EXPRESSIONS_KEYWORD_HPP_INCLUDED_
