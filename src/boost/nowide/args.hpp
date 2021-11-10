//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NOWIDE_ARGS_HPP_INCLUDED
#define BOOST_NOWIDE_ARGS_HPP_INCLUDED

#include <boost/config.hpp>
#ifdef BOOST_WINDOWS
#include <boost/nowide/stackstring.hpp>
#include <boost/nowide/windows.hpp>
#include <stdexcept>
#include <vector>
#endif

namespace boost {
namespace nowide {
#if !defined(BOOST_WINDOWS) && !defined(BOOST_NOWIDE_DOXYGEN)
    class args
    {
    public:
        args(int&, char**&)
        {}
        args(int&, char**&, char**&)
        {}
    };

#else

    ///
    /// \brief \c args is a class that temporarily replaces standard main() function arguments with their
    /// equal, but UTF-8 encoded values under Microsoft Windows for the lifetime of the instance.
    ///
    /// The class uses \c GetCommandLineW(), \c CommandLineToArgvW() and \c GetEnvironmentStringsW()
    /// in order to obtain Unicode-encoded values.
    /// It does not relate to actual values of argc, argv and env under Windows.
    ///
    /// It restores the original values in its destructor (usually at the end of the \c main function).
    ///
    /// If any of the system calls fails, an exception of type std::runtime_error will be thrown
    /// and argc, argv, env remain unchanged.
    ///
    /// \note The class owns the memory of the newly allocated strings.
    /// So you need to keep it alive as long as you use the values.
    ///
    /// Usage:
    /// \code
    /// int main(int argc, char** argv, char** env) {
    ///   boost::nowide::args _(argc, argv, env); // Note the _ as a "don't care" name for the instance
    ///   // Use argv and env as usual, they are now UTF-8 encoded on Windows
    ///   return 0; // Memory held by args is released
    /// }
    /// \endcode
    class args
    {
    public:
        ///
        /// Fix command line arguments
        ///
        args(int& argc, char**& argv) :
            old_argc_(argc), old_argv_(argv), old_env_(0), old_argc_ptr_(&argc), old_argv_ptr_(&argv), old_env_ptr_(0)
        {
            fix_args(argc, argv);
        }
        ///
        /// Fix command line arguments and environment
        ///
        args(int& argc, char**& argv, char**& env) :
            old_argc_(argc), old_argv_(argv), old_env_(env), old_argc_ptr_(&argc), old_argv_ptr_(&argv),
            old_env_ptr_(&env)
        {
            fix_args(argc, argv);
            fix_env(env);
        }
        ///
        /// Restore original argc, argv, env values, if changed
        ///
        ~args()
        {
            if(old_argc_ptr_)
                *old_argc_ptr_ = old_argc_;
            if(old_argv_ptr_)
                *old_argv_ptr_ = old_argv_;
            if(old_env_ptr_)
                *old_env_ptr_ = old_env_;
        }

    private:
        class wargv_ptr
        {
            wchar_t** p;
            int argc;

        public:
            wargv_ptr()
            {
                p = CommandLineToArgvW(GetCommandLineW(), &argc);
            }
            ~wargv_ptr()
            {
                if(p)
                    LocalFree(p);
            }
            wargv_ptr(const wargv_ptr&) = delete;
            wargv_ptr& operator=(const wargv_ptr&) = delete;

            int size() const
            {
                return argc;
            }
            operator bool() const
            {
                return p != NULL;
            }
            const wchar_t* operator[](size_t i) const
            {
                return p[i];
            }
        };
        class wenv_ptr
        {
            wchar_t* p;

        public:
            wenv_ptr() : p(GetEnvironmentStringsW())
            {}
            ~wenv_ptr()
            {
                if(p)
                    FreeEnvironmentStringsW(p);
            }
            wenv_ptr(const wenv_ptr&) = delete;
            wenv_ptr& operator=(const wenv_ptr&) = delete;

            operator const wchar_t*() const
            {
                return p;
            }
        };

        void fix_args(int& argc, char**& argv)
        {
            const wargv_ptr wargv;
            if(!wargv)
                throw std::runtime_error("Could not get command line!");
            args_.resize(wargv.size() + 1, 0);
            arg_values_.resize(wargv.size());
            for(int i = 0; i < wargv.size(); i++)
                args_[i] = arg_values_[i].convert(wargv[i]);
            argc = wargv.size();
            argv = &args_[0];
        }
        void fix_env(char**& env)
        {
            const wenv_ptr wstrings;
            if(!wstrings)
                throw std::runtime_error("Could not get environment strings!");
            const wchar_t* wstrings_end = 0;
            int count = 0;
            for(wstrings_end = wstrings; *wstrings_end; wstrings_end += wcslen(wstrings_end) + 1)
                count++;
            env_.convert(wstrings, wstrings_end);
            envp_.resize(count + 1, 0);
            char* p = env_.get();
            int pos = 0;
            for(int i = 0; i < count; i++)
            {
                if(*p != '=')
                    envp_[pos++] = p;
                p += strlen(p) + 1;
            }
            env = &envp_[0];
        }

        std::vector<char*> args_;
        std::vector<short_stackstring> arg_values_;
        stackstring env_;
        std::vector<char*> envp_;

        int old_argc_;
        char** old_argv_;
        char** old_env_;

        int* old_argc_ptr_;
        char*** old_argv_ptr_;
        char*** old_env_ptr_;
    };

#endif

} // namespace nowide
} // namespace boost
#endif
