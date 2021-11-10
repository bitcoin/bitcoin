//  Copyright (c) 2001, Daniel C. Nuffer
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_BUF_ID_CHECK_POLICY_MAR_16_2007_1108AM)
#define BOOST_SPIRIT_ITERATOR_BUF_ID_CHECK_POLICY_MAR_16_2007_1108AM

#include <boost/spirit/home/support/iterators/multi_pass_fwd.hpp>
#include <boost/spirit/home/support/iterators/detail/multi_pass.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <exception>    // for std::exception

namespace boost { namespace spirit { namespace iterator_policies
{
    ///////////////////////////////////////////////////////////////////////////
    //  class illegal_backtracking
    //  thrown by buf_id_check CheckingPolicy if an instance of an iterator is
    //  used after another one has invalidated the queue
    ///////////////////////////////////////////////////////////////////////////
    class BOOST_SYMBOL_VISIBLE illegal_backtracking : public std::exception
    {
    public:
        illegal_backtracking() BOOST_NOEXCEPT_OR_NOTHROW {}
        ~illegal_backtracking() BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE {}

        char const* what() const BOOST_NOEXCEPT_OR_NOTHROW BOOST_OVERRIDE
        { 
            return "boost::spirit::multi_pass::illegal_backtracking"; 
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    //  class buf_id_check
    //  Implementation of the CheckingPolicy used by multi_pass
    //  This policy is most effective when used together with the std_deque
    //  StoragePolicy.
    // 
    //  If used with the fixed_size_queue StoragePolicy, it will not detect
    //  iterator dereferences that are out of the range of the queue.
    ///////////////////////////////////////////////////////////////////////////////
    struct buf_id_check
    {
        ///////////////////////////////////////////////////////////////////////
        struct unique //: detail::default_checking_policy
        {
            unique() : buf_id(0) {}
            unique(unique const& x) : buf_id(x.buf_id) {}

            void swap(unique& x)
            {
                boost::swap(buf_id, x.buf_id);
            }

            // called to verify that everything is ok.
            template <typename MultiPass>
            static void docheck(MultiPass const& mp) 
            {
                if (mp.buf_id != mp.shared()->shared_buf_id)
                    boost::throw_exception(illegal_backtracking());
            }

            // called from multi_pass::clear_queue, so we can increment the count
            template <typename MultiPass>
            static void clear_queue(MultiPass& mp)
            {
                ++mp.shared()->shared_buf_id;
                ++mp.buf_id;
            }

            template <typename MultiPass>
            static void destroy(MultiPass&) {}

        protected:
            unsigned long buf_id;
        };

        ///////////////////////////////////////////////////////////////////////
        struct shared
        {
            shared() : shared_buf_id(0) {}
            unsigned long shared_buf_id;
        };
    };

}}}

#endif
