/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   basic_logger.hpp
 * \author Andrey Semashev
 * \date   08.03.2007
 *
 * The header contains implementation of a base class for loggers. Convenience macros
 * for defining custom loggers are also provided.
 */

#ifndef BOOST_LOG_SOURCES_BASIC_LOGGER_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_BASIC_LOGGER_HPP_INCLUDED_

#include <exception>
#include <utility>
#include <ostream>
#include <boost/assert.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/core/addressof.hpp>
#include <boost/type_traits/is_nothrow_swappable.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/identity.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/sources/features.hpp>
#include <boost/log/sources/threading_models.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sources {

/*!
 * \brief Basic logger class
 *
 * The \c basic_logger class template serves as a base class for all loggers
 * provided by the library. It can also be used as a base for user-defined
 * loggers. The template parameters are:
 *
 * \li \c CharT - logging character type
 * \li \c FinalT - final type of the logger that eventually derives from
 *     the \c basic_logger. There may be other classes in the hierarchy
 *     between the final class and \c basic_logger.
 * \li \c ThreadingModelT - threading model policy. Must provide methods
 *     of the Boost.Thread locking concept used in \c basic_logger class
 *     and all its derivatives in the hierarchy up to the \c FinalT class.
 *     The \c basic_logger class itself requires methods of the
 *     SharedLockable concept. The threading model policy must also be
 *     default and copy-constructible and support member function \c swap.
 *     There are currently two policies provided: \c single_thread_model
 *     and \c multi_thread_model.
 *
 * The logger implements fundamental facilities of loggers, such as storing
 * source-specific attribute set and formatting log record messages. The basic
 * logger interacts with the logging core in order to apply filtering and
 * pass records to sinks.
 */
template< typename CharT, typename FinalT, typename ThreadingModelT >
class basic_logger :
    public ThreadingModelT
{
    typedef basic_logger this_type;
    BOOST_COPYABLE_AND_MOVABLE_ALT(this_type)

public:
    //! Character type
    typedef CharT char_type;
    //! Final logger type
    typedef FinalT final_type;
    //! Threading model type
    typedef ThreadingModelT threading_model;

#if !defined(BOOST_LOG_NO_THREADS)
    //! Lock requirement for the swap_unlocked method
    typedef boost::log::aux::multiple_unique_lock2< threading_model, threading_model > swap_lock;
    //! Lock requirement for the add_attribute_unlocked method
    typedef boost::log::aux::exclusive_lock_guard< threading_model > add_attribute_lock;
    //! Lock requirement for the remove_attribute_unlocked method
    typedef boost::log::aux::exclusive_lock_guard< threading_model > remove_attribute_lock;
    //! Lock requirement for the remove_all_attributes_unlocked method
    typedef boost::log::aux::exclusive_lock_guard< threading_model > remove_all_attributes_lock;
    //! Lock requirement for the get_attributes method
    typedef boost::log::aux::shared_lock_guard< const threading_model > get_attributes_lock;
    //! Lock requirement for the open_record_unlocked method
    typedef boost::log::aux::shared_lock_guard< threading_model > open_record_lock;
    //! Lock requirement for the set_attributes method
    typedef boost::log::aux::exclusive_lock_guard< threading_model > set_attributes_lock;
#else
    typedef no_lock< threading_model > swap_lock;
    typedef no_lock< threading_model > add_attribute_lock;
    typedef no_lock< threading_model > remove_attribute_lock;
    typedef no_lock< threading_model > remove_all_attributes_lock;
    typedef no_lock< const threading_model > get_attributes_lock;
    typedef no_lock< threading_model > open_record_lock;
    typedef no_lock< threading_model > set_attributes_lock;
#endif

    //! Lock requirement for the push_record_unlocked method
    typedef no_lock< threading_model > push_record_lock;

private:
    //! A pointer to the logging system
    core_ptr m_pCore;

    //! Logger-specific attribute set
    attribute_set m_Attributes;

public:
    /*!
     * Constructor. Initializes internal data structures of the basic logger class,
     * acquires reference to the logging core.
     */
    basic_logger() :
        threading_model(),
        m_pCore(core::get())
    {
    }
    /*!
     * Copy constructor. Copies all attributes from the source logger.
     *
     * \note Not thread-safe. The source logger must be locked in the final class before copying.
     *
     * \param that Source logger
     */
    basic_logger(basic_logger const& that) :
        threading_model(static_cast< threading_model const& >(that)),
        m_pCore(that.m_pCore),
        m_Attributes(that.m_Attributes)
    {
    }
    /*!
     * Move constructor. Moves all attributes from the source logger.
     *
     * \note Not thread-safe. The source logger must be locked in the final class before copying.
     *
     * \param that Source logger
     */
    basic_logger(BOOST_RV_REF(basic_logger) that) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible< threading_model >::value &&
                                                                    boost::is_nothrow_move_constructible< core_ptr >::value &&
                                                                    boost::is_nothrow_move_constructible< attribute_set >::value) :
        threading_model(boost::move(static_cast< threading_model& >(that))),
        m_pCore(boost::move(that.m_pCore)),
        m_Attributes(boost::move(that.m_Attributes))
    {
    }
    /*!
     * Constructor with named arguments. The constructor ignores all arguments. The result of
     * construction is equivalent to default construction.
     */
    template< typename ArgsT >
    explicit basic_logger(ArgsT const&) :
        threading_model(),
        m_pCore(core::get())
    {
    }

protected:
    /*!
     * An accessor to the logging system pointer
     */
    core_ptr const& core() const { return m_pCore; }
    /*!
     * An accessor to the logger attributes
     */
    attribute_set& attributes() { return m_Attributes; }
    /*!
     * An accessor to the logger attributes
     */
    attribute_set const& attributes() const { return m_Attributes; }
    /*!
     * An accessor to the threading model base
     */
    threading_model& get_threading_model() BOOST_NOEXCEPT { return *this; }
    /*!
     * An accessor to the threading model base
     */
    threading_model const& get_threading_model() const BOOST_NOEXCEPT { return *this; }
    /*!
     * An accessor to the final logger
     */
    final_type* final_this() BOOST_NOEXCEPT
    {
        BOOST_LOG_ASSUME(this != NULL);
        return static_cast< final_type* >(this);
    }
    /*!
     * An accessor to the final logger
     */
    final_type const* final_this() const BOOST_NOEXCEPT
    {
        BOOST_LOG_ASSUME(this != NULL);
        return static_cast< final_type const* >(this);
    }

    /*!
     * Unlocked \c swap
     */
    void swap_unlocked(basic_logger& that)
    {
        get_threading_model().swap(that.get_threading_model());
        m_Attributes.swap(that.m_Attributes);
    }

    /*!
     * Unlocked \c add_attribute
     */
    std::pair< attribute_set::iterator, bool > add_attribute_unlocked(attribute_name const& name, attribute const& attr)
    {
        return m_Attributes.insert(name, attr);
    }

    /*!
     * Unlocked \c remove_attribute
     */
    void remove_attribute_unlocked(attribute_set::iterator it)
    {
        m_Attributes.erase(it);
    }

    /*!
     * Unlocked \c remove_all_attributes
     */
    void remove_all_attributes_unlocked()
    {
        m_Attributes.clear();
    }

    /*!
     * Unlocked \c open_record
     */
    record open_record_unlocked()
    {
        return m_pCore->open_record(m_Attributes);
    }
    /*!
     * Unlocked \c open_record
     */
    template< typename ArgsT >
    record open_record_unlocked(ArgsT const&)
    {
        return m_pCore->open_record(m_Attributes);
    }

    /*!
     * Unlocked \c push_record
     */
    void push_record_unlocked(BOOST_RV_REF(record) rec)
    {
        m_pCore->push_record(boost::move(rec));
    }

    /*!
     * Unlocked \c get_attributes
     */
    attribute_set get_attributes_unlocked() const
    {
        return m_Attributes;
    }

    /*!
     * Unlocked \c set_attributes
     */
    void set_attributes_unlocked(attribute_set const& attrs)
    {
        m_Attributes = attrs;
    }

    //! Assignment is closed (should be implemented through copy and swap in the final class)
    BOOST_DELETED_FUNCTION(basic_logger& operator= (basic_logger const&))
};

/*!
 * Free-standing swap for all loggers
 */
template< typename CharT, typename FinalT, typename ThreadingModelT >
inline void swap(
    basic_logger< CharT, FinalT, ThreadingModelT >& left,
    basic_logger< CharT, FinalT, ThreadingModelT >& right) BOOST_NOEXCEPT_IF(boost::is_nothrow_swappable< FinalT >::value)
{
    static_cast< FinalT& >(left).swap(static_cast< FinalT& >(right));
}

/*!
 * \brief A composite logger that inherits a number of features
 *
 * The composite logger is a helper class that simplifies feature composition into the final logger.
 * The user's logger class is expected to derive from the composite logger class, instantiated with
 * the character type, the user's logger class, the threading model and the list of the required features.
 * The former three parameters are passed to the \c basic_logger class template. The feature list
 * must be an MPL type sequence, where each element is a unary MPL metafunction class, that upon
 * applying on its argument results in a logging feature class that derives from the argument.
 * Every logger feature provided by the library can participate in the feature list.
 */
template< typename CharT, typename FinalT, typename ThreadingModelT, typename FeaturesT >
class basic_composite_logger :
    public boost::log::sources::aux::inherit_features<
        basic_logger< CharT, FinalT, ThreadingModelT >,
        FeaturesT
    >::type
{
private:
    //! Base type (the hierarchy of features)
    typedef typename boost::log::sources::aux::inherit_features<
        basic_logger< CharT, FinalT, ThreadingModelT >,
        FeaturesT
    >::type base_type;

protected:
    //! The composite logger type (for use in the user's logger class)
    typedef basic_composite_logger logger_base;
    BOOST_COPYABLE_AND_MOVABLE_ALT(logger_base)

public:
    //! Threading model being used
    typedef typename base_type::threading_model threading_model;

    //! Lock requirement for the swap_unlocked method
    typedef typename base_type::swap_lock swap_lock;

#if !defined(BOOST_LOG_NO_THREADS)

public:
    /*!
     * Default constructor (default-constructs all features)
     */
    basic_composite_logger() {}
    /*!
     * Copy constructor
     */
    basic_composite_logger(basic_composite_logger const& that) :
        base_type
        ((
            boost::log::aux::shared_lock_guard< const threading_model >(that.get_threading_model()),
            static_cast< base_type const& >(that)
        ))
    {
    }
    /*!
     * Move constructor
     */
    basic_composite_logger(BOOST_RV_REF(logger_base) that) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible< base_type >::value) :
        base_type(boost::move(static_cast< base_type& >(that)))
    {
    }
    /*!
     * Constructor with named parameters
     */
    template< typename ArgsT >
    explicit basic_composite_logger(ArgsT const& args) : base_type(args)
    {
    }

    /*!
     * The method adds an attribute to the source-specific attribute set. The attribute will be implicitly added to
     * every log record made with the current logger.
     *
     * \param name The attribute name.
     * \param attr The attribute factory.
     * \return A pair of values. If the second member is \c true, then the attribute is added and the first member points to the
     *         attribute. Otherwise the attribute was not added and the first member points to the attribute that prevents
     *         addition.
     */
    std::pair< attribute_set::iterator, bool > add_attribute(attribute_name const& name, attribute const& attr)
    {
        typename base_type::add_attribute_lock lock(base_type::get_threading_model());
        return base_type::add_attribute_unlocked(name, attr);
    }
    /*!
     * The method removes an attribute from the source-specific attribute set.
     *
     * \pre The attribute was added with the add_attribute call for this instance of the logger.
     * \post The attribute is no longer registered as a source-specific attribute for this logger. The iterator is invalidated after removal.
     *
     * \param it Iterator to the previously added attribute.
     */
    void remove_attribute(attribute_set::iterator it)
    {
        typename base_type::remove_attribute_lock lock(base_type::get_threading_model());
        base_type::remove_attribute_unlocked(it);
    }

    /*!
     * The method removes all attributes from the logger. All iterators and references to the removed attributes are invalidated.
     */
    void remove_all_attributes()
    {
        typename base_type::remove_all_attributes_lock lock(base_type::get_threading_model());
        base_type::remove_all_attributes_unlocked();
    }

    /*!
     * The method retrieves a copy of a set with all attributes from the logger.
     *
     * \return The copy of the attribute set. Attributes are shallow-copied.
     */
    attribute_set get_attributes() const
    {
        typename base_type::get_attributes_lock lock(base_type::get_threading_model());
        return base_type::get_attributes_unlocked();
    }

    /*!
     * The method installs the whole attribute set into the logger. All iterators and references to elements of
     * the previous set are invalidated. Iterators to the \a attrs set are not valid to be used with the logger (that is,
     * the logger owns a copy of \a attrs after completion).
     *
     * \param attrs The set of attributes to install into the logger. Attributes are shallow-copied.
     */
    void set_attributes(attribute_set const& attrs)
    {
        typename base_type::set_attributes_lock lock(base_type::get_threading_model());
        base_type::set_attributes_unlocked(attrs);
    }

    /*!
     * The method opens a new log record in the logging core.
     *
     * \return A valid record handle if the logging record is opened successfully, an invalid handle otherwise.
     */
    record open_record()
    {
        // Perform a quick check first
        if (this->core()->get_logging_enabled())
        {
            typename base_type::open_record_lock lock(base_type::get_threading_model());
            return base_type::open_record_unlocked(boost::log::aux::empty_arg_list());
        }
        else
            return record();
    }
    /*!
     * The method opens a new log record in the logging core.
     *
     * \param args A set of additional named arguments. The parameter is ignored.
     * \return A valid record handle if the logging record is opened successfully, an invalid handle otherwise.
     */
    template< typename ArgsT >
    record open_record(ArgsT const& args)
    {
        // Perform a quick check first
        if (this->core()->get_logging_enabled())
        {
            typename base_type::open_record_lock lock(base_type::get_threading_model());
            return base_type::open_record_unlocked(args);
        }
        else
            return record();
    }
    /*!
     * The method pushes the constructed message to the logging core
     *
     * \param rec The log record with the formatted message
     */
    void push_record(BOOST_RV_REF(record) rec)
    {
        typename base_type::push_record_lock lock(base_type::get_threading_model());
        base_type::push_record_unlocked(boost::move(rec));
    }
    /*!
     * Thread-safe implementation of swap
     */
    void swap(basic_composite_logger& that)
    {
        swap_lock lock(base_type::get_threading_model(), that.get_threading_model());
        base_type::swap_unlocked(static_cast< base_type& >(that));
    }

protected:
    /*!
     * Assignment for the final class. Threadsafe, provides strong exception guarantee.
     */
    FinalT& assign(FinalT const& that)
    {
        BOOST_LOG_ASSUME(this != NULL);
        if (static_cast< FinalT* >(this) != boost::addressof(that))
        {
            // We'll have to explicitly create the copy in order to make sure it's unlocked when we attempt to lock *this
            FinalT tmp(that);
            boost::log::aux::exclusive_lock_guard< threading_model > lock(base_type::get_threading_model());
            base_type::swap_unlocked(tmp);
        }
        return static_cast< FinalT& >(*this);
    }
};

//! An optimized composite logger version with no multithreading support
template< typename CharT, typename FinalT, typename FeaturesT >
class basic_composite_logger< CharT, FinalT, single_thread_model, FeaturesT > :
    public boost::log::sources::aux::inherit_features<
        basic_logger< CharT, FinalT, single_thread_model >,
        FeaturesT
    >::type
{
private:
    typedef typename boost::log::sources::aux::inherit_features<
        basic_logger< CharT, FinalT, single_thread_model >,
        FeaturesT
    >::type base_type;

protected:
    typedef basic_composite_logger logger_base;
    BOOST_COPYABLE_AND_MOVABLE_ALT(logger_base)

public:
    typedef typename base_type::threading_model threading_model;

#endif // !defined(BOOST_LOG_NO_THREADS)

public:
    basic_composite_logger() {}
    basic_composite_logger(basic_composite_logger const& that) :
        base_type(static_cast< base_type const& >(that))
    {
    }
    basic_composite_logger(BOOST_RV_REF(logger_base) that) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible< base_type >::value) :
        base_type(boost::move(static_cast< base_type& >(that)))
    {
    }
    template< typename ArgsT >
    explicit basic_composite_logger(ArgsT const& args) : base_type(args)
    {
    }

    std::pair< attribute_set::iterator, bool > add_attribute(attribute_name const& name, attribute const& attr)
    {
        return base_type::add_attribute_unlocked(name, attr);
    }
    void remove_attribute(attribute_set::iterator it)
    {
        base_type::remove_attribute_unlocked(it);
    }
    void remove_all_attributes()
    {
        base_type::remove_all_attributes_unlocked();
    }
    attribute_set get_attributes() const
    {
        return base_type::get_attributes_unlocked();
    }
    void set_attributes(attribute_set const& attrs)
    {
        base_type::set_attributes_unlocked(attrs);
    }
    record open_record()
    {
        // Perform a quick check first
        if (this->core()->get_logging_enabled())
            return base_type::open_record_unlocked(boost::log::aux::empty_arg_list());
        else
            return record();
    }
    template< typename ArgsT >
    record open_record(ArgsT const& args)
    {
        // Perform a quick check first
        if (this->core()->get_logging_enabled())
            return base_type::open_record_unlocked(args);
        else
            return record();
    }
    void push_record(BOOST_RV_REF(record) rec)
    {
        base_type::push_record_unlocked(boost::move(rec));
    }
    void swap(basic_composite_logger& that)
    {
        base_type::swap_unlocked(static_cast< base_type& >(that));
    }

protected:
    FinalT& assign(FinalT that)
    {
        base_type::swap_unlocked(that);
        return static_cast< FinalT& >(*this);
    }
};


#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS_IMPL(class_type, typename_keyword)\
    public:\
        BOOST_DEFAULTED_FUNCTION(class_type(), {})\
        class_type(class_type const& that) : class_type::logger_base(\
            static_cast< typename_keyword() class_type::logger_base const& >(that)) {}\
        class_type(BOOST_RV_REF(class_type) that) BOOST_NOEXCEPT_IF(boost::is_nothrow_move_constructible< typename_keyword() class_type::logger_base >::value) : class_type::logger_base(\
            ::boost::move(static_cast< typename_keyword() class_type::logger_base& >(that))) {}\
        BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_FORWARD(class_type, class_type::logger_base)\

#endif // BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS(class_type)\
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS_IMPL(class_type, BOOST_PP_EMPTY)

#define BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS_TEMPLATE(class_type)\
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS_IMPL(class_type, BOOST_PP_IDENTITY(typename))

#define BOOST_LOG_FORWARD_LOGGER_ASSIGNMENT(class_type)\
    public:\
        class_type& operator= (BOOST_COPY_ASSIGN_REF(class_type) that)\
        {\
            return class_type::logger_base::assign(static_cast< class_type const& >(that));\
        }\
        class_type& operator= (BOOST_RV_REF(class_type) that)\
        {\
            BOOST_LOG_EXPR_IF_MT(::boost::log::aux::exclusive_lock_guard< class_type::threading_model > lock(this->get_threading_model());)\
            this->swap_unlocked(that);\
            return *this;\
        }

#define BOOST_LOG_FORWARD_LOGGER_ASSIGNMENT_TEMPLATE(class_type)\
    public:\
        class_type& operator= (BOOST_COPY_ASSIGN_REF(class_type) that)\
        {\
            return class_type::logger_base::assign(static_cast< class_type const& >(that));\
        }\
        class_type& operator= (BOOST_RV_REF(class_type) that)\
        {\
            BOOST_LOG_EXPR_IF_MT(::boost::log::aux::exclusive_lock_guard< typename class_type::threading_model > lock(this->get_threading_model());)\
            this->swap_unlocked(that);\
            return *this;\
        }

#define BOOST_LOG_FORWARD_LOGGER_MEMBERS(class_type)\
    BOOST_COPYABLE_AND_MOVABLE(class_type)\
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS(class_type)\
    BOOST_LOG_FORWARD_LOGGER_ASSIGNMENT(class_type)

#define BOOST_LOG_FORWARD_LOGGER_MEMBERS_TEMPLATE(class_type)\
    BOOST_COPYABLE_AND_MOVABLE(class_type)\
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS_TEMPLATE(class_type)\
    BOOST_LOG_FORWARD_LOGGER_ASSIGNMENT_TEMPLATE(class_type)

} // namespace sources

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

/*!
 *  \brief The macro declares a logger class that inherits a number of base classes
 *
 *  \param type_name The name of the logger class to declare
 *  \param char_type The character type of the logger. Either char or wchar_t expected.
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 *  \param threading A threading model class
 */
#define BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char_type, base_seq, threading)\
    class type_name :\
        public ::boost::log::sources::basic_composite_logger<\
            char_type,\
            type_name,\
            threading,\
            ::boost::log::sources::features< BOOST_PP_SEQ_ENUM(base_seq) >\
        >\
    {\
        BOOST_LOG_FORWARD_LOGGER_MEMBERS(type_name)\
    }



#ifdef BOOST_LOG_USE_CHAR

/*!
 *  \brief The macro declares a narrow-char logger class that inherits a number of base classes
 *
 *  Equivalent to BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char, base_seq, single_thread_model)
 *
 *  \param type_name The name of the logger class to declare
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 */
#define BOOST_LOG_DECLARE_LOGGER(type_name, base_seq)\
    BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char, base_seq, ::boost::log::sources::single_thread_model)

#if !defined(BOOST_LOG_NO_THREADS)

/*!
 *  \brief The macro declares a narrow-char thread-safe logger class that inherits a number of base classes
 *
 *  Equivalent to <tt>BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char, base_seq, multi_thread_model< shared_mutex >)</tt>
 *
 *  \param type_name The name of the logger class to declare
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 */
#define BOOST_LOG_DECLARE_LOGGER_MT(type_name, base_seq)\
    BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char, base_seq,\
        ::boost::log::sources::multi_thread_model< ::boost::shared_mutex >)

#endif // !defined(BOOST_LOG_NO_THREADS)
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 *  \brief The macro declares a wide-char logger class that inherits a number of base classes
 *
 *  Equivalent to BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, wchar_t, base_seq, single_thread_model)
 *
 *  \param type_name The name of the logger class to declare
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 */
#define BOOST_LOG_DECLARE_WLOGGER(type_name, base_seq)\
    BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, wchar_t, base_seq, ::boost::log::sources::single_thread_model)

#if !defined(BOOST_LOG_NO_THREADS)

/*!
 *  \brief The macro declares a wide-char thread-safe logger class that inherits a number of base classes
 *
 *  Equivalent to <tt>BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, wchar_t, base_seq, multi_thread_model< shared_mutex >)</tt>
 *
 *  \param type_name The name of the logger class to declare
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 */
#define BOOST_LOG_DECLARE_WLOGGER_MT(type_name, base_seq)\
    BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, wchar_t, base_seq,\
        ::boost::log::sources::multi_thread_model< ::boost::shared_mutex >)

#endif // !defined(BOOST_LOG_NO_THREADS)
#endif // BOOST_LOG_USE_WCHAR_T

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SOURCES_BASIC_LOGGER_HPP_INCLUDED_
