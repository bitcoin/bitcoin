/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTIONS_HPP
#define BOOST_SPIRIT_ACTIONS_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/core/ignore_unused.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

    ///////////////////////////////////////////////////////////////////////////
    //
    //  action class
    //
    //      The action class binds a parser with a user defined semantic
    //      action. Instances of action are never created manually. Instead,
    //      action objects are typically created indirectly through
    //      expression templates of the form:
    //
    //          p[f]
    //
    //      where p is a parser and f is a function or functor. The semantic
    //      action may be a function or a functor. When the parser is
    //      successful, the actor calls the scanner's action_policy policy
    //      (see scanner.hpp):
    //
    //          scan.do_action(actor, attribute, first, last);
    //
    //      passing in these information:
    //
    //          actor:        The action's function or functor
    //          attribute:    The match (returned by the parser) object's
    //                        attribute (see match.hpp)
    //          first:        Iterator pointing to the start of the matching
    //                        portion of the input
    //          last:         Iterator pointing to one past the end of the
    //                        matching portion of the input
    //
    //      It is the responsibility of the scanner's action_policy policy to
    //      dispatch the function or functor as it sees fit. The expected
    //      function or functor signature depends on the parser being
    //      wrapped. In general, if the attribute type of the parser being
    //      wrapped is a nil_t, the function or functor expect the signature:
    //
    //          void func(Iterator first, Iterator last); // functions
    //
    //          struct ftor // functors
    //          {
    //              void func(Iterator first, Iterator last) const;
    //          };
    //
    //      where Iterator is the type of the iterator that is being used and
    //      first and last are the iterators pointing to the matching portion
    //      of the input.
    //
    //      If the attribute type of the parser being wrapped is not a nil_t,
    //      the function or functor usually expect the signature:
    //
    //          void func(T val); // functions
    //
    //          struct ftor // functors
    //          {
    //              void func(T val) const;
    //          };
    //
    //      where T is the attribute type and val is the attribute value
    //      returned by the parser being wrapped.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ParserT, typename ActionT>
    class action : public unary<ParserT, parser<action<ParserT, ActionT> > >
    {
    public:

        typedef action<ParserT, ActionT>        self_t;
        typedef action_parser_category          parser_category_t;
        typedef unary<ParserT, parser<self_t> > base_t;
        typedef ActionT                         predicate_t;

        template <typename ScannerT>
        struct result
        {
            typedef typename parser_result<ParserT, ScannerT>::type type;
        };

        action(ParserT const& p, ActionT const& a)
        : base_t(p)
        , actor(a) {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename ScannerT::iterator_t iterator_t;
            typedef typename parser_result<self_t, ScannerT>::type result_t;

            ignore_unused(scan.at_end()); // allow skipper to take effect
            iterator_t save = scan.first;
            result_t hit = this->subject().parse(scan);
            if (hit)
            {
                typename result_t::return_t val = hit.value();
                scan.do_action(actor, val, save, scan.first);
            }
            return hit;
        }

        ActionT const& predicate() const { return actor; }

    private:

        ActionT actor;
    };

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
