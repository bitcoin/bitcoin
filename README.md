# tinyformat.h

## A minimal type safe printf() replacement

**tinyformat.h** is a type safe printf replacement library in a single C++
header file.  If you've ever wanted `printf("%s", s)` to just work regardless
of the type of `s`, tinyformat might be for you.  Design goals include:

* Type safety and extensibility for user defined types.
* C99 `printf()` compatibility, to the extent possible using `std::ostream`
* Simplicity and minimalism.  A single header file to include and distribute
  with your projects.
* Augment rather than replace the standard stream formatting mechanism
* C++98 support, with optional C++11 niceties

Build status, master branch:
[![Linux/OSX build](https://travis-ci.org/c42f/tinyformat.svg?branch=master)](https://travis-ci.org/c42f/tinyformat)
[![Windows build](https://ci.appveyor.com/api/projects/status/rwxqhhy6v5m0p1aq/branch/master?svg=true)](https://ci.appveyor.com/project/c42f/tinyformat/branch/master)

## Quickstart

To print a date to `std::cout`:

```C++
std::string weekday = "Wednesday";
const char* month = "July";
size_t day = 27;
long hour = 14;
int min = 44;

tfm::printf("%s, %s %d, %.2d:%.2d\n", weekday, month, day, hour, min);
```

The strange types here emphasize the type safety of the interface, for example
it is possible to print a `std::string` using the `"%s"` conversion, and a
`size_t` using the `"%d"` conversion.  A similar result could be achieved
using either of the `tfm::format()` functions.  One prints on a user provided
stream:

```C++
tfm::format(std::cerr, "%s, %s %d, %.2d:%.2d\n",
            weekday, month, day, hour, min);
```

The other returns a `std::string`:

```C++
std::string date = tfm::format("%s, %s %d, %.2d:%.2d\n",
                               weekday, month, day, hour, min);
std::cout << date;
```


It is safe to use tinyformat inside a template function.  For any type which
has the usual stream insertion `operator<<` defined, the following will work
as desired:

```C++
template<typename T>
void myPrint(const T& value)
{
    tfm::printf("My value is '%s'\n", value);
}
```

(The above is a compile error for types `T` without a stream insertion
operator.)


## Function reference

All user facing functions are defined in the namespace `tinyformat`.  A
namespace alias `tfm` is provided to encourage brevity, but can easily be
disabled if desired.

Three main interface functions are available: an iostreams-based `format()`,
a string-based `format()` and a `printf()` replacement.  These functions
can be thought of as C++ replacements for C's `fprintf()`, `sprintf()` and
`printf()` functions respectively.  All the interface functions can take an
unlimited number of input arguments if compiled with C++11 variadic templates
support.  In C++98 mode, the number of arguments must be limited to some fixed
upper bound which is currently 16 as of version 1.3. Supporting more arguments
is quite easy using the in-source code generator based on
[cog.py](http://nedbatchelder.com/code/cog) - see the source for details.

The `format()` function which takes a stream as the first argument is the
main part of the tinyformat interface.  `stream` is the output stream,
`formatString` is a format string in C99 `printf()` format, and the values
to be formatted have arbitrary types:

```C++
template<typename... Args>
void format(std::ostream& stream, const char* formatString,
            const Args&... args);
```

The second version of `format()` is a convenience function which returns a
`std::string` rather than printing onto a stream.  This function simply
calls the main version of `format()` using a `std::ostringstream`, and
returns the resulting string:

```C++
template<typename... Args>
std::string format(const char* formatString, const Args&... args);
```

Finally, `printf()` and `printfln()` are convenience functions which call
`format()` with `std::cout` as the first argument; both have the same
signature:

```C++
template<typename... Args>
void printf(const char* formatString, const Args&... args);
```

`printfln()` is the same as `printf()` but appends an additional newline
for convenience - a concession to the author's tendency to forget the newline
when using the library for simple logging.

## Format strings and type safety

Tinyformat parses C99 format strings to guide the formatting process --- please
refer to any standard C99 printf documentation for format string syntax.  In
contrast to printf, tinyformat does not use the format string to decide on
the type to be formatted so this does not compromise the type safety: *you may
use any format specifier with any C++ type*.  The author suggests standardising
on the `%s` conversion unless formatting numeric types.

Let's look at what happens when you execute the function call:

```C++
tfm::format(outStream, "%+6.4f", yourType);
```

First, the library parses the format string, and uses it to modify the state of
`outStream`:

1. The `outStream` formatting flags are cleared and the width, precision and
   fill reset to the default.
2. The flag `'+'` means to prefix positive numbers with a `'+'`; tinyformat
   executes `outStream.setf(std::ios::showpos)`
3. The number 6 gives the field width; execute `outStream.width(6)`.
4. The number 4 gives the precision; execute `outStream.precision(4)`.
5. The conversion specification character `'f'` means that floats should be
   formatted with a fixed number of digits; this corresponds to executing
   `outStream.setf(std::ios::fixed, std::ios::floatfield);`

After all these steps, tinyformat executes:

```C++
outStream << yourType;
```

and finally restores the stream flags, precision and fill.

What happens if `yourType` isn't actually a floating point type?  In this
case the flags set above are probably irrelevant and will be ignored by the
underlying `std::ostream` implementation.  The field width of six may cause
some padding in the output of `yourType`, but that's about it.


### Special cases for "%p", "%c" and "%s"

Tinyformat normally uses `operator<<` to convert types to strings.  However,
the "%p" and "%c" conversions require special rules for robustness.  Consider:

```C++
uint8_t* pixels = get_pixels(/* ... */);
tfm::printf("%p", pixels);
```

Clearly the intention here is to print a representation of the *pointer* to
`pixels`, but since `uint8_t` is a character type the compiler would
attempt to print it as a C string if we blindly fed it into `operator<<`.  To
counter this kind of madness, tinyformat tries to static_cast any type fed to
the "%p" conversion into a `const void*` before printing.  If this can't be
done at compile time the library falls back to using `operator<<` as usual.

The "%c" conversion has a similar problem: it signifies that the given integral
type should be converted into a `char` before printing.  The solution is
identical: attempt to convert the provided type into a char using
`static_cast` if possible, and if not fall back to using `operator<<`.

The "%s" conversion sets the boolalpha flag on the formatting stream.  This
means that a `bool` variable printed with "%s" will come out as `true` or
`false` rather than the `1` or `0` that you would otherwise get.


### Incompatibilities with C99 printf

Not all features of printf can be simulated simply using standard iostreams.
Here's a list of known incompatibilities:

* The C99 `"%a"` and `"%A"` hexadecimal floating point conversions are not
  supported since the iostreams don't have the necessary flags.  Using either
  of these flags will result in a call to `TINYFORMAT_ERROR`.
* The precision for integer conversions cannot be supported by the iostreams
  state independently of the field width.  (Note: **this is only a
  problem for certain obscure integer conversions**; float conversions like
  `%6.4f` work correctly.)  In tinyformat the field width takes precedence,
  so the 4 in `%6.4d` will be ignored.  However, if the field width is not
  specified, the width used internally is set equal to the precision and padded
  with zeros on the left.  That is, a conversion like `%.4d` effectively
  becomes `%04d` internally.  This isn't correct for every case (eg, negative
  numbers end up with one less digit than desired) but it's about the closest
  simple solution within the iostream model.
* The `"%n"` query specifier isn't supported to keep things simple and will
  result in a call to `TINYFORMAT_ERROR`.
* The `"%ls"` conversion is not supported, and attempting to format a
  `wchar_t` array will cause a compile time error to minimise unexpected
  surprises.  If you know the encoding of your wchar_t strings, you could write
  your own `std::ostream` insertion operator for them, and disable the
  compile time check by defining the macro `TINYFORMAT_ALLOW_WCHAR_STRINGS`.
  If you want to print the *address* of a wide character with the `"%p"`
  conversion, you should cast it to a `void*` before passing it to one of the
  formatting functions.


## Error handling

By default, tinyformat calls `assert()` if it encounters an error in the
format string or number of arguments.  This behaviour can be changed (for
example, to throw an exception) by defining the `TINYFORMAT_ERROR` macro
before including tinyformat.h, or editing the config section of the header.


## Formatting user defined types

User defined types with a stream insertion operator will be formatted using
`operator<<(std::ostream&, T)` by default.  The `"%s"` format specifier is
suggested for user defined types, unless the type is inherently numeric.

For further customization, the user can override the `formatValue()`
function to specify formatting independently of the stream insertion operator.
If you override this function, the library will have already parsed the format
specification and set the stream flags accordingly - see the source for details.


## Wrapping tfm::format() inside a user defined format function

Suppose you wanted to define your own function which wraps `tfm::format`.
For example, consider an error function taking an error code, which in C++11
might be written simply as:

```C++
template<typename... Args>
void error(int code, const char* fmt, const Args&... args)
{
    std::cerr << "error (code " << code << ")";
    tfm::format(std::cerr, fmt, args...);
}
```

Simulating this functionality in C++98 is pretty painful since it requires
writing out a version of `error()` for each desired number of arguments.  To
make this bearable tinyformat comes with a set of macros which are used
internally to generate the API, but which may also be used in user code.

The three macros `TINYFORMAT_ARGTYPES(n)`, `TINYFORMAT_VARARGS(n)` and
`TINYFORMAT_PASSARGS(n)` will generate a list of `n` argument types,
type/name pairs and argument names respectively when called with an integer
`n` between 1 and 16.  We can use these to define a macro which generates the
desired user defined function with `n` arguments.  This should be followed by
a call to `TINYFORMAT_FOREACH_ARGNUM` to generate the set of functions for
all supported `n`:

```C++
#define MAKE_ERROR_FUNC(n)                                    \
template<TINYFORMAT_ARGTYPES(n)>                              \
void error(int code, const char* fmt, TINYFORMAT_VARARGS(n))  \
{                                                             \
    std::cerr << "error (code " << code << ")";               \
    tfm::format(std::cerr, fmt, TINYFORMAT_PASSARGS(n));      \
}
TINYFORMAT_FOREACH_ARGNUM(MAKE_ERROR_FUNC)
```

Sometimes it's useful to be able to pass a list of format arguments through to
a non-template function.  The `FormatList` class is provided as a way to do
this by storing the argument list in a type-opaque way.  For example:

```C++
template<typename... Args>
void error(int code, const char* fmt, const Args&... args)
{
    tfm::FormatListRef formatList = tfm::makeFormatList(args...);
    errorImpl(code, fmt, formatList);
}
```

What's interesting here is that `errorImpl()` is a non-template function so
it could be separately compiled if desired.  The `FormatList` instance can be
used via a call to the `vformat()` function (the name chosen for semantic
similarity to `vprintf()`):

```C++
void errorImpl(int code, const char* fmt, tfm::FormatListRef formatList)
{
    std::cerr << "error (code " << code << ")";
    tfm::vformat(std::cout, fmt, formatList);
}
```

The construction of a `FormatList` instance is very lightweight - it defers
all formatting and simply stores a couple of function pointers and a value
pointer per argument.  Since most of the actual work is done inside
`vformat()`, any logic which causes an early exit of `errorImpl()` -
filtering of verbose log messages based on error code for example - could be a
useful optimization for programs using tinyformat.  (A faster option would be
to write any early bailout code inside `error()`, though this must be done in
the header.)


## Benchmarks

### Compile time and code bloat

The script `bloat_test.sh` included in the repository tests whether
tinyformat succeeds in avoiding compile time and code bloat for nontrivial
projects.  The idea is to include `tinyformat.h` into 100 translation units
and use `printf()` five times in each to simulate a medium sized project.
The resulting executable size and compile time (g++-4.8.2, linux ubuntu 14.04)
is shown in the following tables, which can be regenerated using `make
bloat_test`:

**Non-optimized build**

| test name              | compiler wall time | executable size (stripped) |
| ---------------------- | ------------------ | -------------------------- |
| libc printf            | 1.8s               | 48K  (36K)                 |
| std::ostream           | 10.7s              | 96K  (76K)                 |
| tinyformat, no inlines | 18.9s              | 140K (104K)                |
| tinyformat             | 21.1s              | 220K (180K)                |
| tinyformat, c++0x mode | 20.7s              | 220K (176K)                |
| boost::format          | 70.1s              | 844K (736K)                |

**Optimized build (-O3 -DNDEBUG)**

| test name              | compiler wall time | executable size (stripped) |
| ---------------------- | ------------------ | -------------------------- |
| libc printf            | 2.3s               | 40K  (28K)                 |
| std::ostream           | 11.8s              | 104K (80K)                 |
| tinyformat, no inlines | 23.0s              | 128K (104K)                |
| tinyformat             | 32.9s              | 128K (104K)                |
| tinyformat, c++0x mode | 34.0s              | 128K (104K)                |
| boost::format          | 147.9s             | 644K (600K)                |

For large projects it's arguably worthwhile to do separate compilation of the
non-templated parts of tinyformat, as shown in the rows labelled *tinyformat,
no inlines*.  These were generated by putting the implementation of `vformat`
(`detail::formatImpl()` etc) it into a separate file, tinyformat.cpp.  Note
that the results above can vary considerably with different compilers.  For
example, the `-fipa-cp-clone` optimization pass in g++-4.6 resulted in
excessively large binaries.  On the other hand, the g++-4.8 results are quite
similar to using clang++-3.4.


### Speed tests

The following speed tests results were generated by building
`tinyformat_speed_test.cpp` on an Intel core i7-2600K running Linux Ubuntu
14.04 with g++-4.8.2 using `-O3 -DNDEBUG`.  In the test, the format string
`"%0.10f:%04d:%+g:%s:%p:%c:%%\n"` is filled 2000000 times with output sent to
`/dev/null`; for further details see the source and Makefile.

| test name      | run time |
| -------------- | -------- |
| libc printf    | 1.20s    |
| std::ostream   | 1.82s    |
| tinyformat     | 2.08s    |
| boost::format  | 9.04s    |

It's likely that tinyformat has an advantage over boost.format because it tries
reasonably hard to avoid formatting into temporary strings, preferring instead
to send the results directly to the stream buffer.  Tinyformat cannot
be faster than the iostreams because it uses them internally, but it comes
acceptably close.


## Rationale

Or, why did I reinvent this particularly well studied wheel?

Nearly every program needs text formatting in some form but in many cases such
formatting is *incidental* to the main purpose of the program.  In these cases,
you really want a library which is simple to use but as lightweight as
possible.

The ultimate in lightweight dependencies are the solutions provided by the C++
and C libraries.  However, both the C++ iostreams and C's printf() have
well known usability problems: iostreams are hopelessly verbose for complicated
formatting and printf() lacks extensibility and type safety.  For example:

```C++
// Verbose; hard to read, hard to type:
std::cout << std::setprecision(2) << std::fixed << 1.23456 << "\n";
// The alternative using a format string is much easier on the eyes
tfm::printf("%.2f\n", 1.23456);

// Type mismatch between "%s" and int: will cause a segfault at runtime!
printf("%s", 1);
// The following is perfectly fine, and will result in "1" being printed.
tfm::printf("%s", 1);
```

On the other hand, there are plenty of excellent and complete libraries which
solve the formatting problem in great generality (boost.format and fastformat
come to mind, but there are many others).  Unfortunately these kind of
libraries tend to be rather heavy dependencies, far too heavy for projects
which need to do only a little formatting.  Problems include

1. Having many large source files.  This makes a heavy dependency unsuitable to
   bundle within other projects for convenience.
2. Slow build times for every file using any sort of formatting (this is very
   noticeable with g++ and boost/format.hpp. I'm not sure about the various
   other alternatives.)
3. Code bloat due to instantiating many templates

Tinyformat tries to solve these problems while providing formatting which is
sufficiently general and fast for incidental day to day uses.


## License

For minimum license-related fuss, tinyformat.h is distributed under the boost
software license, version 1.0.  (Summary: you must keep the license text on
all source copies, but don't have to mention tinyformat when distributing
binaries.)


## Author and acknowledgements

Tinyformat was written by Chris Foster, with contributions from various people
as recorded in the git repository.
The implementation owes a lot to `boost::format` for showing that it's fairly
easy to use stream based formatting to simulate most of the `printf()`
syntax.  Douglas Gregor's introduction to variadic templates --- see
http://www.generic-programming.org/~dgregor/cpp/variadic-templates.html --- was
also helpful, especially since it solves exactly the `printf()` problem for
the case of trivial format strings.

## Bugs

Here's a list of known bugs which are probably cumbersome to fix:

* Field padding won't work correctly with complicated user defined types.  For
  general types, the only way to do this correctly seems to be format to a
  temporary string stream, check the length, and finally send to the output
  stream with padding if necessary.  Doing this for all types would be
  quite inelegant because it implies extra allocations to make the temporary
  stream.  A workaround is to add logic to `operator<<()` for composite user
  defined types so they are aware of the stream field width.
