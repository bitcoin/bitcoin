///////////////////////////////////////////////////////////////////////////////
/// \file regex_actions.hpp
/// Defines the syntax elements of xpressive's action expressions.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_ACTIONS_HPP_EAN_03_22_2007
#define BOOST_XPRESSIVE_ACTIONS_HPP_EAN_03_22_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/ref.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/xpressive/detail/core/matcher/attr_matcher.hpp>
#include <boost/xpressive/detail/core/matcher/attr_end_matcher.hpp>
#include <boost/xpressive/detail/core/matcher/attr_begin_matcher.hpp>
#include <boost/xpressive/detail/core/matcher/predicate_matcher.hpp>
#include <boost/xpressive/detail/utility/ignore_unused.hpp>
#include <boost/xpressive/detail/static/type_traits.hpp>

// These are very often needed by client code.
#include <boost/typeof/std/map.hpp>
#include <boost/typeof/std/string.hpp>

// Doxygen can't handle proto :-(
#ifndef BOOST_XPRESSIVE_DOXYGEN_INVOKED
# include <boost/proto/core.hpp>
# include <boost/proto/transform.hpp>
# include <boost/xpressive/detail/core/matcher/action_matcher.hpp>
#endif

#if BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4510) // default constructor could not be generated
#pragma warning(disable : 4512) // assignment operator could not be generated
#pragma warning(disable : 4610) // can never be instantiated - user defined constructor required
#endif

namespace boost { namespace xpressive
{

    namespace detail
    {
        template<typename T, typename U>
        struct action_arg
        {
            typedef T type;
            typedef typename add_reference<T>::type reference;

            reference cast(void *pv) const
            {
                return *static_cast<typename remove_reference<T>::type *>(pv);
            }
        };

        template<typename T>
        struct value_wrapper
          : private noncopyable
        {
            value_wrapper()
              : value()
            {}

            value_wrapper(T const &t)
              : value(t)
            {}

            T value;
        };

        struct check_tag
        {};

        struct BindArg
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename MatchResults, typename Expr>
            struct result<This(MatchResults, Expr)>
            {
                typedef Expr type;
            };

            template<typename MatchResults, typename Expr>
            Expr const & operator ()(MatchResults &what, Expr const &expr) const
            {
                what.let(expr);
                return expr;
            }
        };

        struct let_tag
        {};

        // let(_a = b, _c = d)
        struct BindArgs
          : proto::function<
                proto::terminal<let_tag>
              , proto::vararg<
                    proto::when<
                        proto::assign<proto::_, proto::_>
                      , proto::call<BindArg(proto::_data, proto::_)>
                    >
                >
            >
        {};

        struct let_domain
          : boost::proto::domain<boost::proto::pod_generator<let_> >
        {};

        template<typename Expr>
        struct let_
        {
            BOOST_PROTO_BASIC_EXTENDS(Expr, let_<Expr>, let_domain)
            BOOST_PROTO_EXTENDS_FUNCTION()
        };

        template<typename Args, typename BidiIter>
        void bind_args(let_<Args> const &args, match_results<BidiIter> &what)
        {
            BindArgs()(args, 0, what);
        }

        typedef boost::proto::functional::make_expr<proto::tag::function, proto::default_domain> make_function;
    }

    namespace op
    {
        /// \brief \c at is a PolymorphicFunctionObject for indexing into a sequence
        struct at
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename Cont, typename Idx>
            struct result<This(Cont &, Idx)>
            {
                typedef typename Cont::reference type;
            };

            template<typename This, typename Cont, typename Idx>
            struct result<This(Cont const &, Idx)>
            {
                typedef typename Cont::const_reference type;
            };

            template<typename This, typename Cont, typename Idx>
            struct result<This(Cont, Idx)>
            {
                typedef typename Cont::const_reference type;
            };

            /// \pre    \c Cont is a model of RandomAccessSequence
            /// \param  c The RandomAccessSequence to index into
            /// \param  idx The index
            /// \return <tt>c[idx]</tt>
            template<typename Cont, typename Idx>
            typename Cont::reference operator()(Cont &c, Idx idx BOOST_PROTO_DISABLE_IF_IS_CONST(Cont)) const
            {
                return c[idx];
            }

            /// \overload
            ///
            template<typename Cont, typename Idx>
            typename Cont::const_reference operator()(Cont const &c, Idx idx) const
            {
                return c[idx];
            }
        };

        /// \brief \c push is a PolymorphicFunctionObject for pushing an element into a container.
        struct push
        {
            BOOST_PROTO_CALLABLE()
            typedef void result_type;

            /// \param seq The sequence into which the value should be pushed.
            /// \param val The value to push into the sequence.
            /// \brief Equivalent to <tt>seq.push(val)</tt>.
            /// \return \c void
            template<typename Sequence, typename Value>
            void operator()(Sequence &seq, Value const &val) const
            {
                seq.push(val);
            }
        };

        /// \brief \c push_back is a PolymorphicFunctionObject for pushing an element into the back of a container.
        struct push_back
        {
            BOOST_PROTO_CALLABLE()
            typedef void result_type;

            /// \param seq The sequence into which the value should be pushed.
            /// \param val The value to push into the sequence.
            /// \brief Equivalent to <tt>seq.push_back(val)</tt>.
            /// \return \c void
            template<typename Sequence, typename Value>
            void operator()(Sequence &seq, Value const &val) const
            {
                seq.push_back(val);
            }
        };

        /// \brief \c push_front is a PolymorphicFunctionObject for pushing an element into the front of a container.
        struct push_front
        {
            BOOST_PROTO_CALLABLE()
            typedef void result_type;

            /// \param seq The sequence into which the value should be pushed.
            /// \param val The value to push into the sequence.
            /// \brief Equivalent to <tt>seq.push_front(val)</tt>.
            /// \return \c void
            template<typename Sequence, typename Value>
            void operator()(Sequence &seq, Value const &val) const
            {
                seq.push_front(val);
            }
        };

        /// \brief \c pop is a PolymorphicFunctionObject for popping an element from a container.
        struct pop
        {
            BOOST_PROTO_CALLABLE()
            typedef void result_type;

            /// \param seq The sequence from which to pop.
            /// \brief Equivalent to <tt>seq.pop()</tt>.
            /// \return \c void
            template<typename Sequence>
            void operator()(Sequence &seq) const
            {
                seq.pop();
            }
        };

        /// \brief \c pop_back is a PolymorphicFunctionObject for popping an element from the back of a container.
        struct pop_back
        {
            BOOST_PROTO_CALLABLE()
            typedef void result_type;

            /// \param seq The sequence from which to pop.
            /// \brief Equivalent to <tt>seq.pop_back()</tt>.
            /// \return \c void
            template<typename Sequence>
            void operator()(Sequence &seq) const
            {
                seq.pop_back();
            }
        };

        /// \brief \c pop_front is a PolymorphicFunctionObject for popping an element from the front of a container.
        struct pop_front
        {
            BOOST_PROTO_CALLABLE()
            typedef void result_type;

            /// \param seq The sequence from which to pop.
            /// \brief Equivalent to <tt>seq.pop_front()</tt>.
            /// \return \c void
            template<typename Sequence>
            void operator()(Sequence &seq) const
            {
                seq.pop_front();
            }
        };

        /// \brief \c front is a PolymorphicFunctionObject for fetching the front element of a container.
        struct front
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename Sequence>
            struct result<This(Sequence)>
            {
                typedef typename remove_reference<Sequence>::type sequence_type;
                typedef
                    typename mpl::if_c<
                        is_const<sequence_type>::value
                      , typename sequence_type::const_reference
                      , typename sequence_type::reference
                    >::type
                type;
            };

            /// \param seq The sequence from which to fetch the front.
            /// \return <tt>seq.front()</tt>
            template<typename Sequence>
            typename result<front(Sequence &)>::type operator()(Sequence &seq) const
            {
                return seq.front();
            }
        };

        /// \brief \c back is a PolymorphicFunctionObject for fetching the back element of a container.
        struct back
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename Sequence>
            struct result<This(Sequence)>
            {
                typedef typename remove_reference<Sequence>::type sequence_type;
                typedef
                    typename mpl::if_c<
                        is_const<sequence_type>::value
                      , typename sequence_type::const_reference
                      , typename sequence_type::reference
                    >::type
                type;
            };

            /// \param seq The sequence from which to fetch the back.
            /// \return <tt>seq.back()</tt>
            template<typename Sequence>
            typename result<back(Sequence &)>::type operator()(Sequence &seq) const
            {
                return seq.back();
            }
        };

        /// \brief \c top is a PolymorphicFunctionObject for fetching the top element of a stack.
        struct top
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename Sequence>
            struct result<This(Sequence)>
            {
                typedef typename remove_reference<Sequence>::type sequence_type;
                typedef
                    typename mpl::if_c<
                        is_const<sequence_type>::value
                      , typename sequence_type::value_type const &
                      , typename sequence_type::value_type &
                    >::type
                type;
            };

            /// \param seq The sequence from which to fetch the top.
            /// \return <tt>seq.top()</tt>
            template<typename Sequence>
            typename result<top(Sequence &)>::type operator()(Sequence &seq) const
            {
                return seq.top();
            }
        };

        /// \brief \c first is a PolymorphicFunctionObject for fetching the first element of a pair.
        struct first
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename Pair>
            struct result<This(Pair)>
            {
                typedef typename remove_reference<Pair>::type::first_type type;
            };

            /// \param p The pair from which to fetch the first element.
            /// \return <tt>p.first</tt>
            template<typename Pair>
            typename Pair::first_type operator()(Pair const &p) const
            {
                return p.first;
            }
        };

        /// \brief \c second is a PolymorphicFunctionObject for fetching the second element of a pair.
        struct second
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename Pair>
            struct result<This(Pair)>
            {
                typedef typename remove_reference<Pair>::type::second_type type;
            };

            /// \param p The pair from which to fetch the second element.
            /// \return <tt>p.second</tt>
            template<typename Pair>
            typename Pair::second_type operator()(Pair const &p) const
            {
                return p.second;
            }
        };

        /// \brief \c matched is a PolymorphicFunctionObject for assessing whether a \c sub_match object
        ///        matched or not.
        struct matched
        {
            BOOST_PROTO_CALLABLE()
            typedef bool result_type;

            /// \param sub The \c sub_match object.
            /// \return <tt>sub.matched</tt>
            template<typename Sub>
            bool operator()(Sub const &sub) const
            {
                return sub.matched;
            }
        };

        /// \brief \c length is a PolymorphicFunctionObject for fetching the length of \c sub_match.
        struct length
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename Sub>
            struct result<This(Sub)>
            {
                typedef typename remove_reference<Sub>::type::difference_type type;
            };

            /// \param sub The \c sub_match object.
            /// \return <tt>sub.length()</tt>
            template<typename Sub>
            typename Sub::difference_type operator()(Sub const &sub) const
            {
                return sub.length();
            }
        };

        /// \brief \c str is a PolymorphicFunctionObject for turning a \c sub_match into an
        ///        equivalent \c std::string.
        struct str
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename Sub>
            struct result<This(Sub)>
            {
                typedef typename remove_reference<Sub>::type::string_type type;
            };

            /// \param sub The \c sub_match object.
            /// \return <tt>sub.str()</tt>
            template<typename Sub>
            typename Sub::string_type operator()(Sub const &sub) const
            {
                return sub.str();
            }
        };

        // This codifies the return types of the various insert member
        // functions found in sequence containers, the 2 flavors of
        // associative containers, and strings.
        //
        /// \brief \c insert is a PolymorphicFunctionObject for inserting a value or a
        ///        sequence of values into a sequence container, an associative
        ///        container, or a string.
        struct insert
        {
            BOOST_PROTO_CALLABLE()

            /// INTERNAL ONLY
            ///
            struct detail
            {
                template<typename Sig, typename EnableIf = void>
                struct result_detail
                {};

                // assoc containers
                template<typename This, typename Cont, typename Value>
                struct result_detail<This(Cont, Value), void>
                {
                    typedef typename remove_reference<Cont>::type cont_type;
                    typedef typename remove_reference<Value>::type value_type;
                    static cont_type &scont_;
                    static value_type &svalue_;
                    typedef char yes_type;
                    typedef char (&no_type)[2];
                    static yes_type check_insert_return(typename cont_type::iterator);
                    static no_type check_insert_return(std::pair<typename cont_type::iterator, bool>);
                    BOOST_STATIC_CONSTANT(bool, is_iterator = (sizeof(yes_type) == sizeof(check_insert_return(scont_.insert(svalue_)))));
                    typedef
                        typename mpl::if_c<
                            is_iterator
                          , typename cont_type::iterator
                          , std::pair<typename cont_type::iterator, bool>
                        >::type
                    type;
                };

                // sequence containers, assoc containers, strings
                template<typename This, typename Cont, typename It, typename Value>
                struct result_detail<This(Cont, It, Value),
                    typename disable_if<
                        mpl::or_<
                            is_integral<typename remove_cv<typename remove_reference<It>::type>::type>
                          , is_same<
                                typename remove_cv<typename remove_reference<It>::type>::type
                              , typename remove_cv<typename remove_reference<Value>::type>::type
                            >
                        >
                    >::type
                >
                {
                    typedef typename remove_reference<Cont>::type::iterator type;
                };

                // strings
                template<typename This, typename Cont, typename Size, typename T>
                struct result_detail<This(Cont, Size, T),
                    typename enable_if<
                        is_integral<typename remove_cv<typename remove_reference<Size>::type>::type>
                    >::type
                >
                {
                    typedef typename remove_reference<Cont>::type &type;
                };

                // assoc containers
                template<typename This, typename Cont, typename It>
                struct result_detail<This(Cont, It, It), void>
                {
                    typedef void type;
                };

                // sequence containers, strings
                template<typename This, typename Cont, typename It, typename Size, typename Value>
                struct result_detail<This(Cont, It, Size, Value),
                    typename disable_if<
                        is_integral<typename remove_cv<typename remove_reference<It>::type>::type>
                    >::type
                >
                {
                    typedef void type;
                };

                // strings
                template<typename This, typename Cont, typename Size, typename A0, typename A1>
                struct result_detail<This(Cont, Size, A0, A1),
                    typename enable_if<
                        is_integral<typename remove_cv<typename remove_reference<Size>::type>::type>
                    >::type
                >
                {
                    typedef typename remove_reference<Cont>::type &type;
                };

                // strings
                template<typename This, typename Cont, typename Pos0, typename String, typename Pos1, typename Length>
                struct result_detail<This(Cont, Pos0, String, Pos1, Length)>
                {
                    typedef typename remove_reference<Cont>::type &type;
                };
            };

            template<typename Sig>
            struct result
            {
                typedef typename detail::result_detail<Sig>::type type;
            };

            /// \overload
            ///
            template<typename Cont, typename A0>
            typename result<insert(Cont &, A0 const &)>::type
            operator()(Cont &cont, A0 const &a0) const
            {
                return cont.insert(a0);
            }

            /// \overload
            ///
            template<typename Cont, typename A0, typename A1>
            typename result<insert(Cont &, A0 const &, A1 const &)>::type
            operator()(Cont &cont, A0 const &a0, A1 const &a1) const
            {
                return cont.insert(a0, a1);
            }

            /// \overload
            ///
            template<typename Cont, typename A0, typename A1, typename A2>
            typename result<insert(Cont &, A0 const &, A1 const &, A2 const &)>::type
            operator()(Cont &cont, A0 const &a0, A1 const &a1, A2 const &a2) const
            {
                return cont.insert(a0, a1, a2);
            }

            /// \param cont The container into which to insert the element(s)
            /// \param a0 A value, iterator, or count
            /// \param a1 A value, iterator, string, count, or character
            /// \param a2 A value, iterator, or count
            /// \param a3 A count
            /// \return \li For the form <tt>insert()(cont, a0)</tt>, return <tt>cont.insert(a0)</tt>.
            ///         \li For the form <tt>insert()(cont, a0, a1)</tt>, return <tt>cont.insert(a0, a1)</tt>.
            ///         \li For the form <tt>insert()(cont, a0, a1, a2)</tt>, return <tt>cont.insert(a0, a1, a2)</tt>.
            ///         \li For the form <tt>insert()(cont, a0, a1, a2, a3)</tt>, return <tt>cont.insert(a0, a1, a2, a3)</tt>.
            template<typename Cont, typename A0, typename A1, typename A2, typename A3>
            typename result<insert(Cont &, A0 const &, A1 const &, A2 const &, A3 const &)>::type
            operator()(Cont &cont, A0 const &a0, A1 const &a1, A2 const &a2, A3 const &a3) const
            {
                return cont.insert(a0, a1, a2, a3);
            }
        };

        /// \brief \c make_pair is a PolymorphicFunctionObject for building a \c std::pair out of two parameters
        struct make_pair
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename First, typename Second>
            struct result<This(First, Second)>
            {
                /// \brief For exposition only
                typedef typename decay<First>::type first_type;
                /// \brief For exposition only
                typedef typename decay<Second>::type second_type;
                typedef std::pair<first_type, second_type> type;
            };

            /// \param first The first element of the pair
            /// \param second The second element of the pair
            /// \return <tt>std::make_pair(first, second)</tt>
            template<typename First, typename Second>
            std::pair<First, Second> operator()(First const &first, Second const &second) const
            {
                return std::make_pair(first, second);
            }
        };

        /// \brief \c as\<\> is a PolymorphicFunctionObject for lexically casting a parameter to a different type.
        /// \tparam T The type to which to lexically cast the parameter.
        template<typename T>
        struct as
        {
            BOOST_PROTO_CALLABLE()
            typedef T result_type;

            /// \param val The value to lexically cast.
            /// \return <tt>boost::lexical_cast\<T\>(val)</tt>
            template<typename Value>
            T operator()(Value const &val) const
            {
                return boost::lexical_cast<T>(val);
            }

            // Hack around some limitations in boost::lexical_cast
            /// INTERNAL ONLY
            T operator()(csub_match const &val) const
            {
                return val.matched
                  ? boost::lexical_cast<T>(boost::make_iterator_range(val.first, val.second))
                  : boost::lexical_cast<T>("");
            }

            #ifndef BOOST_XPRESSIVE_NO_WREGEX
            /// INTERNAL ONLY
            T operator()(wcsub_match const &val) const
            {
                return val.matched
                  ? boost::lexical_cast<T>(boost::make_iterator_range(val.first, val.second))
                  : boost::lexical_cast<T>("");
            }
            #endif

            /// INTERNAL ONLY
            template<typename BidiIter>
            T operator()(sub_match<BidiIter> const &val) const
            {
                // If this assert fires, you're trying to coerce a sequences of non-characters
                // to some other type. Xpressive doesn't know how to do that.
                typedef typename iterator_value<BidiIter>::type char_type;
                BOOST_MPL_ASSERT_MSG(
                    (xpressive::detail::is_char<char_type>::value)
                  , CAN_ONLY_CONVERT_FROM_CHARACTER_SEQUENCES
                  , (char_type)
                );
                return this->impl(val, xpressive::detail::is_string_iterator<BidiIter>());
            }

        private:
            /// INTERNAL ONLY
            template<typename RandIter>
            T impl(sub_match<RandIter> const &val, mpl::true_) const
            {
                return val.matched
                  ? boost::lexical_cast<T>(boost::make_iterator_range(&*val.first, &*val.first + (val.second - val.first)))
                  : boost::lexical_cast<T>("");
            }

            /// INTERNAL ONLY
            template<typename BidiIter>
            T impl(sub_match<BidiIter> const &val, mpl::false_) const
            {
                return boost::lexical_cast<T>(val.str());
            }
        };

        /// \brief \c static_cast_\<\> is a PolymorphicFunctionObject for statically casting a parameter to a different type.
        /// \tparam T The type to which to statically cast the parameter.
        template<typename T>
        struct static_cast_
        {
            BOOST_PROTO_CALLABLE()
            typedef T result_type;

            /// \param val The value to statically cast.
            /// \return <tt>static_cast\<T\>(val)</tt>
            template<typename Value>
            T operator()(Value const &val) const
            {
                return static_cast<T>(val);
            }
        };

        /// \brief \c dynamic_cast_\<\> is a PolymorphicFunctionObject for dynamically casting a parameter to a different type.
        /// \tparam T The type to which to dynamically cast the parameter.
        template<typename T>
        struct dynamic_cast_
        {
            BOOST_PROTO_CALLABLE()
            typedef T result_type;

            /// \param val The value to dynamically cast.
            /// \return <tt>dynamic_cast\<T\>(val)</tt>
            template<typename Value>
            T operator()(Value const &val) const
            {
                return dynamic_cast<T>(val);
            }
        };

        /// \brief \c const_cast_\<\> is a PolymorphicFunctionObject for const-casting a parameter to a cv qualification.
        /// \tparam T The type to which to const-cast the parameter.
        template<typename T>
        struct const_cast_
        {
            BOOST_PROTO_CALLABLE()
            typedef T result_type;

            /// \param val The value to const-cast.
            /// \pre Types \c T and \c Value differ only in cv-qualification.
            /// \return <tt>const_cast\<T\>(val)</tt>
            template<typename Value>
            T operator()(Value const &val) const
            {
                return const_cast<T>(val);
            }
        };

        /// \brief \c construct\<\> is a PolymorphicFunctionObject for constructing a new object.
        /// \tparam T The type of the object to construct.
        template<typename T>
        struct construct
        {
            BOOST_PROTO_CALLABLE()
            typedef T result_type;

            /// \overload
            T operator()() const
            {
                return T();
            }

            /// \overload
            template<typename A0>
            T operator()(A0 const &a0) const
            {
                return T(a0);
            }

            /// \overload
            template<typename A0, typename A1>
            T operator()(A0 const &a0, A1 const &a1) const
            {
                return T(a0, a1);
            }

            /// \param a0 The first argument to the constructor
            /// \param a1 The second argument to the constructor
            /// \param a2 The third argument to the constructor
            /// \return <tt>T(a0,a1,...)</tt>
            template<typename A0, typename A1, typename A2>
            T operator()(A0 const &a0, A1 const &a1, A2 const &a2) const
            {
                return T(a0, a1, a2);
            }
        };

        /// \brief \c throw_\<\> is a PolymorphicFunctionObject for throwing an exception.
        /// \tparam Except The type of the object to throw.
        template<typename Except>
        struct throw_
        {
            BOOST_PROTO_CALLABLE()
            typedef void result_type;

            /// \overload
            void operator()() const
            {
                BOOST_THROW_EXCEPTION(Except());
            }

            /// \overload
            template<typename A0>
            void operator()(A0 const &a0) const
            {
                BOOST_THROW_EXCEPTION(Except(a0));
            }

            /// \overload
            template<typename A0, typename A1>
            void operator()(A0 const &a0, A1 const &a1) const
            {
                BOOST_THROW_EXCEPTION(Except(a0, a1));
            }

            /// \param a0 The first argument to the constructor
            /// \param a1 The second argument to the constructor
            /// \param a2 The third argument to the constructor
            /// \throw <tt>Except(a0,a1,...)</tt>
            /// \note This function makes use of the \c BOOST_THROW_EXCEPTION macro
            ///       to actually throw the exception. See the documentation for the
            ///       Boost.Exception library.
            template<typename A0, typename A1, typename A2>
            void operator()(A0 const &a0, A1 const &a1, A2 const &a2) const
            {
                BOOST_THROW_EXCEPTION(Except(a0, a1, a2));
            }
        };

        /// \brief \c unwrap_reference is a PolymorphicFunctionObject for unwrapping a <tt>boost::reference_wrapper\<\></tt>.
        struct unwrap_reference
        {
            BOOST_PROTO_CALLABLE()
            template<typename Sig>
            struct result {};

            template<typename This, typename Ref>
            struct result<This(Ref)>
            {
                typedef typename boost::unwrap_reference<Ref>::type &type;
            };

            template<typename This, typename Ref>
            struct result<This(Ref &)>
            {
                typedef typename boost::unwrap_reference<Ref>::type &type;
            };

            /// \param r The <tt>boost::reference_wrapper\<T\></tt> to unwrap.
            /// \return <tt>static_cast\<T &\>(r)</tt>
            template<typename T>
            T &operator()(boost::reference_wrapper<T> r) const
            {
                return static_cast<T &>(r);
            }
        };
    }

    /// \brief A unary metafunction that turns an ordinary function object type into the type of
    /// a deferred function object for use in xpressive semantic actions.
    ///
    /// Use \c xpressive::function\<\> to turn an ordinary polymorphic function object type
    /// into a type that can be used to declare an object for use in xpressive semantic actions.
    ///
    /// For example, the global object \c xpressive::push_back can be used to create deferred actions
    /// that have the effect of pushing a value into a container. It is defined with
    /// \c xpressive::function\<\> as follows:
    ///
    /** \code
        xpressive::function<xpressive::op::push_back>::type const push_back = {};
        \endcode
    */
    ///
    /// where \c op::push_back is an ordinary function object that pushes its second argument into
    /// its first. Thus defined, \c xpressive::push_back can be used in semantic actions as follows:
    ///
    /** \code
        namespace xp = boost::xpressive;
        using xp::_;
        std::list<int> result;
        std::string str("1 23 456 7890");
        xp::sregex rx = (+_d)[ xp::push_back(xp::ref(result), xp::as<int>(_) ]
            >> *(' ' >> (+_d)[ xp::push_back(xp::ref(result), xp::as<int>(_) ) ]);
        \endcode
    */
    template<typename PolymorphicFunctionObject>
    struct function
    {
        typedef typename proto::terminal<PolymorphicFunctionObject>::type type;
    };

    /// \brief \c at is a lazy PolymorphicFunctionObject for indexing into a sequence in an
    /// xpressive semantic action.
    function<op::at>::type const at = {{}};

    /// \brief \c push is a lazy PolymorphicFunctionObject for pushing a value into a container in an
    /// xpressive semantic action.
    function<op::push>::type const push = {{}};

    /// \brief \c push_back is a lazy PolymorphicFunctionObject for pushing a value into a container in an
    /// xpressive semantic action.
    function<op::push_back>::type const push_back = {{}};

    /// \brief \c push_front is a lazy PolymorphicFunctionObject for pushing a value into a container in an
    /// xpressive semantic action.
    function<op::push_front>::type const push_front = {{}};

    /// \brief \c pop is a lazy PolymorphicFunctionObject for popping the top element from a sequence in an
    /// xpressive semantic action.
    function<op::pop>::type const pop = {{}};

    /// \brief \c pop_back is a lazy PolymorphicFunctionObject for popping the back element from a sequence in an
    /// xpressive semantic action.
    function<op::pop_back>::type const pop_back = {{}};

    /// \brief \c pop_front is a lazy PolymorphicFunctionObject for popping the front element from a sequence in an
    /// xpressive semantic action.
    function<op::pop_front>::type const pop_front = {{}};

    /// \brief \c top is a lazy PolymorphicFunctionObject for accessing the top element from a stack in an
    /// xpressive semantic action.
    function<op::top>::type const top = {{}};

    /// \brief \c back is a lazy PolymorphicFunctionObject for fetching the back element of a sequence in an
    /// xpressive semantic action.
    function<op::back>::type const back = {{}};

    /// \brief \c front is a lazy PolymorphicFunctionObject for fetching the front element of a sequence in an
    /// xpressive semantic action.
    function<op::front>::type const front = {{}};

    /// \brief \c first is a lazy PolymorphicFunctionObject for accessing the first element of a \c std::pair\<\> in an
    /// xpressive semantic action.
    function<op::first>::type const first = {{}};

    /// \brief \c second is a lazy PolymorphicFunctionObject for accessing the second element of a \c std::pair\<\> in an
    /// xpressive semantic action.
    function<op::second>::type const second = {{}};

    /// \brief \c matched is a lazy PolymorphicFunctionObject for accessing the \c matched member of a \c xpressive::sub_match\<\> in an
    /// xpressive semantic action.
    function<op::matched>::type const matched = {{}};

    /// \brief \c length is a lazy PolymorphicFunctionObject for computing the length of a \c xpressive::sub_match\<\> in an
    /// xpressive semantic action.
    function<op::length>::type const length = {{}};

    /// \brief \c str is a lazy PolymorphicFunctionObject for converting a \c xpressive::sub_match\<\> to a \c std::basic_string\<\> in an
    /// xpressive semantic action.
    function<op::str>::type const str = {{}};

    /// \brief \c insert is a lazy PolymorphicFunctionObject for inserting a value or a range of values into a sequence in an
    /// xpressive semantic action.
    function<op::insert>::type const insert = {{}};

    /// \brief \c make_pair is a lazy PolymorphicFunctionObject for making a \c std::pair\<\> in an
    /// xpressive semantic action.
    function<op::make_pair>::type const make_pair = {{}};

    /// \brief \c unwrap_reference is a lazy PolymorphicFunctionObject for unwrapping a \c boost::reference_wrapper\<\> in an
    /// xpressive semantic action.
    function<op::unwrap_reference>::type const unwrap_reference = {{}};

    /// \brief \c value\<\> is a lazy wrapper for a value that can be used in xpressive semantic actions.
    /// \tparam T The type of the value to store.
    ///
    /// Below is an example that shows where \c <tt>value\<\></tt> is useful.
    ///
    /** \code
        sregex good_voodoo(boost::shared_ptr<int> pi)
        {
            using namespace boost::xpressive;
            // Use val() to hold the shared_ptr by value:
            sregex rex = +( _d [ ++*val(pi) ] >> '!' );
            // OK, rex holds a reference count to the integer.
            return rex;
        }
        \endcode
    */
    ///
    /// In the above code, \c xpressive::val() is a function that returns a \c value\<\> object. Had
    /// \c val() not been used here, the operation <tt>++*pi</tt> would have been evaluated eagerly
    /// once, instead of lazily when the regex match happens.
    template<typename T>
    struct value
      : proto::extends<typename proto::terminal<T>::type, value<T> >
    {
        /// INTERNAL ONLY
        typedef proto::extends<typename proto::terminal<T>::type, value<T> > base_type;

        /// \brief Store a default-constructed \c T
        value()
          : base_type()
        {}

        /// \param t The initial value.
        /// \brief Store a copy of \c t.
        explicit value(T const &t)
          : base_type(base_type::proto_base_expr::make(t))
        {}

        using base_type::operator=;

        /// \overload
        T &get()
        {
            return proto::value(*this);
        }

        /// \brief Fetch the stored value
        T const &get() const
        {
            return proto::value(*this);
        }
    };

    /// \brief \c reference\<\> is a lazy wrapper for a reference that can be used in 
    /// xpressive semantic actions.
    ///
    /// \tparam T The type of the referent.
    ///
    /// Here is an example of how to use \c reference\<\> to create a lazy reference to
    /// an existing object so it can be read and written in an xpressive semantic action.
    ///
    /** \code
        using namespace boost::xpressive;
        std::map<std::string, int> result;
        reference<std::map<std::string, int> > result_ref(result);
       
        // Match a word and an integer, separated by =>,
        // and then stuff the result into a std::map<>
        sregex pair = ( (s1= +_w) >> "=>" >> (s2= +_d) )
            [ result_ref[s1] = as<int>(s2) ];
        \endcode
    */
    template<typename T>
    struct reference
      : proto::extends<typename proto::terminal<reference_wrapper<T> >::type, reference<T> >
    {
        /// INTERNAL ONLY
        typedef proto::extends<typename proto::terminal<reference_wrapper<T> >::type, reference<T> > base_type;

        /// \param t Reference to object
        /// \brief Store a reference to \c t
        explicit reference(T &t)
          : base_type(base_type::proto_base_expr::make(boost::ref(t)))
        {}

        using base_type::operator=;

        /// \brief Fetch the stored value
        T &get() const
        {
            return proto::value(*this).get();
        }
    };

    /// \brief \c local\<\> is a lazy wrapper for a reference to a value that is stored within the local itself.
    /// It is for use within xpressive semantic actions.
    ///
    /// \tparam T The type of the local variable.
    ///
    /// Below is an example of how to use \c local\<\> in semantic actions.
    ///
    /** \code
        using namespace boost::xpressive;
        local<int> i(0);
        std::string str("1!2!3?");
        // count the exciting digits, but not the
        // questionable ones.
        sregex rex = +( _d [ ++i ] >> '!' );
        regex_search(str, rex);
        assert( i.get() == 2 );
        \endcode
    */
    ///
    /// \note As the name "local" suggests, \c local\<\> objects and the regexes
    /// that refer to them should never leave the local scope. The value stored
    /// within the local object will be destroyed at the end of the \c local\<\>'s
    /// lifetime, and any regex objects still holding the \c local\<\> will be
    /// left with a dangling reference.
    template<typename T>
    struct local
      : detail::value_wrapper<T>
      , proto::terminal<reference_wrapper<T> >::type
    {
        /// INTERNAL ONLY
        typedef typename proto::terminal<reference_wrapper<T> >::type base_type;

        /// \brief Store a default-constructed value of type \c T
        local()
          : detail::value_wrapper<T>()
          , base_type(base_type::make(boost::ref(detail::value_wrapper<T>::value)))
        {}

        /// \param t The initial value.
        /// \brief Store a default-constructed value of type \c T
        explicit local(T const &t)
          : detail::value_wrapper<T>(t)
          , base_type(base_type::make(boost::ref(detail::value_wrapper<T>::value)))
        {}

        using base_type::operator=;

        /// Fetch the wrapped value.
        T &get()
        {
            return proto::value(*this);
        }

        /// \overload
        T const &get() const
        {
            return proto::value(*this);
        }
    };

    /// \brief \c as() is a lazy funtion for lexically casting a parameter to a different type.
    /// \tparam T The type to which to lexically cast the parameter.
    /// \param a The lazy value to lexically cast.
    /// \return A lazy object that, when evaluated, lexically casts its argument to the desired type.
    template<typename T, typename A>
    typename detail::make_function::impl<op::as<T> const, A const &>::result_type const
    as(A const &a)
    {
        return detail::make_function::impl<op::as<T> const, A const &>()((op::as<T>()), a);
    }

    /// \brief \c static_cast_ is a lazy funtion for statically casting a parameter to a different type.
    /// \tparam T The type to which to statically cast the parameter.
    /// \param a The lazy value to statically cast.
    /// \return A lazy object that, when evaluated, statically casts its argument to the desired type.
    template<typename T, typename A>
    typename detail::make_function::impl<op::static_cast_<T> const, A const &>::result_type const
    static_cast_(A const &a)
    {
        return detail::make_function::impl<op::static_cast_<T> const, A const &>()((op::static_cast_<T>()), a);
    }

    /// \brief \c dynamic_cast_ is a lazy funtion for dynamically casting a parameter to a different type.
    /// \tparam T The type to which to dynamically cast the parameter.
    /// \param a The lazy value to dynamically cast.
    /// \return A lazy object that, when evaluated, dynamically casts its argument to the desired type.
    template<typename T, typename A>
    typename detail::make_function::impl<op::dynamic_cast_<T> const, A const &>::result_type const
    dynamic_cast_(A const &a)
    {
        return detail::make_function::impl<op::dynamic_cast_<T> const, A const &>()((op::dynamic_cast_<T>()), a);
    }

    /// \brief \c dynamic_cast_ is a lazy funtion for const-casting a parameter to a different type.
    /// \tparam T The type to which to const-cast the parameter.
    /// \param a The lazy value to const-cast.
    /// \return A lazy object that, when evaluated, const-casts its argument to the desired type.
    template<typename T, typename A>
    typename detail::make_function::impl<op::const_cast_<T> const, A const &>::result_type const
    const_cast_(A const &a)
    {
        return detail::make_function::impl<op::const_cast_<T> const, A const &>()((op::const_cast_<T>()), a);
    }

    /// \brief Helper for constructing \c value\<\> objects.
    /// \return <tt>value\<T\>(t)</tt>
    template<typename T>
    value<T> const val(T const &t)
    {
        return value<T>(t);
    }

    /// \brief Helper for constructing \c reference\<\> objects.
    /// \return <tt>reference\<T\>(t)</tt>
    template<typename T>
    reference<T> const ref(T &t)
    {
        return reference<T>(t);
    }

    /// \brief Helper for constructing \c reference\<\> objects that
    /// store a reference to const.
    /// \return <tt>reference\<T const\>(t)</tt>
    template<typename T>
    reference<T const> const cref(T const &t)
    {
        return reference<T const>(t);
    }

    /// \brief For adding user-defined assertions to your regular expressions.
    ///
    /// \param t The UnaryPredicate object or Boolean semantic action.
    ///
    /// A \RefSect{user_s_guide.semantic_actions_and_user_defined_assertions.user_defined_assertions,user-defined assertion}
    /// is a kind of semantic action that evaluates
    /// a Boolean lambda and, if it evaluates to false, causes the match to
    /// fail at that location in the string. This will cause backtracking,
    /// so the match may ultimately succeed.
    ///
    /// To use \c check() to specify a user-defined assertion in a regex, use the
    /// following syntax:
    ///
    /** \code
        sregex s = (_d >> _d)[check( XXX )]; // XXX is a custom assertion
        \endcode
    */
    ///
    /// The assertion is evaluated with a \c sub_match\<\> object that delineates
    /// what part of the string matched the sub-expression to which the assertion
    /// was attached.
    ///
    /// \c check() can be used with an ordinary predicate that takes a
    /// \c sub_match\<\> object as follows:
    ///
    /** \code
        // A predicate that is true IFF a sub-match is
        // either 3 or 6 characters long.
        struct three_or_six
        {
            bool operator()(ssub_match const &sub) const
            {
                return sub.length() == 3 || sub.length() == 6;
            }
        };

        // match words of 3 characters or 6 characters.
        sregex rx = (bow >> +_w >> eow)[ check(three_or_six()) ] ;
        \endcode
    */
    ///
    /// Alternately, \c check() can be used to define inline custom
    /// assertions with the same syntax as is used to define semantic
    /// actions. The following code is equivalent to above:
    ///
    /** \code
        // match words of 3 characters or 6 characters.
        sregex rx = (bow >> +_w >> eow)[ check(length(_)==3 || length(_)==6) ] ;
        \endcode
    */
    ///
    /// Within a custom assertion, \c _ is a placeholder for the \c sub_match\<\>
    /// That delineates the part of the string matched by the sub-expression to
    /// which the custom assertion was attached.
#ifdef BOOST_XPRESSIVE_DOXYGEN_INVOKED // A hack so Doxygen emits something more meaningful.
    template<typename T>
    detail::unspecified check(T const &t);
#else
    proto::terminal<detail::check_tag>::type const check = {{}};
#endif

    /// \brief For binding local variables to placeholders in semantic actions when
    /// constructing a \c regex_iterator or a \c regex_token_iterator.
    ///
    /// \param args A set of argument bindings, where each argument binding is an assignment
    /// expression, the left hand side of which must be an instance of \c placeholder\<X\>
    /// for some \c X, and the right hand side is an lvalue of type \c X.
    ///
    /// \c xpressive::let() serves the same purpose as <tt>match_results::let()</tt>;
    /// that is, it binds a placeholder to a local value. The purpose is to allow a
    /// regex with semantic actions to be defined that refers to objects that do not yet exist.
    /// Rather than referring directly to an object, a semantic action can refer to a placeholder,
    /// and the value of the placeholder can be specified later with a <em>let expression</em>.
    /// The <em>let expression</em> created with \c let() is passed to the constructor of either
    /// \c regex_iterator or \c regex_token_iterator.
    ///
    /// See the section \RefSect{user_s_guide.semantic_actions_and_user_defined_assertions.referring_to_non_local_variables, "Referring to Non-Local Variables"}
    /// in the Users' Guide for more discussion.
    ///
    /// \em Example:
    ///
    /**
        \code
        // Define a placeholder for a map object:
        placeholder<std::map<std::string, int> > _map;

        // Match a word and an integer, separated by =>,
        // and then stuff the result into a std::map<>
        sregex pair = ( (s1= +_w) >> "=>" >> (s2= +_d) )
            [ _map[s1] = as<int>(s2) ];

        // The string to parse
        std::string str("aaa=>1 bbb=>23 ccc=>456");

        // Here is the actual map to fill in:
        std::map<std::string, int> result;

        // Create a regex_iterator to find all the matches
        sregex_iterator it(str.begin(), str.end(), pair, let(_map=result));
        sregex_iterator end;

        // step through all the matches, and fill in
        // the result map
        while(it != end)
            ++it;

        std::cout << result["aaa"] << '\n';
        std::cout << result["bbb"] << '\n';
        std::cout << result["ccc"] << '\n';
        \endcode
    */
    ///
    /// The above code displays:
    ///
    /** \code{.txt}
        1
        23
        456
        \endcode
    */
#ifdef BOOST_XPRESSIVE_DOXYGEN_INVOKED // A hack so Doxygen emits something more meaningful.
    template<typename...ArgBindings>
    detail::unspecified let(ArgBindings const &...args);
#else
    detail::let_<proto::terminal<detail::let_tag>::type> const let = {{{}}};
#endif

    /// \brief For defining a placeholder to stand in for a variable a semantic action.
    ///
    /// Use \c placeholder\<\> to define a placeholder for use in semantic actions to stand
    /// in for real objects. The use of placeholders allows regular expressions with actions
    /// to be defined once and reused in many contexts to read and write from objects which
    /// were not available when the regex was defined.
    ///
    /// \tparam T The type of the object for which this placeholder stands in.
    /// \tparam I An optional identifier that can be used to distinguish this placeholder
    ///           from others that may be used in the same semantic action that happen
    ///           to have the same type.
    ///
    /// You can use \c placeholder\<\> by creating an object of type \c placeholder\<T\>
    /// and using that object in a semantic action exactly as you intend an object of
    /// type \c T to be used.
    ///
    /**
        \code
        placeholder<int> _i;
        placeholder<double> _d;

        sregex rex = ( some >> regex >> here )
            [ ++_i, _d *= _d ];
        \endcode
    */
    ///
    /// Then, when doing a pattern match with either \c regex_search(),
    /// \c regex_match() or \c regex_replace(), pass a \c match_results\<\> object that
    /// contains bindings for the placeholders used in the regex object's semantic actions.
    /// You can create the bindings by calling \c match_results::let as follows:
    ///
    /**
        \code
        int i = 0;
        double d = 3.14;

        smatch what;
        what.let(_i = i)
            .let(_d = d);

        if(regex_match("some string", rex, what))
           // i and d mutated here
        \endcode
    */
    ///
    /// If a semantic action executes that contains an unbound placeholder, a exception of
    /// type \c regex_error is thrown.
    ///
    /// See the discussion for \c xpressive::let() and the
    /// \RefSect{user_s_guide.semantic_actions_and_user_defined_assertions.referring_to_non_local_variables, "Referring to Non-Local Variables"}
    /// section in the Users' Guide for more information.
    ///
    /// <em>Example:</em>
    ///
    /**
        \code
        // Define a placeholder for a map object:
        placeholder<std::map<std::string, int> > _map;

        // Match a word and an integer, separated by =>,
        // and then stuff the result into a std::map<>
        sregex pair = ( (s1= +_w) >> "=>" >> (s2= +_d) )
            [ _map[s1] = as<int>(s2) ];

        // Match one or more word/integer pairs, separated
        // by whitespace.
        sregex rx = pair >> *(+_s >> pair);

        // The string to parse
        std::string str("aaa=>1 bbb=>23 ccc=>456");

        // Here is the actual map to fill in:
        std::map<std::string, int> result;

        // Bind the _map placeholder to the actual map
        smatch what;
        what.let( _map = result );

        // Execute the match and fill in result map
        if(regex_match(str, what, rx))
        {
            std::cout << result["aaa"] << '\n';
            std::cout << result["bbb"] << '\n';
            std::cout << result["ccc"] << '\n';
        }
        \endcode
    */
#ifdef BOOST_XPRESSIVE_DOXYGEN_INVOKED // A hack so Doxygen emits something more meaningful.
    template<typename T, int I = 0>
    struct placeholder
    {
        /// \param t The object to associate with this placeholder
        /// \return An object of unspecified type that records the association of \c t
        /// with \c *this.
        detail::unspecified operator=(T &t) const;
        /// \overload
        detail::unspecified operator=(T const &t) const;
    };
#else
    template<typename T, int I, typename Dummy>
    struct placeholder
    {
        typedef placeholder<T, I, Dummy> this_type;
        typedef
            typename proto::terminal<detail::action_arg<T, mpl::int_<I> > >::type
        action_arg_type;

        BOOST_PROTO_EXTENDS(action_arg_type, this_type, proto::default_domain)
    };
#endif

    /// \brief A lazy funtion for constructing objects objects of the specified type.
    /// \tparam T The type of object to construct.
    /// \param args The arguments to the constructor.
    /// \return A lazy object that, when evaluated, returns <tt>T(xs...)</tt>, where
    ///         <tt>xs...</tt> is the result of evaluating the lazy arguments
    ///         <tt>args...</tt>.
#ifdef BOOST_XPRESSIVE_DOXYGEN_INVOKED // A hack so Doxygen emits something more meaningful.
    template<typename T, typename ...Args>
    detail::unspecified construct(Args const &...args);
#else
/// INTERNAL ONLY
#define BOOST_PROTO_LOCAL_MACRO(N, typename_A, A_const_ref, A_const_ref_a, a)                       \
    template<typename X2_0 BOOST_PP_COMMA_IF(N) typename_A(N)>                                      \
    typename detail::make_function::impl<                                                           \
        op::construct<X2_0> const                                                                   \
        BOOST_PP_COMMA_IF(N) A_const_ref(N)                                                         \
    >::result_type const                                                                            \
    construct(A_const_ref_a(N))                                                                     \
    {                                                                                               \
        return detail::make_function::impl<                                                         \
            op::construct<X2_0> const                                                               \
            BOOST_PP_COMMA_IF(N) A_const_ref(N)                                                     \
        >()((op::construct<X2_0>()) BOOST_PP_COMMA_IF(N) a(N));                                     \
    }                                                                                               \
                                                                                                    \
    template<typename X2_0 BOOST_PP_COMMA_IF(N) typename_A(N)>                                      \
    typename detail::make_function::impl<                                                           \
        op::throw_<X2_0> const                                                                      \
        BOOST_PP_COMMA_IF(N) A_const_ref(N)                                                         \
    >::result_type const                                                                            \
    throw_(A_const_ref_a(N))                                                                        \
    {                                                                                               \
        return detail::make_function::impl<                                                         \
            op::throw_<X2_0> const                                                                  \
            BOOST_PP_COMMA_IF(N) A_const_ref(N)                                                     \
        >()((op::throw_<X2_0>()) BOOST_PP_COMMA_IF(N) a(N));                                        \
    }                                                                                               \
    /**/

    #define BOOST_PROTO_LOCAL_a         BOOST_PROTO_a                               ///< INTERNAL ONLY
    #define BOOST_PROTO_LOCAL_LIMITS    (0, BOOST_PP_DEC(BOOST_PROTO_MAX_ARITY))    ///< INTERNAL ONLY
    #include BOOST_PROTO_LOCAL_ITERATE()
#endif

    namespace detail
    {
        inline void ignore_unused_regex_actions()
        {
            detail::ignore_unused(xpressive::at);
            detail::ignore_unused(xpressive::push);
            detail::ignore_unused(xpressive::push_back);
            detail::ignore_unused(xpressive::push_front);
            detail::ignore_unused(xpressive::pop);
            detail::ignore_unused(xpressive::pop_back);
            detail::ignore_unused(xpressive::pop_front);
            detail::ignore_unused(xpressive::top);
            detail::ignore_unused(xpressive::back);
            detail::ignore_unused(xpressive::front);
            detail::ignore_unused(xpressive::first);
            detail::ignore_unused(xpressive::second);
            detail::ignore_unused(xpressive::matched);
            detail::ignore_unused(xpressive::length);
            detail::ignore_unused(xpressive::str);
            detail::ignore_unused(xpressive::insert);
            detail::ignore_unused(xpressive::make_pair);
            detail::ignore_unused(xpressive::unwrap_reference);
            detail::ignore_unused(xpressive::check);
            detail::ignore_unused(xpressive::let);
        }

        struct mark_nbr
        {
            BOOST_PROTO_CALLABLE()
            typedef int result_type;

            int operator()(mark_placeholder m) const
            {
                return m.mark_number_;
            }
        };

        struct ReplaceAlgo
          : proto::or_<
                proto::when<
                    proto::terminal<mark_placeholder>
                  , op::at(proto::_data, proto::call<mark_nbr(proto::_value)>)
                >
              , proto::when<
                    proto::terminal<any_matcher>
                  , op::at(proto::_data, proto::size_t<0>)
                >
              , proto::when<
                    proto::terminal<reference_wrapper<proto::_> >
                  , op::unwrap_reference(proto::_value)
                >
              , proto::_default<ReplaceAlgo>
            >
        {};
    }
}}

#if BOOST_MSVC
#pragma warning(pop)
#endif

#endif // BOOST_XPRESSIVE_ACTIONS_HPP_EAN_03_22_2007
