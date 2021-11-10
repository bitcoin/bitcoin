//  Copyright (c) 2001 Daniel C. Nuffer
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ISTREAM_POLICY_JAN_04_2010_0130PM)
#define BOOST_SPIRIT_ISTREAM_POLICY_JAN_04_2010_0130PM

#include <boost/spirit/home/support/iterators/multi_pass_fwd.hpp>
#include <boost/spirit/home/support/iterators/detail/multi_pass.hpp>

namespace boost { namespace spirit { namespace iterator_policies
{
    ///////////////////////////////////////////////////////////////////////////
    //  class istream
    //  Implementation of the InputPolicy used by multi_pass
    // 
    //  The istream encapsulates an std::basic_istream
    ///////////////////////////////////////////////////////////////////////////
    struct istream
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        class unique // : public detail::default_input_policy
        {
        private:
            typedef typename T::char_type result_type;

        public:
            typedef typename T::off_type difference_type;
            typedef typename T::off_type distance_type;
            typedef result_type const* pointer;
            typedef result_type const& reference;
            typedef result_type value_type;

        protected:
            unique() {}
            explicit unique(T&) {}

            void swap(unique&) {}

        public:
            template <typename MultiPass>
            static void destroy(MultiPass&) {}

            template <typename MultiPass>
            static typename MultiPass::reference get_input(MultiPass& mp)
            {
                if (!mp.shared()->initialized_)
                    mp.shared()->read_one();
                return mp.shared()->curtok_;
            }

            template <typename MultiPass>
            static void advance_input(MultiPass& mp)
            {
                // We invalidate the currently cached input character to avoid
                // reading more input from the underlying iterator than 
                // required. Without this we would always read ahead one 
                // character, even if this character never gets consumed by the 
                // client.
                mp.shared()->peek_one();
            }

            // test, whether we reached the end of the underlying stream
            template <typename MultiPass>
            static bool input_at_eof(MultiPass const& mp) 
            {
                return mp.shared()->eof_reached_;
            }

            template <typename MultiPass>
            static bool input_is_valid(MultiPass const& mp, value_type const&) 
            {
                return mp.shared()->initialized_;
            }

            // no unique data elements
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        struct shared
        {
        private:
            typedef typename T::char_type result_type;

        public:
            explicit shared(T& input) 
              : input_(input), curtok_(-1)
              , initialized_(false), eof_reached_(false) 
            {
                peek_one();   // istreams may be at eof right in the beginning
            }

            void read_one()
            {
                if (!(input_ >> curtok_)) {
                    initialized_ = false;
                    eof_reached_ = true;
                }
                else {
                    initialized_ = true;
                }
            }

            void peek_one()
            {
                input_.peek();    // try for eof
                initialized_ = false;
                eof_reached_ = input_.eof();
            }

            T& input_;
            result_type curtok_;
            bool initialized_;
            bool eof_reached_;
        };
    };

}}}

#endif
