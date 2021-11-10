#ifndef BOOST_LEAF_EXCEPTION_HPP_INCLUDED
#define BOOST_LEAF_EXCEPTION_HPP_INCLUDED

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
#include <exception>

#define BOOST_LEAF_EXCEPTION ::boost::leaf::leaf_detail::inject_loc{__FILE__,__LINE__,__FUNCTION__}+::boost::leaf::exception
#define BOOST_LEAF_THROW_EXCEPTION ::boost::leaf::leaf_detail::throw_with_loc{__FILE__,__LINE__,__FUNCTION__}+::boost::leaf::exception

////////////////////////////////////////

namespace boost { namespace leaf {

namespace leaf_detail
{
    struct throw_with_loc
    {
        char const * const file;
        int const line;
        char const * const fn;

        template <class Ex>
        [[noreturn]] friend void operator+( throw_with_loc loc, Ex const & ex )
        {
            ex.load_source_location_(loc.file, loc.line, loc.fn);
            ::boost::leaf::throw_exception(ex);
        }
    };
}

} }

////////////////////////////////////////

namespace boost { namespace leaf {

namespace leaf_detail
{
    inline void enforce_std_exception( std::exception const & ) noexcept { }

    class BOOST_LEAF_SYMBOL_VISIBLE exception_base
    {
        std::shared_ptr<void const> auto_id_bump_;
    public:

        virtual error_id get_error_id() const noexcept = 0;

    protected:

        exception_base():
            auto_id_bump_(0, [](void const *) { (void) new_id(); })
        {
        }

        ~exception_base() noexcept { }
    };

    template <class Ex>
    class BOOST_LEAF_SYMBOL_VISIBLE exception:
        public Ex,
        public exception_base,
        public error_id
    {
        error_id get_error_id() const noexcept final override
        {
            return *this;
        }

    public:

        exception( exception const & ) = default;
        exception( exception && ) = default;

        BOOST_LEAF_CONSTEXPR exception( error_id id, Ex && ex ) noexcept:
            Ex(std::move(ex)),
            error_id(id)
        {
            enforce_std_exception(*this);
        }

        explicit BOOST_LEAF_CONSTEXPR exception( error_id id ) noexcept:
            error_id(id)
        {
            enforce_std_exception(*this);
        }
    };

    template <class... T>
    struct at_least_one_derives_from_std_exception;

    template <>
    struct at_least_one_derives_from_std_exception<>: std::false_type { };

    template <class T, class... Rest>
    struct at_least_one_derives_from_std_exception<T, Rest...>
    {
        constexpr static const bool value = std::is_base_of<std::exception,T>::value || at_least_one_derives_from_std_exception<Rest...>::value;
    };
}

template <class Ex, class... E>
inline
typename std::enable_if<std::is_base_of<std::exception,Ex>::value, leaf_detail::exception<Ex>>::type
exception( error_id err, Ex && ex, E && ... e ) noexcept
{
    static_assert(!leaf_detail::at_least_one_derives_from_std_exception<E...>::value, "Error objects passed to leaf::exception may not derive from std::exception");
    return leaf_detail::exception<Ex>( err.load(std::forward<E>(e)...), std::forward<Ex>(ex) );
}

template <class E1, class... E>
inline
typename std::enable_if<!std::is_base_of<std::exception,E1>::value, leaf_detail::exception<std::exception>>::type
exception( error_id err, E1 && car, E && ... cdr ) noexcept
{
    static_assert(!leaf_detail::at_least_one_derives_from_std_exception<E...>::value, "Error objects passed to leaf::exception may not derive from std::exception");
    return leaf_detail::exception<std::exception>( err.load(std::forward<E1>(car), std::forward<E>(cdr)...) );
}

inline leaf_detail::exception<std::exception> exception( error_id err ) noexcept
{
    return leaf_detail::exception<std::exception>(err);
}

template <class Ex, class... E>
inline
typename std::enable_if<std::is_base_of<std::exception,Ex>::value, leaf_detail::exception<Ex>>::type
exception( Ex && ex, E && ... e ) noexcept
{
    static_assert(!leaf_detail::at_least_one_derives_from_std_exception<E...>::value, "Error objects passed to leaf::exception may not derive from std::exception");
    return leaf_detail::exception<Ex>( new_error().load(std::forward<E>(e)...), std::forward<Ex>(ex) );
}

template <class E1, class... E>
inline
typename std::enable_if<!std::is_base_of<std::exception,E1>::value, leaf_detail::exception<std::exception>>::type
exception( E1 && car, E && ... cdr ) noexcept
{
    static_assert(!leaf_detail::at_least_one_derives_from_std_exception<E...>::value, "Error objects passed to leaf::exception may not derive from std::exception");
    return leaf_detail::exception<std::exception>( new_error().load(std::forward<E1>(car), std::forward<E>(cdr)...) );
}

inline leaf_detail::exception<std::exception> exception() noexcept
{
    return leaf_detail::exception<std::exception>(leaf::new_error());
}

} }

#if defined(_MSC_VER) && !defined(BOOST_LEAF_ENABLE_WARNINGS) ///
#pragma warning(pop) ///
#endif ///

#endif
