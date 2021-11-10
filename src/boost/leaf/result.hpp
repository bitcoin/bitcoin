#ifndef BOOST_LEAF_RESULT_HPP_INCLUDED
#define BOOST_LEAF_RESULT_HPP_INCLUDED

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
#include <climits>

namespace boost { namespace leaf {

class bad_result:
    public std::exception,
    public error_id
{
    char const * what() const noexcept final override
    {
        return "boost::leaf::bad_result";
    }

public:

    explicit bad_result( error_id id ) noexcept:
        error_id(id)
    {
        BOOST_LEAF_ASSERT(value());
    }
};

////////////////////////////////////////

namespace leaf_detail
{
    template <class T>
    struct stored
    {
        using type = T;
        using value_type = T;
        using value_type_const = T const;
        using value_cref = T const &;
        using value_ref = T &;
        using value_rv_cref = T const &&;
        using value_rv_ref = T &&;
    };

    template <class T>
    struct stored<T &>
    {
        using type = std::reference_wrapper<T>;
        using value_type_const = T;
        using value_type = T;
        using value_ref = T &;
        using value_cref = T &;
        using value_rv_ref = T &;
        using value_rv_cref = T &;
    };

    class result_discriminant
    {
        unsigned state_;

    public:

        enum kind_t
        {
            no_error = 0,
            err_id = 1,
            ctx_ptr = 2,
            val = 3
        };

        explicit result_discriminant( error_id id ) noexcept:
            state_(id.value())
        {
            BOOST_LEAF_ASSERT(state_==0 || (state_&3)==1);
        }

        struct kind_val { };
        explicit result_discriminant( kind_val ) noexcept:
            state_(val)
        {
        }

        struct kind_ctx_ptr { };
        explicit result_discriminant( kind_ctx_ptr ) noexcept:
            state_(ctx_ptr)
        {
        }

        kind_t kind() const noexcept
        {
            return kind_t(state_&3);
        }

        error_id get_error_id() const noexcept
        {
            BOOST_LEAF_ASSERT(kind()==no_error || kind()==err_id);
            return make_error_id(state_);
        }
    };
}

////////////////////////////////////////

template <class T>
class result
{
    template <class U>
    friend class result;

    using result_discriminant = leaf_detail::result_discriminant;

    struct error_result
    {
        error_result( error_result && ) = default;
        error_result( error_result const & ) = delete;
        error_result & operator=( error_result const & ) = delete;

        result & r_;

        error_result( result & r ) noexcept:
            r_(r)
        {
        }

        template <class U>
        operator result<U>() noexcept
        {
            switch(r_.what_.kind())
            {
            case result_discriminant::val:
                return result<U>(error_id());
            case result_discriminant::ctx_ptr:
                return result<U>(std::move(r_.ctx_));
            default:
                return result<U>(std::move(r_.what_));
            }
        }

        operator error_id() noexcept
        {
            switch(r_.what_.kind())
            {
            case result_discriminant::val:
                return error_id();
            case result_discriminant::ctx_ptr:
            {
                error_id captured_id = r_.ctx_->propagate_captured_errors();
                leaf_detail::id_factory<>::current_id = captured_id.value();
                return captured_id;
            }
            default:
                return r_.what_.get_error_id();
            }
        }
    };

    using stored_type = typename leaf_detail::stored<T>::type;
    using value_type = typename leaf_detail::stored<T>::value_type;
    using value_type_const = typename leaf_detail::stored<T>::value_type_const;
    using value_ref = typename leaf_detail::stored<T>::value_ref;
    using value_cref = typename leaf_detail::stored<T>::value_cref;
    using value_rv_ref = typename leaf_detail::stored<T>::value_rv_ref;
    using value_rv_cref = typename leaf_detail::stored<T>::value_rv_cref;

    union
    {
        stored_type stored_;
        context_ptr ctx_;
    };

    result_discriminant what_;

    void destroy() const noexcept
    {
        switch(this->what_.kind())
        {
        case result_discriminant::val:
            stored_.~stored_type();
            break;
        case result_discriminant::ctx_ptr:
            BOOST_LEAF_ASSERT(!ctx_ || ctx_->captured_id_);
            ctx_.~context_ptr();
        default:
            break;
        }
    }

    template <class U>
    result_discriminant move_from( result<U> && x ) noexcept
    {
        auto x_what = x.what_;
        switch(x_what.kind())
        {
        case result_discriminant::val:
            (void) new(&stored_) stored_type(std::move(x.stored_));
            break;
        case result_discriminant::ctx_ptr:
            BOOST_LEAF_ASSERT(!x.ctx_ || x.ctx_->captured_id_);
            (void) new(&ctx_) context_ptr(std::move(x.ctx_));
        default:
            break;
        }
        return x_what;
    }

    result( result_discriminant && what ) noexcept:
        what_(std::move(what))
    {
        BOOST_LEAF_ASSERT(what_.kind()==result_discriminant::err_id || what_.kind()==result_discriminant::no_error);
    }

    error_id get_error_id() const noexcept
    {
        BOOST_LEAF_ASSERT(what_.kind()!=result_discriminant::val);
        return what_.kind()==result_discriminant::ctx_ptr ? ctx_->captured_id_ : what_.get_error_id();
    }

protected:

    void enforce_value_state() const
    {
        if( what_.kind() != result_discriminant::val )
            ::boost::leaf::throw_exception(bad_result(get_error_id()));
    }

public:

    result( result && x ) noexcept:
        what_(move_from(std::move(x)))
    {
    }

    template <class U, class = typename std::enable_if<std::is_convertible<U, T>::value>::type>
    result( result<U> && x ) noexcept:
        what_(move_from(std::move(x)))
    {
    }

    result():
        stored_(stored_type()),
        what_(result_discriminant::kind_val{})
    {
    }

    result( value_type && v ) noexcept:
        stored_(std::forward<value_type>(v)),
        what_(result_discriminant::kind_val{})
    {
    }

    result( value_type const & v ):
        stored_(v),
        what_(result_discriminant::kind_val{})
    {
    }

    result( error_id err ) noexcept:
        what_(err)
    {
    }

#if defined(BOOST_STRICT_CONFIG) || !defined(__clang__)

    // This should be the default implementation, but std::is_convertible
    // breaks under COMPILER=/usr/bin/clang++ CXXSTD=11 clang 3.3.
    // On the other hand, the workaround exposes a rather severe bug in
    //__GNUC__ under 11: https://github.com/boostorg/leaf/issues/25.

    // SFINAE: T can be initialized with a U, e.g. result<std::string>("literal").
    template <class U, class = typename std::enable_if<std::is_convertible<U, T>::value>::type>
    result( U && u ):
        stored_(std::forward<U>(u)),
        what_(result_discriminant::kind_val{})
    {
    }

#else

private:
    static int init_T_with_U( T && );
public:

    // SFINAE: T can be initialized with a U, e.g. result<std::string>("literal").
    template <class U>
    result( U && u, decltype(init_T_with_U(std::forward<U>(u))) * = 0 ):
        stored_(std::forward<U>(u)),
        what_(result_discriminant::kind_val{})
    {
    }

#endif

    result( std::error_code const & ec ) noexcept:
        what_(error_id(ec))
    {
    }

    template <class Enum>
    result( Enum e, typename std::enable_if<std::is_error_code_enum<Enum>::value, int>::type * = 0 ) noexcept:
        what_(error_id(e))
    {
    }

    result( context_ptr && ctx ) noexcept:
        ctx_(std::move(ctx)),
        what_(result_discriminant::kind_ctx_ptr{})
    {
    }

    ~result() noexcept
    {
        destroy();
    }

    result & operator=( result && x ) noexcept
    {
        destroy();
        what_ = move_from(std::move(x));
        return *this;
    }

    template <class U>
    result & operator=( result<U> && x ) noexcept
    {
        destroy();
        what_ = move_from(std::move(x));
        return *this;
    }

    explicit operator bool() const noexcept
    {
        return what_.kind() == result_discriminant::val;
    }

    value_cref value() const &
    {
        enforce_value_state();
        return stored_;
    }

    value_ref value() &
    {
        enforce_value_state();
        return stored_;
    }

    value_rv_cref value() const &&
    {
        enforce_value_state();
        return std::move(stored_);
    }

    value_rv_ref value() &&
    {
        enforce_value_state();
        return std::move(stored_);
    }

    value_cref operator*() const &
    {
        return value();
    }

    value_ref operator*() &
    {
        return value();
    }

    value_rv_cref operator*() const &&
    {
        return value();
    }

    value_rv_ref operator*() &&
    {
        return value();
    }

    value_type_const * operator->() const
    {
        return &value();
    }

    value_type * operator->()
    {
        return &value();
    }

    error_result error() noexcept
    {
        return error_result{*this};
    }

    template <class... Item>
    error_id load( Item && ... item ) noexcept
    {
        return error_id(error()).load(std::forward<Item>(item)...);
    }
};

////////////////////////////////////////

namespace leaf_detail
{
    struct void_ { };
}

template <>
class result<void>:
    result<leaf_detail::void_>
{
    using result_discriminant = leaf_detail::result_discriminant;
    using void_ = leaf_detail::void_;
    using base = result<void_>;

    template <class U>
    friend class result;

    result( result_discriminant && what ) noexcept:
        base(std::move(what))
    {
    }

public:

    using value_type = void;

    result( result && x ) noexcept:
        base(std::move(x))
    {
    }

    result() noexcept
    {
    }

    result( error_id err ) noexcept:
        base(err)
    {
    }

    result( std::error_code const & ec ) noexcept:
        base(ec)
    {
    }

    template <class Enum>
    result( Enum e, typename std::enable_if<std::is_error_code_enum<Enum>::value, Enum>::type * = 0 ) noexcept:
        base(e)
    {
    }

    result( context_ptr && ctx ) noexcept:
        base(std::move(ctx))
    {
    }

    ~result() noexcept
    {
    }

    void value() const
    {
        base::enforce_value_state();
    }

    using base::operator=;
    using base::operator bool;
    using base::get_error_id;
    using base::error;
    using base::load;
};

////////////////////////////////////////

template <class R>
struct is_result_type;

template <class T>
struct is_result_type<result<T>>: std::true_type
{
};

} }

#if defined(_MSC_VER) && !defined(BOOST_LEAF_ENABLE_WARNINGS) ///
#pragma warning(pop) ///
#endif ///

#endif
