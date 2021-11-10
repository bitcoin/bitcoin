//  (C) Copyright Gennadiy Rozental 2001.
//  Use, modification, and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : debug interfaces implementation
// ***************************************************************************

#ifndef BOOST_TEST_DEBUG_API_IPP_112006GER
#define BOOST_TEST_DEBUG_API_IPP_112006GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/detail/global_typedef.hpp>

#include <boost/test/debug.hpp>
#include <boost/test/debug_config.hpp>

#include <boost/core/ignore_unused.hpp>

// Implementation on Windows
#if defined(_WIN32) && !defined(UNDER_CE) && !defined(BOOST_DISABLE_WIN32) // ******* WIN32

#  define BOOST_WIN32_BASED_DEBUG

// SYSTEM API
#  include <windows.h>
#  include <winreg.h>
#  include <cstdio>
#  include <cstring>

#  if !defined(NDEBUG) && defined(_MSC_VER)
#    define BOOST_MS_CRT_BASED_DEBUG
#    include <crtdbg.h>
#  endif


#  ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::memset; using ::sprintf; }
#  endif

#elif defined(unix) || defined(__unix) // ********************* UNIX

#  define BOOST_UNIX_BASED_DEBUG

// Boost.Test
#include <boost/test/utils/class_properties.hpp>
#include <boost/test/utils/algorithm.hpp>

// STL
#include <cstring>  // std::memcpy
#include <map>
#include <cstdio>
#include <stdarg.h> // !! ?? cstdarg

// SYSTEM API
#  include <unistd.h>
#  include <signal.h>
#  include <fcntl.h>

#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/wait.h>
#  include <sys/time.h>
#  include <stdio.h>
#  include <stdlib.h>

#  if defined(sun) || defined(__sun)

#    define BOOST_SUN_BASED_DEBUG

#    ifndef BOOST_TEST_DBG_LIST
#      define BOOST_TEST_DBG_LIST dbx;gdb
#    endif

#    define BOOST_TEST_CNL_DBG  dbx
#    define BOOST_TEST_GUI_DBG  dbx-ddd

#    include <procfs.h>

#  elif defined(linux) || defined(__linux__)

#    define BOOST_LINUX_BASED_DEBUG

#    include <sys/ptrace.h>

#    ifndef BOOST_TEST_STAT_LINE_MAX
#      define BOOST_TEST_STAT_LINE_MAX 500
#    endif

#    ifndef BOOST_TEST_DBG_LIST
#      define BOOST_TEST_DBG_LIST gdb;lldb
#    endif

#    define BOOST_TEST_CNL_DBG  gdb
#    define BOOST_TEST_GUI_DBG  gdb-xterm

#  endif

#elif defined(__APPLE__) // ********************* APPLE

#  define BOOST_APPLE_BASED_DEBUG

#  include <assert.h>
#  include <sys/types.h>
#  include <unistd.h>
#  include <sys/sysctl.h>

#endif

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace debug {

using unit_test::const_string;

// ************************************************************************** //
// **************                debug::info_t                 ************** //
// ************************************************************************** //

namespace {

#if defined(BOOST_WIN32_BASED_DEBUG) // *********************** WIN32

template<typename T>
inline void
dyn_symbol( T& res, char const* module_name, char const* symbol_name )
{
    HMODULE m = ::GetModuleHandleA( module_name );

    if( !m )
        m = ::LoadLibraryA( module_name );

    res = reinterpret_cast<T>( ::GetProcAddress( m, symbol_name ) );
}

//____________________________________________________________________________//

static struct info_t {
    typedef BOOL (WINAPI* IsDebuggerPresentT)();
    typedef LONG (WINAPI* RegQueryValueExT)( HKEY, char const* /*LPTSTR*/, LPDWORD, LPDWORD, LPBYTE, LPDWORD );
    typedef LONG (WINAPI* RegOpenKeyT)( HKEY, char const* /*LPCTSTR*/, PHKEY );
    typedef LONG (WINAPI* RegCloseKeyT)( HKEY );

    info_t();

    IsDebuggerPresentT  m_is_debugger_present;
    RegOpenKeyT         m_reg_open_key;
    RegQueryValueExT    m_reg_query_value;
    RegCloseKeyT        m_reg_close_key;

} s_info;

//____________________________________________________________________________//

info_t::info_t()
{
    dyn_symbol( m_is_debugger_present, "kernel32", "IsDebuggerPresent" );
    dyn_symbol( m_reg_open_key, "advapi32", "RegOpenKeyA" );
    dyn_symbol( m_reg_query_value, "advapi32", "RegQueryValueExA" );
    dyn_symbol( m_reg_close_key, "advapi32", "RegCloseKey" );
}

//____________________________________________________________________________//

#elif defined(BOOST_UNIX_BASED_DEBUG)

// ************************************************************************** //
// **************                   fd_holder                  ************** //
// ************************************************************************** //

struct fd_holder {
    explicit fd_holder( int fd ) : m_fd( fd ) {}
    ~fd_holder()
    {
        if( m_fd != -1 )
            ::close( m_fd );
    }

    operator int() { return m_fd; }

private:
    // Data members
    int m_fd;
};


// ************************************************************************** //
// **************                 process_info                 ************** //
// ************************************************************************** //

struct process_info {
    // Constructor
    explicit        process_info( int pid );

    // access methods
    int             parent_pid() const  { return m_parent_pid; }
    const_string    binary_name() const { return m_binary_name; }
    const_string    binary_path() const { return m_binary_path; }

private:
    // Data members
    int             m_parent_pid;
    const_string    m_binary_name;
    const_string    m_binary_path;

#if defined(BOOST_SUN_BASED_DEBUG)
    struct psinfo   m_psi;
    char            m_binary_path_buff[500+1]; // !! ??
#elif defined(BOOST_LINUX_BASED_DEBUG)
    char            m_stat_line[BOOST_TEST_STAT_LINE_MAX+1];
    char            m_binary_path_buff[500+1]; // !! ??
#endif
};

//____________________________________________________________________________//

process_info::process_info( int pid )
: m_parent_pid( 0 )
{
#if defined(BOOST_SUN_BASED_DEBUG)
    char fname_buff[30];

    ::snprintf( fname_buff, sizeof(fname_buff), "/proc/%d/psinfo", pid );

    fd_holder psinfo_fd( ::open( fname_buff, O_RDONLY ) );

    if( psinfo_fd == -1 )
        return;

    if( ::read( psinfo_fd, &m_psi, sizeof(m_psi) ) == -1 )
        return;

    m_parent_pid = m_psi.pr_ppid;

    m_binary_name.assign( m_psi.pr_fname );

    //-------------------------- //

    ::snprintf( fname_buff, sizeof(fname_buff), "/proc/%d/as", pid );

    fd_holder as_fd( ::open( fname_buff, O_RDONLY ) );
    uintptr_t   binary_name_pos;

    // !! ?? could we avoid reading whole m_binary_path_buff?
    if( as_fd == -1 ||
        ::lseek( as_fd, m_psi.pr_argv, SEEK_SET ) == -1 ||
        ::read ( as_fd, &binary_name_pos, sizeof(binary_name_pos) ) == -1 ||
        ::lseek( as_fd, binary_name_pos, SEEK_SET ) == -1 ||
        ::read ( as_fd, m_binary_path_buff, sizeof(m_binary_path_buff) ) == -1 )
        return;

    m_binary_path.assign( m_binary_path_buff );

#elif defined(BOOST_LINUX_BASED_DEBUG)
    char fname_buff[30];

    ::snprintf( fname_buff, sizeof(fname_buff), "/proc/%d/stat", pid );

    fd_holder psinfo_fd( ::open( fname_buff, O_RDONLY ) );

    if( psinfo_fd == -1 )
        return;

    ssize_t num_read = ::read( psinfo_fd, m_stat_line, sizeof(m_stat_line)-1 );
    if( num_read == -1 )
        return;

    m_stat_line[num_read] = 0;

    char const* name_beg = m_stat_line;
    while( *name_beg && *name_beg != '(' )
        ++name_beg;

    char const* name_end = name_beg+1;
    while( *name_end && *name_end != ')' )
        ++name_end;

    std::sscanf( name_end+1, "%*s%d", &m_parent_pid );

    m_binary_name.assign( name_beg+1, name_end );

    ::snprintf( fname_buff, sizeof(fname_buff), "/proc/%d/exe", pid );
    num_read = ::readlink( fname_buff, m_binary_path_buff, sizeof(m_binary_path_buff)-1 );

    if( num_read == -1 )
        return;

    m_binary_path_buff[num_read] = 0;
    m_binary_path.assign( m_binary_path_buff, num_read );
#endif
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************             prepare_window_title             ************** //
// ************************************************************************** //

static char*
prepare_window_title( dbg_startup_info const& dsi )
{
    typedef unit_test::const_string str_t;

    static char title_str[50];

    str_t path_sep( "\\/" );

    str_t::iterator  it = unit_test::utils::find_last_of( dsi.binary_path.begin(), dsi.binary_path.end(),
                                                          path_sep.begin(), path_sep.end() );

    if( it == dsi.binary_path.end() )
        it = dsi.binary_path.begin();
    else
        ++it;

    ::snprintf( title_str, sizeof(title_str), "%*s %ld", (int)(dsi.binary_path.end()-it), it, dsi.pid );

    return title_str;
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                  save_execlp                 ************** //
// ************************************************************************** //

typedef unit_test::basic_cstring<char> mbuffer;

inline char*
copy_arg( mbuffer& dest, const_string arg )
{
    if( dest.size() < arg.size()+1 )
        return 0;

    char* res = dest.begin();

    std::memcpy( res, arg.begin(), arg.size()+1 );

    dest.trim_left( arg.size()+1 );

    return res;
}

//____________________________________________________________________________//

bool
safe_execlp( char const* file, ... )
{
    static char* argv_buff[200];

    va_list     args;
    char const* arg;

    // first calculate actual number of arguments
    int         num_args = 2; // file name and 0 at least

    va_start( args, file );
    while( !!(arg = va_arg( args, char const* )) )
        num_args++;
    va_end( args );

    // reserve space for the argument pointers array
    char**      argv_it  = argv_buff;
    mbuffer     work_buff( reinterpret_cast<char*>(argv_buff), sizeof(argv_buff) );
    work_buff.trim_left( num_args * sizeof(char*) );

    // copy all the argument values into local storage
    if( !(*argv_it++ = copy_arg( work_buff, file )) )
        return false;

    printf( "!! %s\n", file );

    va_start( args, file );
    while( !!(arg = va_arg( args, char const* )) ) {
        printf( "!! %s\n", arg );
        if( !(*argv_it++ = copy_arg( work_buff, arg )) ) {
            va_end( args );
            return false;
        }
    }
    va_end( args );

    *argv_it = 0;

    return ::execvp( file, argv_buff ) != -1;
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************            start_debugger_in_emacs           ************** //
// ************************************************************************** //

static void
start_debugger_in_emacs( dbg_startup_info const& dsi, char const* emacs_name, char const* dbg_command )
{
    char const* title = prepare_window_title( dsi );

    if( !title )
        return;

    dsi.display.is_empty()
        ? safe_execlp( emacs_name, "-title", title, "--eval", dbg_command, 0 )
        : safe_execlp( emacs_name, "-title", title, "-display", dsi.display.begin(), "--eval", dbg_command, 0 );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                 gdb starters                 ************** //
// ************************************************************************** //

static char const*
prepare_gdb_cmnd_file( dbg_startup_info const& dsi )
{
    // prepare pid value
    char pid_buff[16];
    ::snprintf( pid_buff, sizeof(pid_buff), "%ld", dsi.pid );
    unit_test::const_string pid_str( pid_buff );

    static char cmd_file_name[] = "/tmp/btl_gdb_cmd_XXXXXX"; // !! ??

    // prepare commands
    const mode_t cur_umask = ::umask( S_IRWXO | S_IRWXG );
    fd_holder cmd_fd( ::mkstemp( cmd_file_name ) );
    ::umask( cur_umask );

    if( cmd_fd == -1 )
        return 0;

#define WRITE_STR( str )  if( ::write( cmd_fd, str.begin(), str.size() ) == -1 ) return 0;
#define WRITE_CSTR( str ) if( ::write( cmd_fd, str, sizeof( str )-1 ) == -1 ) return 0;

    WRITE_CSTR( "file " );
    WRITE_STR( dsi.binary_path );
    WRITE_CSTR( "\nattach " );
    WRITE_STR( pid_str );
    WRITE_CSTR( "\nshell unlink " );
    WRITE_STR( dsi.init_done_lock );
    WRITE_CSTR( "\ncont" );
    if( dsi.break_or_continue )
        WRITE_CSTR( "\nup 4" );

    WRITE_CSTR( "\necho \\n" ); // !! ??
    WRITE_CSTR( "\nlist -" );
    WRITE_CSTR( "\nlist" );
    WRITE_CSTR( "\nshell unlink " );
    WRITE_CSTR( cmd_file_name );

    return cmd_file_name;
}

//____________________________________________________________________________//

static void
start_gdb_in_console( dbg_startup_info const& dsi )
{
    char const* cmnd_file_name = prepare_gdb_cmnd_file( dsi );

    if( !cmnd_file_name )
        return;

    safe_execlp( "gdb", "-q", "-x", cmnd_file_name, 0 );
}

//____________________________________________________________________________//

static void
start_gdb_in_xterm( dbg_startup_info const& dsi )
{
    char const* title           = prepare_window_title( dsi );
    char const* cmnd_file_name  = prepare_gdb_cmnd_file( dsi );

    if( !title || !cmnd_file_name )
        return;

    safe_execlp( "xterm", "-T", title, "-display", dsi.display.begin(),
                    "-bg", "black", "-fg", "white", "-geometry", "88x30+10+10", "-fn", "9x15", "-e",
                 "gdb", "-q", "-x", cmnd_file_name, 0 );
}

//____________________________________________________________________________//

static void
start_gdb_in_emacs( dbg_startup_info const& dsi )
{
    char const* cmnd_file_name  = prepare_gdb_cmnd_file( dsi );
    if( !cmnd_file_name )
        return;

    char dbg_cmd_buff[500]; // !! ??
    ::snprintf( dbg_cmd_buff, sizeof(dbg_cmd_buff), "(progn (gdb \"gdb -q -x %s\"))", cmnd_file_name );

    start_debugger_in_emacs( dsi, "emacs", dbg_cmd_buff );
}

//____________________________________________________________________________//

static void
start_gdb_in_xemacs( dbg_startup_info const& )
{
    // !! ??
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                 dbx starters                 ************** //
// ************************************************************************** //

static char const*
prepare_dbx_cmd_line( dbg_startup_info const& dsi, bool list_source = true )
{
    static char cmd_line_buff[500]; // !! ??

    ::snprintf( cmd_line_buff, sizeof(cmd_line_buff), "unlink %s;cont;%s%s",
                   dsi.init_done_lock.begin(),
                   dsi.break_or_continue ? "up 2;": "",
                   list_source ? "echo \" \";list -w3;" : "" );

    return cmd_line_buff;
}

//____________________________________________________________________________//

static void
start_dbx_in_console( dbg_startup_info const& dsi )
{
    char pid_buff[16];
    ::snprintf( pid_buff, sizeof(pid_buff), "%ld", dsi.pid );

    safe_execlp( "dbx", "-q", "-c", prepare_dbx_cmd_line( dsi ), dsi.binary_path.begin(), pid_buff, 0 );
}

//____________________________________________________________________________//

static void
start_dbx_in_xterm( dbg_startup_info const& dsi )
{
    char const* title = prepare_window_title( dsi );
    if( !title )
        return;

    char pid_buff[16]; // !! ??
    ::snprintf( pid_buff, sizeof(pid_buff), "%ld", dsi.pid );

    safe_execlp( "xterm", "-T", title, "-display", dsi.display.begin(),
                    "-bg", "black", "-fg", "white", "-geometry", "88x30+10+10", "-fn", "9x15", "-e",
                 "dbx", "-q", "-c", prepare_dbx_cmd_line( dsi ), dsi.binary_path.begin(), pid_buff, 0 );
}

//____________________________________________________________________________//

static void
start_dbx_in_emacs( dbg_startup_info const& /*dsi*/ )
{
//    char dbg_cmd_buff[500]; // !! ??
//
//    ::snprintf( dbg_cmd_buff, sizeof(dbg_cmd_buff), "(progn (dbx \"dbx -q -c cont %s %ld\"))", dsi.binary_path.begin(), dsi.pid );

//    start_debugger_in_emacs( dsi, "emacs", dbg_cmd_buff );
}

//____________________________________________________________________________//

static void
start_dbx_in_xemacs( dbg_startup_info const& )
{
    // !! ??
}

//____________________________________________________________________________//

static void
start_dbx_in_ddd( dbg_startup_info const& dsi )
{
    char const* title = prepare_window_title( dsi );
    if( !title )
        return;

    char pid_buff[16]; // !! ??
    ::snprintf( pid_buff, sizeof(pid_buff), "%ld", dsi.pid );

    safe_execlp( "ddd", "-display", dsi.display.begin(),
                 "--dbx", "-q", "-c", prepare_dbx_cmd_line( dsi, false ), dsi.binary_path.begin(), pid_buff, 0 );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                debug::info_t                 ************** //
// ************************************************************************** //

static struct info_t {
    // Constructor
    info_t();

    // Public properties
    unit_test::readwrite_property<std::string>  p_dbg;

    // Data members
    std::map<std::string,dbg_starter>           m_dbg_starter_reg;
} s_info;

//____________________________________________________________________________//

info_t::info_t()
{
    p_dbg.value = ::getenv( "DISPLAY" )
        ? std::string( BOOST_STRINGIZE( BOOST_TEST_GUI_DBG ) )
        : std::string( BOOST_STRINGIZE( BOOST_TEST_CNL_DBG ) );

    m_dbg_starter_reg[std::string("gdb")]           = &start_gdb_in_console;
    m_dbg_starter_reg[std::string("gdb-emacs")]     = &start_gdb_in_emacs;
    m_dbg_starter_reg[std::string("gdb-xterm")]     = &start_gdb_in_xterm;
    m_dbg_starter_reg[std::string("gdb-xemacs")]    = &start_gdb_in_xemacs;

    m_dbg_starter_reg[std::string("dbx")]           = &start_dbx_in_console;
    m_dbg_starter_reg[std::string("dbx-emacs")]     = &start_dbx_in_emacs;
    m_dbg_starter_reg[std::string("dbx-xterm")]     = &start_dbx_in_xterm;
    m_dbg_starter_reg[std::string("dbx-xemacs")]    = &start_dbx_in_xemacs;
    m_dbg_starter_reg[std::string("dbx-ddd")]       = &start_dbx_in_ddd;
}

//____________________________________________________________________________//

#endif

} // local namespace

// ************************************************************************** //
// **************  check if program is running under debugger  ************** //
// ************************************************************************** //

bool
under_debugger()
{
#if defined(BOOST_WIN32_BASED_DEBUG) // *********************** WIN32

    return !!s_info.m_is_debugger_present && s_info.m_is_debugger_present();

#elif defined(BOOST_UNIX_BASED_DEBUG) // ********************** UNIX

    // !! ?? could/should we cache the result somehow?
    const_string    dbg_list = BOOST_TEST_STRINGIZE( BOOST_TEST_DBG_LIST );

    pid_t pid = ::getpid();

    while( pid != 0 ) {
        process_info pi( pid );

        // !! ?? should we use tokenizer here instead?
        if( dbg_list.find( pi.binary_name() ) != const_string::npos )
            return true;

        pid = (pi.parent_pid() == pid ? 0 : pi.parent_pid());
    }

    return false;

#elif defined(BOOST_APPLE_BASED_DEBUG) // ********************** APPLE

    // See https://developer.apple.com/library/mac/qa/qa1361/_index.html
    int                 junk;
    int                 mib[4];
    struct kinfo_proc   info;
    size_t              size;

    // Initialize the flags so that, if sysctl fails for some bizarre
    // reason, we get a predictable result.
    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    // Call sysctl.
    size = sizeof(info);
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    assert(junk == 0);

    // We're being debugged if the P_TRACED flag is set.
    return ( (info.kp_proc.p_flag & P_TRACED) != 0 );

#else // ****************************************************** default

    return false;

#endif
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************       cause program to break execution       ************** //
// **************           in debugger at call point          ************** //
// ************************************************************************** //

void
debugger_break()
{
    // !! ?? auto-start debugger?

#if defined(BOOST_WIN32_BASED_DEBUG) // *********************** WIN32

#if defined(__GNUC__) && !defined(__MINGW32__)   ||  \
    defined(__INTEL_COMPILER) || defined(BOOST_EMBTC)
#   define BOOST_DEBUG_BREAK    __debugbreak
#else
#   define BOOST_DEBUG_BREAK    DebugBreak
#endif

#ifndef __MINGW32__
    if( !under_debugger() ) {
        __try {
            __try {
                BOOST_DEBUG_BREAK();
            }
            __except( UnhandledExceptionFilter(GetExceptionInformation()) )
            {
                // User opted to ignore the breakpoint
                return;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // If we got here, the user has pushed Debug. Debugger is already attached to our process and we
            // continue to let the another BOOST_DEBUG_BREAK to be called.
        }
    }
#endif

    BOOST_DEBUG_BREAK();

#elif defined(BOOST_UNIX_BASED_DEBUG) // ********************** UNIX

    ::kill( ::getpid(), SIGTRAP );

#else // ****************************************************** default

#endif
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************            console debugger setup            ************** //
// ************************************************************************** //

#if defined(BOOST_UNIX_BASED_DEBUG) // ************************ UNIX

std::string
set_debugger( unit_test::const_string dbg_id, dbg_starter s )
{
    std::string old = s_info.p_dbg;

    assign_op( s_info.p_dbg.value, dbg_id, 0 );

    if( !!s )
        s_info.m_dbg_starter_reg[s_info.p_dbg.get()] = s;

    return old;
}

#else  // ***************************************************** default

std::string
set_debugger( unit_test::const_string, dbg_starter )
{
    return std::string();
}

#endif

//____________________________________________________________________________//

// ************************************************************************** //
// **************    attach debugger to the current process    ************** //
// ************************************************************************** //

#if defined(BOOST_WIN32_BASED_DEBUG)

struct safe_handle_helper
{
    HANDLE& handle;
    safe_handle_helper(HANDLE &handle_) : handle(handle_) {}

    void close_handle()
    {
        if( handle != INVALID_HANDLE_VALUE )
        {
            ::CloseHandle( handle );
            handle = INVALID_HANDLE_VALUE;
        }
    }

    ~safe_handle_helper()
    {
        close_handle();
    }
};
#endif

bool
attach_debugger( bool break_or_continue )
{
    if( under_debugger() )
        return false;

#if defined(BOOST_WIN32_BASED_DEBUG) // *********************** WIN32

    const int MAX_CMD_LINE = 200;

    // *************************************************** //
    // Debugger "ready" event

    SECURITY_ATTRIBUTES attr;
    attr.nLength                = sizeof(attr);
    attr.lpSecurityDescriptor   = NULL;
    attr.bInheritHandle         = true;

    // manual resettable, initially non signaled, unnamed event,
    // that will signal me that debugger initialization is done
    HANDLE dbg_init_done_ev = ::CreateEvent(
        &attr,          // pointer to security attributes
        true,           // flag for manual-reset event
        false,          // flag for initial state
        NULL            // pointer to event-object name
    );

    if( !dbg_init_done_ev )
        return false;

    safe_handle_helper safe_handle_obj( dbg_init_done_ev );

    // *************************************************** //
    // Debugger command line format

    HKEY reg_key;

    if( !s_info.m_reg_open_key || (*s_info.m_reg_open_key)(
            HKEY_LOCAL_MACHINE,                                         // handle of open key
            "Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug", // name of subkey to open
            &reg_key ) != ERROR_SUCCESS )                               // address of handle of open key
        return false;

    char  format[MAX_CMD_LINE];
    DWORD format_size = MAX_CMD_LINE;
    DWORD type = REG_SZ;

    bool b_read_key = s_info.m_reg_query_value &&
          ((*s_info.m_reg_query_value)(
            reg_key,                            // handle of open key
            "Debugger",                         // name of subkey to query
            0,                                  // reserved
            &type,                              // value type
            (LPBYTE)format,                     // buffer for returned string
            &format_size ) == ERROR_SUCCESS );  // in: buffer size; out: actual size of returned string

    if( !s_info.m_reg_close_key || (*s_info.m_reg_close_key)( reg_key ) != ERROR_SUCCESS )
        return false;

    if( !b_read_key )
        return false;

    // *************************************************** //
    // Debugger command line

    char cmd_line[MAX_CMD_LINE];
    std::sprintf( cmd_line, format, ::GetCurrentProcessId(), dbg_init_done_ev );

    // *************************************************** //
    // Debugger window parameters

    STARTUPINFOA    startup_info;
    std::memset( &startup_info, 0, sizeof(startup_info) );

    startup_info.cb             = sizeof(startup_info);
    startup_info.dwFlags        = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow    = SW_SHOWNORMAL;

    // debugger process s_info
    PROCESS_INFORMATION debugger_info;

    bool created = !!::CreateProcessA(
        NULL,           // pointer to name of executable module; NULL - use the one in command line
        cmd_line,       // pointer to command line string
        NULL,           // pointer to process security attributes; NULL - debugger's handle can't be inherited
        NULL,           // pointer to thread security attributes; NULL - debugger's handle can't be inherited
        true,           // debugger inherit opened handles
        0,              // priority flags; 0 - normal priority
        NULL,           // pointer to new environment block; NULL - use this process environment
        NULL,           // pointer to current directory name; NULL - use this process correct directory
        &startup_info,  // pointer to STARTUPINFO that specifies main window appearance
        &debugger_info  // pointer to PROCESS_INFORMATION that will contain the new process identification
    );

    bool debugger_run_ok = false;
    if( created )
    {
        DWORD ret_code = ::WaitForSingleObject( dbg_init_done_ev, INFINITE );
        debugger_run_ok = ( ret_code == WAIT_OBJECT_0 );
    }

    safe_handle_obj.close_handle();

    if( !created || !debugger_run_ok )
        return false;

    if( break_or_continue )
        debugger_break();

    return true;

#elif defined(BOOST_UNIX_BASED_DEBUG) // ********************** UNIX

    char init_done_lock_fn[] = "/tmp/btl_dbg_init_done_XXXXXX";
    const mode_t cur_umask = ::umask( S_IRWXO | S_IRWXG );
    fd_holder init_done_lock_fd( ::mkstemp( init_done_lock_fn ) );
    ::umask( cur_umask );

    if( init_done_lock_fd == -1 )
        return false;

    pid_t child_pid = fork();

    if( child_pid == -1 )
        return false;

    if( child_pid != 0 ) { // parent process - here we will start the debugger
        dbg_startup_info dsi;

        process_info pi( child_pid );
        if( pi.binary_path().is_empty() )
            ::exit( -1 );

        dsi.pid                 = child_pid;
        dsi.break_or_continue   = break_or_continue;
        dsi.binary_path         = pi.binary_path();
        dsi.display             = ::getenv( "DISPLAY" );
        dsi.init_done_lock      = init_done_lock_fn;

        dbg_starter starter = s_info.m_dbg_starter_reg[s_info.p_dbg];
        if( !!starter )
            starter( dsi );

        ::perror( "Boost.Test execution monitor failed to start a debugger:" );

        ::exit( -1 );
    }

    // child process - here we will continue our test module execution ; // !! ?? should it be vice versa

    while( ::access( init_done_lock_fn, F_OK ) == 0 ) {
        struct timeval to = { 0, 100 };

        ::select( 0, 0, 0, 0, &to );
    }

//    char dummy;
//    while( ::read( init_done_lock_fd, &dummy, sizeof(char) ) == 0 );

    if( break_or_continue )
        debugger_break();

    return true;

#else // ****************************************************** default
    (void) break_or_continue; // silence 'unused variable' warning
    return false;

#endif
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************   switch on/off detect memory leaks feature  ************** //
// ************************************************************************** //

void
detect_memory_leaks( bool on_off, unit_test::const_string report_file )
{
    boost::ignore_unused( on_off );

#ifdef BOOST_MS_CRT_BASED_DEBUG
    int flags = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

    if( !on_off )
        flags &= ~_CRTDBG_LEAK_CHECK_DF;
    else  {
        flags |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);

        if( report_file.is_empty() )
            _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
        else {
            HANDLE hreport_f = ::CreateFileA( report_file.begin(),
                                              GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            _CrtSetReportFile(_CRT_WARN, hreport_f );
        }
    }

    _CrtSetDbgFlag ( flags );
#else
    boost::ignore_unused( report_file );
#endif // BOOST_MS_CRT_BASED_DEBUG
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************      cause program to break execution in     ************** //
// **************     debugger at specific allocation point    ************** //
// ************************************************************************** //

void
break_memory_alloc( long mem_alloc_order_num )
{
    boost::ignore_unused( mem_alloc_order_num );

#ifdef BOOST_MS_CRT_BASED_DEBUG
    // only set the value if one was supplied (do not use default used by UTF just as a indicator to enable leak detection)
    if( mem_alloc_order_num > 1 )
        _CrtSetBreakAlloc( mem_alloc_order_num );
#endif // BOOST_MS_CRT_BASED_DEBUG
}

//____________________________________________________________________________//

} // namespace debug
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DEBUG_API_IPP_112006GER
