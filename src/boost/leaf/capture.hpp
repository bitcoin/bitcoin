#ifndef BOOST_LEAF_CAPTURE_HPP_INCLUDED
#define BOOST_LEAF_CAPTURE_HPP_INCLUDED

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

#include <boost/leaf/exception.hpp>
#include <boost/leaf/on_error.hpp>

namespace boost { namespace leaf {

namespace leaf_detail
{
    template <class R, bool IsResult = is_result_type<R>::value>
    struct is_result_tag;

    template <class R>
    struct is_result_tag<R, false>
    {
    };

    template <class R>
    struct is_result_tag<R, true>
    {
    };
}

#ifdef BOOST_LEAF_NO_EXCEPTIONS

namespace leaf_detail
{
    template <class R, class F, class... A>
    inline
    decltype(std::declval<F>()(std::forward<A>(std::declval<A>())...))
    capture_impl(is_result_tag<R, false>, context_ptr && ctx, F && f, A... a) noexcept
    {
        auto active_context = activate_context(*ctx);
        return std::forward<F>(f)(std::forward<A>(a)...);
    }

    template <class R, class F, class... A>
    inline
    decltype(std::declval<F>()(std::forward<A>(std::declval<A>())...))
    capture_impl(is_result_tag<R, true>, context_ptr && ctx, F && f, A... a) noexcept
    {
        auto active_context = activate_context(*ctx);
        if( auto r = std::forward<F>(f)(std::forward<A>(a)...) )
            return r;
        else
        {
            ctx->captured_id_ = r.error();
            return std::move(ctx);
        }
    }

    template <class R, class Future>
    inline
    decltype(std::declval<Future>().get())
    future_get_impl(is_result_tag<R, false>, Future & fut) noexcept
    {
        return fut.get();
    }

    template <class R, class Future>
    inline
    decltype(std::declval<Future>().get())
    future_get_impl(is_result_tag<R, true>, Future & fut) noexcept
    {
        if( auto r = fut.get() )
            return r;
        else
            return error_id(r.error()); // unloads
    }
}

#else

namespace leaf_detail
{
    class capturing_exception:
        public std::exception
    {
        std::exception_ptr ex_;
        context_ptr ctx_;

    public:

        capturing_exception(std::exception_ptr && ex, context_ptr && ctx) noexcept:
            ex_(std::move(ex)),
            ctx_(std::move(ctx))
        {
            BOOST_LEAF_ASSERT(ex_);
            BOOST_LEAF_ASSERT(ctx_);
            BOOST_LEAF_ASSERT(ctx_->captured_id_);
        }

        [[noreturn]] void unload_and_rethrow_original_exception() const
        {
            BOOST_LEAF_ASSERT(ctx_->captured_id_);
            auto active_context = activate_context(*ctx_);
            id_factory<>::current_id = ctx_->captured_id_.value();
            std::rethrow_exception(ex_);
        }

        template <class CharT, class Traits>
        void print( std::basic_ostream<CharT, Traits> & os ) const
        {
            ctx_->print(os);
        }
    };

    template <class R, class F, class... A>
    inline
    decltype(std::declval<F>()(std::forward<A>(std::declval<A>())...))
    capture_impl(is_result_tag<R, false>, context_ptr && ctx, F && f, A... a)
    {
        auto active_context = activate_context(*ctx);
        error_monitor cur_err;
        try
        {
            return std::forward<F>(f)(std::forward<A>(a)...);
        }
        catch( capturing_exception const & )
        {
            throw;
        }
        catch( exception_base const & e )
        {
            ctx->captured_id_ = e.get_error_id();
            throw_exception( capturing_exception(std::current_exception(), std::move(ctx)) );
        }
        catch(...)
        {
            ctx->captured_id_ = cur_err.assigned_error_id();
            throw_exception( capturing_exception(std::current_exception(), std::move(ctx)) );
        }
    }

    template <class R, class F, class... A>
    inline
    decltype(std::declval<F>()(std::forward<A>(std::declval<A>())...))
    capture_impl(is_result_tag<R, true>, context_ptr && ctx, F && f, A... a)
    {
        auto active_context = activate_context(*ctx);
        error_monitor cur_err;
        try
        {
            if( auto && r = std::forward<F>(f)(std::forward<A>(a)...) )
                return std::move(r);
            else
            {
                ctx->captured_id_ = r.error();
                return std::move(ctx);
            }
        }
        catch( capturing_exception const & )
        {
            throw;
        }
        catch( exception_base const & e )
        {
            ctx->captured_id_ = e.get_error_id();
            throw_exception( capturing_exception(std::current_exception(), std::move(ctx)) );
        }
        catch(...)
        {
            ctx->captured_id_ = cur_err.assigned_error_id();
            throw_exception( capturing_exception(std::current_exception(), std::move(ctx)) );
        }
    }

    template <class R, class Future>
    inline
    decltype(std::declval<Future>().get())
    future_get_impl(is_result_tag<R, false>, Future & fut )
    {
        try
        {
            return fut.get();
        }
        catch( capturing_exception const & cap )
        {
            cap.unload_and_rethrow_original_exception();
        }
    }

    template <class R, class Future>
    inline
    decltype(std::declval<Future>().get())
    future_get_impl(is_result_tag<R, true>, Future & fut )
    {
        try
        {
            if( auto r = fut.get() )
                return r;
            else
                return error_id(r.error()); // unloads
        }
        catch( capturing_exception const & cap )
        {
            cap.unload_and_rethrow_original_exception();
        }
    }
}

#endif

template <class F, class... A>
inline
decltype(std::declval<F>()(std::forward<A>(std::declval<A>())...))
capture(context_ptr && ctx, F && f, A... a)
{
    using namespace leaf_detail;
    return capture_impl(is_result_tag<decltype(std::declval<F>()(std::forward<A>(std::declval<A>())...))>(), std::move(ctx), std::forward<F>(f), std::forward<A>(a)...);
}

template <class Future>
inline
decltype(std::declval<Future>().get())
future_get( Future & fut )
{
    using namespace leaf_detail;
    return future_get_impl(is_result_tag<decltype(std::declval<Future>().get())>(), fut);
}

////////////////////////////////////////

#ifndef BOOST_LEAF_NO_EXCEPTIONS

template <class T>
class result;

namespace leaf_detail
{
    inline error_id catch_exceptions_helper( std::exception const & ex, leaf_detail_mp11::mp_list<> )
    {
        return leaf::new_error(std::current_exception());
    }

    template <class Ex1, class... Ex>
    inline error_id catch_exceptions_helper( std::exception const & ex, leaf_detail_mp11::mp_list<Ex1,Ex...> )
    {
        if( Ex1 const * p = dynamic_cast<Ex1 const *>(&ex) )
            return catch_exceptions_helper(ex, leaf_detail_mp11::mp_list<Ex...>{ }).load(*p);
        else
            return catch_exceptions_helper(ex, leaf_detail_mp11::mp_list<Ex...>{ });
    }

    template <class T>
    struct deduce_exception_to_result_return_type_impl
    {
        using type = result<T>;
    };

    template <class T>
    struct deduce_exception_to_result_return_type_impl<result<T>>
    {
        using type = result<T>;
    };

    template <class T>
    using deduce_exception_to_result_return_type = typename deduce_exception_to_result_return_type_impl<T>::type;
}

template <class... Ex, class F>
inline
leaf_detail::deduce_exception_to_result_return_type<leaf_detail::fn_return_type<F>>
exception_to_result( F && f ) noexcept
{
    try
    {
        return std::forward<F>(f)();
    }
    catch( std::exception const & ex )
    {
        return leaf_detail::catch_exceptions_helper(ex, leaf_detail_mp11::mp_list<Ex...>());
    }
    catch(...)
    {
        return leaf::new_error(std::current_exception());
    }
}

#endif

} }

#if defined(_MSC_VER) && !defined(BOOST_LEAF_ENABLE_WARNINGS) ///
#pragma warning(pop) ///
#endif ///

#endif
