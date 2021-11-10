
#ifndef BOOST_CONTRACT_DETAIL_CHECKING_HPP_
#define BOOST_CONTRACT_DETAIL_CHECKING_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

#include <boost/contract/core/config.hpp>
#include <boost/contract/detail/static_local_var.hpp>
#include <boost/contract/detail/declspec.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>
#include <boost/config.hpp>

namespace boost { namespace contract { namespace detail {

#ifdef BOOST_MSVC
    #pragma warning(push)
    #pragma warning(disable: 4275) // Base w/o DLL spec (noncopyable).
    #pragma warning(disable: 4251) // Member w/o DLL spec (mutex_ type).
#endif

// RAII facility to disable assertions while checking other assertions.
class BOOST_CONTRACT_DETAIL_DECLSPEC checking :
    private boost::noncopyable // Non-copyable resource (might use mutex, etc.).
{
public:
    explicit checking() {
        #ifndef BOOST_CONTRACT_DISABLE_THREADS
            init_locked();
        #else
            init_unlocked();
        #endif
    }

    ~checking() {
        #ifndef BOOST_CONTRACT_DISABLE_THREADS
            done_locked();
        #else
            done_unlocked();
        #endif
    }
    
    static bool already() {
        #ifndef BOOST_CONTRACT_DISABLE_THREADS
            return already_locked();
        #else
            return already_unlocked();
        #endif
    }

private:
    void init_unlocked();
    void init_locked();

    void done_unlocked();
    void done_locked();

    static bool already_unlocked();
    static bool already_locked();

    struct mutex_tag;
    typedef static_local_var<mutex_tag, boost::mutex> mutex;

    struct checking_tag;
    typedef static_local_var_init<checking_tag, bool, bool, false> flag;
};

#ifdef BOOST_MSVC
    #pragma warning(pop)
#endif

} } } // namespace

#ifdef BOOST_CONTRACT_HEADER_ONLY
    #include <boost/contract/detail/inlined/detail/checking.hpp>
#endif

#endif // #include guard

