///////////////////////////////////////////////////////////////////////////////
// as_set.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_SET_HPP_EAN_04_05_2007
#define BOOST_XPRESSIVE_DETAIL_STATIC_TRANSFORMS_AS_SET_HPP_EAN_04_05_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/assert.hpp>
#include <boost/proto/core.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/static/static.hpp>
#include <boost/xpressive/detail/utility/chset/chset.hpp>
#include <boost/xpressive/detail/utility/traits_utils.hpp>

namespace boost { namespace xpressive { namespace grammar_detail
{

    ///////////////////////////////////////////////////////////////////////////
    // CharLiteral
    template<typename Char>
    struct CharLiteral
      : or_<
            terminal<char>
          , terminal<Char>
        >
    {};

    template<>
    struct CharLiteral<char>
      : terminal<char>
    {};

    ///////////////////////////////////////////////////////////////////////////
    // ListSet
    //  matches expressions like (set= 'a','b','c')
    //  calculates the size of the set
    template<typename Char>
    struct ListSet
      : or_<
            when<
                comma<ListSet<Char>, CharLiteral<Char> >
              , make<mpl::next<call<ListSet<Char>(_left)> > > // TODO make a custom transform for this...
            >
          , when<
                assign<detail::set_initializer_type, CharLiteral<Char> >
              , make<mpl::int_<1> >
            >
        >
    {};

    template<typename Char, typename Traits>
    void fill_list_set(Char *&, detail::set_initializer_type, Traits const &)
    {}

    template<typename Char, typename Expr, typename Traits>
    void fill_list_set(Char *&buffer, Expr const &expr, Traits const &traits)
    {
        fill_list_set(buffer, proto::left(expr), traits);
        *buffer++ = traits.translate(detail::char_cast<Char>(proto::value(proto::right(expr)), traits));
    }

    ///////////////////////////////////////////////////////////////////////////////
    // as_list_set_matcher
    template<typename Char, typename Callable = proto::callable>
    struct as_list_set_matcher : proto::transform<as_list_set_matcher<Char, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::data data_type;
            typedef
                detail::set_matcher<
                    typename data_type::traits_type
                  , typename ListSet<Char>::template impl<Expr, State, Data>::result_type
                >
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                result_type set;
                typedef typename impl::data data_type;
                typename data_type::char_type *buffer = set.set_;
                fill_list_set(buffer, expr, data.traits());
                return set;
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // merge_charset
    //
    template<typename Grammar, typename CharSet, typename Data>
    struct merge_charset
    {
        typedef typename Data::traits_type traits_type;
        typedef typename CharSet::char_type char_type;
        typedef typename CharSet::icase_type icase_type;

        merge_charset(CharSet &charset, Data &data)
          : charset_(charset)
          , visitor_(data)
        {}

        template<typename Expr>
        void operator ()(Expr const &expr) const
        {
            this->call_(expr, typename Expr::proto_tag());
        }

    private:
        merge_charset &operator =(merge_charset const &);

        template<typename Expr, typename Tag>
        void call_(Expr const &expr, Tag) const
        {
            this->set_(
                typename Grammar::template impl<Expr const &, detail::end_xpression, Data &>()(
                    expr
                  , detail::end_xpression()
                  , this->visitor_
                )
            );
        }

        template<typename Expr>
        void call_(Expr const &expr, tag::bitwise_or) const
        {
            (*this)(proto::left(expr));
            (*this)(proto::right(expr));
        }

        template<typename Not>
        void set_(detail::literal_matcher<traits_type, icase_type, Not> const &ch) const
        {
            // BUGBUG fixme!
            BOOST_MPL_ASSERT_NOT((Not));
            set_char(this->charset_.charset_, ch.ch_, this->visitor_.traits(), icase_type());
        }

        void set_(detail::range_matcher<traits_type, icase_type> const &rg) const
        {
            // BUGBUG fixme!
            BOOST_ASSERT(!rg.not_);
            set_range(this->charset_.charset_, rg.ch_min_, rg.ch_max_, this->visitor_.traits(), icase_type());
        }

        template<typename Size>
        void set_(detail::set_matcher<traits_type, Size> const &set_) const
        {
            // BUGBUG fixme!
            BOOST_ASSERT(!set_.not_);
            for(int i = 0; i < Size::value; ++i)
            {
                set_char(this->charset_.charset_, set_.set_[i], this->visitor_.traits(), icase_type());
            }
        }

        void set_(detail::posix_charset_matcher<traits_type> const &posix) const
        {
            set_class(this->charset_.charset_, posix.mask_, posix.not_, this->visitor_.traits());
        }

        CharSet &charset_;
        Data &visitor_;
    };

    ///////////////////////////////////////////////////////////////////////////////
    //
    template<typename Grammar, typename Callable = proto::callable>
    struct as_set_matcher : proto::transform<as_set_matcher<Grammar, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::data data_type;
            typedef typename data_type::char_type char_type;

            // if sizeof(char_type)==1, merge everything into a basic_chset
            // BUGBUG this is not optimal.
            typedef
                typename mpl::if_c<
                    detail::is_narrow_char<char_type>::value
                  , detail::basic_chset<char_type>
                  , detail::compound_charset<typename data_type::traits_type>
                >::type
            charset_type;

            typedef
                detail::charset_matcher<
                    typename data_type::traits_type
                  , typename data_type::icase_type
                  , charset_type
                >
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param data
            ) const
            {
                result_type matcher;
                merge_charset<Grammar, result_type, typename impl::data> merge(matcher, data);
                merge(expr); // Walks the tree and fills in the charset
                return matcher;
            }
        };
    };

}}}

#endif
