
#ifndef BOOST_CONTRACT_SPECIFY_HPP_
#define BOOST_CONTRACT_SPECIFY_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Specify preconditions, old values copied at body, postconditions, and exception
guarantees

Preconditions, old values copied at body, postconditions, and exception
guarantees are all optionals but, when they are specified, they need to be
specified in that order.
*/

#include <boost/contract/core/config.hpp>
#include <boost/contract/detail/decl.hpp>
#if     !defined(BOOST_CONTRACT_NO_CONDITIONS) || \
        defined(BOOST_CONTRACT_STATIC_LINK)
    #include <boost/contract/detail/condition/cond_base.hpp>
    #include <boost/contract/detail/condition/cond_post.hpp>
    #include <boost/contract/detail/auto_ptr.hpp>
    #include <boost/contract/detail/none.hpp>
#endif
#if     !defined(BOOST_CONTRACT_NO_PRECONDITIONS) || \
        !defined(BOOST_CONTRACT_NO_POSTCONDITIONS) || \
        !defined(BOOST_CONTRACT_NO_EXCEPTS)
    #include <boost/contract/detail/debug.hpp>
#endif
#include <boost/config.hpp>

// NOTE: No inheritance for faster run-times (macros to avoid duplicated code).

/* PRIVATE */

/* @cond */

// NOTE: Private copy ops below will force compile-time error is `auto c = ...`
// is used instead of `check c = ...` but only up to C++17. C++17 strong copy
// elision on function return values prevents this lib from generating a
// compile-time error in those cases, but the lib will still generate a run-time
// error according with ON_MISSING_CHECK_DECL. Furthermore, on some C++98
// compilers, this private copy ctor gives a warning (because of lack of copy
// optimization on those compilers), this warning can be ignored.
#if     !defined(BOOST_CONTRACT_NO_CONDITIONS) || \
        defined(BOOST_CONTRACT_STATIC_LINK)
    #define BOOST_CONTRACT_SPECIFY_CLASS_IMPL_(class_type, cond_type) \
        private: \
            boost::contract::detail::auto_ptr<cond_type > cond_; \
            explicit class_type(cond_type* cond) : cond_(cond) {} \
            class_type(class_type const& other) : cond_(other.cond_) {} \
            class_type& operator=(class_type const& other) { \
                cond_ = other.cond_; \
                return *this; \
            }
    
    #define BOOST_CONTRACT_SPECIFY_COND_RELEASE_ cond_.release()
#else
    #define BOOST_CONTRACT_SPECIFY_CLASS_IMPL_(class_type, cond_type) \
        private: \
            class_type() {} \
            class_type(class_type const&) {} \
            class_type& operator=(class_type const&) { return *this; }

    #define BOOST_CONTRACT_SPECIFY_COND_RELEASE_ /* nothing */
#endif

#ifndef BOOST_CONTRACT_NO_PRECONDITIONS
    #define BOOST_CONTRACT_SPECIFY_PRECONDITION_IMPL_ \
        BOOST_CONTRACT_DETAIL_DEBUG(cond_); \
        cond_->set_pre(f); \
        return specify_old_postcondition_except<VirtualResult>( \
                BOOST_CONTRACT_SPECIFY_COND_RELEASE_);
#else
    #define BOOST_CONTRACT_SPECIFY_PRECONDITION_IMPL_ \
        return specify_old_postcondition_except<VirtualResult>( \
                BOOST_CONTRACT_SPECIFY_COND_RELEASE_);
#endif
        
#ifndef BOOST_CONTRACT_NO_OLDS
    #define BOOST_CONTRACT_SPECIFY_OLD_IMPL_ \
        BOOST_CONTRACT_DETAIL_DEBUG(cond_); \
        cond_->set_old(f); \
        return specify_postcondition_except<VirtualResult>( \
                BOOST_CONTRACT_SPECIFY_COND_RELEASE_);
#else
    #define BOOST_CONTRACT_SPECIFY_OLD_IMPL_ \
        return specify_postcondition_except<VirtualResult>( \
                BOOST_CONTRACT_SPECIFY_COND_RELEASE_);
#endif
            
#ifndef BOOST_CONTRACT_NO_POSTCONDITIONS
    #define BOOST_CONTRACT_SPECIFY_POSTCONDITION_IMPL_ \
        BOOST_CONTRACT_DETAIL_DEBUG(cond_); \
        cond_->set_post(f); \
        return specify_except(BOOST_CONTRACT_SPECIFY_COND_RELEASE_);
#else
    #define BOOST_CONTRACT_SPECIFY_POSTCONDITION_IMPL_ \
        return specify_except(BOOST_CONTRACT_SPECIFY_COND_RELEASE_);
#endif
        
#ifndef BOOST_CONTRACT_NO_EXCEPTS
    #define BOOST_CONTRACT_SPECIFY_EXCEPT_IMPL_ \
        BOOST_CONTRACT_DETAIL_DEBUG(cond_); \
        cond_->set_except(f); \
        return specify_nothing(BOOST_CONTRACT_SPECIFY_COND_RELEASE_);
#else
    #define BOOST_CONTRACT_SPECIFY_EXCEPT_IMPL_ \
        return specify_nothing(BOOST_CONTRACT_SPECIFY_COND_RELEASE_);
#endif

/* @endcond */

/* CODE */

namespace boost {
    namespace contract {
        class virtual_;
        
        template<typename VR>
        class specify_precondition_old_postcondition_except;
        
        template<typename VR>
        class specify_old_postcondition_except;
        
        template<typename VR>
        class specify_postcondition_except;
        
        class specify_except;
    }
}

namespace boost { namespace contract {

/**
Used to prevent setting other contract conditions after exception guarantees.

This class has no member function so it is used to prevent specifying additional
functors to check any other contract.
This object is internally constructed by the library when users specify
contracts calling @RefFunc{boost::contract::function} and similar functions
(that is why this class does not have a public constructor).

@see @RefSect{tutorial, Tutorial}
*/
class specify_nothing { // Privately copyable (as *).
public:
    /**
    Destruct this object.

    @b Throws:  This is declared @c noexcept(false) since C++11 to allow users
                to program failure handlers that throw exceptions on contract
                assertion failures (not the default, see
                @RefSect{advanced.throw_on_failures__and__noexcept__,
                Throw on Failure}).
    */
    ~specify_nothing() BOOST_NOEXCEPT_IF(false) {}
    
    // No set member function here.

/** @cond */
private:
    BOOST_CONTRACT_SPECIFY_CLASS_IMPL_(specify_nothing,
            boost::contract::detail::cond_base)

    // Friends (used to limit library's public API).

    friend class check;

    template<typename VR>
    friend class specify_precondition_old_postcondition_except;
    
    template<typename VR>
    friend class specify_old_postcondition_except;

    template<typename VR>
    friend class specify_postcondition_except;

    friend class specify_except;
/** @endcond */
};

/**
Allow to specify exception guarantees.

Allow to specify the functor this library will call to check exception
guarantees.
This object is internally constructed by the library when users specify
contracts calling @RefFunc{boost::contract::function} and similar functions
(that is why this class does not have a public constructor).

@see @RefSect{tutorial.exception_guarantees, Exception Guarantees}
*/
class specify_except { // Privately copyable (as *).
public:
    /**
    Destruct this object.

    @b Throws:  This is declared @c noexcept(false) since C++11 to allow users
                to program failure handlers that throw exceptions on contract
                assertion failures (not the default, see
                @RefSect{advanced.throw_on_failures__and__noexcept__,
                Throw on Failure}).
    */
    ~specify_except() BOOST_NOEXCEPT_IF(false) {}

    /**
    Allow to specify exception guarantees.

    @param f    Nullary functor called by this library to check exception
                guarantees @c f().
                Assertions within this functor are usually programmed using
                @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a
                call to this functor indicates a contract assertion failure (and
                will result in this library calling
                @RefFunc{boost::contract::except_failure}).
                This functor should capture variables by (constant) references
                (to access the values they will have at function exit).

    @return After exception guarantees have been specified, the object returned
            by this function does not allow to specify any additional contract.
    */
    template<typename F>
    specify_nothing except(
        F const&
        #if !defined(BOOST_CONTRACT_NO_EXCEPTS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) { BOOST_CONTRACT_SPECIFY_EXCEPT_IMPL_ }

/** @cond */
private:
    BOOST_CONTRACT_SPECIFY_CLASS_IMPL_(specify_except,
            boost::contract::detail::cond_base)

    // Friends (used to limit library's public API).

    friend class check;

    template<typename VR>
    friend class specify_precondition_old_postcondition_except;
    
    template<typename VR>
    friend class specify_old_postcondition_except;
    
    template<typename VR>
    friend class specify_postcondition_except;
/** @endcond */
};

/**
Allow to specify postconditions or exception guarantees.

Allow to specify functors this library will call to check postconditions or
exception guarantees.
This object is internally constructed by the library when users specify
contracts calling @RefFunc{boost::contract::function} and similar functions
(that is why this class does not have a public constructor).

@see    @RefSect{tutorial.postconditions, Postconditions},
        @RefSect{tutorial.exception_guarantees, Exception Guarantees}

@tparam VirtualResult   Return type of the enclosing function declaring the
                        contract if that is either a virtual or an
                        overriding public function, otherwise this is always
                        @c void.
                        (Usually this template parameter is automatically
                        deduced by C++ and it does not need to be explicitly
                        specified by programmers.)
*/
template<typename VirtualResult = void>
class specify_postcondition_except { // Privately copyable (as *).
public:
    /**
    Destruct this object.

    @b Throws:  This is declared @c noexcept(false) since C++11 to allow users
                to program failure handlers that throw exceptions on contract
                assertion failures (not the default, see
                @RefSect{advanced.throw_on_failures__and__noexcept__,
                Throw on Failure}).
    */
    ~specify_postcondition_except() BOOST_NOEXCEPT_IF(false) {}

    /**
    Allow to specify postconditions.

    @param f    Functor called by this library to check postconditions
                @c f() or @c f(result).
                Assertions within this functor are usually programmed using
                @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a
                call to this functor indicates a contract assertion failure (and
                will result in this library calling
                @RefFunc{boost::contract::postcondition_failure}).
                This functor should capture variables by (constant) references
                (to access the values they will have at function exit).
                This functor must be a nullary functor @c f() if
                @c VirtualResult is @c void, otherwise it must be a unary
                functor @c f(result) accepting the return value @c result as a
                parameter of type <c>VirtualResult const&</c> (to avoid extra
                copies of the return value, or of type @c VirtualResult or
                <c>VirtualResult const</c> if extra copies of the return value
                are irrelevant).

    @return After postconditions have been specified, the object returned by
            this function allows to optionally specify exception guarantees.
    */
    template<typename F>
    specify_except postcondition(
        F const&
        #if !defined(BOOST_CONTRACT_NO_POSTCONDITIONS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) {
        BOOST_CONTRACT_SPECIFY_POSTCONDITION_IMPL_
    }
    
    /**
    Allow to specify exception guarantees.

    @param f    Nullary functor called by this library to check exception
                guarantees @c f().
                Assertions within this functor are usually programmed using
                @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a
                call to this functor indicates a contract assertion failure (and
                will result in this library calling
                @RefFunc{boost::contract::except_failure}).
                This functor should capture variables by (constant) references
                (to access the values they will have at function exit).

    @return After exception guarantees have been specified, the object returned
            by this function does not allow to specify any additional contract.
    */
    template<typename F>
    specify_nothing except(
        F const&
        #if !defined(BOOST_CONTRACT_NO_EXCEPTS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) { BOOST_CONTRACT_SPECIFY_EXCEPT_IMPL_ }

/** @cond */
private:
    BOOST_CONTRACT_SPECIFY_CLASS_IMPL_(
        specify_postcondition_except,
        boost::contract::detail::cond_post<typename
                boost::contract::detail::none_if_void<VirtualResult>::type>
    )

    // Friends (used to limit library's public API).

    friend class check;
    friend class specify_precondition_old_postcondition_except<VirtualResult>;
    friend class specify_old_postcondition_except<VirtualResult>;
/** @endcond */
};

/**
Allow to specify old values copied at body, postconditions, and exception
guarantees.

Allow to specify functors this library will call to copy old values at body, 
check postconditions, and check exception guarantees.
This object is internally constructed by the library when users specify
contracts calling @RefFunc{boost::contract::function} and similar functions
(that is why this class does not have a public constructor).

@see    @RefSect{advanced.old_values_copied_at_body, Old Values Copied at Body},
        @RefSect{tutorial.postconditions, Postconditions},
        @RefSect{tutorial.exception_guarantees, Exception Guarantees}

@tparam VirtualResult   Return type of the enclosing function declaring the
                        contract if that is either a virtual or an
                        overriding public function, otherwise this is always
                        @c void.
                        (Usually this template parameter is automatically
                        deduced by C++ and it does not need to be explicitly
                        specified by programmers.)
*/
template<typename VirtualResult = void>
class specify_old_postcondition_except { // Privately copyable (as *).
public:
    /**
    Destruct this object.

    @b Throws:  This is declared @c noexcept(false) since C++11 to allow users
                to program failure handlers that throw exceptions on contract
                assertion failures (not the default, see
                @RefSect{advanced.throw_on_failures__and__noexcept__,
                Throw on Failure}).
    */
    ~specify_old_postcondition_except() BOOST_NOEXCEPT_IF(false) {}
    
    /**
    Allow to specify old values copied at body.

    It should often be sufficient to initialize old value pointers as soon as
    they are declared, without using this function (see
    @RefSect{advanced.old_values_copied_at_body, Old Values Copied at Body}).

    @param f    Nullary functor called by this library @c f() to assign old
                value copies just before the body is executed but after entry
                invariants (when they apply) and preconditions are checked.
                Old value pointers within this functor call are usually assigned
                using @RefMacro{BOOST_CONTRACT_OLDOF}.
                Any exception thrown by a call to this functor will result in
                this library calling @RefFunc{boost::contract::old_failure}
                (because old values could not be copied to check postconditions
                and exception guarantees).
                This functor should capture old value pointers by references so
                they can be assigned (all other variables needed to evaluate old
                value expressions can be captured by (constant) value, or better
                by (constant) reference to avoid extra copies).

    @return After old values copied at body have been specified, the object
            returned by this function allows to optionally specify
            postconditions and exception guarantees.
    */
    template<typename F>
    specify_postcondition_except<VirtualResult> old(
        F const&
        #if !defined(BOOST_CONTRACT_NO_OLDS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) {
        BOOST_CONTRACT_SPECIFY_OLD_IMPL_
    }

    /**
    Allow to specify postconditions.

    @param f    Functor called by this library to check postconditions
                @c f() or @c f(result).
                Assertions within this functor are usually programmed using
                @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a
                call to this functor indicates a contract assertion failure (and
                will result in this library calling
                @RefFunc{boost::contract::postcondition_failure}).
                This functor should capture variables by (constant) references
                (to access the values they will have at function exit).
                This functor must be a nullary functor @c f() if
                @c VirtualResult is @c void, otherwise it must be a unary
                functor @c f(result) accepting the return value @c result as a
                parameter of type <c>VirtualResult const&</c> (to avoid extra
                copies of the return value, or of type @c VirtualResult or
                <c>VirtualResult const</c> if extra copies of the return value
                are irrelevant).

    @return After postconditions have been specified, the object returned by
            this function allows to optionally specify exception guarantees.
    */
    template<typename F>
    specify_except postcondition(
        F const&
        #if !defined(BOOST_CONTRACT_NO_POSTCONDITIONS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) {
        BOOST_CONTRACT_SPECIFY_POSTCONDITION_IMPL_
    }
    
    /**
    Allow to specify exception guarantees.

    @param f    Nullary functor called by this library to check exception
                guarantees @c f().
                Assertions within this functor are usually programmed using
                @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a
                call to this functor indicates a contract assertion failure (and
                will result in this library calling
                @RefFunc{boost::contract::except_failure}).
                This functor should capture variables by (constant) references
                (to access the values they will have at function exit).

    @return After exception guarantees have been specified, the object returned
            by this function does not allow to specify any additional contract.
    */
    template<typename F>
    specify_nothing except(
        F const&
        #if !defined(BOOST_CONTRACT_NO_EXCEPTS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) { BOOST_CONTRACT_SPECIFY_EXCEPT_IMPL_ }

/** @cond */
private:
    BOOST_CONTRACT_SPECIFY_CLASS_IMPL_(
        specify_old_postcondition_except,
        boost::contract::detail::cond_post<typename
                boost::contract::detail::none_if_void<VirtualResult>::type>
    )

    // Friends (used to limit library's public API).

    friend class check;
    friend class specify_precondition_old_postcondition_except<VirtualResult>;

    template<class C>
    friend specify_old_postcondition_except<> constructor(C*);

    template<class C>
    friend specify_old_postcondition_except<> destructor(C*);
/** @endcond */
};

/**
Allow to specify preconditions, old values copied at body, postconditions, and
exception guarantees.

Allow to specify functors this library will call to check preconditions, copy
old values at body, check postconditions, and check exception guarantees.
This object is internally constructed by the library when users specify
contracts calling @RefFunc{boost::contract::function} and similar functions
(that is why this class does not have a public constructor).

@see    @RefSect{tutorial.preconditions, Preconditions},
        @RefSect{advanced.old_values_copied_at_body, Old Values Copied at Body},
        @RefSect{tutorial.postconditions, Postconditions},
        @RefSect{tutorial.exception_guarantees, Exception Guarantees}

@tparam VirtualResult   Return type of the enclosing function declaring the
                        contract if that is either a virtual or an
                        overriding public function, otherwise this is always
                        @c void.
                        (Usually this template parameter is automatically
                        deduced by C++ and it does not need to be explicitly
                        specified by programmers.)
*/
template<
    typename VirtualResult /* = void (already in fwd decl from decl.hpp) */
    #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
        = void
    #endif
>
class specify_precondition_old_postcondition_except { // Priv. copyable (as *).
public:
    /**
    Destruct this object.

    @b Throws:  This is declared @c noexcept(false) since C++11 to allow users
                to program failure handlers that throw exceptions on contract
                assertion failures (not the default, see
                @RefSect{advanced.throw_on_failures__and__noexcept__,
                Throw on Failure}).
    */
    ~specify_precondition_old_postcondition_except() BOOST_NOEXCEPT_IF(false) {}
    
    /**
    Allow to specify preconditions.

    @param f    Nullary functor called by this library to check preconditions
                @c f().
                Assertions within this functor are usually programmed using
                @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a
                call to this functor indicates a contract assertion failure (and
                will result in this library calling
                @RefFunc{boost::contract::precondition_failure}).
                This functor should capture variables by (constant) value, or
                better by (constant) reference (to avoid extra copies).

    @return After preconditions have been specified, the object returned by this
            function allows to optionally specify old values copied at body,
            postconditions, and exception guarantees.
    */
    template<typename F>
    specify_old_postcondition_except<VirtualResult> precondition(
        F const&
        #if !defined(BOOST_CONTRACT_NO_PRECONDITIONS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) {
        BOOST_CONTRACT_SPECIFY_PRECONDITION_IMPL_
    }

    /**
    Allow to specify old values copied at body.

    It should often be sufficient to initialize old value pointers as soon as
    they are declared, without using this function (see
    @RefSect{advanced.old_values_copied_at_body, Old Values Copied at Body}).

    @param f    Nullary functor called by this library @c f() to assign old
                value copies just before the body is executed but after entry
                invariants (when they apply) and preconditions are checked.
                Old value pointers within this functor call are usually assigned
                using @RefMacro{BOOST_CONTRACT_OLDOF}.
                Any exception thrown by a call to this functor will result in
                this library calling @RefFunc{boost::contract::old_failure}
                (because old values could not be copied to check postconditions
                and exception guarantees).
                This functor should capture old value pointers by references so
                they can be assigned (all other variables needed to evaluate old
                value expressions can be captured by (constant) value, or better
                by (constant) reference to avoid extra copies).

    @return After old values copied at body have been specified, the object
            returned by this functions allows to optionally specify
            postconditions and exception guarantees.
    */
    template<typename F>
    specify_postcondition_except<VirtualResult> old(
        F const&
        #if !defined(BOOST_CONTRACT_NO_OLDS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) {
        BOOST_CONTRACT_SPECIFY_OLD_IMPL_
    }

    /**
    Allow to specify postconditions.

    @param f    Functor called by this library to check postconditions
                @c f() or @c f(result).
                Assertions within this functor are usually programmed using
                @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a
                call to this functor indicates a contract assertion failure (and
                will result in this library calling
                @RefFunc{boost::contract::postcondition_failure}).
                This functor should capture variables by (constant) references
                (to access the values they will have at function exit).
                This functor must be a nullary functor @c f() if
                @c VirtualResult is @c void, otherwise it must be a unary
                functor @c f(result) accepting the return value @c result as a
                parameter of type <c>VirtualResult const&</c> (to avoid extra
                copies of the return value, or of type @c VirtualResult or
                <c>VirtualResult const</c> if extra copies of the return value
                are irrelevant).

    @return After postconditions have been specified, the object returned by
            this function allows to optionally specify exception guarantees.
    */
    template<typename F>
    specify_except postcondition(
        F const&
        #if !defined(BOOST_CONTRACT_NO_POSTCONDITIONS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) {
        BOOST_CONTRACT_SPECIFY_POSTCONDITION_IMPL_
    }
    
    /**
    Allow to specify exception guarantees.

    @param f    Nullary functor called by this library to check exception
                guarantees @c f().
                Assertions within this functor are usually programmed using
                @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a
                call to this functor indicates a contract assertion failure (and
                will result in this library calling
                @RefFunc{boost::contract::except_failure}).
                This functor should capture variables by (constant) references
                (to access the values they will have at function exit).

    @return After exception guarantees have been specified, the object returned
            by this function does not allow to specify any additional contract.
    */
    template<typename F>
    specify_nothing except(
        F const&
        #if !defined(BOOST_CONTRACT_NO_EXCEPTS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            f
        #endif // Else, no name (avoid unused param warning).
    ) { BOOST_CONTRACT_SPECIFY_EXCEPT_IMPL_ }

/** @cond */
private:
    BOOST_CONTRACT_SPECIFY_CLASS_IMPL_(
        specify_precondition_old_postcondition_except,
        boost::contract::detail::cond_post<typename
                boost::contract::detail::none_if_void<VirtualResult>::type>
    )

    // Friends (used to limit library's public API).

    friend class check;
    friend specify_precondition_old_postcondition_except<> function();

    template<class C>
    friend specify_precondition_old_postcondition_except<> public_function();

    template<class C>
    friend specify_precondition_old_postcondition_except<> public_function(C*);
    
    template<class C>
    friend specify_precondition_old_postcondition_except<> public_function(
            virtual_*, C*);

    template<typename VR, class C>
    friend specify_precondition_old_postcondition_except<VR> public_function(
            virtual_*, VR&, C*);

    BOOST_CONTRACT_DETAIL_DECL_FRIEND_OVERRIDING_PUBLIC_FUNCTIONS_Z(1,
            O, VR, F, C, Args, v, r, f, obj, args)
/** @endcond */
};

} } // namespace

#endif // #include guard

