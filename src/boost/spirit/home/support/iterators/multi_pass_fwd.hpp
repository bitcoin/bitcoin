/*=============================================================================
    Copyright (c) 2007 Tobias Schwinger
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ITERATOR_MULTI_PASS_FWD_APR_18_2008_1102AM)
#define BOOST_SPIRIT_ITERATOR_MULTI_PASS_FWD_APR_18_2008_1102AM

#include <cstddef>
#include <boost/spirit/home/support/multi_pass_wrapper.hpp>
#include <boost/swap.hpp>

namespace boost { namespace spirit {

    namespace iterator_policies
    {
        // input policies
        struct input_iterator;
        struct buffering_input_iterator;
        struct istream;
        struct lex_input;
        struct functor_input;
        struct split_functor_input;

        // ownership policies
        struct ref_counted;
        struct first_owner;

        // checking policies
        class illegal_backtracking;
        struct buf_id_check;
        struct no_check;

        // storage policies
        struct split_std_deque;
        template<std::size_t N> struct fixed_size_queue;

        // policy combiner
#if defined(BOOST_SPIRIT_DEBUG)
        template<typename Ownership = ref_counted
          , typename Checking = buf_id_check
          , typename Input = buffering_input_iterator
          , typename Storage = split_std_deque>
        struct default_policy;
#else
        template<typename Ownership = ref_counted
          , typename Checking = no_check
          , typename Input = buffering_input_iterator
          , typename Storage = split_std_deque>
        struct default_policy;
#endif
    }

    template <typename T
      , typename Policies = iterator_policies::default_policy<> >
    class multi_pass;

    template <typename T, typename Policies>
    void swap(multi_pass<T, Policies> &x, multi_pass<T, Policies> &y);

}} // namespace boost::spirit

namespace boost { namespace spirit { namespace traits
{
    // declare special functions allowing to integrate any multi_pass iterator
    // with expectation points

    // multi_pass iterators require special handling (for the non-specialized
    // versions of these functions see support/multi_pass_wrapper.hpp)
    template <typename T, typename Policies>
    void clear_queue(multi_pass<T, Policies>&
      , BOOST_SCOPED_ENUM(clear_mode) mode = clear_mode::clear_if_enabled);

    template <typename T, typename Policies>
    void inhibit_clear_queue(multi_pass<T, Policies>&, bool);

    template <typename T, typename Policies>
    bool inhibit_clear_queue(multi_pass<T, Policies>&);

    // Helper template to recognize a multi_pass iterator. This specialization
    // will be instantiated for any multi_pass iterator.
    template <typename T, typename Policies>
    struct is_multi_pass<multi_pass<T, Policies> > : mpl::true_ {};

}}}

#endif

