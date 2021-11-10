//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_POSITIVE_MAR_03_2007_0945PM)
#define BOOST_SPIRIT_KARMA_POSITIVE_MAR_03_2007_0945PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/detail/indirect_iterator.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/karma/detail/get_stricttag.hpp>
#include <boost/spirit/home/karma/detail/pass_container.hpp>
#include <boost/spirit/home/karma/detail/fail_function.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
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
    struct use_operator<karma::domain, proto::tag::unary_plus> // enables +g
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
    template <typename Subject, typename Strict, typename Derived>
    struct base_plus : unary_generator<Derived>
    {
    private:
        // Ignore return value in relaxed mode (failing subject generators 
        // are just skipped). This allows to selectively generate items in 
        // the provided attribute.
        template <typename F, typename Attribute>
        bool generate_subject(F f, Attribute const&, bool& result, mpl::false_) const
        {
            bool r = !f(subject);
            if (r) 
                result = true;
            else if (!f.is_at_end())
                f.next();
            return true;
        }

        template <typename F, typename Attribute>
        bool generate_subject(F f, Attribute const&, bool& result, mpl::true_) const
        {
            bool r = !f(subject);
            if (r)
                result = true;
            return r;
        }

        // There is no way to distinguish a failed generator from a 
        // generator to be skipped. We assume the user takes responsibility
        // for ending the loop if no attribute is specified.
        template <typename F>
        bool generate_subject(F f, unused_type, bool& result, mpl::false_) const
        {
            bool r = f(subject);
            if (!r)
                result = true;
            return !r;
        }

//         template <typename F>
//         bool generate_subject(F f, unused_type, bool& result, mpl::true_) const
//         {
//             bool r = f(subject);
//             if (!r)
//                 result = true;
//             return !r;
//         }

    public:
        typedef Subject subject_type;
        typedef typename subject_type::properties properties;

        // Build a std::vector from the subjects attribute. Note
        // that build_std_vector may return unused_type if the
        // subject's attribute is an unused_type.
        template <typename Context, typename Iterator>
        struct attribute
          : traits::build_std_vector<
                typename traits::attribute_of<subject_type, Context, Iterator>::type
            >
        {};

        base_plus(Subject const& subject)
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

            // plus fails if the parameter is empty
            if (traits::compare(it, end))
                return false;

            pass_container pass(fail_function(sink, ctx, d), 
                indirect_iterator_type(it), indirect_iterator_type(end));

            // from now on plus fails if the underlying output fails or overall
            // no subject generators succeeded
            bool result = false;
            while (!pass.is_at_end())
            {
                if (!generate_subject(pass, attr, result, Strict()))
                    break;
            }
            return result && detail::sink_is_good(sink);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("plus", subject.what(context));
        }

        Subject subject;
    };

    template <typename Subject>
    struct plus 
      : base_plus<Subject, mpl::false_, plus<Subject> >
    {
        typedef base_plus<Subject, mpl::false_, plus> base_plus_;

        plus(Subject const& subject)
          : base_plus_(subject) {}
    };

    template <typename Subject>
    struct strict_plus 
      : base_plus<Subject, mpl::true_, strict_plus<Subject> >
    {
        typedef base_plus<Subject, mpl::true_, strict_plus> base_plus_;

        strict_plus(Subject const& subject)
          : base_plus_(subject) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Elements, bool strict_mode = false>
        struct make_plus 
          : make_unary_composite<Elements, plus>
        {};

        template <typename Elements>
        struct make_plus<Elements, true> 
          : make_unary_composite<Elements, strict_plus>
        {};
    }

    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::unary_plus, Elements, Modifiers>
      : detail::make_plus<Elements, detail::get_stricttag<Modifiers>::value>
    {};
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<karma::plus<Subject> >
      : unary_has_semantic_action<Subject> {};

    template <typename Subject>
    struct has_semantic_action<karma::strict_plus<Subject> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::plus<Subject>, Attribute
          , Context, Iterator>
      : mpl::true_ {};

    template <typename Subject, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::strict_plus<Subject>, Attribute
          , Context, Iterator>
      : mpl::true_ {};
}}}

#endif
