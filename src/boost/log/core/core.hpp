/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   core/core.hpp
 * \author Andrey Semashev
 * \date   19.04.2007
 *
 * This header contains logging core class definition.
 */

#ifndef BOOST_LOG_CORE_CORE_HPP_INCLUDED_
#define BOOST_LOG_CORE_CORE_HPP_INCLUDED_

#include <utility>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/move/core.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/expressions/filter.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace sinks {

class sink;

} // namespace sinks

#endif // BOOST_LOG_DOXYGEN_PASS

class core;

typedef shared_ptr< core > core_ptr;

/*!
 * \brief Logging library core class
 *
 * The logging core is used to interconnect log sources and sinks. It also provides
 * a number of basic features, like global filtering and global and thread-specific attribute storage.
 *
 * The logging core is a singleton. Users can acquire the core instance by calling the static method <tt>get</tt>.
 */
class core
{
public:
    //! Exception handler function type
    typedef boost::log::aux::light_function< void () > exception_handler_type;

private:
    //! Implementation type
    struct implementation;
    friend struct implementation;

private:
    //! A pointer to the implementation
    implementation* m_impl;

private:
    //! \cond
    core();
    //! \endcond

public:
    /*!
     * Destructor. Destroys the core, releases any sinks and attributes that were registered.
     */
    ~core();

    /*!
     * \return The method returns a pointer to the logging core singleton instance.
     */
    BOOST_LOG_API static core_ptr get();

    /*!
     * The method enables or disables logging.
     *
     * Setting this status to \c false allows you to completely wipe out any logging activity, including
     * filtering and generation of attribute values. It is useful if you want to completely disable logging
     * in a running application. The state of logging does not alter any other properties of the logging
     * library, such as filters or sinks, so you can enable logging with the very same settings that you had
     * when the logging was disabled.
     * This feature may also be useful if you want to perform major changes to logging configuration and
     * don't want your application to block on opening or pushing a log record.
     *
     * By default logging is enabled.
     *
     * \param enabled The actual flag of logging activity.
     * \return The previous value of enabled/disabled logging flag
     */
    BOOST_LOG_API bool set_logging_enabled(bool enabled = true);
    /*!
     * The method allows to detect if logging is enabled. See the comment for \c set_logging_enabled.
     */
    BOOST_LOG_API bool get_logging_enabled() const;

    /*!
     * The method sets the global logging filter. The filter is applied to every log record that is processed.
     *
     * \param filter The filter function object to be installed.
     */
    BOOST_LOG_API void set_filter(filter const& filter);
    /*!
     * The method removes the global logging filter. All log records are passed to sinks without global filtering applied.
     */
    BOOST_LOG_API void reset_filter();

    /*!
     * The method adds a new sink. The sink is included into logging process immediately after being added and until being removed.
     * No sink can be added more than once at the same time. If the sink is already registered, the call is ignored.
     *
     * \param s The sink to be registered.
     */
    BOOST_LOG_API void add_sink(shared_ptr< sinks::sink > const& s);
    /*!
     * The method removes the sink from the output. The sink will not receive any log records after removal.
     * The call has no effect if the sink is not registered.
     *
     * \param s The sink to be unregistered.
     */
    BOOST_LOG_API void remove_sink(shared_ptr< sinks::sink > const& s);
    /*!
     * The method removes all registered sinks from the output. The sinks will not receive any log records after removal.
     */
    BOOST_LOG_API void remove_all_sinks();

    /*!
     * The method performs flush on all registered sinks.
     *
     * \note This method may take long time to complete as it may block until all sinks manage to process all buffered log records.
     *       The call will also block all logging attempts until the operation completes.
     */
    BOOST_LOG_API void flush();

    /*!
     * The method adds an attribute to the global attribute set. The attribute will be implicitly added to every log record.
     *
     * \param name The attribute name.
     * \param attr The attribute factory.
     * \return A pair of values. If the second member is \c true, then the attribute is added and the first member points to the
     *         attribute. Otherwise the attribute was not added and the first member points to the attribute that prevents
     *         addition.
     */
    BOOST_LOG_API std::pair< attribute_set::iterator, bool > add_global_attribute(attribute_name const& name, attribute const& attr);
    /*!
     * The method removes an attribute from the global attribute set.
     *
     * \pre The attribute was added with the \c add_global_attribute call.
     * \post The attribute is no longer registered as a global attribute. The iterator is invalidated after removal.
     *
     * \param it Iterator to the previously added attribute.
     */
    BOOST_LOG_API void remove_global_attribute(attribute_set::iterator it);

    /*!
     * The method returns a copy of the complete set of currently registered global attributes.
     */
    BOOST_LOG_API attribute_set get_global_attributes() const;
    /*!
     * The method replaces the complete set of currently registered global attributes with the provided set.
     *
     * \note The method invalidates all iterators and references that may have been returned
     *       from the \c add_global_attribute method.
     *
     * \param attrs The set of attributes to be installed.
     */
    BOOST_LOG_API void set_global_attributes(attribute_set const& attrs);

    /*!
     * The method adds an attribute to the thread-specific attribute set. The attribute will be implicitly added to
     * every log record made in the current thread.
     *
     * \note In single-threaded build the effect is the same as adding the attribute globally. This, however, does
     *       not imply that iterators to thread-specific and global attributes are interchangeable.
     *
     * \param name The attribute name.
     * \param attr The attribute factory.
     * \return A pair of values. If the second member is \c true, then the attribute is added and the first member points to the
     *         attribute. Otherwise the attribute was not added and the first member points to the attribute that prevents
     *         addition.
     */
    BOOST_LOG_API std::pair< attribute_set::iterator, bool > add_thread_attribute(attribute_name const& name, attribute const& attr);
    /*!
     * The method removes an attribute from the thread-specific attribute set.
     *
     * \pre The attribute was added with the \c add_thread_attribute call.
     * \post The attribute is no longer registered as a thread-specific attribute. The iterator is invalidated after removal.
     *
     * \param it Iterator to the previously added attribute.
     */
    BOOST_LOG_API void remove_thread_attribute(attribute_set::iterator it);

    /*!
     * The method returns a copy of the complete set of currently registered thread-specific attributes.
     */
    BOOST_LOG_API attribute_set get_thread_attributes() const;
    /*!
     * The method replaces the complete set of currently registered thread-specific attributes with the provided set.
     *
     * \note The method invalidates all iterators and references that may have been returned
     *       from the \c add_thread_attribute method.
     *
     * \param attrs The set of attributes to be installed.
     */
    BOOST_LOG_API void set_thread_attributes(attribute_set const& attrs);

    /*!
     * The method sets exception handler function. The function will be called with no arguments
     * in case if an exception occurs during either \c open_record or \c push_record method
     * execution. Since exception handler is called from a \c catch statement, the exception
     * can be rethrown in order to determine its type.
     *
     * By default no handler is installed, thus any exception is propagated as usual.
     *
     * \sa See also: <tt>utility/exception_handler.hpp</tt>
     * \param handler Exception handling function
     *
     * \note The exception handler can be invoked in several threads concurrently.
     *       Thread interruptions are not affected by exception handlers.
     */
    BOOST_LOG_API void set_exception_handler(exception_handler_type const& handler);

    /*!
     * The method attempts to open a new record to be written. While attempting to open a log record all filtering is applied.
     * A successfully opened record can be pushed further to sinks by calling the \c push_record method or simply destroyed by
     * destroying the returned object.
     *
     * More than one open records are allowed, such records exist independently. All attribute values are acquired during opening
     * the record and do not interact between records.
     *
     * The returned records can be copied, however, they must not be passed between different threads.
     *
     * \param source_attributes The set of source-specific attributes to be attached to the record to be opened.
     * \return A valid log record if the record is opened, an invalid record object if not (e.g. because it didn't pass filtering).
     *
     * \b Throws: If an exception handler is installed, only throws if the handler throws. Otherwise may
     *            throw if one of the sinks throws, or some system resource limitation is reached.
     */
    BOOST_LOG_API record open_record(attribute_set const& source_attributes);
    /*!
     * The method attempts to open a new record to be written. While attempting to open a log record all filtering is applied.
     * A successfully opened record can be pushed further to sinks by calling the \c push_record method or simply destroyed by
     * destroying the returned object.
     *
     * More than one open records are allowed, such records exist independently. All attribute values are acquired during opening
     * the record and do not interact between records.
     *
     * The returned records can be copied, however, they must not be passed between different threads.
     *
     * \param source_attributes The set of source-specific attribute values to be attached to the record to be opened.
     * \return A valid log record if the record is opened, an invalid record object if not (e.g. because it didn't pass filtering).
     *
     * \b Throws: If an exception handler is installed, only throws if the handler throws. Otherwise may
     *            throw if one of the sinks throws, or some system resource limitation is reached.
     */
    BOOST_LOG_API record open_record(attribute_value_set const& source_attributes);
    /*!
     * The method attempts to open a new record to be written. While attempting to open a log record all filtering is applied.
     * A successfully opened record can be pushed further to sinks by calling the \c push_record method or simply destroyed by
     * destroying the returned object.
     *
     * More than one open records are allowed, such records exist independently. All attribute values are acquired during opening
     * the record and do not interact between records.
     *
     * The returned records can be copied, however, they must not be passed between different threads.
     *
     * \param source_attributes The set of source-specific attribute values to be attached to the record to be opened. The contents
     *                          of this container are unspecified after this call.
     * \return A valid log record if the record is opened, an invalid record object if not (e.g. because it didn't pass filtering).
     *
     * \b Throws: If an exception handler is installed, only throws if the handler throws. Otherwise may
     *            throw if one of the sinks throws, or some system resource limitation is reached.
     */
    BOOST_FORCEINLINE record open_record(BOOST_RV_REF(attribute_value_set) source_attributes)
    {
        return open_record_move(static_cast< attribute_value_set& >(source_attributes));
    }

    /*!
     * The method pushes the record to sinks. The record is moved from in the process.
     *
     * \pre <tt>!!rec == true</tt>
     * \post <tt>!rec == true</tt>
     * \param rec A previously successfully opened log record.
     *
     * \b Throws: If an exception handler is installed, only throws if the handler throws. Otherwise may
     *            throw if one of the sinks throws.
     */
    BOOST_FORCEINLINE void push_record(BOOST_RV_REF(record) rec)
    {
        push_record_move(static_cast< record& >(rec));
    }

    BOOST_DELETED_FUNCTION(core(core const&))
    BOOST_DELETED_FUNCTION(core& operator= (core const&))

#ifndef BOOST_LOG_DOXYGEN_PASS
private:
    //! Opens log record. This function is mostly needed to maintain ABI stable between C++03 and C++11.
    BOOST_LOG_API record open_record_move(attribute_value_set& source_attributes);
    //! The method pushes the record to sinks.
    BOOST_LOG_API void push_record_move(record& rec);
#endif // BOOST_LOG_DOXYGEN_PASS
};

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_CORE_CORE_HPP_INCLUDED_
