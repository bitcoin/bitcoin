/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2011 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_UTREE_OPERATORS)
#define BOOST_SPIRIT_UTREE_OPERATORS

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4804)
# pragma warning(disable: 4805)
#endif

#include <exception>
#if !defined(BOOST_SPIRIT_DISABLE_UTREE_IO)
  #include <ios>
  #include <boost/io/ios_state.hpp>
#endif
#include <boost/spirit/home/support/utree/utree.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_integral.hpp>

namespace boost { namespace spirit 
{
    // Relational operators
    bool operator==(utree const& a, utree const& b);
    bool operator<(utree const& a, utree const& b);
    bool operator!=(utree const& a, utree const& b);
    bool operator>(utree const& a, utree const& b);
    bool operator<=(utree const& a, utree const& b);
    bool operator>=(utree const& a, utree const& b);

#if !defined(BOOST_SPIRIT_DISABLE_UTREE_IO)
    // output
    std::ostream& operator<<(std::ostream& out, utree const& x);
    std::ostream& operator<<(std::ostream& out, utree::invalid_type const& x);
    std::ostream& operator<<(std::ostream& out, utree::nil_type const& x);
#endif

    // Logical operators
    utree operator&&(utree const& a, utree const& b);
    utree operator||(utree const& a, utree const& b);
    utree operator!(utree const& a);

    // Arithmetic operators
    utree operator+(utree const& a, utree const& b);
    utree operator-(utree const& a, utree const& b);
    utree operator*(utree const& a, utree const& b);
    utree operator/(utree const& a, utree const& b);
    utree operator%(utree const& a, utree const& b);
    utree operator-(utree const& a);

    // Bitwise operators
    utree operator&(utree const& a, utree const& b);
    utree operator|(utree const& a, utree const& b);
    utree operator^(utree const& a, utree const& b);
    utree operator<<(utree const& a, utree const& b);
    utree operator>>(utree const& a, utree const& b);
    utree operator~(utree const& a);

    // Implementation
    struct utree_is_equal
    {
        typedef bool result_type;

        template <typename A, typename B>
        bool dispatch(const A&, const B&, boost::mpl::false_) const
        {
            return false; // cannot compare different types by default
        }

        template <typename A, typename B>
        bool dispatch(const A& a, const B& b, boost::mpl::true_) const
        {
            return a == b; // for arithmetic types
        }

        template <typename A, typename B>
        bool operator()(const A& a, const B& b) const
        {
            return dispatch(a, b,
                boost::mpl::and_<
                    boost::is_arithmetic<A>,
                    boost::is_arithmetic<B> >());
        }

        template <typename T>
        bool operator()(const T& a, const T& b) const
        {
            // This code works for lists
            return a == b;
        }

        template <typename Base, utree_type::info type_>
        bool operator()(
            basic_string<Base, type_> const& a,
            basic_string<Base, type_> const& b) const
        {
            return static_cast<Base const&>(a) == static_cast<Base const&>(b);
        }

        bool operator()(utree::invalid_type, utree::invalid_type) const
        {
            return true;
        }

        bool operator()(utree::nil_type, utree::nil_type) const
        {
            return true;
        }

        bool operator()(function_base const&, function_base const&) const
        {
            return false; // just don't allow comparison of functions
        }
    };

    struct utree_is_less_than
    {
        typedef bool result_type;

        template <typename A, typename B>
        bool dispatch(const A&, const B&, boost::mpl::false_) const
        {
            return false; // cannot compare different types by default
        }

        template <typename A, typename B>
        bool dispatch(const A& a, const B& b, boost::mpl::true_) const
        {
            return a < b; // for arithmetic types
        }

        template <typename A, typename B>
        bool operator()(const A& a, const B& b) const
        {
            return dispatch(a, b,
                boost::mpl::and_<
                    boost::is_arithmetic<A>,
                    boost::is_arithmetic<B> >());
        }

        template <typename T>
        bool operator()(const T& a, const T& b) const
        {
            // This code works for lists
            return a < b;
        }

        template <typename Base, utree_type::info type_>
        bool operator()(
            basic_string<Base, type_> const& a,
            basic_string<Base, type_> const& b) const
        {
            return static_cast<Base const&>(a) < static_cast<Base const&>(b);
        }

        bool operator()(utree::invalid_type, utree::invalid_type) const
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("no less-than comparison for this utree type",
               utree_type::invalid_type));
            BOOST_UNREACHABLE_RETURN(false) // no less than comparison for nil
        }

        bool operator()(utree::nil_type, utree::nil_type) const
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("no less-than comparison for this utree type",
               utree_type::nil_type));
            BOOST_UNREACHABLE_RETURN(false) // no less than comparison for nil
        }

        bool operator()(any_ptr const&, any_ptr const&) const
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("no less-than comparison for this utree type",
               utree_type::any_type));
            BOOST_UNREACHABLE_RETURN(false) // no less than comparison for any_ptr
        }

        bool operator()(function_base const&, function_base const&) const
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("no less-than comparison for this utree type",
               utree_type::function_type));
            BOOST_UNREACHABLE_RETURN(false) // no less than comparison of functions
        }
    };

#if !defined(BOOST_SPIRIT_DISABLE_UTREE_IO)
    struct utree_print
    {
        typedef void result_type;

        std::ostream& out;
        utree_print(std::ostream& out) : out(out) {}

        void operator()(utree::invalid_type) const
        {
            out << "<invalid> ";
        }

        void operator()(utree::nil_type) const
        {
            out << "<nil> ";
        }

        template <typename T>
        void operator()(T val) const
        {
            out << val << ' ';
        }

        void operator()(bool b) const
        {
            out << (b ? "true" : "false") << ' ';
        }

        void operator()(binary_range_type const& b) const
        {
            boost::io::ios_all_saver saver(out);
            out << "#";
            out.width(2);
            out.fill('0');

            typedef binary_range_type::const_iterator iterator;
            for (iterator i = b.begin(); i != b.end(); ++i)
                out << std::hex << int((unsigned char)*i);
            out << "# ";
        }

        void operator()(utf8_string_range_type const& str) const
        {
            typedef utf8_string_range_type::const_iterator iterator;
            iterator i = str.begin();
            out << '"';
            for (; i != str.end(); ++i)
                out << *i;
            out << "\" ";
        }

        void operator()(utf8_symbol_range_type const& str) const
        {
            typedef utf8_symbol_range_type::const_iterator iterator;
            iterator i = str.begin();
            for (; i != str.end(); ++i)
                out << *i;
            out << ' ';
        }

        template <typename Iterator>
        void operator()(boost::iterator_range<Iterator> const& range) const
        {
            typedef typename boost::iterator_range<Iterator>::const_iterator iterator;
            (*this)('(');
            for (iterator i = range.begin(); i != range.end(); ++i)
            {
                boost::spirit::utree::visit(*i, *this);
            }
            (*this)(')');
        }

        void operator()(any_ptr const&) const
        {
            return (*this)("<pointer>");
        }

        void operator()(function_base const&) const
        {
            return (*this)("<function>");
        }
    };
#endif

    template <typename Base>
    struct logical_function
    {
        typedef utree result_type;

        // We assume anything except false is true

        // binary
        template <typename A, typename B>
        utree operator()(A const& a, B const& b) const
        {
            return dispatch(a, b
              , boost::is_arithmetic<A>()
              , boost::is_arithmetic<B>());
        }

        // binary
        template <typename A, typename B>
        utree dispatch(A const& a, B const& b, mpl::true_, mpl::true_) const
        {
            return Base::eval(a, b); // for arithmetic types
        }

        // binary
        template <typename A, typename B>
        utree dispatch(A const&, B const& b, mpl::false_, mpl::true_) const
        {
            return Base::eval(true, b);
        }

        // binary
        template <typename A, typename B>
        utree dispatch(A const& a, B const&, mpl::true_, mpl::false_) const
        {
            return Base::eval(a, true);
        }

        // binary
        template <typename A, typename B>
        utree dispatch(A const&, B const&, mpl::false_, mpl::false_) const
        {
            return Base::eval(true, true);
        }
        
        // unary
        template <typename A>
        utree operator()(A const& a) const
        {
            return dispatch(a, boost::is_arithmetic<A>());
        }

        // unary
        template <typename A>
        utree dispatch(A const& a, mpl::true_) const
        {
            return Base::eval(a);
        }

        // unary
        template <typename A>
        utree dispatch(A const&, mpl::false_) const
        {
            return Base::eval(true);
        }
    };

    template <typename Base>
    struct arithmetic_function
    {
        typedef utree result_type;

        template <typename A, typename B>
        utree dispatch(A const&, B const&, boost::mpl::false_) const
        {
            return utree(); // cannot apply to non-arithmetic types
        }

        template <typename A, typename B>
        utree dispatch(A const& a, B const& b, boost::mpl::true_) const
        {
            return Base::eval(a, b); // for arithmetic types
        }

        // binary
        template <typename A, typename B>
        utree operator()(A const& a, B const& b) const
        {
            return dispatch(a, b,
                boost::mpl::and_<
                    boost::is_arithmetic<A>,
                    boost::is_arithmetic<B> >());
        }

        template <typename A>
        utree dispatch(A const&, boost::mpl::false_) const
        {
            return utree(); // cannot apply to non-arithmetic types
        }

        template <typename A>
        utree dispatch(A const& a, boost::mpl::true_) const
        {
            return Base::eval(a); // for arithmetic types
        }

        // unary
        template <typename A>
        utree operator()(A const& a) const
        {
            return dispatch(a, boost::is_arithmetic<A>());
        }
    };

    template <typename Base>
    struct integral_function
    {
        typedef utree result_type;

        template <typename A, typename B>
        utree dispatch(A const&, B const&, boost::mpl::false_) const
        {
            return utree(); // cannot apply to non-integral types
        }

        template <typename A, typename B>
        utree dispatch(A const& a, B const& b, boost::mpl::true_) const
        {
            return Base::eval(a, b); // for integral types
        }

        // binary
        template <typename A, typename B>
        utree operator()(A const& a, B const& b) const
        {
            return dispatch(a, b,
                boost::mpl::and_<
                    boost::is_integral<A>,
                    boost::is_integral<B> >());
        }

        template <typename A>
        utree dispatch(A const&, boost::mpl::false_) const
        {
            return utree(); // cannot apply to non-integral types
        }

        template <typename A>
        utree dispatch(A const& a, boost::mpl::true_) const
        {
            return Base::eval(a); // for integral types
        }

        // unary
        template <typename A>
        utree operator()(A const& a) const
        {
            return dispatch(a, boost::is_integral<A>());
        }
    };

#define BOOST_SPIRIT_UTREE_CREATE_FUNCTION_BINARY                             \
        template <typename Lhs, typename Rhs>                                 \
        static utree eval(Lhs const& a, Rhs const& b)                         \
        /***/

#define BOOST_SPIRIT_UTREE_CREATE_FUNCTION_UNARY                              \
        template <typename Operand>                                           \
        static utree eval(Operand const& a)                                   \
        /***/

#define BOOST_SPIRIT_UTREE_CREATE_FUNCTION(arity, name, expr, base)           \
    struct BOOST_PP_CAT(function_impl_, name)                                 \
    {                                                                         \
        BOOST_SPIRIT_UTREE_CREATE_FUNCTION_##arity                            \
        {                                                                     \
            return utree(expr);                                               \
        }                                                                     \
    };                                                                        \
    base<BOOST_PP_CAT(function_impl_, name)> const                            \
        BOOST_PP_CAT(base, BOOST_PP_CAT(_, name)) = {};                       \
    /***/

#define BOOST_SPIRIT_UTREE_CREATE_ARITHMETIC_FUNCTION(arity, name, expr)      \
    BOOST_SPIRIT_UTREE_CREATE_FUNCTION(arity, name, expr, arithmetic_function)\
    /***/

#define BOOST_SPIRIT_UTREE_CREATE_INTEGRAL_FUNCTION(arity, name, expr)        \
    BOOST_SPIRIT_UTREE_CREATE_FUNCTION(arity, name, expr, integral_function)  \
    /***/

#define BOOST_SPIRIT_UTREE_CREATE_LOGICAL_FUNCTION(arity, name, expr)         \
    BOOST_SPIRIT_UTREE_CREATE_FUNCTION(arity, name, expr, logical_function)   \
    /***/

    inline bool operator==(utree const& a, utree const& b)
    {
        return utree::visit(a, b, utree_is_equal());
    }

    inline bool operator<(utree const& a, utree const& b)
    {
        return utree::visit(a, b, utree_is_less_than());
    }

    inline bool operator!=(utree const& a, utree const& b)
    {
        return !(a == b);
    }

    inline bool operator>(utree const& a, utree const& b)
    {
        return b < a;
    }

    inline bool operator<=(utree const& a, utree const& b)
    {
        return !(b < a);
    }

    inline bool operator>=(utree const& a, utree const& b)
    {
        return !(a < b);
    }

#if !defined(BOOST_SPIRIT_DISABLE_UTREE_IO)
    inline std::ostream& operator<<(std::ostream& out, utree const& x)
    {
        utree::visit(x, utree_print(out));
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, utree::invalid_type const&)
    {
        return out;
    }

    inline std::ostream& operator<<(std::ostream& out, utree::nil_type const&)
    {
        return out;
    }
#endif

    BOOST_SPIRIT_UTREE_CREATE_LOGICAL_FUNCTION(BINARY, and_, a&&b)
    BOOST_SPIRIT_UTREE_CREATE_LOGICAL_FUNCTION(BINARY, or_,  a||b)
    BOOST_SPIRIT_UTREE_CREATE_LOGICAL_FUNCTION(UNARY,  not_, !a)

    BOOST_SPIRIT_UTREE_CREATE_ARITHMETIC_FUNCTION(BINARY, plus,    a+b)
    BOOST_SPIRIT_UTREE_CREATE_ARITHMETIC_FUNCTION(BINARY, minus,   a-b)
    BOOST_SPIRIT_UTREE_CREATE_ARITHMETIC_FUNCTION(BINARY, times,   a*b)
    BOOST_SPIRIT_UTREE_CREATE_ARITHMETIC_FUNCTION(BINARY, divides, a/b)
    BOOST_SPIRIT_UTREE_CREATE_INTEGRAL_FUNCTION  (BINARY, modulus, a%b)
    BOOST_SPIRIT_UTREE_CREATE_ARITHMETIC_FUNCTION(UNARY,  negate,  -a)

    BOOST_SPIRIT_UTREE_CREATE_INTEGRAL_FUNCTION(BINARY, bitand_,     a&b)
    BOOST_SPIRIT_UTREE_CREATE_INTEGRAL_FUNCTION(BINARY, bitor_,      a|b)
    BOOST_SPIRIT_UTREE_CREATE_INTEGRAL_FUNCTION(BINARY, bitxor_,     a^b)
    BOOST_SPIRIT_UTREE_CREATE_INTEGRAL_FUNCTION(BINARY, shift_left,  a<<b)
    BOOST_SPIRIT_UTREE_CREATE_INTEGRAL_FUNCTION(BINARY, shift_right, a>>b)
    BOOST_SPIRIT_UTREE_CREATE_INTEGRAL_FUNCTION(UNARY,  invert,      ~a)

    // avoid `'~' on an expression of type bool` warning
    template <> inline utree function_impl_invert::eval<bool>(bool const& a)
    {
        return utree(!a);
    }

#undef BOOST_SPIRIT_UTREE_CREATE_FUNCTION_BINARY
#undef BOOST_SPIRIT_UTREE_CREATE_FUNCTION_UNARY
#undef BOOST_SPIRIT_UTREE_CREATE_FUNCTION
#undef BOOST_SPIRIT_UTREE_CREATE_LOGICAL_FUNCTION
#undef BOOST_SPIRIT_UTREE_CREATE_INTEGRAL_FUNCTION
#undef BOOST_SPIRIT_UTREE_CREATE_ARITHMETIC_FUNCTION

    inline utree operator&&(utree const& a, utree const& b)
    {
          return utree::visit(a, b, logical_function_and_);
    }

    inline utree operator||(utree const& a, utree const& b)
    {
        return utree::visit(a, b, logical_function_or_);
    }

    inline utree operator!(utree const& a)
    {
        return utree::visit(a, logical_function_not_);
    }

    inline utree operator+(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, arithmetic_function_plus);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("addition performed on non-arithmetic utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator-(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, arithmetic_function_minus);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("subtraction performed on non-arithmetic utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator*(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, arithmetic_function_times);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("multiplication performed on non-arithmetic utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator/(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, arithmetic_function_divides);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("division performed on non-arithmetic utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator%(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, integral_function_modulus);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("modulos performed on non-integral utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator-(utree const& a)
    {
        utree r = utree::visit(a, arithmetic_function_negate);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("negation performed on non-arithmetic utree type",
               a.which()));
        } 
        return r;
    }

    inline utree operator&(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, integral_function_bitand_);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("bitwise and performed on non-integral utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator|(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, integral_function_bitor_);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("bitwise or performed on non-integral utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator^(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, integral_function_bitxor_);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("bitwise xor performed on non-integral utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator<<(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, integral_function_shift_left);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("left shift performed on non-integral utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator>>(utree const& a, utree const& b)
    {
        utree r = utree::visit(a, b, integral_function_shift_right);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("right shift performed on non-integral utree types",
               a.which(), b.which()));
        } 
        return r;
    }

    inline utree operator~(utree const& a)
    {
        utree r = utree::visit(a, integral_function_invert);
        if (r.which() == utree_type::invalid_type)
        {
            BOOST_THROW_EXCEPTION(bad_type_exception
              ("inversion performed on non-integral utree type",
               a.which()));
        } 
        return r;
    }
}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif
