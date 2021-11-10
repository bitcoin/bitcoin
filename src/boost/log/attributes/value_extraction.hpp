/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   value_extraction.hpp
 * \author Andrey Semashev
 * \date   01.03.2008
 *
 * The header contains implementation of tools for extracting an attribute value
 * from the view.
 */

#ifndef BOOST_LOG_ATTRIBUTES_VALUE_EXTRACTION_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_VALUE_EXTRACTION_HPP_INCLUDED_

#include <boost/mpl/vector.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/attributes/value_extraction_fwd.hpp>
#include <boost/log/attributes/fallback_policy.hpp>
#include <boost/log/expressions/keyword_fwd.hpp>
#include <boost/log/utility/value_ref.hpp>
#include <boost/log/utility/type_dispatch/static_type_dispatcher.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace result_of {

/*!
 * \brief A metafunction that allows to acquire the result of the value extraction
 *
 * The metafunction results in a type that is in form of <tt>T const&</tt>, if \c T is
 * not an MPL type sequence and <tt>DefaultT</tt> is the same as <tt>T</tt>,
 * or <tt>value_ref< TypesT, TagT ></tt> otherwise, with
 * \c TypesT being a type sequence comprising the types from sequence \c T and \c DefaultT,
 * if it is not present in \c T already.
 */
template< typename T, typename DefaultT, typename TagT >
struct extract_or_default
{
    typedef typename mpl::eval_if<
        mpl::is_sequence< T >,
        mpl::eval_if<
            mpl::contains< T, DefaultT >,
            mpl::identity< T >,
            mpl::push_back< T, DefaultT >
        >,
        mpl::if_<
            is_same< T, DefaultT >,
            T,
            mpl::vector2< T, DefaultT >
        >
    >::type extracted_type;

    typedef typename mpl::if_<
        mpl::is_sequence< extracted_type >,
        value_ref< extracted_type, TagT >,
        extracted_type const&
    >::type type;
};

/*!
 * \brief A metafunction that allows to acquire the result of the value extraction
 *
 * The metafunction results in a type that is in form of <tt>T const&</tt>, if \c T is
 * not an MPL type sequence, or <tt>value_ref< T, TagT ></tt> otherwise. In the latter
 * case the value reference shall never be empty.
 */
template< typename T, typename TagT >
struct extract_or_throw
{
    typedef typename mpl::if_<
        mpl::is_sequence< T >,
        value_ref< T, TagT >,
        T const&
    >::type type;
};

/*!
 * \brief A metafunction that allows to acquire the result of the value extraction
 *
 * The metafunction results in a type that is in form of <tt>value_ref< T, TagT ></tt>.
 */
template< typename T, typename TagT >
struct extract
{
    typedef value_ref< T, TagT > type;
};

} // namespace result_of

namespace aux {

//! The function object initializes the value reference
template< typename RefT >
struct value_ref_initializer
{
    typedef void result_type;

    value_ref_initializer(RefT& ref) : m_ref(ref)
    {
    }

    template< typename ArgT >
    result_type operator() (ArgT const& arg) const
    {
        m_ref = RefT(arg);
    }

private:
    RefT& m_ref;
};

//! The function unwraps \c value_ref, if possible
template< typename T, typename TagT >
BOOST_FORCEINLINE typename boost::enable_if_c< mpl::is_sequence< T >::value, value_ref< T, TagT > >::type
unwrap_value_ref(value_ref< T, TagT > const& r)
{
    return r;
}

template< typename T, typename TagT >
BOOST_FORCEINLINE typename boost::disable_if_c< mpl::is_sequence< T >::value, T const& >::type
unwrap_value_ref(value_ref< T, TagT > const& r)
{
    return r.get();
}

} // namespace aux

/*!
 * \brief Generic attribute value extractor
 *
 * Attribute value extractor is a functional object that attempts to find and extract the stored
 * attribute value from the attribute values view or a log record. The extracted value is returned
 * from the extractor.
 */
template< typename T, typename FallbackPolicyT, typename TagT >
class value_extractor :
    private FallbackPolicyT
{
public:
    //! Fallback policy
    typedef FallbackPolicyT fallback_policy;
    //! Attribute value types
    typedef T value_type;
    //! Function object result type
    typedef value_ref< value_type, TagT > result_type;

public:
    /*!
     * Default constructor
     */
    BOOST_DEFAULTED_FUNCTION(value_extractor(), {})

    /*!
     * Copy constructor
     */
    value_extractor(value_extractor const& that) : fallback_policy(static_cast< fallback_policy const& >(that))
    {
    }

    /*!
     * Constructor
     *
     * \param arg Fallback policy constructor argument
     */
    template< typename U >
    explicit value_extractor(U const& arg) : fallback_policy(arg) {}

    /*!
     * Extraction operator. Attempts to acquire the stored value of one of the supported types. If extraction succeeds,
     * the extracted value is returned.
     *
     * \param attr The attribute value to extract from.
     * \return The extracted value, if extraction succeeded, an empty value otherwise.
     */
    result_type operator() (attribute_value const& attr) const
    {
        result_type res;
        aux::value_ref_initializer< result_type > initializer(res);
        if (!!attr)
        {
            static_type_dispatcher< value_type > disp(initializer);
            if (!attr.dispatch(disp) && !fallback_policy::apply_default(initializer))
                fallback_policy::on_invalid_type(attr.get_type());
        }
        else if (!fallback_policy::apply_default(initializer))
        {
            fallback_policy::on_missing_value();
        }
        return res;
    }

    /*!
     * Extraction operator. Looks for an attribute value with the specified name
     * and tries to acquire the stored value of one of the supported types. If extraction succeeds,
     * the extracted value is returned.
     *
     * \param name Attribute value name.
     * \param attrs A set of attribute values in which to look for the specified attribute value.
     * \return The extracted value, if extraction succeeded, an empty value otherwise.
     */
    result_type operator() (attribute_name const& name, attribute_value_set const& attrs) const
    {
        try
        {
            attribute_value_set::const_iterator it = attrs.find(name);
            if (it != attrs.end())
                return operator() (it->second);
            else
                return operator() (attribute_value());
        }
        catch (exception& e)
        {
            // Attach the attribute name to the exception
            boost::log::aux::attach_attribute_name_info(e, name);
            throw;
        }
    }

    /*!
     * Extraction operator. Looks for an attribute value with the specified name
     * and tries to acquire the stored value of one of the supported types. If extraction succeeds,
     * the extracted value is returned.
     *
     * \param name Attribute value name.
     * \param rec A log record. The attribute value will be sought among those associated with the record.
     * \return The extracted value, if extraction succeeded, an empty value otherwise.
     */
    result_type operator() (attribute_name const& name, record const& rec) const
    {
        return operator() (name, rec.attribute_values());
    }

    /*!
     * Extraction operator. Looks for an attribute value with the specified name
     * and tries to acquire the stored value of one of the supported types. If extraction succeeds,
     * the extracted value is returned.
     *
     * \param name Attribute value name.
     * \param rec A log record view. The attribute value will be sought among those associated with the record.
     * \return The extracted value, if extraction succeeded, an empty value otherwise.
     */
    result_type operator() (attribute_name const& name, record_view const& rec) const
    {
        return operator() (name, rec.attribute_values());
    }

    /*!
     * \returns Fallback policy
     */
    fallback_policy const& get_fallback_policy() const
    {
        return *static_cast< fallback_policy const* >(this);
    }
};

#if !defined(BOOST_LOG_DOXYGEN_PASS)
#if !defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)
#define BOOST_LOG_AUX_VOID_DEFAULT = void
#else
#define BOOST_LOG_AUX_VOID_DEFAULT
#endif
#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param name The name of the attribute value to extract.
 * \param attrs A set of attribute values in which to look for the specified attribute value.
 * \return A \c value_ref that refers to the extracted value, if found. An empty value otherwise.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
inline typename result_of::extract< T, TagT >::type extract(attribute_name const& name, attribute_value_set const& attrs)
{
    value_extractor< T, fallback_to_none, TagT > extractor;
    return extractor(name, attrs);
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param name The name of the attribute value to extract.
 * \param rec A log record. The attribute value will be sought among those associated with the record.
 * \return A \c value_ref that refers to the extracted value, if found. An empty value otherwise.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
inline typename result_of::extract< T, TagT >::type extract(attribute_name const& name, record const& rec)
{
    value_extractor< T, fallback_to_none, TagT > extractor;
    return extractor(name, rec);
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param name The name of the attribute value to extract.
 * \param rec A log record view. The attribute value will be sought among those associated with the record.
 * \return A \c value_ref that refers to the extracted value, if found. An empty value otherwise.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
inline typename result_of::extract< T, TagT >::type extract(attribute_name const& name, record_view const& rec)
{
    value_extractor< T, fallback_to_none, TagT > extractor;
    return extractor(name, rec);
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param value Attribute value.
 * \return A \c value_ref that refers to the extracted value, if found. An empty value otherwise.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
inline typename result_of::extract< T, TagT >::type extract(attribute_value const& value)
{
    value_extractor< T, fallback_to_none, TagT > extractor;
    return extractor(value);
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param name The name of the attribute value to extract.
 * \param attrs A set of attribute values in which to look for the specified attribute value.
 * \return The extracted value or a non-empty \c value_ref that refers to the value.
 * \throws An exception is thrown if the requested value cannot be extracted.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
inline typename result_of::extract_or_throw< T, TagT >::type extract_or_throw(attribute_name const& name, attribute_value_set const& attrs)
{
    value_extractor< T, fallback_to_throw, TagT > extractor;
    return aux::unwrap_value_ref(extractor(name, attrs));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param name The name of the attribute value to extract.
 * \param rec A log record. The attribute value will be sought among those associated with the record.
 * \return The extracted value or a non-empty \c value_ref that refers to the value.
 * \throws An exception is thrown if the requested value cannot be extracted.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
inline typename result_of::extract_or_throw< T, TagT >::type extract_or_throw(attribute_name const& name, record const& rec)
{
    value_extractor< T, fallback_to_throw, TagT > extractor;
    return aux::unwrap_value_ref(extractor(name, rec));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param name The name of the attribute value to extract.
 * \param rec A log record view. The attribute value will be sought among those associated with the record.
 * \return The extracted value or a non-empty \c value_ref that refers to the value.
 * \throws An exception is thrown if the requested value cannot be extracted.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
inline typename result_of::extract_or_throw< T, TagT >::type extract_or_throw(attribute_name const& name, record_view const& rec)
{
    value_extractor< T, fallback_to_throw, TagT > extractor;
    return aux::unwrap_value_ref(extractor(name, rec));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param value Attribute value.
 * \return The extracted value or a non-empty \c value_ref that refers to the value.
 * \throws An exception is thrown if the requested value cannot be extracted.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
inline typename result_of::extract_or_throw< T, TagT >::type extract_or_throw(attribute_value const& value)
{
    value_extractor< T, fallback_to_throw, TagT > extractor;
    return aux::unwrap_value_ref(extractor(value));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \note Caution must be exercised if the default value is a temporary object. Because the function returns
 *       a reference, if the temporary object is destroyed, the reference may become dangling.
 *
 * \param name The name of the attribute value to extract.
 * \param attrs A set of attribute values in which to look for the specified attribute value.
 * \param def_val The default value
 * \return The extracted value, if found. The default value otherwise.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT, TagT >::type
extract_or_default(attribute_name const& name, attribute_value_set const& attrs, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< T, DefaultT, TagT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& >, TagT > extractor(def_val);
    return aux::unwrap_value_ref(extractor(name, attrs));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be visited.
 *
 * \note Caution must be exercised if the default value is a temporary object. Because the function returns
 *       a reference, if the temporary object is destroyed, the reference may become dangling.
 *
 * \param name The name of the attribute value to extract.
 * \param rec A log record. The attribute value will be sought among those associated with the record.
 * \param def_val The default value
 * \return The extracted value, if found. The default value otherwise.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT, TagT >::type
extract_or_default(attribute_name const& name, record const& rec, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< T, DefaultT, TagT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& >, TagT > extractor(def_val);
    return aux::unwrap_value_ref(extractor(name, rec));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be visited.
 *
 * \note Caution must be exercised if the default value is a temporary object. Because the function returns
 *       a reference, if the temporary object is destroyed, the reference may become dangling.
 *
 * \param name The name of the attribute value to extract.
 * \param rec A log record view. The attribute value will be sought among those associated with the record.
 * \param def_val The default value
 * \return The extracted value, if found. The default value otherwise.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT, TagT >::type
extract_or_default(attribute_name const& name, record_view const& rec, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< T, DefaultT, TagT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& >, TagT > extractor(def_val);
    return aux::unwrap_value_ref(extractor(name, rec));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be visited.
 *
 * \note Caution must be exercised if the default value is a temporary object. Because the function returns
 *       a reference, if the temporary object is destroyed, the reference may become dangling.
 *
 * \param value Attribute value.
 * \param def_val The default value
 * \return The extracted value, if found. The default value otherwise.
 */
template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT, TagT >::type extract_or_default(attribute_value const& value, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< T, DefaultT, TagT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& >, TagT > extractor(def_val);
    return aux::unwrap_value_ref(extractor(value));
}

#if defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)

template< typename T >
inline typename result_of::extract< T >::type extract(attribute_name const& name, attribute_value_set const& attrs)
{
    value_extractor< T, fallback_to_none > extractor;
    return extractor(name, attrs);
}

template< typename T >
inline typename result_of::extract< T >::type extract(attribute_name const& name, record const& rec)
{
    value_extractor< T, fallback_to_none > extractor;
    return extractor(name, rec);
}

template< typename T >
inline typename result_of::extract< T >::type extract(attribute_name const& name, record_view const& rec)
{
    value_extractor< T, fallback_to_none > extractor;
    return extractor(name, rec);
}

template< typename T >
inline typename result_of::extract< T >::type extract(attribute_value const& value)
{
    value_extractor< T, fallback_to_none > extractor;
    return extractor(value);
}

template< typename T >
inline typename result_of::extract_or_throw< T >::type extract_or_throw(attribute_name const& name, attribute_value_set const& attrs)
{
    value_extractor< T, fallback_to_throw > extractor;
    return aux::unwrap_value_ref(extractor(name, attrs));
}

template< typename T >
inline typename result_of::extract_or_throw< T >::type extract_or_throw(attribute_name const& name, record const& rec)
{
    value_extractor< T, fallback_to_throw > extractor;
    return aux::unwrap_value_ref(extractor(name, rec));
}

template< typename T >
inline typename result_of::extract_or_throw< T >::type extract_or_throw(attribute_name const& name, record_view const& rec)
{
    value_extractor< T, fallback_to_throw > extractor;
    return aux::unwrap_value_ref(extractor(name, rec));
}

template< typename T >
inline typename result_of::extract_or_throw< T >::type extract_or_throw(attribute_value const& value)
{
    value_extractor< T, fallback_to_throw > extractor;
    return aux::unwrap_value_ref(extractor(value));
}

template< typename T, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT >::type extract_or_default(
    attribute_name const& name, attribute_value_set const& attrs, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< T, DefaultT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& > > extractor(def_val);
    return aux::unwrap_value_ref(extractor(name, attrs));
}

template< typename T, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT >::type extract_or_default(
    attribute_name const& name, record const& rec, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< T, DefaultT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& > > extractor(def_val);
    return aux::unwrap_value_ref(extractor(name, rec));
}

template< typename T, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT >::type extract_or_default(
    attribute_name const& name, record_view const& rec, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< T, DefaultT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& > > extractor(def_val);
    return aux::unwrap_value_ref(extractor(name, rec));
}

template< typename T, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT >::type extract_or_default(attribute_value const& value, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< T, DefaultT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& > > extractor(def_val);
    return aux::unwrap_value_ref(extractor(value));
}

#endif // defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param keyword The keyword of the attribute value to extract.
 * \param attrs A set of attribute values in which to look for the specified attribute value.
 * \return A \c value_ref that refers to the extracted value, if found. An empty value otherwise.
 */
template< typename DescriptorT, template< typename > class ActorT >
inline typename result_of::extract< typename DescriptorT::value_type, DescriptorT >::type
extract(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword, attribute_value_set const& attrs)
{
    value_extractor< typename DescriptorT::value_type, fallback_to_none, DescriptorT > extractor;
    return extractor(keyword.get_name(), attrs);
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param keyword The keyword of the attribute value to extract.
 * \param rec A log record. The attribute value will be sought among those associated with the record.
 * \return A \c value_ref that refers to the extracted value, if found. An empty value otherwise.
 */
template< typename DescriptorT, template< typename > class ActorT >
inline typename result_of::extract< typename DescriptorT::value_type, DescriptorT >::type
extract(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword, record const& rec)
{
    value_extractor< typename DescriptorT::value_type, fallback_to_none, DescriptorT > extractor;
    return extractor(keyword.get_name(), rec);
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param keyword The keyword of the attribute value to extract.
 * \param rec A log record view. The attribute value will be sought among those associated with the record.
 * \return A \c value_ref that refers to the extracted value, if found. An empty value otherwise.
 */
template< typename DescriptorT, template< typename > class ActorT >
inline typename result_of::extract< typename DescriptorT::value_type, DescriptorT >::type
extract(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword, record_view const& rec)
{
    value_extractor< typename DescriptorT::value_type, fallback_to_none, DescriptorT > extractor;
    return extractor(keyword.get_name(), rec);
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param keyword The keyword of the attribute value to extract.
 * \param attrs A set of attribute values in which to look for the specified attribute value.
 * \return The extracted value or a non-empty \c value_ref that refers to the value.
 * \throws An exception is thrown if the requested value cannot be extracted.
 */
template< typename DescriptorT, template< typename > class ActorT >
inline typename result_of::extract_or_throw< typename DescriptorT::value_type, DescriptorT >::type
extract_or_throw(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword, attribute_value_set const& attrs)
{
    value_extractor< typename DescriptorT::value_type, fallback_to_throw, DescriptorT > extractor;
    return aux::unwrap_value_ref(extractor(keyword.get_name(), attrs));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param keyword The keyword of the attribute value to extract.
 * \param rec A log record. The attribute value will be sought among those associated with the record.
 * \return The extracted value or a non-empty \c value_ref that refers to the value.
 * \throws An exception is thrown if the requested value cannot be extracted.
 */
template< typename DescriptorT, template< typename > class ActorT >
inline typename result_of::extract_or_throw< typename DescriptorT::value_type, DescriptorT >::type
extract_or_throw(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword, record const& rec)
{
    value_extractor< typename DescriptorT::value_type, fallback_to_throw, DescriptorT > extractor;
    return aux::unwrap_value_ref(extractor(keyword.get_name(), rec));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param keyword The keyword of the attribute value to extract.
 * \param rec A log record view. The attribute value will be sought among those associated with the record.
 * \return The extracted value or a non-empty \c value_ref that refers to the value.
 * \throws An exception is thrown if the requested value cannot be extracted.
 */
template< typename DescriptorT, template< typename > class ActorT >
inline typename result_of::extract_or_throw< typename DescriptorT::value_type, DescriptorT >::type
extract_or_throw(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword, record_view const& rec)
{
    value_extractor< typename DescriptorT::value_type, fallback_to_throw, DescriptorT > extractor;
    return aux::unwrap_value_ref(extractor(keyword.get_name(), rec));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \note Caution must be exercised if the default value is a temporary object. Because the function returns
 *       a reference, if the temporary object is destroyed, the reference may become dangling.
 *
 * \param keyword The keyword of the attribute value to extract.
 * \param attrs A set of attribute values in which to look for the specified attribute value.
 * \param def_val The default value
 * \return The extracted value, if found. The default value otherwise.
 */
template< typename DescriptorT, template< typename > class ActorT, typename DefaultT >
inline typename result_of::extract_or_default< typename DescriptorT::value_type, DefaultT, DescriptorT >::type
extract_or_default(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword, attribute_value_set const& attrs, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< typename DescriptorT::value_type, DefaultT, DescriptorT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& >, DescriptorT > extractor(def_val);
    return aux::unwrap_value_ref(extractor(keyword.get_name(), attrs));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be visited.
 *
 * \note Caution must be exercised if the default value is a temporary object. Because the function returns
 *       a reference, if the temporary object is destroyed, the reference may become dangling.
 *
 * \param keyword The keyword of the attribute value to extract.
 * \param rec A log record. The attribute value will be sought among those associated with the record.
 * \param def_val The default value
 * \return The extracted value, if found. The default value otherwise.
 */
template< typename DescriptorT, template< typename > class ActorT, typename DefaultT >
inline typename result_of::extract_or_default< typename DescriptorT::value_type, DefaultT, DescriptorT >::type
extract_or_default(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword, record const& rec, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< typename DescriptorT::value_type, DefaultT, DescriptorT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& >, DescriptorT > extractor(def_val);
    return aux::unwrap_value_ref(extractor(keyword.get_name(), rec));
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be visited.
 *
 * \note Caution must be exercised if the default value is a temporary object. Because the function returns
 *       a reference, if the temporary object is destroyed, the reference may become dangling.
 *
 * \param keyword The keyword of the attribute value to extract.
 * \param rec A log record view. The attribute value will be sought among those associated with the record.
 * \param def_val The default value
 * \return The extracted value, if found. The default value otherwise.
 */
template< typename DescriptorT, template< typename > class ActorT, typename DefaultT >
inline typename result_of::extract_or_default< typename DescriptorT::value_type, DefaultT, DescriptorT >::type
extract_or_default(expressions::attribute_keyword< DescriptorT, ActorT > const& keyword, record_view const& rec, DefaultT const& def_val)
{
    typedef typename result_of::extract_or_default< typename DescriptorT::value_type, DefaultT, DescriptorT >::extracted_type extracted_type;
    value_extractor< extracted_type, fallback_to_default< DefaultT const& >, DescriptorT > extractor(def_val);
    return aux::unwrap_value_ref(extractor(keyword.get_name(), rec));
}

#if !defined(BOOST_LOG_DOXYGEN_PASS)

template< typename T, typename TagT >
inline typename result_of::extract< T, TagT >::type attribute_value::extract() const
{
    return boost::log::extract< T, TagT >(*this);
}

template< typename T, typename TagT >
inline typename result_of::extract_or_throw< T, TagT >::type attribute_value::extract_or_throw() const
{
    return boost::log::extract_or_throw< T, TagT >(*this);
}

template< typename T, typename TagT >
inline typename result_of::extract_or_default< T, T, TagT >::type attribute_value::extract_or_default(T const& def_value) const
{
    return boost::log::extract_or_default< T, TagT >(*this, def_value);
}

template< typename T, typename TagT, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT, TagT >::type attribute_value::extract_or_default(DefaultT const& def_value) const
{
    return boost::log::extract_or_default< T, TagT >(*this, def_value);
}

#if defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)

template< typename T >
inline typename result_of::extract< T >::type attribute_value::extract() const
{
    return boost::log::extract< T >(*this);
}

template< typename T >
inline typename result_of::extract_or_throw< T >::type attribute_value::extract_or_throw() const
{
    return boost::log::extract_or_throw< T >(*this);
}

template< typename T >
inline typename result_of::extract_or_default< T, T >::type attribute_value::extract_or_default(T const& def_value) const
{
    return boost::log::extract_or_default< T >(*this, def_value);
}

template< typename T, typename DefaultT >
inline typename result_of::extract_or_default< T, DefaultT >::type attribute_value::extract_or_default(DefaultT const& def_value) const
{
    return boost::log::extract_or_default< T >(*this, def_value);
}

#endif // defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)

#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

#undef BOOST_LOG_AUX_VOID_DEFAULT

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_VALUE_EXTRACTION_HPP_INCLUDED_
