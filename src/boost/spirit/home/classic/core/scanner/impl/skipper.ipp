/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
============================================================================*/
#if !defined(BOOST_SPIRIT_SKIPPER_IPP)
#define BOOST_SPIRIT_SKIPPER_IPP

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    struct space_parser;
    template <typename BaseT>
    struct no_skipper_iteration_policy;

    namespace impl
    {
        template <typename ST, typename ScannerT, typename BaseT>
        inline void
        skipper_skip(
            ST const& s,
            ScannerT const& scan,
            skipper_iteration_policy<BaseT> const&)
        {
            typedef scanner_policies<
                no_skipper_iteration_policy<
                    BOOST_DEDUCED_TYPENAME ScannerT::iteration_policy_t>,
                BOOST_DEDUCED_TYPENAME ScannerT::match_policy_t,
                BOOST_DEDUCED_TYPENAME ScannerT::action_policy_t
            > policies_t;

            scanner<BOOST_DEDUCED_TYPENAME ScannerT::iterator_t, policies_t>
                scan2(scan.first, scan.last, policies_t(scan));
            typedef typename ScannerT::iterator_t iterator_t;

            for (;;)
            {
                iterator_t save = scan.first;
                if (!s.parse(scan2))
                {
                    scan.first = save;
                    break;
                }
            }
        }

        template <typename ST, typename ScannerT, typename BaseT>
        inline void
        skipper_skip(
            ST const& s,
            ScannerT const& scan,
            no_skipper_iteration_policy<BaseT> const&)
        {
            for (;;)
            {
                typedef typename ScannerT::iterator_t iterator_t;
                iterator_t save = scan.first;
                if (!s.parse(scan))
                {
                    scan.first = save;
                    break;
                }
            }
        }

        template <typename ST, typename ScannerT>
        inline void
        skipper_skip(
            ST const& s,
            ScannerT const& scan,
            iteration_policy const&)
        {
            for (;;)
            {
                typedef typename ScannerT::iterator_t iterator_t;
                iterator_t save = scan.first;
                if (!s.parse(scan))
                {
                    scan.first = save;
                    break;
                }
            }
        }

        template <typename SkipT>
        struct phrase_parser
        {
            template <typename IteratorT, typename ParserT>
            static parse_info<IteratorT>
            parse(
                IteratorT const&    first_,
                IteratorT const&    last,
                ParserT const&      p,
                SkipT const&        skip)
            {
                typedef skip_parser_iteration_policy<SkipT> it_policy_t;
                typedef scanner_policies<it_policy_t> scan_policies_t;
                typedef scanner<IteratorT, scan_policies_t> scanner_t;

                it_policy_t iter_policy(skip);
                scan_policies_t policies(iter_policy);
                IteratorT first = first_;
                scanner_t scan(first, last, policies);
                match<nil_t> hit = p.parse(scan);
                return parse_info<IteratorT>(
                    first, hit, hit && (first == last),
                    hit.length());
            }
        };

        template <>
        struct phrase_parser<space_parser>
        {
            template <typename IteratorT, typename ParserT>
            static parse_info<IteratorT>
            parse(
                IteratorT const&    first_,
                IteratorT const&    last,
                ParserT const&      p,
                space_parser const&)
            {
                typedef skipper_iteration_policy<> it_policy_t;
                typedef scanner_policies<it_policy_t> scan_policies_t;
                typedef scanner<IteratorT, scan_policies_t> scanner_t;

                IteratorT first = first_;
                scanner_t scan(first, last);
                match<nil_t> hit = p.parse(scan);
                return parse_info<IteratorT>(
                    first, hit, hit && (first == last),
                    hit.length());
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Free parse functions using the skippers
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename IteratorT, typename ParserT, typename SkipT>
    inline parse_info<IteratorT>
    parse(
        IteratorT const&        first,
        IteratorT const&        last,
        parser<ParserT> const&  p,
        parser<SkipT> const&    skip)
    {
        return impl::phrase_parser<SkipT>::
            parse(first, last, p.derived(), skip.derived());
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //
    //  Parse function for null terminated strings using the skippers
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT, typename ParserT, typename SkipT>
    inline parse_info<CharT const*>
    parse(
        CharT const*            str,
        parser<ParserT> const&  p,
        parser<SkipT> const&    skip)
    {
        CharT const* last = str;
        while (*last)
            last++;
        return parse(str, last, p, skip);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif

