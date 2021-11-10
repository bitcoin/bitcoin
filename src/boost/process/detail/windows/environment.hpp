// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_ENV_STORAGE_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_ENV_STORAGE_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <boost/winapi/error_codes.hpp>
#include <boost/winapi/environment.hpp>
#include <boost/winapi/get_current_process.hpp>
#include <boost/winapi/get_current_process_id.hpp>
#include <boost/process/detail/config.hpp>
#include <algorithm>
#include <boost/process/locale.hpp>

namespace boost { namespace process { namespace detail { namespace windows {

template<typename Char>
class native_environment_impl
{
    static void _deleter(Char* p) {boost::winapi::free_environment_strings(p);};
    std::unique_ptr<Char[], void(*)(Char*)> _buf{boost::winapi::get_environment_strings<Char>(), &native_environment_impl::_deleter};
    static inline std::vector<Char*> _load_var(Char* p);
    std::vector<Char*> _env_arr{_load_var(_buf.get())};
public:
    using char_type = Char;
    using pointer_type = const char_type*;
    using string_type = std::basic_string<char_type>;
    using native_handle_type = pointer_type;
    void reload()
    {
        _buf.reset(boost::winapi::get_environment_strings<Char>());
        _env_arr = _load_var(_buf.get());
        _env_impl = &*_env_arr.begin();
    }

    string_type get(const pointer_type id);
    void        set(const pointer_type id, const pointer_type value);
    void      reset(const pointer_type id);

    string_type get(const string_type & id) {return get(id.c_str());}
    void        set(const string_type & id, const string_type & value) {set(id.c_str(), value.c_str()); }
    void      reset(const string_type & id) {reset(id.c_str());}

    native_environment_impl() = default;
    native_environment_impl(const native_environment_impl& ) = delete;
    native_environment_impl(native_environment_impl && ) = default;
    native_environment_impl & operator=(const native_environment_impl& ) = delete;
    native_environment_impl & operator=(native_environment_impl && ) = default;
    Char ** _env_impl = &*_env_arr.begin();

    native_handle_type native_handle() const {return _buf.get();}
};

template<typename Char>
inline auto native_environment_impl<Char>::get(const pointer_type id) -> string_type
{
    Char buf[4096];
    auto size = boost::winapi::get_environment_variable(id, buf, sizeof(buf));
    if (size == 0) //failed
    {
        auto err =  ::boost::winapi::GetLastError();
        if (err == ::boost::winapi::ERROR_ENVVAR_NOT_FOUND_)//well, then we consider that an empty value
            return "";
        else
            throw process_error(std::error_code(err, std::system_category()),
                               "GetEnvironmentVariable() failed");
    }

    if (size == sizeof(buf)) //the return size gives the size without the null, so I know this went wrong
    {
        /*limit defined here https://msdn.microsoft.com/en-us/library/windows/desktop/ms683188(v=vs.85).aspx
         * but I used 32768 so it is a multiple of 4096.
         */
        constexpr static std::size_t max_size = 32768;
        //Handle variables longer then buf.
        std::size_t buf_size = sizeof(buf);
        while (buf_size <= max_size)
        {
            std::vector<Char> buf(buf_size);
            auto size = boost::winapi::get_environment_variable(id, buf.data(), buf.size());

            if (size == buf_size) //buffer to small
                buf_size *= 2;
            else if (size == 0)
                ::boost::process::detail::throw_last_error("GetEnvironmentVariable() failed");
            else
                return std::basic_string<Char>(
                        buf.data(), buf.data()+ size + 1);

        }

    }
    return std::basic_string<Char>(buf, buf+size+1);
}

template<typename Char>
inline void native_environment_impl<Char>::set(const pointer_type id, const pointer_type value)
{
    boost::winapi::set_environment_variable(id, value);
}

template<typename Char>
inline void  native_environment_impl<Char>::reset(const pointer_type id)
{
    boost::winapi::set_environment_variable(id, nullptr);
}

template<typename Char>
std::vector<Char*> native_environment_impl<Char>::_load_var(Char* p)
{
    std::vector<Char*> ret;
    if (*p != null_char<Char>())
    {
        ret.push_back(p);
        while ((*p != null_char<Char>()) || (*(p+1) !=  null_char<Char>()))
        {
            if (*p==null_char<Char>())
            {
                p++;
                ret.push_back(p);
            }
            else
                p++;
        }
    }
    p++;
    ret.push_back(nullptr);

    return ret;
}


template<typename Char>
struct basic_environment_impl
{
    std::vector<Char> _data = {null_char<Char>()};
    static std::vector<Char*> _load_var(Char* p);
    std::vector<Char*> _env_arr{_load_var(_data.data())};
public:
    using char_type = Char;
    using pointer_type = const char_type*;
    using string_type = std::basic_string<char_type>;
    using native_handle_type = pointer_type;

    std::size_t size() const { return _data.size();}

    void reload()
    {
        _env_arr = _load_var(_data.data());
        _env_impl = _env_arr.data();
    }

    string_type get(const pointer_type id) {return get(string_type(id));}
    void        set(const pointer_type id, const pointer_type value) {set(string_type(id), value);}
    void      reset(const pointer_type id)  {reset(string_type(id));}

    string_type get(const string_type & id);
    void        set(const string_type & id, const string_type & value);
    void      reset(const string_type & id);

    inline basic_environment_impl(const native_environment_impl<Char> & nei);
    basic_environment_impl() = default;
    basic_environment_impl(const basic_environment_impl& rhs)
        : _data(rhs._data)
    {
    }
    basic_environment_impl(basic_environment_impl && rhs)
        :    _data(std::move(rhs._data)),
            _env_arr(std::move(rhs._env_arr)),
            _env_impl(_env_arr.data())
    {
    }
    basic_environment_impl &operator=(basic_environment_impl && rhs)
    {
        _data = std::move(rhs._data);
        //reload();
        _env_arr  = std::move(rhs._env_arr);
        _env_impl = _env_arr.data();

        return *this;
    }
    basic_environment_impl & operator=(const basic_environment_impl& rhs)
    {
        _data = rhs._data;
        reload();
        return *this;
    }

    template<typename CharR>
    explicit inline  basic_environment_impl(
                const basic_environment_impl<CharR>& rhs,
                const ::boost::process::codecvt_type & cv = ::boost::process::codecvt())
        : _data(::boost::process::detail::convert(rhs._data, cv))
    {
    }

    template<typename CharR>
    basic_environment_impl & operator=(const basic_environment_impl<CharR>& rhs)
    {
        _data = ::boost::process::detail::convert(rhs._data);
        _env_arr = _load_var(&*_data.begin());
        _env_impl = &*_env_arr.begin();
        return *this;
    }

    Char ** _env_impl = &*_env_arr.begin();

    native_handle_type native_handle() const {return &*_data.begin();}
};


template<typename Char>
basic_environment_impl<Char>::basic_environment_impl(const native_environment_impl<Char> & nei)
{
    auto beg = nei.native_handle();
    auto p   = beg;
    while ((*p != null_char<Char>()) || (*(p+1) !=  null_char<Char>()))
        p++;
    p++; //pointing to the second nullchar
    p++; //to get the pointer behing the second nullchar, so it's end.

    this->_data.assign(beg, p);
    this->reload();
}


template<typename Char>
inline auto basic_environment_impl<Char>::get(const string_type &id) -> string_type
{

    if (std::equal(id.begin(), id.end(), _data.begin()) && (_data[id.size()] == equal_sign<Char>()))
        return string_type(_data.data()); //null-char is handled by the string.

    std::vector<Char> seq = {'\0'}; //using a vector, because strings might cause problems with nullchars
    seq.insert(seq.end(), id.begin(), id.end());
    seq.push_back('=');

    auto itr = std::search(_data.begin(), _data.end(), seq.begin(), seq.end());

    if (itr == _data.end()) //not found
        return "";

    itr += seq.size(); //advance to the value behind the '='; the std::string will take care of finding the null-char.

    return string_type(&*itr);
}

template<typename Char>
inline void basic_environment_impl<Char>::set(const string_type &id, const string_type &value)
{
    reset(id);

    std::vector<Char> insertion;

    insertion.insert(insertion.end(), id.begin(),    id.end());
    insertion.push_back('=');
    insertion.insert(insertion.end(), value.begin(), value.end());
    insertion.push_back('\0');

    _data.insert(_data.end() -1, insertion.begin(), insertion.end());

    reload();
}

template<typename Char>
inline void  basic_environment_impl<Char>::reset(const string_type &id)
{
    //ok, we need to check the size of data first
    if (id.size() >= _data.size()) //ok, so it's impossible id is in there.
        return;

    //check if it's the first one, spares us the search.
    if (std::equal(id.begin(), id.end(), _data.begin()) && (_data[id.size()] == equal_sign<Char>()))
    {
        auto beg = _data.begin();
        auto end = beg;

        while (*end != '\0')
           end++;

        end++; //to point behind the last null-char

        _data.erase(beg, end); //and remove the thingy

    }

    std::vector<Char> seq = {'\0'}; //using a vector, because strings might cause problems with nullchars
    seq.insert(seq.end(), id.begin(), id.end());
    seq.push_back('=');

    auto itr = std::search(_data.begin(), _data.end(), seq.begin(), seq.end());

    if (itr == _data.end())
        return;//nothing to return if it's empty anyway...

    auto end = itr;

    while (*++end != '\0');


    _data.erase(itr, end);//and remove it
    reload();


}

template<typename Char>
std::vector<Char*> basic_environment_impl<Char>::_load_var(Char* p)
{
    std::vector<Char*> ret;
    if (*p != null_char<Char>())
    {
        ret.push_back(p);
        while ((*p != null_char<Char>()) || (*(p+1) !=  null_char<Char>()))
        {
            if (*p==null_char<Char>())
            {
                p++;
                ret.push_back(p);
            }
            else
                p++;
        }
    }
    p++;
    ret.push_back(nullptr);
    return ret;
}


template<typename T> constexpr T env_seperator();
template<> constexpr  char   env_seperator() {return  ';'; }
template<> constexpr wchar_t env_seperator() {return L';'; }

inline int   get_id()         {return boost::winapi::GetCurrentProcessId();}
inline void* native_handle()  {return boost::winapi::GetCurrentProcess(); }

typedef void* native_handle_t;

}

}
}
}




#endif /* BOOST_PROCESS_DETAIL_WINDOWS_ENV_STORAGE_HPP_ */
