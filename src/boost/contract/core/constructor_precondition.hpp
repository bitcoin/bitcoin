
#ifndef BOOST_CONTRACT_CONSTRUCTOR_PRECONDITION_HPP_
#define BOOST_CONTRACT_CONSTRUCTOR_PRECONDITION_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Program preconditions for constructors.
*/

// IMPORTANT: Included by contract_macro.hpp so must #if-guard all its includes.
#include <boost/contract/core/config.hpp>
#ifndef BOOST_CONTRACT_NO_PRECONDITIONS
    #include <boost/contract/core/exception.hpp>
    #ifndef BOOST_CONTRACT_ALL_DISABLE_NO_ASSERTION
        #include <boost/contract/detail/checking.hpp>
    #endif
#endif

namespace boost { namespace contract {

/**
Program preconditions for constructors.

This class must be the very first base of the class declaring the
constructor for which preconditions are programmed (that way constructor
arguments can be checked by preconditions even before they are used to
initialize other base classes):

@code
    class u
        #define BASES private boost::contract::constructor_precondition<u>, \
                public b
        : BASES
    {
        friend class boost::contract::access;

        typedef BOOST_CONTRACT_BASE_TYPES(BASES) base_types;
        #undef BASES

    public:
        explicit u(unsigned x) :
            boost::contract::constructor_precondition<u>([&] {
                BOOST_CONTRACT_ASSERT(x != 0);
                ...
            }),
            b(1.0 / float(x))
        {
            ...
        }

        ...
    };
@endcode

User-defined classes should inherit privately from this class (to not alter the
public interface of user-defined classes).
In addition, this class should never be declared as a virtual base (because
virtual bases are initialized only once across the entire inheritance hierarchy
preventing preconditions of other base classes from being checked).

This class cannot be used this way in a @c union because unions cannot have base
classes in C++.
Instead, this class is used in a @c union to declare a local object within the
constructor definition just before @RefFunc{boost::contract::constructor} is
used (see @RefSect{extras.unions, Unions}).

@see @RefSect{tutorial.constructors, Constructors}

@tparam Class   The class type of the constructor for which preconditions are
                being programmed.
*/
template<class Class>
class constructor_precondition { // Copyable (has no data).
public:
    /**
    Construct this object without specifying constructor preconditions.

    This is implicitly called for those constructors of the contracted class
    that do not specify preconditions.
    
    @note   The implementation of this library is optimized so that calling this
            default constructor should amount to negligible compile-time and
            run-time overheads (likely to be optimized away completely by most
            compilers).
    */
    constructor_precondition() {}

    /**
    Construct this object specifying constructor preconditions.

    @param f    Nullary functor called by this library to check constructor
                preconditions @c f().
                Assertions within this functor call are usually programmed
                using @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown
                by a call to this functor indicates a contract failure (and will
                result in this library calling
                @RefFunc{boost::contract::precondition_failure}).
                This functor should capture variables by (constant) value, or
                better by (constant) reference to avoid extra copies.
    */
    template<typename F>
    explicit constructor_precondition(F const& f) {
        #ifndef BOOST_CONTRACT_NO_PRECONDITIONS
            try {
                #ifndef BOOST_CONTRACT_ALL_DISABLE_NO_ASSERTION
                    if(boost::contract::detail::checking::already()) return;
                    #ifndef BOOST_CONTRACT_PRECONDITIONS_DISABLE_NO_ASSERTION
                        boost::contract::detail::checking k;
                    #endif
                #endif
                f();
            } catch(...) { precondition_failure(from_constructor); }
        #endif
    }

    // Default copy operations (so user's derived classes can be copied, etc.).
};

} } // namespace

#endif // #include guard

