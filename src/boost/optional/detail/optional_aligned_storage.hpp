// Copyright (C) 2003, 2008 Fernando Luis Cacciola Carballal.
// Copyright (C) 2016 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  fernando_cacciola@hotmail.com
//  akrzemi1@gmail.com

#ifndef BOOST_OPTIONAL_OPTIONAL_DETAIL_OPTIONAL_ALIGNED_STORAGE_AJK_12FEB2016_HPP
#define BOOST_OPTIONAL_OPTIONAL_DETAIL_OPTIONAL_ALIGNED_STORAGE_AJK_12FEB2016_HPP

namespace boost {

namespace optional_detail {
// This local class is used instead of that in "aligned_storage.hpp"
// because I've found the 'official' class to ICE BCB5.5
// when some types are used with optional<>
// (due to sizeof() passed down as a non-type template parameter)
template <class T>
class aligned_storage
{
    // Borland ICEs if unnamed unions are used for this!
    // BOOST_MAY_ALIAS works around GCC warnings about breaking strict aliasing rules when casting storage address to T*
    union BOOST_MAY_ALIAS dummy_u
    {
        char data[ sizeof(T) ];
        BOOST_DEDUCED_TYPENAME type_with_alignment<
          ::boost::alignment_of<T>::value >::type aligner_;
    } dummy_ ;

  public:

#if defined(BOOST_OPTIONAL_DETAIL_USE_ATTRIBUTE_MAY_ALIAS)
    void const* address() const { return &dummy_; }
    void      * address()       { return &dummy_; }
#else
    void const* address() const { return dummy_.data; }
    void      * address()       { return dummy_.data; }
#endif

#if defined(BOOST_OPTIONAL_DETAIL_USE_ATTRIBUTE_MAY_ALIAS)
    // This workaround is supposed to silence GCC warnings about broken strict aliasing rules
    T const* ptr_ref() const
    {
        union { void const* ap_pvoid; T const* as_ptype; } caster = { address() };
        return caster.as_ptype;
    }
    T *      ptr_ref()
    {
        union { void* ap_pvoid; T* as_ptype; } caster = { address() };
        return caster.as_ptype;
    }
#else
    T const* ptr_ref() const { return static_cast<T const*>(address()); }
    T *      ptr_ref()       { return static_cast<T *>     (address()); }
#endif

    T const& ref() const { return *ptr_ref(); }
    T &      ref()       { return *ptr_ref(); }
  
} ;

} // namespace optional_detail
} // namespace boost

#endif // header guard
