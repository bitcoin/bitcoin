/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2013-2014 Agustin Berge

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_AUXILIARY_ANY_PARSER_APR_09_2014_1145PM)
#define BOOST_SPIRIT_X3_AUXILIARY_ANY_PARSER_APR_09_2014_1145PM

#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/support/subcontext.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/support/traits/container_traits.hpp>
#include <boost/spirit/home/x3/support/traits/has_attribute.hpp>
#include <boost/spirit/home/x3/support/traits/move_to.hpp>
#include <boost/spirit/home/x3/support/traits/is_parser.hpp>
#include <memory>
#include <string>

namespace boost { namespace spirit { namespace x3
{
    template <
        typename Iterator
      , typename Attribute = unused_type
      , typename Context = subcontext<>>
    struct any_parser : parser<any_parser<Iterator, Attribute, Context>>
    {
        typedef Attribute attribute_type;

        static bool const has_attribute =
            !is_same<unused_type, attribute_type>::value;
        static bool const handles_container =
            traits::is_container<Attribute>::value;

    public:
        any_parser() = default;

        template <typename Expr,
            typename Enable = typename enable_if<traits::is_parser<Expr>>::type>
        any_parser(Expr const& expr)
          : _content(new holder<Expr>(expr)) {}

        any_parser(any_parser const& other)
          : _content(other._content ? other._content->clone() : nullptr) {}

        any_parser(any_parser&& other) = default;

        any_parser& operator=(any_parser const& other)
        {
            _content.reset(other._content ? other._content->clone() : nullptr);
            return *this;
        }

        any_parser& operator=(any_parser&& other) = default;

        template <typename Iterator_, typename Context_>
        bool parse(Iterator_& first, Iterator_ const& last
          , Context_ const& context, unused_type, Attribute& attr) const
        {
            BOOST_STATIC_ASSERT_MSG(
                (is_same<Iterator, Iterator_>::value)
              , "Incompatible iterator used"
            );

            BOOST_ASSERT_MSG(
                (_content != nullptr)
              , "Invalid use of uninitialized any_parser"
            );

            return _content->parse(first, last, context, attr);
        }

        template <typename Iterator_, typename Context_, typename Attribute_>
        bool parse(Iterator_& first, Iterator_ const& last
          , Context_ const& context, unused_type, Attribute_& attr_) const
        {
            Attribute attr;
            if (parse(first, last, context, unused, attr))
            {
                traits::move_to(attr, attr_);
                return true;
            }
            return false;
        }

        std::string get_info() const
        {
            return _content ? _content->get_info() : "";
        }

    private:

        struct placeholder
        {
            virtual placeholder* clone() const = 0;

            virtual bool parse(Iterator& first, Iterator const& last
              , Context const& context, Attribute& attr) const = 0;

            virtual std::string get_info() const = 0;

            virtual ~placeholder() {}
        };

        template <typename Expr>
        struct holder : placeholder
        {
            typedef typename extension::as_parser<Expr>::value_type parser_type;

            explicit holder(Expr const& p)
              : _parser(as_parser(p)) {}

            holder* clone() const override
            {
                return new holder(*this);
            }

            bool parse(Iterator& first, Iterator const& last
              , Context const& context, Attribute& attr) const override
            {
                return _parser.parse(first, last, context, unused, attr);
            }

            std::string get_info() const override
            {
                return x3::what(_parser);
            }

            parser_type _parser;
        };

    private:
        std::unique_ptr<placeholder> _content;
    };

    template <typename Iterator, typename Attribute, typename Context>
    struct get_info<any_parser<Iterator, Attribute, Context>>
    {
        typedef std::string result_type;
        std::string operator()(
            any_parser<Iterator, Attribute, Context> const& p) const
        {
            return p.get_info();
        }
    };
}}}

#endif
