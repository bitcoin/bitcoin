/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_PARSER_NAMES_IPP)
#define BOOST_SPIRIT_PARSER_NAMES_IPP

#if defined(BOOST_SPIRIT_DEBUG)

#include <string>
#include <iostream>
#include <map>

#include <boost/config.hpp>
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#define BOOST_SPIRIT_SSTREAM std::strstream
std::string BOOST_SPIRIT_GETSTRING(std::strstream& ss)
{
    ss << ends;
    std::string rval = ss.str();
    ss.freeze(false);
    return rval;
}
#else
#include <sstream>
#define BOOST_SPIRIT_GETSTRING(ss) ss.str()
#define BOOST_SPIRIT_SSTREAM std::stringstream
#endif

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//  from actions.hpp
    template <typename ParserT, typename ActionT>
    inline std::string
    parser_name(action<ParserT, ActionT> const& p)
    {
        return std::string("action")
            + std::string("[")
            + parser_name(p.subject())
            + std::string("]");
    }

///////////////////////////////////////////////////////////////////////////////
//  from directives.hpp
    template <typename ParserT>
    inline std::string
    parser_name(contiguous<ParserT> const& p)
    {
        return std::string("contiguous")
            + std::string("[")
            + parser_name(p.subject())
            + std::string("]");
    }

    template <typename ParserT>
    inline std::string
    parser_name(inhibit_case<ParserT> const& p)
    {
        return std::string("inhibit_case")
            + std::string("[")
            + parser_name(p.subject())
            + std::string("]");
    }

    template <typename A, typename B>
    inline std::string
    parser_name(longest_alternative<A, B> const& p)
    {
        return std::string("longest_alternative")
            + std::string("[")
            + parser_name(p.left()) + std::string(", ") + parser_name(p.right())
            + std::string("]");
    }

    template <typename A, typename B>
    inline std::string
    parser_name(shortest_alternative<A, B> const& p)
    {
        return std::string("shortest_alternative")
            + std::string("[")
            + parser_name(p.left()) + std::string(", ") + parser_name(p.right())
            + std::string("]");
    }

///////////////////////////////////////////////////////////////////////////////
//  from numerics.hpp
    template <typename T, int Radix, unsigned MinDigits, int MaxDigits>
    inline std::string
    parser_name(uint_parser<T, Radix, MinDigits, MaxDigits> const& /*p*/)
    {
        BOOST_SPIRIT_SSTREAM stream;
        stream << Radix << ", " << MinDigits << ", " << MaxDigits;
        return std::string("uint_parser<")
            + BOOST_SPIRIT_GETSTRING(stream)
            + std::string(">");
    }

    template <typename T, int Radix, unsigned MinDigits, int MaxDigits>
    inline std::string
    parser_name(int_parser<T, Radix, MinDigits, MaxDigits> const& /*p*/)
    {
        BOOST_SPIRIT_SSTREAM stream;
        stream << Radix << ", " << MinDigits << ", " << MaxDigits;
        return std::string("int_parser<")
            + BOOST_SPIRIT_GETSTRING(stream)
            + std::string(">");
    }

    template <typename T, typename RealPoliciesT>
    inline std::string
    parser_name(real_parser<T, RealPoliciesT> const& /*p*/)
    {
        return std::string("real_parser");
    }

///////////////////////////////////////////////////////////////////////////////
//  from operators.hpp
    template <typename A, typename B>
    inline std::string
    parser_name(sequence<A, B> const& p)
    {
        return std::string("sequence")
            + std::string("[")
            + parser_name(p.left()) + std::string(", ") + parser_name(p.right())
            + std::string("]");
    }

    template <typename A, typename B>
    inline std::string
    parser_name(sequential_or<A, B> const& p)
    {
        return std::string("sequential_or")
            + std::string("[")
            + parser_name(p.left()) + std::string(", ") + parser_name(p.right())
            + std::string("]");
    }

    template <typename A, typename B>
    inline std::string
    parser_name(alternative<A, B> const& p)
    {
        return std::string("alternative")
            + std::string("[")
            + parser_name(p.left()) + std::string(", ") + parser_name(p.right())
            + std::string("]");
    }

    template <typename A, typename B>
    inline std::string
    parser_name(intersection<A, B> const& p)
    {
        return std::string("intersection")
            + std::string("[")
            + parser_name(p.left()) + std::string(", ") + parser_name(p.right())
            + std::string("]");
    }

    template <typename A, typename B>
    inline std::string
    parser_name(difference<A, B> const& p)
    {
        return std::string("difference")
            + std::string("[")
            + parser_name(p.left()) + std::string(", ") + parser_name(p.right())
            + std::string("]");
    }

    template <typename A, typename B>
    inline std::string
    parser_name(exclusive_or<A, B> const& p)
    {
        return std::string("exclusive_or")
            + std::string("[")
            + parser_name(p.left()) + std::string(", ") + parser_name(p.right())
            + std::string("]");
    }

    template <typename S>
    inline std::string
    parser_name(optional<S> const& p)
    {
        return std::string("optional")
            + std::string("[")
            + parser_name(p.subject())
            + std::string("]");
    }

    template <typename S>
    inline std::string
    parser_name(kleene_star<S> const& p)
    {
        return std::string("kleene_star")
            + std::string("[")
            + parser_name(p.subject())
            + std::string("]");
    }

    template <typename S>
    inline std::string
    parser_name(positive<S> const& p)
    {
        return std::string("positive")
            + std::string("[")
            + parser_name(p.subject())
            + std::string("]");
    }

///////////////////////////////////////////////////////////////////////////////
//  from parser.hpp
    template <typename DerivedT>
    inline std::string
    parser_name(parser<DerivedT> const& /*p*/)
    {
        return std::string("parser");
    }

///////////////////////////////////////////////////////////////////////////////
//  from primitives.hpp
    template <typename DerivedT>
    inline std::string
    parser_name(char_parser<DerivedT> const &/*p*/)
    {
        return std::string("char_parser");
    }

    template <typename CharT>
    inline std::string
    parser_name(chlit<CharT> const &p)
    {
        return std::string("chlit(\'")
            + std::string(1, p.ch)
            + std::string("\')");
    }

    template <typename CharT>
    inline std::string
    parser_name(range<CharT> const &p)
    {
        return std::string("range(")
            + std::string(1, p.first) + std::string(", ") + std::string(1, p.last)
            + std::string(")");
    }

    template <typename IteratorT>
    inline std::string
    parser_name(chseq<IteratorT> const &p)
    {
        return std::string("chseq(\"")
            + std::string(p.first, p.last)
            + std::string("\")");
    }

    template <typename IteratorT>
    inline std::string
    parser_name(strlit<IteratorT> const &p)
    {
        return std::string("strlit(\"")
            + std::string(p.seq.first, p.seq.last)
            + std::string("\")");
    }

    inline std::string
    parser_name(nothing_parser const&)
    {
        return std::string("nothing");
    }

    inline std::string
    parser_name(epsilon_parser const&)
    {
        return std::string("epsilon");
    }

    inline std::string
    parser_name(anychar_parser const&)
    {
        return std::string("anychar");
    }

    inline std::string
    parser_name(alnum_parser const&)
    {
        return std::string("alnum");
    }

    inline std::string
    parser_name(alpha_parser const&)
    {
        return std::string("alpha");
    }

    inline std::string
    parser_name(cntrl_parser const&)
    {
        return std::string("cntrl");
    }

    inline std::string
    parser_name(digit_parser const&)
    {
        return std::string("digit");
    }

    inline std::string
    parser_name(graph_parser const&)
    {
        return std::string("graph");
    }

    inline std::string
    parser_name(lower_parser const&)
    {
        return std::string("lower");
    }

    inline std::string
    parser_name(print_parser const&)
    {
        return std::string("print");
    }

    inline std::string
    parser_name(punct_parser const&)
    {
        return std::string("punct");
    }

    inline std::string
    parser_name(blank_parser const&)
    {
        return std::string("blank");
    }

    inline std::string
    parser_name(space_parser const&)
    {
        return std::string("space");
    }

    inline std::string
    parser_name(upper_parser const&)
    {
        return std::string("upper");
    }

    inline std::string
    parser_name(xdigit_parser const&)
    {
        return std::string("xdigit");
    }

    inline std::string
    parser_name(eol_parser const&)
    {
        return std::string("eol");
    }

    inline std::string
    parser_name(end_parser const&)
    {
        return std::string("end");
    }

///////////////////////////////////////////////////////////////////////////////
//  from rule.hpp
    namespace impl {
        struct node_registry
        {
            typedef std::pair<std::string, bool> rule_info;
            typedef std::map<void const *, rule_info> rule_infos;

            std::string find_node(void const *r)
            {
                rule_infos::const_iterator cit = infos.find(r);
                if (cit != infos.end())
                    return (*cit).second.first;
                return std::string("<unknown>");
            }

            bool trace_node(void const *r)
            {
                rule_infos::const_iterator cit = infos.find(r);
                if (cit != infos.end())
                    return (*cit).second.second;
                return BOOST_SPIRIT_DEBUG_TRACENODE;
            }

            bool register_node(void const *r, char const *name_to_register,
                bool trace_node)
            {
                if (infos.find(r) != infos.end())
                    return false;

                return infos.insert(rule_infos::value_type(r,
                    rule_info(std::string(name_to_register), trace_node))
                ).second;
            }

            bool unregister_node(void const *r)
            {
                if (infos.find(r) == infos.end())
                    return false;
                return (1 == infos.erase(r));
            }

        private:
            rule_infos infos;
        };

        inline node_registry &
        get_node_registry()
        {
            static node_registry node_infos;
            return node_infos;
        }
    }   // namespace impl

    template<
        typename DerivedT, typename EmbedT, 
        typename T0, typename T1, typename T2
    >
    inline std::string
    parser_name(impl::rule_base<DerivedT, EmbedT, T0, T1, T2> const& p)
    {
        return std::string("rule_base")
            + std::string("(")
            + impl::get_node_registry().find_node(&p)
            + std::string(")");
    }

    template<typename T0, typename T1, typename T2>
    inline std::string
    parser_name(rule<T0, T1, T2> const& p)
    {
        return std::string("rule")
            + std::string("(")
            + impl::get_node_registry().find_node(&p)
            + std::string(")");
    }

///////////////////////////////////////////////////////////////////////////////
//  from subrule.hpp
    template <typename FirstT, typename RestT>
    inline std::string
    parser_name(subrule_list<FirstT, RestT> const &p)
    {
        return std::string("subrule_list")
            + std::string("(")
            + impl::get_node_registry().find_node(&p)
            + std::string(")");
    }

    template <int ID, typename DefT, typename ContextT>
    inline std::string
    parser_name(subrule_parser<ID, DefT, ContextT> const &p)
    {
        return std::string("subrule_parser")
            + std::string("(")
            + impl::get_node_registry().find_node(&p)
            + std::string(")");
    }

    template <int ID, typename ContextT>
    inline std::string
    parser_name(subrule<ID, ContextT> const &p)
    {
        BOOST_SPIRIT_SSTREAM stream;
        stream << ID;
        return std::string("subrule<")
            + BOOST_SPIRIT_GETSTRING(stream)
            + std::string(">(")
            + impl::get_node_registry().find_node(&p)
            + std::string(")");
    }

///////////////////////////////////////////////////////////////////////////////
//  from grammar.hpp
    template <typename DerivedT, typename ContextT>
    inline std::string
    parser_name(grammar<DerivedT, ContextT> const& p)
    {
        return std::string("grammar")
            + std::string("(")
            + impl::get_node_registry().find_node(&p)
            + std::string(")");
    }

///////////////////////////////////////////////////////////////////////////////
//  decide, if a node is to be traced or not
    template<
        typename DerivedT, typename EmbedT, 
        typename T0, typename T1, typename T2
    >
    inline bool
    trace_parser(impl::rule_base<DerivedT, EmbedT, T0, T1, T2> 
        const& p)
    {
        return impl::get_node_registry().trace_node(&p);
    }

    template<typename T0, typename T1, typename T2>
    inline bool
    trace_parser(rule<T0, T1, T2> const& p)
    {
        return impl::get_node_registry().trace_node(&p);
    }

    template <typename DerivedT, typename ContextT>
    inline bool
    trace_parser(grammar<DerivedT, ContextT> const& p)
    {
        return impl::get_node_registry().trace_node(&p);
    }

    template <typename DerivedT, int N, typename ContextT>
    inline bool
    trace_parser(impl::entry_grammar<DerivedT, N, ContextT> const& p)
    {
        return impl::get_node_registry().trace_node(&p);
    }

    template <int ID, typename ContextT>
    bool
    trace_parser(subrule<ID, ContextT> const& p)
    {
        return impl::get_node_registry().trace_node(&p);
    }

    template <typename ParserT, typename ActorTupleT>
    bool
    trace_parser(init_closure_parser<ParserT, ActorTupleT> const& p)
    {
        return impl::get_node_registry().trace_node(&p);
    }

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#undef BOOST_SPIRIT_SSTREAM
#undef BOOST_SPIRIT_GETSTRING

#endif // defined(BOOST_SPIRIT_DEBUG)

#endif // !defined(BOOST_SPIRIT_PARSER_NAMES_IPP)
