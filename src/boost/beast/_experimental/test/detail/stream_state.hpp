//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2020 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_TEST_DETAIL_STREAM_STATE_HPP
#define BOOST_BEAST_TEST_DETAIL_STREAM_STATE_HPP

#include <boost/asio/any_io_executor.hpp>
#include <boost/beast/core/detail/service_base.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

namespace boost {
namespace beast {
namespace test {
namespace detail {

struct stream_state;

struct stream_service_impl
{
    std::mutex m_;
    std::vector<stream_state*> v_;

    BOOST_BEAST_DECL
    void
    remove(stream_state& impl);
};

//------------------------------------------------------------------------------

class stream_service
    : public beast::detail::service_base<stream_service>
{
    boost::shared_ptr<detail::stream_service_impl> sp_;

    BOOST_BEAST_DECL
    void
    shutdown() override;

public:
    BOOST_BEAST_DECL
    explicit
    stream_service(net::execution_context& ctx);

    BOOST_BEAST_DECL
    static
    auto
    make_impl(
        net::any_io_executor exec,
        test::fail_count* fc) ->
            boost::shared_ptr<detail::stream_state>;
};

//------------------------------------------------------------------------------

struct stream_read_op_base
{
    virtual ~stream_read_op_base() = default;
    virtual void operator()(error_code ec) = 0;
};

//------------------------------------------------------------------------------

enum class stream_status
{
    ok,
    eof,
};

//------------------------------------------------------------------------------

struct stream_state
{
    net::any_io_executor exec;
    boost::weak_ptr<stream_service_impl> wp;
    std::mutex m;
    flat_buffer b;
    std::condition_variable cv;
    std::unique_ptr<stream_read_op_base> op;
    stream_status code = stream_status::ok;
    fail_count* fc = nullptr;
    std::size_t nread = 0;
    std::size_t nread_bytes = 0;
    std::size_t nwrite = 0;
    std::size_t nwrite_bytes = 0;
    std::size_t read_max =
        (std::numeric_limits<std::size_t>::max)();
    std::size_t write_max =
        (std::numeric_limits<std::size_t>::max)();

    BOOST_BEAST_DECL
    stream_state(
        net::any_io_executor exec_,
        boost::weak_ptr<stream_service_impl> wp_,
        fail_count* fc_);

    BOOST_BEAST_DECL
    ~stream_state();

    BOOST_BEAST_DECL
    void
    remove() noexcept;

    BOOST_BEAST_DECL
    void
    notify_read();

    BOOST_BEAST_DECL
    void
    cancel_read();
};



} // detail
} // test
} // beast
} // boost

#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/_experimental/test/detail/stream_state.ipp>
#endif

#endif // BOOST_BEAST_TEST_DETAIL_STREAM_STATE_HPP
