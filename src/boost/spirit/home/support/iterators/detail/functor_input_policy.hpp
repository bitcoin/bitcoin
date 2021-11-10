//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_SPLIT_FUNCTOR_INPUT_POLICY_JAN_16_2008_0448M)
#define BOOST_SPIRIT_ITERATOR_SPLIT_FUNCTOR_INPUT_POLICY_JAN_16_2008_0448M

#include <boost/spirit/home/support/iterators/multi_pass_fwd.hpp>
#include <boost/spirit/home/support/iterators/detail/multi_pass.hpp>
#include <boost/assert.hpp>

namespace boost { namespace spirit { namespace iterator_policies
{
    namespace is_valid_test_
    {
        template <typename Token>
        inline bool token_is_valid(Token const&)
        {
            return true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //  class functor_input
    //  Implementation of the InputPolicy used by multi_pass
    //  functor_input gets tokens from a functor
    // 
    //  Note: the functor must have a typedef for result_type
    //        It also must have a static variable of type result_type defined 
    //        to represent EOF that is called eof.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct functor_input
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename Functor>
        class unique : public detail::default_input_policy
        {
        private:
            typedef typename Functor::result_type result_type;

        protected:
            unique() {}
            explicit unique(Functor const& x) : ftor(x) {}

            void swap(unique& x)
            {
                boost::swap(ftor, x.ftor);
            }

        public:
            typedef result_type value_type;
            typedef std::ptrdiff_t difference_type;
            typedef std::ptrdiff_t distance_type;
            typedef result_type* pointer;
            typedef result_type& reference;

        public:
            // get the next token
            template <typename MultiPass>
            static typename MultiPass::reference get_input(MultiPass& mp)
            {
                value_type& curtok = mp.shared()->curtok;
                if (!input_is_valid(mp, curtok))
                    curtok = mp.ftor();
                return curtok;
            }

            template <typename MultiPass>
            static void advance_input(MultiPass& mp)
            {
                // if mp.shared is NULL then this instance of the multi_pass 
                // represents a end iterator
                BOOST_ASSERT(0 != mp.shared());
                mp.shared()->curtok = mp.ftor();
            }

            // test, whether we reached the end of the underlying stream
            template <typename MultiPass>
            static bool input_at_eof(MultiPass const& mp) 
            {
                return mp.shared()->curtok == mp.ftor.eof;
            }

            template <typename MultiPass>
            static bool input_is_valid(MultiPass const&, value_type const& t) 
            {
                using namespace is_valid_test_;
                return token_is_valid(t);
            }

            Functor& get_functor() const
            {
                return ftor;
            }

        protected:
            mutable Functor ftor;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Functor>
        struct shared
        {
            explicit shared(Functor const&) : curtok(0) {}

            typename Functor::result_type curtok;
        };
    };

}}}

#endif
