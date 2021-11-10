//
// Copyright 2013-2021 Antony Polukhin.
//
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_TYPE_INDEX_TYPE_INDEX_FACADE_HPP
#define BOOST_TYPE_INDEX_TYPE_INDEX_FACADE_HPP

#include <boost/config.hpp>
#include <boost/container_hash/hash_fwd.hpp>
#include <string>
#include <cstring>

#if !defined(BOOST_NO_IOSTREAM)
#if !defined(BOOST_NO_IOSFWD)
#include <iosfwd>               // for std::basic_ostream
#else
#include <ostream>
#endif
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

namespace boost { namespace typeindex {

/// \class type_index_facade
///
/// This class takes care about the comparison operators, hash functions and 
/// ostream operators. Use this class as a public base class for defining new
/// type_info-conforming classes.
///
/// \b Example:
/// \code
/// class stl_type_index: public type_index_facade<stl_type_index, std::type_info> 
/// {
/// public:
///     typedef std::type_info type_info_t;
/// private:
///     const type_info_t* data_;
///
/// public:
///     stl_type_index(const type_info_t& data) noexcept
///         : data_(&data)
///     {}
/// // ...
/// };
/// \endcode
///
/// \tparam Derived Class derived from type_index_facade.
/// \tparam TypeInfo Class that will be used as a base type_info class.
/// \note Take a look at the protected methods. They are \b not \b defined in type_index_facade. 
/// Protected member functions raw_name() \b must be defined in Derived class. All the other 
/// methods are mandatory.
/// \see 'Making a custom type_index' section for more information about 
/// creating your own type_index using type_index_facade.
template <class Derived, class TypeInfo>
class type_index_facade {
private:
    /// @cond
    BOOST_CXX14_CONSTEXPR const Derived & derived() const BOOST_NOEXCEPT {
      return *static_cast<Derived const*>(this);
    }
    /// @endcond
public:
    typedef TypeInfo                                type_info_t;

    /// \b Override: This function \b may be redefined in Derived class. Overrides \b must not throw.
    /// \return Name of a type. By default returns Derived::raw_name().
    inline const char* name() const BOOST_NOEXCEPT {
        return derived().raw_name();
    }

    /// \b Override: This function \b may be redefined in Derived class. Overrides may throw.
    /// \return Human readable type name. By default returns Derived::name().
    inline std::string pretty_name() const {
        return derived().name();
    }

    /// \b Override: This function \b may be redefined in Derived class. Overrides \b must not throw.
    /// \return True if two types are equal. By default compares types by raw_name().
    inline bool equal(const Derived& rhs) const BOOST_NOEXCEPT {
        const char* const left = derived().raw_name();
        const char* const right = rhs.raw_name();
        return left == right || !std::strcmp(left, right);
    }

    /// \b Override: This function \b may be redefined in Derived class. Overrides \b must not throw.
    /// \return True if rhs is greater than this. By default compares types by raw_name().
    inline bool before(const Derived& rhs) const BOOST_NOEXCEPT {
        const char* const left = derived().raw_name();
        const char* const right = rhs.raw_name();
        return left != right && std::strcmp(left, right) < 0;
    }

    /// \b Override: This function \b may be redefined in Derived class. Overrides \b must not throw.
    /// \return Hash code of a type. By default hashes types by raw_name().
    /// \note Derived class header \b must include <boost/container_hash/hash.hpp>, \b unless this function is redefined in
    /// Derived class to not use boost::hash_range().
    inline std::size_t hash_code() const BOOST_NOEXCEPT {
        const char* const name_raw = derived().raw_name();
        return boost::hash_range(name_raw, name_raw + std::strlen(name_raw));
    }

#if defined(BOOST_TYPE_INDEX_DOXYGEN_INVOKED)
protected:
    /// \b Override: This function \b must be redefined in Derived class. Overrides \b must not throw.
    /// \return Pointer to unredable/raw type name.
    inline const char* raw_name() const BOOST_NOEXCEPT;

    /// \b Override: This function \b may be redefined in Derived class. Overrides \b must not throw.
    /// \return Const reference to underlying low level type_info_t.
    inline const type_info_t& type_info() const BOOST_NOEXCEPT;

    /// This is a factory method that is used to create instances of Derived classes.
    /// boost::typeindex::type_id() will call this method, if Derived has same type as boost::typeindex::type_index.
    ///
    /// \b Override: This function \b may be redefined and made public in Derived class. Overrides \b must not throw. 
    /// Overrides \b must remove const, volatile && and & modifiers from T.
    /// \tparam T Type for which type_index must be created.
    /// \return type_index for type T.
    template <class T>
    static Derived type_id() BOOST_NOEXCEPT;

    /// This is a factory method that is used to create instances of Derived classes.
    /// boost::typeindex::type_id_with_cvr() will call this method, if Derived has same type as boost::typeindex::type_index.
    ///
    /// \b Override: This function \b may be redefined and made public in Derived class. Overrides \b must not throw. 
    /// Overrides \b must \b not remove const, volatile && and & modifiers from T.
    /// \tparam T Type for which type_index must be created.
    /// \return type_index for type T.
    template <class T>
    static Derived type_id_with_cvr() BOOST_NOEXCEPT;

    /// This is a factory method that is used to create instances of Derived classes.
    /// boost::typeindex::type_id_runtime(const T&) will call this method, if Derived has same type as boost::typeindex::type_index.
    ///
    /// \b Override: This function \b may be redefined and made public in Derived class.
    /// \param variable Variable which runtime type will be stored in type_index.
    /// \return type_index with runtime type of variable.
    template <class T>
    static Derived type_id_runtime(const T& variable) BOOST_NOEXCEPT;

#endif

};

/// @cond
template <class Derived, class TypeInfo>
BOOST_CXX14_CONSTEXPR inline bool operator == (const type_index_facade<Derived, TypeInfo>& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return static_cast<Derived const&>(lhs).equal(static_cast<Derived const&>(rhs));
}

template <class Derived, class TypeInfo>
BOOST_CXX14_CONSTEXPR inline bool operator < (const type_index_facade<Derived, TypeInfo>& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return static_cast<Derived const&>(lhs).before(static_cast<Derived const&>(rhs));
}



template <class Derived, class TypeInfo>
BOOST_CXX14_CONSTEXPR inline bool operator > (const type_index_facade<Derived, TypeInfo>& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return rhs < lhs;
}

template <class Derived, class TypeInfo>
BOOST_CXX14_CONSTEXPR inline bool operator <= (const type_index_facade<Derived, TypeInfo>& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return !(lhs > rhs);
}

template <class Derived, class TypeInfo>
BOOST_CXX14_CONSTEXPR inline bool operator >= (const type_index_facade<Derived, TypeInfo>& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return !(lhs < rhs);
}

template <class Derived, class TypeInfo>
BOOST_CXX14_CONSTEXPR inline bool operator != (const type_index_facade<Derived, TypeInfo>& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return !(lhs == rhs);
}

// ######################### COMPARISONS with Derived ############################ //
template <class Derived, class TypeInfo>
inline bool operator == (const TypeInfo& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return Derived(lhs) == rhs;
}

template <class Derived, class TypeInfo>
inline bool operator < (const TypeInfo& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return Derived(lhs) < rhs;
}

template <class Derived, class TypeInfo>
inline bool operator > (const TypeInfo& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return rhs < Derived(lhs);
}

template <class Derived, class TypeInfo>
inline bool operator <= (const TypeInfo& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return !(Derived(lhs) > rhs);
}

template <class Derived, class TypeInfo>
inline bool operator >= (const TypeInfo& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return !(Derived(lhs) < rhs);
}

template <class Derived, class TypeInfo>
inline bool operator != (const TypeInfo& lhs, const type_index_facade<Derived, TypeInfo>& rhs) BOOST_NOEXCEPT {
    return !(Derived(lhs) == rhs);
}


template <class Derived, class TypeInfo>
inline bool operator == (const type_index_facade<Derived, TypeInfo>& lhs, const TypeInfo& rhs) BOOST_NOEXCEPT {
    return lhs == Derived(rhs);
}

template <class Derived, class TypeInfo>
inline bool operator < (const type_index_facade<Derived, TypeInfo>& lhs, const TypeInfo& rhs) BOOST_NOEXCEPT {
    return lhs < Derived(rhs);
}

template <class Derived, class TypeInfo>
inline bool operator > (const type_index_facade<Derived, TypeInfo>& lhs, const TypeInfo& rhs) BOOST_NOEXCEPT {
    return Derived(rhs) < lhs;
}

template <class Derived, class TypeInfo>
inline bool operator <= (const type_index_facade<Derived, TypeInfo>& lhs, const TypeInfo& rhs) BOOST_NOEXCEPT {
    return !(lhs > Derived(rhs));
}

template <class Derived, class TypeInfo>
inline bool operator >= (const type_index_facade<Derived, TypeInfo>& lhs, const TypeInfo& rhs) BOOST_NOEXCEPT {
    return !(lhs < Derived(rhs));
}

template <class Derived, class TypeInfo>
inline bool operator != (const type_index_facade<Derived, TypeInfo>& lhs, const TypeInfo& rhs) BOOST_NOEXCEPT {
    return !(lhs == Derived(rhs));
}

// ######################### COMPARISONS with Derived END ############################ //

/// @endcond

#if defined(BOOST_TYPE_INDEX_DOXYGEN_INVOKED)

/// noexcept comparison operators for type_index_facade classes.
bool operator ==, !=, <, ... (const type_index_facade& lhs, const type_index_facade& rhs) noexcept;

/// noexcept comparison operators for type_index_facade and it's TypeInfo classes.
bool operator ==, !=, <, ... (const type_index_facade& lhs, const TypeInfo& rhs) noexcept;

/// noexcept comparison operators for type_index_facade's TypeInfo and type_index_facade classes.
bool operator ==, !=, <, ... (const TypeInfo& lhs, const type_index_facade& rhs) noexcept;

#endif

#ifndef BOOST_NO_IOSTREAM
#ifdef BOOST_NO_TEMPLATED_IOSTREAMS
/// @cond
/// Ostream operator that will output demangled name
template <class Derived, class TypeInfo>
inline std::ostream& operator<<(std::ostream& ostr, const type_index_facade<Derived, TypeInfo>& ind) {
    ostr << static_cast<Derived const&>(ind).pretty_name();
    return ostr;
}
/// @endcond
#else
/// Ostream operator that will output demangled name.
template <class CharT, class TriatT, class Derived, class TypeInfo>
inline std::basic_ostream<CharT, TriatT>& operator<<(
    std::basic_ostream<CharT, TriatT>& ostr, 
    const type_index_facade<Derived, TypeInfo>& ind) 
{
    ostr << static_cast<Derived const&>(ind).pretty_name();
    return ostr;
}
#endif // BOOST_NO_TEMPLATED_IOSTREAMS
#endif // BOOST_NO_IOSTREAM

/// This free function is used by Boost's unordered containers.
/// \note <boost/container_hash/hash.hpp> has to be included if this function is used.
template <class Derived, class TypeInfo>
inline std::size_t hash_value(const type_index_facade<Derived, TypeInfo>& lhs) BOOST_NOEXCEPT {
    return static_cast<Derived const&>(lhs).hash_code();
}

}} // namespace boost::typeindex

#endif // BOOST_TYPE_INDEX_TYPE_INDEX_FACADE_HPP

