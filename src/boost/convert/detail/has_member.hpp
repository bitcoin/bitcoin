// Copyright (c) 2009-2020 Vladimir Batov.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. See http://www.boost.org/LICENSE_1_0.txt.

#ifndef BOOST_CONVERT_HAS_MEMBER_HPP
#define BOOST_CONVERT_HAS_MEMBER_HPP

#include <boost/type_traits/detail/yes_no_type.hpp>

// This macro allows to check if a type has a member named "__member_name__"...
// ... regardless of the signature. If takes advantage of the following behavior related to
// function resolution. Say, both, foo and base, declare a method with the same name "func":
//
//      struct   foo { int  func (int, int) { return 0; } };
//      struct  base { void func () {} };
//      struct mixin : public foo, public base {};
//
// Now, if we inherit from both -- foo and base -- classes, then the following calls will fail
//      mixin_ptr(0)->func();
//      mixin_ptr(0)->func(5, 5);
// with the error message (gcc): request for member func is ambiguous
// regardless if we provide any arguments or not even though one might expect that
// arg-based signature resolution might kick in. The only way to deploy those methods is:
//
//      mixin_ptr(0)->foo::func();
//      mixin_ptr(0)->base::func(5, 5);
//
// C2. The actual signature of __member_name__ is not taken into account. If
//     __T__::__member_name__(any-signature) exists, then the introduced base::__member_name__
//     will cause mixin->__member_name__() call to fail to compile (due to ambiguity).
// C3. &U::__member_name__ (a.k.a. &mixin::__member_name__)
//     has the type of func_type only if __T__::__member_name__ does not exist.
//     If __T__::member_name does exist, then mixin::__member_name__ is ambiguous
//     and "yes_type test (...)" kicks in instead.
// C4. Need to find some unique/ugly name so that it does not clash if this macro is
//     used inside some other template class;

#define BOOST_DECLARE_HAS_MEMBER(__trait_name__, __member_name__)                           \
                                                                                            \
    template <typename __boost_has_member_T__> /*C4*/                                       \
    class __trait_name__                                                                    \
    {                                                                                       \
        using check_type = typename boost::remove_const<__boost_has_member_T__>::type;      \
        using   yes_type = ::boost::type_traits::yes_type;                                  \
        using    no_type = ::boost::type_traits:: no_type;                                  \
                                                                                            \
        struct  base { void __member_name__(/*C2*/) {}};                                    \
        struct mixin : public base, public check_type {};                                   \
                                                                                            \
        template <void (base::*)()> struct aux {};                                          \
                                                                                            \
        template <typename U> static no_type  test(aux<&U::__member_name__>*); /*C3*/       \
        template <typename U> static yes_type test(...);                                    \
                                                                                            \
        public:                                                                             \
                                                                                            \
        BOOST_STATIC_CONSTANT(bool, value = (sizeof(yes_type) == sizeof(test<mixin>(0))));  \
    }

#endif // BOOST_CONVERT_HAS_MEMBER_HPP
