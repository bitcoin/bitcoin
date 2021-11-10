/*=============================================================================
    Copyright (c) 1998-2002 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_SCANNER_HPP)
#define BOOST_SPIRIT_SCANNER_HPP

#include <iterator>
#include <boost/config.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/match.hpp>
#include <boost/spirit/home/classic/core/non_terminal/parser_id.hpp>

#include <boost/spirit/home/classic/core/scanner/scanner_fwd.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  iteration_policy class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct iteration_policy
    {
        template <typename ScannerT>
        void
        advance(ScannerT const& scan) const
        {
            ++scan.first;
        }

        template <typename ScannerT>
        bool at_end(ScannerT const& scan) const
        {
            return scan.first == scan.last;
        }

        template <typename T>
        T filter(T ch) const
        {
            return ch;
        }

        template <typename ScannerT>
        typename ScannerT::ref_t
        get(ScannerT const& scan) const
        {
            return *scan.first;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  match_policy class
    //
    ///////////////////////////////////////////////////////////////////////////
    struct match_policy
    {
        template <typename T>
        struct result { typedef match<T> type; };

        const match<nil_t>
        no_match() const
        {
            return match<nil_t>();
        }

        const match<nil_t>
        empty_match() const
        {
            return match<nil_t>(0, nil_t());
        }

        template <typename AttrT, typename IteratorT>
        match<AttrT>
        create_match(
            std::size_t         length,
            AttrT const&        val,
            IteratorT const&    /*first*/,
            IteratorT const&    /*last*/) const
        {
            return match<AttrT>(length, val);
        }

        template <typename MatchT, typename IteratorT>
        void group_match(
            MatchT&             /*m*/,
            parser_id const&    /*id*/,
            IteratorT const&    /*first*/,
            IteratorT const&    /*last*/) const {}

        template <typename Match1T, typename Match2T>
        void concat_match(Match1T& l, Match2T const& r) const
        {
            l.concat(r);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  match_result class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename MatchPolicyT, typename T>
    struct match_result
    {
        typedef typename MatchPolicyT::template result<T>::type type;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  action_policy class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename AttrT>
    struct attributed_action_policy
    {
        template <typename ActorT, typename IteratorT>
        static void
        call(
            ActorT const& actor,
            AttrT& val,
            IteratorT const&,
            IteratorT const&)
        {
            actor(val);
        }
    };

    //////////////////////////////////
    template <>
    struct attributed_action_policy<nil_t>
    {
        template <typename ActorT, typename IteratorT>
        static void
        call(
            ActorT const& actor,
            nil_t,
            IteratorT const& first,
            IteratorT const& last)
        {
            actor(first, last);
        }
    };

    //////////////////////////////////
    struct action_policy
    {
        template <typename ActorT, typename AttrT, typename IteratorT>
        void
        do_action(
            ActorT const&       actor,
            AttrT&              val,
            IteratorT const&    first,
            IteratorT const&    last) const
        {
            attributed_action_policy<AttrT>::call(actor, val, first, last);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  scanner_policies class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename IterationPolicyT,
        typename MatchPolicyT,
        typename ActionPolicyT>
    struct scanner_policies :
        public IterationPolicyT,
        public MatchPolicyT,
        public ActionPolicyT
    {
        typedef IterationPolicyT    iteration_policy_t;
        typedef MatchPolicyT        match_policy_t;
        typedef ActionPolicyT       action_policy_t;

        scanner_policies(
            IterationPolicyT const& i_policy = IterationPolicyT(),
            MatchPolicyT const&     m_policy = MatchPolicyT(),
            ActionPolicyT const&    a_policy = ActionPolicyT())
        : IterationPolicyT(i_policy)
        , MatchPolicyT(m_policy)
        , ActionPolicyT(a_policy) {}

        template <typename ScannerPoliciesT>
        scanner_policies(ScannerPoliciesT const& policies)
        : IterationPolicyT(policies)
        , MatchPolicyT(policies)
        , ActionPolicyT(policies) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  scanner_policies_base class: the base class of all scanners
    //
    ///////////////////////////////////////////////////////////////////////////
    struct scanner_base {};

    ///////////////////////////////////////////////////////////////////////////
    //
    //  scanner class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename IteratorT,
        typename PoliciesT>
    class scanner : public PoliciesT, public scanner_base
    {
    public:

        typedef IteratorT iterator_t;
        typedef PoliciesT policies_t;

        typedef typename std::
            iterator_traits<IteratorT>::value_type value_t;
        typedef typename std::
            iterator_traits<IteratorT>::reference ref_t;
        typedef typename boost::
            call_traits<IteratorT>::param_type iter_param_t;

        scanner(
            IteratorT&          first_,
            iter_param_t        last_,
            PoliciesT const&    policies = PoliciesT())
        : PoliciesT(policies), first(first_), last(last_)
        {
            at_end();
        }

        scanner(scanner const& other)
        : PoliciesT(other), first(other.first), last(other.last) {}

        scanner(scanner const& other, IteratorT& first_)
        : PoliciesT(other), first(first_), last(other.last) {}

        template <typename PoliciesT1>
        scanner(scanner<IteratorT, PoliciesT1> const& other)
        : PoliciesT(other), first(other.first), last(other.last) {}

        bool
        at_end() const
        {
            typedef typename PoliciesT::iteration_policy_t iteration_policy_type;
            return iteration_policy_type::at_end(*this);
        }

        value_t
        operator*() const
        {
            typedef typename PoliciesT::iteration_policy_t iteration_policy_type;
            return iteration_policy_type::filter(iteration_policy_type::get(*this));
        }

        scanner const&
        operator++() const
        {
            typedef typename PoliciesT::iteration_policy_t iteration_policy_type;
            iteration_policy_type::advance(*this);
            return *this;
        }

        template <typename PoliciesT2>
        struct rebind_policies
        {
            typedef scanner<IteratorT, PoliciesT2> type;
        };

        template <typename PoliciesT2>
        scanner<IteratorT, PoliciesT2>
        change_policies(PoliciesT2 const& policies) const
        {
            return scanner<IteratorT, PoliciesT2>(first, last, policies);
        }

        template <typename IteratorT2>
        struct rebind_iterator
        {
            typedef scanner<IteratorT2, PoliciesT> type;
        };

        template <typename IteratorT2>
        scanner<IteratorT2, PoliciesT>
        change_iterator(IteratorT2 const& first_, IteratorT2 const &last_) const
        {
            return scanner<IteratorT2, PoliciesT>(first_, last_, *this);
        }

        IteratorT& first;
        IteratorT const last;

    private:

        scanner&
        operator=(scanner const& other);
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  rebind_scanner_policies class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ScannerT, typename PoliciesT>
    struct rebind_scanner_policies
    {
        typedef typename ScannerT::template
            rebind_policies<PoliciesT>::type type;
    };

    //////////////////////////////////
    template <typename ScannerT, typename IteratorT>
    struct rebind_scanner_iterator
    {
        typedef typename ScannerT::template
            rebind_iterator<IteratorT>::type type;
    };

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
