#ifndef BOOST_SERIALIZATION_SHARED_PTR_HELPER_HPP
#define BOOST_SERIALIZATION_SHARED_PTR_HELPER_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// shared_ptr_helper.hpp: serialization for boost shared pointern

// (C) Copyright 2004-2009 Robert Ramey, Martin Ecker and Takatoshi Kondo
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <map>
#include <list>
#include <utility>
#include <cstddef> // NULL

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_polymorphic.hpp>
#include <boost/mpl/if.hpp>

#include <boost/serialization/singleton.hpp>
#include <boost/serialization/extended_type_info.hpp>
#include <boost/serialization/throw_exception.hpp>
#include <boost/serialization/type_info_implementation.hpp>
#include <boost/archive/archive_exception.hpp>

namespace boost_132 {
    template<class T> class shared_ptr;
}
namespace boost {
namespace serialization {

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
template<class Archive, template<class U> class SPT >
void load(
    Archive & ar,
    SPT< class U > &t,
    const unsigned int file_version
);
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// a common class for holding various types of shared pointers

template<template<class T> class SPT>
class shared_ptr_helper {
    typedef std::map<
        const void *, // address of object
        SPT<const void> // address shared ptr to single instance
    > object_shared_pointer_map;

    // list of shared_pointers create accessable by raw pointer. This
    // is used to "match up" shared pointers loaded at different
    // points in the archive. Note, we delay construction until
    // it is actually used since this is by default included as
    // a "mix-in" even if shared_ptr isn't used.
    object_shared_pointer_map * m_o_sp;

    struct null_deleter {
        void operator()(void const *) const {}
    };

#if defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS) \
|| defined(BOOST_MSVC) \
|| defined(__SUNPRO_CC)
public:
#else
    template<class Archive, class U>
    friend void boost::serialization::load(
        Archive & ar,
        SPT< U > &t,
        const unsigned int file_version
    );
#endif

    #ifdef BOOST_SERIALIZATION_SHARED_PTR_132_HPP
    // list of loaded pointers.  This is used to be sure that the pointers
    // stay around long enough to be "matched" with other pointers loaded
    // by the same archive.  These are created with a "null_deleter" so that
    // when this list is destroyed - the underlaying raw pointers are not
    // destroyed.  This has to be done because the pointers are also held by
    // new system which is disjoint from this set.  This is implemented
    // by a change in load_construct_data below.  It makes this file suitable
    // only for loading pointers into a 1.33 or later boost system.
    std::list<boost_132::shared_ptr<const void> > * m_pointers_132;
    void
    append(const boost_132::shared_ptr<const void> & t){
        if(NULL == m_pointers_132)
            m_pointers_132 = new std::list<boost_132::shared_ptr<const void> >;
        m_pointers_132->push_back(t);
    }
    #endif

    struct non_polymorphic {
        template<class U>
        static const boost::serialization::extended_type_info *
        get_object_type(U & ){
            return & boost::serialization::singleton<
                typename
                boost::serialization::type_info_implementation< U >::type
            >::get_const_instance();
        }
    };
    struct polymorphic {
        template<class U>
        static const boost::serialization::extended_type_info *
        get_object_type(U & u){
            return boost::serialization::singleton<
                typename
                boost::serialization::type_info_implementation< U >::type
            >::get_const_instance().get_derived_extended_type_info(u);
        }
    };

public:
    template<class T>
    void reset(SPT< T > & s, T * t){
        if(NULL == t){
            s.reset();
            return;
        }
        const boost::serialization::extended_type_info * this_type
            = & boost::serialization::type_info_implementation< T >::type
                    ::get_const_instance();

        // get pointer to the most derived object's eti.  This is effectively
        // the object type identifer
        typedef typename mpl::if_<
            is_polymorphic< T >,
            polymorphic,
            non_polymorphic
        >::type type;

        const boost::serialization::extended_type_info * true_type
            = type::get_object_type(*t);

        // note:if this exception is thrown, be sure that derived pointern
        // is either registered or exported.
        if(NULL == true_type)
            boost::serialization::throw_exception(
                boost::archive::archive_exception(
                    boost::archive::archive_exception::unregistered_class,
                    this_type->get_debug_info()
                )
            );
        // get void pointer to the most derived type
        // this uniquely identifies the object referred to
        // oid = "object identifier"
        const void * oid = void_downcast(
            *true_type,
            *this_type,
            t
        );
        if(NULL == oid)
            boost::serialization::throw_exception(
                boost::archive::archive_exception(
                    boost::archive::archive_exception::unregistered_cast,
                    true_type->get_debug_info(),
                    this_type->get_debug_info()
                )
            );

        // make tracking array if necessary
        if(NULL == m_o_sp)
            m_o_sp = new object_shared_pointer_map;

        typename object_shared_pointer_map::iterator i = m_o_sp->find(oid);

        // if it's a new object
        if(i == m_o_sp->end()){
            s.reset(t);
            std::pair<typename object_shared_pointer_map::iterator, bool> result;
            result = m_o_sp->insert(std::make_pair(oid, s));
            BOOST_ASSERT(result.second);
        }
        // if the object has already been seen
        else{
            s = SPT<T>(i->second, t);
        }
    }

    shared_ptr_helper() :
        m_o_sp(NULL)
        #ifdef BOOST_SERIALIZATION_SHARED_PTR_132_HPP
            , m_pointers_132(NULL)
        #endif
    {}
    virtual ~shared_ptr_helper(){
        if(NULL != m_o_sp)
            delete m_o_sp;
        #ifdef BOOST_SERIALIZATION_SHARED_PTR_132_HPP
        if(NULL != m_pointers_132)
            delete m_pointers_132;
        #endif
    }
};

} // namespace serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_SHARED_PTR_HELPER_HPP
