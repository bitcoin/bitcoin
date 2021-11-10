
#ifndef BOOST_CONTRACT_OLD_HPP_
#define BOOST_CONTRACT_OLD_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Handle old values.
*/

#include <boost/contract/core/config.hpp>
#include <boost/contract/core/virtual.hpp>
#ifndef BOOST_CONTRACT_ALL_DISABLE_NO_ASSERTION
    #include <boost/contract/detail/checking.hpp>
#endif
#include <boost/contract/detail/operator_safe_bool.hpp>
#include <boost/contract/detail/declspec.hpp>
#include <boost/contract/detail/debug.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_copy_constructible.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/static_assert.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/config/config.hpp>
#include <queue>

#if !BOOST_PP_VARIADICS

#define BOOST_CONTRACT_OLDOF \
BOOST_CONTRACT_ERROR_macro_OLDOF_requires_variadic_macros_otherwise_manually_program_old_values

#else // variadics

#include <boost/preprocessor/facilities/overload.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/config.hpp>

/* PRIVATE */

/** @cond */

#ifdef BOOST_NO_CXX11_AUTO_DECLARATIONS
    #define BOOST_CONTRACT_OLDOF_AUTO_TYPEOF_(value) /* nothing */
#else
    #include <boost/typeof/typeof.hpp>
    // Explicitly force old_ptr<...> conversion to allow for C++11 auto decl.
    #define BOOST_CONTRACT_OLDOF_AUTO_TYPEOF_(value) \
        boost::contract::old_ptr<BOOST_TYPEOF(value)>
#endif

#define BOOST_CONTRACT_ERROR_macro_OLDOF_has_invalid_number_of_arguments_2( \
        v, value) \
    BOOST_CONTRACT_OLDOF_AUTO_TYPEOF_(value)(boost::contract::make_old(v, \
        boost::contract::copy_old(v) ? (value) : boost::contract::null_old() \
    ))

#define BOOST_CONTRACT_ERROR_macro_OLDOF_has_invalid_number_of_arguments_1( \
        value) \
    BOOST_CONTRACT_OLDOF_AUTO_TYPEOF_(value)(boost::contract::make_old( \
        boost::contract::copy_old() ? (value) : boost::contract::null_old() \
    ))

/** @endcond */

/* PUBLIC */

// NOTE: Leave this #defined the same regardless of ..._NO_OLDS.
/**
Macro typically used to copy an old value expression and assign it to an old
value pointer.

The expression expanded by this macro should be assigned to an old value
pointer of type @RefClass{boost::contract::old_ptr} or
@RefClass{boost::contract::old_ptr_if_copyable}.
This is an overloaded variadic macro and it can be used in the following
different ways.

1\. From within virtual public functions and public functions overrides:

@code
BOOST_CONTRACT_OLDOF(v, old_expr)
@endcode

2\. From all other operations:

@code
BOOST_CONTRACT_OLDOF(old_expr)
@endcode

Where:

@arg    <c><b>v</b></c> is the extra parameter of type
        @RefClass{boost::contract::virtual_}<c>*</c> and default value @c 0
        from the enclosing virtual public function or public function
        overrides declaring the contract.
@arg    <c><b>old_expr</b></c> is the expression to be evaluated and copied into
        the old value pointer.
        (This is not a variadic macro parameter so any comma it might contain
        must be protected by round parenthesis and
        <c>BOOST_CONTRACT_OLDOF(v, (old_expr))</c> will always work.)

On compilers that do not support variadic macros, programmers can manually copy
old value expressions without using this macro (see
@RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}).

@see @RefSect{tutorial.old_values, Old Values}
*/
#define BOOST_CONTRACT_OLDOF(...) \
    BOOST_PP_CAT( /* CAT(..., EMTPY()) required on MSVC */ \
        BOOST_PP_OVERLOAD( \
  BOOST_CONTRACT_ERROR_macro_OLDOF_has_invalid_number_of_arguments_, \
            __VA_ARGS__ \
        )(__VA_ARGS__), \
        BOOST_PP_EMPTY() \
    )

#endif // variadics

/* CODE */

namespace boost { namespace contract {

/**
Trait to check if an old value type can be copied or not.

By default, this unary boolean meta-function is equivalent to
@c boost::is_copy_constructible<T> but programmers can chose to specialize it
for user-defined types (in general some kind of specialization is also needed on
compilers that do not support C++11, see
<a href="http://www.boost.org/doc/libs/release/libs/type_traits/doc/html/boost_typetraits/reference/is_copy_constructible.html">
<c>boost::is_copy_constructible</c></a>):

@code
class u; // Some user-defined type for which old values shall not be copied.

namespace boost { namespace contract {
    template<> // Specialization to not copy old values of type `u`.
    struct is_old_value_copyable<u> : boost::false_type {};
} } // namespace
@endcode

A given old value type @c T is copied only if
@c boost::contract::is_old_value_copyable<T>::value is @c true.
A copyable old value type @c V is always copied using
@c boost::contract::old_value_copy<V>.
A non-copyable old value type @c W generates a compile-time error when
@c boost::contract::old_ptr<W> is dereferenced, but instead leaves
@c boost::contract::old_ptr_if_copyable<W> always null (without generating
compile-time errors).

@see    @RefSect{extras.old_value_requirements__templates_,
        Old Value Requirements}
*/
template<typename T>
struct is_old_value_copyable : boost::is_copy_constructible<T> {};

/** @cond */
class old_value;

template<> // Needed because `old_value` incomplete type when trait first used.
struct is_old_value_copyable<old_value> : boost::true_type {};
/** @endcond */

/**
Trait to copy an old value.

By default, the implementation of this trait uses @c T's copy constructor to
make one single copy of the specified value.
However, programmers can specialize this trait to copy old values using
user-specific operations different from @c T's copy constructor.
The default implementation of this trait is equivalent to:

@code
template<typename T>
class old_value_copy {
public:
    explicit old_value_copy(T const& old) :
        old_(old) // One single copy of value using T's copy constructor.
    {}

    T const& old() const { return old_; }

private:
    T const old_; // The old value copy.
};
@endcode

This library will instantiate and use this trait only on old value types @c T
that are copyable (i.e., for which
<c>boost::contract::is_old_value_copyable<T>::value</c> is @c true).

@see    @RefSect{extras.old_value_requirements__templates_,
        Old Value Requirements}
*/
template<typename T> // Used only if is_old_value_copyable<T>.
struct old_value_copy {
    /**
    Construct this object by making one single copy of the specified old value.

    This is the only operation within this library that actually copies old
    values.
    This ensures this library makes one and only one copy of an old value (if
    they actually need to be copied, see @RefMacro{BOOST_CONTRACT_NO_OLDS}).

    @param old The old value to copy.
    */
    explicit old_value_copy(T const& old) :
            old_(old) {} // This makes the one single copy of T.

    /**
    Return a (constant) reference to the old value that was copied.
    
    Contract assertions should not change the state of the program so the old
    value copy is returned as @c const (see
    @RefSect{contract_programming_overview.constant_correctness,
    Constant Correctness}).
    */
    T const& old() const { return old_; }

private:
    T const old_;
};

template<typename T>
class old_ptr_if_copyable;

/**
Old value pointer that requires the pointed old value type to be copyable.

This pointer can be set to point an actual old value copy using either
@RefMacro{BOOST_CONTRACT_OLDOF} or @RefFunc{boost::contract::make_old} (that is
why this class does not have public non-default constructors):

@code
class u {
public:
    virtual void f(..., boost::contract::virtual_* v = 0) {
        boost::contract::old_ptr<old_type> old_var = // For copyable `old_type`.
                BOOST_CONTRACT_OLDOF(v, old_expr);
        ...
    }

    ...
};
@endcode

@see @RefSect{tutorial.old_values, Old Values}

@tparam T Type of the pointed old value.
        This type must be copyable (i.e.,
        <c>boost::contract::is_old_value_copyable<T>::value</c> must be
        @c true), otherwise this pointer will always be null and this library
        will generate a compile-time error when the pointer is dereferenced.
*/
template<typename T>
class old_ptr { /* copyable (as *) */
public:
    /** Pointed old value type. */
    typedef T element_type;

    /** Construct this old value pointer as null. */
    old_ptr() {}

    /**
    Dereference this old value pointer.

    This will generate a run-time error if this pointer is null and a
    compile-time error if the pointed type @c T is not copyable (i.e., if
    @c boost::contract::is_old_value_copyable<T>::value is @c false).
    
    @return The pointed old value.
            Contract assertions should not change the state of the program so
            this member function is @c const and it returns the old value as a
            reference to a constant object (see
            @RefSect{contract_programming_overview.constant_correctness,
            Constant Correctness}).
    */
    T const& operator*() const {
        BOOST_STATIC_ASSERT_MSG(
            boost::contract::is_old_value_copyable<T>::value,
            "old_ptr<T> requires T copyable (see is_old_value_copyable<T>), "
            "otherwise use old_ptr_if_copyable<T>"
        );
        BOOST_CONTRACT_DETAIL_DEBUG(typed_copy_);
        return typed_copy_->old();
    }

    /**
    Structure-dereference this old value pointer.

    This will generate a compile-time error if the pointed type @c T is not
    copyable (i.e., if @c boost::contract::is_old_value_copyable<T>::value is
    @c false).

    @return A pointer to the old value (null if this old value pointer is null).
            Contract assertions should not change the state of the program so
            this member function is @c const and it returns the old value as a
            pointer to a constant object (see
            @RefSect{contract_programming_overview.constant_correctness,
            Constant Correctness}).
    */
    T const* operator->() const {
        BOOST_STATIC_ASSERT_MSG(
            boost::contract::is_old_value_copyable<T>::value,
            "old_ptr<T> requires T copyble (see is_old_value_copyable<T>), "
            "otherwise use old_ptr_if_copyable<T>"
        );
        if(typed_copy_) return &typed_copy_->old();
        return 0;
    }

    #ifndef BOOST_CONTRACT_DETAIL_DOXYGEN
        BOOST_CONTRACT_DETAIL_OPERATOR_SAFE_BOOL(old_ptr<T>,
                !!typed_copy_)
    #else
        /**
        Query if this old value pointer is null or not (safe-bool operator).

        (This is implemented using safe-bool emulation on compilers that do not
        support C++11 explicit type conversion operators.)

        @return True if this pointer is not null, false otherwise.
        */
        explicit operator bool() const;
    #endif

/** @cond */
private:
    #ifndef BOOST_CONTRACT_NO_OLDS
        explicit old_ptr(boost::shared_ptr<old_value_copy<T> > old)
                : typed_copy_(old) {}
    #endif

    boost::shared_ptr<old_value_copy<T> > typed_copy_;

    friend class old_pointer;
    friend class old_ptr_if_copyable<T>;
/** @endcond */
};

/**
Old value pointer that does not require the pointed old value type to be
copyable.

This pointer can be set to point to an actual old value copy using either
@RefMacro{BOOST_CONTRACT_OLDOF} or @RefFunc{boost::contract::make_old}:

@code
template<typename T> // Type `T` might or not be copyable.
class u {
public:
    virtual void f(..., boost::contract::virtual_* v = 0) {
        boost::contract::old_ptr_if_copyable<T> old_var =
                BOOST_CONTRACT_OLDOF(v, old_expr);
        ...
            if(old_var) ... // Always null for non-copyable types.
        ...
    }

    ...
};
@endcode

@see    @RefSect{extras.old_value_requirements__templates_,
        Old Value Requirements}

@tparam T Type of the pointed old value.
        If this type is not copyable (i.e.,
        <c>boost::contract::is_old_value_copyable<T>::value</c> is @c false),
        this pointer will always be null (but this library will not generate a
        compile-time error when this pointer is dereferenced).
*/
template<typename T>
class old_ptr_if_copyable { /* copyable (as *) */
public:
    /** Pointed old value type. */
    typedef T element_type;

    /** Construct this old value pointer as null. */
    old_ptr_if_copyable() {}

    /**
    Construct this old value pointer from an old value pointer that requires
    the old value type to be copyable.
    
    Ownership of the pointed value object is transferred to this pointer.
    This constructor is implicitly called by this library when assigning an
    object of this type using @RefMacro{BOOST_CONTRACT_OLDOF} (this constructor
    is usually not explicitly called by user code).

    @param other    Old value pointer that requires the old value type to be
                    copyable.
    */
    /* implicit */ old_ptr_if_copyable(old_ptr<T> const& other) :
            typed_copy_(other.typed_copy_) {}

    /**
    Dereference this old value pointer.

    This will generate a run-time error if this pointer is null, but no
    compile-time error is generated if the pointed type @c T is not copyable
    (i.e., if @c boost::contract::is_old_value_copyable<T>::value is @c false).
    
    @return The pointed old value.
            Contract assertions should not change the state of the program so
            this member function is @c const and it returns the old value as a
            reference to a constant object (see
            @RefSect{contract_programming_overview.constant_correctness,
            Constant Correctness}).
    */
    T const& operator*() const {
        BOOST_CONTRACT_DETAIL_DEBUG(typed_copy_);
        return typed_copy_->old();
    }

    /**
    Structure-dereference this old value pointer.

    This will return null but will not generate a compile-time error if the
    pointed type @c T is not copyable (i.e., if
    @c boost::contract::is_old_value_copyable<T>::value is @c false).

    @return A pointer to the old value (null if this old value pointer is null).
            Contract assertions should not change the state of the program so
            this member function is @c const and it returns the old value as a
            pointer to a constant object (see
            @RefSect{contract_programming_overview.constant_correctness,
            Constant Correctness}).
    */
    T const* operator->() const {
        if(typed_copy_) return &typed_copy_->old();
        return 0;
    }

    #ifndef BOOST_CONTRACT_DETAIL_DOXYGEN
        BOOST_CONTRACT_DETAIL_OPERATOR_SAFE_BOOL(old_ptr_if_copyable<T>,
                !!typed_copy_)
    #else
        /**
        Query if this old value pointer is null or not (safe-bool operator).

        (This is implemented using safe-bool emulation on compilers that do not
        support C++11 explicit type conversion operators.)

        @return True if this pointer is not null, false otherwise.
        */
        explicit operator bool() const;
    #endif

/** @cond */
private:
    #ifndef BOOST_CONTRACT_NO_OLDS
        explicit old_ptr_if_copyable(boost::shared_ptr<old_value_copy<T> > old)
                : typed_copy_(old) {}
    #endif

    boost::shared_ptr<old_value_copy<T> > typed_copy_;

    friend class old_pointer;
/** @endcond */
};

/**
Convert user-specified expressions to old values.

This class is usually only implicitly used by this library and it does not
explicitly appear in user code.

On older compilers that cannot correctly deduce the
@c boost::contract::is_old_value_copyable trait used in the declaration of this
class, programmers can manually specialize that trait to make sure that only old
value types that are copyable are actually copied.

@see    @RefSect{extras.old_value_requirements__templates_,
        Old Value Requirements}
*/
class old_value { // Copyable (as *). 
public:
    // Following implicitly called by ternary operator `... ? ... : null_old()`.

    /**
    Construct this object from the specified old value when the old value type
    is copy constructible.

    The specified old value @c old is copied (one time only) using
    @c boost::contract::old_value_copy, in which case the related old value
    pointer will not be null (but no copy is made if postconditions and
    exception  guarantees are not being checked, see
    @RefMacro{BOOST_CONTRACT_NO_OLDS}).

    @param old Old value to be copied.

    @tparam T Old value type.
    */
    template<typename T>
    /* implicit */ old_value(
        T const&
        #if !defined(BOOST_CONTRACT_NO_OLDS) || \
                defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
            old
        #endif // Else, no name (avoid unused param warning).
    ,
        typename boost::enable_if<boost::contract::is_old_value_copyable<T>
                >::type* = 0
    )
        #ifndef BOOST_CONTRACT_NO_OLDS
            : untyped_copy_(new old_value_copy<T>(old))
        #endif // Else, leave ptr_ null (thus no copy of T).
    {}
    
    /**
    Construct this object from the specified old value when the old value type
    is not copyable.

    The specified old value @c old cannot be copied in this case so it is not
    copied and the related old value pointer will always be null (thus calls to
    this constructor have no effect and they will likely be optimized away by
    most compilers).
    
    @param old Old value (that will not be copied in this case).
    
    @tparam T Old value type.
    */
    template<typename T>
    /* implicit */ old_value(
        T const&
        #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
            old
        #endif // Else, no name (avoid unused param warning).
    ,
        typename boost::disable_if<boost::contract::is_old_value_copyable<T>
                >::type* = 0
    ) {} // Leave ptr_ null (thus no copy of T).

/** @cond */
private:
    explicit old_value() {}
    
    #ifndef BOOST_CONTRACT_NO_OLDS
        boost::shared_ptr<void> untyped_copy_; // Type erasure.
    #endif

    friend class old_pointer;
    friend BOOST_CONTRACT_DETAIL_DECLSPEC old_value null_old();
/** @endcond */
};

/**
Convert old value copies into old value pointers.

This class is usually only implicitly used by this library and it does not
explicitly appear in user code (that is why this class does not have public
constructors, etc.).
*/
class old_pointer { // Copyable (as *).
public:
    /**
    Convert this object to an actual old value pointer for which the old value
    type @c T might or not be copyable.

    For example, this is implicitly called when assigning or initializing old
    value pointers of type @c boost::contract::old_ptr_if_copyable.
    
    @tparam T   Type of the pointed old value.
                The old value pointer will always be null if this type is not
                copyable (see
                @c boost::contract::is_old_value_copyable), but this library
                will not generate a compile-time error.
    */
    template<typename T>
    /* implicit */ operator old_ptr_if_copyable<T>() {
        return get<old_ptr_if_copyable<T> >();
    }
    
    /**
    Convert this object to an actual old value pointer for which the old value
    type @c T must be copyable.

    For example, this is implicitly called when assigning or initializing old
    value pointers of type @c boost::contract::old_ptr.
    
    @tparam T   Type of the pointed old value. This type must be copyable
                (see @c boost::contract::is_old_value_copyable),
                otherwise this library will generate a compile-time error when
                the old value pointer is dereferenced.
    */
    template<typename T>
    /* implicit */ operator old_ptr<T>() {
        return get<old_ptr<T> >();
    }

/** @cond */
private:
    #ifndef BOOST_CONTRACT_NO_OLDS
        explicit old_pointer(virtual_* v, old_value const& old)
            : v_(v), untyped_copy_(old.untyped_copy_) {}
    #else
        explicit old_pointer(virtual_* /* v */, old_value const& /* old */) {}
    #endif
    
    template<typename Ptr>
    Ptr get() {
        #ifndef BOOST_CONTRACT_NO_OLDS
            if(!boost::contract::is_old_value_copyable<typename
                    Ptr::element_type>::value) {
                BOOST_CONTRACT_DETAIL_DEBUG(!untyped_copy_);
                return Ptr(); // Non-copyable so no old value and return null.
        #ifndef BOOST_CONTRACT_ALL_DISABLE_NO_ASSERTION
            } else if(!v_ && boost::contract::detail::checking::already()) {
                return Ptr(); // Not checking (so return null).
        #endif
            } else if(!v_) {
                BOOST_CONTRACT_DETAIL_DEBUG(untyped_copy_);
                typedef old_value_copy<typename Ptr::element_type> copied_type;
                boost::shared_ptr<copied_type> typed_copy = // Un-erase type.
                        boost::static_pointer_cast<copied_type>(untyped_copy_);
                BOOST_CONTRACT_DETAIL_DEBUG(typed_copy);
                return Ptr(typed_copy);
            } else if(
                v_->action_ == boost::contract::virtual_::push_old_init_copy ||
                v_->action_ == boost::contract::virtual_::push_old_ftor_copy
            ) {
                BOOST_CONTRACT_DETAIL_DEBUG(untyped_copy_);
                std::queue<boost::shared_ptr<void> >& copies = v_->action_ ==
                        boost::contract::virtual_::push_old_ftor_copy ?
                    v_->old_ftor_copies_
                :
                    v_->old_init_copies_
                ;
                copies.push(untyped_copy_);
                return Ptr(); // Pushed (so return null).
            } else if(
                boost::contract::virtual_::pop_old_init_copy(v_->action_) ||
                v_->action_ == boost::contract::virtual_::pop_old_ftor_copy
            ) {
                // Copy not null, but still pop it from the queue.
                BOOST_CONTRACT_DETAIL_DEBUG(!untyped_copy_);

                std::queue<boost::shared_ptr<void> >& copies = v_->action_ ==
                        boost::contract::virtual_::pop_old_ftor_copy ?
                    v_->old_ftor_copies_
                :
                    v_->old_init_copies_
                ;
                boost::shared_ptr<void> untyped_copy = copies.front();
                BOOST_CONTRACT_DETAIL_DEBUG(untyped_copy);
                copies.pop();

                typedef old_value_copy<typename Ptr::element_type> copied_type;
                boost::shared_ptr<copied_type> typed_copy = // Un-erase type.
                        boost::static_pointer_cast<copied_type>(untyped_copy);
                BOOST_CONTRACT_DETAIL_DEBUG(typed_copy);
                return Ptr(typed_copy);
            }
            BOOST_CONTRACT_DETAIL_DEBUG(!untyped_copy_);
        #endif
        return Ptr();
    }

    #ifndef BOOST_CONTRACT_NO_OLDS
        virtual_* v_;
        boost::shared_ptr<void> untyped_copy_; // Type erasure.
    #endif
    
    friend BOOST_CONTRACT_DETAIL_DECLSPEC
    old_pointer make_old(old_value const&);

    friend BOOST_CONTRACT_DETAIL_DECLSPEC
    old_pointer make_old(virtual_*, old_value const&);
/** @endcond */
};

/**
Return a "null" old value.

The related old value pointer will also be null.
This function is usually only called by the code expanded by
@RefMacro{BOOST_CONTRACT_OLDOF}.

@see @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}

@return Null old value.
*/
/** @cond */ BOOST_CONTRACT_DETAIL_DECLSPEC /** @endcond */
old_value null_old();

/**
Make an old value pointer (but not for virtual public functions and public
functions overrides).

The related old value pointer will not be null if the specified old value was
actually copied.
This function is usually only called by code expanded by
@c BOOST_CONTRACT_OLDOF(old_expr) as in:

@code
boost::contract::make_old(boost::contract::copy_old() ? old_expr :
        boost::contract::null_old())
@endcode

@see @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}

@param old  Old value which is usually implicitly constructed from the user old
            value expression to be copied (use the ternary operator <c>?:</c>
            to completely avoid to evaluate the old value expression when
            @c boost::contract::copy_old() is @c false).

@return Old value pointer (usually implicitly converted to either
        @RefClass{boost::contract::old_ptr} or
        @RefClass{boost::contract::old_ptr_if_copyable} in user code).
*/
/** @cond */ BOOST_CONTRACT_DETAIL_DECLSPEC /** @endcond */
old_pointer make_old(old_value const& old);

/**
Make an old value pointer (for virtual public functions and public functions
overrides).

The related old value pointer will not be null if the specified old value was
actually copied.
This function is usually only called by code expanded by
@c BOOST_CONTRACT_OLDOF(v, old_expr) as in:

@code
boost::contract::make_old(v, boost::contract::copy_old(v) ? old_expr :
        boost::contract::null_old())
@endcode

@see @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}

@param v    The trailing parameter of type
            @RefClass{boost::contract::virtual_}<c>*</c> and default value @c 0
            from the enclosing virtual or overriding public function declaring
            the contract.
@param old  Old value which is usually implicitly constructed from the user old
            value expression to be copied (use the ternary operator <c>?:</c>
            to completely avoid to evaluate the old value expression when
            @c boost::contract::copy_old(v) is @c false).

@return Old value pointer (usually implicitly converted to either
        @RefClass{boost::contract::old_ptr} or
        @RefClass{boost::contract::old_ptr_if_copyable} in user code).
*/
/** @cond */ BOOST_CONTRACT_DETAIL_DECLSPEC /** @endcond */
old_pointer make_old(virtual_* v, old_value const& old);

/**
Query if old values need to be copied (but not for virtual public functions and
public function overrides).

For example, this function always returns false when both postconditions and
exception guarantees are not being checked (see
@RefMacro{BOOST_CONTRACT_NO_OLDS}).
This function is usually only called by the code expanded by
@RefMacro{BOOST_CONTRACT_OLDOF}.

@see @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}

@return True if old values need to be copied, false otherwise.
*/
inline bool copy_old() {
    #ifndef BOOST_CONTRACT_NO_OLDS
        #ifndef BOOST_CONTRACT_ALL_DISABLE_NO_ASSERTION
            return !boost::contract::detail::checking::already();
        #else
            return true;
        #endif
    #else
        return false; // No post checking, so never copy old values.
    #endif
}

/**
Query if old values need to be copied (for virtual public functions and public
function overrides).

For example, this function always returns false when both postconditions and
exception guarantees are not being checked (see
@RefMacro{BOOST_CONTRACT_NO_OLDS}).
In addition, this function returns false when overridden functions are being
called subsequent times by this library to support subcontracting.
This function is usually only called by the code expanded by
@RefMacro{BOOST_CONTRACT_OLDOF}.

@see @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}

@param v    The trailing parameter of type
            @RefClass{boost::contract::virtual_}<c>*</c> and default value @c 0
            from the enclosing virtual or overriding public function declaring
            the contract.

@return True if old values need to be copied, false otherwise.
*/
#ifndef BOOST_CONTRACT_NO_OLDS
    inline bool copy_old(virtual_* v) {
        if(!v) {
            #ifndef BOOST_CONTRACT_ALL_DISABLE_NO_ASSERTION
                return !boost::contract::detail::checking::already();
            #else
                return true;
            #endif
        }
        return v->action_ == boost::contract::virtual_::push_old_init_copy ||
                v->action_ == boost::contract::virtual_::push_old_ftor_copy;
    }
#else
    inline bool copy_old(virtual_*
        #ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
            v
        #endif
    ) {
        return false; // No post checking, so never copy old values.
    }
#endif

} } // namespace

#ifdef BOOST_CONTRACT_HEADER_ONLY
    #include <boost/contract/detail/inlined/old.hpp>
#endif

#endif // #include guard

