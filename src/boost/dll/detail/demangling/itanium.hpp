//  Copyright 2016 Klemens Morgenstern
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DLL_DETAIL_DEMANGLING_ITANIUM_HPP_
#define BOOST_DLL_DETAIL_DEMANGLING_ITANIUM_HPP_

#include <boost/dll/detail/demangling/mangled_storage_base.hpp>
#include <iterator>
#include <algorithm>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_volatile.hpp>
#include <boost/type_traits/is_rvalue_reference.hpp>
#include <boost/type_traits/is_lvalue_reference.hpp>
#include <boost/type_traits/function_traits.hpp>


namespace boost { namespace dll { namespace detail {



class mangled_storage_impl : public mangled_storage_base
{
    template<typename T>
    struct dummy {};

    template<typename Return, typename ...Args>
    std::vector<std::string> get_func_params(dummy<Return(Args...)>)  const
    {
        return {get_name<Args>()...};
    }
    template<typename Return, typename ...Args>
    std::string get_return_type(dummy<Return(Args...)>)  const
    {
        return get_name<Return>();
    }
public:
    using mangled_storage_base::mangled_storage_base;
    struct ctor_sym
    {
        std::string C1;
        std::string C2;
        std::string C3;

        bool empty() const
        {
            return C1.empty() && C2.empty() && C3.empty();
        }
    };

    struct dtor_sym
    {
        std::string D0;
        std::string D1;
        std::string D2;
        bool empty() const
        {
            return D0.empty() && D1.empty() && D2.empty();
        }
    };

    template<typename T>
    std::string get_variable(const std::string &name) const;

    template<typename Func>
    std::string get_function(const std::string &name) const;

    template<typename Class, typename Func>
    std::string get_mem_fn(const std::string &name) const;

    template<typename Signature>
    ctor_sym get_constructor() const;

    template<typename Class>
    dtor_sym get_destructor() const;

    template<typename T>
    std::string get_type_info() const;

    template<typename T>
    std::vector<std::string> get_related() const;

};



namespace parser
{
    //! declare
    template <typename... T>
    struct dummy;

    template <typename T>
    std::string parse_type_helper(const mangled_storage_impl & ms, dummy<T>*);

    template <typename... T, template <typename...> class Tn>
    std::string parse_type_helper(const mangled_storage_impl & ms, dummy<Tn<T...>>*);

    template <typename... T, template <typename...> class Tn>
    std::string parse_type(const mangled_storage_impl & ms, dummy<Tn<T...>>*);

    template <typename T>
    std::string parse_type(const mangled_storage_impl & ms, dummy<T>*);

    template <typename T1, typename T2, typename... T3>
    std::string parse_type(const mangled_storage_impl & ms, dummy<T1, T2, T3...>*);

    template <typename R, typename... Args>
    std::string parse_type(const mangled_storage_impl & ms, dummy<R(Args...)>*);

    std::string parse_type(const mangled_storage_impl & ms, dummy<>*);

    template<typename T>
    std::string
    type_name(const mangled_storage_impl &);


    //The purpose of this class template is to separate the pure type from the rule name from the target type
    template<typename T>
    struct pure_type
    {
        typedef T type;
        inline static std::string type_rule() { return ""; }
    };

    template<typename T>
    struct pure_type<T*>
    {
        typedef typename pure_type<T>::type type;
        inline static std::string type_rule()
        {
            return pure_type<T>::type_rule() + "*";
        }
    };

    template<typename T>
    struct pure_type<T const>
    {
        typedef typename pure_type<T>::type type;
        inline static std::string type_rule()
        {
            return pure_type<T>::type_rule() + " const";
        }
    };

    template<typename T>
    struct pure_type<T volatile>
    {
        typedef typename pure_type<T>::type type;
        inline static std::string type_rule()
        {
            return pure_type<T>::type_rule() + " volatile";
        }
    };

    template<typename T>
    struct pure_type<T const volatile>
    {
        typedef typename pure_type<T>::type type;
        inline static std::string type_rule()
        {
            return pure_type<T>::type_rule() + " const volatile";
        }
    };

    template<typename T>
    struct pure_type<T&>
    {
        typedef typename pure_type<T>::type type;
        inline static std::string type_rule()
        {
            return pure_type<T>::type_rule() + "&";
        }
    };

    template<typename T>
    struct pure_type<T&&>
    {
        typedef typename pure_type<T>::type type;
        inline static std::string type_rule()
        {
            return pure_type<T>::type_rule() + "&&";
        }
    };

    inline std::string const_rule_impl(true_type )  {return " const";}
    inline std::string const_rule_impl(false_type)  {return "";}
    template<typename T>
    std::string const_rule() {using t = is_const<typename remove_reference<T>::type>; return const_rule_impl(t());}

    inline std::string volatile_rule_impl(true_type )  {return " volatile";}
    inline std::string volatile_rule_impl(false_type)  {return "";}
    template<typename T>
    std::string volatile_rule() {using t = is_volatile<typename remove_reference<T>::type>; return volatile_rule_impl(t());}

    inline std::string reference_rule_impl(false_type, false_type) {return "";}
    inline std::string reference_rule_impl(true_type,  false_type) {return "&" ;}
    inline std::string reference_rule_impl(false_type, true_type ) {return "&&";}

    template<typename T>
    std::string reference_rule() {using t_l = is_lvalue_reference<T>; using t_r = is_rvalue_reference<T>; return reference_rule_impl(t_l(), t_r());}

    //it takes a string, because it may be overloaded.
    template<typename Return, typename Arg>
    std::string arg_list(const mangled_storage_impl & ms, Return (*)(Arg))
    {
        using namespace std;
        return type_name<Arg>(ms);
    }

    template<typename Return, typename First, typename Second, typename ...Args>
    std::string arg_list(const mangled_storage_impl & ms, Return (*)(First, Second, Args...))
    {
        using next_type = Return (*)(Second, Args...);
        return type_name<First>(ms) + ", " + arg_list(ms, next_type());
    }

    template<typename Return>
    std::string arg_list(const mangled_storage_impl &, Return (*)())
    {
        return "";
    }

    //! implement
    template <typename T>
    inline std::string parse_type_helper(const mangled_storage_impl & ms, dummy<T>*) {
        return  ms.get_name<T>();
    }

    template <typename... T, template <typename...> class Tn>
    inline std::string parse_type_helper(const mangled_storage_impl & ms, dummy<Tn<T...>>*) {
        using type = dummy<Tn<T...>>*;
        return parse_type(ms, type());
    }

    template <typename R, typename... Args>
    inline std::string parse_type(const mangled_storage_impl & ms, dummy<R(*)(Args...)>*) {
        using args_type = dummy<Args...>*;
        using return_type = dummy<R>*;
        return parse_type(ms, return_type()) + " (*)(" + parse_type(ms, args_type()) + ")";
    }

    template <typename R, typename... Args>
    inline std::string parse_type(const mangled_storage_impl & ms, dummy<R(Args...)>*) {
        using args_type = dummy<Args...>*;
        using return_type = dummy<R>*;
        return parse_type(ms, return_type()) + " (" + parse_type(ms, args_type()) + ")";
    }

    template <typename T>
    inline std::string parse_type(const mangled_storage_impl & ms, dummy<T>*) {
        using type = dummy<typename pure_type<T>::type>*;
        auto str = parse_type_helper(ms, type());
        return str + pure_type<T>::type_rule();
    }

    template <typename T1, typename T2, typename... T3>
    inline std::string parse_type(const mangled_storage_impl & ms, dummy<T1, T2, T3...>*) {
        using first_type = dummy<T1>*;
        using next_type = dummy<T2, T3...>*;
        return parse_type(ms, first_type()) + ", " + parse_type(ms, next_type());
    }

    template <typename... T, template <typename...> class Tn>
    inline std::string parse_type(const mangled_storage_impl & ms, dummy<Tn<T...>>*) {
        using next_type = dummy<T...>*;
        std::string str = ms.get_name<Tn<T...>>();
        auto frist = str.find_first_of("<");
        std::string template_name = str.substr(0, frist);
        std::string args_name = parse_type(ms, next_type());
        char last_ch = args_name[args_name.size() - 1];
        return template_name + "<" + args_name + (last_ch == '>' ? " >" : ">");
    }

    inline std::string parse_type(const mangled_storage_impl &, dummy<>*) {
        return "";
    }

    template<typename T>
    inline  std::string
    type_name(const mangled_storage_impl &ms)
    {
        using namespace parser;
        using type = dummy<T>*;
        return  parse_type(ms, type());
    }
}



template<typename T> std::string mangled_storage_impl::get_variable(const std::string &name) const
{
    auto found = std::find_if(storage_.begin(), storage_.end(),
            [&](const entry& e) {return e.demangled == name;});

    if (found != storage_.end())
        return found->mangled;
    else
        return "";
}

template<typename Func> std::string mangled_storage_impl::get_function(const std::string &name) const
{
    using func_type = Func*;

    auto matcher = name + '(' + parser::arg_list(*this, func_type()) + ')';

    auto found = std::find_if(storage_.begin(), storage_.end(), [&](const entry& e) {return e.demangled == matcher;});
    if (found != storage_.end())
        return found->mangled;
    else
        return "";

}

template<typename Class, typename Func>
std::string mangled_storage_impl::get_mem_fn(const std::string &name) const
{
    using namespace parser;

    using func_type = Func*;

    std::string cname = get_name<Class>();

    const auto matcher = cname + "::" + name +
             '(' + parser::arg_list(*this, func_type()) + ')'
             + const_rule<Class>() + volatile_rule<Class>();

    // Linux export table contains int MyClass::Func<float>(), but expected in import_mangled MyClass::Func<float>() without returned type.
    auto found = std::find_if(storage_.begin(), storage_.end(), [&matcher](const entry& e) {
        if (e.demangled == matcher) {
          return true;
        }

        const auto pos = e.demangled.rfind(matcher);
        if (pos == std::string::npos) {
          // Not found.
          return false;
        }

        if (pos + matcher.size() != e.demangled.size()) {
          // There are some characters after the `matcher` string.
          return false;
        }

        // Double checking that we matched a full function name
        return e.demangled[pos - 1] == ' '; // `if (e.demangled == matcher)` makes sure that `pos > 0`
    });

    if (found != storage_.end())
        return found->mangled;
    else
        return "";

}


template<typename Signature>
auto mangled_storage_impl::get_constructor() const -> ctor_sym
{
    using namespace parser;

    using func_type = Signature*;

    std::string ctor_name; // = class_name + "::" + name;
    std::string unscoped_cname; //the unscoped class-name
    {
        auto class_name = get_return_type(dummy<Signature>());
        auto pos = class_name.rfind("::");
        if (pos == std::string::npos)
        {
            ctor_name = class_name+ "::" +class_name ;
            unscoped_cname = class_name;
        }
        else
        {
            unscoped_cname = class_name.substr(pos+2) ;
            ctor_name = class_name+ "::" + unscoped_cname;
        }
    }

    auto matcher =
                ctor_name + '(' + parser::arg_list(*this, func_type()) + ')';


    std::vector<entry> findings;
    std::copy_if(storage_.begin(), storage_.end(),
            std::back_inserter(findings), [&](const entry& e) {return e.demangled == matcher;});

    ctor_sym ct;

    for (auto & e : findings)
    {

        if (e.mangled.find(unscoped_cname +"C1E") != std::string::npos)
            ct.C1 = e.mangled;
        else if (e.mangled.find(unscoped_cname +"C2E") != std::string::npos)
            ct.C2 = e.mangled;
        else if (e.mangled.find(unscoped_cname +"C3E") != std::string::npos)
            ct.C3 = e.mangled;
    }
    return ct;
}

template<typename Class>
auto mangled_storage_impl::get_destructor() const -> dtor_sym
{
    std::string dtor_name; // = class_name + "::" + name;
    std::string unscoped_cname; //the unscoped class-name
    {
        auto class_name = get_name<Class>();
        auto pos = class_name.rfind("::");
        if (pos == std::string::npos)
        {
            dtor_name = class_name+ "::~" + class_name  + "()";
            unscoped_cname = class_name;
        }
        else
        {
            unscoped_cname = class_name.substr(pos+2) ;
            dtor_name = class_name+ "::~" + unscoped_cname + "()";
        }
    }

    auto d0 = unscoped_cname + "D0Ev";
    auto d1 = unscoped_cname + "D1Ev";
    auto d2 = unscoped_cname + "D2Ev";

    dtor_sym dt;
    //this is so simple, i don#t need a predicate
    for (auto & s : storage_)
    {
        //alright, name fits
        if (s.demangled == dtor_name)
        {
            if (s.mangled.find(d0) != std::string::npos)
                dt.D0 = s.mangled;
            else if (s.mangled.find(d1) != std::string::npos)
                dt.D1 = s.mangled;
            else if (s.mangled.find(d2) != std::string::npos)
                dt.D2 = s.mangled;

        }
    }
    return dt;

}

template<typename T>
std::string mangled_storage_impl::get_type_info() const
{
    std::string id = "typeinfo for " + get_name<T>();


    auto predicate = [&](const mangled_storage_base::entry & e)
                {
                    return e.demangled == id;
                };

    auto found = std::find_if(storage_.begin(), storage_.end(), predicate);


    if (found != storage_.end())
        return found->mangled;
    else
        return "";
}

template<typename T>
std::vector<std::string> mangled_storage_impl::get_related()  const
{
    std::vector<std::string> ret;
    auto name = get_name<T>();

    for (auto & c : storage_)
    {
        if (c.demangled.find(name) != std::string::npos)
            ret.push_back(c.demangled);
    }

    return ret;
}

}}}


#endif /* BOOST_DLL_DETAIL_DEMANGLING_ITANIUM_HPP_ */
