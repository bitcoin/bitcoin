/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_SKIPPER_HPP)
#define BOOST_SPIRIT_SKIPPER_HPP

///////////////////////////////////////////////////////////////////////////////
#include <cctype>

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/scanner/scanner.hpp>
#include <boost/spirit/home/classic/core/primitives/impl/primitives.ipp>

#include <boost/spirit/home/classic/core/scanner/skipper_fwd.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  skipper_iteration_policy class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename BaseT>
    struct skipper_iteration_policy : public BaseT
    {
        typedef BaseT base_t;
    
        skipper_iteration_policy()
        : BaseT() {}
    
        template <typename PolicyT>
        skipper_iteration_policy(PolicyT const& other)
        : BaseT(other) {}
    
        template <typename ScannerT>
        void
        advance(ScannerT const& scan) const
        {
            BaseT::advance(scan);
            scan.skip(scan);
        }
    
        template <typename ScannerT>
        bool
        at_end(ScannerT const& scan) const
        {
            scan.skip(scan);
            return BaseT::at_end(scan);
        }
    
        template <typename ScannerT>
        void
        skip(ScannerT const& scan) const
        {
            while (!BaseT::at_end(scan) && impl::isspace_(BaseT::get(scan)))
                BaseT::advance(scan);
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    //
    //  no_skipper_iteration_policy class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename BaseT>
    struct no_skipper_iteration_policy : public BaseT
    {
        typedef BaseT base_t;

        no_skipper_iteration_policy()
        : BaseT() {}

        template <typename PolicyT>
        no_skipper_iteration_policy(PolicyT const& other)
        : BaseT(other) {}

        template <typename ScannerT>
        void
        skip(ScannerT const& /*scan*/) const {}
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  skip_parser_iteration_policy class
    //
    ///////////////////////////////////////////////////////////////////////////
    namespace impl
    {
        template <typename ST, typename ScannerT, typename BaseT>
        void
        skipper_skip(
            ST const& s,
            ScannerT const& scan,
            skipper_iteration_policy<BaseT> const&);

        template <typename ST, typename ScannerT, typename BaseT>
        void
        skipper_skip(
            ST const& s,
            ScannerT const& scan,
            no_skipper_iteration_policy<BaseT> const&);

        template <typename ST, typename ScannerT>
        void
        skipper_skip(
            ST const& s,
            ScannerT const& scan,
            iteration_policy const&);
    }

    template <typename ParserT, typename BaseT>
    class skip_parser_iteration_policy : public skipper_iteration_policy<BaseT>
    {
    public:
    
        typedef skipper_iteration_policy<BaseT> base_t;
    
        skip_parser_iteration_policy(
            ParserT const& skip_parser,
            base_t const& base = base_t())
        : base_t(base), subject(skip_parser) {}
    
        template <typename PolicyT>
        skip_parser_iteration_policy(PolicyT const& other)
        : base_t(other), subject(other.skipper()) {}
    
        template <typename ScannerT>
        void
        skip(ScannerT const& scan) const
        {
            impl::skipper_skip(subject, scan, scan);
        }
    
        ParserT const&
        skipper() const
        { 
            return subject; 
        }
    
    private:
    
        ParserT const& subject;
    };
    
    ///////////////////////////////////////////////////////////////////////////////
    //
    //  Free parse functions using the skippers
    //
    ///////////////////////////////////////////////////////////////////////////////
    template <typename IteratorT, typename ParserT, typename SkipT>
    parse_info<IteratorT>
    parse(
        IteratorT const&        first,
        IteratorT const&        last,
        parser<ParserT> const&  p,
        parser<SkipT> const&    skip);
    
    ///////////////////////////////////////////////////////////////////////////////
    //
    //  Parse function for null terminated strings using the skippers
    //
    ///////////////////////////////////////////////////////////////////////////////
    template <typename CharT, typename ParserT, typename SkipT>
    parse_info<CharT const*>
    parse(
        CharT const*            str,
        parser<ParserT> const&  p,
        parser<SkipT> const&    skip);
    
    ///////////////////////////////////////////////////////////////////////////////
    //
    //  phrase_scanner_t and wide_phrase_scanner_t
    //
    //      The most common scanners. Use these typedefs when you need
    //      a scanner that skips white spaces.
    //
    ///////////////////////////////////////////////////////////////////////////////
    typedef skipper_iteration_policy<>                  iter_policy_t;
    typedef scanner_policies<iter_policy_t>             scanner_policies_t;
    typedef scanner<char const*, scanner_policies_t>    phrase_scanner_t;
    typedef scanner<wchar_t const*, scanner_policies_t> wide_phrase_scanner_t;
    
    ///////////////////////////////////////////////////////////////////////////////

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#include <boost/spirit/home/classic/core/scanner/impl/skipper.ipp>
#endif

