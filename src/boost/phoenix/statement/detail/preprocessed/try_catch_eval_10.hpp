/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
    
    
    
    
    
    
    
        template <typename Try, typename A0, typename Context>
        typename boost::enable_if<
            proto::matches<
                A0
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); }
        }
        template <typename Try, typename A0, typename Context>
        typename boost::disable_if<
            proto::matches<
                A0
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a0
                    )
                  , ctx);
            }
        }
    
    
    
    
    
    
    
        template <typename Try, typename A0 , typename A1, typename Context>
        typename boost::enable_if<
            proto::matches<
                A1
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); }
        }
        template <typename Try, typename A0 , typename A1, typename Context>
        typename boost::disable_if<
            proto::matches<
                A1
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); }
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a1
                    )
                  , ctx);
            }
        }
    
    
    
    
    
    
    
        template <typename Try, typename A0 , typename A1 , typename A2, typename Context>
        typename boost::enable_if<
            proto::matches<
                A2
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); }
        }
        template <typename Try, typename A0 , typename A1 , typename A2, typename Context>
        typename boost::disable_if<
            proto::matches<
                A2
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); }
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a2
                    )
                  , ctx);
            }
        }
    
    
    
    
    
    
    
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3, typename Context>
        typename boost::enable_if<
            proto::matches<
                A3
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); }
        }
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3, typename Context>
        typename boost::disable_if<
            proto::matches<
                A3
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); }
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a3
                    )
                  , ctx);
            }
        }
    
    
    
    
    
    
    
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4, typename Context>
        typename boost::enable_if<
            proto::matches<
                A4
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); }
        }
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4, typename Context>
        typename boost::disable_if<
            proto::matches<
                A4
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); }
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a4
                    )
                  , ctx);
            }
        }
    
    
    
    
    
    
    
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5, typename Context>
        typename boost::enable_if<
            proto::matches<
                A5
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type::type &e ) { eval_catch_body(a5, e, ctx); }
        }
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5, typename Context>
        typename boost::disable_if<
            proto::matches<
                A5
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); }
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a5
                    )
                  , ctx);
            }
        }
    
    
    
    
    
    
    
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6, typename Context>
        typename boost::enable_if<
            proto::matches<
                A6
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type::type &e ) { eval_catch_body(a5, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type::type &e ) { eval_catch_body(a6, e, ctx); }
        }
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6, typename Context>
        typename boost::disable_if<
            proto::matches<
                A6
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type::type &e ) { eval_catch_body(a5, e, ctx); }
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a6
                    )
                  , ctx);
            }
        }
    
    
    
    
    
    
    
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7, typename Context>
        typename boost::enable_if<
            proto::matches<
                A7
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type::type &e ) { eval_catch_body(a5, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type::type &e ) { eval_catch_body(a6, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type::type &e ) { eval_catch_body(a7, e, ctx); }
        }
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7, typename Context>
        typename boost::disable_if<
            proto::matches<
                A7
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type::type &e ) { eval_catch_body(a5, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type::type &e ) { eval_catch_body(a6, e, ctx); }
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a7
                    )
                  , ctx);
            }
        }
    
    
    
    
    
    
    
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8, typename Context>
        typename boost::enable_if<
            proto::matches<
                A8
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7 , A8 const& a8, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type::type &e ) { eval_catch_body(a5, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type::type &e ) { eval_catch_body(a6, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type::type &e ) { eval_catch_body(a7, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A8 , 0 >::type >::type::type &e ) { eval_catch_body(a8, e, ctx); }
        }
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8, typename Context>
        typename boost::disable_if<
            proto::matches<
                A8
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7 , A8 const& a8, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type::type &e ) { eval_catch_body(a5, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type::type &e ) { eval_catch_body(a6, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type::type &e ) { eval_catch_body(a7, e, ctx); }
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a8
                    )
                  , ctx);
            }
        }
    
    
    
    
    
    
    
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9, typename Context>
        typename boost::enable_if<
            proto::matches<
                A9
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7 , A8 const& a8 , A9 const& a9, Context const & ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type::type &e ) { eval_catch_body(a5, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type::type &e ) { eval_catch_body(a6, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type::type &e ) { eval_catch_body(a7, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A8 , 0 >::type >::type::type &e ) { eval_catch_body(a8, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A9 , 0 >::type >::type::type &e ) { eval_catch_body(a9, e, ctx); }
        }
        template <typename Try, typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9, typename Context>
        typename boost::disable_if<
            proto::matches<
                A9
              , rule::catch_
            >
          , result_type
        >::type
        operator()(Try const & try_, A0 const& a0 , A1 const& a1 , A2 const& a2 , A3 const& a3 , A4 const& a4 , A5 const& a5 , A6 const& a6 , A7 const& a7 , A8 const& a8 , A9 const& a9, Context const & ctx) const
        {
            try
            {
                boost::phoenix::eval(proto::child_c<0>(try_), ctx);
            }
            catch( typename proto::result_of::value< typename proto::result_of::child_c< A0 , 0 >::type >::type::type &e ) { eval_catch_body(a0, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A1 , 0 >::type >::type::type &e ) { eval_catch_body(a1, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A2 , 0 >::type >::type::type &e ) { eval_catch_body(a2, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A3 , 0 >::type >::type::type &e ) { eval_catch_body(a3, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A4 , 0 >::type >::type::type &e ) { eval_catch_body(a4, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A5 , 0 >::type >::type::type &e ) { eval_catch_body(a5, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A6 , 0 >::type >::type::type &e ) { eval_catch_body(a6, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A7 , 0 >::type >::type::type &e ) { eval_catch_body(a7, e, ctx); } catch( typename proto::result_of::value< typename proto::result_of::child_c< A8 , 0 >::type >::type::type &e ) { eval_catch_body(a8, e, ctx); }
            catch(...)
            {
                boost::phoenix::eval(
                    proto::child_c<0>(
                        a9
                    )
                  , ctx);
            }
        }
