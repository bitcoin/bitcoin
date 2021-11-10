//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_OPERATOR_LIST_HPP
#define BOOST_SPIRIT_KARMA_OPERATOR_LIST_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/karma/detail/indirect_iterator.hpp>
#include <boost/spirit/home/karma/detail/get_stricttag.hpp>
#include <boost/spirit/home/karma/detail/pass_container.hpp>
#include <boost/spirit/home/karma/detail/fail_function.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/proto/operators.hpp>
#include <boost/proto/tags.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<karma::domain, proto::tag::modulus> // enables g % d
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
    template <typename Left, typename Right, typename Strict, typename Derived>
    struct base_list : binary_generator<Derived>
    {
    private:
        // iterate over the given container until its exhausted or the embedded
        // (left) generator succeeds
        template <typename F, typename Attribute>
        bool generate_left(F f, Attribute const&, mpl::false_) const
        {
            // Failing subject generators are just skipped. This allows to
            // selectively generate items in the provided attribute.
            while (!f.is_at_end())
            {
                bool r = !f(left);
                if (r)
                    return true;
                if (!f.is_at_end())
                    f.next();
            }
            return false;
        }

        template <typename F, typename Attribute>
        bool generate_left(F f, Attribute const&, mpl::true_) const
        {
            return !f(left);
        }

        // There is no way to distinguish a failed generator from a
        // generator to be skipped. We assume the user takes responsibility
        // for ending the loop if no attribute is specified.
        template <typename F>
        bool generate_left(F f, unused_type, mpl::false_) const
        {
            return !f(left);
        }

    public:
        typedef Left left_type;
        typedef Right right_type;

        typedef mpl::int_<
            left_type::properties::value
          | right_type::properties::value
          | generator_properties::buffering
          | generator_properties::counting
        > properties;

        // Build a std::vector from the LHS's attribute. Note
        // that build_std_vector may return unused_type if the
        // subject's attribute is an unused_type.
        template <typename Context, typename Iterator>
        struct attribute
          : traits::build_std_vector<
                typename traits::attribute_of<Left, Context, Iterator>::type>
        {};

        base_list(Left const& left, Right const& right)
          : left(left), right(right)
        {}

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Attribute const& attr) const
        {
            typedef detail::fail_function<
                OutputIterator, Context, Delimiter
            > fail_function;

            typedef typename traits::container_iterator<
                typename add_const<Attribute>::type
            >::type iterator_type;

            typedef
                typename traits::make_indirect_iterator<iterator_type>::type
            indirect_iterator_type;
            typedef detail::pass_container<
                fail_function, Attribute, indirect_iterator_type, mpl::false_>
            pass_container;

            iterator_type it = traits::begin(attr);
            iterator_type end = traits::end(attr);

            pass_container pass(fail_function(sink, ctx, d),
                indirect_iterator_type(it), indirect_iterator_type(end));

            if (generate_left(pass, attr, Strict()))
            {
                while (!pass.is_at_end())
                {
                    // wrap the given output iterator as generate_left might fail
                    detail::enable_buffering<OutputIterator> buffering(sink);
                    {
                        detail::disable_counting<OutputIterator> nocounting(sink);

                        if (!right.generate(sink, ctx, d, unused))
                            return false;     // shouldn't happen

                        if (!generate_left(pass, attr, Strict()))
                            break;            // return true as one item succeeded
                    }
                    buffering.buffer_copy();
                }
                return detail::sink_is_good(sink);
            }
            return false;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("list",
                std::make_pair(left.what(context), right.what(context)));
        }

        Left left;
        Right right;
    };

    template <typename Left, typename Right>
    struct list
      : base_list<Left, Right, mpl::false_, list<Left, Right> >
    {
        typedef base_list<Left, Right, mpl::false_, list> base_list_;

        list(Left const& left, Right const& right)
          : base_list_(left, right) {}
    };

    template <typename Left, typename Right>
    struct strict_list
      : base_list<Left, Right, mpl::true_, strict_list<Left, Right> >
    {
        typedef base_list<Left, Right, mpl::true_, strict_list> base_list_;

        strict_list (Left const& left, Right const& right)
          : base_list_(left, right) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Subject, bool strict_mode = false>
        struct make_list
          : make_binary_composite<Subject, list>
        {};

        template <typename Subject>
        struct make_list<Subject, true>
          : make_binary_composite<Subject, strict_list>
        {};
    }

    template <typename Subject, typename Modifiers>
    struct make_composite<proto::tag::modulus, Subject, Modifiers>
      : detail::make_list<Subject, detail::get_stricttag<Modifiers>::value>
    {};
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Left, typename Right>
    struct has_semantic_action<karma::list<Left, Right> >
      : binary_has_semantic_action<Left, Right> {};

    template <typename Left, typename Right>
    struct has_semantic_action<karma::strict_list<Left, Right> >
      : binary_has_semantic_action<Left, Right> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Left, typename Right, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<karma::list<Left, Right>, Attribute
          , Context, Iterator>
      : mpl::true_ {};

    template <typename Left, typename Right, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<karma::strict_list<Left, Right>, Attribute
          , Context, Iterator>
      : mpl::true_ {};
}}}

#endif
