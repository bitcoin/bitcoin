///////////////////////////////////////////////////////////////////////////////
/// \file literal.hpp
/// The literal\<\> terminal wrapper, and the proto::lit() function for
/// creating literal\<\> wrappers.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_LITERAL_HPP_EAN_01_03_2007
#define BOOST_PROTO_LITERAL_HPP_EAN_01_03_2007

#include <boost/config.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/expr.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/extends.hpp>

namespace boost { namespace proto
{
    namespace utility
    {
        /// \brief A simple wrapper for a terminal, provided for
        /// ease of use.
        ///
        /// A simple wrapper for a terminal, provided for
        /// ease of use. In all cases, <tt>literal\<X\> l(x);</tt>
        /// is equivalent to <tt>terminal\<X\>::type l = {x};</tt>.
        ///
        /// The \c Domain template parameter defaults to
        /// \c proto::default_domain.
        template<
            typename T
          , typename Domain // = default_domain
        >
        struct literal
          : extends<basic_expr<tag::terminal, term<T>, 0>, literal<T, Domain>, Domain>
        {
        private:
            typedef basic_expr<tag::terminal, term<T>, 0> terminal_type;
            typedef extends<terminal_type, literal<T, Domain>, Domain> base_type;
            typedef literal<T, Domain> literal_t;

        public:
            typedef typename detail::term_traits<T>::value_type       value_type;
            typedef typename detail::term_traits<T>::reference        reference;
            typedef typename detail::term_traits<T>::const_reference  const_reference;

            literal()
              : base_type(terminal_type::make(T()))
            {}

#ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
            literal(literal const &) = default;
#endif

            template<typename U>
            literal(U &u)
              : base_type(terminal_type::make(u))
            {}

            template<typename U>
            literal(U const &u)
              : base_type(terminal_type::make(u))
            {}

            template<typename U>
            literal(literal<U, Domain> const &u)
              : base_type(terminal_type::make(u.get()))
            {}

            BOOST_PROTO_EXTENDS_USING_ASSIGN(literal_t)

            reference get()
            {
                return proto::value(*this);
            }

            const_reference get() const
            {
                return proto::value(*this);
            }
        };
    }

    /// \brief A helper function for creating a \c literal\<\> wrapper.
    /// \param t The object to wrap.
    /// \return literal\<T &\>(t)
    /// \attention The returned value holds the argument by reference.
    /// \throw nothrow
    template<typename T>
    inline literal<T &> const lit(T &t)
    {
        return literal<T &>(t);
    }

    /// \overload
    ///
    template<typename T>
    inline literal<T const &> const lit(T const &t)
    {
        #ifdef BOOST_MSVC
        #pragma warning(push)
        #pragma warning(disable: 4180) // warning C4180: qualifier applied to function type has no meaning; ignored
        #endif

        return literal<T const &>(t);

        #ifdef BOOST_MSVC
        #pragma warning(pop)
        #endif
    }

}}

#endif
