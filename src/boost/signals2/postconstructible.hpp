// DEPRECATED in favor of adl_postconstruct with deconstruct<T>().
// A simple framework for creating objects with postconstructors.
// The objects must inherit from boost::signals2::postconstructible, and
// have their lifetimes managed by
// boost::shared_ptr created with the boost::signals2::deconstruct_ptr()
// function.
//
// Copyright Frank Mori Hess 2007-2008.
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SIGNALS2_POSTCONSTRUCTIBLE_HPP
#define BOOST_SIGNALS2_POSTCONSTRUCTIBLE_HPP

namespace boost
{
  template<typename T> class shared_ptr;

  namespace signals2
  {
    namespace postconstructible_adl_barrier
    {
      class postconstructible;
    }
    namespace detail
    {
      void do_postconstruct(const boost::signals2::postconstructible_adl_barrier::postconstructible *ptr);
    } // namespace detail

    namespace postconstructible_adl_barrier
    {
      class postconstructible
      {
      public:
        friend void detail::do_postconstruct(const postconstructible *ptr);
        template<typename T>
          friend void adl_postconstruct(const shared_ptr<T> &, postconstructible *p)
        {
          p->postconstruct();
        }
      protected:
        postconstructible() {}
        virtual ~postconstructible() {}
        virtual void postconstruct() = 0;
      };
    } // namespace postconstructible_adl_barrier
    using postconstructible_adl_barrier::postconstructible;

  }
}

#endif // BOOST_SIGNALS2_POSTCONSTRUCTIBLE_HPP
