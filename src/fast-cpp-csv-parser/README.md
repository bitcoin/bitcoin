# Fast C++ CSV Parser

This is a small, easy-to-use and fast header-only library for reading comma separated value (CSV) files. 

## Features

  * Automatically rearranges columns by parsing the header line.
  * Disk I/O and CSV-parsing are overlapped using threads for efficiency.
  * Parsing features such as escaped strings can be enabled and disabled at compile time using templates. You only pay in speed for the features you actually use.
  * Can read multiple GB files in reasonable time.
  * Support for custom columns separators (i.e. Tab separated value files are supported), quote escaped strings, automatic space trimming. 
  * Works with `*`nix and Windows newlines and automatically ignores UTF-8 BOMs.
  * Exception classes with enough context to format useful error messages. what() returns error messages ready to be shown to a user. 

## Getting Started

The following small example should contain most of the syntax you need to use the library.

```cpp
# include "csv.h"

int main(){
  io::CSVReader<3> in("ram.csv");
  in.read_header(io::ignore_extra_column, "vendor", "size", "speed");
  std::string vendor; int size; double speed;
  while(in.read_row(vendor, size, speed)){
    // do stuff with the data
  }
}
```

## Installation

The library only needs a standard conformant C++11 compiler. It has no further dependencies. The library is completely contained inside a single header file and therefore it is sufficient to copy this file to some place on your include path. The library does not have to be explicitly build. 

Note however, that threads are used and some compiler (for example GCC) require you to link against additional librarie to make it work. With GCC it is important to add -lpthread as the last item when linking, i.e. the order in 

```
g++ -std=c++0x a.o b.o -o prog -lpthread
```

is important. If you for some reason do not want to use threads you can define CSV_IO_NO_THREAD before including the header.

Remember that the library makes use of C++11 features and therefore you have to enable support for it (f.e. add -std=c++0x or -std=gnu++0x). 

The library was developed and tested with GCC 4.6.1

Note that VS2013 is not C++11 compilant and will therefore not work out of the box. See [here](https://code.google.com/p/fast-cpp-csv-parser/issues/detail?id=6) for what needs to be adjusted to make the code work.

## Documentation

The libary provides two classes: 

  * `LineReader`: A class to efficiently read large files line by line.
  * `CSVReader`: A class that efficiently reads large CSV files.

Note that everything is contained in the `io` namespace.

### `LineReader`

```cpp
class LineReader{
public:
  // Constructors
  LineReader(some_string_type file_name);
  LineReader(some_string_type file_name, std::FILE*source);
  LineReader(some_string_type file_name, std::istream&source);
  LineReader(some_string_type file_name, std::unique_ptr<ByteSourceBase>source);

  // Reading
  char*next_line();

  // File Location
  void set_file_line(unsigned);
  unsigned get_file_line(unsigned)const;
  void set_file_name(some_string_type file_name);
  const char*get_truncated_file_name()const;
};
```

The constructor takes a file name and optionally a data source. If no data source is provided the function tries to open the file with the given name and throws an `error::can_not_open_file exception` on failure. If a data source is provided then the file name is only used to format error messages. In that case you can essentially put any string there. Using a string that describes the data source results in more informative error messages.

`some_string_type` can be a `std::string` or a `char*`. If the data source is a `std::FILE*` then the library will take care of calling `std::fclose`. If it is a `std::istream` then the stream is not closed by the library. For best performance open the streams in binary mode. However using text mode also works. `ByteSourceBase` provides an interface that you can use to implement further data sources. 

```
class ByteSourceBase{
public:
  virtual int read(char*buffer, int size)=0;
  virtual ~ByteSourceBase(){}
};
```

The read function should fill the provided buffer with at most `size` bytes from the data source. It should return the number of bytes actually written to the buffer. If data source has run out of bytes (because for example an end of file was reached) then the function should return 0. If a fatal error occures then you can throw an exception. Note that the function can be called both from the main and the worker thread. However, it is guarenteed that they do not call the function at the same time. 

Lines are read by calling the `next_line` function. It returns a pointer to a null terminated C-string that contains the line. If the end of file is reached a null pointer is returned. The newline character is not included in the string. You may modify the string as long as you do not write past the null terminator. The string stays valid until the destructor is called or until next_line is called again. Windows and `*`nix newlines are handled transparently. UTF-8 BOMs are automatically ignored and missing newlines at the end of the file are no problem.

**Important:** There is a limit of 2^24-1 characters per line. If this limit is exceeded a `error::line_length_limit_exceeded` exception is thrown.

Looping over all the lines in a file can be done in the following way.
```cpp
LineReader in(...);
while(char*line = in.next_line()){
  ...
}
```

The remaining functions are mainly used used to format error messages. The file line indicates the current position in the file, i.e., after the first `next_line` call it is 1 and after the second 2. Before the first call it is 0. The file name is truncated as internally C-strings are used to avoid `std::bad_alloc` exceptions during error reporting.

**Note:** It is not possible to exchange the line termination character.

### `CSVReader`

`CSVReader` uses policies. These are classes with only static members to allow core functionality to be exchanged in an efficient way.

```cpp
template<
  unsigned column_count,
  class trim_policy = trim_chars<' ', '\t'>, 
  class quote_policy = no_quote_escape<','>,
  class overflow_policy = throw_on_overflow,
  class comment_policy = no_comment
>
class CSVReader{
public:
  // Constructors
  // same as for LineReader

  // Parsing Header
  void read_header(ignore_column ignore_policy, some_string_type col_name1, some_string_type col_name2, ...);
  void set_header(some_string_type col_name1, some_string_type col_name2, ...);
  bool has_column(some_string_type col_name)const;

  // Read
  char*next_line();
  bool read_row(ColType1&col1, ColType2&col2, ...);

  // File Location 
  void set_file_line(unsigned);
  unsigned get_file_line()const;
  void set_file_name(some_string_type file_name);
  const char*get_truncated_file_name()const;
};
```

The `column_count` template parameter indicates how many columns you want to read from the CSV file. This must not necessarily coincide with the actual number of columns in the file. The three policies govern various aspects of the parsing.

The trim policy indicates what characters should be ignored at the begin and the end of every column. The default ignores spaces and tabs. This makes sure that

```
a,b,c
1,2,3
```

is interpreted in the same way as

```
  a, b,   c
1  , 2,   3
```

The trim_chars can take any number of template parameters. For example `trim_chars<' ', '\t', '_'> `is also valid. If no character should be trimmed use `trim_chars<>`.

The quote policy indicates how string should be escaped. It also specifies the column separator. The predefined policies are:

  * `no_quote_escape<sep>` : Strings are not escaped. "`sep`" is used as column separator.
  * `double_quote_escape<sep, quote>` : Strings are escaped using quotes. Quotes are escaped using two consecutive quotes. "`sep`" is used as column separator and "`quote`" as quoting character.

**Important**: When combining trimming and quoting the rows are first trimmed and then unquoted. A consequence is that spaces inside the quotes will be conserved. If you want to get rid of spaces inside the quotes, you need to remove them yourself.

**Important**: Quoting can be quite expensive. Disable it if you do not need it.

The overflow policy indicates what should be done if the integers in the input are too large to fit into the variables. There following policies are predefined:

  * `throw_on_overflow` : Throw an `error::integer_overflow` or `error::integer_underflow` exception.
  * `ignore_overflow` : Do nothing and let the overflow happen.
  * `set_to_max_on_overflow` : Set the value to `numeric_limits<...>::max()` (or to the min-pendant).

The comment policy allows to skip lines based on some criteria. Valid predefined policies are:

  * `no_comment` : Do not ignore any line.
  * `empty_line_comment` : Ignore all lines that are empty or only contains spaces and tabs. 
  * `single_line_comment<com1, com2, ...>` : Ignore all lines that start with com1 or com2 or ... as the first character. There may not be any space between the beginning of the line and the comment character. 
  * `single_and_empty_line_comment<com1, com2, ...>` : Ignore all empty lines and single line comments.

Examples:

  * `CSVReader<4, trim_chars<' '>, double_quote_escape<',','\"'> >` reads 4 columns from a normal CSV file with string escaping enabled.
  * `CSVReader<3, trim_chars<' '>, no_quote_escape<'\t'>, single_line_comment<'#'> >` reads 3 columns from a tab separated file with string escaping disabled. Lines starting with a # are ignored.

The constructors and the file location functions are exactly the same as for `LineReader`. See its documentation for details.

There are three methods that deal with headers. The `read_header` methods reads a line from the file and rearranges the columns to match that order. It also checks whether all necessary columns are present. The `set_header` method does *not* read any input. Use it if the file does not have any header. Obviously it is impossible to rearrange columns or check for their availability when using it. The order in the file and in the program must match when using `set_header`. The `has_column` method checks whether a column is present in the file. The first argument of `read_header` is a bitfield that determines how the function should react to column mismatches. The default behavior is to throw an `error::extra_column_in_header` exception if the file contains more columns than expected and an `error::missing_column_in_header` when there are not enough. This behavior can be altered using the following flags.

  * `ignore_no_column`: The default behavior, no flags are set
  * `ignore_extra_column`: If a column with a name is in the file but not in the argument list, then it is silently ignored.
  * `ignore_missing_column`: If a column with a name is not in the file but is in the argument list, then `read_row` will not modify the corresponding variable. 

When using `ignore_column_missing` it is a good idea to initialize the variables passed to `read_row` with a default value, for example:

```cpp
// The file only contains column "a"
CSVReader<2>in(...);
in.read_header(ignore_missing_column, "a", "b");
int a,b = 42;
while(in.read_row(a,b)){
  // a contains the value from the file
  // b is left unchanged by read_row, i.e., it is 42
}
```

If only some columns are optional or their default value depends on other columns you have to use `has_column`, for example:

```cpp
// The file only contains the columns "a" and "b"
CSVReader<2>in(...);
in.read_header(ignore_missing_column, "a", "b", "sum");
if(!in.has_column("a") || !in.has_column("b"))
  throw my_neat_error_class();
bool has_sum = in.has_column("sum");
int a,b,sum;
while(in.read_row(a,b,sum)){
  if(!has_sum)
    sum = a+b;
}
```

**Important**: Do not call `has_column` from within the read-loop. It would work correctly but significantly slowdown processing.

If two columns have the same name an error::duplicated_column_in_header exception is thrown. If `read_header` is called but the file is empty a `error::header_missing` exception is thrown.

The `next_line` functions reads a line without parsing it. It works analogous to `LineReader::next_line`. This can be used to skip broken lines in a CSV file. However, in nearly all applications you will want to use the `read_row` function.

The `read_row` function reads a line, splits it into the columns and arranges them correctly. It trims the entries and unescapes them. If requested the content is interpreted as integer or as floating point. The variables passed to read_row may be of the following types.

  * builtin signed integer: These are `signed char`, `short`, `int`, `long` and `long long`. The input must be encoded as a base 10 ASCII number optionally preceded by a + or -. The function detects whether the integer is too large would overflow (or underflow) and behaves as indicated by overflow_policy.
  * builtin unsigned integer: Just as the signed counterparts except that a leading + or - is not allowed.
  * builtin floating point: These are `float`, `double` and `long double`. The input may have a leading + or -. The number must be base 10 encoded. The decimal point may either be a dot or a comma. (Note that a comma will only work if it is not also used as column separator or the number is escaped.) A base 10 exponent may be specified using the "1e10" syntax. The "e" may be lower- or uppercase. Examples for valid floating points are "1", "-42.42" and "+123.456E789". The input is rounded to the next floating point or infinity if it is too large or small.
  * `char`: The column content must be a single character.
  * `std::string`: The column content is assigned to the string. The std::string is filled with the trimmed and unescaped version.
  * `char*`: A pointer directly into the buffer. The string is trimmed and unescaped and null terminated. This pointer stays valid until read_row is called again or the CSVReader is destroyed. Use this for user defined types. 

Note that there is no inherent overhead to using `char*` and then interpreting it compared to using one of the parsers directly build into `CSVReader`. The builtin number parsers are pure convenience. If you need a slightly different syntax then use `char*` and do the parsing yourself.

## FAQ

Q: The library is throwing a std::system_error with code -1. How to get it to work?

A: Your compiler's std::thread implementation is broken. Define CSV\_IO\_NO\_THREAD to disable threading support.


Q: My values are not just ints or strings. I want to parse my customized type. Is this possible?

A: Read a `char*` and parse the string. At first this seems expensive but it is not as the pointer you get points directly into the memory buffer. In fact there is no inherent reason why a custom int-parser realized this way must be any slower than the int-parser build into the library. By reading a `char*` the library takes care of column reordering and quote escaping and leaves the actual parsing to you. Note that using a std::string is slower as it involves a memory copy.


Q: I get lots of compiler errors when compiling the header! Please fix it. :(

A: Have you enabled the C++11 mode of your compiler? If you use GCC you have to add -std=c++0x to the commandline. If this does not resolve the problem, then please open a ticket.


Q: The library crashes when parsing large files! Please fix it. :(

A: When using GCC have you linked against -lpthread? Read the installation section for details on how to do this. If this does not resolve the issue then please open a ticket. (The reason why it only crashes only on large files is that the first chuck is read synchronous and if the whole file fits into this chuck then no asynchronous call is performed.) Alternatively you can define CSV\_IO\_NO\_THREAD.


Q: Does the library support UTF?

A: The library has basic UTF-8 support, or to be more precise it does not break when passing UTF-8 strings through it. If you read a `char*` then you get a pointer to the UTF-8 string. You will have to decode the string on your own. The separator, quoting, and commenting characters used by the library can only be ASCII characters.
