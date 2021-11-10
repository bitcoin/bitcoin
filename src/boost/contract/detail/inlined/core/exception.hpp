
#ifndef BOOST_CONTRACT_DETAIL_INLINED_EXCEPTION_HPP_
#define BOOST_CONTRACT_DETAIL_INLINED_EXCEPTION_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

// IMPORTANT: Do NOT use config macros BOOST_CONTRACT_... in this file so lib
// .cpp does not need recompiling if config changes (recompile only user code).

#include <boost/contract/core/exception.hpp>
#include <boost/contract/detail/static_local_var.hpp>
#include <boost/contract/detail/declspec.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/config.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <exception>

namespace boost { namespace contract {

BOOST_CONTRACT_DETAIL_DECLINLINE
exception::~exception() BOOST_NOEXCEPT_OR_NOTHROW {}

BOOST_CONTRACT_DETAIL_DECLINLINE
bad_virtual_result_cast::bad_virtual_result_cast(char const* from_type_name,
            char const* to_type_name) {
    std::ostringstream text;
    text
        << "incompatible contracted virtual function result type "
        << "conversion from '" << from_type_name << "' to '"
        << to_type_name  << "'"
    ;
    what_ = text.str();
}

BOOST_CONTRACT_DETAIL_DECLINLINE
bad_virtual_result_cast::~bad_virtual_result_cast() BOOST_NOEXCEPT_OR_NOTHROW {}

BOOST_CONTRACT_DETAIL_DECLINLINE
char const* bad_virtual_result_cast::what() const BOOST_NOEXCEPT_OR_NOTHROW {
    return what_.c_str();
}

BOOST_CONTRACT_DETAIL_DECLINLINE
assertion_failure::assertion_failure(char const* const file,
        unsigned long const line, char const* const code) :
    file_(file), line_(line), code_(code)
{ init(); }

BOOST_CONTRACT_DETAIL_DECLINLINE
assertion_failure::assertion_failure(char const* const code) :
    file_(""), line_(0), code_(code)
{ init(); }

BOOST_CONTRACT_DETAIL_DECLINLINE
assertion_failure::~assertion_failure() BOOST_NOEXCEPT_OR_NOTHROW {}

BOOST_CONTRACT_DETAIL_DECLINLINE
char const* assertion_failure::what() const BOOST_NOEXCEPT_OR_NOTHROW {
    return what_.c_str();
}

BOOST_CONTRACT_DETAIL_DECLINLINE
char const* assertion_failure::file() const { return file_; }

BOOST_CONTRACT_DETAIL_DECLINLINE
unsigned long assertion_failure::line() const { return line_; }

BOOST_CONTRACT_DETAIL_DECLINLINE
char const* assertion_failure::code() const { return code_; }

BOOST_CONTRACT_DETAIL_DECLINLINE
void assertion_failure::init() {
    std::ostringstream text;
    text << "assertion";
    if(std::string(code_) != "") text << " \"" << code_ << "\"";
    text << " failed";
    if(std::string(file_) != "") {
        text << ": file \"" << file_ << "\"";
        if(line_ != 0) text << ", line " << line_;
    }
    what_ = text.str();
}

namespace exception_ {
    enum failure_key {
        check_failure_key,
        pre_failure_key,
        post_failure_key,
        except_failure_key,
        old_failure_key,
        entry_inv_failure_key,
        exit_inv_failure_key
    };

    template<failure_key Key>
    void default_handler() {
        std::string k = "";
        switch(Key) {
            case check_failure_key: k = "check "; break;
            case pre_failure_key: k = "precondition "; break;
            case post_failure_key: k = "postcondition "; break;
            case except_failure_key: k = "except "; break;
            case old_failure_key: k = "old copy "; break;
            case entry_inv_failure_key: k = "entry invariant "; break;
            case exit_inv_failure_key: k = "exit invariant "; break;
            // No default (so compiler warning/error on missing enum case).
        }
        try { throw; }
        catch(boost::contract::assertion_failure const& error) {
            // what = "assertion '...' failed: ...".
            std::cerr << k << error.what() << std::endl;
        } catch(...) { // old_failure_key prints this, not above.
            std::cerr << k << "threw following exception:" << std::endl
                    << boost::current_exception_diagnostic_information();
        }
        std::terminate(); // Default handlers log and call terminate.
    }
    
    template<failure_key Key>
    void default_from_handler(from) { default_handler<Key>(); }

    // Check failure.

    struct check_failure_mutex_tag;
    typedef boost::contract::detail::static_local_var<check_failure_mutex_tag,
            boost::mutex> check_failure_mutex;

    struct check_failure_handler_tag;
    typedef boost::contract::detail::static_local_var_init<
        check_failure_handler_tag,
        failure_handler, 
        void (*)(),
        &default_handler<check_failure_key>
    > check_failure_handler;

    BOOST_CONTRACT_DETAIL_DECLINLINE
    failure_handler const& set_check_failure_unlocked(failure_handler const& f)
            BOOST_NOEXCEPT_OR_NOTHROW {
        check_failure_handler::ref() = f;
        return f;
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    failure_handler const& set_check_failure_locked(failure_handler const& f)
            BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(check_failure_mutex::ref());
        return set_check_failure_unlocked(f);
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    failure_handler get_check_failure_unlocked() BOOST_NOEXCEPT_OR_NOTHROW {
        return check_failure_handler::ref();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    failure_handler get_check_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(check_failure_mutex::ref());
        return get_check_failure_unlocked();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    void check_failure_unlocked() /* can throw */ {
        check_failure_handler::ref()();
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    void check_failure_locked() /* can throw */ {
        boost::lock_guard<boost::mutex> lock(check_failure_mutex::ref());
        check_failure_unlocked();
    }
    
    // Precondition failure.

    struct pre_failure_mutex_tag;
    typedef boost::contract::detail::static_local_var<pre_failure_mutex_tag,
            boost::mutex> pre_failure_mutex;

    struct pre_failure_handler_tag;
    typedef boost::contract::detail::static_local_var_init<
        pre_failure_handler_tag,
        from_failure_handler,
        void (*)(from),
        &default_from_handler<pre_failure_key>
    > pre_failure_handler;

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_pre_failure_unlocked(from_failure_handler
            const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        pre_failure_handler::ref() = f;
        return f;
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_pre_failure_locked(from_failure_handler
            const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(pre_failure_mutex::ref());
        return set_pre_failure_unlocked(f);
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_pre_failure_unlocked() BOOST_NOEXCEPT_OR_NOTHROW {
        return pre_failure_handler::ref();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_pre_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(pre_failure_mutex::ref());
        return get_pre_failure_unlocked();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    void pre_failure_unlocked(from where) /* can throw */ {
        pre_failure_handler::ref()(where);
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    void pre_failure_locked(from where) /* can throw */ {
        boost::lock_guard<boost::mutex> lock(pre_failure_mutex::ref());
        pre_failure_unlocked(where);
    }
    
    // Postcondition failure.

    struct post_failure_mutex_tag;
    typedef boost::contract::detail::static_local_var<post_failure_mutex_tag,
            boost::mutex> post_failure_mutex;

    struct post_failure_handler_tag;
    typedef boost::contract::detail::static_local_var_init<
        post_failure_handler_tag,
        from_failure_handler,
        void (*)(from),
        &default_from_handler<post_failure_key>
    > post_failure_handler;

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_post_failure_unlocked(from_failure_handler
            const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        post_failure_handler::ref() = f;
        return f;
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_post_failure_locked(from_failure_handler
            const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(post_failure_mutex::ref());
        return set_post_failure_unlocked(f);
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_post_failure_unlocked() BOOST_NOEXCEPT_OR_NOTHROW {
        return post_failure_handler::ref();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_post_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(post_failure_mutex::ref());
        return get_post_failure_unlocked();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    void post_failure_unlocked(from where) /* can throw */ {
        post_failure_handler::ref()(where);
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    void post_failure_locked(from where) /* can throw */ {
        boost::lock_guard<boost::mutex> lock(post_failure_mutex::ref());
        post_failure_unlocked(where);
    }
    
    // Except failure.

    struct except_failure_mutex_tag;
    typedef boost::contract::detail::static_local_var<except_failure_mutex_tag,
            boost::mutex> except_failure_mutex;

    struct except_failure_handler_tag;
    typedef boost::contract::detail::static_local_var_init<
        except_failure_handler_tag,
        from_failure_handler,
        void (*)(from),
        &default_from_handler<except_failure_key>
    > except_failure_handler;

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_except_failure_unlocked(from_failure_handler
            const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        except_failure_handler::ref() = f;
        return f;
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_except_failure_locked(from_failure_handler
            const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(except_failure_mutex::ref());
        return set_except_failure_unlocked(f);
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_except_failure_unlocked()
            BOOST_NOEXCEPT_OR_NOTHROW {
        return except_failure_handler::ref();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_except_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(except_failure_mutex::ref());
        return get_except_failure_unlocked();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    void except_failure_unlocked(from where) /* can throw */ {
        except_failure_handler::ref()(where);
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    void except_failure_locked(from where) /* can throw */ {
        boost::lock_guard<boost::mutex> lock(except_failure_mutex::ref());
        except_failure_unlocked(where);
    }

    // Old-copy failure.

    struct old_failure_mutex_tag;
    typedef boost::contract::detail::static_local_var<old_failure_mutex_tag,
            boost::mutex> old_failure_mutex;

    struct old_failure_handler_tag;
    typedef boost::contract::detail::static_local_var_init<
        old_failure_handler_tag,
        from_failure_handler,
        void (*)(from),
        &default_from_handler<old_failure_key>
    > old_failure_handler;

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_old_failure_unlocked(from_failure_handler
            const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        old_failure_handler::ref() = f;
        return f;
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_old_failure_locked(from_failure_handler
            const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(old_failure_mutex::ref());
        return set_old_failure_unlocked(f);
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_old_failure_unlocked() BOOST_NOEXCEPT_OR_NOTHROW {
        return old_failure_handler::ref();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_old_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(old_failure_mutex::ref());
        return get_old_failure_unlocked();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    void old_failure_unlocked(from where) /* can throw */ {
        old_failure_handler::ref()(where);
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    void old_failure_locked(from where) /* can throw */ {
        boost::lock_guard<boost::mutex> lock(old_failure_mutex::ref());
        old_failure_unlocked(where);
    }
    
    // Entry invariant failure.

    struct entry_inv_failure_mutex_tag;
    typedef boost::contract::detail::static_local_var<
            entry_inv_failure_mutex_tag, boost::mutex> entry_inv_failure_mutex;

    struct entry_inv_failure_handler_tag;
    typedef boost::contract::detail::static_local_var_init<
        entry_inv_failure_handler_tag,
        from_failure_handler,
        void (*)(from),
        &default_from_handler<entry_inv_failure_key>
    > entry_inv_failure_handler;

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_entry_inv_failure_unlocked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        entry_inv_failure_handler::ref() = f;
        return f;
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_entry_inv_failure_locked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(entry_inv_failure_mutex::ref());
        return set_entry_inv_failure_unlocked(f);
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_entry_inv_failure_unlocked()
            BOOST_NOEXCEPT_OR_NOTHROW {
        return entry_inv_failure_handler::ref();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_entry_inv_failure_locked()
            BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(entry_inv_failure_mutex::ref());
        return get_entry_inv_failure_unlocked();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    void entry_inv_failure_unlocked(from where) /* can throw */ {
        entry_inv_failure_handler::ref()(where);
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    void entry_inv_failure_locked(from where) /* can throw */ {
        boost::lock_guard<boost::mutex> lock(entry_inv_failure_mutex::ref());
        entry_inv_failure_unlocked(where);
    }
    
    // Exit invariant failure.

    struct exit_inv_failure_mutex_tag;
    typedef boost::contract::detail::static_local_var<
            exit_inv_failure_mutex_tag, boost::mutex> exit_inv_failure_mutex;

    struct exit_inv_failure_handler_tag;
    typedef boost::contract::detail::static_local_var_init<
        exit_inv_failure_handler_tag,
        from_failure_handler,
        void (*)(from),
        &default_from_handler<exit_inv_failure_key>
    > exit_inv_failure_handler;

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_exit_inv_failure_unlocked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        exit_inv_failure_handler::ref() = f;
        return f;
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler const& set_exit_inv_failure_locked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(exit_inv_failure_mutex::ref());
        return set_exit_inv_failure_unlocked(f);
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_exit_inv_failure_unlocked()
            BOOST_NOEXCEPT_OR_NOTHROW {
        return exit_inv_failure_handler::ref();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    from_failure_handler get_exit_inv_failure_locked()
            BOOST_NOEXCEPT_OR_NOTHROW {
        boost::lock_guard<boost::mutex> lock(exit_inv_failure_mutex::ref());
        return get_exit_inv_failure_unlocked();
    }

    BOOST_CONTRACT_DETAIL_DECLINLINE
    void exit_inv_failure_unlocked(from where) /* can throw */ {
        exit_inv_failure_handler::ref()(where);
    }
    
    BOOST_CONTRACT_DETAIL_DECLINLINE
    void exit_inv_failure_locked(from where) /* can throw */ {
        boost::lock_guard<boost::mutex> lock(exit_inv_failure_mutex::ref());
        exit_inv_failure_unlocked(where);
    }
}

from_failure_handler const& set_entry_invariant_failure(
        from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;

from_failure_handler const& set_exit_invariant_failure(
        from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;

BOOST_CONTRACT_DETAIL_DECLINLINE
from_failure_handler const& set_invariant_failure(
        from_failure_handler const& f)  BOOST_NOEXCEPT_OR_NOTHROW {
    set_entry_invariant_failure(f);
    set_exit_invariant_failure(f);
    return f;
}

} } // namespace

#endif // #include guard

