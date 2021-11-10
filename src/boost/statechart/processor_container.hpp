#ifndef BOOST_STATECHART_PROCESSOR_CONTAINER_HPP_INCLUDED
#define BOOST_STATECHART_PROCESSOR_CONTAINER_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/event_base.hpp>
#include <boost/statechart/event_processor.hpp>

#include <boost/assert.hpp>
#include <boost/ref.hpp>
#include <boost/noncopyable.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/config.hpp> // BOOST_INTEL

#include <boost/detail/workaround.hpp>
#include <boost/detail/allocator_utilities.hpp>

#include <set>
#include <memory>   // std::allocator, std::unique_ptr



namespace boost
{
namespace statechart
{
namespace detail
{
  template<bool IsReferenceWrapper>
  struct unwrap_impl
  {
    template< typename T >
    struct apply { typedef T type; };
  };

  template<>
  struct unwrap_impl<true>
  {
    template< typename T >
    struct apply { typedef typename T::type & type; };
  };

  template<typename T>
  struct unwrap
  {
    typedef typename unwrap_impl<
      is_reference_wrapper< T >::value >::template apply< T >::type type;
  };
}


template<
  class Scheduler,
  class WorkItem,
  class Allocator = std::allocator< none > >
class processor_container : noncopyable
{
  typedef event_processor< Scheduler > processor_base_type;
#ifdef BOOST_NO_AUTO_PTR
  typedef std::unique_ptr< processor_base_type > processor_holder_type;
#else
  typedef std::auto_ptr< processor_base_type > processor_holder_type;
#endif
  typedef shared_ptr< processor_holder_type > processor_holder_ptr_type;

  public:
    //////////////////////////////////////////////////////////////////////////
    typedef weak_ptr< processor_holder_type > processor_handle;

    class processor_context
    {
        processor_context(
          Scheduler & scheduler, const processor_handle & handle
        ) :
          scheduler_( scheduler ),
          handle_( handle )
        {
        }

      #if BOOST_WORKAROUND( BOOST_INTEL, BOOST_TESTED_AT( 800 ) )
      public:
      // for some reason Intel 8.0 seems to think that the following functions
      // are inaccessible from event_processor<>::event_processor
      #endif

        Scheduler & my_scheduler() const { return scheduler_; }
        const processor_handle & my_handle() const { return handle_; }

      #if BOOST_WORKAROUND( BOOST_INTEL, BOOST_TESTED_AT( 800 ) )
      private:
      #endif

        // avoids C4512 (assignment operator could not be generated)
        processor_context & operator=( const processor_context & );

        Scheduler & scheduler_;
        const processor_handle handle_;

        friend class processor_container;
        friend class event_processor< Scheduler >;
    };

    template< class Processor >
    WorkItem create_processor( processor_handle & handle, Scheduler & scheduler )
    {
      processor_holder_ptr_type pProcessor = make_processor_holder();
      handle = pProcessor;
      typedef void ( processor_container::*impl_fun_ptr )(
        const processor_holder_ptr_type &, const processor_context & );
      impl_fun_ptr pImpl =
        &processor_container::template create_processor_impl0< Processor >;
      return WorkItem(
        boost::bind( pImpl, this, pProcessor,
          processor_context( scheduler, handle ) ),
        Allocator() );
    }

    template< class Processor, typename Arg1 >
    WorkItem create_processor(
      processor_handle & handle, Scheduler & scheduler, Arg1 arg1 )
    {
      processor_holder_ptr_type pProcessor = make_processor_holder();
      handle = pProcessor;
      typedef typename detail::unwrap< Arg1 >::type arg1_type;
      typedef void ( processor_container::*impl_fun_ptr )(
        const processor_holder_ptr_type &, const processor_context &,
        arg1_type );
      impl_fun_ptr pImpl =
        &processor_container::template create_processor_impl1<
          Processor, arg1_type >;
      return WorkItem(
        boost::bind( pImpl, this, pProcessor, processor_context( scheduler, handle ),
          arg1 ),
        Allocator() );
    }

    template< class Processor, typename Arg1, typename Arg2 >
    WorkItem create_processor(
      processor_handle & handle, Scheduler & scheduler, Arg1 arg1, Arg2 arg2 )
    {
      processor_holder_ptr_type pProcessor = make_processor_holder();
      handle = pProcessor;
      typedef typename detail::unwrap< Arg1 >::type arg1_type;
      typedef typename detail::unwrap< Arg2 >::type arg2_type;
      typedef void ( processor_container::*impl_fun_ptr )(
        const processor_holder_ptr_type &, const processor_context &,
         arg1_type, arg2_type );
      impl_fun_ptr pImpl =
        &processor_container::template create_processor_impl2<
          Processor, arg1_type, arg2_type >;
      return WorkItem(
        boost::bind( pImpl, this, pProcessor, processor_context( scheduler, handle ),
          arg1, arg2 ),
        Allocator() );
    }

    template< class Processor, typename Arg1, typename Arg2, typename Arg3 >
    WorkItem create_processor(
      processor_handle & handle, Scheduler & scheduler,
      Arg1 arg1, Arg2 arg2, Arg3 arg3 )
    {
      processor_holder_ptr_type pProcessor = make_processor_holder();
      handle = pProcessor;
      typedef typename detail::unwrap< Arg1 >::type arg1_type;
      typedef typename detail::unwrap< Arg2 >::type arg2_type;
      typedef typename detail::unwrap< Arg3 >::type arg3_type;
      typedef void ( processor_container::*impl_fun_ptr )(
        const processor_holder_ptr_type &, const processor_context &,
        arg1_type, arg2_type, arg3_type );
      impl_fun_ptr pImpl =
        &processor_container::template create_processor_impl3<
          Processor, arg1_type, arg2_type, arg3_type >;
      return WorkItem(
        boost::bind( pImpl, this, pProcessor, processor_context( scheduler, handle ),
          arg1, arg2, arg3 ),
        Allocator() );
    }

    template<
      class Processor, typename Arg1, typename Arg2,
      typename Arg3, typename Arg4 >
    WorkItem create_processor(
      processor_handle & handle, Scheduler & scheduler,
      Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4 )
    {
      processor_holder_ptr_type pProcessor = make_processor_holder();
      handle = pProcessor;
      typedef typename detail::unwrap< Arg1 >::type arg1_type;
      typedef typename detail::unwrap< Arg2 >::type arg2_type;
      typedef typename detail::unwrap< Arg3 >::type arg3_type;
      typedef typename detail::unwrap< Arg4 >::type arg4_type;
      typedef void ( processor_container::*impl_fun_ptr )(
        const processor_holder_ptr_type &, const processor_context &,
        arg1_type, arg2_type, arg3_type, arg4_type );
      impl_fun_ptr pImpl =
        &processor_container::template create_processor_impl4<
          Processor, arg1_type, arg2_type, arg3_type, arg4_type >;
      return WorkItem(
        boost::bind( pImpl, this, pProcessor, processor_context( scheduler, handle ),
          arg1, arg2, arg3, arg4 ),
        Allocator() );
    }

    template<
      class Processor, typename Arg1, typename Arg2,
      typename Arg3, typename Arg4, typename Arg5 >
    WorkItem create_processor(
      processor_handle & handle, Scheduler & scheduler,
      Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5 )
    {
      processor_holder_ptr_type pProcessor = make_processor_holder();
      handle = pProcessor;
      typedef typename detail::unwrap< Arg1 >::type arg1_type;
      typedef typename detail::unwrap< Arg2 >::type arg2_type;
      typedef typename detail::unwrap< Arg3 >::type arg3_type;
      typedef typename detail::unwrap< Arg4 >::type arg4_type;
      typedef typename detail::unwrap< Arg5 >::type arg5_type;
      typedef void ( processor_container::*impl_fun_ptr )(
        const processor_holder_ptr_type &, const processor_context &,
        arg1_type, arg2_type, arg3_type, arg4_type, arg5_type );
      impl_fun_ptr pImpl =
        &processor_container::template create_processor_impl5<
          Processor, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type >;
      return WorkItem(
        boost::bind( pImpl, this, pProcessor, processor_context( scheduler, handle ),
          arg1, arg2, arg3, arg4, arg5 ),
        Allocator() );
    }

    template<
      class Processor, typename Arg1, typename Arg2,
      typename Arg3, typename Arg4, typename Arg5, typename Arg6 >
    WorkItem create_processor(
      processor_handle & handle, Scheduler & scheduler,
      Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6 )
    {
      processor_holder_ptr_type pProcessor = make_processor_holder();
      handle = pProcessor;
      typedef typename detail::unwrap< Arg1 >::type arg1_type;
      typedef typename detail::unwrap< Arg2 >::type arg2_type;
      typedef typename detail::unwrap< Arg3 >::type arg3_type;
      typedef typename detail::unwrap< Arg4 >::type arg4_type;
      typedef typename detail::unwrap< Arg5 >::type arg5_type;
      typedef typename detail::unwrap< Arg6 >::type arg6_type;
      typedef void ( processor_container::*impl_fun_ptr )(
        const processor_holder_ptr_type &, const processor_context &,
        arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type );
      impl_fun_ptr pImpl =
        &processor_container::template create_processor_impl6<
          Processor,
          arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type >;
      return WorkItem(
        boost::bind( pImpl, this, pProcessor, processor_context( scheduler, handle ),
          arg1, arg2, arg3, arg4, arg5, arg6 ),
        Allocator() );
    }

    WorkItem destroy_processor( const processor_handle & processor )
    {
      return WorkItem(
        boost::bind( &processor_container::destroy_processor_impl, this, processor ),
        Allocator() );
    }

    WorkItem initiate_processor( const processor_handle & processor )
    {
      return WorkItem(
        boost::bind( &processor_container::initiate_processor_impl, this,
          processor ),
        Allocator() );
    }

    WorkItem terminate_processor( const processor_handle & processor )
    {
      return WorkItem(
        boost::bind( &processor_container::terminate_processor_impl, this,
          processor ),
        Allocator() );
    }

    typedef intrusive_ptr< const event_base > event_ptr_type;

    WorkItem queue_event(
      const processor_handle & processor, const event_ptr_type & pEvent )
    {
      BOOST_ASSERT( pEvent.get() != 0 );

      return WorkItem(
        boost::bind( &processor_container::queue_event_impl, this, processor,
          pEvent ),
        Allocator() );
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    processor_holder_ptr_type make_processor_holder()
    {
      return processor_holder_ptr_type( new processor_holder_type() );
    }

    template< class Processor >
    void create_processor_impl0(
      const processor_holder_ptr_type & pProcessor,
      const processor_context & context )
    {
      processorSet_.insert( pProcessor );
      *pProcessor = processor_holder_type( new Processor( context ) );
    }

    template< class Processor, typename Arg1 >
    void create_processor_impl1(
      const processor_holder_ptr_type & pProcessor,
      const processor_context & context, Arg1 arg1 )
    {
      processorSet_.insert( pProcessor );
      *pProcessor = processor_holder_type( new Processor( context, arg1 ) );
    }

    template< class Processor, typename Arg1, typename Arg2 >
    void create_processor_impl2(
      const processor_holder_ptr_type & pProcessor,
      const processor_context & context, Arg1 arg1, Arg2 arg2 )
    {
      processorSet_.insert( pProcessor );
      *pProcessor = processor_holder_type( new Processor( context, arg1, arg2 ) );
    }

    template< class Processor, typename Arg1, typename Arg2, typename Arg3 >
    void create_processor_impl3(
      const processor_holder_ptr_type & pProcessor,
      const processor_context & context, Arg1 arg1, Arg2 arg2, Arg3 arg3 )
    {
      processorSet_.insert( pProcessor );
      *pProcessor = processor_holder_type( new Processor( context, arg1, arg2, arg3 ) );
    }

    template<
      class Processor, typename Arg1, typename Arg2,
      typename Arg3, typename Arg4 >
    void create_processor_impl4(
      const processor_holder_ptr_type & pProcessor,
      const processor_context & context,
      Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4 )
    {
      processorSet_.insert( pProcessor );
      *pProcessor = processor_holder_type( new Processor( context, arg1, arg2, arg3, arg4 ) );
    }

    template<
      class Processor, typename Arg1, typename Arg2,
      typename Arg3, typename Arg4, typename Arg5 >
    void create_processor_impl5(
      const processor_holder_ptr_type & pProcessor,
      const processor_context & context,
      Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5 )
    {
      processorSet_.insert( pProcessor );
      *pProcessor = processor_holder_type( new Processor( context, arg1, arg2, arg3, arg4, arg5 ) );
    }

    template<
      class Processor, typename Arg1, typename Arg2,
      typename Arg3, typename Arg4, typename Arg5, typename Arg6 >
    void create_processor_impl6(
      const processor_holder_ptr_type & pProcessor,
      const processor_context & context,
      Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6 )
    {
      processorSet_.insert( pProcessor );
      *pProcessor = processor_holder_type( new Processor( context, arg1, arg2, arg3, arg4, arg5, arg6 ) );
    }

    void destroy_processor_impl( const processor_handle & processor )
    {
      const processor_holder_ptr_type pProcessor = processor.lock();

      if ( pProcessor != 0 )
      {
        processorSet_.erase( pProcessor );
      }
    }

    void initiate_processor_impl( const processor_handle & processor )
    {
      const processor_holder_ptr_type pProcessor = processor.lock();

      if ( pProcessor != 0 )
      {
        ( *pProcessor )->initiate();
      }
    }

    void terminate_processor_impl( const processor_handle & processor )
    {
      const processor_holder_ptr_type pProcessor = processor.lock();

      if ( pProcessor != 0 )
      {
        ( *pProcessor )->terminate();
      }
    }

    void queue_event_impl(
      const processor_handle & processor, const event_ptr_type & pEvent )
    {
      const processor_holder_ptr_type pProcessor = processor.lock();

      if ( pProcessor != 0 )
      {
        ( *pProcessor )->process_event( *pEvent );
      }
    }

    typedef std::set< 
      processor_holder_ptr_type, 
      std::less< processor_holder_ptr_type >,
      typename boost::detail::allocator::rebind_to<
        Allocator, processor_holder_ptr_type >::type
    > event_processor_set_type;

    event_processor_set_type processorSet_;
};


} // namespace statechart
} // namespace boost



#endif
