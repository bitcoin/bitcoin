//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ISTREAM_ITERATOR_JAN_03_2010_0522PM)
#define BOOST_SPIRIT_ISTREAM_ITERATOR_JAN_03_2010_0522PM

#include <boost/spirit/home/support/iterators/detail/ref_counted_policy.hpp>
#if defined(BOOST_SPIRIT_DEBUG)
#include <boost/spirit/home/support/iterators/detail/buf_id_check_policy.hpp>
#else
#include <boost/spirit/home/support/iterators/detail/no_check_policy.hpp>
#endif
#include <boost/spirit/home/support/iterators/detail/istream_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/split_std_deque_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/combine_policies.hpp>
#include <boost/spirit/home/support/iterators/multi_pass.hpp>

namespace boost { namespace spirit 
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elem, typename Traits = std::char_traits<Elem> >
    class basic_istream_iterator :
        public multi_pass<
            std::basic_istream<Elem, Traits>
          , iterator_policies::default_policy<
                iterator_policies::ref_counted
#if defined(BOOST_SPIRIT_DEBUG)
              , iterator_policies::buf_id_check
#else
              , iterator_policies::no_check
#endif
              , iterator_policies::istream
              , iterator_policies::split_std_deque> 
        >
    {
    private:
        typedef multi_pass<
            std::basic_istream<Elem, Traits>
          , iterator_policies::default_policy<
                iterator_policies::ref_counted
#if defined(BOOST_SPIRIT_DEBUG)
              , iterator_policies::buf_id_check
#else
              , iterator_policies::no_check
#endif
              , iterator_policies::istream
              , iterator_policies::split_std_deque> 
        > base_type;

    public:
        basic_istream_iterator()
          : base_type() {}

        explicit basic_istream_iterator(std::basic_istream<Elem, Traits>& x)
          : base_type(x) {}

#if BOOST_WORKAROUND(__GLIBCPP__, == 20020514)
        basic_istream_iterator(int)   // workaround for a bug in the library
          : base_type() {}            // shipped with gcc 3.1
#endif // BOOST_WORKAROUND(__GLIBCPP__, == 20020514)

        basic_istream_iterator operator= (base_type const& rhs)
        {
            this->base_type::operator=(rhs);
            return *this;
        }

    // default generated operators, destructor and assignment operator are ok.
    };

    typedef basic_istream_iterator<char> istream_iterator;

}}

#endif
