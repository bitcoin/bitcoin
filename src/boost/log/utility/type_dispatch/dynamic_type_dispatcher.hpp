/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   dynamic_type_dispatcher.hpp
 * \author Andrey Semashev
 * \date   15.04.2007
 *
 * The header contains implementation of the run-time type dispatcher.
 */

#ifndef BOOST_LOG_DYNAMIC_TYPE_DISPATCHER_HPP_INCLUDED_
#define BOOST_LOG_DYNAMIC_TYPE_DISPATCHER_HPP_INCLUDED_

#include <new>
#include <memory>
#include <map>
#include <boost/ref.hpp>
#include <boost/type_index.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/utility/type_dispatch/type_dispatcher.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief A dynamic type dispatcher
 *
 * The type dispatcher can be used to pass objects of arbitrary types from one
 * component to another. With regard to the library, the type dispatcher
 * can be used to extract attribute values.
 *
 * The dynamic type dispatcher can be initialized in run time and, therefore,
 * can support different types, depending on runtime conditions. Each
 * supported type is associated with a functional object that will be called
 * when an object of the type is dispatched.
 */
class dynamic_type_dispatcher :
    public type_dispatcher
{
private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    template< typename T, typename VisitorT >
    class callback_impl :
        public callback_base
    {
    private:
        VisitorT m_Visitor;

    public:
        explicit callback_impl(VisitorT const& visitor) : m_Visitor(visitor)
        {
            this->m_pVisitor = (void*)boost::addressof(m_Visitor);
            typedef void (*trampoline_t)(void*, T const&);
            BOOST_STATIC_ASSERT_MSG(sizeof(trampoline_t) == sizeof(void*), "Boost.Log: Unsupported platform, the size of a function pointer differs from the size of a pointer");
            union
            {
                void* as_pvoid;
                trampoline_t as_trampoline;
            }
            caster;
            caster.as_trampoline = (trampoline_t)&callback_base::trampoline< VisitorT, T >;
            this->m_pTrampoline = caster.as_pvoid;
        }
    };
#endif // BOOST_LOG_DOXYGEN_PASS

    //! The dispatching map
    typedef std::map< typeindex::type_index, shared_ptr< callback_base > > dispatching_map;
    dispatching_map m_DispatchingMap;

public:
    /*!
     * Default constructor
     */
    dynamic_type_dispatcher() : type_dispatcher(&dynamic_type_dispatcher::get_callback)
    {
    }

    /*!
     * Copy constructor
     */
    dynamic_type_dispatcher(dynamic_type_dispatcher const& that) :
        type_dispatcher(static_cast< type_dispatcher const& >(that)),
        m_DispatchingMap(that.m_DispatchingMap)
    {
    }

    /*!
     * Copy assignment
     */
    dynamic_type_dispatcher& operator= (dynamic_type_dispatcher const& that)
    {
        m_DispatchingMap = that.m_DispatchingMap;
        return *this;
    }

    /*!
     * The method registers a new type
     *
     * \param visitor Function object that will be associated with the type \c T
     */
    template< typename T, typename VisitorT >
    void register_type(VisitorT const& visitor)
    {
        boost::shared_ptr< callback_base > p(
            boost::make_shared< callback_impl< T, VisitorT > >(boost::cref(visitor)));

        typeindex::type_index wrapper(typeindex::type_id< T >());
        m_DispatchingMap[wrapper].swap(p);
    }

    /*!
     * The method returns the number of registered types
     */
    dispatching_map::size_type registered_types_count() const
    {
        return m_DispatchingMap.size();
    }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    static callback_base get_callback(type_dispatcher* p, typeindex::type_index type)
    {
        dynamic_type_dispatcher* const self = static_cast< dynamic_type_dispatcher* >(p);
        dispatching_map::iterator it = self->m_DispatchingMap.find(type);
        if (it != self->m_DispatchingMap.end())
            return *it->second;
        else
            return callback_base();
    }
#endif // BOOST_LOG_DOXYGEN_PASS
};

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DYNAMIC_TYPE_DISPATCHER_HPP_INCLUDED_
