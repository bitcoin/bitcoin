/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attribute_value.hpp
 * \author Andrey Semashev
 * \date   21.05.2010
 *
 * The header contains \c attribute_value class definition.
 */

#ifndef BOOST_LOG_ATTRIBUTE_VALUE_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTE_VALUE_HPP_INCLUDED_

#include <boost/type_index.hpp>
#include <boost/move/core.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/core/explicit_operator_bool.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/utility/type_dispatch/type_dispatcher.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/value_extraction_fwd.hpp>
#include <boost/log/attributes/value_visitation_fwd.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief An attribute value class
 *
 * An attribute value is an object that contains a piece of data that represents an attribute state
 * at the point of the value acquisition. All major operations with log records, such as filtering and
 * formatting, involve attribute values contained in a single view. Most likely an attribute value is
 * implemented as a simple holder of some typed value. This holder implements the
 * \c attribute_value::implementation interface and acts as a pimpl for the \c attribute_value
 * object. The \c attribute_value class provides type dispatching support in order to allow
 * to extract the value from the holder.
 *
 * Normally, attributes and their values shall be designed in order to exclude as much interference as
 * reasonable. Such approach allows to have more than one attribute value simultaneously, which improves
 * scalability and allows to implement generating attributes.
 *
 * However, there are cases when this approach does not help to achieve the required level of independency
 * of attribute values and attribute itself from each other at a reasonable performance tradeoff.
 * For example, an attribute or its values may use thread-specific data, which is global and shared
 * between all the instances of the attribute/value. Passing such an attribute value to another thread
 * would be a disaster. To solve this the library defines an additional method for attribute values,
 * namely \c detach_from_thread. The \c attribute_value class forwards the call to its pimpl,
 * which is supposed to ensure that it no longer refers to any thread-specific data after the call.
 * The pimpl can create a new holder as a result of this method and return it to the \c attribute_value
 * wrapper, which will keep the returned reference for any further calls.
 * This method is called for all attribute values that are passed to another thread.
 */
class attribute_value
{
    BOOST_COPYABLE_AND_MOVABLE(attribute_value)

public:
    /*!
     * \brief A base class for an attribute value implementation
     *
     * All attribute value holders should derive from this interface.
     */
    struct BOOST_LOG_NO_VTABLE impl :
        public attribute::impl
    {
    public:
        /*!
         * The method dispatches the value to the given object.
         *
         * \param dispatcher The object that attempts to dispatch the stored value.
         * \return true if \a dispatcher was capable to consume the real attribute value type and false otherwise.
         */
        virtual bool dispatch(type_dispatcher& dispatcher) = 0;

        /*!
         * The method is called when the attribute value is passed to another thread (e.g.
         * in case of asynchronous logging). The value should ensure it properly owns all thread-specific data.
         *
         * \return An actual pointer to the attribute value. It may either point to this object or another.
         *         In the latter case the returned pointer replaces the pointer used by caller to invoke this
         *         method and is considered to be a functional equivalent to the previous pointer.
         */
        virtual intrusive_ptr< impl > detach_from_thread()
        {
            return this;
        }

        /*!
         * \return The attribute value that refers to self implementation.
         */
        attribute_value get_value() BOOST_OVERRIDE { return attribute_value(this); }

        /*!
         * \return The attribute value type
         */
        virtual typeindex::type_index get_type() const { return typeindex::type_index(); }
    };

private:
    //! Pointer to the value implementation
    intrusive_ptr< impl > m_pImpl;

public:
    /*!
     * Default constructor. Creates an empty (absent) attribute value.
     */
    BOOST_DEFAULTED_FUNCTION(attribute_value(), BOOST_NOEXCEPT {})

    /*!
     * Copy constructor
     */
    attribute_value(attribute_value const& that) BOOST_NOEXCEPT : m_pImpl(that.m_pImpl) {}

    /*!
     * Move constructor
     */
    attribute_value(BOOST_RV_REF(attribute_value) that) BOOST_NOEXCEPT { m_pImpl.swap(that.m_pImpl); }

    /*!
     * Initializing constructor. Creates an attribute value that refers to the specified holder.
     *
     * \param p A pointer to the attribute value holder.
     */
    explicit attribute_value(intrusive_ptr< impl > p) BOOST_NOEXCEPT { m_pImpl.swap(p); }

    /*!
     * Copy assignment
     */
    attribute_value& operator= (BOOST_COPY_ASSIGN_REF(attribute_value) that) BOOST_NOEXCEPT
    {
        m_pImpl = that.m_pImpl;
        return *this;
    }

    /*!
     * Move assignment
     */
    attribute_value& operator= (BOOST_RV_REF(attribute_value) that) BOOST_NOEXCEPT
    {
        m_pImpl.swap(that.m_pImpl);
        return *this;
    }

    /*!
     * The operator checks if the attribute value is empty
     */
    BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()
    /*!
     * The operator checks if the attribute value is empty
     */
    bool operator! () const BOOST_NOEXCEPT { return !m_pImpl; }

    /*!
     * The method returns the type information of the stored value of the attribute.
     * The returned type info wrapper may be empty if the attribute value is empty or
     * the information cannot be provided. If the returned value is not empty, the type
     * can be used for value extraction.
     */
    typeindex::type_index get_type() const
    {
        if (m_pImpl.get())
            return m_pImpl->get_type();
        else
            return typeindex::type_index();
    }

    /*!
     * The method is called when the attribute value is passed to another thread (e.g.
     * in case of asynchronous logging). The value should ensure it properly owns all thread-specific data.
     *
     * \post The attribute value no longer refers to any thread-specific resources.
     */
    void detach_from_thread()
    {
        if (m_pImpl.get())
            m_pImpl->detach_from_thread().swap(m_pImpl);
    }

    /*!
     * The method dispatches the value to the given object. This method is a low level interface for
     * attribute value visitation and extraction. For typical usage these interfaces may be more convenient.
     *
     * \param dispatcher The object that attempts to dispatch the stored value.
     * \return \c true if the value is not empty and the \a dispatcher was capable to consume
     *         the real attribute value type and \c false otherwise.
     */
    bool dispatch(type_dispatcher& dispatcher) const
    {
        if (m_pImpl.get())
            return m_pImpl->dispatch(dispatcher);
        else
            return false;
    }

#if !defined(BOOST_LOG_DOXYGEN_PASS)
#if !defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)
#define BOOST_LOG_AUX_VOID_DEFAULT = void
#else
#define BOOST_LOG_AUX_VOID_DEFAULT
#endif
#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

    /*!
     * The method attempts to extract the stored value, assuming the value has the specified type.
     * One can specify either a single type or an MPL type sequence, in which case the stored value
     * is checked against every type in the sequence.
     *
     * \note Include <tt>value_extraction.hpp</tt> prior to using this method.
     *
     * \return The extracted value, if the attribute value is not empty and the value is the same
     *         as specified. Otherwise returns an empty value. See description of the \c result_of::extract
     *         metafunction for information on the nature of the result value.
     */
    template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
    typename result_of::extract< T, TagT >::type extract() const;

    /*!
     * The method attempts to extract the stored value, assuming the value has the specified type.
     * One can specify either a single type or an MPL type sequence, in which case the stored value
     * is checked against every type in the sequence.
     *
     * \note Include <tt>value_extraction.hpp</tt> prior to using this method.
     *
     * \return The extracted value, if the attribute value is not empty and the value is the same
     *         as specified. Otherwise an exception is thrown. See description of the \c result_of::extract_or_throw
     *         metafunction for information on the nature of the result value.
     */
    template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
    typename result_of::extract_or_throw< T, TagT >::type extract_or_throw() const;

    /*!
     * The method attempts to extract the stored value, assuming the value has the specified type.
     * One can specify either a single type or an MPL type sequence, in which case the stored value
     * is checked against every type in the sequence. If extraction fails, the default value is returned.
     *
     * \note Include <tt>value_extraction.hpp</tt> prior to using this method.
     *
     * \param def_value Default value.
     *
     * \return The extracted value, if the attribute value is not empty and the value is the same
     *         as specified. Otherwise returns the default value. See description of the \c result_of::extract_or_default
     *         metafunction for information on the nature of the result value.
     */
    template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT >
    typename result_of::extract_or_default< T, T, TagT >::type extract_or_default(T const& def_value) const;

    /*!
     * The method attempts to extract the stored value, assuming the value has the specified type.
     * One can specify either a single type or an MPL type sequence, in which case the stored value
     * is checked against every type in the sequence. If extraction fails, the default value is returned.
     *
     * \note Include <tt>value_extraction.hpp</tt> prior to using this method.
     *
     * \param def_value Default value.
     *
     * \return The extracted value, if the attribute value is not empty and the value is the same
     *         as specified. Otherwise returns the default value. See description of the \c result_of::extract_or_default
     *         metafunction for information on the nature of the result value.
     */
    template< typename T, typename TagT BOOST_LOG_AUX_VOID_DEFAULT, typename DefaultT >
    typename result_of::extract_or_default< T, DefaultT, TagT >::type extract_or_default(DefaultT const& def_value) const;

#if defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)
    /*!
     * The method attempts to extract the stored value, assuming the value has the specified type.
     * One can specify either a single type or an MPL type sequence, in which case the stored value
     * is checked against every type in the sequence.
     *
     * \note Include <tt>value_extraction.hpp</tt> prior to using this method.
     *
     * \return The extracted value, if the attribute value is not empty and the value is the same
     *         as specified. Otherwise returns an empty value. See description of the \c result_of::extract
     *         metafunction for information on the nature of the result value.
     */
    template< typename T >
    typename result_of::extract< T >::type extract() const;

    /*!
     * The method attempts to extract the stored value, assuming the value has the specified type.
     * One can specify either a single type or an MPL type sequence, in which case the stored value
     * is checked against every type in the sequence.
     *
     * \note Include <tt>value_extraction.hpp</tt> prior to using this method.
     *
     * \return The extracted value, if the attribute value is not empty and the value is the same
     *         as specified. Otherwise an exception is thrown. See description of the \c result_of::extract_or_throw
     *         metafunction for information on the nature of the result value.
     */
    template< typename T >
    typename result_of::extract_or_throw< T >::type extract_or_throw() const;

    /*!
     * The method attempts to extract the stored value, assuming the value has the specified type.
     * One can specify either a single type or an MPL type sequence, in which case the stored value
     * is checked against every type in the sequence. If extraction fails, the default value is returned.
     *
     * \note Include <tt>value_extraction.hpp</tt> prior to using this method.
     *
     * \param def_value Default value.
     *
     * \return The extracted value, if the attribute value is not empty and the value is the same
     *         as specified. Otherwise returns the default value. See description of the \c result_of::extract_or_default
     *         metafunction for information on the nature of the result value.
     */
    template< typename T >
    typename result_of::extract_or_default< T, T >::type extract_or_default(T const& def_value) const;

    /*!
     * The method attempts to extract the stored value, assuming the value has the specified type.
     * One can specify either a single type or an MPL type sequence, in which case the stored value
     * is checked against every type in the sequence. If extraction fails, the default value is returned.
     *
     * \note Include <tt>value_extraction.hpp</tt> prior to using this method.
     *
     * \param def_value Default value.
     *
     * \return The extracted value, if the attribute value is not empty and the value is the same
     *         as specified. Otherwise returns the default value. See description of the \c result_of::extract_or_default
     *         metafunction for information on the nature of the result value.
     */
    template< typename T, typename DefaultT >
    typename result_of::extract_or_default< T, DefaultT >::type extract_or_default(DefaultT const& def_value) const;
#endif // defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS)

#undef BOOST_LOG_AUX_VOID_DEFAULT

    /*!
     * The method attempts to extract the stored value, assuming the value has the specified type,
     * and pass it to the \a visitor function object.
     * One can specify either a single type or an MPL type sequence, in which case the stored value
     * is checked against every type in the sequence.
     *
     * \note Include <tt>value_visitation.hpp</tt> prior to using this method.
     *
     * \param visitor A function object that will be invoked on the extracted attribute value.
     *                The visitor should be capable to be called with a single argument of
     *                any type of the specified types in \c T.
     *
     * \return The result of visitation.
     */
    template< typename T, typename VisitorT >
    visitation_result visit(VisitorT visitor) const;

    /*!
     * The method swaps two attribute values
     */
    void swap(attribute_value& that) BOOST_NOEXCEPT
    {
        m_pImpl.swap(that.m_pImpl);
    }
};

/*!
 * The function swaps two attribute values
 */
inline void swap(attribute_value& left, attribute_value& right) BOOST_NOEXCEPT
{
    left.swap(right);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>
#if defined(BOOST_LOG_ATTRIBUTES_ATTRIBUTE_HPP_INCLUDED_)
#include <boost/log/detail/attribute_get_value_impl.hpp>
#endif

#endif // BOOST_LOG_ATTRIBUTE_VALUE_HPP_INCLUDED_
