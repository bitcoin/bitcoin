//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_SPLIT_FUNCTOR_INPUT_POLICY_JAN_17_2008_0103PM)
#define BOOST_SPIRIT_ITERATOR_SPLIT_FUNCTOR_INPUT_POLICY_JAN_17_2008_0103PM

#include <boost/spirit/home/support/iterators/multi_pass_fwd.hpp>
#include <boost/spirit/home/support/iterators/detail/multi_pass.hpp>
#include <boost/assert.hpp>
#include <boost/type_traits/is_empty.hpp>

namespace boost { namespace spirit { namespace iterator_policies
{
    namespace split_functor_input_is_valid_test_
    {
        template <typename Token>
        inline bool token_is_valid(Token const&)
        {
            return true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //  class split_functor_input
    //  Implementation of the InputPolicy used by multi_pass
    //  split_functor_input gets tokens from a functor
    // 
    //  This policy should be used when the functor holds two parts of data: a
    //  unique part (unique for each instance of the iterator) and a shared 
    //  part (to be shared between the different copies of the same iterator).
    //  Using this policy allows to merge the shared part of the functor with 
    //  the shared part of the iterator data, saving one pointer and one 
    //  allocation per iterator instance.
    //
    //  The Functor template parameter of this policy is expected to be a 
    //  std::pair<unique, shared>, where 'unique' and 'shared' represent the
    //  respective parts of the functor itself.
    //
    //  Note: the unique part of the functor must have a typedef for result_type
    //        It also must have a static variable of type result_type defined 
    //        to represent EOF that is called eof.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct split_functor_input
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename Functor
          , bool FunctorIsEmpty = is_empty<typename Functor::first_type>::value>
        class unique;

        // the unique part of the functor is empty, do not include the functor 
        // as a member at all to avoid unnecessary padding bytes to be included 
        // into the generated structure
        template <typename Functor>
        class unique<Functor, true> // : public detail::default_input_policy
        {
        protected:
            typedef typename Functor::first_type functor_type;
            typedef typename functor_type::result_type result_type;

        public:
            typedef result_type value_type;
            typedef std::ptrdiff_t difference_type;
            typedef std::ptrdiff_t distance_type;
            typedef result_type const* pointer;
            typedef result_type const& reference;

        protected:
            unique() {}
            explicit unique(Functor const&) {}

        public:
            void swap(unique&) {}

            // get the next token
            template <typename MultiPass>
            static typename MultiPass::reference get_input(MultiPass& mp)
            {
                value_type& curtok = mp.shared()->curtok;
                using namespace split_functor_input_is_valid_test_;
                if (!token_is_valid(curtok))
                    functor_type::get_next(mp, curtok);
                return curtok;
            }

            template <typename MultiPass>
            static void advance_input(MultiPass& mp)
            {
                functor_type::get_next(mp, mp.shared()->curtok);
            }

            // test, whether we reached the end of the underlying stream
            template <typename MultiPass>
            static bool input_at_eof(MultiPass const& mp) 
            {
                return mp.shared()->curtok == functor_type::eof;
            }

            template <typename MultiPass>
            static bool input_is_valid(MultiPass const&, value_type const& t) 
            {
                using namespace split_functor_input_is_valid_test_;
                return token_is_valid(t);
            }

            template <typename MultiPass>
            static void destroy(MultiPass& mp) 
            {
                functor_type::destroy(mp);
            }
        };

        // the unique part of the functor is non-empty
        template <typename Functor>
        class unique<Functor, false> : public unique<Functor, true>
        {
        protected:
            typedef typename Functor::first_type functor_type;
            typedef typename functor_type::result_type result_type;

        protected:
            unique() {}
            explicit unique(Functor const& x) : ftor(x.first) {}

            void swap(unique& x)
            {
                boost::swap(ftor, x.ftor);
            }

        public:
            typedef result_type value_type;
            typedef std::ptrdiff_t difference_type;
            typedef std::ptrdiff_t distance_type;
            typedef result_type const* pointer;
            typedef result_type const& reference;

        public:
            // get the next token
            template <typename MultiPass>
            static typename MultiPass::reference get_input(MultiPass& mp)
            {
                value_type& curtok = mp.shared()->curtok;
                using namespace split_functor_input_is_valid_test_;
                if (!token_is_valid(curtok))
                    functor_type::get_next(mp, curtok);
                return curtok;
            }

            template <typename MultiPass>
            static void advance_input(MultiPass& mp)
            {
                mp.ftor.get_next(mp, mp.shared()->curtok);
            }

            template <typename MultiPass>
            static bool input_is_valid(MultiPass const&, value_type const& t) 
            {
                using namespace split_functor_input_is_valid_test_;
                return token_is_valid(t);
            }

            // test, whether we reached the end of the underlying stream
            template <typename MultiPass>
            static bool input_at_eof(MultiPass const& mp) 
            {
                return mp.shared()->curtok == mp.ftor.eof;
            }

            typename Functor::first_type& get_functor() const
            {
                return ftor;
            }

            mutable functor_type ftor;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Functor>
        struct shared
        {
        protected:
            typedef typename Functor::first_type functor_type;
            typedef typename functor_type::result_type result_type;

        public:
            explicit shared(Functor const& x) : ftor(x.second), curtok(0) {}

            mutable typename Functor::second_type ftor;
            result_type curtok;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(shared& operator= (shared const&))
        };
    };

}}}

#endif
