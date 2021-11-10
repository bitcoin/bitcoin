/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_RULE_HPP)
#define BOOST_SPIRIT_RULE_HPP

#include <boost/static_assert.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit predefined maximum number of simultaneously usable different
//  scanner types.
//
//  This limit defines the maximum number of possible different scanner
//  types for which a specific rule<> may be used. If this isn't defined, a
//  rule<> may be used with one scanner type only (multiple scanner support
//  is disabled).
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT)
#  define BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT 1
#endif

//  Ensure a meaningful maximum number of simultaneously usable scanner types
BOOST_STATIC_ASSERT(BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 0);

#include <boost/scoped_ptr.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/non_terminal/impl/rule.ipp>

#if BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 1
#  include <boost/preprocessor/enum_params.hpp>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

#if BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT > 1

    ///////////////////////////////////////////////////////////////////////////
    //
    //  scanner_list (a fake scanner)
    //
    //      Typically, rules are tied to a specific scanner type and
    //      a particular rule cannot be used with anything else. Sometimes
    //      there's a need for rules that can accept more than one scanner
    //      type. The scanner_list<S0, ...SN> can be used as a template
    //      parameter to the rule class to specify up to the number of
    //      scanner types defined by the BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT
    //      constant. Example:
    //
    //          rule<scanner_list<ScannerT0, ScannerT1> > r;
    //
    //      *** This feature is available only to compilers that support
    //      partial template specialization. ***
    //
    ///////////////////////////////////////////////////////////////////////////
    template <
        BOOST_PP_ENUM_PARAMS(
            BOOST_SPIRIT_RULE_SCANNERTYPE_LIMIT,
            typename ScannerT
        )
    >
    struct scanner_list : scanner_base {};

#endif

    ///////////////////////////////////////////////////////////////////////////
    //
    //  rule class
    //
    //      The rule is a polymorphic parser that acts as a named place-
    //      holder capturing the behavior of an EBNF expression assigned to
    //      it.
    //
    //      The rule is a template class parameterized by:
    //
    //          1) scanner (scanner_t, see scanner.hpp),
    //          2) the rule's context (context_t, see parser_context.hpp)
    //          3) an arbitrary tag (tag_t, see parser_id.hpp) that allows
    //             a rule to be tagged for identification.
    //
    //      These template parameters may be specified in any order. The
    //      scanner will default to scanner<> when it is not specified.
    //      The context will default to parser_context when not specified.
    //      The tag will default to parser_address_tag when not specified.
    //
    //      The definition of the rule (its right hand side, RHS) held by
    //      the rule through a scoped_ptr. When a rule is seen in the RHS
    //      of an assignment or copy construction EBNF expression, the rule
    //      is held by the LHS rule by reference.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T0 = nil_t
      , typename T1 = nil_t
      , typename T2 = nil_t
    >
    class rule
        : public impl::rule_base<
            rule<T0, T1, T2>
          , rule<T0, T1, T2> const&
          , T0, T1, T2>
    {
    public:

        typedef rule<T0, T1, T2> self_t;
        typedef impl::rule_base<
            self_t
          , self_t const&
          , T0, T1, T2>
        base_t;

        typedef typename base_t::scanner_t scanner_t;
        typedef typename base_t::attr_t attr_t;
        typedef impl::abstract_parser<scanner_t, attr_t> abstract_parser_t;

        rule() : ptr() {}
        ~rule() {}

        rule(rule const& r)
        : ptr(new impl::concrete_parser<rule, scanner_t, attr_t>(r)) {}

        template <typename ParserT>
        rule(ParserT const& p)
        : ptr(new impl::concrete_parser<ParserT, scanner_t, attr_t>(p)) {}

        template <typename ParserT>
        rule& operator=(ParserT const& p)
        {
            ptr.reset(new impl::concrete_parser<ParserT, scanner_t, attr_t>(p));
            return *this;
        }

        rule& operator=(rule const& r)
        {
            ptr.reset(new impl::concrete_parser<rule, scanner_t, attr_t>(r));
            return *this;
        }

        rule<T0, T1, T2>
        copy() const
        {
            return rule<T0, T1, T2>(ptr.get() ? ptr->clone() : 0);
        }

    private:
        friend class impl::rule_base_access;

        abstract_parser_t*
        get() const
        {
            return ptr.get();
        }

        rule(abstract_parser_t* ptr_)
        : ptr(ptr_) {}

        rule(abstract_parser_t const* ptr_)
        : ptr(ptr_) {}

        scoped_ptr<abstract_parser_t> ptr;
    };

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
