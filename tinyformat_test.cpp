#if defined(__linux__) && defined(__clang__)
// Workaround for bug in gcc 4.4 standard library headers when compling with
// clang in C++11 mode.
namespace std { class type_info; }
#endif

#if defined(_MSC_VER) && _MSC_VER == 1800
    // Disable spurious warning about printf format string mismatch in VS 2012
#   pragma warning(disable:4313)
#endif

#include <stdexcept>
#include <climits>
#include <cfloat>
#include <cstddef>

// Throw instead of abort() so we can test error conditions.
#define TINYFORMAT_ERROR(reason) \
    throw std::runtime_error(reason);

#include "tinyformat.h"
#include <cassert>

#if 0
// Compare result of tfm::format() to C's sprintf().
template<typename... Args>
void compareSprintf(const Args&... args)
{
    std::string tfmResult = tfm::format(args...);
    char sprintfResult[200];
    sprintf(sprintfResult, args...);
    if(tfmResult != sprintfResult)
    {
        std::cout << tfmResult << std::endl;
        std::cout << sprintfResult << std::endl;
        assert(0 && "results didn't match, see above.");
    }
}
#endif

#define EXPECT_ERROR(expression)                                \
try                                                             \
{                                                               \
    expression;                                                 \
    std::cout << "test failed, line " << __LINE__ << "\n";      \
    std::cout << "expected exception in " #expression << "\n";  \
    ++nfailed;                                                  \
}                                                               \
catch(std::runtime_error&) {}                                   \

#define CHECK_EQUAL(a, b)                                  \
if(!((a) == (b)))                                          \
{                                                          \
    std::cout << "test failed, line " << __LINE__ << "\n"; \
    std::cout << (a) << " != " << (b) << "\n";             \
    std::cout << "[" #a ", " #b "]\n";                     \
    ++nfailed;                                             \
}


// Test wrapping to create our own function which calls through to tfm::format
struct TestWrap
{
    std::ostringstream m_oss;
    // template<typename... Args>
    // std::string error(int code, const char* fmt, const Args&... args);
#   define MAKE_ERROR_FUNC(n)                                            \
    template<TINYFORMAT_ARGTYPES(n)>                                     \
    std::string error(int code, const char* fmt, TINYFORMAT_VARARGS(n))  \
    {                                                                    \
        m_oss.clear();                                                   \
        m_oss << code << ": ";                                           \
        tfm::format(m_oss, fmt, TINYFORMAT_PASSARGS(n));                 \
        return m_oss.str();                                              \
    }
    TINYFORMAT_FOREACH_ARGNUM(MAKE_ERROR_FUNC)
};


struct TestExceptionDef : public std::runtime_error
{
#   define MAKE_CONSTRUCTOR(n)                                          \
    template<TINYFORMAT_ARGTYPES(n)>                                    \
    TestExceptionDef(const char* fmt, TINYFORMAT_VARARGS(n))            \
        : std::runtime_error(tfm::format(fmt, TINYFORMAT_PASSARGS(n)))  \
    { }
    TINYFORMAT_FOREACH_ARGNUM(MAKE_CONSTRUCTOR)
};


struct MyInt {
public:
    MyInt(int value) : m_value(value) {}
    int value() const { return m_value; }
private:
    int m_value;
};

std::ostream& operator<<(std::ostream& os, const MyInt& obj) {
    os << obj.value();
    return os;
}


int unitTests()
{
    int nfailed = 0;
#   if defined(_MSC_VER) && _MSC_VER < 1900 // VC++ older than 2015
    // floats are printed with three digit exponents on windows, which messes
    // up the tests.  Turn this off for consistency:
    _set_output_format(_TWO_DIGIT_EXPONENT);
#   endif
    // Test various basic format specs against results of sprintf
    CHECK_EQUAL(tfm::format("%s", "asdf"), "asdf");
    CHECK_EQUAL(tfm::format("%d", 1234), "1234");
    CHECK_EQUAL(tfm::format("%i", -5678), "-5678");
    CHECK_EQUAL(tfm::format("%o", 012), "12");
    CHECK_EQUAL(tfm::format("%u", 123456u), "123456");
    CHECK_EQUAL(tfm::format("%x", 0xdeadbeef), "deadbeef");
    CHECK_EQUAL(tfm::format("%X", 0xDEADBEEF), "DEADBEEF");
    CHECK_EQUAL(tfm::format("%e", 1.23456e10), "1.234560e+10");
    CHECK_EQUAL(tfm::format("%E", -1.23456E10), "-1.234560E+10");
    CHECK_EQUAL(tfm::format("%f", -9.8765), "-9.876500");
    CHECK_EQUAL(tfm::format("%F", 9.8765), "9.876500");
    CHECK_EQUAL(tfm::format("%g", 10), "10");
    CHECK_EQUAL(tfm::format("%G", 100), "100");
    CHECK_EQUAL(tfm::format("%c", 65), "A");
    CHECK_EQUAL(tfm::format("%hc", (short)65), "A");
    CHECK_EQUAL(tfm::format("%lc", (long)65), "A");
    CHECK_EQUAL(tfm::format("%s", "asdf_123098"), "asdf_123098");
    // Note: All tests printing pointers are different on windows, since
    // there's no standard numerical representation.
    // Representation also differs between 32-bit and 64-bit windows.
#   ifdef _MSC_VER
#   ifdef _WIN64
    CHECK_EQUAL(tfm::format("%p", (void*)0x12345), "0000000000012345");
#   else
    CHECK_EQUAL(tfm::format("%p", (void*)0x12345), "00012345");
#   endif // _WIN64
#   else
    CHECK_EQUAL(tfm::format("%p", (void*)0x12345), "0x12345");
#   endif // _MSC_VER
    CHECK_EQUAL(tfm::format("%%%s", "asdf"), "%asdf"); // note: plain "%%" format gives warning with gcc
    // chars with int format specs are printed as ints:
    CHECK_EQUAL(tfm::format("%hhd", (char)65), "65");
    CHECK_EQUAL(tfm::format("%hhu", (unsigned char)65), "65");
    CHECK_EQUAL(tfm::format("%hhd", (signed char)65), "65");
#   ifdef _MSC_VER
#   ifdef _WIN64
    CHECK_EQUAL(tfm::format("%p", (const char*)0x10), "0000000000000010");
#   else
    CHECK_EQUAL(tfm::format("%p", (const char*)0x10), "00000010");
#   endif // _WIN64
#   else
    CHECK_EQUAL(tfm::format("%p", (const char*)0x10), "0x10"); // should print address, not string.
#   endif // _MSC_VER
    // bools with string format spec are printed as "true" or "false"
    CHECK_EQUAL(tfm::format("%s", true), "true");
    CHECK_EQUAL(tfm::format("%d", true), "1");

    // Test precision & width
    CHECK_EQUAL(tfm::format("%10d", -10), "       -10");
    CHECK_EQUAL(tfm::format("%.4d", 10), "0010");
    CHECK_EQUAL(tfm::format("%10.4f", 1234.1234567890), " 1234.1235");
    CHECK_EQUAL(tfm::format("%.f", 10.1), "10");
    CHECK_EQUAL(tfm::format("%.2s", "asdf"), "as"); // strings truncate to precision
    CHECK_EQUAL(tfm::format("%.2s", std::string("asdf")), "as");
//    // Test variable precision & width
    CHECK_EQUAL(tfm::format("%*.4f", 10, 1234.1234567890), " 1234.1235");
    CHECK_EQUAL(tfm::format("%10.*f", 4, 1234.1234567890), " 1234.1235");
    CHECK_EQUAL(tfm::format("%*.*f", 10, 4, 1234.1234567890), " 1234.1235");
    CHECK_EQUAL(tfm::format("%*.*f", -10, 4, 1234.1234567890), "1234.1235 ");

    // Test flags
    CHECK_EQUAL(tfm::format("%#x", 0x271828), "0x271828");
    CHECK_EQUAL(tfm::format("%#o", 0x271828), "011614050");
    CHECK_EQUAL(tfm::format("%#f", 3.0), "3.000000");
    CHECK_EQUAL(tfm::format("%+d", 3), "+3");
    CHECK_EQUAL(tfm::format("%+d", 0), "+0");
    CHECK_EQUAL(tfm::format("%+d", -3), "-3");
    CHECK_EQUAL(tfm::format("%010d", 100), "0000000100");
    CHECK_EQUAL(tfm::format("%010d", -10), "-000000010"); // sign should extend
    CHECK_EQUAL(tfm::format("%#010X", 0xBEEF), "0X0000BEEF");
    CHECK_EQUAL(tfm::format("% d",  10), " 10");
    CHECK_EQUAL(tfm::format("% d", -10), "-10");
    // Test flags with variable precision & width
    CHECK_EQUAL(tfm::format("%+.2d", 3), "+03");
    CHECK_EQUAL(tfm::format("%+.2d", -3), "-03");
    // flag override precedence
    CHECK_EQUAL(tfm::format("%+ d", 10), "+10"); // '+' overrides ' '
    CHECK_EQUAL(tfm::format("% +d", 10), "+10");
    CHECK_EQUAL(tfm::format("%-010d", 10), "10        "); // '-' overrides '0'
    CHECK_EQUAL(tfm::format("%0-10d", 10), "10        ");

    // Check that length modifiers are ignored
    CHECK_EQUAL(tfm::format("%hd", (short)1000), "1000");
    CHECK_EQUAL(tfm::format("%ld", (long)100000), "100000");
    CHECK_EQUAL(tfm::format("%lld", (long long)100000), "100000");
    CHECK_EQUAL(tfm::format("%zd", (size_t)100000), "100000");
    CHECK_EQUAL(tfm::format("%td", (ptrdiff_t)100000), "100000");
    CHECK_EQUAL(tfm::format("%jd", 100000), "100000");

    // Check that 0-argument formatting is printf-compatible
    CHECK_EQUAL(tfm::format("100%%"), "100%");

    // printf incompatibilities:
    // compareSprintf("%6.4x", 10); // precision & width can't be supported independently
    // compareSprintf("%.4d", -10); // negative numbers + precision don't quite work.

    // General "complicated" format spec test
    CHECK_EQUAL(tfm::format("%0.10f:%04d:%+g:%s:%#X:%c:%%:%%asdf",
                       1.234, 42, 3.13, "str", 0XDEAD, (int)'X'),
                "1.2340000000:0042:+3.13:str:0XDEAD:X:%:%asdf");

    // Test wrong number of args
    EXPECT_ERROR( tfm::format("%d", 5, 10) )
    EXPECT_ERROR( tfm::format("%d %d", 1)  )
    // Unterminated format spec
    EXPECT_ERROR( tfm::format("%123", 10)  )
    // Types used to specify variable width/precision must be convertible to int.
    EXPECT_ERROR( tfm::format("%0*d", "thing that can't convert to int", 42)  )
    EXPECT_ERROR( tfm::format("%0.*d", "thing that can't convert to int", 42) )
    // Error required if not enough args for variable width/precision
    EXPECT_ERROR( tfm::format("%*d", 1)      )
    EXPECT_ERROR( tfm::format("%.*d", 1)     )
    EXPECT_ERROR( tfm::format("%*.*d", 1, 2) )

    // Unhandled C99 format spec
    EXPECT_ERROR( tfm::format("%n", 10) )
    EXPECT_ERROR( tfm::format("%a", 10) )
    EXPECT_ERROR( tfm::format("%A", 10) )

#ifdef TEST_WCHAR_T_COMPILE
    // Test wchar_t handling - should fail to compile!
    tfm::format("%ls", L"blah");
#endif

    // Test that formatting is independent of underlying stream state.
    std::ostringstream oss;
    oss.width(20);
    oss.precision(10);
    oss.fill('*');
    oss.setf(std::ios::scientific);
    tfm::format(oss, "%f", 10.1234123412341234);
    CHECK_EQUAL(oss.str(), "10.123412");

    // Test formatting a custom object
    MyInt myobj(42);
    CHECK_EQUAL(tfm::format("myobj: %s", myobj), "myobj: 42");

    // Test that interface wrapping works correctly
    TestWrap wrap;
    CHECK_EQUAL(wrap.error(10, "someformat %s:%d:%d", "asdf", 2, 4),
                "10: someformat asdf:2:4");

    TestExceptionDef ex("blah %d", 100);
    CHECK_EQUAL(ex.what(), std::string("blah 100"));

    // Test tfm::printf by swapping the std::cout stream buffer to capture data
    // which would noramlly go to the stdout
    std::ostringstream coutCapture;
    std::streambuf* coutBuf = std::cout.rdbuf(coutCapture.rdbuf());
    tfm::printf("%s %s %d\n", "printf", "test", 1);
    tfm::printfln("%s %s %d", "printfln", "test", 1);
    std::cout.rdbuf(coutBuf); // restore buffer
    CHECK_EQUAL(coutCapture.str(), "printf test 1\nprintfln test 1\n");

    return nfailed;
}


int main()
{
    try
    {
        return unitTests();
    }
    catch (std::runtime_error & e)
    {
        std::cout << "Failure due to uncaught exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
