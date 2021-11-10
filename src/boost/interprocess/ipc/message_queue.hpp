//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_MESSAGE_QUEUE_HPP
#define BOOST_INTERPROCESS_MESSAGE_QUEUE_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/timed_utils.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/move/detail/type_traits.hpp> //make_unsigned, alignment_of
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/assert.hpp>
#include <algorithm> //std::lower_bound
#include <cstddef>   //std::size_t
#include <cstring>   //memcpy


//!\file
//!Describes an inter-process message queue. This class allows sending
//!messages between processes and allows blocking, non-blocking and timed
//!sending and receiving.

namespace boost{  namespace interprocess{

namespace ipcdetail
{
   template<class VoidPointer>
   class msg_queue_initialization_func_t;

}

//Blocking modes
enum mqblock_types   {  blocking,   timed,   non_blocking   };

//!A class that allows sending messages
//!between processes.
template<class VoidPointer>
class message_queue_t
{
   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   message_queue_t();
   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

   public:
   typedef VoidPointer                                                 void_pointer;
   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<char>::type                                    char_ptr;
   typedef typename boost::intrusive::pointer_traits<char_ptr>::difference_type difference_type;
   typedef typename boost::container::dtl::make_unsigned<difference_type>::type        size_type;

   //!Creates a process shared message queue with name "name". For this message queue,
   //!the maximum number of messages will be "max_num_msg" and the maximum message size
   //!will be "max_msg_size". Throws on error and if the queue was previously created.
   message_queue_t(create_only_t create_only,
                 const char *name,
                 size_type max_num_msg,
                 size_type max_msg_size,
                 const permissions &perm = permissions());

   //!Opens or creates a process shared message queue with name "name".
   //!If the queue is created, the maximum number of messages will be "max_num_msg"
   //!and the maximum message size will be "max_msg_size". If queue was previously
   //!created the queue will be opened and "max_num_msg" and "max_msg_size" parameters
   //!are ignored. Throws on error.
   message_queue_t(open_or_create_t open_or_create,
                 const char *name,
                 size_type max_num_msg,
                 size_type max_msg_size,
                 const permissions &perm = permissions());

   //!Opens a previously created process shared message queue with name "name".
   //!If the queue was not previously created or there are no free resources,
   //!throws an error.
   message_queue_t(open_only_t open_only, const char *name);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Creates a process shared message queue with name "name". For this message queue,
   //!the maximum number of messages will be "max_num_msg" and the maximum message size
   //!will be "max_msg_size". Throws on error and if the queue was previously created.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   message_queue_t(create_only_t create_only,
                 const wchar_t *name,
                 size_type max_num_msg,
                 size_type max_msg_size,
                 const permissions &perm = permissions());

   //!Opens or creates a process shared message queue with name "name".
   //!If the queue is created, the maximum number of messages will be "max_num_msg"
   //!and the maximum message size will be "max_msg_size". If queue was previously
   //!created the queue will be opened and "max_num_msg" and "max_msg_size" parameters
   //!are ignored. Throws on error.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   message_queue_t(open_or_create_t open_or_create,
                 const wchar_t *name,
                 size_type max_num_msg,
                 size_type max_msg_size,
                 const permissions &perm = permissions());

   //!Opens a previously created process shared message queue with name "name".
   //!If the queue was not previously created or there are no free resources,
   //!throws an error.
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   message_queue_t(open_only_t open_only, const wchar_t *name);

   #endif //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. All opened message queues are still
   //!valid after destruction. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the message queue from the system
   //!use remove().
   ~message_queue_t();

   //!Sends a message stored in buffer "buffer" with size "buffer_size" in the
   //!message queue with priority "priority". If the message queue is full
   //!the sender is blocked. Throws interprocess_error on error.
   void send (const void *buffer,     size_type buffer_size,
              unsigned int priority);

   //!Sends a message stored in buffer "buffer" with size "buffer_size" through the
   //!message queue with priority "priority". If the message queue is full
   //!the sender is not blocked and returns false, otherwise returns true.
   //!Throws interprocess_error on error.
   bool try_send    (const void *buffer,     size_type buffer_size,
                         unsigned int priority);

   //!Sends a message stored in buffer "buffer" with size "buffer_size" in the
   //!message queue with priority "priority". If the message queue is full
   //!the sender retries until time "abs_time" is reached. Returns true if
   //!the message has been successfully sent. Returns false if timeout is reached.
   //!Throws interprocess_error on error.
   template<class TimePoint>
   bool timed_send    (const void *buffer,     size_type buffer_size,
                           unsigned int priority,  const TimePoint& abs_time);

   //!Receives a message from the message queue. The message is stored in buffer
   //!"buffer", which has size "buffer_size". The received message has size
   //!"recvd_size" and priority "priority". If the message queue is empty
   //!the receiver is blocked. Throws interprocess_error on error.
   void receive (void *buffer,           size_type buffer_size,
                 size_type &recvd_size,unsigned int &priority);

   //!Receives a message from the message queue. The message is stored in buffer
   //!"buffer", which has size "buffer_size". The received message has size
   //!"recvd_size" and priority "priority". If the message queue is empty
   //!the receiver is not blocked and returns false, otherwise returns true.
   //!Throws interprocess_error on error.
   bool try_receive (void *buffer,           size_type buffer_size,
                     size_type &recvd_size,unsigned int &priority);

   //!Receives a message from the message queue. The message is stored in buffer
   //!"buffer", which has size "buffer_size". The received message has size
   //!"recvd_size" and priority "priority". If the message queue is empty
   //!the receiver retries until time "abs_time" is reached. Returns true if
   //!the message has been successfully sent. Returns false if timeout is reached.
   //!Throws interprocess_error on error.
   template<class TimePoint>
   bool timed_receive (void *buffer,           size_type buffer_size,
                       size_type &recvd_size,unsigned int &priority,
                       const TimePoint &abs_time);

   //!Returns the maximum number of messages allowed by the queue. The message
   //!queue must be opened or created previously. Otherwise, returns 0.
   //!Never throws
   size_type get_max_msg() const;

   //!Returns the maximum size of message allowed by the queue. The message
   //!queue must be opened or created previously. Otherwise, returns 0.
   //!Never throws
   size_type get_max_msg_size() const;

   //!Returns the number of messages currently stored.
   //!Never throws
   size_type get_num_msg() const;

   //!Removes the message queue from the system.
   //!Returns false on error. Never throws
   static bool remove(const char *name);

   #if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

   //!Removes the message queue from the system.
   //!Returns false on error. Never throws
   //! 
   //!Note: This function is only available on operating systems with
   //!      native wchar_t APIs (e.g. Windows).
   static bool remove(const wchar_t *name);

   #endif

   #if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)
   private:

   friend class ipcdetail::msg_queue_initialization_func_t<VoidPointer>;

   template<mqblock_types Block, class TimePoint>
   bool do_receive(void *buffer,         size_type buffer_size,
                   size_type &recvd_size, unsigned int &priority,
                   const TimePoint &abs_time);

   template<mqblock_types Block, class TimePoint>
   bool do_send(const void *buffer,      size_type buffer_size,
                unsigned int priority,   const TimePoint &abs_time);

   //!Returns the needed memory size for the shared message queue.
   //!Never throws
   static size_type get_mem_size(size_type max_msg_size, size_type max_num_msg);
   typedef ipcdetail::managed_open_or_create_impl<shared_memory_object, 0, true, false> open_create_impl_t;
   open_create_impl_t m_shmem;

   template<class Lock, class TimePoint>
   static bool do_cond_wait(ipcdetail::bool_<true>, interprocess_condition &cond, Lock &lock, const TimePoint &abs_time)
   {  return cond.timed_wait(lock, abs_time);  }

   template<class Lock, class TimePoint>
   static bool do_cond_wait(ipcdetail::bool_<false>, interprocess_condition &cond, Lock &lock, const TimePoint &)
   {  cond.wait(lock); return true;  }

   #endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED
};

#if !defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

namespace ipcdetail {

//!This header is the prefix of each message in the queue
template<class VoidPointer>
class msg_hdr_t
{
   typedef VoidPointer                                                           void_pointer;
   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<char>::type                                              char_ptr;
   typedef typename boost::intrusive::pointer_traits<char_ptr>::difference_type  difference_type;
   typedef typename boost::container::dtl::make_unsigned<difference_type>::type                  size_type;

   public:
   size_type               len;     // Message length
   unsigned int            priority;// Message priority
   //!Returns the data buffer associated with this this message
   void * data(){ return this+1; }  //
};

//!This functor is the predicate to order stored messages by priority
template<class VoidPointer>
class priority_functor
{
   typedef typename boost::intrusive::
      pointer_traits<VoidPointer>::template
         rebind_pointer<msg_hdr_t<VoidPointer> >::type                  msg_hdr_ptr_t;

   public:
   bool operator()(const msg_hdr_ptr_t &msg1,
                   const msg_hdr_ptr_t &msg2) const
      {  return msg1->priority < msg2->priority;  }
};

//!This header is placed in the beginning of the shared memory and contains
//!the data to control the queue. This class initializes the shared memory
//!in the following way: in ascending memory address with proper alignment
//!fillings:
//!
//!-> mq_hdr_t:
//!   Main control block that controls the rest of the elements
//!
//!-> offset_ptr<msg_hdr_t> index [max_num_msg]
//!   An array of pointers with size "max_num_msg" called index. Each pointer
//!   points to a preallocated message. Elements of this array are
//!   reordered in runtime in the following way:
//!
//!   IF BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX is defined:
//!
//!   When the current number of messages is "cur_num_msg", the array
//!   is treated like a circular buffer. Starting from position "cur_first_msg"
//!   "cur_num_msg" in a circular way, pointers point to inserted messages and the rest
//!   point to free messages. Those "cur_num_msg" pointers are
//!   ordered by the priority of the pointed message and by insertion order
//!   if two messages have the same priority. So the next message to be
//!   used in a "receive" is pointed by index [(cur_first_msg + cur_num_msg-1)%max_num_msg]
//!   and the first free message ready to be used in a "send" operation is
//!   [cur_first_msg] if circular buffer is extended from front,
//!   [(cur_first_msg + cur_num_msg)%max_num_msg] otherwise.
//!
//!   This transforms the index in a circular buffer with an embedded free
//!   message queue.
//!
//!   ELSE (BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX is NOT defined):
//!
//!   When the current number of messages is "cur_num_msg", the first
//!   "cur_num_msg" pointers point to inserted messages and the rest
//!   point to free messages. The first "cur_num_msg" pointers are
//!   ordered by the priority of the pointed message and by insertion order
//!   if two messages have the same priority. So the next message to be
//!   used in a "receive" is pointed by index [cur_num_msg-1] and the first free
//!   message ready to be used in a "send" operation is index [cur_num_msg].
//!
//!   This transforms the index in a fixed size priority queue with an embedded free
//!   message queue.
//!
//!-> struct message_t
//!   {
//!      msg_hdr_t            header;
//!      char[max_msg_size]   data;
//!   } messages [max_num_msg];
//!
//!   An array of buffers of preallocated messages, each one prefixed with the
//!   msg_hdr_t structure. Each of this message is pointed by one pointer of
//!   the index structure.
template<class VoidPointer>
class mq_hdr_t
   : public ipcdetail::priority_functor<VoidPointer>
{
   typedef VoidPointer                                                     void_pointer;
   typedef msg_hdr_t<void_pointer>                                         msg_header;
   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<msg_header>::type                                  msg_hdr_ptr_t;
   typedef typename boost::intrusive::pointer_traits
      <msg_hdr_ptr_t>::difference_type                                     difference_type;
   typedef typename boost::container::
      dtl::make_unsigned<difference_type>::type               size_type;
   typedef typename boost::intrusive::
      pointer_traits<void_pointer>::template
         rebind_pointer<msg_hdr_ptr_t>::type                              msg_hdr_ptr_ptr_t;
   typedef ipcdetail::managed_open_or_create_impl<shared_memory_object, 0, true, false> open_create_impl_t;

   public:
   //!Constructor. This object must be constructed in the beginning of the
   //!shared memory of the size returned by the function "get_mem_size".
   //!This constructor initializes the needed resources and creates
   //!the internal structures like the priority index. This can throw.
   mq_hdr_t(size_type max_num_msg, size_type max_msg_size)
      : m_max_num_msg(max_num_msg),
         m_max_msg_size(max_msg_size),
         m_cur_num_msg(0)
         #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
         ,m_cur_first_msg(0u)
         ,m_blocked_senders(0u)
         ,m_blocked_receivers(0u)
         #endif
      {  this->initialize_memory();  }

   //!Returns true if the message queue is full
   bool is_full() const
      {  return m_cur_num_msg == m_max_num_msg;  }

   //!Returns true if the message queue is empty
   bool is_empty() const
      {  return !m_cur_num_msg;  }

   //!Frees the top priority message and saves it in the free message list
   void free_top_msg()
      {  --m_cur_num_msg;  }

   #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)

   typedef msg_hdr_ptr_t *iterator;

   size_type end_pos() const
   {
      const size_type space_until_bufend = m_max_num_msg - m_cur_first_msg;
      return space_until_bufend > m_cur_num_msg
         ? m_cur_first_msg + m_cur_num_msg : m_cur_num_msg - space_until_bufend;
   }

   //!Returns the inserted message with top priority
   msg_header &top_msg()
   {
      size_type pos = this->end_pos();
      return *mp_index[pos ? --pos : m_max_num_msg - 1];
   }

   //!Returns the inserted message with bottom priority
   msg_header &bottom_msg()
      {  return *mp_index[m_cur_first_msg];   }

   iterator inserted_ptr_begin() const
   {  return &mp_index[m_cur_first_msg]; }

   iterator inserted_ptr_end() const
      {  return &mp_index[this->end_pos()];  }

   iterator lower_bound(const msg_hdr_ptr_t & value, priority_functor<VoidPointer> func)
   {
      iterator begin(this->inserted_ptr_begin()), end(this->inserted_ptr_end());
      if(end < begin){
         iterator idx_end = &mp_index[m_max_num_msg];
         iterator ret = std::lower_bound(begin, idx_end, value, func);
         if(idx_end == ret){
            iterator idx_beg = &mp_index[0];
            ret = std::lower_bound(idx_beg, end, value, func);
            //sanity check, these cases should not call lower_bound (optimized out)
            BOOST_ASSERT(ret != end);
            BOOST_ASSERT(ret != begin);
            return ret;
         }
         else{
            return ret;
         }
      }
      else{
         return std::lower_bound(begin, end, value, func);
      }
   }

   msg_header & insert_at(iterator where)
   {
      iterator it_inserted_ptr_end = this->inserted_ptr_end();
      iterator it_inserted_ptr_beg = this->inserted_ptr_begin();
      if(where == it_inserted_ptr_beg){
         //unsigned integer guarantees underflow
         m_cur_first_msg = m_cur_first_msg ? m_cur_first_msg : m_max_num_msg;
         --m_cur_first_msg;
         ++m_cur_num_msg;
         return *mp_index[m_cur_first_msg];
      }
      else if(where == it_inserted_ptr_end){
         ++m_cur_num_msg;
         return **it_inserted_ptr_end;
      }
      else{
         size_type pos  = where - &mp_index[0];
         size_type circ_pos = pos >= m_cur_first_msg ? pos - m_cur_first_msg : pos + (m_max_num_msg - m_cur_first_msg);
         //Check if it's more efficient to move back or move front
         if(circ_pos < m_cur_num_msg/2){
            //The queue can't be full so m_cur_num_msg == 0 or m_cur_num_msg <= pos
            //indicates two step insertion
            if(!pos){
               pos   = m_max_num_msg;
               where = &mp_index[m_max_num_msg-1];
            }
            else{
               --where;
            }
            const bool unique_segment = m_cur_first_msg && m_cur_first_msg <= pos;
            const size_type first_segment_beg  = unique_segment ? m_cur_first_msg : 1u;
            const size_type first_segment_end  = pos;
            const size_type second_segment_beg = unique_segment || !m_cur_first_msg ? m_max_num_msg : m_cur_first_msg;
            const size_type second_segment_end = m_max_num_msg;
            const msg_hdr_ptr_t backup   = *(&mp_index[0] + (unique_segment ?  first_segment_beg : second_segment_beg) - 1);

            //First segment
            if(!unique_segment){
               std::copy( &mp_index[0] + second_segment_beg
                        , &mp_index[0] + second_segment_end
                        , &mp_index[0] + second_segment_beg - 1);
               mp_index[m_max_num_msg-1] = mp_index[0];
            }
            std::copy( &mp_index[0] + first_segment_beg
                     , &mp_index[0] + first_segment_end
                     , &mp_index[0] + first_segment_beg - 1);
            *where = backup;
            m_cur_first_msg = m_cur_first_msg ? m_cur_first_msg : m_max_num_msg;
            --m_cur_first_msg;
            ++m_cur_num_msg;
            return **where;
         }
         else{
            //The queue can't be full so end_pos < m_cur_first_msg
            //indicates two step insertion
            const size_type pos_end = this->end_pos();
            const bool unique_segment = pos < pos_end;
            const size_type first_segment_beg  = pos;
            const size_type first_segment_end  = unique_segment  ? pos_end : m_max_num_msg-1;
            const size_type second_segment_beg = 0u;
            const size_type second_segment_end = unique_segment ? 0u : pos_end;
            const msg_hdr_ptr_t backup   = *it_inserted_ptr_end;

            //First segment
            if(!unique_segment){
               std::copy_backward( &mp_index[0] + second_segment_beg
                                 , &mp_index[0] + second_segment_end
                                 , &mp_index[0] + second_segment_end + 1);
               mp_index[0] = mp_index[m_max_num_msg-1];
            }
            std::copy_backward( &mp_index[0] + first_segment_beg
                              , &mp_index[0] + first_segment_end
                              , &mp_index[0] + first_segment_end + 1);
            *where = backup;
            ++m_cur_num_msg;
            return **where;
         }
      }
   }

   #else //BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX

   typedef msg_hdr_ptr_t *iterator;

   //!Returns the inserted message with top priority
   msg_header &top_msg()
      {  return *mp_index[m_cur_num_msg-1];   }

   //!Returns the inserted message with bottom priority
   msg_header &bottom_msg()
      {  return *mp_index[0];   }

   iterator inserted_ptr_begin() const
   {  return &mp_index[0]; }

   iterator inserted_ptr_end() const
   {  return &mp_index[m_cur_num_msg]; }

   iterator lower_bound(const msg_hdr_ptr_t & value, priority_functor<VoidPointer> func)
   {  return std::lower_bound(this->inserted_ptr_begin(), this->inserted_ptr_end(), value, func);  }

   msg_header & insert_at(iterator pos)
   {
      const msg_hdr_ptr_t backup = *inserted_ptr_end();
      std::copy_backward(pos, inserted_ptr_end(), inserted_ptr_end()+1);
      *pos = backup;
      ++m_cur_num_msg;
      return **pos;
   }

   #endif   //BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX

   //!Inserts the first free message in the priority queue
   msg_header & queue_free_msg(unsigned int priority)
   {
      //Get priority queue's range
      iterator it  (inserted_ptr_begin()), it_end(inserted_ptr_end());
      //Optimize for non-priority usage
      if(m_cur_num_msg && priority > this->bottom_msg().priority){
         //Check for higher priority than all stored messages
         if(priority > this->top_msg().priority){
            it = it_end;
         }
         else{
            //Since we don't now which free message we will pick
            //build a dummy header for searches
            msg_header dummy_hdr;
            dummy_hdr.priority = priority;

            //Get free msg
            msg_hdr_ptr_t dummy_ptr(&dummy_hdr);

            //Check where the free message should be placed
            it = this->lower_bound(dummy_ptr, static_cast<priority_functor<VoidPointer>&>(*this));
         }
      }
      //Insert the free message in the correct position
      return this->insert_at(it);
   }

   //!Returns the number of bytes needed to construct a message queue with
   //!"max_num_size" maximum number of messages and "max_msg_size" maximum
   //!message size. Never throws.
   static size_type get_mem_size
      (size_type max_msg_size, size_type max_num_msg)
   {
      const size_type
       msg_hdr_align  = ::boost::container::dtl::alignment_of<msg_header>::value,
       index_align    = ::boost::container::dtl::alignment_of<msg_hdr_ptr_t>::value,
         r_hdr_size     = ipcdetail::ct_rounded_size<sizeof(mq_hdr_t), index_align>::value,
         r_index_size   = ipcdetail::get_rounded_size<size_type>(max_num_msg*sizeof(msg_hdr_ptr_t), msg_hdr_align),
         r_max_msg_size = ipcdetail::get_rounded_size<size_type>(max_msg_size, msg_hdr_align) + sizeof(msg_header);
      return r_hdr_size + r_index_size + (max_num_msg*r_max_msg_size) +
         open_create_impl_t::ManagedOpenOrCreateUserOffset;
   }

   //!Initializes the memory structures to preallocate messages and constructs the
   //!message index. Never throws.
   void initialize_memory()
   {
      const size_type
        msg_hdr_align  = ::boost::container::dtl::alignment_of<msg_header>::value,
        index_align    = ::boost::container::dtl::alignment_of<msg_hdr_ptr_t>::value,
         r_hdr_size     = ipcdetail::ct_rounded_size<sizeof(mq_hdr_t), index_align>::value,
         r_index_size   = ipcdetail::get_rounded_size<size_type>(m_max_num_msg*sizeof(msg_hdr_ptr_t), msg_hdr_align),
         r_max_msg_size = ipcdetail::get_rounded_size<size_type>(m_max_msg_size, msg_hdr_align) + sizeof(msg_header);

      //Pointer to the index
      msg_hdr_ptr_t *index =  reinterpret_cast<msg_hdr_ptr_t*>
                                 (reinterpret_cast<char*>(this)+r_hdr_size);

      //Pointer to the first message header
      msg_header *msg_hdr   =  reinterpret_cast<msg_header*>
                                 (reinterpret_cast<char*>(this)+r_hdr_size+r_index_size);

      //Initialize the pointer to the index
      mp_index             = index;

      //Initialize the index so each slot points to a preallocated message
      for(size_type i = 0; i < m_max_num_msg; ++i){
         index[i] = msg_hdr;
         msg_hdr  = reinterpret_cast<msg_header*>
                        (reinterpret_cast<char*>(msg_hdr)+r_max_msg_size);
      }
   }

   public:
   //Pointer to the index
   msg_hdr_ptr_ptr_t          mp_index;
   //Maximum number of messages of the queue
   const size_type            m_max_num_msg;
   //Maximum size of messages of the queue
   const size_type            m_max_msg_size;
   //Current number of messages
   size_type                  m_cur_num_msg;
   //Mutex to protect data structures
   interprocess_mutex         m_mutex;
   //Condition block receivers when there are no messages
   interprocess_condition     m_cond_recv;
   //Condition block senders when the queue is full
   interprocess_condition     m_cond_send;
   #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
   //Current start offset in the circular index
   size_type                  m_cur_first_msg;
   size_type                  m_blocked_senders;
   size_type                  m_blocked_receivers;
   #endif
};


//!This is the atomic functor to be executed when creating or opening
//!shared memory. Never throws
template<class VoidPointer>
class msg_queue_initialization_func_t
{
   public:
   typedef typename boost::intrusive::
      pointer_traits<VoidPointer>::template
         rebind_pointer<char>::type                               char_ptr;
   typedef typename boost::intrusive::pointer_traits<char_ptr>::
      difference_type                                             difference_type;
   typedef typename boost::container::dtl::
      make_unsigned<difference_type>::type                        size_type;

   msg_queue_initialization_func_t(size_type maxmsg = 0,
                         size_type maxmsgsize = 0)
      : m_maxmsg (maxmsg), m_maxmsgsize(maxmsgsize) {}

   bool operator()(void *address, size_type, bool created)
   {
      char      *mptr;

      if(created){
         mptr     = reinterpret_cast<char*>(address);
         //Construct the message queue header at the beginning
         BOOST_TRY{
            new (mptr) mq_hdr_t<VoidPointer>(m_maxmsg, m_maxmsgsize);
         }
         BOOST_CATCH(...){
            return false;
         }
         BOOST_CATCH_END
      }
      return true;
   }

   std::size_t get_min_size() const
   {
      return mq_hdr_t<VoidPointer>::get_mem_size(m_maxmsgsize, m_maxmsg)
      - message_queue_t<VoidPointer>::open_create_impl_t::ManagedOpenOrCreateUserOffset;
   }

   const size_type m_maxmsg;
   const size_type m_maxmsgsize;
};

}  //namespace ipcdetail {

template<class VoidPointer>
inline message_queue_t<VoidPointer>::~message_queue_t()
{}

template<class VoidPointer>
inline typename message_queue_t<VoidPointer>::size_type message_queue_t<VoidPointer>::get_mem_size
   (size_type max_msg_size, size_type max_num_msg)
{  return ipcdetail::mq_hdr_t<VoidPointer>::get_mem_size(max_msg_size, max_num_msg);   }

template<class VoidPointer>
inline message_queue_t<VoidPointer>::message_queue_t(create_only_t,
                                    const char *name,
                                    size_type max_num_msg,
                                    size_type max_msg_size,
                                    const permissions &perm)
      //Create shared memory and execute functor atomically
   :  m_shmem(create_only,
              name,
              get_mem_size(max_msg_size, max_num_msg),
              read_write,
              static_cast<void*>(0),
              //Prepare initialization functor
              ipcdetail::msg_queue_initialization_func_t<VoidPointer> (max_num_msg, max_msg_size),
              perm)
{}

template<class VoidPointer>
inline message_queue_t<VoidPointer>::message_queue_t(open_or_create_t,
                                    const char *name,
                                    size_type max_num_msg,
                                    size_type max_msg_size,
                                    const permissions &perm)
      //Create shared memory and execute functor atomically
   :  m_shmem(open_or_create,
              name,
              get_mem_size(max_msg_size, max_num_msg),
              read_write,
              static_cast<void*>(0),
              //Prepare initialization functor
              ipcdetail::msg_queue_initialization_func_t<VoidPointer> (max_num_msg, max_msg_size),
              perm)
{}

template<class VoidPointer>
inline message_queue_t<VoidPointer>::message_queue_t(open_only_t, const char *name)
   //Create shared memory and execute functor atomically
   :  m_shmem(open_only,
              name,
              read_write,
              static_cast<void*>(0),
              //Prepare initialization functor
              ipcdetail::msg_queue_initialization_func_t<VoidPointer> ())
{}

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

template<class VoidPointer>
inline message_queue_t<VoidPointer>::message_queue_t(create_only_t,
                                    const wchar_t *name,
                                    size_type max_num_msg,
                                    size_type max_msg_size,
                                    const permissions &perm)
      //Create shared memory and execute functor atomically
   :  m_shmem(create_only,
              name,
              get_mem_size(max_msg_size, max_num_msg),
              read_write,
              static_cast<void*>(0),
              //Prepare initialization functor
              ipcdetail::msg_queue_initialization_func_t<VoidPointer> (max_num_msg, max_msg_size),
              perm)
{}

template<class VoidPointer>
inline message_queue_t<VoidPointer>::message_queue_t(open_or_create_t,
                                    const wchar_t *name,
                                    size_type max_num_msg,
                                    size_type max_msg_size,
                                    const permissions &perm)
      //Create shared memory and execute functor atomically
   :  m_shmem(open_or_create,
              name,
              get_mem_size(max_msg_size, max_num_msg),
              read_write,
              static_cast<void*>(0),
              //Prepare initialization functor
              ipcdetail::msg_queue_initialization_func_t<VoidPointer> (max_num_msg, max_msg_size),
              perm)
{}

template<class VoidPointer>
inline message_queue_t<VoidPointer>::message_queue_t(open_only_t, const wchar_t *name)
   //Create shared memory and execute functor atomically
   :  m_shmem(open_only,
              name,
              read_write,
              static_cast<void*>(0),
              //Prepare initialization functor
              ipcdetail::msg_queue_initialization_func_t<VoidPointer> ())
{}

#endif //defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

template<class VoidPointer>
inline void message_queue_t<VoidPointer>::send
   (const void *buffer, size_type buffer_size, unsigned int priority)
{  this->do_send<blocking>(buffer, buffer_size, priority, 0); }

template<class VoidPointer>
inline bool message_queue_t<VoidPointer>::try_send
   (const void *buffer, size_type buffer_size, unsigned int priority)
{  return this->do_send<non_blocking>(buffer, buffer_size, priority, 0); }

template<class VoidPointer>
template<class TimePoint>
inline bool message_queue_t<VoidPointer>::timed_send
   (const void *buffer, size_type buffer_size
   ,unsigned int priority, const TimePoint &abs_time)
{
   if(ipcdetail::is_pos_infinity(abs_time)){
      this->send(buffer, buffer_size, priority);
      return true;
   }
   return this->do_send<timed>(buffer, buffer_size, priority, abs_time);
}

template<class VoidPointer>
template<mqblock_types Block, class TimePoint>
inline bool message_queue_t<VoidPointer>::do_send(
                                const void *buffer,      size_type buffer_size,
                                unsigned int priority,   const TimePoint &abs_time)
{
   ipcdetail::mq_hdr_t<VoidPointer> *p_hdr = static_cast<ipcdetail::mq_hdr_t<VoidPointer>*>(m_shmem.get_user_address());
   //Check if buffer is smaller than maximum allowed
   if (buffer_size > p_hdr->m_max_msg_size) {
      throw interprocess_exception(size_error);
   }

   #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
   bool notify_blocked_receivers = false;
   #endif
   //---------------------------------------------
   scoped_lock<interprocess_mutex> lock(p_hdr->m_mutex);
   //---------------------------------------------
   {
      //If the queue is full execute blocking logic
      if (p_hdr->is_full()) {
         BOOST_TRY{
            #ifdef BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX
            ++p_hdr->m_blocked_senders;
            #endif
            switch(Block){
               case non_blocking :
                  #ifdef BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX
                  --p_hdr->m_blocked_senders;
                  #endif
                  return false;
               break;

               case blocking :
                  do{
                     (void)do_cond_wait(ipcdetail::bool_<false>(), p_hdr->m_cond_send, lock, abs_time);
                  }
                  while (p_hdr->is_full());
               break;

               case timed :
                  do{
                     if(!do_cond_wait(ipcdetail::bool_<Block == timed>(), p_hdr->m_cond_send, lock, abs_time)) {
                        if(p_hdr->is_full()){
                           #ifdef BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX
                           --p_hdr->m_blocked_senders;
                           #endif
                           return false;
                        }
                        break;
                     }
                  }
                  while (p_hdr->is_full());
               break;
               default:
               break;
            }
            #ifdef BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX
            --p_hdr->m_blocked_senders;
            #endif
         }
         BOOST_CATCH(...){
            #ifdef BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX
            --p_hdr->m_blocked_senders;
            #endif
            BOOST_RETHROW;
         }
         BOOST_CATCH_END
      }

      #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
      notify_blocked_receivers = 0 != p_hdr->m_blocked_receivers;
      #endif
      //Insert the first free message in the priority queue
      ipcdetail::msg_hdr_t<VoidPointer> &free_msg_hdr = p_hdr->queue_free_msg(priority);

      //Sanity check, free msgs are always cleaned when received
      BOOST_ASSERT(free_msg_hdr.priority == 0);
      BOOST_ASSERT(free_msg_hdr.len == 0);

      //Copy control data to the free message
      free_msg_hdr.priority = priority;
      free_msg_hdr.len      = buffer_size;

      //Copy user buffer to the message
      std::memcpy(free_msg_hdr.data(), buffer, buffer_size);
   }  // Lock end

   //Notify outside lock to avoid contention. This might produce some
   //spurious wakeups, but it's usually far better than notifying inside.
   //If this message changes the queue empty state, notify it to receivers
   #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
   if (notify_blocked_receivers){
      p_hdr->m_cond_recv.notify_one();
   }
   #else
   p_hdr->m_cond_recv.notify_one();
   #endif

   return true;
}

template<class VoidPointer>
inline void message_queue_t<VoidPointer>::receive(void *buffer,        size_type buffer_size,
                        size_type &recvd_size,   unsigned int &priority)
{  this->do_receive<blocking>(buffer, buffer_size, recvd_size, priority, 0); }

template<class VoidPointer>
inline bool
   message_queue_t<VoidPointer>::try_receive(void *buffer,              size_type buffer_size,
                              size_type &recvd_size,   unsigned int &priority)
{  return this->do_receive<non_blocking>(buffer, buffer_size, recvd_size, priority, 0); }

template<class VoidPointer>
template<class TimePoint>
inline bool
   message_queue_t<VoidPointer>::timed_receive(void *buffer,            size_type buffer_size,
                                size_type &recvd_size,   unsigned int &priority,
                                const TimePoint &abs_time)
{
   if(ipcdetail::is_pos_infinity(abs_time)){
      this->receive(buffer, buffer_size, recvd_size, priority);
      return true;
   }
   return this->do_receive<timed>(buffer, buffer_size, recvd_size, priority, abs_time);
}

template<class VoidPointer>
template<mqblock_types Block, class TimePoint>
inline bool
   message_queue_t<VoidPointer>::do_receive(
                          void *buffer,            size_type buffer_size,
                          size_type &recvd_size,   unsigned int &priority,
                          const TimePoint &abs_time)
{
   ipcdetail::mq_hdr_t<VoidPointer> *p_hdr = static_cast<ipcdetail::mq_hdr_t<VoidPointer>*>(m_shmem.get_user_address());
   //Check if buffer is big enough for any message
   if (buffer_size < p_hdr->m_max_msg_size) {
      throw interprocess_exception(size_error);
   }

   #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
   bool notify_blocked_senders = false;
   #endif
   //---------------------------------------------
   scoped_lock<interprocess_mutex> lock(p_hdr->m_mutex);
   //---------------------------------------------
   {
      //If there are no messages execute blocking logic
      if (p_hdr->is_empty()) {
         BOOST_TRY{
            #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
            ++p_hdr->m_blocked_receivers;
            #endif
            switch(Block){
               case non_blocking :
                  #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
                  --p_hdr->m_blocked_receivers;
                  #endif
                  return false;
               break;

               case blocking :
                  do{
                     (void)do_cond_wait(ipcdetail::bool_<false>(), p_hdr->m_cond_recv, lock, abs_time);
                  }
                  while (p_hdr->is_empty());
               break;

               case timed :
                  do{
                     if(!do_cond_wait(ipcdetail::bool_<Block == timed>(), p_hdr->m_cond_recv, lock, abs_time)) {
                        if(p_hdr->is_empty()){
                           #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
                           --p_hdr->m_blocked_receivers;
                           #endif
                           return false;
                        }
                        break;
                     }
                  }
                  while (p_hdr->is_empty());
               break;

               //Paranoia check
               default:
               break;
            }
            #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
            --p_hdr->m_blocked_receivers;
            #endif
         }
         BOOST_CATCH(...){
            #if defined(BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX)
            --p_hdr->m_blocked_receivers;
            #endif
            BOOST_RETHROW;
         }
         BOOST_CATCH_END
      }

      #ifdef BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX
      notify_blocked_senders = 0 != p_hdr->m_blocked_senders;
      #endif

      //There is at least one message ready to pick, get the top one
      ipcdetail::msg_hdr_t<VoidPointer> &top_msg = p_hdr->top_msg();

      //Get data from the message
      recvd_size     = top_msg.len;
      priority       = top_msg.priority;

      //Some cleanup to ease debugging
      top_msg.len       = 0;
      top_msg.priority  = 0;

      //Copy data to receiver's bufers
      std::memcpy(buffer, top_msg.data(), recvd_size);

      //Free top message and put it in the free message list
      p_hdr->free_top_msg();
   }  //Lock end

   //Notify outside lock to avoid contention. This might produce some
   //spurious wakeups, but it's usually far better than notifying inside.
   //If this reception changes the queue full state, notify senders
   #ifdef BOOST_INTERPROCESS_MSG_QUEUE_CIRCULAR_INDEX
   if (notify_blocked_senders){
      p_hdr->m_cond_send.notify_one();
   }
   #else
   p_hdr->m_cond_send.notify_one();
   #endif

   return true;
}

template<class VoidPointer>
inline typename message_queue_t<VoidPointer>::size_type message_queue_t<VoidPointer>::get_max_msg() const
{
   ipcdetail::mq_hdr_t<VoidPointer> *p_hdr = static_cast<ipcdetail::mq_hdr_t<VoidPointer>*>(m_shmem.get_user_address());
   return p_hdr ? p_hdr->m_max_num_msg : 0;  }

template<class VoidPointer>
inline typename message_queue_t<VoidPointer>::size_type message_queue_t<VoidPointer>::get_max_msg_size() const
{
   ipcdetail::mq_hdr_t<VoidPointer> *p_hdr = static_cast<ipcdetail::mq_hdr_t<VoidPointer>*>(m_shmem.get_user_address());
   return p_hdr ? p_hdr->m_max_msg_size : 0;
}

template<class VoidPointer>
inline typename message_queue_t<VoidPointer>::size_type message_queue_t<VoidPointer>::get_num_msg() const
{
   ipcdetail::mq_hdr_t<VoidPointer> *p_hdr = static_cast<ipcdetail::mq_hdr_t<VoidPointer>*>(m_shmem.get_user_address());
   if(p_hdr){
      //---------------------------------------------
      scoped_lock<interprocess_mutex> lock(p_hdr->m_mutex);
      //---------------------------------------------
      return p_hdr->m_cur_num_msg;
   }

   return 0;
}

template<class VoidPointer>
inline bool message_queue_t<VoidPointer>::remove(const char *name)
{  return shared_memory_object::remove(name);  }

#if defined(BOOST_INTERPROCESS_WCHAR_NAMED_RESOURCES) || defined(BOOST_INTERPROCESS_DOXYGEN_INVOKED)

template<class VoidPointer>
inline bool message_queue_t<VoidPointer>::remove(const wchar_t *name)
{  return shared_memory_object::remove(name);  }

#endif

#else

//!Typedef for a default message queue
//!to be used between processes
typedef message_queue_t<offset_ptr<void> > message_queue;

#endif   //#ifndef BOOST_INTERPROCESS_DOXYGEN_INVOKED

}} //namespace boost{  namespace interprocess{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_MESSAGE_QUEUE_HPP
