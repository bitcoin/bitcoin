// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_OPERATORS_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_OPERATORS_HPP_INCLUDED

#include <iosfwd>
#include <boost/utility/enable_if.hpp>
#include <boost/type_erasure/detail/const.hpp>
#include <boost/type_erasure/call.hpp>
#include <boost/type_erasure/concept_interface.hpp>
#include <boost/type_erasure/placeholder.hpp>
#include <boost/type_erasure/concept_of.hpp>
#include <boost/type_erasure/derived.hpp>
#include <boost/type_erasure/rebind_any.hpp>
#include <boost/type_erasure/param.hpp>
#include <boost/type_erasure/check_match.hpp>
#include <boost/type_erasure/relaxed.hpp>
#include <boost/type_erasure/typeid_of.hpp>

namespace boost {
namespace type_erasure {

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_UNARY_INPLACE_OPERATOR(name, op)                         \
    template<class T = _self>                                                       \
    struct name                                                                     \
    {                                                                               \
        static void apply(T& arg) { op arg; }                                       \
    };                                                                              \
                                                                                    \
    template<class T, class Base>                                                   \
    struct concept_interface<name<T>, Base, T,                                      \
        typename ::boost::enable_if<                                                \
            detail::should_be_non_const<T, Base>                                    \
        >::type                                                                     \
    > : Base                                                                        \
    {                                                                               \
        typedef typename ::boost::type_erasure::derived<Base>::type _derived;       \
        _derived& operator op()                                                     \
        {                                                                           \
            ::boost::type_erasure::call(name<T>(), *this);                          \
            return static_cast<_derived&>(*this);                                   \
        }                                                                           \
        typename ::boost::type_erasure::rebind_any<Base, T>::type operator op(int)  \
        {                                                                           \
            typename ::boost::type_erasure::rebind_any<Base, T>::type result(       \
                static_cast<_derived&>(*this));                                     \
            ::boost::type_erasure::call(name<T>(), *this);                          \
            return result;                                                          \
        }                                                                           \
    };                                                                              \
                                                                                    \
    template<class T, class Base>                                                   \
    struct concept_interface<name<T>, Base, T,                                      \
        typename ::boost::enable_if<                                                \
            detail::should_be_const<T, Base>                                        \
        >::type                                                                     \
    > : Base                                                                        \
    {                                                                               \
        typedef typename ::boost::type_erasure::derived<Base>::type _derived;       \
        const _derived& operator op() const                                         \
        {                                                                           \
            ::boost::type_erasure::call(name<T>(), *this);                          \
            return static_cast<const _derived&>(*this);                             \
        }                                                                           \
        typename ::boost::type_erasure::rebind_any<Base, T>::type operator op(int) const \
        {                                                                           \
            typename ::boost::type_erasure::rebind_any<Base, T>::type result(       \
                static_cast<const _derived&>(*this));                               \
            ::boost::type_erasure::call(name<T>(), *this);                          \
            return result;                                                          \
        }                                                                           \
    };

/**
 * The @ref incrementable concept allow pre and
 * post increment on an @ref any.  The contained
 * type must provide a pre-increment operator.
 */
BOOST_TYPE_ERASURE_UNARY_INPLACE_OPERATOR(incrementable, ++)
/**
 * The @ref decrementable concept allow pre and
 * post decrement on an @ref any.  The contained
 * type must provide a pre-decrement operator.
 */
BOOST_TYPE_ERASURE_UNARY_INPLACE_OPERATOR(decrementable, --)

#undef BOOST_TYPE_ERASURE_UNARY_INPLACE_OPERATOR

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_UNARY_OPERATOR(name, op)                                     \
    template<class T = _self, class R = T>                                              \
    struct name                                                                         \
    {                                                                                   \
        static R apply(const T& arg) { return op arg; }                                 \
    };                                                                                  \
                                                                                        \
    template<class T, class R, class Base>                                              \
    struct concept_interface<name<T, R>, Base, T> : Base                                \
    {                                                                                   \
        typename ::boost::type_erasure::rebind_any<Base, R>::type operator op() const   \
        {                                                                               \
            return ::boost::type_erasure::call(name<T, R>(), *this);                    \
        }                                                                               \
    };

/**
 * The @ref complementable concept allow use of the bitwise
 * complement operator on an @ref any.
 */
BOOST_TYPE_ERASURE_UNARY_OPERATOR(complementable, ~)
/**
 * The @ref negatable concept allow use of the unary
 * minus operator on an @ref any.
 */
BOOST_TYPE_ERASURE_UNARY_OPERATOR(negatable, -)

#undef BOOST_TYPE_ERASURE_UNARY_OPERATOR

template<class R, class T = _self>
struct dereferenceable
{
    static R apply(const T& arg) { return *arg; }
};

/// \cond show_operators

template<class R, class T, class Base>
struct concept_interface<dereferenceable<R, T>, Base, T> : Base
{
    typename ::boost::type_erasure::rebind_any<Base, R>::type operator*() const
    {
        return ::boost::type_erasure::call(dereferenceable<R, T>(), *this);
    }
};

/// \endcond

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_BINARY_OPERATOR(name, op)                        \
    template<class T = _self, class U = T, class R = T>                     \
    struct name                                                             \
    {                                                                       \
        static R apply(const T& lhs, const U& rhs) { return lhs op rhs; }   \
    };                                                                      \
                                                                            \
    template<class T, class U, class R, class Base>                         \
    struct concept_interface<name<T, U, R>, Base, T> : Base                 \
    {                                                                       \
        friend typename rebind_any<Base, R>::type                           \
        operator op(const typename derived<Base>::type& lhs,                \
                    typename as_param<Base, const U&>::type rhs)            \
        {                                                                   \
            return ::boost::type_erasure::call(name<T, U, R>(), lhs, rhs);  \
        }                                                                   \
    };                                                                      \
                                                                            \
    template<class T, class U, class R, class Base>                         \
    struct concept_interface<                                               \
        name<T, U, R>,                                                      \
        Base,                                                               \
        U,                                                                  \
        typename ::boost::disable_if<                                       \
            ::boost::type_erasure::is_placeholder<T> >::type                \
    > : Base                                                                \
    {                                                                       \
        friend typename rebind_any<Base, R>::type                           \
        operator op(const T& lhs,                                           \
                    const typename derived<Base>::type& rhs)                \
        {                                                                   \
            return ::boost::type_erasure::call(name<T, U, R>(), lhs, rhs);  \
        }                                                                   \
    };

BOOST_TYPE_ERASURE_BINARY_OPERATOR(addable, +)
BOOST_TYPE_ERASURE_BINARY_OPERATOR(subtractable, -)
BOOST_TYPE_ERASURE_BINARY_OPERATOR(multipliable, *)
BOOST_TYPE_ERASURE_BINARY_OPERATOR(dividable, /)
BOOST_TYPE_ERASURE_BINARY_OPERATOR(modable, %)
BOOST_TYPE_ERASURE_BINARY_OPERATOR(left_shiftable, <<)
BOOST_TYPE_ERASURE_BINARY_OPERATOR(right_shiftable, >>)
BOOST_TYPE_ERASURE_BINARY_OPERATOR(bitandable, &)
BOOST_TYPE_ERASURE_BINARY_OPERATOR(bitorable, |)
BOOST_TYPE_ERASURE_BINARY_OPERATOR(bitxorable, ^)

#undef BOOST_TYPE_ERASURE_BINARY_OPERATOR

/** INTERNAL ONLY */
#define BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(name, op)                    \
    template<class T = _self, class U = T>                                  \
    struct name                                                             \
    {                                                                       \
        static void apply(T& lhs, const U& rhs) { lhs op rhs; }             \
    };                                                                      \
                                                                            \
    template<class T, class U, class Base>                                  \
    struct concept_interface<name<T, U>, Base, T,                           \
        typename ::boost::disable_if<                                       \
            ::boost::is_same<                                               \
                typename ::boost::type_erasure::placeholder_of<Base>::type, \
                const T&                                                    \
            >                                                               \
        >::type                                                             \
    > : Base                                                                \
    {                                                                       \
        friend typename detail::non_const_this_param<Base>::type&           \
        operator op(typename detail::non_const_this_param<Base>::type& lhs, \
                    typename as_param<Base, const U&>::type rhs)            \
        {                                                                   \
            ::boost::type_erasure::call(name<T, U>(),lhs, rhs);             \
            return lhs;                                                     \
        }                                                                   \
    };                                                                      \
                                                                            \
    template<class T, class U, class Base>                                  \
    struct concept_interface<                                               \
        name<T, U>,                                                         \
        Base,                                                               \
        U,                                                                  \
        typename ::boost::disable_if<                                       \
            ::boost::type_erasure::is_placeholder<T> >::type                \
    > : Base                                                                \
    {                                                                       \
        friend T&                                                           \
        operator op(T& lhs, const typename derived<Base>::type& rhs)        \
        {                                                                   \
            ::boost::type_erasure::call(name<T, U>(),lhs, rhs);             \
            return lhs;                                                     \
        }                                                                   \
    };

BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(add_assignable, +=)
BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(subtract_assignable, -=)
BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(multiply_assignable, *=)
BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(divide_assignable, /=)
BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(mod_assignable, %=)
BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(left_shift_assignable, <<=)
BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(right_shift_assignable, >>=)
BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(bitand_assignable, &=)
BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(bitor_assignable, |=)
BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR(bitxor_assignable, ^=)

#undef BOOST_TYPE_ERASURE_ASSIGNMENT_OPERATOR

template<class T = _self, class U = T>
struct equality_comparable
{
    static bool apply(const T& lhs, const U& rhs) { return lhs == rhs; }
};

/// \cond show_operators

template<class T, class U, class Base>
struct concept_interface<equality_comparable<T, U>, Base, T> : Base
{
    friend bool operator==(const typename derived<Base>::type& lhs,
                           typename as_param<Base, const U&>::type rhs)
    {
        if(::boost::type_erasure::check_match(equality_comparable<T, U>(), lhs, rhs)) {
            return ::boost::type_erasure::unchecked_call(equality_comparable<T, U>(), lhs, rhs);
        } else {
            return false;
        }
    }
    friend bool operator!=(const typename derived<Base>::type& lhs,
                           typename as_param<Base, const U&>::type rhs)
    {
        return !(lhs == rhs);
    }
};

template<class T, class U, class Base>
struct concept_interface<
    equality_comparable<T, U>,
    Base,
    U,
    typename ::boost::disable_if< ::boost::type_erasure::is_placeholder<T> >::type
> : Base
{
    friend bool operator==(const T& lhs, const typename derived<Base>::type& rhs)
    {
        return ::boost::type_erasure::call(equality_comparable<T, U>(), lhs, rhs);
    }
    friend bool operator!=(const T& lhs, const typename derived<Base>::type& rhs)
    {
        return !(lhs == rhs);
    }
};

/// \endcond

template<class T = _self, class U = T>
struct less_than_comparable
{
    static bool apply(const T& lhs, const U& rhs) { return lhs < rhs; }
};

namespace detail {

template<class F, class T, class U>
bool less_impl(const F& f, const T& lhs, const U& rhs, ::boost::mpl::true_)
{
    if(::boost::type_erasure::check_match(f, lhs, rhs)) {
        return ::boost::type_erasure::unchecked_call(f, lhs, rhs);
    } else {
        return ::boost::type_erasure::typeid_of(
            static_cast<const typename derived<T>::type&>(lhs)
        ).before(
            ::boost::type_erasure::typeid_of(
                static_cast<const typename derived<U>::type&>(rhs)
            )
        ) != false;
    }
}

template<class F, class T, class U>
bool less_impl(const F& f, const T& lhs, const U& rhs, ::boost::mpl::false_)
{
    return ::boost::type_erasure::call(f, lhs, rhs);
}

}

/// \cond show_operators

template<class T, class Base>
struct concept_interface<less_than_comparable<T, T>, Base, T> : Base
{
    friend bool operator<(const typename derived<Base>::type& lhs,
                          typename as_param<Base, const T&>::type rhs)
    {
        return ::boost::type_erasure::detail::less_impl(
            less_than_comparable<T, T>(),
            lhs, rhs,
            ::boost::type_erasure::is_relaxed<
                typename ::boost::type_erasure::concept_of<Base>::type>());
    }
    friend bool operator>=(const typename derived<Base>::type& lhs,
                           typename as_param<Base, const T&>::type rhs)
    {
        return !(lhs < rhs);
    }
    friend bool operator>(typename as_param<Base, const T&>::type lhs,
                          const typename derived<Base>::type& rhs)
    {
        return rhs < lhs;
    }
    friend bool operator<=(typename as_param<Base, const T&>::type lhs,
                          const typename derived<Base>::type& rhs)
    {
        return !(rhs < lhs);
    }
};

template<class T, class U, class Base>
struct concept_interface<less_than_comparable<T, U>, Base, T> : Base
{
    friend bool operator<(const typename derived<Base>::type& lhs,
                          typename as_param<Base, const U&>::type rhs)
    {
        return ::boost::type_erasure::call(less_than_comparable<T, U>(), lhs, rhs);
    }
    friend bool operator>=(const typename derived<Base>::type& lhs,
                           typename as_param<Base, const U&>::type rhs)
    {
        return !(lhs < rhs);
    }
    friend bool operator>(typename as_param<Base, const U&>::type lhs,
                          const typename derived<Base>::type& rhs)
    {
        return rhs < lhs;
    }
    friend bool operator<=(typename as_param<Base, const U&>::type lhs,
                          const typename derived<Base>::type& rhs)
    {
        return !(rhs < lhs);
    }
};

template<class T, class U, class Base>
struct concept_interface<
    less_than_comparable<T, U>,
    Base,
    U,
    typename ::boost::disable_if< ::boost::type_erasure::is_placeholder<T> >::type
> : Base
{
    friend bool operator<(const T& lhs, const typename derived<Base>::type& rhs)
    {
        return ::boost::type_erasure::call(less_than_comparable<T, U>(), lhs, rhs);
    }
    friend bool operator>=(const T& lhs, const typename derived<Base>::type& rhs)
    {
        return !(lhs < rhs);
    }
    friend bool operator>(const typename derived<Base>::type& lhs, const T& rhs)
    {
        return rhs < lhs;
    }
    friend bool operator<=(const typename derived<Base>::type& lhs, const T& rhs)
    {
        return !(rhs < lhs);
    }
};

/// \endcond

template<class R, class T = _self, class N = std::ptrdiff_t>
struct subscriptable
{
    static R apply(T& arg, const N& index) { return arg[index]; }
};

/// \cond show_operators

template<class R, class T, class N, class Base>
struct concept_interface<subscriptable<R, T, N>, Base, typename ::boost::remove_const<T>::type,
    typename ::boost::enable_if<
        ::boost::type_erasure::detail::should_be_non_const<T, Base>
    >::type
> : Base
{
    typename ::boost::type_erasure::rebind_any<Base, R>::type operator[](
        typename ::boost::type_erasure::as_param<Base, const N&>::type index)
    {
        return ::boost::type_erasure::call(subscriptable<R, T, N>(), *this, index);
    }
};

template<class R, class T, class N, class Base>
struct concept_interface<subscriptable<R, T, N>, Base, typename ::boost::remove_const<T>::type,
    typename ::boost::enable_if<
        ::boost::type_erasure::detail::should_be_const<T, Base>
    >::type
> : Base
{
    typename ::boost::type_erasure::rebind_any<Base, R>::type operator[](
        typename ::boost::type_erasure::as_param<Base, const N&>::type index) const
    {
        return ::boost::type_erasure::call(subscriptable<R, const T, N>(), *this, index);
    }
};

/// \endcond

/**
 * The @ref ostreamable concept allows an @ref any to be
 * written to a @c std::ostream.
 */
template<class Os = std::ostream, class T = _self>
struct ostreamable
{
    static void apply(Os& out, const T& arg) { out << arg; }
};

/// \cond show_operators

template<class Base, class Os, class T>
struct concept_interface<ostreamable<Os, T>, Base, Os> : Base
{
    friend typename detail::non_const_this_param<Base>::type&
    operator<<(typename detail::non_const_this_param<Base>::type& lhs,
               typename ::boost::type_erasure::as_param<Base, const T&>::type rhs)
    {
        ::boost::type_erasure::call(ostreamable<Os, T>(), lhs, rhs);
        return lhs;
    }
};

template<class Base, class Os, class T>
struct concept_interface<
    ostreamable<Os, T>,
    Base,
    T,
    typename ::boost::disable_if< ::boost::type_erasure::is_placeholder<Os> >::type
> : Base
{
    friend Os&
    operator<<(Os& lhs,
               const typename ::boost::type_erasure::derived<Base>::type& rhs)
    {
        ::boost::type_erasure::call(ostreamable<Os, T>(), lhs, rhs);
        return lhs;
    }
};

/// \endcond

/**
 * The @ref istreamable concept allows an @ref any to be
 * read from a @c std::istream.
 */
template<class Is = std::istream, class T = _self>
struct istreamable
{
    static void apply(Is& out, T& arg) { out >> arg; }
};

/// \cond show_operators


template<class Base, class Is, class T>
struct concept_interface<istreamable<Is, T>, Base, Is> : Base
{
    friend typename detail::non_const_this_param<Base>::type&
    operator>>(typename detail::non_const_this_param<Base>::type& lhs,
               typename ::boost::type_erasure::as_param<Base, T&>::type rhs)
    {
        ::boost::type_erasure::call(istreamable<Is, T>(), lhs, rhs);
        return lhs;
    }
};

template<class Base, class Is, class T>
struct concept_interface<
    istreamable<Is, T>,
    Base,
    T,
    typename ::boost::disable_if< ::boost::type_erasure::is_placeholder<Is> >::type
> : Base
{
    friend Is&
    operator>>(Is& lhs,
               typename ::boost::type_erasure::derived<Base>::type& rhs)
    {
        ::boost::type_erasure::call(istreamable<Is, T>(), lhs, rhs);
        return lhs;
    }
};

/// \endcond

}
}

#endif
