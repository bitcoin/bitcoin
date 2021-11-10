/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   unlocked_frontend.hpp
 * \author Andrey Semashev
 * \date   14.07.2009
 *
 * The header contains declaration of an unlocked sink frontend.
 */

#ifndef BOOST_LOG_SINKS_UNLOCKED_FRONTEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_UNLOCKED_FRONTEND_HPP_INCLUDED_

#include <boost/static_assert.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/detail/fake_mutex.hpp>
#include <boost/log/sinks/basic_sink_frontend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_1(z, n, data)\
    template< typename T0 >\
    explicit unlocked_sink(T0 const& arg0, typename boost::log::aux::enable_if_named_parameters< T0, boost::log::aux::sfinae_dummy >::type = boost::log::aux::sfinae_dummy()) :\
        base_type(false),\
        m_pBackend(boost::make_shared< sink_backend_type >(arg0)) {}

#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_N(z, n, data)\
    template< BOOST_PP_ENUM_PARAMS_Z(z, n, typename T) >\
    explicit unlocked_sink(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, T, const& arg)) :\
        base_type(false),\
        m_pBackend(boost::make_shared< sink_backend_type >(BOOST_PP_ENUM_PARAMS_Z(z, n, arg))) {}

#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL(z, n, data)\
    BOOST_PP_IF(BOOST_PP_EQUAL(n, 1), BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_1, BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_N)(z, n, data)

#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief Non-blocking logging sink frontend
 *
 * The sink frontend does not perform thread synchronization and
 * simply passes logging records to the sink backend.
 */
template< typename SinkBackendT >
class unlocked_sink :
    public aux::make_sink_frontend_base< SinkBackendT >::type
{
    typedef typename aux::make_sink_frontend_base< SinkBackendT >::type base_type;

public:
    //! Sink implementation type
    typedef SinkBackendT sink_backend_type;
    //! \cond
    BOOST_STATIC_ASSERT_MSG((has_requirement< typename sink_backend_type::frontend_requirements, concurrent_feeding >::value), "Unlocked sink frontend is incompatible with the specified backend: thread synchronization requirements are not met");
    //! \endcond

    //! Type of pointer to the backend
    typedef shared_ptr< sink_backend_type > locked_backend_ptr;

private:
    //! Pointer to the backend
    const shared_ptr< sink_backend_type > m_pBackend;

public:
    /*!
     * Default constructor. Constructs the sink backend instance.
     * Requires the backend to be default-constructible.
     */
    unlocked_sink() :
        base_type(false),
        m_pBackend(boost::make_shared< sink_backend_type >())
    {
    }
    /*!
     * Constructor attaches user-constructed backend instance
     *
     * \param backend Pointer to the backend instance
     *
     * \pre \a backend is not \c NULL.
     */
    explicit unlocked_sink(shared_ptr< sink_backend_type > const& backend) :
        base_type(false),
        m_pBackend(backend)
    {
    }

    /*!
     * Constructor that passes arbitrary named parameters to the interprocess sink backend constructor.
     * Refer to the backend documentation for the list of supported parameters.
     */
#ifndef BOOST_LOG_DOXYGEN_PASS
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_GEN(BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL, ~)
#else
    template< typename... Args >
    explicit unlocked_sink(Args&&... args);
#endif

    /*!
     * Locking accessor to the attached backend.
     *
     * \note Does not do any actual locking, provided only for interface consistency
     *       with other frontends.
     */
    locked_backend_ptr locked_backend()
    {
        return m_pBackend;
    }

    /*!
     * Passes the log record to the backend
     */
    void consume(record_view const& rec)
    {
        boost::log::aux::fake_mutex m;
        base_type::feed_record(rec, m, *m_pBackend);
    }

    /*!
     * The method performs flushing of any internal buffers that may hold log records. The method
     * may take considerable time to complete and may block both the calling thread and threads
     * attempting to put new records into the sink while this call is in progress.
     */
    void flush()
    {
        boost::log::aux::fake_mutex m;
        base_type::flush_backend(m, *m_pBackend);
    }
};

#undef BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_1
#undef BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL_N
#undef BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_SINKS_UNLOCKED_FRONTEND_HPP_INCLUDED_
