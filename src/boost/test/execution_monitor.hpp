//  (C) Copyright Gennadiy Rozental 2001.
//  (C) Copyright Beman Dawes 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief Defines public interface of the Execution Monitor and related classes
// ***************************************************************************

#ifndef BOOST_TEST_EXECUTION_MONITOR_HPP_071894GER
#define BOOST_TEST_EXECUTION_MONITOR_HPP_071894GER

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/detail/fwd_decl.hpp>
#include <boost/test/detail/throw_exception.hpp>

#include <boost/test/utils/class_properties.hpp>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/type.hpp>
#include <boost/cstdlib.hpp>
#include <boost/function/function0.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

#ifdef BOOST_SEH_BASED_SIGNAL_HANDLING

// for the FP constants and control routines
#include <float.h>

#ifndef EM_INVALID
#define EM_INVALID _EM_INVALID
#endif

#ifndef EM_DENORMAL
#define EM_DENORMAL _EM_DENORMAL
#endif

#ifndef EM_ZERODIVIDE
#define EM_ZERODIVIDE _EM_ZERODIVIDE
#endif

#ifndef EM_OVERFLOW
#define EM_OVERFLOW _EM_OVERFLOW
#endif

#ifndef EM_UNDERFLOW
#define EM_UNDERFLOW _EM_UNDERFLOW
#endif

#ifndef MCW_EM
#define MCW_EM _MCW_EM
#endif

#else // based on ISO C standard

#if !defined(BOOST_NO_FENV_H)
  #include <boost/detail/fenv.hpp>
#endif

#endif

#if defined(BOOST_SEH_BASED_SIGNAL_HANDLING) && !defined(UNDER_CE)
  //! Indicates tha the floating point exception handling is supported
  //! through SEH
  #define BOOST_TEST_FPE_SUPPORT_WITH_SEH__
#elif !defined(BOOST_SEH_BASED_SIGNAL_HANDLING) && !defined(UNDER_CE)
  #if !defined(BOOST_NO_FENV_H) && !defined(BOOST_CLANG) && \
      defined(__GLIBC__) && defined(__USE_GNU) && \
      !(defined(__UCLIBC__) || defined(__nios2__) || defined(__microblaze__))
  //! Indicates that floating point exception handling is supported for the
  //! non SEH version of it, for the GLIBC extensions only
  // see discussions on the related topic: https://svn.boost.org/trac/boost/ticket/11756
  #define BOOST_TEST_FPE_SUPPORT_WITH_GLIBC_EXTENSIONS__
  #endif
#endif


// Additional macro documentations not being generated without this hack
#ifdef BOOST_TEST_DOXYGEN_DOC__

//! Disables the support of the alternative stack
//! during the compilation of the Boost.test framework. This is especially useful
//! in case it is not possible to detect the lack of alternative stack support for
//! your compiler (for instance, ESXi).
#define BOOST_TEST_DISABLE_ALT_STACK

#endif

//____________________________________________________________________________//

namespace boost {

/// @defgroup ExecutionMonitor Function Execution Monitor
/// @{
/// @section Intro Introduction
/// Sometimes we need to call a function and make sure that no user or system originated exceptions are being thrown by it. Uniform exception reporting
/// is also may be convenient. That's the purpose of the Boost.Test's Execution Monitor.
///
/// The Execution Monitor is a lower-level component of the Boost Test Library. It is the base for implementing all other Boost.Test components, but also
/// can be used standalone to get controlled execution of error-prone functions with a uniform error notification. The Execution Monitor calls a user-supplied
/// function in a controlled environment, relieving users from messy error detection.
///
/// The Execution Monitor usage is demonstrated in the example exec_mon_example.
///
/// @section DesignRationale Design Rationale
///
/// The Execution Monitor design assumes that it can be used when no (or almost no) memory available. Also the Execution Monitor
/// is intended to be portable to as many platforms as possible.
///
/// @section UserGuide User's guide
/// The Execution Monitor is designed to solve the problem of executing potentially dangerous function that may result in any number of error conditions,
/// in monitored environment that should prevent any undesirable exceptions to propagate out of function call and produce consistent result report for all outcomes.
/// The Execution Monitor is able to produce informative report for all standard C++ exceptions and intrinsic types. All other exceptions are reported as unknown.
/// If you prefer different message for your exception type or need to perform any action, the Execution Monitor supports custom exception translators.
/// There are several other parameters of the monitored environment can be configured by setting appropriate properties of the Execution Monitor.
///
/// All symbols in the Execution Monitor implementation are located in the namespace boost. To use the Execution Monitor you need to:
/// -# include @c boost/test/execution_monitor.hpp
/// -# Make an instance of execution_monitor.
/// -# Optionally register custom exception translators for exception classes which require special processing.
///
/// @subsection FuncExec Monitored function execution
///
/// The class execution_monitor can monitor functions with the following signatures:
/// - int ()
/// - void ()
///
/// This function is expected to be self sufficient part of your application. You can't pass any arguments to this function directly. Instead you
/// should bind them into executable nullary function using bind function (either standard or boost variant). Neither you can return any other value,
/// but an integer result code. If necessary you can bind output parameters by reference or use some other more complicated nullary functor, which
/// maintains state. This includes class methods, static class methods etc.
///
/// To start the monitored function, invoke the method execution_monitor::execute and pass the monitored function as an argument. If the call succeeds,
/// the method returns the result code produced by the monitored function. If any of the following conditions occur:
/// - Uncaught C++ exception
/// - Hardware or software signal, trap, or other exception
/// - Timeout reached
/// - Debug assert event occurred (under Microsoft Visual C++ or compatible compiler)
///
/// then the method throws the execution_exception. The exception contains unique error_code value identifying the error condition and the detailed message
/// that can be used to report the error.
///
/// @subsection Reporting Errors reporting and translation
///
/// If you need to report an error inside monitored function execution you have to throw an exception. Do not use the execution_exception - it's not intended
/// to be used for this purpose. The simplest choice is to use one of the following C++ types as an exception:
/// - C string
/// - std:string
/// - any exception class in std::exception hierarchy
/// - boost::exception
///
/// execution_monitor will catch and report these types of exceptions. If exception is thrown which is unknown to execution_monitor, it can only
/// report the fact of the exception. So in case if you prefer to use your own exception types or can't govern what exceptions are generated by monitored
/// function and would like to see proper error message in a report, execution_monitor can be configured with custom "translator" routine, which will have
/// a chance to either record the fact of the exception itself or translate it into one of standard exceptions and rethrow (or both). The translator routine
/// is registered per exception type and is invoked when exception of this class (or one inherited from it) is thrown inside monitored routine. You can
/// register as many independent translators as you like. See execution_monitor::register_exception_translator specification for requirements on translator
/// function.
///
/// Finally, if you need to abort the monitored function execution without reporting any errors, you can throw an exception execution_aborted. As a result
/// the execution is aborted and zero result code is produced by the method execution_monitor::execute.
///
/// @subsection Parameters Supported parameters
///
/// The Execution Monitor behavior is configurable through the set of parameters (properties) associated with the instance of the monitor. See execution_monitor
/// specification for a list of supported parameters and their semantic.

// ************************************************************************** //
// **************        detail::translator_holder_base        ************** //
// ************************************************************************** //

namespace detail {

class translator_holder_base;
typedef boost::shared_ptr<translator_holder_base> translator_holder_base_ptr;

class BOOST_TEST_DECL translator_holder_base {
protected:
    typedef boost::unit_test::const_string const_string;
public:
    // Constructor
    translator_holder_base( translator_holder_base_ptr next, const_string tag )
    : m_next( next )
    , m_tag( std::string() + tag )
    {
    }

    // Destructor
    virtual     ~translator_holder_base() {}

    // translator holder interface
    // invokes the function F inside the try/catch guarding against specific exception
    virtual int operator()( boost::function<int ()> const& F ) = 0;

    // erases specific translator holder from the chain
    translator_holder_base_ptr erase( translator_holder_base_ptr this_, const_string tag )
    {
        if( m_next )
            m_next = m_next->erase( m_next, tag );

        return m_tag == tag ? m_next : this_;
    }
#ifndef BOOST_NO_RTTI
    virtual translator_holder_base_ptr erase( translator_holder_base_ptr this_, std::type_info const& ) = 0;
    template<typename ExceptionType>
    translator_holder_base_ptr erase( translator_holder_base_ptr this_, boost::type<ExceptionType>* = 0 )
    {
        if( m_next )
            m_next = m_next->erase<ExceptionType>( m_next );

        return erase( this_, typeid(ExceptionType) );
    }
#endif

protected:
    // Data members
    translator_holder_base_ptr  m_next;
    std::string                 m_tag;
};

} // namespace detail

// ************************************************************************** //
/// @class execution_exception
/// @brief This class is used to report any kind of an failure during execution of a monitored function inside of execution_monitor
///
/// The instance of this class is thrown out of execution_monitor::execute invocation when failure is detected. Regardless of a kind of failure occurred
/// the instance will provide a uniform way to catch and report it.
///
/// One important design rationale for this class is that we should be ready to work after fatal memory corruptions or out of memory conditions. To facilitate
/// this class never allocates any memory and assumes that strings it refers to are either some constants or live in a some kind of persistent (preallocated) memory.
// ************************************************************************** //

class BOOST_SYMBOL_VISIBLE execution_exception {
    typedef boost::unit_test::const_string const_string;
public:
    /// These values are sometimes used as program return codes.
    /// The particular values have been chosen to avoid conflicts with
    /// commonly used program return codes: values < 100 are often user
    /// assigned, values > 255 are sometimes used to report system errors.
    /// Gaps in values allow for orderly expansion.
    ///
    /// @note(1) Only uncaught C++ exceptions are treated as errors.
    /// If a function catches a C++ exception, it never reaches
    /// the execution_monitor.
    ///
    /// The implementation decides what is a system_fatal_error and what is
    /// just a system_exception. Fatal errors are so likely to have corrupted
    /// machine state (like a stack overflow or addressing exception) that it
    /// is unreasonable to continue execution.
    ///
    /// @note(2) These errors include Unix signals and Windows structured
    /// exceptions. They are often initiated by hardware traps.
    enum error_code {
        no_error               = 0,   ///< for completeness only; never returned
        user_error             = 200, ///< user reported non-fatal error
        cpp_exception_error    = 205, ///< see note (1) above
        system_error           = 210, ///< see note (2) above
        timeout_error          = 215, ///< only detectable on certain platforms
        user_fatal_error       = 220, ///< user reported fatal error
        system_fatal_error     = 225  ///< see note (2) above
    };

    /// Simple model for the location of failure in a source code
    struct BOOST_TEST_DECL location {
        explicit    location( char const* file_name = 0, size_t line_num = 0, char const* func = 0 );
        explicit    location( const_string file_name, size_t line_num = 0, char const* func = 0 );

        const_string    m_file_name;    ///< File name
        size_t          m_line_num;     ///< Line number
        const_string    m_function;     ///< Function name
    };

    /// @name Constructors

    /// Constructs instance based on message, location and error code

    /// @param[in] ec           error code
    /// @param[in] what_msg     error message
    /// @param[in] location     error location
    execution_exception( error_code ec, const_string what_msg, location const& location );

    /// @name Access methods

    /// Exception error code
    error_code      code() const    { return m_error_code; }
    /// Exception message
    const_string    what() const    { return m_what; }
    /// Exception location
    location const& where() const   { return m_location; }
    ///@}

private:
    // Data members
    error_code      m_error_code;
    const_string    m_what;
    location        m_location;
}; // execution_exception

// ************************************************************************** //
/// @brief Function execution monitor

/// This class is used to uniformly detect and report an occurrence of several types of signals and exceptions, reducing various
/// errors to a uniform execution_exception that is returned to a caller.
///
/// The execution_monitor behavior can be customized through a set of public parameters (properties) associated with the execution_monitor instance.
/// All parameters are implemented as public unit_test::readwrite_property data members of the class execution_monitor.
// ************************************************************************** //

class BOOST_TEST_DECL execution_monitor {
    typedef boost::unit_test::const_string const_string;
public:

    /// Default constructor initializes all execution monitor properties
    execution_monitor();

    /// Should monitor catch system errors.
    ///
    /// The @em p_catch_system_errors property is a boolean flag (default value is true) specifying whether or not execution_monitor should trap system
    /// errors/system level exceptions/signals, which would cause program to crash in a regular case (without execution_monitor).
    /// Set this property to false, for example, if you wish to force coredump file creation. The Unit Test Framework provides a
    /// runtime parameter @c \-\-catch_system_errors=yes to alter the behavior in monitored test cases.
    unit_test::readwrite_property<bool> p_catch_system_errors;

    ///  Should monitor try to attach debugger in case of caught system error.
    ///
    /// The @em p_auto_start_dbg property is a boolean flag (default value is false) specifying whether or not execution_monitor should try to attach debugger
    /// in case system error is caught.
    unit_test::readwrite_property<bool> p_auto_start_dbg;


    ///  Specifies the seconds that elapse before a timer_error occurs.
    ///
    /// The @em p_timeout property is an integer timeout (in microseconds) for monitored function execution. Use this parameter to monitor code with possible deadlocks
    /// or infinite loops. This feature is only available for some operating systems (not yet Microsoft Windows).
    unit_test::readwrite_property<unsigned long int>  p_timeout;

    ///  Should monitor use alternative stack for the signal catching.
    ///
    /// The @em p_use_alt_stack property is a boolean flag (default value is false) specifying whether or not execution_monitor should use an alternative stack
    /// for the sigaction based signal catching. When enabled the signals are delivered to the execution_monitor on a stack different from current execution
    /// stack, which is safer in case if it is corrupted by monitored function. For more details on alternative stack handling see appropriate manuals.
    unit_test::readwrite_property<bool> p_use_alt_stack;

    /// Should monitor try to detect hardware floating point exceptions (!= 0), and which specific exception to catch.
    ///
    /// The @em p_detect_fp_exceptions property is a boolean flag (default value is false) specifying whether or not execution_monitor should install hardware
    /// traps for the floating point exception on platforms where it's supported.
    unit_test::readwrite_property<unsigned> p_detect_fp_exceptions;


    // @name Monitoring entry points

    /// @brief Execution monitor entry point for functions returning integer value
    ///
    /// This method executes supplied function F inside a try/catch block and also may include other unspecified platform dependent error detection code.
    ///
    /// This method throws an execution_exception on an uncaught C++ exception, a hardware or software signal, trap, or other user exception.
    ///
    /// @note execute() doesn't consider it an error for F to return a non-zero value.
    /// @param[in] F  Function to monitor
    /// @returns  value returned by function call F().
    /// @see vexecute
    int         execute( boost::function<int ()> const& F );

    /// @brief Execution monitor entry point for functions returning void
    ///
    /// This method is semantically identical to execution_monitor::execute, but doesn't produce any result code.
    /// @param[in] F  Function to monitor
    /// @see execute
    void         vexecute( boost::function<void ()> const& F );
    // @}

    // @name Exception translator registration

    /// @brief Registers custom (user supplied) exception translator

    /// This method template registers a translator for an exception type specified as a first template argument. For example
    /// @code
    ///    void myExceptTr( MyException const& ex ) { /*do something with the exception here*/}
    ///    em.register_exception_translator<MyException>( myExceptTr );
    /// @endcode
    /// The translator should be any unary function/functor object which accepts MyException const&. This can be free standing function
    /// or bound class method. The second argument is an optional string tag you can associate with this translator routine. The only reason
    /// to specify the tag is if you plan to erase the translator eventually. This can be useful in scenario when you reuse the same
    /// execution_monitor instance to monitor different routines and need to register a translator specific to the routine being monitored.
    /// While it is possible to erase the translator based on an exception type it was registered for, tag string provides simpler way of doing this.
    /// @tparam ExceptionType type of the exception we register a translator for
    /// @tparam ExceptionTranslator type of the translator we register for this exception
    /// @param[in] tr         translator function object with the signature <em> void (ExceptionType const&)</em>
    /// @param[in] tag        tag associated with this translator
    template<typename ExceptionType, typename ExceptionTranslator>
    void        register_exception_translator( ExceptionTranslator const& tr, const_string tag = const_string(), boost::type<ExceptionType>* = 0 );

    /// @brief Erases custom exception translator based on a tag

    /// Use the same tag as the one used during translator registration
    /// @param[in] tag  tag associated with translator you wants to erase
    void        erase_exception_translator( const_string tag )
    {
        m_custom_translators = m_custom_translators->erase( m_custom_translators, tag );
    }
#ifndef BOOST_NO_RTTI
    /// @brief Erases custom exception translator based on an exception type
    ///
    /// tparam ExceptionType Exception type for which you want to erase the translator
    template<typename ExceptionType>
    void        erase_exception_translator( boost::type<ExceptionType>* = 0 )
    {
        m_custom_translators = m_custom_translators->erase<ExceptionType>( m_custom_translators );
    }
    //@}
#endif

private:
    // implementation helpers
    int         catch_signals( boost::function<int ()> const& F );

    // Data members
    detail::translator_holder_base_ptr  m_custom_translators;
    boost::scoped_array<char>           m_alt_stack;
}; // execution_monitor

// ************************************************************************** //
// **************          detail::translator_holder           ************** //
// ************************************************************************** //

namespace detail {

template<typename ExceptionType, typename ExceptionTranslator>
class translator_holder : public translator_holder_base
{
public:
    explicit    translator_holder( ExceptionTranslator const& tr, translator_holder_base_ptr& next, const_string tag = const_string() )
    : translator_holder_base( next, tag ), m_translator( tr ) {}

    // translator holder interface
    int operator()( boost::function<int ()> const& F ) BOOST_OVERRIDE
    {
        BOOST_TEST_I_TRY {
            return m_next ? (*m_next)( F ) : F();
        }
        BOOST_TEST_I_CATCH( ExceptionType, e ) {
            m_translator( e );
            return boost::exit_exception_failure;
        }
    }
#ifndef BOOST_NO_RTTI
    translator_holder_base_ptr erase( translator_holder_base_ptr this_, std::type_info const& ti ) BOOST_OVERRIDE
    {
        return ti == typeid(ExceptionType) ? m_next : this_;
    }
#endif

private:
    // Data members
    ExceptionTranslator m_translator;
};

} // namespace detail

template<typename ExceptionType, typename ExceptionTranslator>
void
execution_monitor::register_exception_translator( ExceptionTranslator const& tr, const_string tag, boost::type<ExceptionType>* )
{
    m_custom_translators.reset(
        new detail::translator_holder<ExceptionType,ExceptionTranslator>( tr, m_custom_translators, tag ) );
}

// ************************************************************************** //
/// @class execution_aborted
/// @brief This is a trivial default constructible class. Use it to report graceful abortion of a monitored function execution.
// ************************************************************************** //

struct execution_aborted {};

// ************************************************************************** //
// **************                  system_error                ************** //
// ************************************************************************** //

class system_error {
public:
    // Constructor
    explicit    system_error( char const* exp );

    long const          p_errno;
    char const* const   p_failed_exp;
};

//!@internal
#define BOOST_TEST_SYS_ASSERT( cond ) BOOST_TEST_I_ASSRT( cond, ::boost::system_error( BOOST_STRINGIZE( exp ) ) )

// ************************************************************************** //
// **************Floating point exception management interface ************** //
// ************************************************************************** //

namespace fpe {

enum masks {
    BOOST_FPE_OFF       = 0,

#if defined(BOOST_TEST_FPE_SUPPORT_WITH_SEH__) /* *** */
    BOOST_FPE_DIVBYZERO = EM_ZERODIVIDE,
    BOOST_FPE_INEXACT   = EM_INEXACT,
    BOOST_FPE_INVALID   = EM_INVALID,
    BOOST_FPE_OVERFLOW  = EM_OVERFLOW,
    BOOST_FPE_UNDERFLOW = EM_UNDERFLOW|EM_DENORMAL,

    BOOST_FPE_ALL       = MCW_EM,

#elif !defined(BOOST_TEST_FPE_SUPPORT_WITH_GLIBC_EXTENSIONS__)/* *** */
    BOOST_FPE_DIVBYZERO = BOOST_FPE_OFF,
    BOOST_FPE_INEXACT   = BOOST_FPE_OFF,
    BOOST_FPE_INVALID   = BOOST_FPE_OFF,
    BOOST_FPE_OVERFLOW  = BOOST_FPE_OFF,
    BOOST_FPE_UNDERFLOW = BOOST_FPE_OFF,
    BOOST_FPE_ALL       = BOOST_FPE_OFF,
#else /* *** */

#if defined(FE_DIVBYZERO)
    BOOST_FPE_DIVBYZERO = FE_DIVBYZERO,
#else
    BOOST_FPE_DIVBYZERO = BOOST_FPE_OFF,
#endif

#if defined(FE_INEXACT)
    BOOST_FPE_INEXACT   = FE_INEXACT,
#else
    BOOST_FPE_INEXACT   = BOOST_FPE_OFF,
#endif

#if defined(FE_INVALID)
    BOOST_FPE_INVALID   = FE_INVALID,
#else
    BOOST_FPE_INVALID   = BOOST_FPE_OFF,
#endif

#if defined(FE_OVERFLOW)
    BOOST_FPE_OVERFLOW  = FE_OVERFLOW,
#else
    BOOST_FPE_OVERFLOW  = BOOST_FPE_OFF,
#endif

#if defined(FE_UNDERFLOW)
    BOOST_FPE_UNDERFLOW = FE_UNDERFLOW,
#else
    BOOST_FPE_UNDERFLOW = BOOST_FPE_OFF,
#endif

#if defined(FE_ALL_EXCEPT)
    BOOST_FPE_ALL       = FE_ALL_EXCEPT,
#else
    BOOST_FPE_ALL       = BOOST_FPE_OFF,
#endif

#endif /* *** */
    BOOST_FPE_INV       = BOOST_FPE_ALL+1
};

//____________________________________________________________________________//

// return the previous set of enabled exceptions when successful, and BOOST_FPE_INV otherwise
unsigned BOOST_TEST_DECL enable( unsigned mask );
unsigned BOOST_TEST_DECL disable( unsigned mask );

//____________________________________________________________________________//

} // namespace fpe

///@}

}  // namespace boost


#include <boost/test/detail/enable_warnings.hpp>

#endif
