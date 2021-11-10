//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Benedek Thaler 2015-2016
// (C) Copyright Ion Gaztanaga 2019-2020. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://erenon.hu/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_DETAIL_GUARDS_HPP
#define BOOST_CONTAINER_DETAIL_GUARDS_HPP

#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>

#include <boost/move/core.hpp> // BOOST_MOVABLE_BUT_NOT_COPYABLE

// move/detail
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <boost/move/detail/fwd_macros.hpp>
#endif

#include <boost/container/allocator_traits.hpp>

namespace boost {
namespace container {
namespace detail {

class null_construction_guard
{
public:

   #if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

   template <typename... Args>
   null_construction_guard(Args&&...) {}

   #else

   #define NULL_CONSTRUCTION_GUARD_CODE(N) \
   BOOST_MOVE_TMPL_LT##N BOOST_MOVE_CLASS##N BOOST_MOVE_GT##N \
   BOOST_CONTAINER_FORCEINLINE null_construction_guard(BOOST_MOVE_UREFANON##N)\
   {}\
   //
   BOOST_MOVE_ITERATE_0TO9(NULL_CONSTRUCTION_GUARD_CODE)
   #undef NULL_CONSTRUCTION_GUARD_CODE
   #endif

   void release() {}
   void extend() {}
};

template <typename Allocator>
class construction_guard
{
  typedef typename boost::container::allocator_traits<Allocator>::pointer pointer;
  typedef typename boost::container::allocator_traits<Allocator>::size_type size_type;

  BOOST_MOVABLE_BUT_NOT_COPYABLE(construction_guard)

public:
   construction_guard()
      : _alloc_ptr()
      , _elem_count()
      , _allocator()
   {}

  construction_guard(pointer alloc_ptr, Allocator& allocator)
      :_alloc_ptr(alloc_ptr)
      , _elem_count(0)
      , _allocator(&allocator)
  {}

  construction_guard(BOOST_RV_REF(construction_guard) rhs)
      :_alloc_ptr(rhs._alloc_ptr)
      , _elem_count(rhs._elem_count)
      , _allocator(rhs._allocator)
  {
    rhs._elem_count = 0;
  }

  ~construction_guard()
  {
    while (_elem_count) {
      --_elem_count;
      boost::container::allocator_traits<Allocator>::destroy(*_allocator, _alloc_ptr++);
    }
  }

  void release()
  {
    _elem_count = 0;
  }

  void extend()
  {
    ++_elem_count;
  }

private:
  pointer _alloc_ptr;
  size_type _elem_count;
  Allocator* _allocator;
};


/**
 * Has two ranges
 *
 * On success, destroys the first range (src),
 * on failure, destroys the second range (dst).
 *
 * Can be used when copying/moving a range
 */
template <class Allocator>
class nand_construction_guard
{
   typedef typename boost::container::allocator_traits<Allocator>::pointer pointer;
   typedef typename boost::container::allocator_traits<Allocator>::size_type size_type;

   construction_guard<Allocator> _src;
   construction_guard<Allocator> _dst;
   bool _dst_released;

   public:
   nand_construction_guard()
      : _src()
      , _dst()
      , _dst_released(false)
   {}

   nand_construction_guard( pointer src, Allocator& src_alloc
                          , pointer dst, Allocator& dst_alloc)
    :_src(src, src_alloc),
     _dst(dst, dst_alloc),
     _dst_released(false)
   {}

   void extend()
   {
      _src.extend();
      _dst.extend();
   }

   void release() // on success
   {
      _dst.release();
      _dst_released = true;
   }

   ~nand_construction_guard()
   {
      if (! _dst_released) { _src.release(); }
   }
};


template <typename Allocator>
class allocation_guard
{
  typedef typename boost::container::allocator_traits<Allocator>::pointer pointer;
  typedef typename boost::container::allocator_traits<Allocator>::size_type size_type;

  BOOST_MOVABLE_BUT_NOT_COPYABLE(allocation_guard)

public:
  allocation_guard(pointer alloc_ptr, size_type alloc_size, Allocator& allocator)
    :_alloc_ptr(alloc_ptr),
     _alloc_size(alloc_size),
     _allocator(allocator)
  {}

  ~allocation_guard()
  {
    if (_alloc_ptr)
    {
      boost::container::allocator_traits<Allocator>::deallocate(_allocator, _alloc_ptr, _alloc_size);
    }
  }

  void release()
  {
    _alloc_ptr = 0;
  }

private:
  pointer _alloc_ptr;
  size_type _alloc_size;
  Allocator& _allocator;
};

}}} // namespace boost::container::detail

#include <boost/container/detail/config_end.hpp>

#endif // BOOST_CONTAINER_DETAIL_GUARDS_HPP
