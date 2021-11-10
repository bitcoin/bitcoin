#ifndef BOOST_STATECHART_FIFO_SCHEDULER_HPP_INCLUDED
#define BOOST_STATECHART_FIFO_SCHEDULER_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/event_base.hpp>
#include <boost/statechart/fifo_worker.hpp>
#include <boost/statechart/processor_container.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/config.hpp> // BOOST_HAS_THREADS



namespace boost
{
namespace statechart
{



//////////////////////////////////////////////////////////////////////////////
template<
  class FifoWorker = fifo_worker<>,
  class Allocator = std::allocator< none > >
class fifo_scheduler : noncopyable
{
  typedef processor_container<
    fifo_scheduler, typename FifoWorker::work_item, Allocator > container;
  public:
    //////////////////////////////////////////////////////////////////////////
    #ifdef BOOST_HAS_THREADS
    fifo_scheduler( bool waitOnEmptyQueue = false ) :
      worker_( waitOnEmptyQueue )
    {
    }
    #endif

    typedef typename container::processor_handle processor_handle;
    typedef typename container::processor_context processor_context;

    template< class Processor >
    processor_handle create_processor()
    {
      processor_handle result;
      work_item item =
        container_.template create_processor< Processor >( result, *this );
      worker_.queue_work_item( item );
      return result;
    }

    template< class Processor, typename Arg1 >
    processor_handle create_processor( Arg1 arg1 )
    {
      processor_handle result;
      work_item item = container_.template create_processor< Processor >(
        result, *this, arg1 );
      worker_.queue_work_item( item );
      return result;
    }

    template< class Processor, typename Arg1, typename Arg2 >
    processor_handle create_processor( Arg1 arg1, Arg2 arg2 )
    {
      processor_handle result;
      work_item item = container_.template create_processor< Processor >(
        result, *this, arg1, arg2 );
      worker_.queue_work_item( item );
      return result;
    }

    template< class Processor, typename Arg1, typename Arg2, typename Arg3 >
    processor_handle create_processor( Arg1 arg1, Arg2 arg2, Arg3 arg3 )
    {
      processor_handle result;
      work_item item = container_.template create_processor< Processor >(
        result, *this, arg1, arg2, arg3 );
      worker_.queue_work_item( item );
      return result;
    }

    template<
      class Processor, typename Arg1, typename Arg2,
      typename Arg3, typename Arg4 >
    processor_handle create_processor(
      Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4 )
    {
      processor_handle result;
      work_item item = container_.template create_processor< Processor >(
        result, *this, arg1, arg2, arg3, arg4 );
      worker_.queue_work_item( item );
      return result;
    }

    template<
      class Processor, typename Arg1, typename Arg2,
      typename Arg3, typename Arg4, typename Arg5 >
    processor_handle create_processor(
      Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5 )
    {
      processor_handle result;
      work_item item = container_.template create_processor< Processor >(
        result, *this, arg1, arg2, arg3, arg4, arg5 );
      worker_.queue_work_item( item );
      return result;
    }

    template<
      class Processor, typename Arg1, typename Arg2,
      typename Arg3, typename Arg4, typename Arg5, typename Arg6 >
    processor_handle create_processor(
      Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6 )
    {
      processor_handle result;
      work_item item = container_.template create_processor< Processor >(
        result, *this, arg1, arg2, arg3, arg4, arg5, arg6 );
      worker_.queue_work_item( item );
      return result;
    }

    void destroy_processor( const processor_handle & processor )
    {
      work_item item = container_.destroy_processor( processor );
      worker_.queue_work_item( item );
    }

    void initiate_processor( const processor_handle & processor )
    {
      work_item item = container_.initiate_processor( processor );
      worker_.queue_work_item( item );
    }

    void terminate_processor( const processor_handle & processor )
    {
      work_item item = container_.terminate_processor( processor );
      worker_.queue_work_item( item );
    }

    typedef intrusive_ptr< const event_base > event_ptr_type;

    void queue_event(
      const processor_handle & processor, const event_ptr_type & pEvent )
    {
      work_item item = container_.queue_event( processor, pEvent );
      worker_.queue_work_item( item );
    }

    typedef typename FifoWorker::work_item work_item;

    // We take a non-const reference so that we can move (i.e. swap) the item
    // into the queue, what avoids copying the (possibly heap-allocated)
    // implementation object inside work_item.
    void queue_work_item( work_item & item )
    {
      worker_.queue_work_item( item );
    }

    // Convenience overload so that temporary objects can be passed directly
    // instead of having to create a work_item object first. Under most
    // circumstances, this will lead to one unnecessary copy of the
    // function implementation object.
    void queue_work_item( const work_item & item )
    {
      worker_.queue_work_item( item );
    }

    void terminate()
    {
      worker_.terminate();
    }

    // Is not mutex-protected! Must only be called from the thread that also
    // calls operator().
    bool terminated() const
    {
      return worker_.terminated();
    }

    unsigned long operator()( unsigned long maxEventCount = 0 )
    {
      return worker_( maxEventCount );
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    container container_;
    FifoWorker worker_;
};



} // namespace statechart
} // namespace boost



#endif
