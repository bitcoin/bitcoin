// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PROCESS_DETAIL_BASIC_CMD_HPP_
#define BOOST_PROCESS_DETAIL_BASIC_CMD_HPP_

#include <boost/process/detail/config.hpp>

#include <boost/process/detail/handler_base.hpp>
#include <boost/process/detail/traits/cmd_or_exe.hpp>
#include <boost/process/detail/traits/wchar_t.hpp>

#if defined( BOOST_WINDOWS_API )
#include <boost/process/detail/windows/basic_cmd.hpp>
#include <boost/process/detail/windows/cmd.hpp>
#elif defined( BOOST_POSIX_API )
#include <boost/process/detail/posix/basic_cmd.hpp>
#include <boost/process/detail/posix/cmd.hpp>
#endif

#include <boost/process/shell.hpp>

#include <iterator>


namespace boost { namespace process { namespace detail {

template<typename Char>
struct exe_setter_
{
    typedef Char value_type;
    typedef std::basic_string<Char> string_type;

    string_type exe_;
    exe_setter_(string_type && str)      : exe_(std::move(str)) {}
    exe_setter_(const string_type & str) : exe_(str) {}
};

template<> struct is_wchar_t<exe_setter_<wchar_t>> : std::true_type {};


template<>
struct char_converter<char, exe_setter_<wchar_t>>
{
    static exe_setter_<char> conv(const exe_setter_<wchar_t> & in)
    {
        return {::boost::process::detail::convert(in.exe_)};
    }
};

template<>
struct char_converter<wchar_t, exe_setter_<char>>
{
    static exe_setter_<wchar_t> conv(const exe_setter_<char> & in)
    {
        return {::boost::process::detail::convert(in.exe_)};
    }
};



template <typename Char, bool Append >
struct arg_setter_
{
    using value_type = Char;
    using string_type = std::basic_string<value_type>;
    std::vector<string_type> _args;

    typedef typename std::vector<string_type>::iterator       iterator;
    typedef typename std::vector<string_type>::const_iterator const_iterator;

    template<typename Iterator>
    arg_setter_(Iterator && begin, Iterator && end) : _args(begin, end) {}

    template<typename Range>
    arg_setter_(Range && str) :
            _args(std::begin(str),
                  std::end(str)) {}

    iterator begin() {return _args.begin();}
    iterator end()   {return _args.end();}
    const_iterator begin() const {return _args.begin();}
    const_iterator end()   const {return _args.end();}
    arg_setter_(string_type & str)     : _args{{str}} {}
    arg_setter_(string_type && s)      : _args({std::move(s)}) {}
    arg_setter_(const string_type & s) : _args({s}) {}
    arg_setter_(const value_type* s)   : _args({std::move(s)}) {}

    template<std::size_t Size>
    arg_setter_(const value_type (&s) [Size]) : _args({s}) {}
};

template<> struct is_wchar_t<arg_setter_<wchar_t, true >> : std::true_type {};
template<> struct is_wchar_t<arg_setter_<wchar_t, false>> : std::true_type {};

template<>
struct char_converter<char, arg_setter_<wchar_t, true>>
{
    static arg_setter_<char, true> conv(const arg_setter_<wchar_t, true> & in)
    {
        std::vector<std::string> vec(in._args.size());
        std::transform(in._args.begin(), in._args.end(), vec.begin(),
                [](const std::wstring & ws)
                {
                    return ::boost::process::detail::convert(ws);
                });
        return {vec};
    }
};

template<>
struct char_converter<wchar_t, arg_setter_<char, true>>
{
    static arg_setter_<wchar_t, true> conv(const arg_setter_<char, true> & in)
    {
        std::vector<std::wstring> vec(in._args.size());
        std::transform(in._args.begin(), in._args.end(), vec.begin(),
                [](const std::string & ws)
                {
                    return ::boost::process::detail::convert(ws);
                });

        return {vec};
    }
};

template<>
struct char_converter<char, arg_setter_<wchar_t, false>>
{
    static arg_setter_<char, false> conv(const arg_setter_<wchar_t, false> & in)
    {
        std::vector<std::string> vec(in._args.size());
        std::transform(in._args.begin(), in._args.end(), vec.begin(),
                [](const std::wstring & ws)
                {
                    return ::boost::process::detail::convert(ws);
                });
        return {vec};    }
};

template<>
struct char_converter<wchar_t, arg_setter_<char, false>>
{
    static arg_setter_<wchar_t, false> conv(const arg_setter_<char, false> & in)
    {
        std::vector<std::wstring> vec(in._args.size());
        std::transform(in._args.begin(), in._args.end(), vec.begin(),
                [](const std::string & ws)
                {
                    return ::boost::process::detail::convert(ws);
                });
        return {vec};
    }
};

using api::exe_cmd_init;

template<typename Char>
struct exe_builder
{
    //set by path, because that will not be interpreted as a cmd
    bool not_cmd = false;
    bool shell   = false;
    using string_type = std::basic_string<Char>;
    string_type exe;
    std::vector<string_type> args;

    void operator()(const boost::filesystem::path & data)
    {
        not_cmd = true;
        if (exe.empty())
            exe = data.native();
        else
            args.push_back(data.native());
    }

    void operator()(const string_type & data)
    {
        if (exe.empty())
            exe = data;
        else
            args.push_back(data);
    }
    void operator()(const Char* data)
    {
        if (exe.empty())
            exe = data;
        else
            args.push_back(data);
    }
    void operator()(shell_) {shell = true;}
    void operator()(std::vector<string_type> && data)
    {
        if (data.empty())
            return;

        auto itr = std::make_move_iterator(data.begin());
        auto end = std::make_move_iterator(data.end());

        if (exe.empty())
        {
            exe = *itr;
            itr++;
        }
        args.insert(args.end(), itr, end);
    }

    void operator()(const std::vector<string_type> & data)
    {
        if (data.empty())
            return;

        auto itr = data.begin();
        auto end = data.end();

        if (exe.empty())
        {
            exe = *itr;
            itr++;
        }
        args.insert(args.end(), itr, end);
    }
    void operator()(exe_setter_<Char> && data)
    {
        not_cmd = true;
        exe = std::move(data.exe_);
    }
    void operator()(const exe_setter_<Char> & data)
    {
        not_cmd = true;
        exe = data.exe_;
    }
    void operator()(arg_setter_<Char, false> && data)
    {
        args.assign(
                std::make_move_iterator(data._args.begin()),
                std::make_move_iterator(data._args.end()));
    }
    void operator()(arg_setter_<Char, true> && data)
    {
        args.insert(args.end(),
                std::make_move_iterator(data._args.begin()),
                std::make_move_iterator(data._args.end()));
    }
    void operator()(const arg_setter_<Char, false> & data)
    {
        args.assign(data._args.begin(), data._args.end());
    }
    void operator()(const arg_setter_<Char, true> & data)
    {
        args.insert(args.end(), data._args.begin(), data._args.end());
    }

    api::exe_cmd_init<Char> get_initializer()
    {
        if (not_cmd || !args.empty())
        {
            if (shell)
                return api::exe_cmd_init<Char>::exe_args_shell(std::move(exe), std::move(args));
            else
                return api::exe_cmd_init<Char>::exe_args(std::move(exe), std::move(args));
        }
        else
            if (shell)
                return api::exe_cmd_init<Char>::cmd_shell(std::move(exe));
            else
                return api::exe_cmd_init<Char>::cmd(std::move(exe));

    }
    typedef api::exe_cmd_init<Char> result_type;
};

template<>
struct initializer_builder<cmd_or_exe_tag<char>>
{
    typedef exe_builder<char> type;
};

template<>
struct initializer_builder<cmd_or_exe_tag<wchar_t>>
{
    typedef exe_builder<wchar_t> type;
};

}}}



#endif /* BOOST_PROCESS_DETAIL_EXE_BUILDER_HPP_ */
