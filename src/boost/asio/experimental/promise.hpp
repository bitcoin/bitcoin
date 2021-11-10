//
// experimental/promise.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2021 Klemens D. Morgenstern
//                    (klemens dot morgenstern at gmx dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_PROMISE_HPP
#define BOOST_ASIO_EXPERIMENTAL_PROMISE_HPP

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/experimental/detail/completion_handler_erasure.hpp>
#include <boost/asio/experimental/impl/promise.hpp>
#include <boost/asio/post.hpp>

#include <variant>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace experimental {

template <typename Executor = any_io_executor>
struct use_promise_t {};

constexpr use_promise_t<> use_promise;

template <typename T>
struct is_promise : std::false_type {};

template <typename ... Ts>
struct is_promise<promise<Ts...>> : std::true_type {};

template <typename T>
constexpr bool is_promise_v = is_promise<T>::value;

template <typename T>
concept is_promise_c = is_promise_v<std::remove_reference_t<T>>;

template <typename ... Ts>
struct promise_value_type
{
  using type = std::tuple<Ts...>;
};

template <typename T>
struct promise_value_type<T>
{
  using type = T;
};

template <>
struct promise_value_type<>
{
  using type = std::monostate;
};

#if defined(GENERATING_DOCUMENTATION)
/// The primary template is not defined.
template<typename Signature = void(), typename Executor = any_io_executor>
struct promise
{
};
#endif // defined(GENERATING_DOCUMENTATION)

template <typename ... Ts, typename Executor>
struct promise<void(Ts...), Executor>
{
  using value_type = typename promise_value_type<Ts...>::type;
  using tuple_type = std::tuple<Ts...>;
  using executor_type = Executor;

  executor_type get_executor() const
  {
    if (impl_)
      return impl_->executor;
    else
      return {};
  }

  void cancel(cancellation_type level = cancellation_type::all)
  {
    if (impl_ && !impl_->done)
    {
      boost::asio::dispatch(impl_->executor,
          [level, impl = impl_]{impl->cancel.emit(level);});
    }
  }

  bool complete() const noexcept
  {
    return impl_ && impl_->done;
  }

  template <typename CompletionToken>
  auto async_wait(CompletionToken&& token)
  {
    assert(impl_);

    return async_initiate<CompletionToken, void(Ts...)>(
        initiate_async_wait{impl_}, token);
  }

  promise() = delete;
  promise(const promise& ) = delete;
  promise(promise&& ) noexcept = default;

  ~promise() { cancel(); }

  template <execution::executor Executor1, is_promise_c ... Ps>
  static auto race(Executor1 exec, Ps ... ps)
    -> promise<void(std::variant<typename Ps::value_type...>), Executor1>
  {
    using var_t = std::variant<typename Ps::value_type...>;
    using pi = detail::promise_impl<void(var_t), Executor1>;

    struct impl_t : pi
    {
      impl_t(Executor1 exec, Ps&& ... ps)
        : pi(std::move(exec)),
          tup(std::move(ps)...)
      {
        this->slot.template emplace<cancel_handler>(this);
      }

      struct cancel_handler
      {
        impl_t* self;

        cancel_handler(impl_t* self)
          : self(self)
        {
        }

        void operator()(cancellation_type ct)
        {
          [ct, s=self]<std::size_t... Idx>(std::index_sequence<Idx...>)
          {
            (std::get<Idx>(s->tup).cancel(ct), ... );
          }(std::make_index_sequence<sizeof...(Ps)>{});
        }
      };

      std::tuple<std::remove_reference_t<Ps>...> tup;
      cancellation_slot slot{this->cancel.slot()};
    };

    auto impl = std::allocate_shared<impl_t>(
        get_associated_allocator(exec), exec, std::move(ps)...);

    impl->executor = exec;

    [impl, exec]<std::size_t... Idx>(std::index_sequence<Idx...>)
    {
      auto step =
        [&]<std::size_t I>(std::integral_constant<std::size_t, I>)
        {
          return [impl]<typename... Args > (Args&& ... args)
          {
            if (impl->done)
              return;
            impl->result = var_t(std::in_place_index<I>,
                std::forward<Args>(args)...);
            impl->done = true;
            if (auto f = std::exchange(impl->completion, nullptr); !!f)
              std::apply(std::move(f), std::move(*impl->result));

            auto cancel =
              [&]<std::size_t Id>(std::integral_constant<std::size_t, Id>)
              {
                if constexpr (I != Id)
                  get<I>(impl->tup).cancel();
              };

            (cancel(std::integral_constant<std::size_t, Idx>{}), ...);
          };
        };

      (
        std::get<Idx>(impl->tup).async_wait(
          bind_executor(exec,
            step(std::integral_constant<std::size_t, Idx>{}))),
        ...
      );
    }(std::make_index_sequence<sizeof...(Ps)>{});

    return {impl};
  }

  template <execution::executor Executor1, is_promise_c ... Ps>
  static auto all(Executor1 exec, Ps ... ps)
    -> promise<void(typename Ps::value_type...), Executor1>
  {
    using pi = detail::promise_impl<
        void(typename Ps::value_type...), Executor1>;

    struct impl_t : pi
    {
      impl_t(Executor1 exec, Ps&& ... ps)
        : pi(std::move(exec)),
          tup(std::move(ps)...)
      {
        this->slot.template emplace<cancel_handler>(this);
      }

      struct cancel_handler
      {
        impl_t* self;

        cancel_handler(impl_t* self)
          : self(self)
        {
        }

        void operator()(cancellation_type level)
        {
          [level, s=self]<std::size_t... Idx>(std::index_sequence<Idx...>)
          {
            (std::get<Idx>(s->tup).cancel(level), ... );
          }(std::make_index_sequence<sizeof...(Ps)>{});
        }
      };

      std::tuple<std::remove_reference_t<Ps>...> tup;
      std::tuple<std::optional<typename Ps::value_type>...> partial_result;
      cancellation_slot slot{this->cancel.slot()};
    };

    auto impl = std::allocate_shared<impl_t>(
        get_associated_allocator(exec), exec, std::move(ps)...);
    impl->executor = exec;

    [impl, exec]<std::size_t... Idx>(std::index_sequence<Idx...>)
    {
      auto step =
        [&]<std::size_t I>(std::integral_constant<std::size_t, I>)
        {
          return [impl]<typename... Args>(Args&& ... args)
          {
            std::get<I>(impl->partial_result).emplace(
                std::forward<Args>(args)...);
            if ((std::get<Idx>(impl->partial_result) && ...)) // we're done.
            {
              impl->result = {*std::get<Idx>(impl->partial_result)...};

              impl->done = true;
              if (auto f = std::exchange(impl->completion, nullptr); !!f)
                std::apply(std::move(f), std::move(*impl->result));
            }
          };
        };

      (
        std::get<Idx>(impl->tup).async_wait(
          bind_executor(exec,
            step(std::integral_constant<std::size_t, Idx>{}))),
        ...
      );
    }(std::make_index_sequence<sizeof...(Ps)>{});

    return {impl};
  }

  template <is_promise_c ... Ps>
  static auto race(Ps  ... ps)
  {
    auto exec = get<0>(std::tie(ps...)).get_executor();
    return race(std::move(exec), std::move(ps)...);
  }

  template <is_promise_c ... Ps>
  static auto all(Ps ... ps)
  {
    auto exec = get<0>(std::tie(ps...)).get_executor();
    return all(std::move(exec), std::move(ps)...);
  }

  template <execution::executor Executor1, typename Range>
#if !defined(GENERATING_DOCUMENTATION)
    requires requires (Range r)
    {
      {*std::begin(r)} -> is_promise_c;
      {*std::  end(r)} -> is_promise_c;
    }
#endif // !defined(GENERATING_DOCUMENTATION)
  static auto race(Executor1 exec, Range range)
  {
    using var_t = typename std::decay_t<
        decltype(*std::begin(range))>::value_type;
    using signature_type = std::conditional_t<
        std::is_same_v<var_t, std::monostate>,
        void(std::size_t),
        void(std::size_t, var_t)>;
    using pi = detail::promise_impl<signature_type, Executor1>;
    using promise_t = promise<signature_type, Executor1>;

    struct impl_t : pi
    {
      impl_t(Executor1 exec, Range&& range)
        : pi(std::move(exec)),
          range(std::move(range))
      {
        this->slot.template emplace<cancel_handler>(this);
      }

      struct cancel_handler
      {
        impl_t* self;

        cancel_handler(impl_t* self)
          : self(self)
        {
        }

        void operator()(boost::asio::cancellation_type ct)
        {
          for (auto& r : self->range)
            r.cancel(ct);
        }
      };

      Range range;
      cancellation_slot slot{this->cancel.slot()};
    };

    const auto size = std::distance(std::begin(range), std::end(range));
    auto impl =  std::allocate_shared<impl_t>(
        get_associated_allocator(exec), exec, std::move(range));
    impl->executor = exec;

    if (size == 0u)
    {
      if constexpr (std::is_same_v<var_t, std::monostate>)
        impl->result = {-1};
      else
        impl->result = {-1, var_t{}};

      impl->done = true;
      if (auto f = std::exchange(impl->completion, nullptr); !!f)
      {
        boost::asio::post(exec,
            [impl, f = std::move(f)]() mutable
            {
              std::apply(std::move(f), std::move(*impl->result));
            });
      }
      return promise_t{impl};
    }
    auto idx = 0u;
    for (auto& val : impl->range)
    {
      val.async_wait(
          bind_executor(exec,
            [idx, impl]<typename... Args>(Args&&... args)
            {
              if (impl->done)
                return;
              if constexpr (std::is_same_v<var_t, std::monostate>)
                impl->result = idx;
              else
                impl->result = std::make_tuple(idx,
                    var_t(std::forward<Args>(args)...));
              impl->done = true;
              if (auto f = std::exchange(impl->completion, nullptr); !!f)
                std::apply(std::move(f), std::move(*impl->result));

              auto jdx = 0u;

              for (auto &tc : impl->range)
                if (jdx++ != idx)
                  tc.cancel();
            }));
      idx++;
    }
    return promise_t{impl};
  }


  template <execution::executor Executor1, typename Range>
#if !defined(GENERATING_DOCUMENTATION)
    requires requires (Range r)
    {
      {*std::begin(r)} -> is_promise_c;
      {*std::  end(r)} -> is_promise_c;
    }
#endif // !defined(GENERATING_DOCUMENTATION)
  static auto all(Executor1 exec, Range  range)
    -> promise<
        void(
          std::vector<
            typename std::decay_t<
              decltype(*std::begin(range))
            >::value_type
          >
        ), Executor1>
  {
    using var_t = typename std::decay_t<
      decltype(*std::begin(range))>::value_type;
    using pi = detail::promise_impl<void(std::vector<var_t>), Executor1>;

    struct impl_t : pi
    {
      impl_t(Executor1 exec, Range&& range)
        : pi(std::move(exec)),
          range(std::move(range))
      {
        this->slot.template emplace<cancel_handler>(this);
      }

      struct cancel_handler
      {
        impl_t* self;

        cancel_handler(impl_t* self)
          : self(self)
        {
        }

        void operator()(cancellation_type ct)
        {
          for (auto& r : self->range)
            r.cancel(ct);
        }
      };

      Range range;
      std::vector<std::optional<var_t>> partial_result;
      cancellation_slot slot{this->cancel.slot()};
    };

    const auto size = std::distance(std::begin(range), std::end(range));
    auto impl =  std::allocate_shared<impl_t>(
        get_associated_allocator(exec), exec, std::move(range));
    impl->executor = exec;
    impl->partial_result.resize(size);

    if (size == 0u)
    {
      impl->result.emplace();
      impl->done = true;
      if (auto f = std::exchange(impl->completion, nullptr); !!f)
        boost::asio::post(exec, [impl, f = std::move(f)]() mutable
            {
              std::apply(std::move(f), std::move(*impl->result));
            });
      return {impl};
    }
    auto idx = 0u;
    for (auto& val : impl->range) {
      val.async_wait(bind_executor(
          exec,
          [idx, impl]<typename... Args>(Args&&... args) {

            impl->partial_result[idx].emplace(std::forward<Args>(args)...);
            if (std::all_of(impl->partial_result.begin(),
                  impl->partial_result.end(),
                  [](auto &opt) {return opt.has_value();}))
            {
              impl->result.emplace();
              get<0>(*impl->result).reserve(impl->partial_result.size());
              for (auto& p : impl->partial_result)
                get<0>(*impl->result).push_back(std::move(*p));

              impl->done = true;
              if (auto f = std::exchange(impl->completion, nullptr); !!f)
                std::apply(std::move(f), std::move(*impl->result));
            }

          }));
      idx++;
    }
    return {impl};
  }

  template <typename Range>
#if !defined(GENERATING_DOCUMENTATION)
    requires requires (Range r)
    {
      {*std::begin(r)} -> is_promise_c;
      {*std::  end(r)} -> is_promise_c;
    }
#endif // !defined(GENERATING_DOCUMENTATION)
  static auto race(Range range)
  {
    if (std::begin(range) == std::end(range))
      throw std::logic_error(
          "Can't use race on an empty range with deduced executor");
    else
      return race(std::begin(range)->get_executor(), std::move(range));
  }

  template <typename Range>
#if !defined(GENERATING_DOCUMENTATION)
    requires requires (Range&& r)
    {
      {*std::begin(r)} -> is_promise_c;
      {*std::  end(r)} -> is_promise_c;
    }
#endif // !defined(GENERATING_DOCUMENTATION)
  static auto all(Range range)
  {
    if (std::begin(range) == std::end(range))
      throw std::logic_error(
          "Can't use all on an empty range with deduced executor");
    else
      return all(std::begin(range)->get_executor(), std::move(range));
  }

private:
#if !defined(GENERATING_DOCUMENTATION)
  template <typename, typename> friend struct promise;
  friend struct detail::promise_handler<void(Ts...)>;
#endif // !defined(GENERATING_DOCUMENTATION)

  std::shared_ptr<detail::promise_impl<void(Ts...), Executor>> impl_;
  promise(std::shared_ptr<detail::promise_impl<void(Ts...), Executor>> impl)
    : impl_(impl)
  {
  }

  struct initiate_async_wait
  {
    std::shared_ptr<detail::promise_impl<void(Ts...), Executor>> self_;

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) const
    {
      const auto exec = get_associated_executor(handler, self_->executor);
      auto cancel = get_associated_cancellation_slot(handler);
      if (self_->done)
      {
        boost::asio::post(exec,
            [self = self_, h = std::forward<WaitHandler>(handler)]() mutable
            {
              std::apply(std::forward<WaitHandler>(h),
                  std::move(*self->result));
            });
      }
      else
      {
        if (cancel.is_connected())
        {
          struct cancel_handler
          {
            std::weak_ptr<detail::promise_impl<void(Ts...), Executor>> self;

            cancel_handler(
                std::weak_ptr<detail::promise_impl<void(Ts...), Executor>> self)
              : self(std::move(self))
            {
            }

            void operator()(cancellation_type level) const
            {
              if (auto p = self.lock(); p != nullptr)
                p->cancel.emit(level);

            }
          };

          cancel.template emplace<cancel_handler>(self_);
        }

        self_->completion = {exec, std::forward<WaitHandler>(handler)};
      }
    }
  };
};

} // namespace experimental

#if !defined(GENERATING_DOCUMENTATION)

template <typename Executor, typename R, typename... Args>
struct async_result<experimental::use_promise_t<Executor>, R(Args...)>
{
  using handler_type = experimental::detail::promise_handler<
    void(typename decay<Args>::type...), Executor>;

  template <typename Initiation, typename... InitArgs>
  static auto initiate(Initiation initiation,
      experimental::use_promise_t<Executor>, InitArgs... args)
    -> typename handler_type::promise_type
  {
    handler_type ht{get_associated_executor(initiation)};
    std::move(initiation)(ht, std::move(args)...);
    return ht.make_promise();
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXPERIMENTAL_PROMISE_HPP
