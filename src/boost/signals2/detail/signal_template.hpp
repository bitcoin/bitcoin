/*
  Template for Signa1, Signal2, ... classes that support signals
  with 1, 2, ... parameters

  Begin: 2007-01-23
*/
// Copyright Frank Mori Hess 2007-2008
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is included iteratively, and should not be protected from multiple inclusion

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
#define BOOST_SIGNALS2_NUM_ARGS BOOST_PP_ITERATION()
#else
#define BOOST_SIGNALS2_NUM_ARGS 1
#endif

// R, T1, T2, ..., TN, Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex
#define BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION \
  BOOST_SIGNALS2_SIGNATURE_TEMPLATE_INSTANTIATION(BOOST_SIGNALS2_NUM_ARGS), \
  Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex

namespace boost
{
  namespace signals2
  {
    namespace detail
    {
      // helper for bound_extended_slot_function that handles specialization for void return
      template<typename R>
        class BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_INVOKER_N(BOOST_SIGNALS2_NUM_ARGS)
      {
      public:
        typedef R result_type;
        template<typename ExtendedSlotFunction BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
          BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)>
          result_type operator()(ExtendedSlotFunction &func, const connection &conn
            BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
            BOOST_SIGNALS2_FULL_FORWARD_ARGS(BOOST_SIGNALS2_NUM_ARGS)) const
        {
          return func(conn BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
            BOOST_SIGNALS2_FORWARDED_ARGS(BOOST_SIGNALS2_NUM_ARGS));
        }
      };
#ifdef BOOST_NO_VOID_RETURNS
      template<>
        class BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_INVOKER_N(BOOST_SIGNALS2_NUM_ARGS)<void>
      {
      public:
        typedef result_type_wrapper<void>::type result_type;
        template<typename ExtendedSlotFunction BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
          BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)>
          result_type operator()(ExtendedSlotFunction &func, const connection &conn
            BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
            BOOST_SIGNALS2_FULL_FORWARD_ARGS(BOOST_SIGNALS2_NUM_ARGS)) const
        {
          func(conn BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
            BOOST_SIGNALS2_FORWARDED_ARGS(BOOST_SIGNALS2_NUM_ARGS));
          return result_type();
        }
      };
#endif
// wrapper around an signalN::extended_slot_function which binds the
// connection argument so it looks like a normal
// signalN::slot_function

      template<typename ExtendedSlotFunction>
        class BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_N(BOOST_SIGNALS2_NUM_ARGS)
      {
      public:
        typedef typename result_type_wrapper<typename ExtendedSlotFunction::result_type>::type result_type;
        BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_N(BOOST_SIGNALS2_NUM_ARGS)(const ExtendedSlotFunction &fun):
          _fun(fun), _connection(new connection)
        {}
        void set_connection(const connection &conn)
        {
          *_connection = conn;
        }

#if BOOST_SIGNALS2_NUM_ARGS > 0
        template<BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)>
#endif // BOOST_SIGNALS2_NUM_ARGS > 0
          result_type operator()(BOOST_SIGNALS2_FULL_FORWARD_ARGS(BOOST_SIGNALS2_NUM_ARGS))
        {
          return BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_INVOKER_N(BOOST_SIGNALS2_NUM_ARGS)
            <typename ExtendedSlotFunction::result_type>()
            (_fun, *_connection BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
              BOOST_SIGNALS2_FORWARDED_ARGS(BOOST_SIGNALS2_NUM_ARGS));
        }
        // const overload
#if BOOST_SIGNALS2_NUM_ARGS > 0
        template<BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)>
#endif // BOOST_SIGNALS2_NUM_ARGS > 0
          result_type operator()(BOOST_SIGNALS2_FULL_FORWARD_ARGS(BOOST_SIGNALS2_NUM_ARGS)) const
        {
          return BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_INVOKER_N(BOOST_SIGNALS2_NUM_ARGS)
            <typename ExtendedSlotFunction::result_type>()
            (_fun, *_connection BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
              BOOST_SIGNALS2_FORWARDED_ARGS(BOOST_SIGNALS2_NUM_ARGS));
        }
        template<typename T>
          bool operator==(const T &other) const
        {
          return _fun == other;
        }
      private:
        BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_N(BOOST_SIGNALS2_NUM_ARGS)()
        {}

        ExtendedSlotFunction _fun;
        boost::shared_ptr<connection> _connection;
      };

      template<BOOST_SIGNALS2_SIGNAL_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)>
        class BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS);

      template<BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION_DECL(BOOST_SIGNALS2_NUM_ARGS)>
      class BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION
      {
      public:
        typedef SlotFunction slot_function_type;
        // typedef slotN<Signature, SlotFunction> slot_type;
        typedef BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
          <BOOST_SIGNALS2_SIGNATURE_TEMPLATE_INSTANTIATION(BOOST_SIGNALS2_NUM_ARGS),
          slot_function_type> slot_type;
        typedef ExtendedSlotFunction extended_slot_function_type;
        // typedef slotN+1<R, const connection &, T1, T2, ..., TN, extended_slot_function_type> extended_slot_type;
        typedef BOOST_SIGNALS2_EXTENDED_SLOT_TYPE(BOOST_SIGNALS2_NUM_ARGS) extended_slot_type;
        typedef typename nonvoid<typename slot_function_type::result_type>::type nonvoid_slot_result_type;
      private:
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
        class slot_invoker;
#else // BOOST_NO_CXX11_VARIADIC_TEMPLATES
        typedef variadic_slot_invoker<nonvoid_slot_result_type, Args...> slot_invoker;
#endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES
        typedef slot_call_iterator_cache<nonvoid_slot_result_type, slot_invoker> slot_call_iterator_cache_type;
        typedef typename group_key<Group>::type group_key_type;
        typedef shared_ptr<connection_body<group_key_type, slot_type, Mutex> > connection_body_type;
        typedef grouped_list<Group, GroupCompare, connection_body_type> connection_list_type;
        typedef BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_N(BOOST_SIGNALS2_NUM_ARGS)<extended_slot_function_type>
          bound_extended_slot_function_type;
      public:
        typedef Combiner combiner_type;
        typedef typename result_type_wrapper<typename combiner_type::result_type>::type result_type;
        typedef Group group_type;
        typedef GroupCompare group_compare_type;
        typedef typename detail::slot_call_iterator_t<slot_invoker,
          typename connection_list_type::iterator, connection_body<group_key_type, slot_type, Mutex> > slot_call_iterator;

        BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)(const combiner_type &combiner_arg,
          const group_compare_type &group_compare):
          _shared_state(new invocation_state(connection_list_type(group_compare), combiner_arg)),
          _garbage_collector_it(_shared_state->connection_bodies().end()),
          _mutex(new mutex_type())
        {}
        // connect slot
        connection connect(const slot_type &slot, connect_position position = at_back)
        {
          garbage_collecting_lock<mutex_type> lock(*_mutex);
          return nolock_connect(lock, slot, position);
        }
        connection connect(const group_type &group,
          const slot_type &slot, connect_position position = at_back)
        {
          garbage_collecting_lock<mutex_type> lock(*_mutex);
          return nolock_connect(lock, group, slot, position);
        }
        // connect extended slot
        connection connect_extended(const extended_slot_type &ext_slot, connect_position position = at_back)
        {
          garbage_collecting_lock<mutex_type> lock(*_mutex);
          bound_extended_slot_function_type bound_slot(ext_slot.slot_function());
          slot_type slot = replace_slot_function<slot_type>(ext_slot, bound_slot);
          connection conn = nolock_connect(lock, slot, position);
          bound_slot.set_connection(conn);
          return conn;
        }
        connection connect_extended(const group_type &group,
          const extended_slot_type &ext_slot, connect_position position = at_back)
        {
          garbage_collecting_lock<Mutex> lock(*_mutex);
          bound_extended_slot_function_type bound_slot(ext_slot.slot_function());
          slot_type slot = replace_slot_function<slot_type>(ext_slot, bound_slot);
          connection conn = nolock_connect(lock, group, slot, position);
          bound_slot.set_connection(conn);
          return conn;
        }
        // disconnect slot(s)
        void disconnect_all_slots()
        {
          shared_ptr<invocation_state> local_state =
            get_readable_state();
          typename connection_list_type::iterator it;
          for(it = local_state->connection_bodies().begin();
            it != local_state->connection_bodies().end(); ++it)
          {
            (*it)->disconnect();
          }
        }
        void disconnect(const group_type &group)
        {
          shared_ptr<invocation_state> local_state =
            get_readable_state();
          group_key_type group_key(grouped_slots, group);
          typename connection_list_type::iterator it;
          typename connection_list_type::iterator end_it =
            local_state->connection_bodies().upper_bound(group_key);
          for(it = local_state->connection_bodies().lower_bound(group_key);
            it != end_it; ++it)
          {
            (*it)->disconnect();
          }
        }
        template <typename T>
        void disconnect(const T &slot)
        {
          typedef mpl::bool_<(is_convertible<T, group_type>::value)> is_group;
          do_disconnect(slot, is_group());
        }
        // emit signal
        result_type operator ()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS))
        {
          shared_ptr<invocation_state> local_state;
          typename connection_list_type::iterator it;
          {
            garbage_collecting_lock<mutex_type> list_lock(*_mutex);
            // only clean up if it is safe to do so
            if(_shared_state.unique())
              nolock_cleanup_connections(list_lock, false, 1);
            /* Make a local copy of _shared_state while holding mutex, so we are
            thread safe against the combiner or connection list getting modified
            during invocation. */
            local_state = _shared_state;
          }
          slot_invoker invoker = slot_invoker(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
          slot_call_iterator_cache_type cache(invoker);
          invocation_janitor janitor(cache, *this, &local_state->connection_bodies());
          return detail::combiner_invoker<typename combiner_type::result_type>()
            (
              local_state->combiner(),
              slot_call_iterator(local_state->connection_bodies().begin(), local_state->connection_bodies().end(), cache),
              slot_call_iterator(local_state->connection_bodies().end(), local_state->connection_bodies().end(), cache)
            );
        }
        result_type operator ()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS)) const
        {
          shared_ptr<invocation_state> local_state;
          typename connection_list_type::iterator it;
          {
            garbage_collecting_lock<mutex_type> list_lock(*_mutex);
            // only clean up if it is safe to do so
            if(_shared_state.unique())
              nolock_cleanup_connections(list_lock, false, 1);
            /* Make a local copy of _shared_state while holding mutex, so we are
            thread safe against the combiner or connection list getting modified
            during invocation. */
            local_state = _shared_state;
          }
          slot_invoker invoker = slot_invoker(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
          slot_call_iterator_cache_type cache(invoker);
          invocation_janitor janitor(cache, *this, &local_state->connection_bodies());
          return detail::combiner_invoker<typename combiner_type::result_type>()
            (
              local_state->combiner(),
              slot_call_iterator(local_state->connection_bodies().begin(), local_state->connection_bodies().end(), cache),
              slot_call_iterator(local_state->connection_bodies().end(), local_state->connection_bodies().end(), cache)
            );
        }
        std::size_t num_slots() const
        {
          shared_ptr<invocation_state> local_state =
            get_readable_state();
          typename connection_list_type::iterator it;
          std::size_t count = 0;
          for(it = local_state->connection_bodies().begin();
            it != local_state->connection_bodies().end(); ++it)
          {
            if((*it)->connected()) ++count;
          }
          return count;
        }
        bool empty() const
        {
          shared_ptr<invocation_state> local_state =
            get_readable_state();
          typename connection_list_type::iterator it;
          for(it = local_state->connection_bodies().begin();
            it != local_state->connection_bodies().end(); ++it)
          {
            if((*it)->connected()) return false;
          }
          return true;
        }
        combiner_type combiner() const
        {
          unique_lock<mutex_type> lock(*_mutex);
          return _shared_state->combiner();
        }
        void set_combiner(const combiner_type &combiner_arg)
        {
          unique_lock<mutex_type> lock(*_mutex);
          if(_shared_state.unique())
            _shared_state->combiner() = combiner_arg;
          else
            _shared_state.reset(new invocation_state(*_shared_state, combiner_arg));
        }
      private:
        typedef Mutex mutex_type;

        // slot_invoker is passed to slot_call_iterator_t to run slots
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
        class slot_invoker
        {
        public:
          typedef nonvoid_slot_result_type result_type;
// typename add_reference<Tn>::type
#define BOOST_SIGNALS2_ADD_REF_TYPE(z, n, data) \
  typename add_reference<BOOST_PP_CAT(T, BOOST_PP_INC(n))>::type
// typename add_reference<Tn>::type argn
#define BOOST_SIGNALS2_ADD_REF_ARG(z, n, data) \
  BOOST_SIGNALS2_ADD_REF_TYPE(~, n, ~) \
  BOOST_SIGNALS2_SIGNATURE_ARG_NAME(~, n, ~)
// typename add_reference<T1>::type arg1, typename add_reference<T2>::type arg2, ..., typename add_reference<Tn>::type argn
#define BOOST_SIGNALS2_ADD_REF_ARGS(arity) \
  BOOST_PP_ENUM(arity, BOOST_SIGNALS2_ADD_REF_ARG, ~)
          slot_invoker(BOOST_SIGNALS2_ADD_REF_ARGS(BOOST_SIGNALS2_NUM_ARGS)) BOOST_PP_EXPR_IF(BOOST_SIGNALS2_NUM_ARGS, :)
#undef BOOST_SIGNALS2_ADD_REF_ARGS

// m_argn
#define BOOST_SIGNALS2_M_ARG_NAME(z, n, data) BOOST_PP_CAT(m_arg, BOOST_PP_INC(n))
// m_argn ( argn )
#define BOOST_SIGNALS2_MISC_STATEMENT(z, n, data) \
  BOOST_SIGNALS2_M_ARG_NAME(~, n, ~) ( BOOST_SIGNALS2_SIGNATURE_ARG_NAME(~, n, ~) )
// m_arg1(arg1), m_arg2(arg2), ..., m_argn(argn)
            BOOST_PP_ENUM(BOOST_SIGNALS2_NUM_ARGS, BOOST_SIGNALS2_MISC_STATEMENT, ~)
#undef BOOST_SIGNALS2_MISC_STATEMENT
          {}
          result_type operator ()(const connection_body_type &connectionBody) const
          {
            return m_invoke<typename slot_type::result_type>(connectionBody);
          }
        private:
          // declare assignment operator private since this class might have reference or const members
          slot_invoker & operator=(const slot_invoker &);

#define BOOST_SIGNALS2_ADD_REF_M_ARG_STATEMENT(z, n, data) \
  BOOST_SIGNALS2_ADD_REF_TYPE(~, n, ~) BOOST_SIGNALS2_M_ARG_NAME(~, n, ~) ;
          BOOST_PP_REPEAT(BOOST_SIGNALS2_NUM_ARGS, BOOST_SIGNALS2_ADD_REF_M_ARG_STATEMENT, ~)
#undef BOOST_SIGNALS2_ADD_REF_M_ARG_STATEMENT
#undef BOOST_SIGNALS2_ADD_REF_ARG
#undef BOOST_SIGNALS2_ADD_REF_TYPE

// m_arg1, m_arg2, ..., m_argn
#define BOOST_SIGNALS2_M_ARG_NAMES(arity) BOOST_PP_ENUM(arity, BOOST_SIGNALS2_M_ARG_NAME, ~)
          template<typename SlotResultType>
          result_type m_invoke(const connection_body_type &connectionBody,
            typename boost::enable_if<boost::is_void<SlotResultType> >::type * = 0) const
          {
            connectionBody->slot().slot_function()(BOOST_SIGNALS2_M_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
            return void_type();
          }
          template<typename SlotResultType>
          result_type m_invoke(const connection_body_type &connectionBody, 
            typename boost::disable_if<boost::is_void<SlotResultType> >::type * = 0) const
          {
            return connectionBody->slot().slot_function()(BOOST_SIGNALS2_M_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
          }
        };
#undef BOOST_SIGNALS2_M_ARG_NAMES
#undef BOOST_SIGNALS2_M_ARG_NAME

#endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES
        // a struct used to optimize (minimize) the number of shared_ptrs that need to be created
        // inside operator()
        class invocation_state
        {
        public:
          invocation_state(const connection_list_type &connections_in,
            const combiner_type &combiner_in): _connection_bodies(new connection_list_type(connections_in)),
            _combiner(new combiner_type(combiner_in))
          {}
          invocation_state(const invocation_state &other, const connection_list_type &connections_in):
            _connection_bodies(new connection_list_type(connections_in)),
            _combiner(other._combiner)
          {}
          invocation_state(const invocation_state &other, const combiner_type &combiner_in):
            _connection_bodies(other._connection_bodies),
            _combiner(new combiner_type(combiner_in))
          {}
          connection_list_type & connection_bodies() { return *_connection_bodies; }
          const connection_list_type & connection_bodies() const { return *_connection_bodies; }
          combiner_type & combiner() { return *_combiner; }
          const combiner_type & combiner() const { return *_combiner; }
        private:
          invocation_state(const invocation_state &);

          shared_ptr<connection_list_type> _connection_bodies;
          shared_ptr<combiner_type> _combiner;
        };
        // Destructor of invocation_janitor does some cleanup when a signal invocation completes.
        // Code can't be put directly in signal's operator() due to complications from void return types.
        class invocation_janitor: noncopyable
        {
        public:
          typedef BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) signal_type;
          invocation_janitor
          (
            const slot_call_iterator_cache_type &cache,
            const signal_type &sig,
            const connection_list_type *connection_bodies
          ):_cache(cache), _sig(sig), _connection_bodies(connection_bodies)
          {}
          ~invocation_janitor()
          {
            // force a full cleanup of disconnected slots if there are too many
            if(_cache.disconnected_slot_count > _cache.connected_slot_count)
            {
              _sig.force_cleanup_connections(_connection_bodies);
            }
          }
        private:
          const slot_call_iterator_cache_type &_cache;
          const signal_type &_sig;
          const connection_list_type *_connection_bodies;
        };

        // clean up disconnected connections
        void nolock_cleanup_connections_from(garbage_collecting_lock<mutex_type> &lock,
          bool grab_tracked,
          const typename connection_list_type::iterator &begin, unsigned count = 0) const
        {
          BOOST_ASSERT(_shared_state.unique());
          typename connection_list_type::iterator it;
          unsigned i;
          for(it = begin, i = 0;
            it != _shared_state->connection_bodies().end() && (count == 0 || i < count);
            ++i)
          {
            bool connected;
            if(grab_tracked)
              (*it)->disconnect_expired_slot(lock);
            connected = (*it)->nolock_nograb_connected();
            if(connected == false)
            {
              it = _shared_state->connection_bodies().erase((*it)->group_key(), it);
            }else
            {
              ++it;
            }
          }
          _garbage_collector_it = it;
        }
        // clean up a few connections in constant time
        void nolock_cleanup_connections(garbage_collecting_lock<mutex_type> &lock,
          bool grab_tracked, unsigned count) const
        {
          BOOST_ASSERT(_shared_state.unique());
          typename connection_list_type::iterator begin;
          if(_garbage_collector_it == _shared_state->connection_bodies().end())
          {
            begin = _shared_state->connection_bodies().begin();
          }else
          {
            begin = _garbage_collector_it;
          }
          nolock_cleanup_connections_from(lock, grab_tracked, begin, count);
        }
        /* Make a new copy of the slot list if it is currently being read somewhere else
        */
        void nolock_force_unique_connection_list(garbage_collecting_lock<mutex_type> &lock)
        {
          if(_shared_state.unique() == false)
          {
            _shared_state.reset(new invocation_state(*_shared_state, _shared_state->connection_bodies()));
            nolock_cleanup_connections_from(lock, true, _shared_state->connection_bodies().begin());
          }else
          {
            /* We need to try and check more than just 1 connection here to avoid corner
            cases where certain repeated connect/disconnect patterns cause the slot
            list to grow without limit. */
            nolock_cleanup_connections(lock, true, 2);
          }
        }
        // force a full cleanup of the connection list
        void force_cleanup_connections(const connection_list_type *connection_bodies) const
        {
          garbage_collecting_lock<mutex_type> list_lock(*_mutex);
          // if the connection list passed in as a parameter is no longer in use,
          // we don't need to do any cleanup.
          if(&_shared_state->connection_bodies() != connection_bodies)
          {
            return;
          }
          if(_shared_state.unique() == false)
          {
            _shared_state.reset(new invocation_state(*_shared_state, _shared_state->connection_bodies()));
          }
          nolock_cleanup_connections_from(list_lock, false, _shared_state->connection_bodies().begin());
        }
        shared_ptr<invocation_state> get_readable_state() const
        {
          unique_lock<mutex_type> list_lock(*_mutex);
          return _shared_state;
        }
        connection_body_type create_new_connection(garbage_collecting_lock<mutex_type> &lock,
          const slot_type &slot)
        {
          nolock_force_unique_connection_list(lock);
          return connection_body_type(new connection_body<group_key_type, slot_type, Mutex>(slot, _mutex));
        }
        void do_disconnect(const group_type &group, mpl::bool_<true> /* is_group */)
        {
          disconnect(group);
        }
        template<typename T>
        void do_disconnect(const T &slot, mpl::bool_<false> /* is_group */)
        {
          shared_ptr<invocation_state> local_state =
            get_readable_state();
          typename connection_list_type::iterator it;
          for(it = local_state->connection_bodies().begin();
            it != local_state->connection_bodies().end(); ++it)
          {
            garbage_collecting_lock<connection_body_base> lock(**it);
            if((*it)->nolock_nograb_connected() == false) continue;
            if((*it)->slot().slot_function() == slot)
            {
              (*it)->nolock_disconnect(lock);
            }else
            {
              // check for wrapped extended slot
              bound_extended_slot_function_type *fp;
              fp = (*it)->slot().slot_function().template target<bound_extended_slot_function_type>();
              if(fp && *fp == slot)
              {
                (*it)->nolock_disconnect(lock);
              }
            }
          }
        }
        // connect slot
        connection nolock_connect(garbage_collecting_lock<mutex_type> &lock,
          const slot_type &slot, connect_position position)
        {
          connection_body_type newConnectionBody =
            create_new_connection(lock, slot);
          group_key_type group_key;
          if(position == at_back)
          {
            group_key.first = back_ungrouped_slots;
            _shared_state->connection_bodies().push_back(group_key, newConnectionBody);
          }else
          {
            group_key.first = front_ungrouped_slots;
            _shared_state->connection_bodies().push_front(group_key, newConnectionBody);
          }
          newConnectionBody->set_group_key(group_key);
          return connection(newConnectionBody);
        }
        connection nolock_connect(garbage_collecting_lock<mutex_type> &lock,
          const group_type &group,
          const slot_type &slot, connect_position position)
        {
          connection_body_type newConnectionBody =
            create_new_connection(lock, slot);
          // update map to first connection body in group if needed
          group_key_type group_key(grouped_slots, group);
          newConnectionBody->set_group_key(group_key);
          if(position == at_back)
          {
            _shared_state->connection_bodies().push_back(group_key, newConnectionBody);
          }else  // at_front
          {
            _shared_state->connection_bodies().push_front(group_key, newConnectionBody);
          }
          return connection(newConnectionBody);
        }

        // _shared_state is mutable so we can do force_cleanup_connections during a const invocation
        mutable shared_ptr<invocation_state> _shared_state;
        mutable typename connection_list_type::iterator _garbage_collector_it;
        // connection list mutex must never be locked when attempting a blocking lock on a slot,
        // or you could deadlock.
        const boost::shared_ptr<mutex_type> _mutex;
      };

      template<BOOST_SIGNALS2_SIGNAL_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)>
        class BOOST_SIGNALS2_WEAK_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS);
    }

    template<BOOST_SIGNALS2_SIGNAL_TEMPLATE_DEFAULTED_DECL(BOOST_SIGNALS2_NUM_ARGS)>
      class BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS);

    template<BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION_DECL(BOOST_SIGNALS2_NUM_ARGS)>
    class BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
      BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION: public signal_base,
      public detail::BOOST_SIGNALS2_STD_FUNCTIONAL_BASE
    {
      typedef detail::BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
        <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION> impl_class;
    public:
      typedef detail::BOOST_SIGNALS2_WEAK_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
        <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION> weak_signal_type;
      friend class detail::BOOST_SIGNALS2_WEAK_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
        <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION>;

      typedef SlotFunction slot_function_type;
      // typedef slotN<Signature, SlotFunction> slot_type;
      typedef typename impl_class::slot_type slot_type;
      typedef typename impl_class::extended_slot_function_type extended_slot_function_type;
      typedef typename impl_class::extended_slot_type extended_slot_type;
      typedef typename slot_function_type::result_type slot_result_type;
      typedef Combiner combiner_type;
      typedef typename impl_class::result_type result_type;
      typedef Group group_type;
      typedef GroupCompare group_compare_type;
      typedef typename impl_class::slot_call_iterator
        slot_call_iterator;
      typedef typename mpl::identity<BOOST_SIGNALS2_SIGNATURE_FUNCTION_TYPE(BOOST_SIGNALS2_NUM_ARGS)>::type signature_type;

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES

// typedef Tn argn_type;
#define BOOST_SIGNALS2_MISC_STATEMENT(z, n, data) \
  typedef BOOST_PP_CAT(T, BOOST_PP_INC(n)) BOOST_PP_CAT(BOOST_PP_CAT(arg, BOOST_PP_INC(n)), _type);
      BOOST_PP_REPEAT(BOOST_SIGNALS2_NUM_ARGS, BOOST_SIGNALS2_MISC_STATEMENT, ~)
#undef BOOST_SIGNALS2_MISC_STATEMENT
#if BOOST_SIGNALS2_NUM_ARGS == 1
      typedef arg1_type argument_type;
#elif BOOST_SIGNALS2_NUM_ARGS == 2
      typedef arg1_type first_argument_type;
      typedef arg2_type second_argument_type;
#endif

      template<unsigned n> class arg : public
        detail::BOOST_SIGNALS2_PREPROCESSED_ARG_N_TYPE_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
        <n BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
        BOOST_SIGNALS2_ARGS_TEMPLATE_INSTANTIATION(BOOST_SIGNALS2_NUM_ARGS)>
      {};

      BOOST_STATIC_CONSTANT(int, arity = BOOST_SIGNALS2_NUM_ARGS);

#else // BOOST_NO_CXX11_VARIADIC_TEMPLATES

      template<unsigned n> class arg
      {
      public:
        typedef typename detail::variadic_arg_type<n, Args...>::type type;
      };
      BOOST_STATIC_CONSTANT(int, arity = sizeof...(Args));

#endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES

      BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)(const combiner_type &combiner_arg = combiner_type(),
        const group_compare_type &group_compare = group_compare_type()):
        _pimpl(new impl_class(combiner_arg, group_compare))
      {}
      virtual ~BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)()
      {
      }
      
      //move support
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)(
        BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) && other) BOOST_NOEXCEPT
      {
        using std::swap;
        swap(_pimpl, other._pimpl);
      }
      
      BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) & 
        operator=(BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) && rhs) BOOST_NOEXCEPT
      {
        if(this == &rhs)
        {
          return *this;
        }
        _pimpl.reset();
        using std::swap;
        swap(_pimpl, rhs._pimpl);
        return *this;
      }
#endif // !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      
      connection connect(const slot_type &slot, connect_position position = at_back)
      {
        return (*_pimpl).connect(slot, position);
      }
      connection connect(const group_type &group,
        const slot_type &slot, connect_position position = at_back)
      {
        return (*_pimpl).connect(group, slot, position);
      }
      connection connect_extended(const extended_slot_type &slot, connect_position position = at_back)
      {
        return (*_pimpl).connect_extended(slot, position);
      }
      connection connect_extended(const group_type &group,
        const extended_slot_type &slot, connect_position position = at_back)
      {
        return (*_pimpl).connect_extended(group, slot, position);
      }
      void disconnect_all_slots()
      {
        (*_pimpl).disconnect_all_slots();
      }
      void disconnect(const group_type &group)
      {
        (*_pimpl).disconnect(group);
      }
      template <typename T>
      void disconnect(const T &slot)
      {
        (*_pimpl).disconnect(slot);
      }
      result_type operator ()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS))
      {
        return (*_pimpl)(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
      }
      result_type operator ()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS)) const
      {
        return (*_pimpl)(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
      }
      std::size_t num_slots() const
      {
        return (*_pimpl).num_slots();
      }
      bool empty() const
      {
        return (*_pimpl).empty();
      }
      combiner_type combiner() const
      {
        return (*_pimpl).combiner();
      }
      void set_combiner(const combiner_type &combiner_arg)
      {
        return (*_pimpl).set_combiner(combiner_arg);
      }
      void swap(BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) & other) BOOST_NOEXCEPT
      {
        using std::swap;
        swap(_pimpl, other._pimpl);
      }
    protected:
      virtual shared_ptr<void> lock_pimpl() const
      {
        return _pimpl;
      }
    private:
      shared_ptr<impl_class>
        _pimpl;
    };

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    // free swap function for signalN classes, findable by ADL
    template<BOOST_SIGNALS2_SIGNAL_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)>
      void swap(
        BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION> &sig1,
        BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION> &sig2 ) BOOST_NOEXCEPT
    {
      sig1.swap(sig2);
    }
#endif

    namespace detail
    {
      // wrapper class for storing other signals as slots with automatic lifetime tracking
      template<BOOST_SIGNALS2_SIGNAL_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)>
        class BOOST_SIGNALS2_WEAK_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS);

      template<BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION_DECL(BOOST_SIGNALS2_NUM_ARGS)>
        class BOOST_SIGNALS2_WEAK_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
        BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION
      {
      public:
        typedef typename BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
          <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION>::result_type
          result_type;

        BOOST_SIGNALS2_WEAK_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
          (const BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
          <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION>
          &signal):
          _weak_pimpl(signal._pimpl)
        {}
        result_type operator ()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS))
        {
          shared_ptr<detail::BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
            <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION> >
            shared_pimpl(_weak_pimpl.lock());
          return (*shared_pimpl)(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
        }
        result_type operator ()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS)) const
        {
          shared_ptr<detail::BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
            <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION> >
            shared_pimpl(_weak_pimpl.lock());
          return (*shared_pimpl)(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
        }
      private:
        boost::weak_ptr<detail::BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
          <BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION> > _weak_pimpl;
      };

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
      template<int arity, typename Signature>
        class extended_signature: public variadic_extended_signature<Signature>
      {};
#else // BOOST_NO_CXX11_VARIADIC_TEMPLATES
      template<int arity, typename Signature>
        class extended_signature;
      // partial template specialization
      template<typename Signature>
        class extended_signature<BOOST_SIGNALS2_NUM_ARGS, Signature>
      {
      public:
// typename function_traits<Signature>::result_type (
// const boost::signals2::connection &,
// typename function_traits<Signature>::arg1_type,
// typename function_traits<Signature>::arg2_type,
// ...,
// typename function_traits<Signature>::argn_type)
#define BOOST_SIGNALS2_EXT_SIGNATURE(arity, Signature) \
  typename function_traits<Signature>::result_type ( \
  const boost::signals2::connection & BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS) \
  BOOST_PP_ENUM(arity, BOOST_SIGNALS2_SIGNATURE_TO_ARGN_TYPE, Signature) )
        typedef function<BOOST_SIGNALS2_EXT_SIGNATURE(BOOST_SIGNALS2_NUM_ARGS, Signature)> function_type;
#undef BOOST_SIGNALS2_EXT_SIGNATURE
      };

      template<unsigned arity, typename Signature, typename Combiner,
        typename Group, typename GroupCompare, typename SlotFunction,
        typename ExtendedSlotFunction, typename Mutex>
      class signalN;
      // partial template specialization
      template<typename Signature, typename Combiner, typename Group,
        typename GroupCompare, typename SlotFunction,
        typename ExtendedSlotFunction, typename Mutex>
      class signalN<BOOST_SIGNALS2_NUM_ARGS, Signature, Combiner, Group,
        GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>
      {
      public:
        typedef BOOST_SIGNALS2_SIGNAL_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)<
          BOOST_SIGNALS2_PORTABLE_SIGNATURE(BOOST_SIGNALS2_NUM_ARGS, Signature),
          Combiner, Group,
          GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex> type;
      };

#endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES

    } // namespace detail
  } // namespace signals2
} // namespace boost

#undef BOOST_SIGNALS2_NUM_ARGS
#undef BOOST_SIGNALS2_SIGNAL_TEMPLATE_INSTANTIATION
