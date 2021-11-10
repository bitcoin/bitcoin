
#ifndef BOOST_CONTRACT_DETAIL_COND_BASE_HPP_
#define BOOST_CONTRACT_DETAIL_COND_BASE_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

// NOTE: It seemed not possible to implement this library without inheritance
// here because some sort of base type needs to be used to hold contract objects
// in instances of boost::contract::check while polymorphically calling
// init and destructor functions to check contracts at entry and exit. This
// could be possible without inheritance only if boost::contract::check was made
// a template type but that would complicate user code. In any case, early
// experimentation with removing this base class and its virtual methods did not
// seem to reduce compilation and/or run time.

#include <boost/contract/core/exception.hpp>
#include <boost/contract/core/config.hpp>
#if     !defined(BOOST_CONTRACT_NO_PRECONDITIONS) || \
        !defined(BOOST_CONTRACT_NO_OLDS) || \
        !defined(BOOST_CONTRACT_NO_EXEPTS)
    #include <boost/function.hpp>
#endif
#include <boost/noncopyable.hpp>
#ifndef BOOST_CONTRACT_ON_MISSING_CHECK_DECL
    #include <boost/assert.hpp>
#endif
#include <boost/config.hpp>

namespace boost { namespace contract { namespace detail {

class cond_base : // Base to hold all contract objects for RAII.
    private boost::noncopyable // Avoid copying possible user's ftor captures.
{
public:
    explicit cond_base(boost::contract::from from) :
          BOOST_CONTRACT_ERROR_missing_check_object_declaration(false)
        , init_asserted_(false)
        #ifndef BOOST_CONTRACT_NO_CONDITIONS
            , from_(from)
            , failed_(false)
        #endif
    {}
    
    // Can override for checking on exit, but should call assert_initialized().
    virtual ~cond_base() BOOST_NOEXCEPT_IF(false) {
        // Catch error (but later) even if overrides miss assert_initialized().
        if(!init_asserted_) assert_initialized();
    }

    void initialize() { // Must be called by owner ctor (i.e., check class).
        BOOST_CONTRACT_ERROR_missing_check_object_declaration = true;
        this->init(); // So all inits (pre, old, post) done after owner decl.
    }
    
    #ifndef BOOST_CONTRACT_NO_PRECONDITIONS
        template<typename F>
        void set_pre(F const& f) { pre_ = f; }
    #endif

    #ifndef BOOST_CONTRACT_NO_OLDS
        template<typename F>
        void set_old(F const& f) { old_ = f; }
    #endif

    #ifndef BOOST_CONTRACT_NO_EXCEPTS
        template<typename F>
        void set_except(F const& f) { except_ = f; }
    #endif

protected:
    void assert_initialized() { // Derived dtors must assert this at entry.
        init_asserted_ = true;
        #ifdef BOOST_CONTRACT_ON_MISSING_CHECK_DECL
            if(!BOOST_CONTRACT_ERROR_missing_check_object_declaration) {
                BOOST_CONTRACT_ON_MISSING_CHECK_DECL;
            }
        #else
            // Cannot use a macro instead of this ERROR_... directly here
            // because assert will not expand it in the error message.
            BOOST_ASSERT(BOOST_CONTRACT_ERROR_missing_check_object_declaration);
        #endif
    }
    
    virtual void init() {} // Override for checking on entry.
    
    // Return true if actually checked calling user ftor.
    #ifndef BOOST_CONTRACT_NO_PRECONDITIONS
        bool check_pre(bool throw_on_failure = false) {
            if(failed()) return true;
            try { if(pre_) pre_(); else return false; }
            catch(...) {
                // Subcontracted pre must throw on failure (instead of
                // calling failure handler) so to be checked in logic-or.
                if(throw_on_failure) throw;
                fail(&boost::contract::precondition_failure);
            }
            return true;
        }
    #endif

    #ifndef BOOST_CONTRACT_NO_OLDS
        void copy_old() {
            if(failed()) return;
            try { if(old_) old_(); }
            catch(...) { fail(&boost::contract::old_failure); }
        }
    #endif

    #ifndef BOOST_CONTRACT_NO_EXCEPTS
        void check_except() {
            if(failed()) return;
            try { if(except_) except_(); }
            catch(...) { fail(&boost::contract::except_failure); }
        }
    #endif
    
    #ifndef BOOST_CONTRACT_NO_CONDITIONS
        void fail(void (*h)(boost::contract::from)) {
            failed(true);
            if(h) h(from_);
        }
    
        // Virtual so overriding pub func can use virtual_::failed_ instead.
        virtual bool failed() const { return failed_; }
        virtual void failed(bool value) { failed_ = value; }
    #endif

private:
    bool BOOST_CONTRACT_ERROR_missing_check_object_declaration;
    bool init_asserted_; // Avoid throwing twice from dtors (undef behavior).
    #ifndef BOOST_CONTRACT_NO_CONDITIONS
        boost::contract::from from_;
        bool failed_;
    #endif
    // Following use Boost.Function to handle also lambdas, binds, etc.
    #ifndef BOOST_CONTRACT_NO_PRECONDITIONS
        boost::function<void ()> pre_;
    #endif
    #ifndef BOOST_CONTRACT_NO_OLDS
        boost::function<void ()> old_;
    #endif
    #ifndef BOOST_CONTRACT_NO_EXCEPTS
        boost::function<void ()> except_;
    #endif
};

} } } // namespace

#endif // #include guard

