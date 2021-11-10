#ifndef BOOST_BEAST_CORE_DETAIL_WORK_GUARD_HPP
#define BOOST_BEAST_CORE_DETAIL_WORK_GUARD_HPP

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/execution.hpp>
#include <boost/optional.hpp>

namespace boost {
namespace beast {
namespace detail {

template<class Executor, class Enable = void>
struct select_work_guard;

template<class Executor>
using select_work_guard_t = typename
    select_work_guard<Executor>::type;

#if !defined(BOOST_ASIO_NO_TS_EXECUTORS)
template<class Executor>
struct select_work_guard
<
    Executor,
    typename std::enable_if
    <
        net::is_executor<Executor>::value
    >::type
>
{
    using type = net::executor_work_guard<Executor>;
};
#endif

template<class Executor>
struct execution_work_guard
{
    using executor_type = decltype(
        net::prefer(std::declval<Executor const&>(),
            net::execution::outstanding_work.tracked));

    execution_work_guard(Executor const& exec)
    : ex_(net::prefer(exec, net::execution::outstanding_work.tracked))
    {

    }

    executor_type
    get_executor() const noexcept
    {
        BOOST_ASSERT(ex_.has_value());
        return *ex_;
    }

    void reset() noexcept
    {
        ex_.reset();
    }

private:

    boost::optional<executor_type> ex_;
};

template<class Executor>
struct select_work_guard
<
    Executor,
    typename std::enable_if
    <
        net::execution::is_executor<Executor>::value
#if defined(BOOST_ASIO_NO_TS_EXECUTORS)
        || net::is_executor<Executor>::value
#else
        && !net::is_executor<Executor>::value
#endif
    >::type
>
{
    using type = execution_work_guard<Executor>;
};

template<class Executor>
select_work_guard_t<Executor>
make_work_guard(Executor const& exec) noexcept
{
    return select_work_guard_t<Executor>(exec);
}

}
}
}

#endif // BOOST_BEAST_CORE_DETAIL_WORK_GUARD_HPP
