
// Copyright (C) 2005 Igor Chesnokov, mailto:ichesnokov@gmail.com (VC 6.5,VC 7.1 + counter code)
// Copyright (C) 2005-2007 Peder Holt (VC 7.0 + framework)
// Copyright (C) 2006 Steven Watanabe (VC 8.0)

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_MSVC_TYPEOF_IMPL_HPP_INCLUDED
# define BOOST_TYPEOF_MSVC_TYPEOF_IMPL_HPP_INCLUDED

# include <boost/config.hpp>
# include <boost/config/workaround.hpp>
# include <boost/typeof/constant.hpp>
# include <boost/type_traits/enable_if.hpp>
# include <boost/type_traits/is_function.hpp>

# include <typeinfo>

namespace boost
{
    namespace type_of
    {

        //Compile time constant code
# if defined(_MSC_EXTENSIONS)
        template<int N> struct the_counter;

        template<typename T,int N = 5/*for similarity*/>
        struct encode_counter
        {
            __if_exists(the_counter<N + 256>)
            {
                BOOST_STATIC_CONSTANT(unsigned,count=(encode_counter<T,N + 257>::count));
            }
            __if_not_exists(the_counter<N + 256>)
            {
                __if_exists(the_counter<N + 64>)
                {
                    BOOST_STATIC_CONSTANT(unsigned,count=(encode_counter<T,N + 65>::count));
                }
                __if_not_exists(the_counter<N + 64>)
                {
                    __if_exists(the_counter<N + 16>)
                    {
                        BOOST_STATIC_CONSTANT(unsigned,count=(encode_counter<T,N + 17>::count));
                    }
                    __if_not_exists(the_counter<N + 16>)
                    {
                        __if_exists(the_counter<N + 4>)
                        {
                            BOOST_STATIC_CONSTANT(unsigned,count=(encode_counter<T,N + 5>::count));
                        }
                        __if_not_exists(the_counter<N + 4>)
                        {
                            __if_exists(the_counter<N>)
                            {
                                BOOST_STATIC_CONSTANT(unsigned,count=(encode_counter<T,N + 1>::count));
                            }
                            __if_not_exists(the_counter<N>)
                            {
                                BOOST_STATIC_CONSTANT(unsigned,count=N);
                                typedef the_counter<N> type;
                            }
                        }
                    }
                }
            }
        };

# define BOOST_TYPEOF_INDEX(T) (encode_counter<T>::count)
# define BOOST_TYPEOF_NEXT_INDEX(next)
# else
        template<int N> struct encode_counter : encode_counter<N - 1> {};
        template<> struct encode_counter<0> {};

        //Need to default to a larger value than 4, as due to MSVC's ETI errors. (sizeof(int)==4)
        char (*encode_index(...))[5];

# define BOOST_TYPEOF_INDEX(T) (sizeof(*boost::type_of::encode_index((boost::type_of::encode_counter<1005>*)0)))
# define BOOST_TYPEOF_NEXT_INDEX(next) friend char (*encode_index(encode_counter<next>*))[next];
# endif

        //Typeof code

# if BOOST_WORKAROUND(BOOST_MSVC,>=1400)
        struct msvc_extract_type_default_param {};

        template<typename ID, typename T = msvc_extract_type_default_param>
        struct msvc_extract_type;

        template<typename ID>
        struct msvc_extract_type<ID, msvc_extract_type_default_param> {
            template<bool>
            struct id2type_impl;

            typedef id2type_impl<true> id2type;
        };

        template<typename ID, typename T>
        struct msvc_extract_type : msvc_extract_type<ID,msvc_extract_type_default_param>
        {
            template<>
            struct id2type_impl<true>  //VC8.0 specific bugfeature
            {
                typedef T type;
            };
            template<bool>
            struct id2type_impl;

            typedef id2type_impl<true> id2type;
        };

        template<typename T, typename ID>
        struct msvc_register_type : msvc_extract_type<ID, T>
        {
        };
# else
        template<typename ID>
        struct msvc_extract_type
        {
            struct id2type;
        };

        template<typename T, typename ID>
        struct msvc_register_type : msvc_extract_type<ID>
        {
            typedef msvc_extract_type<ID> base_type;
            struct base_type::id2type // This uses nice VC6.5 and VC7.1 bugfeature
            {
                typedef T type;
            };
        };
# endif

        template<int ID>
        struct msvc_typeid_wrapper {
            typedef typename msvc_extract_type<constant<int,ID> >::id2type id2type;
            typedef typename id2type::type type;
        };
        //Workaround for ETI-bug for VC6 and VC7
        template<>
        struct msvc_typeid_wrapper<1> {
            typedef msvc_typeid_wrapper<1> type;
        };
        //Workaround for ETI-bug for VC7.1
        template<>
        struct msvc_typeid_wrapper<4> {
            typedef msvc_typeid_wrapper<4> type;
        };

        //Tie it all together
        template<typename T>
        struct encode_type
        {
            //Get the next available compile time constants index
            BOOST_STATIC_CONSTANT(unsigned,value=BOOST_TYPEOF_INDEX(T));
            //Instantiate the template
            typedef typename msvc_register_type<T,constant<int,value> >::id2type type;
            //Set the next compile time constants index
            BOOST_STATIC_CONSTANT(unsigned,next=value+1);
            //Increment the compile time constant (only needed when extensions are not active
            BOOST_TYPEOF_NEXT_INDEX(next);
        };

        template<class T>
        struct sizer
        {
            typedef char(*type)[encode_type<T>::value];
        };
        template<typename T> typename enable_if_<
            !is_function<T>::value,
            typename sizer<T>::type>::type encode_start(T const&);

        template<typename T> typename enable_if_<
            is_function<T>::value,
            typename sizer<T>::type>::type encode_start(T&);
        template<typename Organizer, typename T>
        msvc_register_type<T,Organizer> typeof_register_type(const T&,Organizer* =0);

# define BOOST_TYPEOF(expr) \
    boost::type_of::msvc_typeid_wrapper<sizeof(*boost::type_of::encode_start(expr))>::type

# define BOOST_TYPEOF_TPL(expr) typename BOOST_TYPEOF(expr)

# define BOOST_TYPEOF_NESTED_TYPEDEF_TPL(name,expr) \
    struct name {\
        enum {_typeof_register_value=sizeof(boost::type_of::typeof_register_type<name>(expr))};\
        typedef typename boost::type_of::msvc_extract_type<name>::id2type id2type;\
        typedef typename id2type::type type;\
    };

# define BOOST_TYPEOF_NESTED_TYPEDEF(name,expr) \
    struct name {\
        enum {_typeof_register_value=sizeof(boost::type_of::typeof_register_type<name>(expr))};\
        typedef boost::type_of::msvc_extract_type<name>::id2type id2type;\
        typedef id2type::type type;\
    };

    }
}

#endif//BOOST_TYPEOF_MSVC_TYPEOF_IMPL_HPP_INCLUDED
