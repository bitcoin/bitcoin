//////////////////////////////////////////////////////////////////////////////
// (c) Copyright Andreas Huber Doenni 2002-2005, Eric Niebler 2006
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_XPRESSIVE_DETAIL_UTILITY_COUNTED_BASE_HPP_EAN_04_16_2006
#define BOOST_XPRESSIVE_DETAIL_UTILITY_COUNTED_BASE_HPP_EAN_04_16_2006

#include <boost/assert.hpp>
#include <boost/checked_delete.hpp>
#include <boost/detail/atomic_count.hpp>

namespace boost { namespace xpressive { namespace detail
{
    template<typename Derived>
    struct counted_base_access;

    //////////////////////////////////////////////////////////////////////////////
    // counted_base
    template<typename Derived>
    struct counted_base
    {
        long use_count() const
        {
            return this->count_;
        }

    protected:
        counted_base()
          : count_(0)
        {
        }

        counted_base(counted_base<Derived> const &)
          : count_(0)
        {
        }

        counted_base &operator =(counted_base<Derived> const &)
        {
            return *this;
        }

    private:
        friend struct counted_base_access<Derived>;
        mutable boost::detail::atomic_count count_;
    };

    //////////////////////////////////////////////////////////////////////////////
    // counted_base_access
    template<typename Derived>
    struct counted_base_access
    {
        static void add_ref(counted_base<Derived> const *that)
        {
            ++that->count_;
        }

        static void release(counted_base<Derived> const *that)
        {
            BOOST_ASSERT(0 < that->count_);
            if(0 == --that->count_)
            {
                boost::checked_delete(static_cast<Derived const *>(that));
            }
        }
    };

    template<typename Derived>
    inline void intrusive_ptr_add_ref(counted_base<Derived> const *that)
    {
        counted_base_access<Derived>::add_ref(that);
    }

    template<typename Derived>
    inline void intrusive_ptr_release(counted_base<Derived> const *that)
    {
        counted_base_access<Derived>::release(that);
    }

}}} // namespace boost::xpressive::detail

#endif
