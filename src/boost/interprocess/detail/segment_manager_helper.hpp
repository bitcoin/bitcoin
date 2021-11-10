//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_BASE_HPP
#define BOOST_INTERPROCESS_SEGMENT_MANAGER_BASE_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

// interprocess
#include <boost/interprocess/exceptions.hpp>
// interprocess/detail
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/in_place_interface.hpp>
// container/detail
#include <boost/container/detail/type_traits.hpp> //alignment_of
#include <boost/container/detail/minimal_char_traits_header.hpp>
// intrusive
#include <boost/intrusive/pointer_traits.hpp>
// move/detail
#include <boost/move/detail/type_traits.hpp> //make_unsigned
// other boost
#include <boost/assert.hpp>   //BOOST_ASSERT
#include <boost/core/no_exceptions_support.hpp>
// std
#include <cstddef>   //std::size_t

//!\file
//!Describes the object placed in a memory segment that provides
//!named object allocation capabilities.

namespace boost{
namespace interprocess{

template<class MemoryManager>
class segment_manager_base;

//!An integer that describes the type of the
//!instance constructed in memory
enum instance_type {   anonymous_type, named_type, unique_type, max_allocation_type };

namespace ipcdetail{

template<class MemoryAlgorithm>
class mem_algo_deallocator
{
   void *            m_ptr;
   MemoryAlgorithm & m_algo;

   public:
   mem_algo_deallocator(void *ptr, MemoryAlgorithm &algo)
      :  m_ptr(ptr), m_algo(algo)
   {}

   void release()
   {  m_ptr = 0;  }

   ~mem_algo_deallocator()
   {  if(m_ptr) m_algo.deallocate(m_ptr);  }
};

template<class size_type>
struct block_header
{
   size_type      m_value_bytes;
   unsigned short m_num_char;
   unsigned char  m_value_alignment;
   unsigned char  m_alloc_type_sizeof_char;

   block_header(size_type val_bytes
               ,size_type val_alignment
               ,unsigned char al_type
               ,std::size_t szof_char
               ,std::size_t num_char
               )
      :  m_value_bytes(val_bytes)
      ,  m_num_char((unsigned short)num_char)
      ,  m_value_alignment((unsigned char)val_alignment)
      ,  m_alloc_type_sizeof_char( (al_type << 5u) | ((unsigned char)szof_char & 0x1F) )
   {};

   template<class T>
   block_header &operator= (const T& )
   {  return *this;  }

   size_type total_size() const
   {
      if(alloc_type() != anonymous_type){
         return name_offset() + (m_num_char+1)*sizeof_char();
      }
      else{
         return this->value_offset() + m_value_bytes;
      }
   }

   size_type value_bytes() const
   {  return m_value_bytes;   }

   template<class Header>
   size_type total_size_with_header() const
   {
      return get_rounded_size
               ( size_type(sizeof(Header))
            , size_type(::boost::container::dtl::alignment_of<block_header<size_type> >::value))
           + total_size();
   }

   unsigned char alloc_type() const
   {  return (m_alloc_type_sizeof_char >> 5u)&(unsigned char)0x7;  }

   unsigned char sizeof_char() const
   {  return m_alloc_type_sizeof_char & (unsigned char)0x1F;  }

   template<class CharType>
   CharType *name() const
   {
      return const_cast<CharType*>(reinterpret_cast<const CharType*>
         (reinterpret_cast<const char*>(this) + name_offset()));
   }

   unsigned short name_length() const
   {  return m_num_char;   }

   size_type name_offset() const
   {
      return this->value_offset() + get_rounded_size(size_type(m_value_bytes), size_type(sizeof_char()));
   }

   void *value() const
   {
      return const_cast<char*>((reinterpret_cast<const char*>(this) + this->value_offset()));
   }

   size_type value_offset() const
   {
      return get_rounded_size(size_type(sizeof(block_header<size_type>)), size_type(m_value_alignment));
   }

   template<class CharType>
   bool less_comp(const block_header<size_type> &b) const
   {
      return m_num_char < b.m_num_char ||
             (m_num_char < b.m_num_char &&
              std::char_traits<CharType>::compare(name<CharType>(), b.name<CharType>(), m_num_char) < 0);
   }

   template<class CharType>
   bool equal_comp(const block_header<size_type> &b) const
   {
      return m_num_char == b.m_num_char &&
             std::char_traits<CharType>::compare(name<CharType>(), b.name<CharType>(), m_num_char) == 0;
   }

   template<class T>
   static block_header<size_type> *block_header_from_value(T *value)
   {  return block_header_from_value(value, sizeof(T), ::boost::container::dtl::alignment_of<T>::value);  }

   static block_header<size_type> *block_header_from_value(const void *value, std::size_t sz, std::size_t algn)
   {
      block_header * hdr =
         const_cast<block_header*>
            (reinterpret_cast<const block_header*>(reinterpret_cast<const char*>(value) -
               get_rounded_size(sizeof(block_header), algn)));
      (void)sz;
      //Some sanity checks
      BOOST_ASSERT(hdr->m_value_alignment == algn);
      BOOST_ASSERT(hdr->m_value_bytes % sz == 0);
      return hdr;
   }

   template<class Header>
   static block_header<size_type> *from_first_header(Header *header)
   {
      block_header<size_type> * hdr =
         reinterpret_cast<block_header<size_type>*>(reinterpret_cast<char*>(header) +
       get_rounded_size( size_type(sizeof(Header))
                       , size_type(::boost::container::dtl::alignment_of<block_header<size_type> >::value)));
      //Some sanity checks
      return hdr;
   }

   template<class Header>
   static Header *to_first_header(block_header<size_type> *bheader)
   {
      Header * hdr =
         reinterpret_cast<Header*>(reinterpret_cast<char*>(bheader) -
       get_rounded_size( size_type(sizeof(Header))
                       , size_type(::boost::container::dtl::alignment_of<block_header<size_type> >::value)));
      //Some sanity checks
      return hdr;
   }
};

inline void array_construct(void *mem, std::size_t num, in_place_interface &table)
{
   //Try constructors
   std::size_t constructed = 0;
   BOOST_TRY{
      table.construct_n(mem, num, constructed);
   }
   //If there is an exception call destructors and erase index node
   BOOST_CATCH(...){
      std::size_t destroyed = 0;
      table.destroy_n(mem, constructed, destroyed);
      BOOST_RETHROW
   }
   BOOST_CATCH_END
}

template<class CharT>
struct intrusive_compare_key
{
   typedef CharT char_type;

   intrusive_compare_key(const CharT *str, std::size_t len)
      :  mp_str(str), m_len(len)
   {}

   const CharT *  mp_str;
   std::size_t    m_len;
};

//!This struct indicates an anonymous object creation
//!allocation
template<instance_type type>
class instance_t
{
   instance_t(){}
};

template<class T>
struct char_if_void
{
   typedef T type;
};

template<>
struct char_if_void<void>
{
   typedef char type;
};

typedef instance_t<anonymous_type>  anonymous_instance_t;
typedef instance_t<unique_type>     unique_instance_t;


template<class Hook, class CharType, class SizeType>
struct intrusive_value_type_impl
   :  public Hook
{
   private:
   //Non-copyable
   intrusive_value_type_impl(const intrusive_value_type_impl &);
   intrusive_value_type_impl& operator=(const intrusive_value_type_impl &);

   public:
   typedef CharType char_type;
   typedef SizeType size_type;

   intrusive_value_type_impl(){}

   enum  {  BlockHdrAlignment = ::boost::container::dtl::alignment_of<block_header<size_type> >::value  };

   block_header<size_type> *get_block_header() const
   {
      return const_cast<block_header<size_type>*>
         (reinterpret_cast<const block_header<size_type> *>(reinterpret_cast<const char*>(this) +
            get_rounded_size(size_type(sizeof(*this)), size_type(BlockHdrAlignment))));
   }

   bool operator <(const intrusive_value_type_impl<Hook, CharType, SizeType> & other) const
   {  return (this->get_block_header())->template less_comp<CharType>(*other.get_block_header());  }

   bool operator ==(const intrusive_value_type_impl<Hook, CharType, SizeType> & other) const
   {  return (this->get_block_header())->template equal_comp<CharType>(*other.get_block_header());  }

   static intrusive_value_type_impl *get_intrusive_value_type(block_header<size_type> *hdr)
   {
      return reinterpret_cast<intrusive_value_type_impl *>(reinterpret_cast<char*>(hdr) -
         get_rounded_size(size_type(sizeof(intrusive_value_type_impl)), size_type(BlockHdrAlignment)));
   }

   CharType *name() const
   {  return get_block_header()->template name<CharType>(); }

   unsigned short name_length() const
   {  return get_block_header()->name_length(); }

   void *value() const
   {  return get_block_header()->value(); }
};

template<class CharType>
class char_ptr_holder
{
   public:
   char_ptr_holder(const CharType *name)
      : m_name(name)
   {}

   char_ptr_holder(const anonymous_instance_t *)
      : m_name(static_cast<CharType*>(0))
   {}

   char_ptr_holder(const unique_instance_t *)
      : m_name(reinterpret_cast<CharType*>(-1))
   {}

   operator const CharType *()
   {  return m_name;  }

   const CharType *get() const
   {  return m_name;  }

   bool is_unique() const
   {  return m_name == reinterpret_cast<CharType*>(-1);  }

   bool is_anonymous() const
   {  return m_name == static_cast<CharType*>(0);  }

   private:
   const CharType *m_name;
};

//!The key of the the named allocation information index. Stores an offset pointer
//!to a null terminated string and the length of the string to speed up sorting
template<class CharT, class VoidPointer>
struct index_key
{
   typedef typename boost::intrusive::
      pointer_traits<VoidPointer>::template
         rebind_pointer<const CharT>::type               const_char_ptr_t;
   typedef CharT                                         char_type;
   typedef typename boost::intrusive::pointer_traits<const_char_ptr_t>::difference_type difference_type;
   typedef typename boost::move_detail::make_unsigned<difference_type>::type size_type;

   private:
   //Offset pointer to the object's name
   const_char_ptr_t  mp_str;
   //Length of the name buffer (null NOT included)
   size_type         m_len;
   public:

   //!Constructor of the key
   index_key (const char_type *nm, size_type length)
      : mp_str(nm), m_len(length)
   {}

   //!Less than function for index ordering
   bool operator < (const index_key & right) const
   {
      return (m_len < right.m_len) ||
               (m_len == right.m_len &&
                std::char_traits<char_type>::compare
                  (to_raw_pointer(mp_str),to_raw_pointer(right.mp_str), m_len) < 0);
   }

   //!Equal to function for index ordering
   bool operator == (const index_key & right) const
   {
      return   m_len == right.m_len &&
               std::char_traits<char_type>::compare
                  (to_raw_pointer(mp_str), to_raw_pointer(right.mp_str), m_len) == 0;
   }

   void name(const CharT *nm)
   {  mp_str = nm; }

   void name_length(size_type len)
   {  m_len = len; }

   const CharT *name() const
   {  return to_raw_pointer(mp_str); }

   size_type name_length() const
   {  return m_len; }
};

//!The index_data stores a pointer to a buffer and the element count needed
//!to know how many destructors must be called when calling destroy
template<class VoidPointer>
struct index_data
{
   typedef VoidPointer void_pointer;
   void_pointer    m_ptr;
   explicit index_data(void *ptr) : m_ptr(ptr){}

   void *value() const
   {  return static_cast<void*>(to_raw_pointer(m_ptr));  }
};

template<class MemoryAlgorithm>
struct segment_manager_base_type
{  typedef segment_manager_base<MemoryAlgorithm> type;   };

template<class CharT, class MemoryAlgorithm>
struct index_config
{
   typedef typename MemoryAlgorithm::void_pointer        void_pointer;
   typedef CharT                                         char_type;
   typedef index_key<CharT, void_pointer>        key_type;
   typedef index_data<void_pointer>              mapped_type;
   typedef typename segment_manager_base_type
      <MemoryAlgorithm>::type                            segment_manager_base;

   template<class HeaderBase>
   struct intrusive_value_type
   {  typedef intrusive_value_type_impl<HeaderBase, CharT, typename segment_manager_base::size_type>  type; };

   typedef intrusive_compare_key<CharT>            intrusive_compare_key_type;
};

template<class Iterator, bool intrusive>
class segment_manager_iterator_value_adaptor
{
   typedef typename Iterator::value_type        iterator_val_t;
   typedef typename iterator_val_t::char_type   char_type;

   public:
   segment_manager_iterator_value_adaptor(const typename Iterator::value_type &val)
      :  m_val(&val)
   {}

   const char_type *name() const
   {  return m_val->name(); }

   unsigned short name_length() const
   {  return m_val->name_length(); }

   const void *value() const
   {  return m_val->value(); }

   const typename Iterator::value_type *m_val;
};


template<class Iterator>
class segment_manager_iterator_value_adaptor<Iterator, false>
{
   typedef typename Iterator::value_type        iterator_val_t;
   typedef typename iterator_val_t::first_type  first_type;
   typedef typename iterator_val_t::second_type second_type;
   typedef typename first_type::char_type       char_type;
   typedef typename first_type::size_type       size_type;

   public:
   segment_manager_iterator_value_adaptor(const typename Iterator::value_type &val)
      :  m_val(&val)
   {}

   const char_type *name() const
   {  return m_val->first.name(); }

   size_type name_length() const
   {  return m_val->first.name_length(); }

   const void *value() const
   {
      return reinterpret_cast<block_header<size_type>*>
         (to_raw_pointer(m_val->second.m_ptr))->value();
   }

   const typename Iterator::value_type *m_val;
};

template<class Iterator, bool intrusive>
struct segment_manager_iterator_transform
{
   typedef segment_manager_iterator_value_adaptor<Iterator, intrusive> result_type;

   template <class T> result_type operator()(const T &arg) const
   {  return result_type(arg); }
};

}  //namespace ipcdetail {

//These pointers are the ones the user will use to
//indicate previous allocation types
static const ipcdetail::anonymous_instance_t   * anonymous_instance = 0;
static const ipcdetail::unique_instance_t      * unique_instance = 0;

namespace ipcdetail_really_deep_namespace {

//Otherwise, gcc issues a warning of previously defined
//anonymous_instance and unique_instance
struct dummy
{
   dummy()
   {
      (void)anonymous_instance;
      (void)unique_instance;
   }
};

}  //detail_really_deep_namespace

}} //namespace boost { namespace interprocess

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_BASE_HPP

