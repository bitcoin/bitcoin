//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_KLEENE_MAR_03_2007_0337AM)
#define BOOST_SPIRIT_KARMA_KLEENE_MAR_03_2007_0337AM

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
#include <boost/type_traits/add_const.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<karma::domain, proto::tag::dereference> // enables *g
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
    template <typename Subject, typename Strict, typename Derived>
    struct base_kleene : unary_generator<Derived>
    {
    private:
        // Ignore return value in relaxed mode (failing subject generators 
        // are just skipped). This allows to selectively generate items in 
        // the provided attribute.
        template <typename F, typename Attribute>
        bool generate_subject(F f, Attribute const&, mpl::false_) const
        {
            bool r = !f(subject);
            if (!r && !f.is_at_end())
                f.next();
            return true;
        }

        template <typename F, typename Attribute>
        bool generate_subject(F f, Attribute const&, mpl::true_) const
        {
            return !f(subject);
        }

        // There is no way to distinguish a failed generator from a 
        // generator to be skipped. We assume the user takes responsibility 
        // for ending the loop if no attribute is specified.
        template <typename F>
        bool generate_subject(F f, unused_type, mpl::false_) const
        {
            return !f(subject);
        }

//         template <typename F>
//         bool generate_subject(F f, unused_type, mpl::true_) const
//         {
//             return !f(subject);
//         }

    public:
        typedef Subject subject_type;
        typedef typename subject_type::properties properties;

        // Build a std::vector from the subject's attribute. Note
        // that build_std_vector may return unused_type if the
        // subject's attribute is an unused_type.
        template <typename Context, typename Iterator>
        struct attribute
          : traits::build_std_vector<
                typename traits::attribute_of<Subject, Context, Iterator>::type
            >
        {};

        base_kleene(Subject const& subject)
          : subject(subject) {}

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Attribute const& attr) const
        {
            typedef detail::fail_function<
                OutputIterator, Context, Delimiter> fail_function;

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

            // kleene fails only if the underlying output fails
            while (!pass.is_at_end())
            {
                if (!generate_subject(pass, attr, Strict()))
                    break;
            }
            return detail::sink_is_good(sink);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("kleene", subject.what(context));
        }

        Subject subject;
    };

    template <typename Subject>
    struct kleene 
      : base_kleene<Subject, mpl::false_, kleene<Subject> >
    {
        typedef base_kleene<Subject, mpl::false_, kleene> base_kleene_;

        kleene(Subject const& subject)
          : base_kleene_(subject) {}
    };

    template <typename Subject>
    struct strict_kleene 
      : base_kleene<Subject, mpl::true_, strict_kleene<Subject> >
    {
        typedef base_kleene<Subject, mpl::true_, strict_kleene> base_kleene_;

        strict_kleene(Subject const& subject)
          : base_kleene_(subject) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Subject, bool strict_mode = false>
        struct make_kleene 
          : make_unary_composite<Subject, kleene>
        {};

        template <typename Subject>
        struct make_kleene<Subject, true> 
          : make_unary_composite<Subject, strict_kleene>
        {};
    }

    template <typename Subject, typename Modifiers>
    struct make_composite<proto::tag::dereference, Subject, Modifiers>
      : detail::make_kleene<Subject, detail::get_stricttag<Modifiers>::value>
    {};
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<karma::kleene<Subject> >
      : unary_has_semantic_action<Subject> {};

    template <typename Subject>
    struct has_semantic_action<karma::strict_kleene<Subject> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::kleene<Subject>, Attribute
          , Context, Iterator> 
      : mpl::true_ {};

    template <typename Subject, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::strict_kleene<Subject>, Attribute
          , Context, Iterator> 
      : mpl::true_ {};
}}}

#endif
