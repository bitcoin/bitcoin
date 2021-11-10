//  Copyright 2016 Klemens Morgenstern
//  Copyright 2019-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_SMART_LIBRARY_HPP_
#define BOOST_DLL_SMART_LIBRARY_HPP_

/// \file boost/dll/smart_library.hpp
/// \warning Extremely experimental! Requires C++11! Will change in next version of Boost! boost/dll/smart_library.hpp is not included in boost/dll.hpp
/// \brief Contains the boost::dll::experimental::smart_library class for loading mangled symbols.

#include <boost/dll/config.hpp>
#if defined(_MSC_VER) // MSVC, Clang-cl, and ICC on Windows
#   include <boost/dll/detail/demangling/msvc.hpp>
#else
#   include <boost/dll/detail/demangling/itanium.hpp>
#endif

#if (__cplusplus < 201103L) && (!defined(_MSVC_LANG) || _MSVC_LANG < 201103L)
#  error This file requires C++11 at least!
#endif

#include <boost/dll/shared_library.hpp>
#include <boost/dll/detail/get_mem_fn_type.hpp>
#include <boost/dll/detail/ctor_dtor.hpp>
#include <boost/dll/detail/type_info.hpp>
#include <boost/type_traits/is_object.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/is_function.hpp>



namespace boost {
namespace dll {
namespace experimental {

using boost::dll::detail::constructor;
using boost::dll::detail::destructor;

/*!
* \brief This class is an extension of \ref shared_library, which allows to load C++ symbols.
*
* This class allows type safe loading of overloaded functions, member-functions, constructors and variables.
* It also allows to overwrite classes so they can be loaded, while being declared with different names.
*
* \warning Is still very experimental.
*
* Currently known limitations:
*
* Member functions must be defined outside of the class to be exported. That is:
* \code
* //not exported:
* struct BOOST_SYMBOL_EXPORT my_class { void func() {}};
* //exported
* struct BOOST_SYMBOL_EXPORT my_class { void func();};
* void my_class::func() {};
* \endcode
*
* With the current analysis, the first version does get exported in MSVC.
* MinGW also does export it, BOOST_SYMBOL_EXPORT is written before it. To allow this on windows one can use
* BOOST_DLL_MEMBER_EXPORT for this, so that MinGW and MSVC can provide those functions. This does however not work with gcc on linux.
*
* Direct initialization of members.
* On linux the following member variable i will not be initialized when using the allocating constructor:
* \code
* struct BOOST_SYMBOL_EXPORT my_class { int i; my_class() : i(42) {} };
* \endcode
*
* This does however not happen when the value is set inside the constructor function.
*/
class smart_library {
    shared_library _lib;
    detail::mangled_storage_impl _storage;

public:
    /*!
     * Get the underlying shared_library
     */
    const shared_library &shared_lib() const {return _lib;}

    using mangled_storage = detail::mangled_storage_impl;
    /*!
    * Access to the mangled storage, which is created on construction.
    *
    * \throw Nothing.
    */
    const mangled_storage &symbol_storage() const {return _storage;}

    ///Overload, for current development.
    mangled_storage &symbol_storage() {return _storage;}

    //! \copydoc shared_library::shared_library()
    smart_library() BOOST_NOEXCEPT {};

    //! \copydoc shared_library::shared_library(const boost::dll::fs::path& lib_path, load_mode::type mode = load_mode::default_mode)
    smart_library(const boost::dll::fs::path& lib_path, load_mode::type mode = load_mode::default_mode) {
        _lib.load(lib_path, mode);
        _storage.load(lib_path);
    }

    //! \copydoc shared_library::shared_library(const boost::dll::fs::path& lib_path, boost::dll::fs::error_code& ec, load_mode::type mode = load_mode::default_mode)
    smart_library(const boost::dll::fs::path& lib_path, boost::dll::fs::error_code& ec, load_mode::type mode = load_mode::default_mode) {
        load(lib_path, mode, ec);
    }

    //! \copydoc shared_library::shared_library(const boost::dll::fs::path& lib_path, load_mode::type mode, boost::dll::fs::error_code& ec)
    smart_library(const boost::dll::fs::path& lib_path, load_mode::type mode, boost::dll::fs::error_code& ec) {
        load(lib_path, mode, ec);
    }
    /*!
     * copy a smart_library object.
     *
     * \param lib A smart_library to move from.
     *
     * \throw Nothing.
     */
     smart_library(const smart_library & lib) BOOST_NOEXCEPT
         : _lib(lib._lib), _storage(lib._storage)
     {}
   /*!
    * Move a smart_library object.
    *
    * \param lib A smart_library to move from.
    *
    * \throw Nothing.
    */
    smart_library(BOOST_RV_REF(smart_library) lib) BOOST_NOEXCEPT
        : _lib(boost::move(lib._lib)), _storage(boost::move(lib._storage))
    {}

    /*!
      * Construct from a shared_library object.
      *
      * \param lib A shared_library to move from.
      *
      * \throw Nothing.
      */
      explicit smart_library(const shared_library & lib) BOOST_NOEXCEPT
          : _lib(lib)
      {
          _storage.load(lib.location());
      }
     /*!
     * Construct from a shared_library object.
     *
     * \param lib A shared_library to move from.
     *
     * \throw Nothing.
     */
     explicit smart_library(BOOST_RV_REF(shared_library) lib) BOOST_NOEXCEPT
         : _lib(boost::move(static_cast<shared_library&>(lib)))
     {
         _storage.load(lib.location());
     }

    /*!
    * Destroys the smart_library.
    * `unload()` is called if the DLL/DSO was loaded. If library was loaded multiple times
    * by different instances of shared_library, the actual DLL/DSO won't be unloaded until
    * there is at least one instance of shared_library.
    *
    * \throw Nothing.
    */
    ~smart_library() BOOST_NOEXCEPT {};

    //! \copydoc shared_library::load(const boost::dll::fs::path& lib_path, load_mode::type mode = load_mode::default_mode)
    void load(const boost::dll::fs::path& lib_path, load_mode::type mode = load_mode::default_mode) {
        boost::dll::fs::error_code ec;
        _storage.load(lib_path);
        _lib.load(lib_path, mode, ec);

        if (ec) {
            boost::dll::detail::report_error(ec, "load() failed");
        }
    }

    //! \copydoc shared_library::load(const boost::dll::fs::path& lib_path, boost::dll::fs::error_code& ec, load_mode::type mode = load_mode::default_mode)
    void load(const boost::dll::fs::path& lib_path, boost::dll::fs::error_code& ec, load_mode::type mode = load_mode::default_mode) {
        ec.clear();
        _storage.load(lib_path);
        _lib.load(lib_path, mode, ec);
    }

    //! \copydoc shared_library::load(const boost::dll::fs::path& lib_path, load_mode::type mode, boost::dll::fs::error_code& ec)
    void load(const boost::dll::fs::path& lib_path, load_mode::type mode, boost::dll::fs::error_code& ec) {
        ec.clear();
        _storage.load(lib_path);
        _lib.load(lib_path, mode, ec);
    }

    /*!
     * Load a variable from the referenced library.
     *
     * Unlinke shared_library::get this function will also load scoped variables, which also includes static class members.
     *
     * \note When mangled, MSVC will also check the type.
     *
     * \param name Name of the variable
     * \tparam T Type of the variable
     * \return A reference to the variable of type T.
     *
     * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
     */
    template<typename T>
    T& get_variable(const std::string &name) const {
        return _lib.get<T>(_storage.get_variable<T>(name));
    }

    /*!
     * Load a function from the referenced library.
     *
     * \b Example:
     *
     * \code
     * smart_library lib("test_lib.so");
     * typedef int      (&add_ints)(int, int);
     * typedef double (&add_doubles)(double, double);
     * add_ints     f1 = lib.get_function<int(int, int)>         ("func_name");
     * add_doubles  f2 = lib.get_function<double(double, double)>("func_name");
     * \endcode
     *
     * \note When mangled, MSVC will also check the return type.
     *
     * \param name Name of the function.
     * \tparam Func Type of the function, required for determining the overload
     * \return A reference to the function of type F.
     *
     * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
     */
    template<typename Func>
    Func& get_function(const std::string &name) const {
        return _lib.get<Func>(_storage.get_function<Func>(name));
    }

    /*!
     * Load a member-function from the referenced library.
     *
     * \b Example (import class is MyClass, which is available inside the library and the host):
     *
     * \code
     * smart_library lib("test_lib.so");
     *
     * typedef int      MyClass(*func)(int);
     * typedef int   MyClass(*func_const)(int) const;
     *
     * add_ints     f1 = lib.get_mem_fn<MyClass, int(int)>              ("MyClass::function");
     * add_doubles  f2 = lib.get_mem_fn<const MyClass, double(double)>("MyClass::function");
     * \endcode
     *
     * \note When mangled, MSVC will also check the return type.
     *
     * \param name Name of the function.
     * \tparam Class The class the function is a member of. If Class is const, the function will be assumed as taking a const this-pointer. The same applies for volatile.
     * \tparam Func Signature of the function, required for determining the overload
     * \return A pointer to the member-function with the signature provided
     *
     * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
     */
    template<typename Class, typename Func>
    typename boost::dll::detail::get_mem_fn_type<Class, Func>::mem_fn get_mem_fn(const std::string& name) const {
        return _lib.get<typename boost::dll::detail::get_mem_fn_type<Class, Func>::mem_fn>(
                _storage.get_mem_fn<Class, Func>(name)
        );
    }

    /*!
     * Load a constructor from the referenced library.
     *
     * \b Example (import class is MyClass, which is available inside the library and the host):
     *
     * \code
     * smart_library lib("test_lib.so");
     *
     * constructor<MyClass(int)    f1 = lib.get_mem_fn<MyClass(int)>();
     * \endcode
     *
     * \tparam Signature Signature of the function, required for determining the overload. The return type is the class which this is the constructor of.
     * \return A constructor object.
     *
     * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
     */
    template<typename Signature>
    constructor<Signature> get_constructor() const {
        return boost::dll::detail::load_ctor<Signature>(_lib, _storage.get_constructor<Signature>());
    }

    /*!
     * Load a destructor from the referenced library.
     *
     * \b Example (import class is MyClass, which is available inside the library and the host):
     *
     * \code
     * smart_library lib("test_lib.so");
     *
     * destructor<MyClass>     f1 = lib.get_mem_fn<MyClass>();
     * \endcode
     *
     * \tparam Class The class whose destructor shall be loaded
     * \return A destructor object.
     *
     * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
     *
     */
    template<typename Class>
    destructor<Class> get_destructor() const {
        return boost::dll::detail::load_dtor<Class>(_lib, _storage.get_destructor<Class>());
    }
    /*!
     * Load the typeinfo of the given type.
     *
     * \b Example (import class is MyClass, which is available inside the library and the host):
     *
     * \code
     * smart_library lib("test_lib.so");
     *
     * std::type_info &ti = lib.get_Type_info<MyClass>();
     * \endcode
     *
     * \tparam Class The class whose typeinfo shall be loaded
     * \return A reference to a type_info object.
     *
     * \throw \forcedlinkfs{system_error} if symbol does not exist or if the DLL/DSO was not loaded.
     *
     */
    template<typename Class>
    const std::type_info& get_type_info() const
    {
        return boost::dll::detail::load_type_info<Class>(_lib, _storage);
    }
    /**
     * This function can be used to add a type alias.
     *
     * This is to be used, when a class shall be imported, which is not declared on the host side.
     *
     * Example:
     * \code
     * smart_library lib("test_lib.so");
     *
     * lib.add_type_alias<MyAlias>("MyClass"); //when using MyAlias, the library will look for MyClass
     *
     * //get the destructor of MyClass
     * destructor<MyAlias> dtor = lib.get_destructor<MyAlias>();
     * \endcode
     *
     *
     * \param name Name of the class the alias is for.
     *
     * \attention If the alias-type is not large enough for the imported class, it will result in undefined behaviour.
     * \warning The alias will only be applied for the type signature, it will not replace the token in the scoped name.
     */
    template<typename Alias> void add_type_alias(const std::string& name) {
        this->_storage.add_alias<Alias>(name);
    }

    //! \copydoc shared_library::unload()
    void unload() BOOST_NOEXCEPT {
        _storage.clear();
        _lib.unload();
    }

    //! \copydoc shared_library::is_loaded() const
    bool is_loaded() const BOOST_NOEXCEPT {
        return _lib.is_loaded();
    }

    //! \copydoc shared_library::operator!() const
    bool operator!() const BOOST_NOEXCEPT {
        return !is_loaded();
    }

    //! \copydoc shared_library::operator bool() const
    BOOST_EXPLICIT_OPERATOR_BOOL()

    //! \copydoc shared_library::has(const char* symbol_name) const
    bool has(const char* symbol_name) const BOOST_NOEXCEPT {
        return _lib.has(symbol_name);
    }

    //! \copydoc shared_library::has(const std::string& symbol_name) const
    bool has(const std::string& symbol_name) const BOOST_NOEXCEPT {
        return _lib.has(symbol_name);
    }

    //! \copydoc shared_library::assign(const shared_library& lib)
    smart_library& assign(const smart_library& lib) {
       _lib.assign(lib._lib);
       _storage.assign(lib._storage);
       return *this;
    }

    //! \copydoc shared_library::swap(shared_library& rhs)
    void swap(smart_library& rhs) BOOST_NOEXCEPT {
        _lib.swap(rhs._lib);
        _storage.swap(rhs._storage);
    }
};

/// Very fast equality check that compares the actual DLL/DSO objects. Throws nothing.
inline bool operator==(const smart_library& lhs, const smart_library& rhs) BOOST_NOEXCEPT {
    return lhs.shared_lib().native() == rhs.shared_lib().native();
}

/// Very fast inequality check that compares the actual DLL/DSO objects. Throws nothing.
inline bool operator!=(const smart_library& lhs, const smart_library& rhs) BOOST_NOEXCEPT {
    return lhs.shared_lib().native() != rhs.shared_lib().native();
}

/// Compare the actual DLL/DSO objects without any guarantee to be stable between runs. Throws nothing.
inline bool operator<(const smart_library& lhs, const smart_library& rhs) BOOST_NOEXCEPT {
    return lhs.shared_lib().native() < rhs.shared_lib().native();
}

/// Swaps two shared libraries. Does not invalidate symbols and functions loaded from libraries. Throws nothing.
inline void swap(smart_library& lhs, smart_library& rhs) BOOST_NOEXCEPT {
    lhs.swap(rhs);
}


#ifdef BOOST_DLL_DOXYGEN
/** Helper functions for overloads.
 *
 * Gets either a variable, function or member-function, depending on the signature.
 *
 * @code
 * smart_library sm("lib.so");
 * get<int>(sm, "space::value"); //import a variable
 * get<void(int)>(sm, "space::func"); //import a function
 * get<some_class, void(int)>(sm, "space::class_::mem_fn"); //import a member function
 * @endcode
 *
 * @param sm A reference to the @ref smart_library
 * @param name The name of the entity to import
 */
template<class T, class T2>
void get(const smart_library& sm, const std::string &name);
#endif

template<class T>
typename boost::enable_if<boost::is_object<T>, T&>::type get(const smart_library& sm, const std::string &name)

{
    return sm.get_variable<T>(name);
}

template<class T>
typename boost::enable_if<boost::is_function<T>, T&>::type get(const smart_library& sm, const std::string &name)
{
    return sm.get_function<T>(name);
}

template<class Class, class Signature>
auto get(const smart_library& sm, const std::string &name) -> typename detail::get_mem_fn_type<Class, Signature>::mem_fn
{
    return sm.get_mem_fn<Class, Signature>(name);
}


} /* namespace experimental */
} /* namespace dll */
} /* namespace boost */

#endif /* BOOST_DLL_SMART_LIBRARY_HPP_ */
