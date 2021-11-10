//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_VALUE_REF_HPP
#define BOOST_JSON_VALUE_REF_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/string.hpp>
#include <initializer_list>
#include <type_traits>
#include <utility>

BOOST_JSON_NS_BEGIN

#ifndef BOOST_JSON_DOCS
class value;
class object;
class array;
class string;
#endif

//----------------------------------------------------------

/** The type used in initializer lists.

    This type is used in initializer lists for
    lazy construction of and assignment to the
    container types @ref value, @ref array,
    and @ref object. The two types of initializer
    lists used are:

    @li `std::initializer_list< value_ref >` for
    constructing or assigning a @ref value or
    @ref array, and

    @li `std::initializer_list< std::pair< string_view, value_ref > >`
    for constructing or assigning an @ref object.

    A `value_ref` uses reference semantics. Creation
    of the actual container from the initializer
    list is lazily deferred until the list is used.
    This means that the @ref memory_resource used to
    construct a container can be specified after the
    point where the initializer list is specified.

    @par Example

    This example demonstrates how a user-defined type
    containing a JSON value can be constructed from
    an initializer list:

    @code

    class my_type
    {
        value jv_;

    public:
        my_type( std::initializer_list< value_ref > init )
            : jv_(init)
        {
        }
    };

    @endcode

    @note Never declare a variable of type
    `std::initializer_list` except in function
    parameter lists, otherwise the behavior may
    be undefined.

    @see
        @ref value,
        @ref array,
        @ref object
*/
class value_ref
{
    friend class value;
    friend class object;
    friend class array;

    friend class value_ref_test;

    enum class what
    {
        str,
        ini,
        func,
        cfunc,
        strfunc,
    };

    using init_list =
        std::initializer_list<value_ref>;

    struct func_type
    {
        value(*f)(void*, storage_ptr);
        void* p;
    };

    struct cfunc_type
    {
        value(*f)(void const*, storage_ptr);
        void const* p;
    };

    union arg_type
    {
        string_view         str_;
        init_list           init_list_;

        signed char         schar_;
        short               short_;
        int                 int_;
        long                long_;
        long long           long_long_;
        unsigned char       uchar_;
        unsigned short      ushort_;
        unsigned int        uint_;
        unsigned long       ulong_;
        unsigned long long  ulong_long_;
        float               float_;
        double              double_;
        bool                bool_;
        std::nullptr_t      nullptr_;

        arg_type() {}
        explicit arg_type(string_view t) noexcept : str_(t) {}
        explicit arg_type(init_list t) noexcept : init_list_(t) {}
        explicit arg_type(signed char t) noexcept : schar_(t) {}
        explicit arg_type(short t) noexcept : short_(t) {}
        explicit arg_type(int t) noexcept : int_(t) {}
        explicit arg_type(long t) noexcept : long_(t) {}
        explicit arg_type(long long t) noexcept : long_long_(t) {}
        explicit arg_type(unsigned char t) noexcept : uchar_(t) {}
        explicit arg_type(unsigned short t) noexcept : ushort_(t) {}
        explicit arg_type(unsigned int t) noexcept : uint_(t) {}
        explicit arg_type(unsigned long t) noexcept : ulong_(t) {}
        explicit arg_type(unsigned long long t) noexcept : ulong_long_(t) {}
        explicit arg_type(float t) noexcept : float_(t) {}
        explicit arg_type(double t) noexcept : double_(t) {}
        explicit arg_type(bool t) noexcept : bool_(t) {}
        explicit arg_type(std::nullptr_t) noexcept : nullptr_() {}
    };

    template<class T>
    using is_builtin =
        std::integral_constant<bool,
            std::is_same<T, signed char>::value ||
            std::is_same<T, short>::value ||
            std::is_same<T, int>::value ||
            std::is_same<T, long>::value ||
            std::is_same<T, long long>::value ||
            std::is_same<T, unsigned char>::value ||
            std::is_same<T, unsigned short>::value ||
            std::is_same<T, unsigned int>::value ||
            std::is_same<T, unsigned long>::value ||
            std::is_same<T, unsigned long long>::value ||
            std::is_same<T, float>::value ||
            std::is_same<T, double>::value ||
            std::is_same<T, std::nullptr_t>::value>;

    arg_type arg_;
#ifndef BOOST_JSON_DOCS
    // VFALCO doc toolchain erroneously
    // displays private, anonymous unions as public
    union
    {
        func_type f_;
        cfunc_type cf_;
    };
#endif
    what what_;

public:
    /// Constructor
    value_ref(
        value_ref const&) = default;

    /// Constructor
#ifdef BOOST_JSON_DOCS
    value_ref(string_view s) noexcept;
#else
    template<
        class T
        ,class = typename
            std::enable_if<
                std::is_constructible<
                    string_view, T>::value>::type
    >
    value_ref(
        T const& t) noexcept
        : arg_(string_view(t))
        , what_(what::str)
    {

    }
#endif

    /// Constructor
    template<class T>
    value_ref(
        T const& t
#ifndef BOOST_JSON_DOCS
        ,typename std::enable_if<
            ! std::is_constructible<
                string_view, T>::value &&
            ! std::is_same<bool, T>::value
                >::type* = 0
#endif
        ) noexcept
        : cf_{&from_const<T>, &t}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    template<class T>
    value_ref(
        T&& t
#ifndef BOOST_JSON_DOCS
        ,typename std::enable_if<
            (! std::is_constructible<
                string_view, T>::value ||
            std::is_same<string, T>::value) &&
            ! std::is_same<bool,
                detail::remove_cvref<T>>::value &&
            std::is_same<T, detail::remove_cvref<T>>
                ::value>::type* = 0
#endif
        ) noexcept
        : f_{&from_rvalue<
            detail::remove_cvref<T>>, &t}
        , what_(std::is_same<string, T>::value ?
                what::strfunc : what::func)
    {
    }

    /// Constructor
#ifdef BOOST_JSON_DOCS
    value_ref(bool b) noexcept;
#else
    template<
        class Bool
        ,class = typename std::enable_if<
            std::is_same<Bool, bool>::value>::type
    >
    value_ref(
        Bool b) noexcept
        : arg_(b)
        , cf_{&from_builtin<bool>, &arg_.bool_}
        , what_(what::cfunc)
    {
    }
#endif

    /// Constructor
    value_ref(
        std::initializer_list<
            value_ref> t) noexcept
        : arg_(t)
        , what_(what::ini)
    {
    }

    /// Constructor
    value_ref(signed char t) noexcept
        : arg_(t)
        , cf_{&from_builtin<signed char>, &arg_.schar_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(short t) noexcept
        : arg_(t)
        , cf_{&from_builtin<short>, &arg_.short_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(int t) noexcept
        : arg_(t)
        , cf_{&from_builtin<int>, &arg_.int_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(long t) noexcept
        : arg_(t)
        , cf_{&from_builtin<
            long>, &arg_.long_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(long long t) noexcept
        : arg_(t)
        , cf_{&from_builtin<
            long long>, &arg_.long_long_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(unsigned char t) noexcept
        : arg_(t)
        , cf_{&from_builtin<
            unsigned char>, &arg_.uchar_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(unsigned short t) noexcept
        : arg_(t)
        , cf_{&from_builtin<
            unsigned short>, &arg_.ushort_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(unsigned int t) noexcept
        : arg_(t)
        , cf_{&from_builtin<
            unsigned int>, &arg_.uint_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(unsigned long t) noexcept
        : arg_(t)
        , cf_{&from_builtin<
            unsigned long>, &arg_.ulong_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(unsigned long long t) noexcept
        : arg_(t)
        , cf_{&from_builtin<
            unsigned long long>, &arg_.ulong_long_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(float t) noexcept
        : arg_(t)
        , cf_{&from_builtin<
            float>, &arg_.float_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(double t) noexcept
        : arg_(t)
        , cf_{&from_builtin<
            double>, &arg_.double_}
        , what_(what::cfunc)
    {
    }

    /// Constructor
    value_ref(std::nullptr_t) noexcept
        : arg_(nullptr)
        , cf_{&from_builtin<
            std::nullptr_t>, &arg_.nullptr_}
        , what_(what::cfunc)
    {
    }

#ifndef BOOST_JSON_DOCS
// Not public
//private:
    // VFALCO Why is this needed?
    /** Operator conversion to @ref value

        This allows creation of a @ref value from
        an initializer list element.
    */
    BOOST_JSON_DECL
    operator value() const;
#endif

private:
    template<class T>
    static
    value
    from_builtin(
        void const* p,
        storage_ptr sp) noexcept;

    template<class T>
    static
    value
    from_const(
        void const* p,
        storage_ptr sp);

    template<class T>
    static
    value
    from_rvalue(
        void* p,
        storage_ptr sp);

    static
    BOOST_JSON_DECL
    value
    from_init_list(
        void const* p,
        storage_ptr sp);

    inline
    bool
    is_key_value_pair() const noexcept;

    static
    inline
    bool
    maybe_object(
        std::initializer_list<
            value_ref> init) noexcept;

    inline
    string_view
    get_string() const noexcept;

    BOOST_JSON_DECL
    value
    make_value(
        storage_ptr sp) const;

    BOOST_JSON_DECL
    static
    value
    make_value(
        std::initializer_list<
            value_ref> init,
        storage_ptr sp);

    BOOST_JSON_DECL
    static
    object
    make_object(
        std::initializer_list<value_ref> init,
        storage_ptr sp);

    BOOST_JSON_DECL
    static
    array
    make_array(
        std::initializer_list<
            value_ref> init,
        storage_ptr sp);

    BOOST_JSON_DECL
    static
    void
    write_array(
        value* dest,
        std::initializer_list<
            value_ref> init,
        storage_ptr const& sp);
};

BOOST_JSON_NS_END

// Must be included here for this file to stand alone
#include <boost/json/value.hpp>

// includes are at the bottom of <boost/json/value.hpp>
//#include <boost/json/impl/value.hpp>
//#include <boost/json/impl/value.ipp>

#endif
