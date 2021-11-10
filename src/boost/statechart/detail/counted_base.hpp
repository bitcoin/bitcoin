#ifndef BOOST_STATECHART_DETAIL_COUNTED_BASE_HPP_INCLUDED
#define BOOST_STATECHART_DETAIL_COUNTED_BASE_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/detail/atomic_count.hpp>
#include <boost/config.hpp> // BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP



namespace boost
{
namespace statechart
{
namespace detail
{


  
template< bool NeedsLocking >
struct count_base
{
  count_base() : count_( 0 ) {}
  mutable boost::detail::atomic_count count_;
};

template<>
struct count_base< false >
{
  count_base() : count_( 0 ) {}
  mutable long count_;
};

//////////////////////////////////////////////////////////////////////////////
template< bool NeedsLocking = true >
class counted_base : private count_base< NeedsLocking >
{
  typedef count_base< NeedsLocking > base_type;
  public:
    //////////////////////////////////////////////////////////////////////////
    bool ref_counted() const
    {
      return base_type::count_ != 0;
    }

    long ref_count() const
    {
      return base_type::count_;
    }

  protected:
    //////////////////////////////////////////////////////////////////////////
    counted_base() {}
    ~counted_base() {}

    // do nothing copy implementation is intentional (the number of
    // referencing pointers of the source and the destination is not changed
    // through the copy operation)
    counted_base( const counted_base & ) : base_type() {}
    counted_base & operator=( const counted_base & ) { return *this; }

  public:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only public because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    void add_ref() const
    {
      ++base_type::count_;
    }

    bool release() const
    {
      BOOST_ASSERT( base_type::count_ > 0 );
      return --base_type::count_ == 0;
    }
};



} // namespace detail
} // namespace statechart
} // namespace boost



#endif
