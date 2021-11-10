/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    Persistent application configuration

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_WAVE_CONFIG_CONSTANT_HPP)
#define BOOST_WAVE_CONFIG_CONSTANT_HPP

#include <boost/preprocessor/stringize.hpp>
#include <boost/wave/wave_config.hpp>

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS != 0
#define BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS_CONFIG 0x00000001
#else
#define BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS_CONFIG 0x00000000
#endif

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
#define BOOST_WAVE_SUPPORT_PRAGMA_ONCE_CONFIG 0x00000002
#else
#define BOOST_WAVE_SUPPORT_PRAGMA_ONCE_CONFIG 0x00000000
#endif

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WAVE_SUPPORT_MS_EXTENSIONS != 0
#define BOOST_WAVE_SUPPORT_MS_EXTENSIONS_CONFIG 0x00000004
#else
#define BOOST_WAVE_SUPPORT_MS_EXTENSIONS_CONFIG 0x00000000
#endif

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WAVE_PREPROCESS_PRAGMA_BODY != 0
#define BOOST_WAVE_PREPROCESS_PRAGMA_BODY_CONFIG 0x00000008
#else
#define BOOST_WAVE_PREPROCESS_PRAGMA_BODY_CONFIG 0x00000000
#endif

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WAVE_USE_STRICT_LEXER != 0
#define BOOST_WAVE_USE_STRICT_LEXER_CONFIG 0x00000010
#else
#define BOOST_WAVE_USE_STRICT_LEXER_CONFIG 0x00000000
#endif

///////////////////////////////////////////////////////////////////////////////
#if BOOST_WAVE_SUPPORT_IMPORT_KEYWORD != 0
#define BOOST_WAVE_SUPPORT_IMPORT_KEYWORD_CONFIG 0x00000020
#else
#define BOOST_WAVE_SUPPORT_IMPORT_KEYWORD_CONFIG 0x00000000
#endif

///////////////////////////////////////////////////////////////////////////////
#define BOOST_WAVE_CONFIG (                                                   \
        BOOST_WAVE_SUPPORT_VARIADICS_PLACEMARKERS_CONFIG |                    \
        BOOST_WAVE_SUPPORT_PRAGMA_ONCE_CONFIG |                               \
        BOOST_WAVE_SUPPORT_MS_EXTENSIONS_CONFIG |                             \
        BOOST_WAVE_PREPROCESS_PRAGMA_BODY_CONFIG |                            \
        BOOST_WAVE_USE_STRICT_LEXER_CONFIG |                                  \
        BOOST_WAVE_SUPPORT_IMPORT_KEYWORD_CONFIG                              \
    )                                                                         \
    /**/

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace wave {

    ///////////////////////////////////////////////////////////////////////////
    //  Call this function to test the configuration of the calling application
    //  against the configuration of the linked library.
    BOOST_WAVE_DECL bool test_configuration(unsigned int config,
        char const* pragma_keyword, char const* string_type);

///////////////////////////////////////////////////////////////////////////////
}}  // namespace boost::wave

#define BOOST_WAVE_TEST_CONFIGURATION()                                       \
        boost::wave::test_configuration(                                      \
            BOOST_WAVE_CONFIG,                                                \
            BOOST_WAVE_PRAGMA_KEYWORD,                                        \
            BOOST_PP_STRINGIZE((BOOST_WAVE_STRINGTYPE))                       \
        )                                                                     \
    /**/

#endif // !BOOST_WAVE_CONFIG_CONSTANT_HPP
