
#ifndef BOOST_CONTRACT_ACCESS_HPP_
#define BOOST_CONTRACT_ACCESS_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Allow to declare invariants, base types, etc all as private members.
*/

// IMPORTANT: Included by contract_macro.hpp so must #if-guard all its includes.
#include <boost/contract/core/config.hpp>
#if !defined(BOOST_CONTRACT_NO_CONDITIONS) || \
        defined(BOOST_CONTRACT_STATIC_LINK)
    #include <boost/contract/detail/decl.hpp>
    #include <boost/contract/detail/type_traits/mirror.hpp>
#endif
#ifndef BOOST_CONTRACT_NO_INVARIANTS
    #include <boost/contract/detail/debug.hpp>
    #include <boost/function_types/property_tags.hpp>
    #include <boost/mpl/vector.hpp>
#endif

namespace boost { namespace contract {
        
#if !defined(BOOST_CONTRACT_NO_CONDITIONS) || \
        defined(BOOST_CONTRACT_STATIC_LINK)
    class virtual_;

    namespace detail {
        BOOST_CONTRACT_DETAIL_DECL_DETAIL_COND_SUBCONTRACTING_Z(1,
                /* is_friend = */ 0, OO, RR, FF, CC, AArgs);
    }
#endif
#ifndef BOOST_CONTRACT_NO_INVARIANTS
    namespace detail {
        template<typename RR, class CC>
        class cond_inv;
    }
#endif

/**
Declare this class as friend to program invariants and base types as private
members.

Declare this class a friend of the user-defined class specifying the contracts
and then invariant functions and the base types @c typedef can be declared as
non-public members:

@code
class u
    #define BASES public b, private w
    : BASES
{
    friend class boost::contract::access;

    typedef BOOST_CONTRACT_BASE_TYPES(BASES) base_types; // Private.
    #undef BASES

    void invariant() const { ... } // Private (same for static and volatile).

public:
    ...
};
@endcode

In real code, programmers will likely chose to declare this class as friend so
to fully control public interfaces of their user-defined classes (this is not
extensively done in the examples of this documentation only for brevity).
This class is not intended to be directly used by programmers a part from
being declared as @c friend (and that is why this class does not have any public
member and it is not copyable).

@warning    Not declaring this class friend of user-defined classes will cause
            compiler errors on some compilers (e.g., MSVC) because the private
            members needed to check the contracts will not be accessible.
            On other compilers (e.g., GCC and CLang), the private access will
            instead fail SFINAE and no compiler error will be reported while
            invariants and subcontracting will be silently skipped at run-time.
            Therefore, programmers must make sure to either declare this class
            as friend or to always declare invariant functions and base types
            @c typedef as public members.

@see @RefSect{advanced.access_specifiers, Access Specifiers}
*/
class access { // Non-copyable (see below).
/** @cond */
private: // No public APIs (so users cannot use it directly by mistake).

    access(); // Should never be constructed (not even internally).
    ~access();
    
    // No boost::noncopyable to avoid its overhead when contracts disabled.
    access(access&);
    access& operator=(access&);
    
    #if !defined(BOOST_CONTRACT_NO_CONDITIONS) || \
            defined(BOOST_CONTRACT_STATIC_LINK)
        BOOST_CONTRACT_DETAIL_MIRROR_HAS_TYPE(has_base_types,
                BOOST_CONTRACT_BASES_TYPEDEF)

        template<class C>
        struct base_types_of {
            typedef typename C::BOOST_CONTRACT_BASES_TYPEDEF type;
        };
    #endif

    #ifndef BOOST_CONTRACT_NO_INVARIANTS
        BOOST_CONTRACT_DETAIL_MIRROR_HAS_MEMBER_FUNCTION(
                has_static_invariant_f, BOOST_CONTRACT_STATIC_INVARIANT_FUNC)
        
        BOOST_CONTRACT_DETAIL_MIRROR_HAS_STATIC_MEMBER_FUNCTION(
                has_static_invariant_s, BOOST_CONTRACT_STATIC_INVARIANT_FUNC)

        template<class C>
        struct has_static_invariant : has_static_invariant_s<C, void,
                boost::mpl::vector<> > {};

        template<class C>
        static void static_invariant() {
            C::BOOST_CONTRACT_STATIC_INVARIANT_FUNC();
        }

        template<class C>
        class static_invariant_addr { // Class so to pass it as tparam.
            typedef void (*func_ptr)();
        public:
            static func_ptr apply() {
                return &C::BOOST_CONTRACT_STATIC_INVARIANT_FUNC;
            }
        };

        BOOST_CONTRACT_DETAIL_MIRROR_HAS_MEMBER_FUNCTION(
                has_invariant_f, BOOST_CONTRACT_INVARIANT_FUNC)
        
        BOOST_CONTRACT_DETAIL_MIRROR_HAS_STATIC_MEMBER_FUNCTION(
                has_invariant_s, BOOST_CONTRACT_INVARIANT_FUNC)

        template<class C>
        struct has_cv_invariant : has_invariant_f<C, void, boost::mpl::vector<>,
                boost::function_types::cv_qualified> {};
        
        template<class C>
        struct has_const_invariant : has_invariant_f<C, void, boost::mpl::
                vector<>, boost::function_types::const_qualified> {};

        template<class C>
        static void cv_invariant(C const volatile* obj) {
            BOOST_CONTRACT_DETAIL_DEBUG(obj);
            obj->BOOST_CONTRACT_INVARIANT_FUNC();
        }
        
        template<class C>
        static void const_invariant(C const* obj) {
            BOOST_CONTRACT_DETAIL_DEBUG(obj);
            obj->BOOST_CONTRACT_INVARIANT_FUNC();
        }
    #endif
    
    // Friends (used to limit library's public API).
    // NOTE: Using friends here and in all other places in this library
    // does not increase compilation times (I experimented replacing all
    // friends with public and got the same compilation times).
    #if !defined(BOOST_CONTRACT_NO_CONDITIONS) || \
            defined(BOOST_CONTRACT_STATIC_LINK)
        BOOST_CONTRACT_DETAIL_DECL_DETAIL_COND_SUBCONTRACTING_Z(1,
                /* is_friend = */ 1, OO, RR, FF, CC, AArgs);
            
        BOOST_CONTRACT_DETAIL_DECL_FRIEND_OVERRIDING_PUBLIC_FUNCTIONS_Z(1,
                OO, RR, FF, CC, AArgs, vv, rr, ff, oobj, aargs)
    #endif
    #ifndef BOOST_CONTRACT_NO_INVARIANTS
        template<typename RR, class CC>
        friend class boost::contract::detail::cond_inv;
    #endif
/** @endcond */
};

} } // namespace

#endif // #include guard

