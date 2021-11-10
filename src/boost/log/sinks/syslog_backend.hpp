/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   syslog_backend.hpp
 * \author Andrey Semashev
 * \date   08.01.2008
 *
 * The header contains implementation of a Syslog sink backend along with its setup facilities.
 */

#ifndef BOOST_LOG_SINKS_SYSLOG_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_SYSLOG_BACKEND_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifndef BOOST_LOG_WITHOUT_SYSLOG

#include <string>
#include <boost/log/detail/asio_fwd.hpp>
#include <boost/log/detail/light_function.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/syslog_constants.hpp>
#include <boost/log/sinks/attribute_mapping.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/keywords/facility.hpp>
#include <boost/log/keywords/use_impl.hpp>
#include <boost/log/keywords/ident.hpp>
#include <boost/log/keywords/ip_version.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

//! Supported IP protocol versions
enum ip_versions
{
    v4,
    v6
};

namespace syslog {

    //! The enumeration defined the possible implementation types for the syslog backend
    enum impl_types
    {
#ifdef BOOST_LOG_USE_NATIVE_SYSLOG
        native = 0                  //!< Use native syslog API
#ifndef BOOST_LOG_NO_ASIO
        ,
#endif
#endif
#ifndef BOOST_LOG_NO_ASIO
        udp_socket_based = 1        //!< Use UDP sockets, according to RFC3164
#endif
    };

    /*!
     * \brief Straightforward severity level mapping
     *
     * This type of mapping assumes that attribute with a particular name always
     * provides values that map directly onto the Syslog levels. The mapping
     * simply returns the extracted attribute value converted to the Syslog severity level.
     */
    template< typename AttributeValueT = int >
    class direct_severity_mapping :
        public basic_direct_mapping< level, AttributeValueT >
    {
        //! Base type
        typedef basic_direct_mapping< level, AttributeValueT > base_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit direct_severity_mapping(attribute_name const& name) :
            base_type(name, info)
        {
        }
    };

    /*!
     * \brief Customizable severity level mapping
     *
     * The class allows to setup a custom mapping between an attribute and Syslog severity levels.
     * The mapping should be initialized similarly to the standard \c map container, by using
     * indexing operator and assignment.
     */
    template< typename AttributeValueT = int >
    class custom_severity_mapping :
        public basic_custom_mapping< level, AttributeValueT >
    {
        //! Base type
        typedef basic_custom_mapping< level, AttributeValueT > base_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit custom_severity_mapping(attribute_name const& name) :
            base_type(name, info)
        {
        }
    };

} // namespace syslog

/*!
 * \brief An implementation of a syslog sink backend
 *
 * The backend provides support for the syslog protocol, defined in RFC3164.
 * The backend sends log records to a remote host via UDP. The host name can
 * be specified by calling the \c set_target_address method. By default log
 * records will be sent to localhost:514. The local address can be specified
 * as well, by calling the \c set_local_address method. By default syslog
 * packets will be sent from any local address available.
 *
 * It is safe to create several sink backends with the same local addresses -
 * the backends within the process will share the same socket. The same applies
 * to different processes that use the syslog backends to send records from
 * the same socket. However, it is not guaranteed to work if some third party
 * facility is using the socket.
 *
 * On systems with native syslog implementation it may be preferable to utilize
 * the POSIX syslog API instead of direct socket management in order to bypass
 * possible security limitations that may be in action. To do so one has to pass
 * the <tt>use_impl = native</tt> to the backend constructor. Note, however,
 * that in that case you will only have one chance to specify syslog facility and
 * process identification string - on the first native syslog backend construction.
 * Other native syslog backends will ignore these parameters.
 * Obviously, the \c set_local_address and \c set_target_address
 * methods have no effect for native backends. Using <tt>use_impl = native</tt>
 * on platforms with no native support for POSIX syslog API will have no effect.
 */
class syslog_backend :
    public basic_formatted_sink_backend< char >
{
    //! Base type
    typedef basic_formatted_sink_backend< char > base_type;
    //! Implementation type
    struct implementation;

public:
    //! Character type
    typedef base_type::char_type char_type;
    //! String type that is used to pass message test
    typedef base_type::string_type string_type;

    //! Syslog severity level mapper type
    typedef boost::log::aux::light_function< syslog::level (record_view const&) > severity_mapper_type;

private:
    //! Pointer to the implementation
    implementation* m_pImpl;

public:
    /*!
     * Constructor. Creates a UDP socket-based backend with <tt>syslog::user</tt> facility code.
     * IPv4 protocol will be used.
     */
    BOOST_LOG_API syslog_backend();
    /*!
     * Constructor. Creates a sink backend with the specified named parameters.
     * The following named parameters are supported:
     *
     * \li \c facility - Specifies the facility code. If not specified, <tt>syslog::user</tt> will be used.
     * \li \c use_impl - Specifies the backend implementation. Can be one of:
     *                   \li \c native - Use the native syslog API, if available. If no native API
     *                                   is available, it is equivalent to \c udp_socket_based.
     *                   \li \c udp_socket_based - Use the UDP socket-based implementation, conforming to
     *                                             RFC3164 protocol specification. This is the default.
     * \li \c ip_version - Specifies IP protocol version to use, in case if socket-based implementation
     *                     is used. Can be either \c v4 (the default one) or \c v6.
     * \li \c ident - Process identification string. This parameter is only supported by native syslog implementation.
     */
#ifndef BOOST_LOG_DOXYGEN_PASS
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_CALL(syslog_backend, construct)
#else
    template< typename... ArgsT >
    explicit syslog_backend(ArgsT... const& args);
#endif

    /*!
     * Destructor
     */
    BOOST_LOG_API ~syslog_backend();

    /*!
     * The method installs the function object that maps application severity levels to syslog levels
     */
    BOOST_LOG_API void set_severity_mapper(severity_mapper_type const& mapper);

#if !defined(BOOST_LOG_NO_ASIO)

    /*!
     * The method sets the local host name which log records will be sent from. The host name
     * is resolved to obtain the final IP address.
     *
     * \note Does not have effect if the backend was constructed to use native syslog API
     *
     * \param addr The local address
     * \param port The local port number
     */
    BOOST_LOG_API void set_local_address(std::string const& addr, unsigned short port = 514);
    /*!
     * The method sets the local address which log records will be sent from.
     *
     * \note Does not have effect if the backend was constructed to use native syslog API
     *
     * \param addr The local address
     * \param port The local port number
     */
    BOOST_LOG_API void set_local_address(boost::asio::ip::address const& addr, unsigned short port = 514);

    /*!
     * The method sets the remote host name where log records will be sent to. The host name
     * is resolved to obtain the final IP address.
     *
     * \note Does not have effect if the backend was constructed to use native syslog API
     *
     * \param addr The remote host address
     * \param port The port number on the remote host
     */
    BOOST_LOG_API void set_target_address(std::string const& addr, unsigned short port = 514);
    /*!
     * The method sets the address of the remote host where log records will be sent to.
     *
     * \note Does not have effect if the backend was constructed to use native syslog API
     *
     * \param addr The remote host address
     * \param port The port number on the remote host
     */
    BOOST_LOG_API void set_target_address(boost::asio::ip::address const& addr, unsigned short port = 514);

#endif // !defined(BOOST_LOG_NO_ASIO)

    /*!
     * The method passes the formatted message to the syslog API or sends to a syslog server
     */
    BOOST_LOG_API void consume(record_view const& rec, string_type const& formatted_message);

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! The method creates the backend implementation
    template< typename ArgsT >
    void construct(ArgsT const& args)
    {
        construct(
            args[keywords::facility | syslog::user],
#if !defined(BOOST_LOG_NO_ASIO)
            args[keywords::use_impl | syslog::udp_socket_based],
#else
            args[keywords::use_impl | syslog::native],
#endif
            args[keywords::ip_version | v4],
            args[keywords::ident | std::string()]);
    }
    BOOST_LOG_API void construct(
        syslog::facility facility, syslog::impl_types use_impl, ip_versions ip_version, std::string const& ident);
#endif // BOOST_LOG_DOXYGEN_PASS
};

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_WITHOUT_SYSLOG

#endif // BOOST_LOG_SINKS_SYSLOG_BACKEND_HPP_INCLUDED_
