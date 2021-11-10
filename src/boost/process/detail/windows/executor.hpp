// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_EXECUTOR_HPP
#define BOOST_PROCESS_WINDOWS_EXECUTOR_HPP

#include <boost/process/detail/child_decl.hpp>
#include <boost/process/detail/windows/is_running.hpp>
#include <boost/process/detail/traits.hpp>
#include <boost/process/error.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/process.hpp>
#include <boost/none.hpp>
#include <system_error>
#include <memory>
#include <atomic>
#include <cstring>

namespace boost { namespace process {

namespace detail { namespace windows {

template<typename CharType> struct startup_info;
#if !defined( BOOST_NO_ANSI_APIS )

template<> struct startup_info<char>
{
    typedef ::boost::winapi::STARTUPINFOA_ type;
};
#endif

template<> struct startup_info<wchar_t>
{
    typedef ::boost::winapi::STARTUPINFOW_ type;
};

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6

template<typename CharType> struct startup_info_ex;

#if !defined( BOOST_NO_ANSI_APIS )
template<> struct startup_info_ex<char>
{
    typedef ::boost::winapi::STARTUPINFOEXA_ type;
};
#endif

template<> struct startup_info_ex<wchar_t>
{
    typedef ::boost::winapi::STARTUPINFOEXW_ type;
};


#endif

#if ( BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6 )

template<typename CharT>
struct startup_info_impl
{
    ::boost::winapi::DWORD_ creation_flags = 0;

    typedef typename startup_info_ex<CharT>::type startup_info_ex_t;
    typedef typename startup_info<CharT>::type    startup_info_t;

    startup_info_ex_t  startup_info_ex
            {startup_info_t {sizeof(startup_info_t), nullptr, nullptr, nullptr,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr,
                               ::boost::winapi::invalid_handle_value,
                               ::boost::winapi::invalid_handle_value,
                               ::boost::winapi::invalid_handle_value},
                nullptr
    };
    startup_info_t & startup_info = startup_info_ex.StartupInfo;

    void set_startup_info_ex()
    {
       startup_info.cb = sizeof(startup_info_ex_t);
       creation_flags |= ::boost::winapi::EXTENDED_STARTUPINFO_PRESENT_;
    }
};


#else

template<typename CharT>
struct startup_info_impl
{
    typedef typename startup_info<CharT>::type    startup_info_t;

    ::boost::winapi::DWORD_ creation_flags = 0;
    startup_info_t          startup_info
            {sizeof(startup_info_t), nullptr, nullptr, nullptr,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr,
             ::boost::winapi::invalid_handle_value,
             ::boost::winapi::invalid_handle_value,
             ::boost::winapi::invalid_handle_value};
};
#endif



template<typename Char, typename Sequence>
class executor : public startup_info_impl<Char>
{

    void internal_error_handle(const std::error_code &, const char*, boost::mpl::false_, boost::mpl::true_) {}
    void internal_error_handle(const std::error_code &, const char*, boost::mpl::true_,  boost::mpl::true_) {}

    void internal_error_handle(const std::error_code &ec, const char*, boost::mpl::true_,  boost::mpl::false_ )
    {
        this->_ec = ec;
    }
    void internal_error_handle(const std::error_code &ec, const char* msg, boost::mpl::false_, boost::mpl::false_ )
    {
        throw process_error(ec, msg);
    }

    struct on_setup_t
    {
        executor & exec;
        on_setup_t(executor & exec) : exec(exec) {};
        template<typename T>
        void operator()(T & t) const
        {
            if (!exec.error())
                t.on_setup(exec);
        }
    };

    struct on_error_t
    {
        executor & exec;
        const std::error_code & error;
        on_error_t(executor & exec, const std::error_code & error) : exec(exec), error(error) {};
        template<typename T>
        void operator()(T & t) const
        {
            t.on_error(exec, error);
        }
    };

    struct on_success_t
    {
        executor & exec;
        on_success_t(executor & exec) : exec(exec) {};
        template<typename T>
        void operator()(T & t) const
        {
            if (!exec.error())
                t.on_success(exec);
        }
    };

    typedef typename ::boost::process::detail::has_error_handler<Sequence>::type has_error_handler;
    typedef typename ::boost::process::detail::has_ignore_error <Sequence>::type has_ignore_error;

    std::error_code _ec{0, std::system_category()};

public:

    std::shared_ptr<std::atomic<int>> exit_status = std::make_shared<std::atomic<int>>(still_active);

    executor(Sequence & seq) : seq(seq)
    {
    }

    child operator()()
    {
        on_setup_t on_setup_fn(*this);
        boost::fusion::for_each(seq, on_setup_fn);

        if (_ec)
        {
            on_error_t on_error_fn(*this, _ec);
            boost::fusion::for_each(seq, on_error_fn);
            return child();
        }

        //NOTE: The non-cast cmd-line string can only be modified by the wchar_t variant which is currently disabled.
        int err_code = ::boost::winapi::create_process(
            exe,                                        //       LPCSTR_ lpApplicationName,
            const_cast<Char*>(cmd_line),                //       LPSTR_ lpCommandLine,
            proc_attrs,                                 //       LPSECURITY_ATTRIBUTES_ lpProcessAttributes,
            thread_attrs,                               //       LPSECURITY_ATTRIBUTES_ lpThreadAttributes,
            inherit_handles,                            //       INT_ bInheritHandles,
            this->creation_flags,                       //       DWORD_ dwCreationFlags,
            reinterpret_cast<void*>(const_cast<Char*>(env)),  //     LPVOID_ lpEnvironment,
            work_dir,                                   //       LPCSTR_ lpCurrentDirectory,
            &this->startup_info,                        //       LPSTARTUPINFOA_ lpStartupInfo,
            &proc_info);                                //       LPPROCESS_INFORMATION_ lpProcessInformation)

        child c{child_handle(proc_info), exit_status};

        if (err_code != 0)
        {
            _ec.clear();
            on_success_t on_success_fn(*this);
            boost::fusion::for_each(seq, on_success_fn);
        }
        else
            set_error(::boost::process::detail::get_last_error(),
                    " CreateProcess failed");

        if ( _ec)
        {
            on_error_t on_err(*this, _ec);
            boost::fusion::for_each(seq, on_err);
            return child();
        }
        else
            return c;

    }

    void set_error(const std::error_code & ec, const char* msg = "Unknown Error.")
    {
        internal_error_handle(ec, msg, has_error_handler(),         has_ignore_error());
    }
    void set_error(const std::error_code & ec, const std::string msg = "Unknown Error.")
    {
        internal_error_handle(ec, msg.c_str(), has_error_handler(), has_ignore_error());
    }

    const std::error_code& error() const {return _ec;}

    ::boost::winapi::LPSECURITY_ATTRIBUTES_ proc_attrs   = nullptr;
    ::boost::winapi::LPSECURITY_ATTRIBUTES_ thread_attrs = nullptr;
    ::boost::winapi::BOOL_ inherit_handles = false;
    const Char * work_dir = nullptr;
    const Char * cmd_line = nullptr;
    const Char * exe      = nullptr;
    const Char * env      = nullptr;


    Sequence & seq;
    ::boost::winapi::PROCESS_INFORMATION_ proc_info{nullptr, nullptr, 0,0};
};



template<typename Char, typename Tup>
executor<Char, Tup> make_executor(Tup & tup)
{
    return executor<Char, Tup>(tup);
}


}}}}

#endif
