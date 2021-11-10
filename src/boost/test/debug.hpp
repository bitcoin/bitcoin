//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//! @file
//! @brief defines portable debug interfaces
//!
//! Intended to standardize interface of programs with debuggers
// ***************************************************************************

#ifndef BOOST_TEST_DEBUG_API_HPP_112006GER
#define BOOST_TEST_DEBUG_API_HPP_112006GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>

// Boost
#include <boost/function/function1.hpp>

// STL
#include <string>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
/// Contains debugger and debug C Runtime interfaces
namespace debug {

/// @defgroup DebuggerInterface Debugger and debug C Runtime portable interfaces
/// @{
/// These interfaces are intended to be used by application to:
/// - check if we are running under debugger
/// - attach the debugger to itself
///
/// Unfortunately these actions differ widely between different debuggers available in a field. These interface present generalized standard form of
/// performing these actions. Implementation depends a lot on the environment application is running in and thus there are several custom implementations
/// supported by the Boost.Test
///
/// In addition here you find interfaces for memory leaks detection and reporting.
///
/// All these interfaces are defined in namespace boost::debug

// ************************************************************************** //
/// Checks if programs runs under debugger

/// @returns true if current process is under debugger. False otherwise
// ************************************************************************** //
bool BOOST_TEST_DECL under_debugger();

// ************************************************************************** //
/// Cause program to break execution in debugger at call point
// ************************************************************************** //

void BOOST_TEST_DECL debugger_break();

// ************************************************************************** //
/// Collection of data, which is used by debugger starter routine
// ************************************************************************** //

struct dbg_startup_info {
    long                    pid;                ///< pid of a program to attach to
    bool                    break_or_continue;  ///< what to do after debugger is attached
    unit_test::const_string binary_path;        ///< path to executable for current process
    unit_test::const_string display;            ///< if debugger has a GUI, which display to use (on UNIX)
    unit_test::const_string init_done_lock;     ///< path to a uniquely named lock file, which is used to pause current application while debugger is being initialized
};

/// Signature of debugger starter routine. Takes an instance of dbg_startup_into as only argument
typedef boost::function<void (dbg_startup_info const&)> dbg_starter;

// ************************************************************************** //
/// Specifies which debugger to use when attaching and optionally what routine to use to start that debugger

/// There are  many different debuggers available for different platforms. Some of them also can be used in a different setups/configuratins.
/// For example, gdb can be used in plain text mode, inside ddd, inside (x)emacs or in a separate xterm window.
/// Boost.Test identifies each configuration with unique string.
/// Also different debuggers configurations require different routines which is specifically tailored to start that debugger configuration.
/// Boost.Test comes with set of predefined configuration names and corresponding routines for these configurations:
///   - TODO
///
/// You can use this routine to select which one of the predefined debugger configurations to use in which case you do not need to provide starter
/// routine (the one provided by Boost.Test will be used). You can also use this routine to select your own debugger by providing unique configuration
/// id and starter routine for this configuration.
///
/// @param[in] dbg_id   Unique id for debugger configuration (for example, gdb)
/// @param[in] s        Optional starter routine for selected configuration (use only you want to define your own configuration)
/// @returns            Id of previously selected debugger configuration
std::string BOOST_TEST_DECL set_debugger( unit_test::const_string dbg_id, dbg_starter s = dbg_starter() );

// ************************************************************************** //
/// Attaches debugger to the current process

/// Using  currently selected debugger, this routine attempts to attach the debugger to this process.
/// @param[in] break_or_continue tells what we wan to do after the debugger is attached. If true - process execution breaks
///                              in the point in invocation of this function. Otherwise execution continues, but now it is
///                              under the debugger
/// @returns true if debugger successfully attached. False otherwise
// ************************************************************************** //

bool BOOST_TEST_DECL attach_debugger( bool break_or_continue = true );

// ************************************************************************** //
/// Switches on/off memory leaks detection

/// On platforms where memory leak detection is possible inside of running application (at the moment this is only Windows family) you can
/// switch this feature on and off using this interface. In addition you can specify the name of the file to write a report into. Otherwise
/// the report is going to be generated in standard error stream.
/// @param[in] on_off boolean switch
/// @param[in] report_file file, where the report should be directed to
// ************************************************************************** //

void BOOST_TEST_DECL detect_memory_leaks( bool on_off, unit_test::const_string report_file = unit_test::const_string() );

// ************************************************************************** //
/// Causes program to break execution in debugger at specific allocation point

/// On some platforms/memory managers (at the moment only on Windows/Visual Studio) one can tell a C Runtime to break
/// on specific memory allocation. This can be used in combination with memory leak detection (which reports leaked memory
/// allocation number) to locate the place where leak initiated.
/// @param[in] mem_alloc_order_num Specific memory allocation number
// ************************************************************************** //

void BOOST_TEST_DECL break_memory_alloc( long mem_alloc_order_num );

} // namespace debug
/// @}

} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif
