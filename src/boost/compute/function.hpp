//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_FUNCTION_HPP
#define BOOST_COMPUTE_FUNCTION_HPP

#include <map>
#include <string>
#include <sstream>
#include <vector>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/static_assert.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/add_pointer.hpp>
#include <boost/type_traits/function_traits.hpp>

#include <boost/compute/cl.hpp>
#include <boost/compute/config.hpp>
#include <boost/compute/type_traits/type_name.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class ResultType, class ArgTuple>
class invoked_function
{
public:
    typedef ResultType result_type;

    BOOST_STATIC_CONSTANT(
        size_t, arity = boost::tuples::length<ArgTuple>::value
    );

    invoked_function(const std::string &name,
                     const std::string &source)
        : m_name(name),
          m_source(source)
    {
    }

    invoked_function(const std::string &name,
                     const std::string &source,
                     const std::map<std::string, std::string> &definitions)
        : m_name(name),
          m_source(source),
          m_definitions(definitions)
    {
    }

    invoked_function(const std::string &name,
                     const std::string &source,
                     const ArgTuple &args)
        : m_name(name),
          m_source(source),
          m_args(args)
    {
    }

    invoked_function(const std::string &name,
                     const std::string &source,
                     const std::map<std::string, std::string> &definitions,
                     const ArgTuple &args)
        : m_name(name),
          m_source(source),
          m_definitions(definitions),
          m_args(args)
    {
    }

    std::string name() const
    {
        return m_name;
    }

    std::string source() const
    {
        return m_source;
    }

    const std::map<std::string, std::string>& definitions() const
    {
        return m_definitions;
    }

    const ArgTuple& args() const
    {
        return m_args;
    }

private:
    std::string m_name;
    std::string m_source;
    std::map<std::string, std::string> m_definitions;
    ArgTuple m_args;
};

} // end detail namespace

/// \class function
/// \brief A function object.
template<class Signature>
class function
{
public:
    /// \internal_
    typedef typename
        boost::function_traits<Signature>::result_type result_type;

    /// \internal_
    BOOST_STATIC_CONSTANT(
        size_t, arity = boost::function_traits<Signature>::arity
    );

    /// \internal_
    typedef Signature signature;

    /// Creates a new function object with \p name.
    function(const std::string &name)
        : m_name(name)
    {
    }

    /// Destroys the function object.
    ~function()
    {
    }

    /// \internal_
    std::string name() const
    {
        return m_name;
    }

    /// \internal_
    void set_source(const std::string &source)
    {
        m_source = source;
    }

    /// \internal_
    std::string source() const
    {
        return m_source;
    }

    /// \internal_
    void define(std::string name, std::string value = std::string())
    {
        m_definitions[name] = value;
    }

    bool operator==(const function<Signature>& other) const
    {
        return
            (m_name == other.m_name)
                && (m_definitions == other.m_definitions)
                && (m_source == other.m_source);
    }

    bool operator!=(const function<Signature>& other) const
    {
        return !(*this == other);
    }

    /// \internal_
    detail::invoked_function<result_type, boost::tuple<> >
    operator()() const
    {
        BOOST_STATIC_ASSERT_MSG(
            arity == 0,
            "Non-nullary function invoked with zero arguments"
        );

        return detail::invoked_function<result_type, boost::tuple<> >(
            m_name, m_source, m_definitions
        );
    }

    /// \internal_
    template<class Arg1>
    detail::invoked_function<result_type, boost::tuple<Arg1> >
    operator()(const Arg1 &arg1) const
    {
        BOOST_STATIC_ASSERT_MSG(
            arity == 1,
            "Non-unary function invoked one argument"
        );

        return detail::invoked_function<result_type, boost::tuple<Arg1> >(
            m_name, m_source, m_definitions, boost::make_tuple(arg1)
        );
    }

    /// \internal_
    template<class Arg1, class Arg2>
    detail::invoked_function<result_type, boost::tuple<Arg1, Arg2> >
    operator()(const Arg1 &arg1, const Arg2 &arg2) const
    {
        BOOST_STATIC_ASSERT_MSG(
            arity == 2,
            "Non-binary function invoked with two arguments"
        );

        return detail::invoked_function<result_type, boost::tuple<Arg1, Arg2> >(
            m_name, m_source, m_definitions, boost::make_tuple(arg1, arg2)
        );
    }

    /// \internal_
    template<class Arg1, class Arg2, class Arg3>
    detail::invoked_function<result_type, boost::tuple<Arg1, Arg2, Arg3> >
    operator()(const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3) const
    {
        BOOST_STATIC_ASSERT_MSG(
            arity == 3,
            "Non-ternary function invoked with three arguments"
        );

        return detail::invoked_function<result_type, boost::tuple<Arg1, Arg2, Arg3> >(
            m_name, m_source, m_definitions, boost::make_tuple(arg1, arg2, arg3)
        );
    }

private:
    std::string m_name;
    std::string m_source;
    std::map<std::string, std::string> m_definitions;
};

/// Creates a function object given its \p name and \p source.
///
/// \param name The function name.
/// \param source The function source code.
///
/// \see BOOST_COMPUTE_FUNCTION()
template<class Signature>
inline function<Signature>
make_function_from_source(const std::string &name, const std::string &source)
{
    function<Signature> f(name);
    f.set_source(source);
    return f;
}

namespace detail {

// given a string containing the arguments declaration for a function
// like: "(int a, const float b)", returns a vector containing the name
// of each argument (e.g. ["a", "b"]).
inline std::vector<std::string> parse_argument_names(const char *arguments)
{
    BOOST_ASSERT_MSG(
        arguments[0] == '(' && arguments[std::strlen(arguments)-1] == ')',
        "Arguments should start and end with parentheses"
    );

    std::vector<std::string> args;

    size_t last_space = 0;
    size_t skip_comma = 0;
    for(size_t i = 1; i < std::strlen(arguments) - 2; i++){
        const char c = arguments[i];

        if(c == ' '){
            last_space = i;
        }
        else if(c == ',' && !skip_comma){
            std::string name(
                arguments + last_space + 1, i - last_space - 1
            );
            args.push_back(name);
        }
        else if(c == '<'){
            skip_comma++;
        }
        else if(c == '>'){
            skip_comma--;
        }
    }

    std::string last_argument(
        arguments + last_space + 1, std::strlen(arguments) - last_space - 2
    );
    args.push_back(last_argument);

    return args;
}

struct signature_argument_inserter
{
    signature_argument_inserter(std::stringstream &s_, const char *arguments, size_t last)
        : s(s_)
    {
        n = 0;
        m_last = last;

        m_argument_names = parse_argument_names(arguments);

        BOOST_ASSERT_MSG(
            m_argument_names.size() == last,
            "Wrong number of arguments"
        );
    }

    template<class T>
    void operator()(const T*)
    {
        s << type_name<T>() << " " << m_argument_names[n];
        if(n+1 < m_last){
            s << ", ";
        }
        n++;
    }

    size_t n;
    size_t m_last;
    std::stringstream &s;
    std::vector<std::string> m_argument_names;
};

template<class Signature>
inline std::string make_function_declaration(const char *name, const char *arguments)
{
    typedef typename
        boost::function_traits<Signature>::result_type result_type;
    typedef typename
        boost::function_types::parameter_types<Signature>::type parameter_types;
    typedef typename
        mpl::size<parameter_types>::type arity_type;

    std::stringstream s;
    s << "inline " << type_name<result_type>() << " " << name;
    s << "(";

    if(arity_type::value > 0){
        signature_argument_inserter i(s, arguments, arity_type::value);
        mpl::for_each<
            typename mpl::transform<parameter_types, boost::add_pointer<mpl::_1>
        >::type>(i);
    }

    s << ")";
    return s.str();
}

struct argument_list_inserter
{
    argument_list_inserter(std::stringstream &s_, const char first, size_t last)
        : s(s_)
    {
        n = 0;
        m_last = last;
        m_name = first;
    }

    template<class T>
    void operator()(const T*)
    {
        s << type_name<T>() << " " << m_name++;
        if(n+1 < m_last){
            s << ", ";
        }
        n++;
    }

    size_t n;
    size_t m_last;
    char m_name;
    std::stringstream &s;
};

template<class Signature>
inline std::string generate_argument_list(const char first = 'a')
{
    typedef typename
        boost::function_types::parameter_types<Signature>::type parameter_types;
    typedef typename
        mpl::size<parameter_types>::type arity_type;

    std::stringstream s;
    s << '(';

    if(arity_type::value > 0){
        argument_list_inserter i(s, first, arity_type::value);
        mpl::for_each<
            typename mpl::transform<parameter_types, boost::add_pointer<mpl::_1>
        >::type>(i);
    }

    s << ')';
    return s.str();
}

// used by the BOOST_COMPUTE_FUNCTION() macro to create a function
// with the given signature, name, arguments, and source.
template<class Signature>
inline function<Signature>
make_function_impl(const char *name, const char *arguments, const char *source)
{
    std::stringstream s;
    s << make_function_declaration<Signature>(name, arguments);
    s << source;

    return make_function_from_source<Signature>(name, s.str());
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

/// Creates a function object with \p name and \p source.
///
/// \param return_type The return type for the function.
/// \param name The name of the function.
/// \param arguments A list of arguments for the function.
/// \param source The OpenCL C source code for the function.
///
/// The function declaration and signature are automatically created using
/// the \p return_type, \p name, and \p arguments macro parameters.
///
/// The source code for the function is interpreted as OpenCL C99 source code
/// which is stringified and passed to the OpenCL compiler when the function
/// is invoked.
///
/// For example, to create a function which squares a number:
/// \code
/// BOOST_COMPUTE_FUNCTION(float, square, (float x),
/// {
///     return x * x;
/// });
/// \endcode
///
/// And to create a function which sums two numbers:
/// \code
/// BOOST_COMPUTE_FUNCTION(int, sum_two, (int x, int y),
/// {
///     return x + y;
/// });
/// \endcode
///
/// \see BOOST_COMPUTE_CLOSURE()
#ifdef BOOST_COMPUTE_DOXYGEN_INVOKED
#define BOOST_COMPUTE_FUNCTION(return_type, name, arguments, source)
#else
#define BOOST_COMPUTE_FUNCTION(return_type, name, arguments, ...) \
    ::boost::compute::function<return_type arguments> name = \
        ::boost::compute::detail::make_function_impl<return_type arguments>( \
            #name, #arguments, #__VA_ARGS__ \
        )
#endif

#endif // BOOST_COMPUTE_FUNCTION_HPP
