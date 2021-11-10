//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_DETAIL_DECORATOR_HPP
#define BOOST_BEAST_WEBSOCKET_DETAIL_DECORATOR_HPP

#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/core/exchange.hpp>
#include <boost/type_traits/make_void.hpp>
#include <algorithm>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

// VFALCO NOTE: When this is two traits, one for
//              request and one for response,
//              Visual Studio 2015 fails.

template<class T, class U, class = void>
struct can_invoke_with : std::false_type
{
};

template<class T, class U>
struct can_invoke_with<T, U, boost::void_t<decltype(
    std::declval<T&>()(std::declval<U&>()))>>
    : std::true_type
{
};

template<class T>
using is_decorator = std::integral_constant<bool,
    can_invoke_with<T, request_type>::value ||
    can_invoke_with<T, response_type>::value>;

class decorator
{
    friend class decorator_test;

    struct incomplete;

    struct exemplar
    {
        void (incomplete::*mf)();
        std::shared_ptr<incomplete> sp;
        void* param;
    };

    union storage
    {
        void* p_;
        void (*fn_)();
        typename std::aligned_storage<
            sizeof(exemplar),
            alignof(exemplar)>::type buf_;
    };

    struct vtable
    {
        void (*move)(
            storage& dst, storage& src) noexcept;
        void (*destroy)(storage& dst) noexcept;
        void (*invoke_req)(
            storage& dst, request_type& req);
        void (*invoke_res)(
            storage& dst, response_type& req);

        static void move_fn(
            storage&, storage&) noexcept
        {
        }

        static void destroy_fn(
            storage&) noexcept
        {
        }

        static void invoke_req_fn(
            storage&, request_type&)
        {
        }

        static void invoke_res_fn(
            storage&, response_type&)
        {
        }

        static vtable const* get_default()
        {
            static const vtable impl{
                &move_fn,
                &destroy_fn,
                &invoke_req_fn,
                &invoke_res_fn
            };
            return &impl;
        }
    };

    template<class F, bool Inline =
        (sizeof(F) <= sizeof(storage) &&
        alignof(F) <= alignof(storage) &&
        std::is_nothrow_move_constructible<F>::value)>
    struct vtable_impl;

    storage storage_;
    vtable const* vtable_ = vtable::get_default();

    // VFALCO NOTE: When this is two traits, one for
    //              request and one for response,
    //              Visual Studio 2015 fails.

    template<class T, class U, class = void>
    struct maybe_invoke
    {
        void
        operator()(T&, U&)
        {
        }
    };

    template<class T, class U>
    struct maybe_invoke<T, U, boost::void_t<decltype(
        std::declval<T&>()(std::declval<U&>()))>>
    {
        void
        operator()(T& t, U& u)
        {
            t(u);
        }
    };

public:
    decorator() = default;
    decorator(decorator const&) = delete;
    decorator& operator=(decorator const&) = delete;

    ~decorator()
    {
        vtable_->destroy(storage_);
    }

    decorator(decorator&& other) noexcept
        : vtable_(boost::exchange(
            other.vtable_, vtable::get_default()))
    {
        vtable_->move(
            storage_, other.storage_);
    }

    template<class F,
        class = typename std::enable_if<
        ! std::is_convertible<
            F, decorator>::value>::type>
    explicit
    decorator(F&& f)
      : vtable_(vtable_impl<
          typename std::decay<F>::type>::
        construct(storage_, std::forward<F>(f)))
    {
    }

    decorator&
    operator=(decorator&& other) noexcept
    {
        vtable_->destroy(storage_);
        vtable_ = boost::exchange(
            other.vtable_, vtable::get_default());
        vtable_->move(storage_, other.storage_);
        return *this;
    }

    void
    operator()(request_type& req)
    {
        vtable_->invoke_req(storage_, req);
    }

    void
    operator()(response_type& res)
    {
        vtable_->invoke_res(storage_, res);
    }
};

template<class F>
struct decorator::vtable_impl<F, true>
{
    template<class Arg>
    static
    vtable const*
    construct(storage& dst, Arg&& arg)
    {
        ::new (static_cast<void*>(&dst.buf_)) F(
            std::forward<Arg>(arg));
        return get();
    }

    static
    void
    move(storage& dst, storage& src) noexcept
    {
        auto& f = *beast::detail::launder_cast<F*>(&src.buf_);
        ::new (&dst.buf_) F(std::move(f));
    }

    static
    void
    destroy(storage& dst) noexcept
    {
        beast::detail::launder_cast<F*>(&dst.buf_)->~F();
    }

    static
    void
    invoke_req(storage& dst, request_type& req)
    {
        maybe_invoke<F, request_type>{}(
            *beast::detail::launder_cast<F*>(&dst.buf_), req);
    }

    static
    void
    invoke_res(storage& dst, response_type& res)
    {
        maybe_invoke<F, response_type>{}(
            *beast::detail::launder_cast<F*>(&dst.buf_), res);
    }

    static
    vtable
    const* get()
    {
        static constexpr vtable impl{
            &move,
            &destroy,
            &invoke_req,
            &invoke_res};
        return &impl;
    }
};

template<class F>
struct decorator::vtable_impl<F, false>
{
    template<class Arg>
    static
    vtable const*
    construct(storage& dst, Arg&& arg)
    {
        dst.p_ = new F(std::forward<Arg>(arg));
        return get();
    }

    static
    void
    move(storage& dst, storage& src) noexcept
    {
        dst.p_ = src.p_;
    }

    static
    void
    destroy(storage& dst) noexcept
    {
        delete static_cast<F*>(dst.p_);
    }

    static
    void
    invoke_req(
        storage& dst, request_type& req)
    {
        maybe_invoke<F, request_type>{}(
            *static_cast<F*>(dst.p_), req);
    }

    static
    void
    invoke_res(
        storage& dst, response_type& res)
    {
        maybe_invoke<F, response_type>{}(
            *static_cast<F*>(dst.p_), res);
    }

    static
    vtable const*
    get()
    {
        static constexpr vtable impl{&move,
            &destroy, &invoke_req, &invoke_res};
        return &impl;
    }
};

} // detail
} // websocket
} // beast
} // boost

#endif
