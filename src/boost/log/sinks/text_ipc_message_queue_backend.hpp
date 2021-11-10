/*
 *                 Copyright Lingxi Li 2015.
 *              Copyright Andrey Semashev 2016.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   text_ipc_message_queue_backend.hpp
 * \author Lingxi Li
 * \author Andrey Semashev
 * \date   14.10.2015
 *
 * The header contains implementation of a text interprocess message queue sink
 * backend along with implementation of a supporting interprocess message queue.
 */

#ifndef BOOST_LOG_SINKS_TEXT_IPC_MESSAGE_QUEUE_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_TEXT_IPC_MESSAGE_QUEUE_BACKEND_HPP_INCLUDED_

#include <limits>
#include <string>
#include <boost/cstdint.hpp>
#include <boost/move/core.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_IPC_BACKEND_CTOR_FORWARD_INTERNAL_1(z, n, data)\
    template< typename T0 >\
    explicit text_ipc_message_queue_backend(T0 const& arg0, typename boost::log::aux::enable_if_named_parameters< T0, boost::log::aux::sfinae_dummy >::type = boost::log::aux::sfinae_dummy()) :\
        m_queue(arg0) {}

#define BOOST_LOG_IPC_BACKEND_CTOR_FORWARD_INTERNAL_N(z, n, data)\
    template< BOOST_PP_ENUM_PARAMS_Z(z, n, typename T) >\
    explicit text_ipc_message_queue_backend(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, T, const& arg)) :\
        m_queue(BOOST_PP_ENUM_PARAMS_Z(z, n, arg)) {}

#define BOOST_LOG_IPC_BACKEND_CTOR_FORWARD_INTERNAL(z, n, data)\
    BOOST_PP_IF(BOOST_PP_EQUAL(n, 1), BOOST_LOG_IPC_BACKEND_CTOR_FORWARD_INTERNAL_1, BOOST_LOG_IPC_BACKEND_CTOR_FORWARD_INTERNAL_N)(z, n, data)

#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief An implementation of a text interprocess message queue sink backend and
 *        a supporting interprocess message queue.
 *
 * The sink backend sends formatted log messages to an interprocess message queue
 * which can be extracted by a viewer process. Methods of this class are not
 * thread-safe, unless otherwise specified.
 */
template< typename QueueT >
class text_ipc_message_queue_backend :
    public basic_formatted_sink_backend< char, concurrent_feeding >
{
    //! Base type
    typedef basic_formatted_sink_backend< char, concurrent_feeding > base_type;

public:
    //! Character type
    typedef base_type::char_type char_type;
    //! String type to be used as a message text holder
    typedef base_type::string_type string_type;
    //! Interprocess message queue type
    typedef QueueT queue_type;

private:
    //! Interprocess queue
    queue_type m_queue;

public:
    /*!
     * Default constructor. The method constructs the backend using the default-constructed
     * interprocess message queue. The queue may need additional setup in order to be able
     * to send messages.
     */
    text_ipc_message_queue_backend() BOOST_NOEXCEPT
    {
    }

    /*!
     * Initializing constructor. The method constructs the backend using the provided
     * interprocess message queue. The constructor moves from the provided queue.
     */
    explicit text_ipc_message_queue_backend(BOOST_RV_REF(queue_type) queue) BOOST_NOEXCEPT :
        m_queue(static_cast< BOOST_RV_REF(queue_type) >(queue))
    {
    }

    /*!
     * Constructor that passes arbitrary named parameters to the interprocess queue constructor.
     * Refer to the queue documentation for the list of supported parameters.
     */
#ifndef BOOST_LOG_DOXYGEN_PASS
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_GEN(BOOST_LOG_IPC_BACKEND_CTOR_FORWARD_INTERNAL, ~)
#else
    template< typename... Args >
    explicit text_ipc_message_queue_backend(Args&&... args);
#endif

    /*!
     * The method returns a reference to the managed \c queue_type object.
     *
     * \return A reference to the managed \c queue_type object.
     */
    queue_type& message_queue() BOOST_NOEXCEPT { return m_queue; }

    /*!
     * The method returns a constant reference to the managed \c queue_type object.
     *
     * \return A constant reference to the managed \c queue_type object.
     */
    queue_type const& message_queue() const BOOST_NOEXCEPT { return m_queue; }

    /*!
     * Tests whether the object is associated with any message queue. Only when the backend has
     * an associated message queue, will any message be sent.
     *
     * \return \c true if the object is associated with a message queue, and \c false otherwise.
     */
    bool is_open() const BOOST_NOEXCEPT { return m_queue.is_open(); }

    /*!
     * The method writes the message to the backend. Concurrent calls to this method
     * are allowed. Therefore, the backend may be used with unlocked frontend. <tt>stop_local()</tt>
     * can be used to have a blocked <tt>consume()</tt> call return and prevent future
     * calls to <tt>consume()</tt> from blocking.
     */
    void consume(record_view const&, string_type const& formatted_message)
    {
        if (m_queue.is_open())
        {
            typedef typename queue_type::size_type size_type;
            const string_type::size_type size = formatted_message.size();
            if (BOOST_UNLIKELY(size > static_cast< string_type::size_type >((std::numeric_limits< size_type >::max)())))
                BOOST_LOG_THROW_DESCR(limitation_error, "Message too long to send to an interprocess queue");
            m_queue.send(formatted_message.data(), static_cast< size_type >(size));
        }
    }
};

#undef BOOST_LOG_IPC_BACKEND_CTOR_FORWARD_INTERNAL_1
#undef BOOST_LOG_IPC_BACKEND_CTOR_FORWARD_INTERNAL_N
#undef BOOST_LOG_IPC_BACKEND_CTOR_FORWARD_INTERNAL

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_TEXT_IPC_MESSAGE_QUEUE_BACKEND_HPP_INCLUDED_
