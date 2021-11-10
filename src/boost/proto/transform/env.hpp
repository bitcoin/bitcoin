///////////////////////////////////////////////////////////////////////////////
// env.hpp
// Helpers for producing and consuming tranform env variables.
//
//  Copyright 2012 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TRANSFORM_ENV_HPP_EAN_18_07_2012
#define BOOST_PROTO_TRANSFORM_ENV_HPP_EAN_18_07_2012

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/ref.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/transform/impl.hpp>
#include <boost/proto/detail/poly_function.hpp>
#include <boost/proto/detail/is_noncopyable.hpp>

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4180) // qualifier applied to function type has no meaning; ignored
#endif

namespace boost
{
    namespace proto
    {
        namespace detail
        {
            template<typename T>
            struct value_type
            {
                typedef typename remove_const<T>::type value;
                typedef typename add_reference<T>::type reference;
                typedef typename mpl::if_c<is_noncopyable<T>::value, reference, value>::type type;
            };

            template<typename T>
            struct value_type<T &>
            {
                typedef T &value;
                typedef T &reference;
                typedef T &type;
            };
        }

    #define BOOST_PROTO_DEFINE_ENV_VAR(TAG, NAME)                                                   \
        struct TAG                                                                                  \
        {                                                                                           \
            template<typename Value>                                                                \
            boost::proto::env<TAG, Value &> const                                                   \
                operator =(boost::reference_wrapper<Value> &value) const                            \
            {                                                                                       \
                return boost::proto::env<TAG, Value &>(value.get());                                \
            }                                                                                       \
            template<typename Value>                                                                \
            boost::proto::env<TAG, Value &> const                                                   \
                operator =(boost::reference_wrapper<Value> const &value) const                      \
            {                                                                                       \
                return boost::proto::env<TAG, Value &>(value.get());                                \
            }                                                                                       \
            template<typename Value>                                                                \
            typename boost::disable_if_c<                                                           \
                boost::is_const<Value>::value                                                       \
              , boost::proto::env<TAG, typename boost::proto::detail::value_type<Value>::type>      \
            >::type const operator =(Value &value) const                                            \
            {                                                                                       \
                return boost::proto::env<TAG, typename boost::proto::detail::value_type<Value>::type>(value); \
            }                                                                                       \
            template<typename Value>                                                                \
            boost::proto::env<TAG, typename boost::proto::detail::value_type<Value const>::type> const \
                operator =(Value const &value) const                                                \
            {                                                                                       \
                return boost::proto::env<TAG, typename boost::proto::detail::value_type<Value const>::type>(value); \
            }                                                                                       \
        };                                                                                          \
                                                                                                    \
        TAG const NAME = {}                                                                         \
        /**/

        namespace envns_
        {
            ////////////////////////////////////////////////////////////////////////////////////////////
            // env
            // A transform env is a slot-based storage mechanism, accessible by tag.
            template<typename Key, typename Value, typename Base /*= empty_env*/>
            struct env
              : private Base
            {
            private:
                Value value_;

            public:
                typedef Value value_type;
                typedef typename add_reference<Value>::type reference;
                typedef typename add_reference<typename add_const<Value>::type>::type const_reference;
                typedef void proto_environment_; ///< INTERNAL ONLY

                explicit env(const_reference value, Base const &base = Base())
                  : Base(base)
                  , value_(value)
                {}

                #if BOOST_WORKAROUND(__GNUC__, == 3) || (BOOST_WORKAROUND(__GNUC__, == 4) && __GNUC_MINOR__ <= 2)
                /// INTERNAL ONLY
                struct found
                {
                    typedef Value type;
                    typedef typename add_reference<typename add_const<Value>::type>::type const_reference;
                };

                template<typename OtherKey, typename OtherValue = key_not_found>
                struct lookup
                  : mpl::if_c<
                        is_same<OtherKey, Key>::value
                      , found
                      , typename Base::template lookup<OtherKey, OtherValue>
                    >::type
                {};
                #else
                /// INTERNAL ONLY
                template<typename OtherKey, typename OtherValue = key_not_found>
                struct lookup
                  : Base::template lookup<OtherKey, OtherValue>
                {};

                /// INTERNAL ONLY
                template<typename OtherValue>
                struct lookup<Key, OtherValue>
                {
                    typedef Value type;
                    typedef typename add_reference<typename add_const<Value>::type>::type const_reference;
                };
                #endif

                // For key-based lookups not intended to fail
                using Base::operator[];
                const_reference operator[](Key) const
                {
                    return this->value_;
                }

                // For key-based lookups that can fail, use the default if key not found.
                using Base::at;
                template<typename T>
                const_reference at(Key, T const &) const
                {
                    return this->value_;
                }
            };

            // define proto::data_type type and proto::data global
            BOOST_PROTO_DEFINE_ENV_VAR(data_type, data);
        }

        using envns_::data;

        namespace functional
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            // as_env
            struct as_env
            {
                BOOST_PROTO_CALLABLE()
                BOOST_PROTO_POLY_FUNCTION()

                /// INTERNAL ONLY
                template<typename T, bool B = is_env<T>::value>
                struct impl
                {
                    typedef env<data_type, typename detail::value_type<T>::type> result_type;

                    result_type const operator()(detail::arg<T> t) const
                    {
                        return result_type(t());
                    }
                };

                /// INTERNAL ONLY
                template<typename T>
                struct impl<T, true>
                {
                    typedef T result_type;

                    typename add_const<T>::type operator()(detail::arg<T> t) const
                    {
                        return t();
                    }
                };

                template<typename Sig>
                struct result;

                template<typename This, typename T>
                struct result<This(T)>
                {
                    typedef typename impl<typename detail::normalize_arg<T>::type>::result_type type;
                };

                template<typename T>
                typename impl<typename detail::normalize_arg<T &>::type>::result_type const
                    operator()(T &t BOOST_PROTO_DISABLE_IF_IS_CONST(T)) const
                {
                    return impl<typename detail::normalize_arg<T &>::type>()(
                        static_cast<typename detail::normalize_arg<T &>::reference>(t)
                    );
                }

                template<typename T>
                typename impl<typename detail::normalize_arg<T const &>::type>::result_type const
                    operator()(T const &t) const
                {
                    return impl<typename detail::normalize_arg<T const &>::type>()(
                        static_cast<typename detail::normalize_arg<T const &>::reference>(t)
                    );
                }
            };

            ////////////////////////////////////////////////////////////////////////////////////////
            // has_env_var
            template<typename Key>
            struct has_env_var
              : detail::poly_function<has_env_var<Key> >
            {
                BOOST_PROTO_CALLABLE()

                template<typename Env, bool IsEnv = is_env<Env>::value>
                struct impl
                {
                    typedef
                        mpl::not_<
                            is_same<
                                typename remove_reference<Env>::type::template lookup<Key>::type
                              , key_not_found
                            >
                        >
                    result_type;

                    result_type operator()(detail::arg<Env>) const
                    {
                        return result_type();
                    }
                };

                template<typename Env>
                struct impl<Env, false>
                {
                    typedef mpl::false_ result_type;

                    result_type operator()(detail::arg<Env>) const
                    {
                        return result_type();
                    }
                };
            };

            template<>
            struct has_env_var<data_type>
              : detail::poly_function<has_env_var<data_type> >
            {
                BOOST_PROTO_CALLABLE()

                template<typename Env, bool IsEnv = is_env<Env>::value>
                struct impl
                {
                    typedef
                        mpl::not_<
                            is_same<
                                typename remove_reference<Env>::type::template lookup<data_type>::type
                              , key_not_found
                            >
                        >
                    result_type;

                    result_type operator()(detail::arg<Env>) const
                    {
                        return result_type();
                    }
                };

                template<typename Env>
                struct impl<Env, false>
                {
                    typedef mpl::true_ result_type;

                    result_type operator()(detail::arg<Env>) const
                    {
                        return result_type();
                    }
                };
            };

            ////////////////////////////////////////////////////////////////////////////////////////
            // env_var
            template<typename Key>
            struct env_var
              : detail::poly_function<env_var<Key> >
            {
                BOOST_PROTO_CALLABLE()

                template<typename Env>
                struct impl
                {
                    typedef
                        typename remove_reference<Env>::type::template lookup<Key>::type
                    result_type;

                    result_type operator()(detail::arg<Env> e) const
                    {
                        return e()[Key()];
                    }
                };
            };

            template<>
            struct env_var<data_type>
              : detail::poly_function<env_var<data_type> >
            {
                BOOST_PROTO_CALLABLE()

                template<typename Env, bool B = is_env<Env>::value>
                struct impl
                {
                    typedef Env result_type;

                    result_type operator()(detail::arg<Env> e) const
                    {
                        return e();
                    }
                };

                template<typename Env>
                struct impl<Env, true>
                {
                    typedef
                        typename remove_reference<Env>::type::template lookup<data_type>::type
                    result_type;

                    result_type operator()(detail::arg<Env> e) const
                    {
                        return e()[proto::data];
                    }
                };
            };
        }

        namespace result_of
        {
            template<typename T>
            struct as_env
              : BOOST_PROTO_RESULT_OF<functional::as_env(T)>
            {};

            template<typename Env, typename Key>
            struct has_env_var
              : BOOST_PROTO_RESULT_OF<functional::has_env_var<Key>(Env)>::type
            {};

            template<typename Env, typename Key>
            struct env_var
              : BOOST_PROTO_RESULT_OF<functional::env_var<Key>(Env)>
            {};
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // as_env
        template<typename T>
        typename proto::result_of::as_env<T &>::type const as_env(T &t BOOST_PROTO_DISABLE_IF_IS_CONST(T))
        {
            return proto::functional::as_env()(t);
        }

        template<typename T>
        typename proto::result_of::as_env<T const &>::type const as_env(T const &t)
        {
            return proto::functional::as_env()(t);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // has_env_var
        template<typename Key, typename Env>
        typename proto::result_of::has_env_var<Env &, Key>::type has_env_var(Env &e BOOST_PROTO_DISABLE_IF_IS_CONST(Env))
        {
            return functional::has_env_var<Key>()(e);
        }

        template<typename Key, typename Env>
        typename proto::result_of::has_env_var<Env const &, Key>::type has_env_var(Env const &e)
        {
            return functional::has_env_var<Key>()(e);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // env_var
        template<typename Key, typename Env>
        typename proto::result_of::env_var<Env &, Key>::type env_var(Env &e BOOST_PROTO_DISABLE_IF_IS_CONST(Env))
        {
            return functional::env_var<Key>()(e);
        }

        template<typename Key, typename Env>
        typename proto::result_of::env_var<Env const &, Key>::type env_var(Env const &e)
        {
            return functional::env_var<Key>()(e);
        }

        namespace envns_
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            // env operator,
            template<typename T, typename T1, typename V1>
            inline typename disable_if_c<
                is_const<T>::value
              , env<T1, V1, BOOST_PROTO_UNCVREF(typename result_of::as_env<T &>::type)>
            >::type const operator,(T &t, env<T1, V1> const &head)
            {
                return env<T1, V1, BOOST_PROTO_UNCVREF(typename result_of::as_env<T &>::type)>(
                    head[T1()]
                  , proto::as_env(t)
                );
            }

            template<typename T, typename T1, typename V1>
            inline env<T1, V1, BOOST_PROTO_UNCVREF(typename result_of::as_env<T const &>::type)> const
                operator,(T const &t, env<T1, V1> const &head)
            {
                return env<T1, V1, BOOST_PROTO_UNCVREF(typename result_of::as_env<T const &>::type)>(
                    head[T1()]
                  , proto::as_env(t)
                );
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // _env_var
        template<typename Key>
        struct _env_var
          : proto::transform<_env_var<Key> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : transform_impl<Expr, State, Data>
            {
                typedef typename impl::data::template lookup<Key>::type result_type;
                BOOST_MPL_ASSERT_NOT((is_same<result_type, key_not_found>)); // lookup failed

                BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, typename impl::data::template lookup<Key>::const_reference)
                operator ()(
                    typename impl::expr_param
                  , typename impl::state_param
                  , typename impl::data_param d
                ) const
                {
                    return d[Key()];
                }
            };
        };

        struct _env
          : transform<_env>
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : transform_impl<Expr, State, Data>
            {
                typedef Data result_type;

                BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, typename impl::data_param)
                operator ()(
                    typename impl::expr_param
                  , typename impl::state_param
                  , typename impl::data_param d
                ) const
                {
                    return d;
                }
            };
        };

        /// INTERNAL ONLY
        template<typename Key>
        struct is_callable<_env_var<Key> >
          : mpl::true_
        {};

        /// INTERNAL ONLY
        template<typename Key>
        struct is_callable<functional::has_env_var<Key> >
          : mpl::true_
        {};

        /// INTERNAL ONLY
        template<typename Key>
        struct is_callable<functional::env_var<Key> >
          : mpl::true_
        {};
    }
}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif
