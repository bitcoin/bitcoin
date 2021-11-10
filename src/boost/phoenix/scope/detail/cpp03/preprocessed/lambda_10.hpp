/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2004 Daniel Wallin
    Copyright (c) 2010 Thomas Heller
    Copyright (c) 2016 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
    
    
    
    
    
    
    
        template <typename A0>
        lambda_actor_gen<
            vector1<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0) const
        {
            typedef
                vector1<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type
                    >
                >(locals);
        }
    
    
    
    
    
    
    
        template <typename A0 , typename A1>
        lambda_actor_gen<
            vector2<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0 , A1 const& a1) const
        {
            typedef
                vector2<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0) , proto::child_c<1>(a1)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type
                    >
                >(locals);
        }
    
    
    
    
    
    
    
        template <typename A0 , typename A1 , typename A2>
        lambda_actor_gen<
            vector3<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0 , A1 const& a1 , A2 const& a2) const
        {
            typedef
                vector3<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0) , proto::child_c<1>(a1) , proto::child_c<1>(a2)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type
                    >
                >(locals);
        }
    
    
    
    
    
    
    
        template <typename A0 , typename A1 , typename A2 , typename A3>
        lambda_actor_gen<
            vector4<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3) const
        {
            typedef
                vector4<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0) , proto::child_c<1>(a1) , proto::child_c<1>(a2) , proto::child_c<1>(a3)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type
                    >
                >(locals);
        }
    
    
    
    
    
    
    
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
        lambda_actor_gen<
            vector5<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4) const
        {
            typedef
                vector5<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0) , proto::child_c<1>(a1) , proto::child_c<1>(a2) , proto::child_c<1>(a3) , proto::child_c<1>(a4)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type
                    >
                >(locals);
        }
    
    
    
    
    
    
    
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
        lambda_actor_gen<
            vector6<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5) const
        {
            typedef
                vector6<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0) , proto::child_c<1>(a1) , proto::child_c<1>(a2) , proto::child_c<1>(a3) , proto::child_c<1>(a4) , proto::child_c<1>(a5)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type
                    >
                >(locals);
        }
    
    
    
    
    
    
    
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
        lambda_actor_gen<
            vector7<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A6 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6) const
        {
            typedef
                vector7<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A6 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0) , proto::child_c<1>(a1) , proto::child_c<1>(a2) , proto::child_c<1>(a3) , proto::child_c<1>(a4) , proto::child_c<1>(a5) , proto::child_c<1>(a6)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type >::type
                    >
                >(locals);
        }
    
    
    
    
    
    
    
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
        lambda_actor_gen<
            vector8<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A6 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A7 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7) const
        {
            typedef
                vector8<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A6 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A7 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0) , proto::child_c<1>(a1) , proto::child_c<1>(a2) , proto::child_c<1>(a3) , proto::child_c<1>(a4) , proto::child_c<1>(a5) , proto::child_c<1>(a6) , proto::child_c<1>(a7)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type >::type
                    >
                >(locals);
        }
    
    
    
    
    
    
    
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
        lambda_actor_gen<
            vector9<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A6 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A7 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A8 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A8 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7 , A8 const& a8) const
        {
            typedef
                vector9<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A6 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A7 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A8 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0) , proto::child_c<1>(a1) , proto::child_c<1>(a2) , proto::child_c<1>(a3) , proto::child_c<1>(a4) , proto::child_c<1>(a5) , proto::child_c<1>(a6) , proto::child_c<1>(a7) , proto::child_c<1>(a8)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A8 , 0 >::type >::type >::type
                    >
                >(locals);
        }
    
    
    
    
    
    
    
        template <typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
        lambda_actor_gen<
            vector10<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A6 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A7 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A8 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A9 , 1 >::type >::type>
          , detail::map_local_index_to_tuple<typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A8 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A9 , 0 >::type >::type >::type>
        >
        operator()(A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7 , A8 const& a8 , A9 const& a9) const
        {
            typedef
                vector10<typename proto::detail::uncvref< typename proto::result_of::child_c< A0 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A1 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A2 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A3 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A4 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A5 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A6 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A7 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A8 , 1 >::type >::type , typename proto::detail::uncvref< typename proto::result_of::child_c< A9 , 1 >::type >::type>
                locals_type;
            locals_type locals = {proto::child_c<1>(a0) , proto::child_c<1>(a1) , proto::child_c<1>(a2) , proto::child_c<1>(a3) , proto::child_c<1>(a4) , proto::child_c<1>(a5) , proto::child_c<1>(a6) , proto::child_c<1>(a7) , proto::child_c<1>(a8) , proto::child_c<1>(a9)};
            return
                lambda_actor_gen<
                    locals_type
                  , detail::map_local_index_to_tuple<
                        typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A8 , 0 >::type >::type >::type , typename proto::detail::uncvref< typename proto::result_of::value< typename proto::result_of::child_c< A9 , 0 >::type >::type >::type
                    >
                >(locals);
        }
