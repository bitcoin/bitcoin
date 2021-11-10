#ifndef BOOST_LEAF_ON_ERROR_HPP_INCLUDED
#define BOOST_LEAF_ON_ERROR_HPP_INCLUDED

/// Copyright (c) 2018-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_LEAF_ENABLE_WARNINGS ///
#   if defined(_MSC_VER) ///
#       pragma warning(push,1) ///
#   elif defined(__clang__) ///
#       pragma clang system_header ///
#   elif (__GNUC__*100+__GNUC_MINOR__>301) ///
#       pragma GCC system_header ///
#   endif ///
#endif ///

#include <boost/leaf/error.hpp>

namespace boost { namespace leaf {

class error_monitor
{
#if !defined(BOOST_LEAF_NO_EXCEPTIONS) && BOOST_LEAF_STD_UNCAUGHT_EXCEPTIONS
    int const uncaught_exceptions_;
#endif
    int const err_id_;

public:

    error_monitor() noexcept:
#if !defined(BOOST_LEAF_NO_EXCEPTIONS) && BOOST_LEAF_STD_UNCAUGHT_EXCEPTIONS
        uncaught_exceptions_(std::uncaught_exceptions()),
#endif
        err_id_(leaf_detail::current_id())
    {
    }

    int check_id() const noexcept
    {
        int err_id = leaf_detail::current_id();
        if( err_id != err_id_ )
            return err_id;
        else
        {
#ifndef BOOST_LEAF_NO_EXCEPTIONS
#   if BOOST_LEAF_STD_UNCAUGHT_EXCEPTIONS
            if( std::uncaught_exceptions() > uncaught_exceptions_ )
#   else
            if( std::uncaught_exception() )
#   endif
                return leaf_detail::new_id();
#endif
            return 0;
        }
    }

    int get_id() const noexcept
    {
        int err_id = leaf_detail::current_id();
        if( err_id != err_id_ )
            return err_id;
        else
            return leaf_detail::new_id();
    }

    error_id check() const noexcept
    {
        return leaf_detail::make_error_id(check_id());
    }

    error_id assigned_error_id() const noexcept
    {
        return leaf_detail::make_error_id(get_id());
    }
};

////////////////////////////////////////////

namespace leaf_detail
{
    template <int I, class Tuple>
    struct tuple_for_each_preload
    {
        BOOST_LEAF_CONSTEXPR static void trigger( Tuple & tup, int err_id ) noexcept
        {
            BOOST_LEAF_ASSERT((err_id&3)==1);
            tuple_for_each_preload<I-1,Tuple>::trigger(tup,err_id);
            std::get<I-1>(tup).trigger(err_id);
        }
    };

    template <class Tuple>
    struct tuple_for_each_preload<0, Tuple>
    {
        BOOST_LEAF_CONSTEXPR static void trigger( Tuple const &, int ) noexcept { }
    };

    template <class E>
    class preloaded_item
    {
        using decay_E = typename std::decay<E>::type;
        slot<decay_E> * s_;
        decay_E e_;

    public:

        BOOST_LEAF_CONSTEXPR preloaded_item( E && e ):
            s_(tl_slot_ptr<decay_E>::p),
            e_(std::forward<E>(e))
        {
        }

        BOOST_LEAF_CONSTEXPR void trigger( int err_id ) noexcept
        {
            BOOST_LEAF_ASSERT((err_id&3)==1);
            if( s_ )
            {
                if( !s_->has_value(err_id) )
                    s_->put(err_id, std::move(e_));
            }
#if BOOST_LEAF_DIAGNOSTICS
            else
            {
                int c = tl_unexpected_enabled<>::counter;
                BOOST_LEAF_ASSERT(c>=0);
                if( c )
                    load_unexpected(err_id, std::move(e_));
            }
#endif
        }
    };

    template <class F>
    class deferred_item
    {
        using E = decltype(std::declval<F>()());
        slot<E> * s_;
        F f_;

    public:

        BOOST_LEAF_CONSTEXPR deferred_item( F && f ) noexcept:
            s_(tl_slot_ptr<E>::p),
            f_(std::forward<F>(f))
        {
        }

        BOOST_LEAF_CONSTEXPR void trigger( int err_id ) noexcept
        {
            BOOST_LEAF_ASSERT((err_id&3)==1);
            if( s_ )
            {
                if( !s_->has_value(err_id) )
                    s_->put(err_id, f_());
            }
#if BOOST_LEAF_DIAGNOSTICS
            else
            {
                int c = tl_unexpected_enabled<>::counter;
                BOOST_LEAF_ASSERT(c>=0);
                if( c )
                    load_unexpected(err_id, std::forward<E>(f_()));
            }
#endif
        }
    };

    template <class F, class A0 = fn_arg_type<F,0>, int arity = function_traits<F>::arity>
    class accumulating_item;

    template <class F, class A0>
    class accumulating_item<F, A0 &, 1>
    {
        using E = A0;
        slot<E> * s_;
        F f_;

    public:

        BOOST_LEAF_CONSTEXPR accumulating_item( F && f ) noexcept:
            s_(tl_slot_ptr<E>::p),
            f_(std::forward<F>(f))
        {
        }

        BOOST_LEAF_CONSTEXPR void trigger( int err_id ) noexcept
        {
            BOOST_LEAF_ASSERT((err_id&3)==1);
            if( s_ )
                if( E * e = s_->has_value(err_id) )
                    (void) f_(*e);
                else
                    (void) f_(s_->put(err_id, E()));
        }
    };

    template <class... Item>
    class preloaded
    {
        preloaded & operator=( preloaded const & ) = delete;

        std::tuple<Item...> p_;
        bool moved_;
        error_monitor id_;

    public:

        BOOST_LEAF_CONSTEXPR explicit preloaded( Item && ... i ):
            p_(std::forward<Item>(i)...),
            moved_(false)
        {
        }

        BOOST_LEAF_CONSTEXPR preloaded( preloaded && x ) noexcept:
            p_(std::move(x.p_)),
            moved_(false),
            id_(std::move(x.id_))
        {
            x.moved_ = true;
        }

        ~preloaded() noexcept
        {
            if( moved_ )
                return;
            if( auto id = id_.check_id() )
                tuple_for_each_preload<sizeof...(Item),decltype(p_)>::trigger(p_,id);
        }
    };

    template <class T, int arity = function_traits<T>::arity>
    struct deduce_item_type;

    template <class T>
    struct deduce_item_type<T, -1>
    {
        using type = preloaded_item<T>;
    };

    template <class F>
    struct deduce_item_type<F, 0>
    {
        using type = deferred_item<F>;
    };

    template <class F>
    struct deduce_item_type<F, 1>
    {
        using type = accumulating_item<F>;
    };
}

template <class... Item>
BOOST_LEAF_NODISCARD BOOST_LEAF_CONSTEXPR inline
leaf_detail::preloaded<typename leaf_detail::deduce_item_type<Item>::type...>
on_error( Item && ... i )
{
    return leaf_detail::preloaded<typename leaf_detail::deduce_item_type<Item>::type...>(std::forward<Item>(i)...);
}

} }

#if defined(_MSC_VER) && !defined(BOOST_LEAF_ENABLE_WARNINGS) ///
#pragma warning(pop) ///
#endif ///

#endif
