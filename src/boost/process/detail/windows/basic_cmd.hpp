// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PROCESS_DETAIL_WINDOWS_BASIC_CMD_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_BASIC_CMD_HPP_

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/process/shell.hpp>
#include <boost/process/detail/windows/handler.hpp>

#include <vector>
#include <string>
#include <iterator>


namespace boost
{
namespace process
{
namespace detail
{
namespace windows
{

inline std::string build_args(const std::string & exe, std::vector<std::string> && data)
{
    std::string st = exe;

    //put in quotes if it has spaces or double quotes
    if(!exe.empty())
    {
        auto it = st.find_first_of(" \"");

        if(it != st.npos)//contains spaces.
        {
            // double existing quotes
            boost::replace_all(st, "\"", "\"\"");

            // surround with quotes
            st.insert(st.begin(), '"');
            st += '"';
        }
    }

    for (auto & arg : data)
    {
        if(!arg.empty())
        {
            auto it = arg.find_first_of(" \"");//contains space or double quotes?
            if(it != arg.npos)//yes
            {
                // double existing quotes
                boost::replace_all(arg, "\"", "\"\"");

                // surround with quotes
                arg.insert(arg.begin(), '"');
                arg += '"';
            }
        }

        if (!st.empty())//first one does not need a preceeding space
            st += ' ';

        st += arg;
    }
    return st;
}

inline std::wstring build_args(const std::wstring & exe, std::vector<std::wstring> && data)
{
    std::wstring st = exe;

    //put in quotes if it has spaces or double quotes
    if(!exe.empty())
    {
        auto it = st.find_first_of(L" \"");

        if(it != st.npos)//contains spaces or double quotes.
        {
            // double existing quotes
            boost::replace_all(st, L"\"", L"\"\"");

            // surround with quotes
            st.insert(st.begin(), L'"');
            st += L'"';
        }
    }

    for(auto & arg : data)
    {
        if(!arg.empty())
        {
            auto it = arg.find_first_of(L" \"");//contains space or double quotes?
            if(it != arg.npos)//yes
            {
                // double existing quotes
                boost::replace_all(arg, L"\"", L"\"\"");

                // surround with quotes
                arg.insert(arg.begin(), L'"');
                arg += '"';
            }
        }

        if (!st.empty())//first one does not need a preceeding space
            st += L' ';

        st += arg;
    }
    return st;
}

template<typename Char>
struct exe_cmd_init : handler_base_ext
{
    using value_type  = Char;
    using string_type = std::basic_string<value_type>;

    static const char*    c_arg(char)    { return "/c";}
    static const wchar_t* c_arg(wchar_t) { return L"/c";}

    exe_cmd_init(const string_type & exe, bool cmd_only = false)
                : exe(exe), args({}), cmd_only(cmd_only) {};
    exe_cmd_init(string_type && exe, bool cmd_only = false)
                : exe(std::move(exe)), args({}), cmd_only(cmd_only) {};

    exe_cmd_init(string_type && exe, std::vector<string_type> && args)
            : exe(std::move(exe)), args(build_args(this->exe, std::move(args))), cmd_only(false) {};
    template <class Executor>
    void on_setup(Executor& exec) const
    {

        if (cmd_only && args.empty())
            exec.cmd_line = exe.c_str();
        else
        {
            exec.exe = exe.c_str();
            exec.cmd_line = args.c_str();
        }
    }
    static exe_cmd_init<Char> exe_args(string_type && exe, std::vector<string_type> && args)
    {
        return exe_cmd_init<Char>(std::move(exe), std::move(args));
    }
    static exe_cmd_init<Char> cmd(string_type&& cmd)
    {
        return exe_cmd_init<Char>(std::move(cmd), true);
    }
    static exe_cmd_init<Char> exe_args_shell(string_type && exe, std::vector<string_type> && args)
    {
        std::vector<string_type> args_ = {c_arg(Char()), std::move(exe)};
        args_.insert(args_.end(), std::make_move_iterator(args.begin()), std::make_move_iterator(args.end()));
        string_type sh = get_shell(Char());

        return exe_cmd_init<Char>(std::move(sh), std::move(args_));
    }

    static std:: string get_shell(char)    {return shell(). string(codecvt()); }
    static std::wstring get_shell(wchar_t) {return shell().wstring(codecvt());}

    static exe_cmd_init<Char> cmd_shell(string_type&& cmd)
    {
        std::vector<string_type> args = {c_arg(Char()), std::move(cmd)};
        string_type sh = get_shell(Char());

        return exe_cmd_init<Char>(
                std::move(sh),
                std::move(args));
    }
private:
    string_type exe;
    string_type args;
    bool cmd_only;
};

}



}
}
}



#endif /* INCLUDE_BOOST_PROCESS_WINDOWS_ARGS_HPP_ */
