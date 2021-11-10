/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    Copyright (c) 2003 Gustavo Guerra
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_DEBUG_NODE_HPP)
#define BOOST_SPIRIT_DEBUG_NODE_HPP

#if !defined(BOOST_SPIRIT_DEBUG_MAIN_HPP)
#error "You must include boost/spirit/debug.hpp, not boost/spirit/debug/debug_node.hpp"
#endif

#if defined(BOOST_SPIRIT_DEBUG)

#include <string>

#include <boost/type_traits/is_convertible.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/primitives/primitives.hpp> // for iscntrl_

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  Debug helper classes for rules, which ensure maximum non-intrusiveness of
//  the Spirit debug support
//
///////////////////////////////////////////////////////////////////////////////

namespace impl {

    struct token_printer_aux_for_chars
    {
        template<typename CharT>
        static void print(std::ostream& o, CharT c)
        {
            if (c == static_cast<CharT>('\a'))
                o << "\\a";

            else if (c == static_cast<CharT>('\b'))
                o << "\\b";

            else if (c == static_cast<CharT>('\f'))
                o << "\\f";

            else if (c == static_cast<CharT>('\n'))
                o << "\\n";

            else if (c == static_cast<CharT>('\r'))
                o << "\\r";

            else if (c == static_cast<CharT>('\t'))
                o << "\\t";

            else if (c == static_cast<CharT>('\v'))
                o << "\\v";

            else if (iscntrl_(c))
                o << "\\" << static_cast<int>(c);

            else
                o << static_cast<char>(c);
        }
    };

    // for token types where the comparison with char constants wouldn't work
    struct token_printer_aux_for_other_types
    {
        template<typename CharT>
        static void print(std::ostream& o, CharT c)
        {
            o << c;
        }
    };

    template <typename CharT>
    struct token_printer_aux
    :   mpl::if_<
            mpl::and_<
                is_convertible<CharT, char>,
                is_convertible<char, CharT> >,
            token_printer_aux_for_chars,
            token_printer_aux_for_other_types
        >::type
    {
    };

    template<typename CharT>
    inline void token_printer(std::ostream& o, CharT c)
    {
    #if !defined(BOOST_SPIRIT_DEBUG_TOKEN_PRINTER)

        token_printer_aux<CharT>::print(o, c);

    #else

        BOOST_SPIRIT_DEBUG_TOKEN_PRINTER(o, c);

    #endif
    }

///////////////////////////////////////////////////////////////////////////////
//
//  Dump infos about the parsing state of a rule
//
///////////////////////////////////////////////////////////////////////////////

#if BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES
    template <typename IteratorT>
    inline void
    print_node_info(bool hit, int level, bool close, std::string const& name,
        IteratorT first, IteratorT last)
    {
        if (!name.empty())
        {
            for (int i = 0; i < level; ++i)
                BOOST_SPIRIT_DEBUG_OUT << "  ";
            if (close)
            {
                if (hit)
                    BOOST_SPIRIT_DEBUG_OUT << "/";
                else
                    BOOST_SPIRIT_DEBUG_OUT << "#";
            }
            BOOST_SPIRIT_DEBUG_OUT << name << ":\t\"";
            IteratorT iter = first;
            IteratorT ilast = last;
            for (int j = 0; j < BOOST_SPIRIT_DEBUG_PRINT_SOME; ++j)
            {
                if (iter == ilast)
                    break;

                token_printer(BOOST_SPIRIT_DEBUG_OUT, *iter);
                ++iter;
            }
            BOOST_SPIRIT_DEBUG_OUT << "\"\n";
        }
    }
#endif  // BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES

#if BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_CLOSURES
    template <typename ResultT>
    inline ResultT &
    print_closure_info(ResultT &hit, int level, std::string const& name)
    {
        if (!name.empty())
        {
            for (int i = 0; i < level-1; ++i)
                BOOST_SPIRIT_DEBUG_OUT << "  ";

        // for now, print out the return value only
            BOOST_SPIRIT_DEBUG_OUT << "^" << name << ":\t";
            if (hit.has_valid_attribute())
                BOOST_SPIRIT_DEBUG_OUT << hit.value();
            else
                BOOST_SPIRIT_DEBUG_OUT << "undefined attribute";
            BOOST_SPIRIT_DEBUG_OUT << "\n";
        }
        return hit;
    }
#endif // BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_CLOSURES

}

///////////////////////////////////////////////////////////////////////////////
//
//  Implementation note: The parser_context_linker, parser_scanner_linker and
//  closure_context_linker classes are wrapped by a PP constant to allow
//  redefinition of this classes outside of Spirit
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_PARSER_CONTEXT_LINKER_DEFINED)
#define BOOST_SPIRIT_PARSER_CONTEXT_LINKER_DEFINED

    ///////////////////////////////////////////////////////////////////////////
    //
    //  parser_context_linker is a debug wrapper for the ContextT template
    //  parameter of the rule<>, subrule<> and the grammar<> classes
    //
    ///////////////////////////////////////////////////////////////////////////
    template<typename ContextT>
    struct parser_context_linker : public ContextT
    {
        typedef ContextT base_t;

        template <typename ParserT>
        parser_context_linker(ParserT const& p)
        : ContextT(p) {}

        template <typename ParserT, typename ScannerT>
        void pre_parse(ParserT const& p, ScannerT &scan)
        {
            this->base_t::pre_parse(p, scan);

#if BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES
            if (trace_parser(p.derived())) {
                impl::print_node_info(
                    false,
                    scan.get_level(),
                    false,
                    parser_name(p.derived()),
                    scan.first,
                    scan.last);
            }
            scan.get_level()++;
#endif  // BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES
        }

        template <typename ResultT, typename ParserT, typename ScannerT>
        ResultT& post_parse(ResultT& hit, ParserT const& p, ScannerT &scan)
        {
#if BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES
            --scan.get_level();
            if (trace_parser(p.derived())) {
                impl::print_node_info(
                    hit,
                    scan.get_level(),
                    true,
                    parser_name(p.derived()),
                    scan.first,
                    scan.last);
            }
#endif  // BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_NODES

            return this->base_t::post_parse(hit, p, scan);
        }
    };

#endif // !defined(BOOST_SPIRIT_PARSER_CONTEXT_LINKER_DEFINED)

#if !defined(BOOST_SPIRIT_PARSER_SCANNER_LINKER_DEFINED)
#define BOOST_SPIRIT_PARSER_SCANNER_LINKER_DEFINED

///////////////////////////////////////////////////////////////////////////////
//  This class is to avoid linker problems and to ensure a real singleton
//  'level' variable
    struct debug_support
    {
        int& get_level()
        {
            static int level = 0;
            return level;
        }
    };

    template<typename ScannerT>
    struct parser_scanner_linker : public ScannerT
    {
        parser_scanner_linker(ScannerT const &scan_) : ScannerT(scan_)
        {}

        int &get_level()
        { return debug.get_level(); }

        private: debug_support debug;
    };

#endif // !defined(BOOST_SPIRIT_PARSER_SCANNER_LINKER_DEFINED)

#if !defined(BOOST_SPIRIT_CLOSURE_CONTEXT_LINKER_DEFINED)
#define BOOST_SPIRIT_CLOSURE_CONTEXT_LINKER_DEFINED

    ///////////////////////////////////////////////////////////////////////////
    //
    //  closure_context_linker is a debug wrapper for the closure template
    //  parameter of the rule<>, subrule<> and grammar classes
    //
    ///////////////////////////////////////////////////////////////////////////

    template<typename ContextT>
    struct closure_context_linker : public parser_context_linker<ContextT>
    {
        typedef parser_context_linker<ContextT> base_t;

        template <typename ParserT>
        closure_context_linker(ParserT const& p)
        : parser_context_linker<ContextT>(p) {}

        template <typename ParserT, typename ScannerT>
        void pre_parse(ParserT const& p, ScannerT &scan)
        { this->base_t::pre_parse(p, scan); }

        template <typename ResultT, typename ParserT, typename ScannerT>
        ResultT&
        post_parse(ResultT& hit, ParserT const& p, ScannerT &scan)
        {
#if BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_CLOSURES
            if (hit && trace_parser(p.derived())) {
            // for now, print out the return value only
                return impl::print_closure_info(
                    this->base_t::post_parse(hit, p, scan),
                    scan.get_level(),
                    parser_name(p.derived())
                );
            }
#endif // BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_CLOSURES

            return this->base_t::post_parse(hit, p, scan);
        }
    };

#endif // !defined(BOOST_SPIRIT_CLOSURE_CONTEXT_LINKER_DEFINED)

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // defined(BOOST_SPIRIT_DEBUG)

#endif // !defined(BOOST_SPIRIT_DEBUG_NODE_HPP)

