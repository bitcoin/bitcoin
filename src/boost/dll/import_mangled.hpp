// Copyright 2015-2018 Klemens D. Morgenstern
// Copyright 2019-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_DLL_IMPORT_MANGLED_HPP_
#define BOOST_DLL_IMPORT_MANGLED_HPP_

/// \file boost/dll/import_mangled.hpp
/// \warning Extremely experimental! Requires C++11! Will change in next version of Boost! boost/dll/import_mangled.hpp is not included in boost/dll.hpp
/// \brief Contains the boost::dll::experimental::import_mangled function for importing mangled symbols.

#include <boost/dll/config.hpp>
#if (__cplusplus < 201103L) && (!defined(_MSVC_LANG) || _MSVC_LANG < 201103L)
#  error This file requires C++11 at least!
#endif

#include <boost/make_shared.hpp>
#include <boost/move/move.hpp>
#include <boost/dll/smart_library.hpp>
#include <boost/dll/detail/import_mangled_helpers.hpp>
#include <boost/core/addressof.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/is_object.hpp>


#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

namespace boost { namespace dll { namespace experimental {

namespace detail
{

template <class ... Ts>
class mangled_library_function {
    // Copying of `boost::dll::shared_library` is very expensive, so we use a `shared_ptr` to make it faster.
    boost::shared_ptr<shared_library> lib_;
    function_tuple<Ts...>   f_;
public:
    constexpr mangled_library_function(const boost::shared_ptr<shared_library>& lib, Ts*... func_ptr) BOOST_NOEXCEPT
        : lib_(lib)
        , f_(func_ptr...)
    {}


    // Compilation error at this point means that imported function
    // was called with unmatching parameters.
    //
    // Example:
    // auto f = dll::import_mangled<void(int), void(double)>("function", "lib.so");
    // f("Hello");  // error: invalid conversion from 'const char*' to 'int'
    // f(1, 2);     // error: too many arguments to function
    // f();         // error: too few arguments to function
    template <class... Args>
    auto operator()(Args&&... args) const
        -> decltype( f_(static_cast<Args&&>(args)...) )
    {
        return f_(static_cast<Args&&>(args)...);
    }
};


template<class Class, class Sequence>
class mangled_library_mem_fn;

template <class Class, class ... Ts>
class mangled_library_mem_fn<Class, sequence<Ts...>> {
    // Copying of `boost::dll::shared_library` is very expensive, so we use a `shared_ptr` to make it faster.
    typedef mem_fn_tuple<Ts...> call_tuple_t;
    boost::shared_ptr<shared_library>   lib_;
    call_tuple_t f_;

public:
    constexpr mangled_library_mem_fn(const boost::shared_ptr<shared_library>& lib, typename Ts::mem_fn... func_ptr) BOOST_NOEXCEPT
        : lib_(lib)
        , f_(func_ptr...)
    {}

    template <class ClassIn, class... Args>
    auto operator()(ClassIn *cl, Args&&... args) const
        -> decltype( f_(cl, static_cast<Args&&>(args)...) )
    {
        return f_(cl, static_cast<Args&&>(args)...);
    }
};




// simple enough to be here
template<class Seq>  struct is_variable : boost::false_type {};
template<typename T> struct is_variable<sequence<T>> : boost::is_object<T> {};

template <class Sequence,
          bool isFunction = is_function_seq<Sequence>::value,
          bool isMemFn    = is_mem_fn_seq  <Sequence>::value,
          bool isVariable = is_variable    <Sequence>::value>
struct mangled_import_type;

template <class ...Args>
struct mangled_import_type<sequence<Args...>, true,false,false> //is function
{
    typedef boost::dll::experimental::detail::mangled_library_function<Args...> type;
    static type make(
           const boost::dll::experimental::smart_library& p,
           const std::string& name)
    {
        return type(
                boost::make_shared<shared_library>(p.shared_lib()),
                boost::addressof(p.get_function<Args>(name))...);
    }
};

template <class Class, class ...Args>
struct mangled_import_type<sequence<Class, Args...>, false, true, false> //is member-function
{
    typedef typename boost::dll::experimental::detail::make_mem_fn_seq<Class, Args...>::type actual_sequence;
    typedef typename boost::dll::experimental::detail::mangled_library_mem_fn<Class, actual_sequence> type;


    template<class ... ArgsIn>
    static type make_impl(
            const boost::dll::experimental::smart_library& p,
            const std::string & name,
            sequence<ArgsIn...> * )
    {
        return type(boost::make_shared<shared_library>(p.shared_lib()),
                    p.get_mem_fn<typename ArgsIn::class_type, typename ArgsIn::func_type>(name)...);
    }

    static type make(
           const boost::dll::experimental::smart_library& p,
           const std::string& name)
    {
        return make_impl(p, name, static_cast<actual_sequence*>(nullptr));
    }

};

template <class T>
struct mangled_import_type<sequence<T>, false, false, true> //is variable
{
    typedef boost::shared_ptr<T> type;

    static type make(
           const boost::dll::experimental::smart_library& p,
           const std::string& name)
    {
        return type(
                boost::make_shared<shared_library>(p.shared_lib()),
                boost::addressof(p.get_variable<T>(name)));
    }

};


} // namespace detail


#ifndef BOOST_DLL_DOXYGEN
#   define BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE inline typename \
    boost::dll::experimental::detail::mangled_import_type<boost::dll::experimental::detail::sequence<Args...>>::type
#endif

/*
 * Variants:
 * import_mangled<int>("Stuff");
 * import_mangled<thingy(xyz)>("Function");
 * import mangled<thingy, void(int)>("Function");
 */

/*!
* Returns callable object or boost::shared_ptr<T> that holds the symbol imported
* from the loaded library. Returned value refcounts usage
* of the loaded shared library, so that it won't get unload until all copies of return value
* are not destroyed.
*
* For importing symbols by \b alias names use \forcedlink{import_alias} method.
*
* \b Examples:
*
* \code
* boost::function<int(int)> f = import_mangled<int(int)>("test_lib.so", "integer_func_name");
*
* auto f_cpp11 = import_mangled<int(int)>("test_lib.so", "integer_func_name");
* \endcode
*
* \code
* boost::shared_ptr<int> i = import_mangled<int>("test_lib.so", "integer_name");
* \endcode
*
* Additionally you can also import overloaded symbols, including member-functions.
*
* \code
* auto fp = import_mangled<void(int), void(double)>("test_lib.so", "func");
* \endcode
*
* \code
* auto fp = import_mangled<my_class, void(int), void(double)>("test_lib.so", "func");
* \endcode
*
* If qualified member-functions are needed, this can be set by repeating the class name with const or volatile.
* All following signatures after the redifintion will use this, i.e. the latest.
*
* * * \code
* auto fp = import_mangled<my_class, void(int), void(double),
*                          const my_class, void(int), void(double)>("test_lib.so", "func");
* \endcode
*
* \b Template \b parameter \b T:    Type of the symbol that we are going to import. Must be explicitly specified.
*
* \param lib Path to shared library or shared library to load function from.
* \param name Null-terminated C or C++ mangled name of the function to import. Can handle std::string, char*, const char*.
* \param mode An mode that will be used on library load.
*
* \return callable object if T is a function type, or boost::shared_ptr<T> if T is an object type.
*
* \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
*       Overload that accepts path also throws std::bad_alloc in case of insufficient memory.
*/


template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(const boost::dll::fs::path& lib, const char* name,
    load_mode::type mode = load_mode::default_mode)
{
    typedef typename boost::dll::experimental::detail::mangled_import_type<
                     boost::dll::experimental::detail::sequence<Args...>> type;

    boost::dll::experimental::smart_library p(lib, mode);
    //the load
    return type::make(p, name);
}



//! \overload boost::dll::import(const boost::dll::fs::path& lib, const char* name, load_mode::type mode)
template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(const boost::dll::fs::path& lib, const std::string& name,
    load_mode::type mode = load_mode::default_mode)
{
    return import_mangled<Args...>(lib, name.c_str(), mode);
}

//! \overload boost::dll::import(const boost::dll::fs::path& lib, const char* name, load_mode::type mode)
template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(const smart_library& lib, const char* name) {
    typedef typename boost::dll::experimental::detail::mangled_import_type<detail::sequence<Args...>> type;

    return type::make(lib, name);
}

//! \overload boost::dll::import(const boost::dll::fs::path& lib, const char* name, load_mode::type mode)
template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(const smart_library& lib, const std::string& name) {
    return import_mangled<Args...>(lib, name.c_str());
}

//! \overload boost::dll::import(const boost::dll::fs::path& lib, const char* name, load_mode::type mode)
template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(BOOST_RV_REF(smart_library) lib, const char* name) {
    typedef typename boost::dll::experimental::detail::mangled_import_type<detail::sequence<Args...>> type;

    return type::make(lib, name);
}

//! \overload boost::dll::import(const boost::dll::fs::path& lib, const char* name, load_mode::type mode)
template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(BOOST_RV_REF(smart_library) lib, const std::string& name) {
    return import_mangled<Args...>(boost::move(lib), name.c_str());
}

//! \overload boost::dll::import(const boost::dll::fs::path& lib, const char* name, load_mode::type mode)
template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(const shared_library& lib, const char* name) {
    typedef typename boost::dll::experimental::detail::mangled_import_type<detail::sequence<Args...>> type;

    boost::shared_ptr<boost::dll::experimental::smart_library> p = boost::make_shared<boost::dll::experimental::smart_library>(lib);
    return type::make(p, name);
}

//! \overload boost::dll::import(const boost::dll::fs::path& lib, const char* name, load_mode::type mode)
template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(const shared_library& lib, const std::string& name) {
    return import_mangled<Args...>(lib, name.c_str());
}

//! \overload boost::dll::import(const boost::dll::fs::path& lib, const char* name, load_mode::type mode)
template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(BOOST_RV_REF(shared_library) lib, const char* name) {
    typedef typename boost::dll::experimental::detail::mangled_import_type<detail::sequence<Args...>> type;

    boost::dll::experimental::smart_library p(boost::move(lib));

    return type::make(p, name);
}

//! \overload boost::dll::import(const boost::dll::fs::path& lib, const char* name, load_mode::type mode)
template <class ...Args>
BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE import_mangled(BOOST_RV_REF(shared_library) lib, const std::string& name) {
    return import_mangled<Args...>(boost::move(lib), name.c_str());
}

#undef BOOST_DLL_MANGLED_IMPORT_RESULT_TYPE

}}}


#endif /* BOOST_DLL_IMPORT_MANGLED_HPP_ */
