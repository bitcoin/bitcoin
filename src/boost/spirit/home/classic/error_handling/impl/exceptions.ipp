/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_EXCEPTIONS_IPP
#define BOOST_SPIRIT_EXCEPTIONS_IPP

namespace boost { namespace spirit { 

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

namespace impl {

#ifdef __BORLANDC__
    template <typename ParserT, typename ScannerT>
    typename parser_result<ParserT, ScannerT>::type
    fallback_parser_helper(ParserT const& subject, ScannerT const& scan);
#endif

    template <typename RT, typename ParserT, typename ScannerT>
    RT fallback_parser_parse(ParserT const& p, ScannerT const& scan)
    {
        typedef typename ScannerT::iterator_t iterator_t;
        typedef typename RT::attr_t attr_t;
        typedef error_status<attr_t> error_status_t;
        typedef typename ParserT::error_descr_t error_descr_t;

        iterator_t save = scan.first;
        error_status_t hr(error_status_t::retry);

        while (hr.result == error_status_t::retry)
        {
            try
            {
            #ifndef __BORLANDC__
                return p.subject().parse(scan);
            #else
                return impl::fallback_parser_helper(p, scan);
            #endif
            }

            catch (parser_error<error_descr_t, iterator_t>& error)
            {
                scan.first = save;
                hr = p.handler(scan, error);
                switch (hr.result)
                {
                    case error_status_t::fail:
                        return scan.no_match();
                    case error_status_t::accept:
                        return scan.create_match
                            (std::size_t(hr.length), hr.value, save, scan.first);
                    case error_status_t::rethrow:
                         boost::throw_exception(error);
                    default:
                        continue;
                }
            }
        }
        return scan.no_match();
    }

///////////////////////////////////////////////////////////////////////////
//
//  Borland does not like calling the subject directly in the try block.
//  Removing the #ifdef __BORLANDC__ code makes Borland complain that
//  some variables and types cannot be found in the catch block. Weird!
//
///////////////////////////////////////////////////////////////////////////
#ifdef __BORLANDC__

    template <typename ParserT, typename ScannerT>
    typename parser_result<ParserT, ScannerT>::type
    fallback_parser_helper(ParserT const& p, ScannerT const& scan)
    {
        return p.subject().parse(scan);
    }

#endif

}

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit::impl

///////////////////////////////////////////////////////////////////////////////
#endif

