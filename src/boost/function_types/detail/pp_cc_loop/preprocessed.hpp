
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

// no include guards, this file is intended for multiple inclusions

// this file has been generated from the master.hpp file in the same directory
# define BOOST_FT_cc_id 1
# define BOOST_FT_cc_name implicit_cc
# define BOOST_FT_cc BOOST_PP_EMPTY
# define BOOST_FT_cond BOOST_FT_CC_IMPLICIT
# if BOOST_FT_cond
# define BOOST_FT_config_valid 1
# include BOOST_FT_cc_file
# endif
# undef BOOST_FT_cond
# undef BOOST_FT_cc_name
# undef BOOST_FT_cc
# undef BOOST_FT_cc_id
# define BOOST_FT_cc_id 2
# define BOOST_FT_cc_name cdecl_cc
# define BOOST_FT_cc BOOST_PP_IDENTITY(__cdecl )
# define BOOST_FT_cond BOOST_FT_CC_CDECL
# if BOOST_FT_cond
# define BOOST_FT_config_valid 1
# include BOOST_FT_cc_file
# endif
# undef BOOST_FT_cond
# undef BOOST_FT_cc_name
# undef BOOST_FT_cc
# undef BOOST_FT_cc_id
# define BOOST_FT_cc_id 3
# define BOOST_FT_cc_name stdcall_cc
# define BOOST_FT_cc BOOST_PP_IDENTITY(__stdcall )
# define BOOST_FT_cond BOOST_FT_CC_STDCALL
# if BOOST_FT_cond
# define BOOST_FT_config_valid 1
# include BOOST_FT_cc_file
# endif
# undef BOOST_FT_cond
# undef BOOST_FT_cc_name
# undef BOOST_FT_cc
# undef BOOST_FT_cc_id
# define BOOST_FT_cc_id 4
# define BOOST_FT_cc_name pascal_cc
# define BOOST_FT_cc BOOST_PP_IDENTITY(pascal )
# define BOOST_FT_cond BOOST_FT_CC_PASCAL
# if BOOST_FT_cond
# define BOOST_FT_config_valid 1
# include BOOST_FT_cc_file
# endif
# undef BOOST_FT_cond
# undef BOOST_FT_cc_name
# undef BOOST_FT_cc
# undef BOOST_FT_cc_id
# define BOOST_FT_cc_id 5
# define BOOST_FT_cc_name fastcall_cc
# define BOOST_FT_cc BOOST_PP_IDENTITY(__fastcall)
# define BOOST_FT_cond BOOST_FT_CC_FASTCALL
# if BOOST_FT_cond
# define BOOST_FT_config_valid 1
# include BOOST_FT_cc_file
# endif
# undef BOOST_FT_cond
# undef BOOST_FT_cc_name
# undef BOOST_FT_cc
# undef BOOST_FT_cc_id
# define BOOST_FT_cc_id 6
# define BOOST_FT_cc_name clrcall_cc
# define BOOST_FT_cc BOOST_PP_IDENTITY(__clrcall )
# define BOOST_FT_cond BOOST_FT_CC_CLRCALL
# if BOOST_FT_cond
# define BOOST_FT_config_valid 1
# include BOOST_FT_cc_file
# endif
# undef BOOST_FT_cond
# undef BOOST_FT_cc_name
# undef BOOST_FT_cc
# undef BOOST_FT_cc_id
# define BOOST_FT_cc_id 7
# define BOOST_FT_cc_name thiscall_cc
# define BOOST_FT_cc BOOST_PP_IDENTITY(__thiscall)
# define BOOST_FT_cond BOOST_FT_CC_THISCALL
# if BOOST_FT_cond
# define BOOST_FT_config_valid 1
# include BOOST_FT_cc_file
# endif
# undef BOOST_FT_cond
# undef BOOST_FT_cc_name
# undef BOOST_FT_cc
# undef BOOST_FT_cc_id
# define BOOST_FT_cc_id 8
# define BOOST_FT_cc_name thiscall_cc
# define BOOST_FT_cc BOOST_PP_EMPTY
# define BOOST_FT_cond BOOST_FT_CC_IMPLICIT_THISCALL
# if BOOST_FT_cond
# define BOOST_FT_config_valid 1
# include BOOST_FT_cc_file
# endif
# undef BOOST_FT_cond
# undef BOOST_FT_cc_name
# undef BOOST_FT_cc
# undef BOOST_FT_cc_id
# ifndef BOOST_FT_config_valid
# define BOOST_FT_cc_id 1
# define BOOST_FT_cc_name implicit_cc
# define BOOST_FT_cc BOOST_PP_EMPTY
# define BOOST_FT_cond 0x00000001
# include BOOST_FT_cc_file
# undef BOOST_FT_cond
# undef BOOST_FT_cc_name
# undef BOOST_FT_cc
# undef BOOST_FT_cc_id
# else
# undef BOOST_FT_config_valid
# endif
