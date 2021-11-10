#ifndef BOOST_SERIALIZATION_ACCESS_HPP
#define BOOST_SERIALIZATION_ACCESS_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// access.hpp: interface for serialization system.

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>

namespace boost {

namespace archive {
namespace detail {
    template<class Archive, class T>
    class iserializer;
    template<class Archive, class T>
    class oserializer;
} // namespace detail
} // namespace archive

namespace serialization {

// forward declarations
template<class Archive, class T>
inline void serialize_adl(Archive &, T &, const unsigned int);
namespace detail {
    template<class Archive, class T>
    struct member_saver;
    template<class Archive, class T>
    struct member_loader;
} // namespace detail

// use an "accessor class so that we can use:
// "friend class boost::serialization::access;"
// in any serialized class to permit clean, safe access to private class members
// by the serialization system

class access {
public:
    // grant access to "real" serialization defaults
#ifdef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
    template<class Archive, class T>
    friend struct detail::member_saver;
    template<class Archive, class T>
    friend struct detail::member_loader;
    template<class Archive, class T>
    friend class archive::detail::iserializer;
    template<class Archive, class T>
    friend class archive::detail::oserializer;
    template<class Archive, class T>
    friend inline void serialize(
        Archive & ar,
        T & t,
        const unsigned int file_version
    );
    template<class Archive, class T>
    friend inline void save_construct_data(
        Archive & ar,
        const T * t,
        const unsigned int file_version
    );
    template<class Archive, class T>
    friend inline void load_construct_data(
        Archive & ar,
        T * t,
        const unsigned int file_version
    );
#endif

    // pass calls to users's class implementation
    template<class Archive, class T>
    static void member_save(
        Archive & ar,
        //const T & t,
        T & t,
        const unsigned int file_version
    ){
        t.save(ar, file_version);
    }
    template<class Archive, class T>
    static void member_load(
        Archive & ar,
        T & t,
        const unsigned int file_version
    ){
        t.load(ar, file_version);
    }
    template<class Archive, class T>
    static void serialize(
        Archive & ar,
        T & t,
        const unsigned int file_version
    ){
        // note: if you get a compile time error here with a
        // message something like:
        // cannot convert parameter 1 from <file type 1> to <file type 2 &>
        // a likely possible cause is that the class T contains a
        // serialize function - but that serialize function isn't
        // a template and corresponds to a file type different than
        // the class Archive.  To resolve this, don't include an
        // archive type other than that for which the serialization
        // function is defined!!!
        t.serialize(ar, file_version);
    }
    template<class T>
    static void destroy( const T * t) // const appropriate here?
    {
        // the const business is an MSVC 6.0 hack that should be
        // benign on everything else
        delete const_cast<T *>(t);
    }
    template<class T>
    static void construct(T * t){
        // default is inplace invocation of default constructor
        // Note the :: before the placement new. Required if the
        // class doesn't have a class-specific placement new defined.
        ::new(t)T;
    }
    template<class T, class U>
    static T & cast_reference(U & u){
        return static_cast<T &>(u);
    }
    template<class T, class U>
    static T * cast_pointer(U * u){
        return static_cast<T *>(u);
    }
};

} // namespace serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_ACCESS_HPP
