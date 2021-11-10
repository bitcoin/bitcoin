//
// Copyright 2013-2021 Antony Polukhin.
//
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_TYPE_INDEX_CTTI_TYPE_INDEX_HPP
#define BOOST_TYPE_INDEX_CTTI_TYPE_INDEX_HPP

/// \file ctti_type_index.hpp
/// \brief Contains boost::typeindex::ctti_type_index class that is constexpr if C++14 constexpr is supported by compiler.
///
/// boost::typeindex::ctti_type_index class can be used as a drop-in replacement
/// for std::type_index.
///
/// It is used in situations when typeid() method is not available or 
/// BOOST_TYPE_INDEX_FORCE_NO_RTTI_COMPATIBILITY macro is defined.

#include <boost/type_index/type_index_facade.hpp>
#include <boost/type_index/detail/compile_time_type_info.hpp>

#include <cstring>
#include <boost/container_hash/hash.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

namespace boost { namespace typeindex {

namespace detail {

// That's the most trickiest part of the TypeIndex library:
//      1) we do not want to give user ability to manually construct and compare `struct-that-represents-type`
//      2) we need to distinguish between `struct-that-represents-type` and `const char*`
//      3) we need a thread-safe way to have references to instances `struct-that-represents-type`
//      4) we need a compile-time control to make sure that user does not copy or
// default construct `struct-that-represents-type`
//
// Solution would be the following:

/// \class ctti_data
/// Standard-layout class with private constructors and assignment operators.
///
/// You can not work with this class directly. The  purpose of this class is to hold type info 
/// \b when \b RTTI \b is \b off and allow ctti_type_index construction from itself.
///
/// \b Example:
/// \code
/// const detail::ctti_data& foo();
/// ...
/// type_index ti = type_index(foo());
/// std::cout << ti.pretty_name();
/// \endcode
class ctti_data {
#ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
public:
    ctti_data() = delete;
    ctti_data(const ctti_data&) = delete;
    ctti_data& operator=(const ctti_data&) = delete;
#else
private:
    ctti_data();
    ctti_data(const ctti_data&);
    ctti_data& operator=(const ctti_data&);
#endif
};

} // namespace detail

/// Helper method for getting detail::ctti_data of a template parameter T.
template <class T>
inline const detail::ctti_data& ctti_construct() BOOST_NOEXCEPT {
    // Standard C++11, 5.2.10 Reinterpret cast:
    // An object pointer can be explicitly converted to an object pointer of a different type. When a prvalue
    // v of type "pointer to T1" is converted to the type "pointer to cv T2", the result is static_cast<cv
    // T2*>(static_cast<cv void*>(v)) if both T1 and T2 are standard-layout types (3.9) and the alignment
    // requirements of T2 are no stricter than those of T1, or if either type is void. Converting a prvalue of type
    // "pointer to T1" to the type "pointer to T2" (where T1 and T2 are object types and where the alignment
    // requirements of T2 are no stricter than those of T1) and back to its original type yields the original pointer
    // value.
    //
    // Alignments are checked in `type_index_test_ctti_alignment.cpp` test.
    return *reinterpret_cast<const detail::ctti_data*>(boost::detail::ctti<T>::n());
}

/// \class ctti_type_index
/// This class is a wrapper that pretends to work exactly like stl_type_index, but does
/// not require RTTI support. \b For \b description \b of \b functions \b see type_index_facade.
///
/// This class on C++14 compatible compilers has following functions marked as constexpr:
///     * default constructor
///     * copy constructors and assignemnt operations
///     * class methods: name(), before(const ctti_type_index& rhs), equal(const ctti_type_index& rhs)
///     * static methods type_id<T>(), type_id_with_cvr<T>()
///     * comparison operators
///
/// This class produces slightly longer type names, so consider using stl_type_index
/// in situations when typeid() is working.
class ctti_type_index: public type_index_facade<ctti_type_index, detail::ctti_data> {
    const char* data_;

    inline std::size_t get_raw_name_length() const BOOST_NOEXCEPT;

    BOOST_CXX14_CONSTEXPR inline explicit ctti_type_index(const char* data) BOOST_NOEXCEPT
        : data_(data)
    {}

public:
    typedef detail::ctti_data type_info_t;

    BOOST_CXX14_CONSTEXPR inline ctti_type_index() BOOST_NOEXCEPT
        : data_(boost::detail::ctti<void>::n())
    {}

    inline ctti_type_index(const type_info_t& data) BOOST_NOEXCEPT
        : data_(reinterpret_cast<const char*>(&data))
    {}

    inline const type_info_t& type_info() const BOOST_NOEXCEPT;
    BOOST_CXX14_CONSTEXPR inline const char* raw_name() const BOOST_NOEXCEPT;
    BOOST_CXX14_CONSTEXPR inline const char* name() const BOOST_NOEXCEPT;
    inline std::string  pretty_name() const;
    inline std::size_t  hash_code() const BOOST_NOEXCEPT;

    BOOST_CXX14_CONSTEXPR inline bool equal(const ctti_type_index& rhs) const BOOST_NOEXCEPT;
    BOOST_CXX14_CONSTEXPR inline bool before(const ctti_type_index& rhs) const BOOST_NOEXCEPT;

    template <class T>
    BOOST_CXX14_CONSTEXPR inline static ctti_type_index type_id() BOOST_NOEXCEPT;

    template <class T>
    BOOST_CXX14_CONSTEXPR inline static ctti_type_index type_id_with_cvr() BOOST_NOEXCEPT;

    template <class T>
    inline static ctti_type_index type_id_runtime(const T& variable) BOOST_NOEXCEPT;
};


inline const ctti_type_index::type_info_t& ctti_type_index::type_info() const BOOST_NOEXCEPT {
    return *reinterpret_cast<const detail::ctti_data*>(data_);
}


BOOST_CXX14_CONSTEXPR inline bool ctti_type_index::equal(const ctti_type_index& rhs) const BOOST_NOEXCEPT {
    const char* const left = raw_name();
    const char* const right = rhs.raw_name();
    return /*left == right ||*/ !boost::typeindex::detail::constexpr_strcmp(left, right);
}

BOOST_CXX14_CONSTEXPR inline bool ctti_type_index::before(const ctti_type_index& rhs) const BOOST_NOEXCEPT {
    const char* const left = raw_name();
    const char* const right = rhs.raw_name();
    return /*left != right &&*/ boost::typeindex::detail::constexpr_strcmp(left, right) < 0;
}


template <class T>
BOOST_CXX14_CONSTEXPR inline ctti_type_index ctti_type_index::type_id() BOOST_NOEXCEPT {
    typedef BOOST_DEDUCED_TYPENAME boost::remove_reference<T>::type no_ref_t;
    typedef BOOST_DEDUCED_TYPENAME boost::remove_cv<no_ref_t>::type no_cvr_t;
    return ctti_type_index(boost::detail::ctti<no_cvr_t>::n());
}



template <class T>
BOOST_CXX14_CONSTEXPR inline ctti_type_index ctti_type_index::type_id_with_cvr() BOOST_NOEXCEPT {
    return ctti_type_index(boost::detail::ctti<T>::n());
}


template <class T>
inline ctti_type_index ctti_type_index::type_id_runtime(const T& variable) BOOST_NOEXCEPT {
    return variable.boost_type_index_type_id_runtime_();
}


BOOST_CXX14_CONSTEXPR inline const char* ctti_type_index::raw_name() const BOOST_NOEXCEPT {
    return data_;
}


BOOST_CXX14_CONSTEXPR inline const char* ctti_type_index::name() const BOOST_NOEXCEPT {
    return data_;
}

inline std::size_t ctti_type_index::get_raw_name_length() const BOOST_NOEXCEPT {
    return std::strlen(raw_name() + detail::ctti_skip_size_at_end);
}


inline std::string ctti_type_index::pretty_name() const {
    std::size_t len = get_raw_name_length();
    while (raw_name()[len - 1] == ' ') --len; // MSVC sometimes adds whitespaces
    return std::string(raw_name(), len);
}


inline std::size_t ctti_type_index::hash_code() const BOOST_NOEXCEPT {
    return boost::hash_range(raw_name(), raw_name() + get_raw_name_length());
}


}} // namespace boost::typeindex

#endif // BOOST_TYPE_INDEX_CTTI_TYPE_INDEX_HPP

