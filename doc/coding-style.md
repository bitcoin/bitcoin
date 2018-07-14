# Coding Style Guide

The rules and conventions described on this page must be followed in all new code. When modifying old code it's advisable to follow them within reason and with respect to style consistency.

# Style

## File organization

### Copyright header

Every file must have a typical copyright header. Files inherited from Bitcoin and Dash must include respective copyrights as well.

```c++
// Copyright (c) 2014-<this year> Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php
```

### File Names

Filenames should be all lowercase and can include dashes (`-`). C++ files should end in `.cpp` and header files should end in `.h`.

### Header Guards

All header files should have `#define` guards to prevent multiple inclusion. The format of the symbol name should be `<PATH>_<FILE>_H`. To guarantee uniqueness, they should be based on the full path in a project's source tree. 

```c++
#ifndef FOO_BAR_BAZ_H
#define FOO_BAR_BAZ_H

...

#endif
```

## Formatting

### Braces placement

Braces in namespaces, classes, functions, enums, unions, try-catch blocks, loops and if statements are always placed on the next line. When there is an if statement with a single line in the conditional branch, no braces are required. However, statements that follow the condition must be on a separate line.
In loops braces are always required.

```c++
namespace MyNamespace
{
    class MyClass
    {
    };

    void MyFunction()
    {
        if (...)
            return;
            
        for (auto element: container)
        {
            DoSomethingWith(element);
        }
    }
}
```



### Indentation

Indentation step is 4 spaces; spaces must be used instead of tabs.

### Class sections

Sections in class definition should be in the following order: `public` - `protected` - `private`. Make separate sections for member functions and data members. Within each section, generally prefer grouping similar kinds of declarations together, and generally prefer the following order: types (including `typedef`,
`using`, and nested structs and classes), constants, factory functions, constructors, assignment operators, destructor, all other methods, data members.

```c++
class MyClass
{
public:
    void PublicMethod();
    
protected:
	void ProtectedMethod();
	
private:
    void PrivateMethod();
    
private:
    std::string m_dataMember;
}
```

### Line length

Break long lines of code. Try to keep them inside reasonable bounds, say, 100-120 chars per line.

## Naming

Give as descriptive a name as possible, within reason.

Names of all identifiers are spelled in camel case (except for the cases where all capitals are used). Abbreviations are spelled with first upper case letter and the rest in lower case.

Prefixing conventions such as putting "C" in front of each class or using Hungarian notation are discouraged.

```c++
class XmlParser
{ 
    ... 
};
void DisplayRtfmMessage();
```



### Variables

| Visibility          | Variable      | Constant                      | Compile-time constant (`constexpr`) |
| ------------------- | ------------- | ----------------------------- | ----------------------------------- |
| Local               | `camelCase`   | `camelCase`                   | `camelCase`                         |
| Static              | `s_camelCase` | `camelCase`                   | `camelCase`                         |
| Class member        | `m_camelCase` | `m_camelCase`                 | `m_camelCase`                       |
| Static class member | `m_camelCase` | `m_camelCase`                 | `m_camelCase`                       |
| Global              | `g_camelCase` | `ALL_CAPITALS` or `camelCase` | `ALL_CAPITALS` or `camelCase`       |

### Enums

Enum types are named in camel case with first capital letter. Enum values follow constant naming conventions: either camel case or all capitals in case of globally visible 

```c++
class SystemnodeDb
{
    enum ReadResult {
        Ok,
        ...
    };
};

enum Network
{
    NET_UNROUTABLE = 0,
    ...
};
```



### Macros

Macros are named in all capitals with underscores.

```c++
#define DO_NOT_USE_ME true
```

### Functions

Both free functions and class member functions are named in camel case with first uppercase letter. The only exception is inheriting from a third-party class which follows a different naming convention, for example, when working with Qt

```c++
class SomeClass
{
    void SomeMethod()
    {
    }
};

class ExceptionalCase: public QObject
{
  virtual void redefinedQtMethod() override
  {
  }
  
  void myBrandNewMethod() 
  {
  }
}
```

### Classes and Structures

Classes are named in camel case with first uppercase letter.

### Namespaces

Namespaces are named in camel case with first uppercase letter. When declaring nested namespaces C++17-style nested namespace declaration is preferred.

```c++
namespace Crw::Net
{
    class Node
    {};
};
```

# Rules and Best Practices

More rules TBD

### Namespaces

Do not use *using-directives* (e.g.`using namespace foo`) in `.h` files. It's acceptable to use it in `.cpp` files, especially with `std::literals`, but try to keep those directives as local as possible.

### Unnamed Namespaces and StaticVariables

When definitions in a `.cpp` file do not need to be referenced outside that file, place them in an unnamed namespace. Prefere unnamed namespaces to declaring them `static`. Do not use either of these constructs in `.h` files

### Preprocessor Macros

Avoid using macros; prefer inline functions, enums, `const` and `constexpr` variables.

# Further Reading
These are books and resources that considered to be good advice on coding style and best practices. It's preferable to follow their advice when it's reasonable:
1. Herb Sutter, Andrei Alexandrescu "C++ Coding Standards: 101 Rules, Guidelines, and Best Practices "
2. Scott Meyers "Effective Modern C++"
3. Bjarne Stroustrup, Herb Sutter ["C++ Core Guidelines"](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
