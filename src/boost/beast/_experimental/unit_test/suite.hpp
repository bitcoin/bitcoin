//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_UNIT_TEST_SUITE_HPP
#define BOOST_BEAST_UNIT_TEST_SUITE_HPP

#include <boost/beast/_experimental/unit_test/runner.hpp>
#include <boost/throw_exception.hpp>
#include <ostream>
#include <sstream>
#include <string>

namespace boost {
namespace beast {
namespace unit_test {

namespace detail {

template<class String>
std::string
make_reason(String const& reason,
    char const* file, int line)
{
    std::string s(reason);
    if(! s.empty())
        s.append(": ");
    char const* path = file + strlen(file);
    while(path != file)
    {
    #ifdef _MSC_VER
        if(path[-1] == '\\')
    #else
        if(path[-1] == '/')
    #endif
            break;
        --path;
    }
    s.append(path);
    s.append("(");
    s.append(std::to_string(line));
    s.append(")");
    return s;
}

} // detail

class thread;

enum abort_t
{
    no_abort_on_fail,
    abort_on_fail
};

/** A testsuite class.

    Derived classes execute a series of testcases, where each testcase is
    a series of pass/fail tests. To provide a unit test using this class,
    derive from it and use the BOOST_BEAST_DEFINE_UNIT_TEST macro in a
    translation unit.
*/
class suite
{
private:
    bool abort_ = false;
    bool aborted_ = false;
    runner* runner_ = nullptr;

    // This exception is thrown internally to stop the current suite
    // in the event of a failure, if the option to stop is set.
    struct abort_exception : public std::exception
    {
        char const*
        what() const noexcept override
        {
            return "test suite aborted";
        }
    };

    template<class CharT, class Traits, class Allocator>
    class log_buf
        : public std::basic_stringbuf<CharT, Traits, Allocator>
    {
        suite& suite_;

    public:
        explicit
        log_buf(suite& self)
            : suite_(self)
        {
        }

        ~log_buf()
        {
            sync();
        }

        int
        sync() override
        {
            auto const& s = this->str();
            if(s.size() > 0)
                suite_.runner_->log(s);
            this->str("");
            return 0;
        }
    };

    template<
        class CharT,
        class Traits = std::char_traits<CharT>,
        class Allocator = std::allocator<CharT>
    >
    class log_os : public std::basic_ostream<CharT, Traits>
    {
        log_buf<CharT, Traits, Allocator> buf_;

    public:
        explicit
        log_os(suite& self)
            : std::basic_ostream<CharT, Traits>(&buf_)
            , buf_(self)
        {
        }
    };

    class scoped_testcase;

    class testcase_t
    {
        suite& suite_;
        std::stringstream ss_;

    public:
        explicit
        testcase_t(suite& self)
            : suite_(self)
        {
        }

        /** Open a new testcase.

            A testcase is a series of evaluated test conditions. A test
            suite may have multiple test cases. A test is associated with
            the last opened testcase. When the test first runs, a default
            unnamed case is opened. Tests with only one case may omit the
            call to testcase.

            @param abort Determines if suite continues running after a failure.
        */
        void
        operator()(std::string const& name,
            abort_t abort = no_abort_on_fail);

        scoped_testcase
        operator()(abort_t abort);

        template<class T>
        scoped_testcase
        operator<<(T const& t);
    };

public:
    /** Logging output stream.

        Text sent to the log output stream will be forwarded to
        the output stream associated with the runner.
    */
    log_os<char> log;

    /** Memberspace for declaring test cases. */
    testcase_t testcase;

    /** Returns the "current" running suite.
        If no suite is running, nullptr is returned.
    */
    static
    suite*
    this_suite()
    {
        return *p_this_suite();
    }

    suite()
        : log(*this)
        , testcase(*this)
    {
    }

    virtual ~suite() = default;
    suite(suite const&) = delete;
    suite& operator=(suite const&) = delete;

    /** Invokes the test using the specified runner.

        Data members are set up here instead of the constructor as a
        convenience to writing the derived class to avoid repetition of
        forwarded constructor arguments to the base.
        Normally this is called by the framework for you.
    */
    template<class = void>
    void
    operator()(runner& r);

    /** Record a successful test condition. */
    template<class = void>
    void
    pass();

    /** Record a failure.

        @param reason Optional text added to the output on a failure.

        @param file The source code file where the test failed.

        @param line The source code line number where the test failed.
    */
    /** @{ */
    template<class String>
    void
    fail(String const& reason, char const* file, int line);

    template<class = void>
    void
    fail(std::string const& reason = "");
    /** @} */

    /** Evaluate a test condition.

        This function provides improved logging by incorporating the
        file name and line number into the reported output on failure,
        as well as additional text specified by the caller.

        @param shouldBeTrue The condition to test. The condition
        is evaluated in a boolean context.

        @param reason Optional added text to output on a failure.

        @param file The source code file where the test failed.

        @param line The source code line number where the test failed.

        @return `true` if the test condition indicates success.
    */
    /** @{ */
    template<class Condition>
    bool
    expect(Condition const& shouldBeTrue)
    {
        return expect(shouldBeTrue, "");
    }

    template<class Condition, class String>
    bool
    expect(Condition const& shouldBeTrue, String const& reason);

    template<class Condition>
    bool
    expect(Condition const& shouldBeTrue,
        char const* file, int line)
    {
        return expect(shouldBeTrue, "", file, line);
    }

    template<class Condition, class String>
    bool
    expect(Condition const& shouldBeTrue,
        String const& reason, char const* file, int line);
    /** @} */

    //
    // DEPRECATED
    //
    // Expect an exception from f()
    template<class F, class String>
    bool
    except(F&& f, String const& reason);
    template<class F>
    bool
    except(F&& f)
    {
        return except(f, "");
    }
    template<class E, class F, class String>
    bool
    except(F&& f, String const& reason);
    template<class E, class F>
    bool
    except(F&& f)
    {
        return except<E>(f, "");
    }
    template<class F, class String>
    bool
    unexcept(F&& f, String const& reason);
    template<class F>
    bool
    unexcept(F&& f)
    {
        return unexcept(f, "");
    }

    /** Return the argument associated with the runner. */
    std::string const&
    arg() const
    {
        return runner_->arg();
    }

    // DEPRECATED
    // @return `true` if the test condition indicates success(a false value)
    template<class Condition, class String>
    bool
    unexpected(Condition shouldBeFalse,
        String const& reason);

    template<class Condition>
    bool
    unexpected(Condition shouldBeFalse)
    {
        return unexpected(shouldBeFalse, "");
    }

private:
    friend class thread;

    static
    suite**
    p_this_suite()
    {
        static suite* pts = nullptr;
        return &pts;
    }

    /** Runs the suite. */
    virtual
    void
    run() = 0;

    void
    propagate_abort();

    template<class = void>
    void
    run(runner& r);
};

//------------------------------------------------------------------------------

// Helper for streaming testcase names
class suite::scoped_testcase
{
private:
    suite& suite_;
    std::stringstream& ss_;

public:
    scoped_testcase& operator=(scoped_testcase const&) = delete;

    ~scoped_testcase()
    {
        auto const& name = ss_.str();
        if(! name.empty())
            suite_.runner_->testcase(name);
    }

    scoped_testcase(suite& self, std::stringstream& ss)
        : suite_(self)
        , ss_(ss)
    {
        ss_.clear();
        ss_.str({});
    }

    template<class T>
    scoped_testcase(suite& self,
            std::stringstream& ss, T const& t)
        : suite_(self)
        , ss_(ss)
    {
        ss_.clear();
        ss_.str({});
        ss_ << t;
    }

    template<class T>
    scoped_testcase&
    operator<<(T const& t)
    {
        ss_ << t;
        return *this;
    }
};

//------------------------------------------------------------------------------

inline
void
suite::testcase_t::operator()(
    std::string const& name, abort_t abort)
{
    suite_.abort_ = abort == abort_on_fail;
    suite_.runner_->testcase(name);
}

inline
suite::scoped_testcase
suite::testcase_t::operator()(abort_t abort)
{
    suite_.abort_ = abort == abort_on_fail;
    return { suite_, ss_ };
}

template<class T>
inline
suite::scoped_testcase
suite::testcase_t::operator<<(T const& t)
{
    return { suite_, ss_, t };
}

//------------------------------------------------------------------------------

template<class>
void
suite::
operator()(runner& r)
{
    *p_this_suite() = this;
    try
    {
        run(r);
        *p_this_suite() = nullptr;
    }
    catch(...)
    {
        *p_this_suite() = nullptr;
        throw;
    }
}

template<class Condition, class String>
bool
suite::
expect(
    Condition const& shouldBeTrue, String const& reason)
{
    if(shouldBeTrue)
    {
        pass();
        return true;
    }
    fail(reason);
    return false;
}

template<class Condition, class String>
bool
suite::
expect(Condition const& shouldBeTrue,
    String const& reason, char const* file, int line)
{
    if(shouldBeTrue)
    {
        pass();
        return true;
    }
    fail(detail::make_reason(reason, file, line));
    return false;
}

// DEPRECATED

template<class F, class String>
bool
suite::
except(F&& f, String const& reason)
{
    try
    {
        f();
        fail(reason);
        return false;
    }
    catch(...)
    {
        pass();
    }
    return true;
}

template<class E, class F, class String>
bool
suite::
except(F&& f, String const& reason)
{
    try
    {
        f();
        fail(reason);
        return false;
    }
    catch(E const&)
    {
        pass();
    }
    return true;
}

template<class F, class String>
bool
suite::
unexcept(F&& f, String const& reason)
{
    try
    {
        f();
        pass();
        return true;
    }
    catch(...)
    {
        fail(reason);
    }
    return false;
}

template<class Condition, class String>
bool
suite::
unexpected(
    Condition shouldBeFalse, String const& reason)
{
    bool const b =
        static_cast<bool>(shouldBeFalse);
    if(! b)
        pass();
    else
        fail(reason);
    return ! b;
}

template<class>
void
suite::
pass()
{
    propagate_abort();
    runner_->pass();
}

// ::fail
template<class>
void
suite::
fail(std::string const& reason)
{
    propagate_abort();
    runner_->fail(reason);
    if(abort_)
    {
        aborted_ = true;
        BOOST_THROW_EXCEPTION(abort_exception());
    }
}

template<class String>
void
suite::
fail(String const& reason, char const* file, int line)
{
    fail(detail::make_reason(reason, file, line));
}

inline
void
suite::
propagate_abort()
{
    if(abort_ && aborted_)
        BOOST_THROW_EXCEPTION(abort_exception());
}

template<class>
void
suite::
run(runner& r)
{
    runner_ = &r;

    try
    {
        run();
    }
    catch(abort_exception const&)
    {
        // ends the suite
    }
    catch(std::exception const& e)
    {
        runner_->fail("unhandled exception: " +
            std::string(e.what()));
    }
    catch(...)
    {
        runner_->fail("unhandled exception");
    }
}

#ifndef BEAST_PASS
#define BEAST_PASS() ::boost::beast::unit_test::suite::this_suite()->pass()
#endif

#ifndef BEAST_FAIL
#define BEAST_FAIL() ::boost::beast::unit_test::suite::this_suite()->fail("", __FILE__, __LINE__)
#endif

#ifndef BEAST_EXPECT
/** Check a precondition.

    If the condition is false, the file and line number are reported.
*/
#define BEAST_EXPECT(cond) ::boost::beast::unit_test::suite::this_suite()->expect(cond, __FILE__, __LINE__)
#endif

#ifndef BEAST_EXPECTS
/** Check a precondition.

    If the condition is false, the file and line number are reported.
*/
#define BEAST_EXPECTS(cond, reason) ((cond) ? \
    (::boost::beast::unit_test::suite::this_suite()->pass(), true) : \
    (::boost::beast::unit_test::suite::this_suite()->fail((reason), __FILE__, __LINE__), false))
#endif

/** Ensure an exception is thrown
*/
#define BEAST_THROWS( EXPR, EXCEP ) \
    try { \
        EXPR; \
        BEAST_FAIL(); \
    } \
    catch(EXCEP const&) { \
        BEAST_PASS(); \
    } \
    catch(...) { \
        BEAST_FAIL(); \
    }

} // unit_test
} // beast
} // boost

//------------------------------------------------------------------------------

// detail:
// This inserts the suite with the given manual flag
#define BEAST_DEFINE_TESTSUITE_INSERT(Library,Module,Class,manual) \
    static ::boost::beast::unit_test::detail::insert_suite <Class##_test>   \
        Library ## Module ## Class ## _test_instance(             \
            #Class, #Module, #Library, manual)

//------------------------------------------------------------------------------

// Preprocessor directives for controlling unit test definitions.

// If this is already defined, don't redefine it. This allows
// programs to provide custom behavior for testsuite definitions
//
#ifndef BEAST_DEFINE_TESTSUITE

/** Enables insertion of test suites into the global container.
    The default is to insert all test suite definitions into the global
    container. If BEAST_DEFINE_TESTSUITE is user defined, this macro
    has no effect.
*/
#ifndef BEAST_NO_UNIT_TEST_INLINE
#define BEAST_NO_UNIT_TEST_INLINE 0
#endif

/** Define a unit test suite.

    Library   Identifies the library.
    Module    Identifies the module.
    Class     The type representing the class being tested.

    The declaration for the class implementing the test should be the same
    as Class ## _test. For example, if Class is aged_ordered_container, the
    test class must be declared as:

    @code

    struct aged_ordered_container_test : beast::unit_test::suite
    {
        //...
    };

    @endcode

    The macro invocation must appear in the same namespace as the test class.
*/

#if BEAST_NO_UNIT_TEST_INLINE
#define BEAST_DEFINE_TESTSUITE(Class,Module,Library)

#else
#include <boost/beast/_experimental/unit_test/global_suites.hpp>
#define BEAST_DEFINE_TESTSUITE(Library,Module,Class) \
        BEAST_DEFINE_TESTSUITE_INSERT(Library,Module,Class,false)
#define BEAST_DEFINE_TESTSUITE_MANUAL(Library,Module,Class) \
        BEAST_DEFINE_TESTSUITE_INSERT(Library,Module,Class,true)

#endif

#endif

//------------------------------------------------------------------------------

#endif
