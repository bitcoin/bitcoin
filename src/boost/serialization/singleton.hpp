#ifndef BOOST_SERIALIZATION_SINGLETON_HPP
#define BOOST_SERIALIZATION_SINGLETON_HPP

/////////1/////////2///////// 3/////////4/////////5/////////6/////////7/////////8
//  singleton.hpp
//
// Copyright David Abrahams 2006. Original version
//
// Copyright Robert Ramey 2007.  Changes made to permit
// application throughout the serialization library.
//
// Copyright Alexander Grund 2018. Corrections to singleton lifetime
//
// Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// The intention here is to define a template which will convert
// any class into a singleton with the following features:
//
// a) initialized before first use.
// b) thread-safe for const access to the class
// c) non-locking
//
// In order to do this,
// a) Initialize dynamically when used.
// b) Require that all singletons be initialized before main
// is called or any entry point into the shared library is invoked.
// This guarentees no race condition for initialization.
// In debug mode, we assert that no non-const functions are called
// after main is invoked.
//

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/noncopyable.hpp>
#include <boost/serialization/force_include.hpp>
#include <boost/serialization/config.hpp>

#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

namespace boost {
namespace serialization {

//////////////////////////////////////////////////////////////////////
// Provides a dynamically-initialized (singleton) instance of T in a
// way that avoids LNK1179 on vc6.  See http://tinyurl.com/ljdp8 or
// http://lists.boost.org/Archives/boost/2006/05/105286.php for
// details.
//

// Singletons created by this code are guaranteed to be unique
// within the executable or shared library which creates them.
// This is sufficient and in fact ideal for the serialization library.
// The singleton is created when the module is loaded and destroyed
// when the module is unloaded.

// This base class has two functions.

// First it provides a module handle for each singleton indicating
// the executable or shared library in which it was created. This
// turns out to be necessary and sufficient to implement the tables
// used by serialization library.

// Second, it provides a mechanism to detect when a non-const function
// is called after initialization.

// Make a singleton to lock/unlock all singletons for alteration.
// The intent is that all singletons created/used by this code
// are to be initialized before main is called. A test program
// can lock all the singletons when main is entered.  Thus any
// attempt to retrieve a mutable instance while locked will
// generate an assertion if compiled for debug.

// The singleton template can be used in 2 ways:
// 1 (Recommended): Publicly inherit your type T from singleton<T>,
// make its ctor protected and access it via T::get_const_instance()
// 2: Simply access singleton<T> without changing T. Note that this only
// provides a global instance accesible by singleton<T>::get_const_instance()
// or singleton<T>::get_mutable_instance() to prevent using multiple instances
// of T make its ctor protected

// Note on usage of BOOST_DLLEXPORT: These functions are in danger of
// being eliminated by the optimizer when building an application in
// release mode. Usage of the macro is meant to signal the compiler/linker
// to avoid dropping these functions which seem to be unreferenced.
// This usage is not related to autolinking.

class BOOST_SYMBOL_VISIBLE singleton_module :
    public boost::noncopyable
{
private:
    BOOST_DLLEXPORT bool & get_lock() BOOST_USED {
        static bool lock = false;
        return lock;
    }

public:
    BOOST_DLLEXPORT void lock(){
        get_lock() = true;
    }
    BOOST_DLLEXPORT void unlock(){
        get_lock() = false;
    }
    BOOST_DLLEXPORT bool is_locked(){
        return get_lock();
    }
};

static inline singleton_module & get_singleton_module(){
    static singleton_module m;
    return m;
}

namespace detail {

// This is the class actually instantiated and hence the real singleton.
// So there will only be one instance of this class. This does not hold
// for singleton<T> as a class derived from singleton<T> could be
// instantiated multiple times.
// It also provides a flag `is_destroyed` which returns true, when the
// class was destructed. It is static and hence accesible even after
// destruction. This can be used to check, if the singleton is still
// accesible e.g. in destructors of other singletons.
template<class T>
class singleton_wrapper : public T
{
    static bool & get_is_destroyed(){
        // Prefer a static function member to avoid LNK1179.
        // Note: As this is for a singleton (1 instance only) it must be set
        // never be reset (to false)!
        static bool is_destroyed_flag = false;
        return is_destroyed_flag;
    }
public:
    singleton_wrapper(){
        BOOST_ASSERT(! is_destroyed());
    }
    ~singleton_wrapper(){
        get_is_destroyed() = true;
    }
    static bool is_destroyed(){
        return get_is_destroyed();
    }
};

} // detail

template <class T>
class singleton {
private:
    static T * m_instance;
    // include this to provoke instantiation at pre-execution time
    static void use(T const &) {}
    static T & get_instance() {
        BOOST_ASSERT(! is_destroyed());

        // use a wrapper so that types T with protected constructors can be used
        // Using a static function member avoids LNK1179
        static detail::singleton_wrapper< T > t;

        // note that the following is absolutely essential.
        // commenting out this statement will cause compilers to fail to
        // construct the instance at pre-execution time.  This would prevent
        // our usage/implementation of "locking" and introduce uncertainty into
        // the sequence of object initialization.
        // Unfortunately, this triggers detectors of undefine behavior
        // and reports an error.  But I've been unable to find a different
        // of guarenteeing that the the singleton is created at pre-main time.
        if (m_instance) use(* m_instance);

        return static_cast<T &>(t);
    }
protected:
    // Do not allow instantiation of a singleton<T>. But we want to allow
    // `class T: public singleton<T>` so we can't delete this ctor
    BOOST_DLLEXPORT singleton(){}

public:
    BOOST_DLLEXPORT static T & get_mutable_instance(){
        BOOST_ASSERT(! get_singleton_module().is_locked());
        return get_instance();
    }
    BOOST_DLLEXPORT static const T & get_const_instance(){
        return get_instance();
    }
    BOOST_DLLEXPORT static bool is_destroyed(){
        return detail::singleton_wrapper< T >::is_destroyed();
    }
};

// Assigning the instance reference to a static member forces initialization
// at startup time as described in
// https://groups.google.com/forum/#!topic/microsoft.public.vc.language/kDVNLnIsfZk
template<class T>
T * singleton< T >::m_instance = & singleton< T >::get_instance();

} // namespace serialization
} // namespace boost

#include <boost/archive/detail/abi_suffix.hpp> // pops abi_suffix.hpp pragmas

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif // BOOST_SERIALIZATION_SINGLETON_HPP
