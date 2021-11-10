#ifndef BOOST_STATECHART_EVENT_PROCESSOR_INCLUDED
#define BOOST_STATECHART_EVENT_PROCESSOR_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



namespace boost
{
namespace statechart
{


  
class event_base;



//////////////////////////////////////////////////////////////////////////////
template< class Scheduler >
class event_processor
{
  public:
    //////////////////////////////////////////////////////////////////////////
    virtual ~event_processor() {}

    Scheduler & my_scheduler() const
    {
      return myScheduler_;
    }

    typedef typename Scheduler::processor_handle processor_handle;

    processor_handle my_handle() const
    {
      return myHandle_;
    }

    void initiate()
    {
      initiate_impl();
    }

    void process_event( const event_base & evt )
    {
      process_event_impl( evt );
    }

    void terminate()
    {
      terminate_impl();
    }

  protected:
    //////////////////////////////////////////////////////////////////////////
    typedef const typename Scheduler::processor_context & my_context;

    event_processor( my_context ctx ) :
      myScheduler_( ctx.my_scheduler() ),
      myHandle_( ctx.my_handle() )
    {
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    virtual void initiate_impl() = 0;
    virtual void process_event_impl( const event_base & evt ) = 0;
    virtual void terminate_impl() = 0;

    // avoids C4512 (assignment operator could not be generated)
    event_processor & operator=( const event_processor & );

    Scheduler & myScheduler_;
    const processor_handle myHandle_;
};



} // namespace statechart
} // namespace boost



#endif
