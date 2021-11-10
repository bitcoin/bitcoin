//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2020 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_TEST_DETAIL_STREAM_STATE_IPP
#define BOOST_BEAST_TEST_DETAIL_STREAM_STATE_IPP

#include <boost/beast/_experimental/test/error.hpp>

namespace boost {
namespace beast {
namespace test {

namespace detail {

//------------------------------------------------------------------------------

stream_service::
stream_service(net::execution_context& ctx)
    : beast::detail::service_base<stream_service>(ctx)
    , sp_(boost::make_shared<stream_service_impl>())
{
}

void
stream_service::
shutdown()
{
    std::vector<std::unique_ptr<detail::stream_read_op_base>> v;
    std::lock_guard<std::mutex> g1(sp_->m_);
    v.reserve(sp_->v_.size());
    for(auto p : sp_->v_)
    {
        std::lock_guard<std::mutex> g2(p->m);
        v.emplace_back(std::move(p->op));
        p->code = detail::stream_status::eof;
    }
}

auto
stream_service::
make_impl(
    net::any_io_executor exec,
    test::fail_count* fc) ->
    boost::shared_ptr<detail::stream_state>
{
#if defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
    auto& ctx = exec.context();
#else
    auto& ctx = net::query(
        exec,
        net::execution::context);
#endif
    auto& svc = net::use_service<stream_service>(ctx);
    auto sp = boost::make_shared<detail::stream_state>(exec, svc.sp_, fc);
    std::lock_guard<std::mutex> g(svc.sp_->m_);
    svc.sp_->v_.push_back(sp.get());
    return sp;
}

//------------------------------------------------------------------------------

void
stream_service_impl::
remove(stream_state& impl)
{
    std::lock_guard<std::mutex> g(m_);
    *std::find(
        v_.begin(), v_.end(),
            &impl) = std::move(v_.back());
    v_.pop_back();
}

//------------------------------------------------------------------------------

stream_state::
stream_state(
    net::any_io_executor exec_,
    boost::weak_ptr<stream_service_impl> wp_,
    fail_count* fc_)
    : exec(std::move(exec_))
    , wp(std::move(wp_))
    , fc(fc_)
{
}

stream_state::
~stream_state()
{
    // cancel outstanding read
    if(op != nullptr)
        (*op)(net::error::operation_aborted);
}

void
stream_state::
remove() noexcept
{
    auto sp = wp.lock();

    // If this goes off, it means the lifetime of a test::stream object
    // extended beyond the lifetime of the associated execution context.
    BOOST_ASSERT(sp);

    sp->remove(*this);
}

void
stream_state::
notify_read()
{
    if(op)
    {
        auto op_ = std::move(op);
        op_->operator()(error_code{});
    }
    else
    {
        cv.notify_all();
    }
}

void
stream_state::
cancel_read()
{
    std::unique_ptr<stream_read_op_base> p;
    {
        std::lock_guard<std::mutex> lock(m);
        code = stream_status::eof;
        p = std::move(op);
    }
    if(p != nullptr)
        (*p)(net::error::operation_aborted);
}

} // detail

} // test
} // beast
} // boost

#endif // BOOST_BEAST_TEST_DETAIL_STREAM_STATE_IPP
