//  This header intentionally has no include guards.
//
//  Copyright (c) 2010 Neil Groves
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// This code utilises the experience gained during the evolution of
// <boost/smart_ptr/operator_bool.hpp>
#ifndef BOOST_RANGE_SAFE_BOOL_INCLUDED_HPP
#define BOOST_RANGE_SAFE_BOOL_INCLUDED_HPP

#include <boost/config.hpp>
#include <boost/range/config.hpp>

namespace boost
{
    namespace range_detail
    {

template<class DataMemberPtr>
class safe_bool
{
public:
    typedef safe_bool this_type;

#if (defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, < 0x570)) || defined(__CINT_)
    typedef bool unspecified_bool_type;
    static unspecified_bool_type to_unspecified_bool(const bool x, DataMemberPtr)
    {
        return x;
    }
#elif defined(_MANAGED)
    static void unspecified_bool(this_type***)
    {
    }
    typedef void(*unspecified_bool_type)(this_type***);
    static unspecified_bool_type to_unspecified_bool(const bool x, DataMemberPtr)
    {
        return x ? unspecified_bool : 0;
    }
#elif \
    ( defined(__MWERKS__) && BOOST_WORKAROUND(__MWERKS__, < 0x3200) ) || \
    ( defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ < 304) ) || \
    ( defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, <= 0x590) )

    typedef bool (this_type::*unspecified_bool_type)() const;

    static unspecified_bool_type to_unspecified_bool(const bool x, DataMemberPtr)
    {
        return x ? &this_type::detail_safe_bool_member_fn : 0;
    }
private:
    bool detail_safe_bool_member_fn() const { return false; }
#else
    typedef DataMemberPtr unspecified_bool_type;
    static unspecified_bool_type to_unspecified_bool(const bool x, DataMemberPtr p)
    {
        return x ? p : 0;
    }
#endif
private:
    safe_bool();
    safe_bool(const safe_bool&);
    void operator=(const safe_bool&);
    ~safe_bool();
};

    } // namespace range_detail
} // namespace boost

#endif // include guard
