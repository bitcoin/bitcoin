//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_ALIGN_HPP
#define BOOST_JSON_DETAIL_ALIGN_HPP

#ifndef BOOST_JSON_STANDALONE
#include <boost/align/align.hpp>
#else
#include <cstddef>
#include <memory>
#endif

BOOST_JSON_NS_BEGIN
namespace detail {

#ifndef BOOST_JSON_STANDALONE
using boost::alignment::align;

// VFALCO workaround until Boost.Align has the type

struct class_type {};
enum unscoped_enumeration_type { };
enum class scoped_enumeration_type { };

// [support.types] p5: The type max_align_t is a trivial
// standard-layout type whose alignment requirement
// is at least as great as that of every scalar type.
struct max_align_t
{
    // arithmetic types
    char a;
    char16_t b;
    char32_t c;
    bool d;
    short int e;
    int f;
    long int g;
    long long int h;
    wchar_t i;
    float j;
    double k;
    long double l;
    // enumeration types
    unscoped_enumeration_type m;
    scoped_enumeration_type n;
    // pointer types
    void* o;
    char* p;
    class_type* q;
    unscoped_enumeration_type* r;
    scoped_enumeration_type* s;
    void(*t)();
    // pointer to member types
    char class_type::* u;
    void (class_type::*v)();
    // nullptr
    std::nullptr_t w;
};

#else
using std::align;
using max_align_t = std::max_align_t;
#endif

} // detail
BOOST_JSON_NS_END

#endif
