//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_IMPL
#define BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_IMPL

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/detail/interprocess_tester.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/container/detail/type_traits.hpp>  //alignment_of, aligned_storage
#include <boost/interprocess/sync/spin/wait.hpp>
#include <boost/move/move.hpp>
#include <boost/cstdint.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

template <bool StoreDevice, class DeviceAbstraction>
class managed_open_or_create_impl_device_holder
{
   public:
   DeviceAbstraction &get_device()
   {  static DeviceAbstraction dev; return dev; }

   const DeviceAbstraction &get_device() const
   {  static DeviceAbstraction dev; return dev; }
};

template <class DeviceAbstraction>
class managed_open_or_create_impl_device_holder<true, DeviceAbstraction>
{
   public:
   DeviceAbstraction &get_device()
   {  return dev; }

   const DeviceAbstraction &get_device() const
   {  return dev; }

   private:
   DeviceAbstraction dev;
};

template<class DeviceAbstraction, std::size_t MemAlignment, bool FileBased, bool StoreDevice>
class managed_open_or_create_impl
   : public managed_open_or_create_impl_device_holder<StoreDevice, DeviceAbstraction>
{
   //Non-copyable
   BOOST_MOVABLE_BUT_NOT_COPYABLE(managed_open_or_create_impl)

   typedef managed_open_or_create_impl_device_holder<StoreDevice, DeviceAbstraction> DevHolder;
   enum
   {
      UninitializedSegment,
      InitializingSegment,
      InitializedSegment,
      CorruptedSegment
   };

   static const std::size_t RequiredAlignment =
      MemAlignment ? MemAlignment
                   : boost::container::dtl::alignment_of< boost::container::dtl::max_align_t >::value
                   ;

   public:
   static const std::size_t ManagedOpenOrCreateUserOffset =
      ct_rounded_size<sizeof(boost::uint32_t), RequiredAlignment>::value;

   managed_open_or_create_impl()
   {}

   template <class DeviceId>
   managed_open_or_create_impl(create_only_t,
                 const DeviceId & id,
                 std::size_t size,
                 mode_t mode,
                 const void *addr,
                 const permissions &perm)
   {
      priv_open_or_create
         ( DoCreate
         , id
         , size
         , mode
         , addr
         , perm
         , null_mapped_region_function());
   }

   template <class DeviceId>
   managed_open_or_create_impl(open_only_t,
                 const DeviceId & id,
                 mode_t mode,
                 const void *addr)
   {
      priv_open_or_create
         ( DoOpen
         , id
         , 0
         , mode
         , addr
         , permissions()
         , null_mapped_region_function());
   }

   template <class DeviceId>
   managed_open_or_create_impl(open_or_create_t,
                 const DeviceId & id,
                 std::size_t size,
                 mode_t mode,
                 const void *addr,
                 const permissions &perm)
   {
      priv_open_or_create
         ( DoOpenOrCreate
         , id
         , size
         , mode
         , addr
         , perm
         , null_mapped_region_function());
   }

   template <class DeviceId, class ConstructFunc>
   managed_open_or_create_impl(create_only_t,
                 const DeviceId & id,
                 std::size_t size,
                 mode_t mode,
                 const void *addr,
                 const ConstructFunc &construct_func,
                 const permissions &perm)
   {
      priv_open_or_create
         (DoCreate
         , id
         , size
         , mode
         , addr
         , perm
         , construct_func);
   }

   template <class DeviceId, class ConstructFunc>
   managed_open_or_create_impl(open_only_t,
                 const DeviceId & id,
                 mode_t mode,
                 const void *addr,
                 const ConstructFunc &construct_func)
   {
      priv_open_or_create
         ( DoOpen
         , id
         , 0
         , mode
         , addr
         , permissions()
         , construct_func);
   }

   template <class DeviceId, class ConstructFunc>
   managed_open_or_create_impl(open_or_create_t,
                 const DeviceId & id,
                 std::size_t size,
                 mode_t mode,
                 const void *addr,
                 const ConstructFunc &construct_func,
                 const permissions &perm)
   {
      priv_open_or_create
         ( DoOpenOrCreate
         , id
         , size
         , mode
         , addr
         , perm
         , construct_func);
   }

   managed_open_or_create_impl(BOOST_RV_REF(managed_open_or_create_impl) moved)
   {  this->swap(moved);   }

   managed_open_or_create_impl &operator=(BOOST_RV_REF(managed_open_or_create_impl) moved)
   {
      managed_open_or_create_impl tmp(boost::move(moved));
      this->swap(tmp);
      return *this;
   }

   ~managed_open_or_create_impl()
   {}

   std::size_t get_user_size()  const
   {  return m_mapped_region.get_size() - ManagedOpenOrCreateUserOffset; }

   void *get_user_address()  const
   {  return static_cast<char*>(m_mapped_region.get_address()) + ManagedOpenOrCreateUserOffset;  }

   std::size_t get_real_size()  const
   {  return m_mapped_region.get_size(); }

   void *get_real_address()  const
   {  return m_mapped_region.get_address();  }

   void swap(managed_open_or_create_impl &other)
   {
      this->m_mapped_region.swap(other.m_mapped_region);
   }

   bool flush()
   {  return m_mapped_region.flush();  }

   const mapped_region &get_mapped_region() const
   {  return m_mapped_region;  }


   DeviceAbstraction &get_device()
   {  return this->DevHolder::get_device(); }

   const DeviceAbstraction &get_device() const
   {  return this->DevHolder::get_device(); }

   private:

   //These are templatized to allow explicit instantiations
   template<bool dummy>
   static void truncate_device(DeviceAbstraction &, offset_t, false_)
   {} //Empty

   template<bool dummy>
   static void truncate_device(DeviceAbstraction &dev, offset_t size, true_)
   {  dev.truncate(size);  }


   template<bool dummy>
   static bool check_offset_t_size(std::size_t , false_)
   { return true; } //Empty

   template<bool dummy>
   static bool check_offset_t_size(std::size_t size, true_)
   { return size == std::size_t(offset_t(size)); }

   //These are templatized to allow explicit instantiations
   template<bool dummy, class DeviceId>
   static void create_device(DeviceAbstraction &dev, const DeviceId & id, std::size_t size, const permissions &perm, false_ file_like)
   {
      (void)file_like;
      DeviceAbstraction tmp(create_only, id, read_write, size, perm);
      tmp.swap(dev);
   }

   template<bool dummy, class DeviceId>
   static void create_device(DeviceAbstraction &dev, const DeviceId & id, std::size_t, const permissions &perm, true_ file_like)
   {
      (void)file_like;
      DeviceAbstraction tmp(create_only, id, read_write, perm);
      tmp.swap(dev);
   }

   template <class DeviceId, class ConstructFunc> inline
   void priv_open_or_create
      (create_enum_t type,
       const DeviceId & id,
       std::size_t size,
       mode_t mode, const void *addr,
       const permissions &perm,
       ConstructFunc construct_func)
   {
      typedef bool_<FileBased> file_like_t;
      (void)mode;
      bool created = false;
      bool ronly   = false;
      bool cow     = false;
      DeviceAbstraction dev;

      if(type != DoOpen){
         //Check if the requested size is enough to build the managed metadata
         const std::size_t func_min_size = construct_func.get_min_size();
         if( (std::size_t(-1) - ManagedOpenOrCreateUserOffset) < func_min_size ||
             size < (func_min_size + ManagedOpenOrCreateUserOffset) ){
            throw interprocess_exception(error_info(size_error));
         }
      }
      //Check size can be represented by offset_t (used by truncate)
      if(type != DoOpen && !check_offset_t_size<FileBased>(size, file_like_t())){
         throw interprocess_exception(error_info(size_error));
      }
      if(type == DoOpen && mode == read_write){
         DeviceAbstraction tmp(open_only, id, read_write);
         tmp.swap(dev);
         created = false;
      }
      else if(type == DoOpen && mode == read_only){
         DeviceAbstraction tmp(open_only, id, read_only);
         tmp.swap(dev);
         created = false;
         ronly   = true;
      }
      else if(type == DoOpen && mode == copy_on_write){
         DeviceAbstraction tmp(open_only, id, read_only);
         tmp.swap(dev);
         created = false;
         cow     = true;
      }
      else if(type == DoCreate){
         create_device<FileBased>(dev, id, size, perm, file_like_t());
         created = true;
      }
      else if(type == DoOpenOrCreate){
         //This loop is very ugly, but brute force is sometimes better
         //than diplomacy. If someone knows how to open or create a
         //file and know if we have really created it or just open it
         //drop me a e-mail!
         bool completed = false;
         spin_wait swait;
         while(!completed){
            try{
               create_device<FileBased>(dev, id, size, perm, file_like_t());
               created     = true;
               completed   = true;
            }
            catch(interprocess_exception &ex){
               if(ex.get_error_code() != already_exists_error){
                  throw;
               }
               else{
                  try{
                     DeviceAbstraction tmp(open_only, id, read_write);
                     dev.swap(tmp);
                     created     = false;
                     completed   = true;
                  }
                  catch(interprocess_exception &e){
                     if(e.get_error_code() != not_found_error){
                        throw;
                     }
                  }
                  catch(...){
                     throw;
                  }
               }
            }
            catch(...){
               throw;
            }
            swait.yield();
         }
      }

      if(created){
         try{
            //If this throws, we are lost
            truncate_device<FileBased>(dev, size, file_like_t());

            //If the following throws, we will truncate the file to 1
            mapped_region        region(dev, read_write, 0, 0, addr);
            boost::uint32_t *patomic_word = 0;  //avoid gcc warning
            patomic_word = static_cast<boost::uint32_t*>(region.get_address());
            boost::uint32_t previous = atomic_cas32(patomic_word, InitializingSegment, UninitializedSegment);

            if(previous == UninitializedSegment){
               try{
                  construct_func( static_cast<char*>(region.get_address()) + ManagedOpenOrCreateUserOffset
                                , size - ManagedOpenOrCreateUserOffset, true);
                  //All ok, just move resources to the external mapped region
                  m_mapped_region.swap(region);
               }
               catch(...){
                  atomic_write32(patomic_word, CorruptedSegment);
                  throw;
               }
               atomic_write32(patomic_word, InitializedSegment);
            }
            else if(previous == InitializingSegment || previous == InitializedSegment){
               throw interprocess_exception(error_info(already_exists_error));
            }
            else{
               throw interprocess_exception(error_info(corrupted_error));
            }
         }
         catch(...){
            try{
               truncate_device<FileBased>(dev, 1u, file_like_t());
            }
            catch(...){
            }
            throw;
         }
      }
      else{
         if(FileBased){
            offset_t filesize = 0;
            spin_wait swait;
            while(filesize == 0){
               if(!get_file_size(file_handle_from_mapping_handle(dev.get_mapping_handle()), filesize)){
                  error_info err = system_error_code();
                  throw interprocess_exception(err);
               }
               swait.yield();
            }
            if(filesize == 1){
               throw interprocess_exception(error_info(corrupted_error));
            }
         }

         mapped_region  region(dev, ronly ? read_only : (cow ? copy_on_write : read_write), 0, 0, addr);

         boost::uint32_t *patomic_word = static_cast<boost::uint32_t*>(region.get_address());
         boost::uint32_t value = atomic_read32(patomic_word);

         spin_wait swait;
         while(value == InitializingSegment || value == UninitializedSegment){
            swait.yield();
            value = atomic_read32(patomic_word);
         }

         if(value != InitializedSegment)
            throw interprocess_exception(error_info(corrupted_error));

         construct_func( static_cast<char*>(region.get_address()) + ManagedOpenOrCreateUserOffset
                        , region.get_size() - ManagedOpenOrCreateUserOffset
                        , false);
         //All ok, just move resources to the external mapped region
         m_mapped_region.swap(region);
      }
      if(StoreDevice){
         this->DevHolder::get_device() = boost::move(dev);
      }
   }

   friend void swap(managed_open_or_create_impl &left, managed_open_or_create_impl &right)
   {
      left.swap(right);
   }

   private:
   friend class interprocess_tester;
   void dont_close_on_destruction()
   {  interprocess_tester::dont_close_on_destruction(m_mapped_region);  }

   mapped_region     m_mapped_region;
};

}  //namespace ipcdetail {

}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_MANAGED_OPEN_OR_CREATE_IMPL
