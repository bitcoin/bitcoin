#ifndef BOOST_LEAF_ERROR_HPP_INCLUDED
#define BOOST_LEAF_ERROR_HPP_INCLUDED

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

#include <boost/leaf/detail/function_traits.hpp>
#include <boost/leaf/detail/print.hpp>
#include <system_error>
#include <type_traits>
#include <memory>
#include <string>

#if BOOST_LEAF_DIAGNOSTICS
#   include <sstream>
#   include <set>
#endif

#define BOOST_LEAF_TOKEN_PASTE(x, y) x ## y
#define BOOST_LEAF_TOKEN_PASTE2(x, y) BOOST_LEAF_TOKEN_PASTE(x, y)
#define BOOST_LEAF_TMP BOOST_LEAF_TOKEN_PASTE2(boost_leaf_tmp_, __LINE__)

#define BOOST_LEAF_ASSIGN(v,r)\
    auto && BOOST_LEAF_TMP = r;\
    static_assert(::boost::leaf::is_result_type<typename std::decay<decltype(BOOST_LEAF_TMP)>::type>::value, "BOOST_LEAF_ASSIGN and BOOST_LEAF_AUTO require a result object as the second argument (see is_result_type)");\
    if( !BOOST_LEAF_TMP )\
        return BOOST_LEAF_TMP.error();\
    v = std::forward<decltype(BOOST_LEAF_TMP)>(BOOST_LEAF_TMP).value()

#define BOOST_LEAF_AUTO(v, r)\
    BOOST_LEAF_ASSIGN(auto v, r)

#define BOOST_LEAF_CHECK(r)\
    auto && BOOST_LEAF_TMP = r;\
    static_assert(::boost::leaf::is_result_type<typename std::decay<decltype(BOOST_LEAF_TMP)>::type>::value, "BOOST_LEAF_CHECK requires a result object (see is_result_type)");\
    if( BOOST_LEAF_TMP )\
        ;\
    else\
        return BOOST_LEAF_TMP.error()

#define BOOST_LEAF_NEW_ERROR ::boost::leaf::leaf_detail::inject_loc{__FILE__,__LINE__,__FUNCTION__}+::boost::leaf::new_error

namespace boost { namespace leaf {

class error_id;

namespace leaf_detail
{
    struct inject_loc
    {
        char const * const file;
        int const line;
        char const * const fn;

        template <class T>
        friend T operator+( inject_loc loc, T && x ) noexcept
        {
            x.load_source_location_(loc.file, loc.line, loc.fn);
            return std::move(x);
        }
    };
}

} }

////////////////////////////////////////

#ifdef BOOST_LEAF_NO_EXCEPTIONS

namespace boost
{
BOOST_LEAF_NORETURN void throw_exception( std::exception const & ); // user defined
}

namespace boost { namespace leaf {

template <class T>
BOOST_LEAF_NORETURN void throw_exception( T const & e )
{
    ::boost::throw_exception(e);
}

} }

#else

namespace boost { namespace leaf {

template <class T>
BOOST_LEAF_NORETURN void throw_exception( T const & e )
{
    throw e;
}

} }

#endif

////////////////////////////////////////

#ifdef BOOST_LEAF_NO_THREADS

#   define BOOST_LEAF_THREAD_LOCAL
namespace boost { namespace leaf {
    namespace leaf_detail
    {
        using atomic_unsigned_int = unsigned int;
    }
} }

#else

#   include <atomic>
#   include <thread>
#   define BOOST_LEAF_THREAD_LOCAL BOOST_LEAF_SYMBOL_VISIBLE thread_local
namespace boost { namespace leaf {
    namespace leaf_detail
    {
        using atomic_unsigned_int = std::atomic<unsigned int>;
    }
} }

#endif

////////////////////////////////////////

namespace boost { namespace leaf {

#if BOOST_LEAF_DIAGNOSTICS

namespace leaf_detail
{
    class BOOST_LEAF_SYMBOL_VISIBLE e_unexpected_count
    {
    public:

        char const * (*first_type)();
        int count;

        BOOST_LEAF_CONSTEXPR explicit e_unexpected_count(char const * (*first_type)()) noexcept:
            first_type(first_type),
            count(1)
        {
        }

        template <class CharT, class Traits>
        void print( std::basic_ostream<CharT, Traits> & os ) const
        {
            BOOST_LEAF_ASSERT(first_type != 0);
            BOOST_LEAF_ASSERT(count>0);
            os << "Detected ";
            if( count==1 )
                os << "1 attempt to communicate an unexpected error object";
            else
                os << count << " attempts to communicate unexpected error objects, the first one";
            (os << " of type " << first_type() << '\n').flush();
        }
    };

    template <>
    struct diagnostic<e_unexpected_count, false, false>
    {
        static constexpr bool is_invisible = true;
        BOOST_LEAF_CONSTEXPR static void print(std::ostream &, e_unexpected_count const &) noexcept { }
    };

    class BOOST_LEAF_SYMBOL_VISIBLE e_unexpected_info
    {
        std::string s_;
        std::set<char const *(*)()> already_;

    public:

        e_unexpected_info() noexcept
        {
        }

        template <class E>
        void add(E && e)
        {
            if( !diagnostic<E>::is_invisible && already_.insert(&type<E>).second  )
            {
                std::stringstream s;
                diagnostic<E>::print(s,e);
                (s << '\n').flush();
                s_ += s.str();
            }
        }

        template <class CharT, class Traits>
        void print( std::basic_ostream<CharT, Traits> & os ) const
        {
            os << "Unhandled error objects:\n" << s_;
        }
    };

    template <>
    struct diagnostic<e_unexpected_info, false, false>
    {
        static constexpr bool is_invisible = true;
        BOOST_LEAF_CONSTEXPR static void print(std::ostream &, e_unexpected_info const &) noexcept { }
    };

    template <class=void>
    struct BOOST_LEAF_SYMBOL_VISIBLE tl_unexpected_enabled
    {
        static BOOST_LEAF_THREAD_LOCAL int counter;
    };

    template <class T>
    BOOST_LEAF_THREAD_LOCAL int tl_unexpected_enabled<T>::counter;
}

#endif

} }

////////////////////////////////////////

namespace boost { namespace leaf {

struct e_source_location
{
    char const * const file;
    int const line;
    char const * const function;

    template <class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits> & operator<<( std::basic_ostream<CharT, Traits> & os, e_source_location const & x )
    {
        return os << leaf::type<e_source_location>() << ": " << x.file << '(' << x.line << ") in function " << x.function;
    }
};

////////////////////////////////////////

namespace leaf_detail
{
    template <class E>
    class slot;

    template <class E>
    struct BOOST_LEAF_SYMBOL_VISIBLE tl_slot_ptr
    {
        static BOOST_LEAF_THREAD_LOCAL slot<E> * p;
    };

    template <class E>
    BOOST_LEAF_THREAD_LOCAL slot<E> * tl_slot_ptr<E>::p;

    template <class E>
    class slot:
        optional<E>
    {
        slot( slot const & ) = delete;
        slot & operator=( slot const & ) = delete;

        using impl = optional<E>;
        slot<E> * * top_;
        slot<E> * prev_;

    public:

        BOOST_LEAF_CONSTEXPR slot() noexcept:
            top_(0)
        {
        }

        BOOST_LEAF_CONSTEXPR slot( slot && x ) noexcept:
            optional<E>(std::move(x)),
            top_(0)
        {
            BOOST_LEAF_ASSERT(x.top_==0);
        }

        BOOST_LEAF_CONSTEXPR void activate() noexcept
        {
            BOOST_LEAF_ASSERT(top_==0 || *top_!=this);
            top_ = &tl_slot_ptr<E>::p;
            prev_ = *top_;
            *top_ = this;
        }

        BOOST_LEAF_CONSTEXPR void deactivate() noexcept
        {
            BOOST_LEAF_ASSERT(top_!=0 && *top_==this);
            *top_ = prev_;
        }

        BOOST_LEAF_CONSTEXPR void propagate() noexcept;

        template <class CharT, class Traits>
        void print( std::basic_ostream<CharT, Traits> & os, int key_to_print ) const
        {
            if( !diagnostic<E>::is_invisible )
                if( int k = this->key() )
                {
                    if( key_to_print )
                    {
                        if( key_to_print!=k )
                            return;
                    }
                    else
                        os << '[' << k << ']';
                    diagnostic<E>::print(os, value(k));
                    (os << '\n').flush();
                }
        }

        using impl::put;
        using impl::has_value;
        using impl::value;
    };

#if BOOST_LEAF_DIAGNOSTICS

    template <class E>
    BOOST_LEAF_CONSTEXPR inline void load_unexpected_count( int err_id ) noexcept
    {
        if( slot<e_unexpected_count> * sl = tl_slot_ptr<e_unexpected_count>::p )
            if( e_unexpected_count * unx = sl->has_value(err_id) )
                ++unx->count;
            else
                sl->put(err_id, e_unexpected_count(&type<E>));
    }

    template <class E>
    BOOST_LEAF_CONSTEXPR inline void load_unexpected_info( int err_id, E && e ) noexcept
    {
        if( slot<e_unexpected_info> * sl = tl_slot_ptr<e_unexpected_info>::p )
            if( e_unexpected_info * unx = sl->has_value(err_id) )
                unx->add(std::forward<E>(e));
            else
                sl->put(err_id, e_unexpected_info()).add(std::forward<E>(e));
    }

    template <class E>
    BOOST_LEAF_CONSTEXPR inline void load_unexpected( int err_id, E && e  ) noexcept
    {
        load_unexpected_count<E>(err_id);
        load_unexpected_info(err_id, std::forward<E>(e));
    }

#endif

    template <class E>
    BOOST_LEAF_CONSTEXPR inline void slot<E>::propagate() noexcept
    {
        BOOST_LEAF_ASSERT(top_!=0 && (*top_==prev_ || *top_==this));
        if( prev_ )
        {
            impl & that_ = *prev_;
            if( that_.empty() )
            {
                impl & this_ = *this;
                that_ = std::move(this_);
            }
        }
#if BOOST_LEAF_DIAGNOSTICS
        else
        {
            int c = tl_unexpected_enabled<>::counter;
            BOOST_LEAF_ASSERT(c>=0);
            if( c )
                if( int err_id = impl::key() )
                    load_unexpected(err_id, std::move(*this).value(err_id));
        }
#endif
    }

    template <class E>
    BOOST_LEAF_CONSTEXPR inline int load_slot( int err_id, E && e ) noexcept
    {
        static_assert(!std::is_pointer<E>::value, "Error objects of pointer types are not allowed");
        static_assert(!std::is_same<typename std::decay<E>::type, error_id>::value, "Error objects of type error_id are not allowed");
        using T = typename std::decay<E>::type;
        BOOST_LEAF_ASSERT((err_id&3)==1);
        if( slot<T> * p = tl_slot_ptr<T>::p )
            (void) p->put(err_id, std::forward<E>(e));
#if BOOST_LEAF_DIAGNOSTICS
        else
        {
            int c = tl_unexpected_enabled<>::counter;
            BOOST_LEAF_ASSERT(c>=0);
            if( c )
                load_unexpected(err_id, std::forward<E>(e));
        }
#endif
        return 0;
    }

    template <class F>
    BOOST_LEAF_CONSTEXPR inline int accumulate_slot( int err_id, F && f ) noexcept
    {
        static_assert(function_traits<F>::arity==1, "Lambdas passed to accumulate must take a single e-type argument by reference");
        using E = typename std::decay<fn_arg_type<F,0>>::type;
        static_assert(!std::is_pointer<E>::value, "Error objects of pointer types are not allowed");
        BOOST_LEAF_ASSERT((err_id&3)==1);
        if( auto sl = tl_slot_ptr<E>::p )
            if( auto v = sl->has_value(err_id) )
                (void) std::forward<F>(f)(*v);
            else
                (void) std::forward<F>(f)(sl->put(err_id,E()));
        return 0;
    }
}

////////////////////////////////////////

namespace leaf_detail
{
    template <class=void>
    struct BOOST_LEAF_SYMBOL_VISIBLE id_factory
    {
        static atomic_unsigned_int counter;
        static BOOST_LEAF_THREAD_LOCAL unsigned current_id;

        BOOST_LEAF_CONSTEXPR static unsigned generate_next_id() noexcept
        {
            auto id = (counter+=4);
            BOOST_LEAF_ASSERT((id&3)==1);
            return id;
        }
    };

    template <class T>
    atomic_unsigned_int id_factory<T>::counter(-3);

    template <class T>
    BOOST_LEAF_THREAD_LOCAL unsigned id_factory<T>::current_id(0);

    inline int current_id() noexcept
    {
        auto id = id_factory<>::current_id;
        BOOST_LEAF_ASSERT(id==0 || (id&3)==1);
        return id;
    }

    inline int new_id() noexcept
    {
        auto id = id_factory<>::generate_next_id();
        return id_factory<>::current_id = id;
    }
}

////////////////////////////////////////

namespace leaf_detail
{
    template <class T, int Arity = function_traits<T>::arity>
    struct load_item
    {
        static_assert(Arity==0 || Arity==1, "If a functions is passed to new_error or load, it must take zero or one argument");
    };

    template <class E>
    struct load_item<E, -1>
    {
        BOOST_LEAF_CONSTEXPR static int load( int err_id, E && e ) noexcept
        {
            return load_slot(err_id, std::forward<E>(e));
        }
    };

    template <class F>
    struct load_item<F, 0>
    {
        BOOST_LEAF_CONSTEXPR static int load( int err_id, F && f ) noexcept
        {
            return load_slot(err_id, std::forward<F>(f)());
        }
    };

    template <class F>
    struct load_item<F, 1>
    {
        BOOST_LEAF_CONSTEXPR static int load( int err_id, F && f ) noexcept
        {
            return accumulate_slot(err_id, std::forward<F>(f));
        }
    };
}

////////////////////////////////////////

namespace leaf_detail
{
    class leaf_category final: public std::error_category
    {
        bool equivalent( int,  std::error_condition const & ) const noexcept final override { return false; }
        bool equivalent( std::error_code const &, int ) const noexcept final override { return false; }
        char const * name() const noexcept final override { return "LEAF error"; }
        std::string message( int condition ) const final override { return name(); }
    public:
        ~leaf_category() noexcept final override { }
    };

    template <class=void>
    struct get_error_category
    {
        static leaf_category cat;
    };

    template <class T>
    leaf_category get_error_category<T>::cat;

    inline int import_error_code( std::error_code const & ec ) noexcept
    {
        if( int err_id = ec.value() )
        {
            std::error_category const & cat = get_error_category<>::cat;
            if( &ec.category()==&cat )
            {
                BOOST_LEAF_ASSERT((err_id&3)==1);
                return (err_id&~3)|1;
            }
            else
            {
                err_id = new_id();
                (void) load_slot(err_id, ec);
                return (err_id&~3)|1;
            }
        }
        else
            return 0;
    }
}

inline bool is_error_id( std::error_code const & ec ) noexcept
{
    bool res = (&ec.category() == &leaf_detail::get_error_category<>::cat);
    BOOST_LEAF_ASSERT(!res || !ec.value() || ((ec.value()&3)==1));
    return res;
}

////////////////////////////////////////

namespace leaf_detail
{
    BOOST_LEAF_CONSTEXPR error_id make_error_id(int) noexcept;
}

class BOOST_LEAF_SYMBOL_VISIBLE error_id
{
    friend error_id BOOST_LEAF_CONSTEXPR leaf_detail::make_error_id(int) noexcept;

    int value_;

    BOOST_LEAF_CONSTEXPR explicit error_id( int value ) noexcept:
        value_(value)
    {
        BOOST_LEAF_ASSERT(value_==0 || ((value_&3)==1));
    }

public:

    BOOST_LEAF_CONSTEXPR error_id() noexcept:
        value_(0)
    {
    }

    error_id( std::error_code const & ec ) noexcept:
        value_(leaf_detail::import_error_code(ec))
    {
        BOOST_LEAF_ASSERT(!value_ || ((value_&3)==1));
    }

    template <class Enum>
    error_id( Enum e, typename std::enable_if<std::is_error_code_enum<Enum>::value, Enum>::type * = 0 ) noexcept:
        value_(leaf_detail::import_error_code(e))
    {
    }

    BOOST_LEAF_CONSTEXPR error_id load() const noexcept
    {
        return *this;
    }

    template <class... Item>
    BOOST_LEAF_CONSTEXPR error_id load( Item && ... item ) const noexcept
    {
        if( int err_id = value() )
        {
            int const unused[ ] = { 42, leaf_detail::load_item<Item>::load(err_id, std::forward<Item>(item))... };
            (void) unused;
        }
        return *this;
    }

    std::error_code to_error_code() const noexcept
    {
        return std::error_code(value_, leaf_detail::get_error_category<>::cat);
    }

    BOOST_LEAF_CONSTEXPR int value() const noexcept
    {
        if( int v = value_ )
        {
            BOOST_LEAF_ASSERT((v&3)==1);
            return (v&~3)|1;
        }
        else
            return 0;
    }

    BOOST_LEAF_CONSTEXPR explicit operator bool() const noexcept
    {
        return value_ != 0;
    }

    BOOST_LEAF_CONSTEXPR friend bool operator==( error_id a, error_id b ) noexcept
    {
        return a.value_ == b.value_;
    }

    BOOST_LEAF_CONSTEXPR friend bool operator!=( error_id a, error_id b ) noexcept
    {
        return !(a == b);
    }

    BOOST_LEAF_CONSTEXPR friend bool operator<( error_id a, error_id b ) noexcept
    {
        return a.value_ < b.value_;
    }

    template <class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits> & operator<<( std::basic_ostream<CharT, Traits> & os, error_id x )
    {
        return os << x.value_;
    }

    BOOST_LEAF_CONSTEXPR void load_source_location_( char const * file, int line, char const * function ) const noexcept
    {
        BOOST_LEAF_ASSERT(file&&*file);
        BOOST_LEAF_ASSERT(line>0);
        BOOST_LEAF_ASSERT(function&&*function);
        BOOST_LEAF_ASSERT(value_);
        (void) load(e_source_location {file,line,function});
    }
};

namespace leaf_detail
{
    BOOST_LEAF_CONSTEXPR inline error_id make_error_id( int err_id ) noexcept
    {
        BOOST_LEAF_ASSERT(err_id==0 || (err_id&3)==1);
        return error_id((err_id&~3)|1);
    }
}

inline error_id new_error() noexcept
{
    return leaf_detail::make_error_id(leaf_detail::new_id());
}

template <class... Item>
inline error_id new_error( Item && ... item ) noexcept
{
    return leaf_detail::make_error_id(leaf_detail::new_id()).load(std::forward<Item>(item)...);
}

inline error_id current_error() noexcept
{
    return leaf_detail::make_error_id(leaf_detail::current_id());
}

////////////////////////////////////////////

class polymorphic_context
{
protected:

    polymorphic_context() noexcept = default;
    ~polymorphic_context() noexcept = default;

public:

    virtual error_id propagate_captured_errors() noexcept = 0;
    virtual void activate() noexcept = 0;
    virtual void deactivate() noexcept = 0;
    virtual void propagate() noexcept = 0;
    virtual bool is_active() const noexcept = 0;
    virtual void print( std::ostream & ) const = 0;
    error_id captured_id_;
};

using context_ptr = std::shared_ptr<polymorphic_context>;

////////////////////////////////////////////

template <class Ctx>
class context_activator
{
    context_activator( context_activator const & ) = delete;
    context_activator & operator=( context_activator const & ) = delete;

#if !defined(BOOST_LEAF_NO_EXCEPTIONS) && BOOST_LEAF_STD_UNCAUGHT_EXCEPTIONS
    int const uncaught_exceptions_;
#endif
    Ctx * ctx_;

public:

    explicit BOOST_LEAF_CONSTEXPR BOOST_LEAF_ALWAYS_INLINE context_activator(Ctx & ctx) noexcept:
#if !defined(BOOST_LEAF_NO_EXCEPTIONS) && BOOST_LEAF_STD_UNCAUGHT_EXCEPTIONS
        uncaught_exceptions_(std::uncaught_exceptions()),
#endif
        ctx_(ctx.is_active() ? 0 : &ctx)
    {
        if( ctx_ )
            ctx_->activate();
    }

    BOOST_LEAF_CONSTEXPR BOOST_LEAF_ALWAYS_INLINE context_activator( context_activator && x ) noexcept:
#if !defined(BOOST_LEAF_NO_EXCEPTIONS) && BOOST_LEAF_STD_UNCAUGHT_EXCEPTIONS
        uncaught_exceptions_(x.uncaught_exceptions_),
#endif
        ctx_(x.ctx_)
    {
        x.ctx_ = 0;
    }

    BOOST_LEAF_ALWAYS_INLINE ~context_activator() noexcept
    {
        if( !ctx_ )
            return;
        if( ctx_->is_active() )
            ctx_->deactivate();
#ifndef BOOST_LEAF_NO_EXCEPTIONS
#   if BOOST_LEAF_STD_UNCAUGHT_EXCEPTIONS
        if( std::uncaught_exceptions() > uncaught_exceptions_ )
#   else
        if( std::uncaught_exception() )
#   endif
            ctx_->propagate();
#endif
    }
};

template <class Ctx>
BOOST_LEAF_CONSTEXPR BOOST_LEAF_ALWAYS_INLINE context_activator<Ctx> activate_context(Ctx & ctx) noexcept
{
    return context_activator<Ctx>(ctx);
}

////////////////////////////////////////////

template <class R>
struct is_result_type: std::false_type
{
};

template <class R>
struct is_result_type<R const>: is_result_type<R>
{
};

} }

#undef BOOST_LEAF_THREAD_LOCAL

#if defined(_MSC_VER) && !defined(BOOST_LEAF_ENABLE_WARNINGS) ///
#pragma warning(pop) ///
#endif ///

#endif
