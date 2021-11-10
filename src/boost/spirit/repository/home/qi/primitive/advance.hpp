//  Copyright (c) 2011 Aaron Graham
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_REPOSITORY_QI_ADVANCE_JAN_23_2011_1203PM)
#define BOOST_SPIRIT_REPOSITORY_QI_ADVANCE_JAN_23_2011_1203PM

#include <boost/spirit/home/support/terminal.hpp>
#include <boost/spirit/include/qi_parse.hpp>

///////////////////////////////////////////////////////////////////////////////
// definition the place holder
namespace boost { namespace spirit { namespace repository { namespace qi
{
    BOOST_SPIRIT_TERMINAL_EX(advance)
}}}}

///////////////////////////////////////////////////////////////////////////////
// implementation the enabler
namespace boost { namespace spirit
{
    template <typename A0>
    struct use_terminal<qi::domain
      , terminal_ex<repository::qi::tag::advance, fusion::vector1<A0> > >
      : mpl::or_<is_integral<A0>, is_enum<A0> >
    {};

    template <>
    struct use_lazy_terminal<qi::domain, repository::qi::tag::advance, 1>
      : mpl::true_
    {};
}}

///////////////////////////////////////////////////////////////////////////////
// implementation of the parser
namespace boost { namespace spirit { namespace repository { namespace qi
{
    template <typename Int>
    struct advance_parser
      : boost::spirit::qi::primitive_parser< advance_parser<Int> >
    {
        // Define the attribute type exposed by this parser component
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef boost::spirit::unused_type type;
        };

        advance_parser(Int dist)
          : dist(dist)
        {}

        // This function is called during the actual parsing process
        template <typename Iterator, typename Context
            , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
            , Context&, Skipper&, Attribute&) const
        {
            // This series of checks is designed to fail parsing on negative
            // values, without generating a "expression always evaluates true"
            // warning on unsigned types.
            if (dist == Int(0)) return true;
            if (dist < Int(1)) return false;

            typedef typename std::iterator_traits<Iterator>::iterator_category
                iterator_category;
            return advance(first, last, iterator_category());
        }

        // This function is called during error handling to create
        // a human readable string for the error context.
        template <typename Context>
        boost::spirit::info what(Context&) const
        {
            return boost::spirit::info("advance");
        }

    private:
        // this is the general implementation used by most iterator categories
        template <typename Iterator, typename IteratorCategory>
        bool advance(Iterator& first, Iterator const& last
            , IteratorCategory) const
        {
            Int n = dist;
            Iterator i = first;
            while (n)
            {
                if (i == last) return false;
                ++i;
                --n;
            }
            first = i;
            return true;
        }

        // this is a specialization for random access iterators
        template <typename Iterator>
        bool advance(Iterator& first, Iterator const& last
            , std::random_access_iterator_tag) const
        {
            Iterator const it = first + dist;
            if (it > last) return false;
            first = it;
            return true;
        }

        Int const dist;
    };
}}}}

///////////////////////////////////////////////////////////////////////////////
// instantiation of the parser
namespace boost { namespace spirit { namespace qi
{
    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<repository::qi::tag::advance, fusion::vector1<A0> >
        , Modifiers>
    {
        typedef repository::qi::advance_parser<A0> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };
}}}

#endif
