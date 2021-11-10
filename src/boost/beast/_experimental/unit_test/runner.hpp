//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_UNIT_TEST_RUNNER_H_INCLUDED
#define BOOST_BEAST_UNIT_TEST_RUNNER_H_INCLUDED

#include <boost/beast/_experimental/unit_test/suite_info.hpp>
#include <boost/assert.hpp>
#include <mutex>
#include <ostream>
#include <string>

namespace boost {
namespace beast {
namespace unit_test {

/** Unit test runner interface.

    Derived classes can customize the reporting behavior. This interface is
    injected into the unit_test class to receive the results of the tests.
*/
class runner
{
    std::string arg_;
    bool default_ = false;
    bool failed_ = false;
    bool cond_ = false;
    std::recursive_mutex mutex_;

public:
    runner() = default;
    virtual ~runner() = default;
    runner(runner const&) = delete;
    runner& operator=(runner const&) = delete;

    /** Set the argument string.

        The argument string is available to suites and
        allows for customization of the test. Each suite
        defines its own syntax for the argument string.
        The same argument is passed to all suites.
    */
    void
    arg(std::string const& s)
    {
        arg_ = s;
    }

    /** Returns the argument string. */
    std::string const&
    arg() const
    {
        return arg_;
    }

    /** Run the specified suite.
        @return `true` if any conditions failed.
    */
    template<class = void>
    bool
    run(suite_info const& s);

    /** Run a sequence of suites.
        The expression
            `FwdIter::value_type`
        must be convertible to `suite_info`.
        @return `true` if any conditions failed.
    */
    template<class FwdIter>
    bool
    run(FwdIter first, FwdIter last);

    /** Conditionally run a sequence of suites.
        pred will be called as:
        @code
            bool pred(suite_info const&);
        @endcode
        @return `true` if any conditions failed.
    */
    template<class FwdIter, class Pred>
    bool
    run_if(FwdIter first, FwdIter last, Pred pred = Pred{});

    /** Run all suites in a container.
        @return `true` if any conditions failed.
    */
    template<class SequenceContainer>
    bool
    run_each(SequenceContainer const& c);

    /** Conditionally run suites in a container.
        pred will be called as:
        @code
            bool pred(suite_info const&);
        @endcode
        @return `true` if any conditions failed.
    */
    template<class SequenceContainer, class Pred>
    bool
    run_each_if(SequenceContainer const& c, Pred pred = Pred{});

protected:
    /// Called when a new suite starts.
    virtual
    void
    on_suite_begin(suite_info const&)
    {
    }

    /// Called when a suite ends.
    virtual
    void
    on_suite_end()
    {
    }

    /// Called when a new case starts.
    virtual
    void
    on_case_begin(std::string const&)
    {
    }

    /// Called when a new case ends.
    virtual
    void
    on_case_end()
    {
    }

    /// Called for each passing condition.
    virtual
    void
    on_pass()
    {
    }

    /// Called for each failing condition.
    virtual
    void
    on_fail(std::string const&)
    {
    }

    /// Called when a test logs output.
    virtual
    void
    on_log(std::string const&)
    {
    }

private:
    friend class suite;

    // Start a new testcase.
    template<class = void>
    void
    testcase(std::string const& name);

    template<class = void>
    void
    pass();

    template<class = void>
    void
    fail(std::string const& reason);

    template<class = void>
    void
    log(std::string const& s);
};

//------------------------------------------------------------------------------

template<class>
bool
runner::run(suite_info const& s)
{
    // Enable 'default' testcase
    default_ = true;
    failed_ = false;
    on_suite_begin(s);
    s.run(*this);
    // Forgot to call pass or fail.
    BOOST_ASSERT(cond_);
    on_case_end();
    on_suite_end();
    return failed_;
}

template<class FwdIter>
bool
runner::run(FwdIter first, FwdIter last)
{
    bool failed(false);
    for(;first != last; ++first)
        failed = run(*first) || failed;
    return failed;
}

template<class FwdIter, class Pred>
bool
runner::run_if(FwdIter first, FwdIter last, Pred pred)
{
    bool failed(false);
    for(;first != last; ++first)
        if(pred(*first))
            failed = run(*first) || failed;
    return failed;
}

template<class SequenceContainer>
bool
runner::run_each(SequenceContainer const& c)
{
    bool failed(false);
    for(auto const& s : c)
        failed = run(s) || failed;
    return failed;
}

template<class SequenceContainer, class Pred>
bool
runner::run_each_if(SequenceContainer const& c, Pred pred)
{
    bool failed(false);
    for(auto const& s : c)
        if(pred(s))
            failed = run(s) || failed;
    return failed;
}

template<class>
void
runner::testcase(std::string const& name)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    // Name may not be empty
    BOOST_ASSERT(default_ || ! name.empty());
    // Forgot to call pass or fail
    BOOST_ASSERT(default_ || cond_);
    if(! default_)
        on_case_end();
    default_ = false;
    cond_ = false;
    on_case_begin(name);
}

template<class>
void
runner::pass()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(default_)
        testcase("");
    on_pass();
    cond_ = true;
}

template<class>
void
runner::fail(std::string const& reason)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(default_)
        testcase("");
    on_fail(reason);
    failed_ = true;
    cond_ = true;
}

template<class>
void
runner::log(std::string const& s)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(default_)
        testcase("");
    on_log(s);
}

} // unit_test
} // beast
} // boost

#endif
