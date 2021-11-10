//  Copyright (c) 2001-2012 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_COMBINE_POLICIES_APR_06_2008_0136PM)
#define BOOST_SPIRIT_ITERATOR_COMBINE_POLICIES_APR_06_2008_0136PM

#include <boost/config.hpp>
#include <boost/type_traits/is_empty.hpp>

namespace boost { namespace spirit { namespace iterator_policies
{
    ///////////////////////////////////////////////////////////////////////////
    //  The purpose of the multi_pass_unique template is to eliminate
    //  empty policy classes (policies not containing any data items) from the
    //  multiple inheritance chain. This is necessary since some compilers
    //  fail to apply the empty base optimization if multiple inheritance is
    //  involved.
    //  Additionally this can be used to combine separate policies into one
    //  single multi_pass_policy as required by the multi_pass template
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    // select the correct derived classes based on if a policy is empty
    template <typename T
      , typename Ownership, typename Checking, typename Input, typename Storage
      , bool OwnershipIsEmpty = boost::is_empty<Ownership>::value
      , bool CheckingIsEmpty = boost::is_empty<Checking>::value
      , bool InputIsEmpty = boost::is_empty<Input>::value>
    struct multi_pass_unique;

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Ownership, typename Checking
      , typename Input, typename Storage>
    struct multi_pass_unique<T, Ownership, Checking, Input, Storage
          , false, false, false>
      : Ownership, Checking, Input, Storage
    {
        multi_pass_unique() {}
        multi_pass_unique(T& x) : Input(x) {}
        multi_pass_unique(T const& x) : Input(x) {}

        template <typename MultiPass>
        static void destroy(MultiPass& mp)
        {
            Ownership::destroy(mp);
            Checking::destroy(mp);
            Input::destroy(mp);
            Storage::destroy(mp);
        }

        void swap(multi_pass_unique& x)
        {
            this->Ownership::swap(x);
            this->Checking::swap(x);
            this->Input::swap(x);
            this->Storage::swap(x);
        }

        template <typename MultiPass>
        inline static void clear_queue(MultiPass& mp)
        {
            Checking::clear_queue(mp);
            Storage::clear_queue(mp);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Ownership, typename Checking
      , typename Input, typename Storage>
    struct multi_pass_unique<T, Ownership, Checking, Input, Storage
          , false, false, true>
      : Ownership, Checking, Storage
    {
        multi_pass_unique() {}
        multi_pass_unique(T const&) {}

        template <typename MultiPass>
        static void destroy(MultiPass& mp)
        {
            Ownership::destroy(mp);
            Checking::destroy(mp);
            Input::destroy(mp);
            Storage::destroy(mp);
        }

        void swap(multi_pass_unique& x)
        {
            this->Ownership::swap(x);
            this->Checking::swap(x);
            this->Storage::swap(x);
        }

        template <typename MultiPass>
        inline static void clear_queue(MultiPass& mp)
        {
            Checking::clear_queue(mp);
            Storage::clear_queue(mp);
        }

        // implement input policy functions by forwarding to the Input type
        template <typename MultiPass>
        inline static void advance_input(MultiPass& mp)
            { Input::advance_input(mp); }

        template <typename MultiPass>
        inline static typename MultiPass::reference get_input(MultiPass& mp)
            { return Input::get_input(mp); }

        template <typename MultiPass>
        inline static bool input_at_eof(MultiPass const& mp)
            { return Input::input_at_eof(mp); }

        template <typename MultiPass, typename TokenType>
        inline static bool input_is_valid(MultiPass& mp, TokenType& curtok)
            { return Input::input_is_valid(mp, curtok); }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Ownership, typename Checking
      , typename Input, typename Storage>
    struct multi_pass_unique<T, Ownership, Checking, Input, Storage
          , false, true, false>
      : Ownership, Input, Storage
    {
        multi_pass_unique() {}
        multi_pass_unique(T& x) : Input(x) {}
        multi_pass_unique(T const& x) : Input(x) {}

        template <typename MultiPass>
        static void destroy(MultiPass& mp)
        {
            Ownership::destroy(mp);
            Input::destroy(mp);
            Storage::destroy(mp);
        }

        void swap(multi_pass_unique& x)
        {
            this->Ownership::swap(x);
            this->Input::swap(x);
            this->Storage::swap(x);
        }

        template <typename MultiPass>
        inline static void clear_queue(MultiPass& mp)
        {
            Checking::clear_queue(mp);
            Storage::clear_queue(mp);
        }

        // checking policy functions are forwarded to the Checking type
        template <typename MultiPass>
        inline static void docheck(MultiPass const& mp)
            { Checking::docheck(mp); }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Ownership, typename Checking
      , typename Input, typename Storage>
    struct multi_pass_unique<T, Ownership, Checking, Input, Storage
          , false, true, true>
      : Ownership, Storage
    {
        multi_pass_unique() {}
        multi_pass_unique(T const&) {}

        template <typename MultiPass>
        static void destroy(MultiPass& mp)
        {
            Ownership::destroy(mp);
            Input::destroy(mp);
            Storage::destroy(mp);
        }

        void swap(multi_pass_unique& x)
        {
            this->Ownership::swap(x);
            this->Storage::swap(x);
        }

        template <typename MultiPass>
        inline static void clear_queue(MultiPass& mp)
        {
            Checking::clear_queue(mp);
            Storage::clear_queue(mp);
        }

        // implement input policy functions by forwarding to the Input type
        template <typename MultiPass>
        inline static void advance_input(MultiPass& mp)
            { Input::advance_input(mp); }

        template <typename MultiPass>
        inline static typename MultiPass::reference get_input(MultiPass& mp)
            { return Input::get_input(mp); }

        template <typename MultiPass>
        inline static bool input_at_eof(MultiPass const& mp)
            { return Input::input_at_eof(mp); }

        template <typename MultiPass, typename TokenType>
        inline static bool input_is_valid(MultiPass& mp, TokenType& curtok)
            { return Input::input_is_valid(mp, curtok); }

        // checking policy functions are forwarded to the Checking type
        template <typename MultiPass>
        inline static void docheck(MultiPass const& mp)
            { Checking::docheck(mp); }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Ownership, typename Checking
      , typename Input, typename Storage>
    struct multi_pass_unique<T, Ownership, Checking, Input, Storage
          , true, false, false>
      : Checking, Input, Storage
    {
        multi_pass_unique() {}
        multi_pass_unique(T& x) : Input(x) {}
        multi_pass_unique(T const& x) : Input(x) {}

        template <typename MultiPass>
        static void destroy(MultiPass& mp)
        {
            Checking::destroy(mp);
            Input::destroy(mp);
            Storage::destroy(mp);
        }

        void swap(multi_pass_unique& x)
        {
            this->Checking::swap(x);
            this->Input::swap(x);
            this->Storage::swap(x);
        }

        template <typename MultiPass>
        inline static void clear_queue(MultiPass& mp)
        {
            Checking::clear_queue(mp);
            Storage::clear_queue(mp);
        }

        // ownership policy functions are forwarded to the Ownership type
        template <typename MultiPass>
        inline static void clone(MultiPass& mp)
            { Ownership::clone(mp); }

        template <typename MultiPass>
        inline static bool release(MultiPass& mp)
            { return Ownership::release(mp); }

        template <typename MultiPass>
        inline static bool is_unique(MultiPass const& mp)
            { return Ownership::is_unique(mp); }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Ownership, typename Checking
      , typename Input, typename Storage>
    struct multi_pass_unique<T, Ownership, Checking, Input, Storage
          , true, false, true>
      : Checking, Storage
    {
        multi_pass_unique() {}
        multi_pass_unique(T const&) {}

        template <typename MultiPass>
        static void destroy(MultiPass& mp)
        {
            Checking::destroy(mp);
            Input::destroy(mp);
            Storage::destroy(mp);
        }

        void swap(multi_pass_unique& x)
        {
            this->Checking::swap(x);
            this->Storage::swap(x);
        }

        template <typename MultiPass>
        inline static void clear_queue(MultiPass& mp)
        {
            Checking::clear_queue(mp);
            Storage::clear_queue(mp);
        }

        // implement input policy functions by forwarding to the Input type
        template <typename MultiPass>
        inline static void advance_input(MultiPass& mp)
            { Input::advance_input(mp); }

        template <typename MultiPass>
        inline static typename MultiPass::reference get_input(MultiPass& mp)
            { return Input::get_input(mp); }

        template <typename MultiPass>
        inline static bool input_at_eof(MultiPass const& mp)
            { return Input::input_at_eof(mp); }

        template <typename MultiPass, typename TokenType>
        inline static bool input_is_valid(MultiPass& mp, TokenType& curtok)
            { return Input::input_is_valid(mp, curtok); }

        // ownership policy functions are forwarded to the Ownership type
        template <typename MultiPass>
        inline static void clone(MultiPass& mp)
            { Ownership::clone(mp); }

        template <typename MultiPass>
        inline static bool release(MultiPass& mp)
            { return Ownership::release(mp); }

        template <typename MultiPass>
        inline static bool is_unique(MultiPass const& mp)
            { return Ownership::is_unique(mp); }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Ownership, typename Checking
      , typename Input, typename Storage>
    struct multi_pass_unique<T, Ownership, Checking, Input, Storage
          , true, true, false>
      : Input, Storage
    {
        multi_pass_unique() {}
        multi_pass_unique(T& x) : Input(x) {}
        multi_pass_unique(T const& x) : Input(x) {}

        template <typename MultiPass>
        static void destroy(MultiPass& mp)
        {
            Input::destroy(mp);
            Storage::destroy(mp);
        }

        void swap(multi_pass_unique& x)
        {
            this->Input::swap(x);
            this->Storage::swap(x);
        }

        template <typename MultiPass>
        inline static void clear_queue(MultiPass& mp)
        {
            Checking::clear_queue(mp);
            Storage::clear_queue(mp);
        }

        // checking policy functions are forwarded to the Checking type
        template <typename MultiPass>
        inline static void docheck(MultiPass const& mp)
            { Checking::docheck(mp); }

        // ownership policy functions are forwarded to the Ownership type
        template <typename MultiPass>
        inline static void clone(MultiPass& mp)
            { Ownership::clone(mp); }

        template <typename MultiPass>
        inline static bool release(MultiPass& mp)
            { return Ownership::release(mp); }

        template <typename MultiPass>
        inline static bool is_unique(MultiPass const& mp)
            { return Ownership::is_unique(mp); }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Ownership, typename Checking
      , typename Input, typename Storage>
    struct multi_pass_unique<T, Ownership, Checking, Input, Storage
          , true, true, true>
      : Storage
    {
        multi_pass_unique() {}
        multi_pass_unique(T const&) {}

        template <typename MultiPass>
        static void destroy(MultiPass& mp)
        {
            Input::destroy(mp);
            Storage::destroy(mp);
        }

        void swap(multi_pass_unique& x)
        {
            this->Storage::swap(x);
        }

        template <typename MultiPass>
        inline static void clear_queue(MultiPass& mp)
        {
            Checking::clear_queue(mp);
            Storage::clear_queue(mp);
        }

        // implement input policy functions by forwarding to the Input type
        template <typename MultiPass>
        inline static void advance_input(MultiPass& mp)
            { Input::advance_input(mp); }

        template <typename MultiPass>
        inline static typename MultiPass::reference get_input(MultiPass& mp)
            { return Input::get_input(mp); }

        template <typename MultiPass>
        inline static bool input_at_eof(MultiPass const& mp)
            { return Input::input_at_eof(mp); }

        template <typename MultiPass, typename TokenType>
        inline static bool input_is_valid(MultiPass& mp, TokenType& curtok)
            { return Input::input_is_valid(mp, curtok); }

        // checking policy functions are forwarded to the Checking type
        template <typename MultiPass>
        inline static void docheck(MultiPass const& mp)
            { Checking::docheck(mp); }

        // ownership policy functions are forwarded to the Ownership type
        template <typename MultiPass>
        inline static void clone(MultiPass& mp)
            { Ownership::clone(mp); }

        template <typename MultiPass>
        inline static bool release(MultiPass& mp)
            { return Ownership::release(mp); }

        template <typename MultiPass>
        inline static bool is_unique(MultiPass const& mp)
            { return Ownership::is_unique(mp); }
    };

    ///////////////////////////////////////////////////////////////////////////
    // the multi_pass_shared structure is used to combine the shared data items
    // of all policies into one single structure
    ///////////////////////////////////////////////////////////////////////////
    template<typename T, typename Ownership, typename Checking, typename Input
      , typename Storage>
    struct multi_pass_shared : Ownership, Checking, Input, Storage
    {
        explicit multi_pass_shared(T& input) : Input(input) {}
        explicit multi_pass_shared(T const& input) : Input(input) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    //  This is a default implementation of a policy class as required by the
    //  multi_pass template, combining 4 separate policies into one. Any other
    //  multi_pass policy class needs to follow the scheme as shown below.
    template<typename Ownership, typename Checking, typename Input
      , typename Storage>
    struct default_policy
    {
        typedef Ownership ownership_policy;
        typedef Checking checking_policy;
        typedef Input input_policy;
        typedef Storage storage_policy;

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        struct unique : multi_pass_unique<T
          , typename Ownership::unique, typename Checking::unique
          , typename Input::BOOST_NESTED_TEMPLATE unique<T>
          , typename Storage::BOOST_NESTED_TEMPLATE unique<
                typename Input::BOOST_NESTED_TEMPLATE unique<T>::value_type> >
        {
            typedef typename Ownership::unique ownership_policy;
            typedef typename Checking::unique checking_policy;
            typedef typename Input::BOOST_NESTED_TEMPLATE unique<T>
                input_policy;
            typedef typename Storage::BOOST_NESTED_TEMPLATE unique<
                typename input_policy::value_type> storage_policy;

            typedef multi_pass_unique<T, ownership_policy, checking_policy
              , input_policy, storage_policy> unique_base_type;

            unique() {}
            explicit unique(T& input) : unique_base_type(input) {}
            explicit unique(T const& input) : unique_base_type(input) {}
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        struct shared : multi_pass_shared<T
          , typename Ownership::shared, typename Checking::shared
          , typename Input::BOOST_NESTED_TEMPLATE shared<T>
          , typename Storage::BOOST_NESTED_TEMPLATE shared<
                typename Input::BOOST_NESTED_TEMPLATE unique<T>::value_type> >
        {
            typedef typename Ownership::shared ownership_policy;
            typedef typename Checking::shared checking_policy;
            typedef typename Input::BOOST_NESTED_TEMPLATE shared<T>
                input_policy;
            typedef typename Storage::BOOST_NESTED_TEMPLATE shared<
                typename Input::BOOST_NESTED_TEMPLATE unique<T>::value_type>
            storage_policy;

            typedef multi_pass_shared<T, ownership_policy, checking_policy
              , input_policy, storage_policy> shared_base_type;

            explicit shared(T& input)
              : shared_base_type(input), inhibit_clear_queue_(false) {}
            explicit shared(T const& input)
              : shared_base_type(input), inhibit_clear_queue_(false) {}

            // This is needed for the correct implementation of expectation
            // points. Normally expectation points flush any multi_pass
            // iterator they may act on, but if the corresponding error handler
            // is of type 'retry' no flushing of the internal buffers should be
            // executed (even if explicitly requested).
            bool inhibit_clear_queue_;
        };
    };

}}}

#endif
