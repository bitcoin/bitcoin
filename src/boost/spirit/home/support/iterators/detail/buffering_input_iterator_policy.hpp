//  Copyright (c) 2001 Daniel C. Nuffer
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_BUFFERING_ITERATOR_INPUT_ITERATOR_POLICY_MAR_04_2010_1224AM)
#define BOOST_SPIRIT_BUFFERING_ITERATOR_INPUT_ITERATOR_POLICY_MAR_04_2010_1224AM

#include <boost/spirit/home/support/iterators/multi_pass_fwd.hpp>
#include <boost/spirit/home/support/iterators/detail/multi_pass.hpp>
#include <boost/spirit/home/support/iterators/detail/input_iterator_policy.hpp>
#include <boost/assert.hpp>
#include <iterator> // for std::iterator_traits

namespace boost { namespace spirit { namespace iterator_policies
{
    ///////////////////////////////////////////////////////////////////////////
    //  class input_iterator
    //
    //  Implementation of the InputPolicy used by multi_pass, this is different 
    //  from the input_iterator policy only as it is buffering the last input
    //  character to allow returning it by reference. This is needed for
    //  wrapping iterators not buffering the last item (such as the 
    //  std::istreambuf_iterator). Unfortunately there is no way to 
    //  automatically figure this out at compile time.
    // 
    //  The buffering_input_iterator encapsulates an input iterator of type T
    ///////////////////////////////////////////////////////////////////////////
    struct buffering_input_iterator
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        class unique // : public detail::default_input_policy
        {
        private:
            typedef
                typename std::iterator_traits<T>::value_type
            result_type;

        public:
            typedef
                typename std::iterator_traits<T>::difference_type
            difference_type;
            typedef
                typename std::iterator_traits<T>::difference_type
            distance_type;
            typedef
                typename std::iterator_traits<T>::pointer
            pointer;
            typedef result_type& reference;
            typedef result_type value_type;

        protected:
            unique() {}
            explicit unique(T) {}

            void swap(unique&) {}

        public:
            template <typename MultiPass>
            static void destroy(MultiPass&) {}

            template <typename MultiPass>
            static typename MultiPass::reference get_input(MultiPass& mp)
            {
                return mp.shared()->get_input();
            }

            template <typename MultiPass>
            static void advance_input(MultiPass& mp)
            {
                BOOST_ASSERT(0 != mp.shared());
                mp.shared()->advance_input();
            }

            // test, whether we reached the end of the underlying stream
            template <typename MultiPass>
            static bool input_at_eof(MultiPass const& mp) 
            {
                static T const end_iter;
                return mp.shared()->input_ == end_iter;
            }

            template <typename MultiPass>
            static bool input_is_valid(MultiPass const& mp, value_type const&)
            {
                return mp.shared()->input_is_valid_;
            }

            // no unique data elements
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        struct shared
        {
            typedef
                typename std::iterator_traits<T>::value_type
            result_type;

            explicit shared(T const& input) 
              : input_(input), curtok_(0), input_is_valid_(false) {}

            void advance_input()
            {
                ++input_;
                input_is_valid_ = false;
            }

            result_type& get_input()
            {
                if (!input_is_valid_) {
                    curtok_ = *input_;
                    input_is_valid_ = true;
                }
                return curtok_;
            }

            T input_;
            result_type curtok_;
            bool input_is_valid_;
        };
    };

}}}

#endif
