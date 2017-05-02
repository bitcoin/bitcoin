--------------------------------------------------------------------------------

# Coding Standards

Coding standards used here are extreme strict and consistent. The style
evolved gradually over the years, incorporating generally acknowledged
best-practice C++ advice, experience, and personal preference.

## Don't Repeat Yourself!

The [Don't Repeat Yourself][1] principle summarises the essence of what it
means to write good code, in all languages, at all levels.

## Formatting

The goal of source code formatting should always be to make things as easy to
read as possible. White space is used to guide the eye so that details are not
overlooked. Blank lines are used to separate code into "paragraphs."

* No tab characters please.
* Tab stops are set to 4 spaces.
* Braces are indented in the [Allman style][2].
* Always place a space before and after all binary operators,
  especially assignments (`operator=`).
* The `!` operator should always be followed by a space.
* The `~` operator should be preceded by a space, but not followed by one.
* The `++` and `--` operators should have no spaces between the operator and
  the operand.
* A space never appears before a comma, and always appears after a comma.
* Always place a space before an opening parenthesis. One exception is if
  the parentheses are empty.
* Don't put spaces after a parenthesis. A typical member function call might
  look like this: `foobar (1, 2, 3);`
* In general, leave a blank line before an `if` statement.
* In general, leave a blank line after a closing brace `}`.
* Do not place code or comments on the same line as any opening or
  closing brace.
* Do not write `if` statements all-on-one-line. The exception to this is when
  you've got a sequence of similar `if` statements, and are aligning them all
  vertically to highlight their similarities.
* In an `if-else` statement, if you surround one half of the statement with
  braces, you also need to put braces around the other half, to match.
* When writing a pointer type, use this spacing: `SomeObject* myObject`.
  Technically, a more correct spacing would be `SomeObject *myObject`, but
  it makes more sense for the asterisk to be grouped with the type name,
  since being a pointer is part of the type, not the variable name. The only
  time that this can lead to any problems is when you're declaring multiple
  pointers of the same type in the same statement - which leads on to the next
  rule:
* When declaring multiple pointers, never do so in a single statement, e.g.
  `SomeObject* p1, *p2;` - instead, always split them out onto separate lines
  and write the type name again, to make it quite clear what's going on, and
  avoid the danger of missing out any vital asterisks.
* The previous point also applies to references, so always put the `&` next to
  the type rather than the variable, e.g. `void foo (Thing const& thing)`. And
  don't put a space on both sides of the `*` or `&` - always put a space after
  it, but never before it.
* The word `const` should be placed to the right of the thing that it modifies,
  for consistency. For example `int const` refers to an int which is const.
  `int const*` is a pointer to an int which is const. `int *const` is a const
  pointer to an int.
* Always place a space in between the template angle brackets and the type
  name. Template code is already hard enough to read!

## Naming conventions

* Member variables and method names are written with camel-case, and never
  begin with a capital letter.
* Class names are also written in camel-case, but always begin with a capital
  letter.
* For global variables... well, you shouldn't have any, so it doesn't matter.
* Class data members begin with `m_`, static data members begin with `s_`.
  Global variables begin with `g_`. This is so the scope of the corresponding
  declaration can be easily determined.
* Avoid underscores in your names, especially leading or trailing underscores.
  In particular, leading underscores should be avoided, as these are often used
  in standard library code, so to use them in your own code looks quite jarring.
* If you really have to write a macro for some reason, then make it all caps,
  with underscores to separate the words. And obviously make sure that its name
  is unlikely to clash with symbols used in other libraries or 3rd party code.

## Types, const-correctness

* If a method can (and should!) be const, make it const!
* If a method definitely doesn't throw an exception (be careful!), mark it as
  `noexcept`
* When returning a temporary object, e.g. a String, the returned object should
  be non-const, so that if the class has a C++11 move operator, it can be used.
* If a local variable can be const, then make it const!
* Remember that pointers can be const as well as primitives; For example, if
  you have a `char*` whose contents are going to be altered, you may still be
  able to make the pointer itself const, e.g. `char* const foobar = getFoobar();`.
* Do not declare all your local variables at the top of a function or method
  (i.e. in the old-fashioned C-style). Declare them at the last possible moment,
  and give them as small a scope as possible.
* Object parameters should be passed as `const&` wherever possible. Only
  pass a parameter as a copy-by-value object if you really need to mutate
  a local copy inside the method, and if making a local copy inside the method
  would be difficult.
* Use portable `for()` loop variable scoping (i.e. do not have multiple for
  loops in the same scope that each re-declare the same variable name, as
  this fails on older compilers)
* When you're testing a pointer to see if it's null, never write
  `if (myPointer)`. Always avoid that implicit cast-to-bool by writing it more
  fully: `if (myPointer != nullptr)`. And likewise, never ever write
  `if (! myPointer)`, instead always write `if (myPointer == nullptr)`.
  It is more readable that way.
* Avoid C-style casts except when converting between primitive numeric types.
  Some people would say "avoid C-style casts altogether", but `static_cast` is
  a bit unreadable when you just want to cast an `int` to a `float`. But
  whenever a pointer is involved, or a non-primitive object, always use
  `static_cast`. And when you're reinterpreting data, always use
  `reinterpret_cast`.
* Until C++ gets a universal 64-bit primitive type (part of the C++11
  standard), it's best to stick to the `int64` and `uint64` typedefs.

## Object lifetime and ownership

* Absolutely do NOT use `delete`, `deleteAndZero`, etc. There are very very few
  situations where you can't use a `ScopedPointer` or some other automatic
  lifetime management class.
* Do not use `new` unless there's no alternative. Whenever you type `new`, always
  treat it as a failure to find a better solution. If a local variable can be
  allocated on the stack rather than the heap, then always do so.
* Do not ever use `new` or `malloc` to allocate a C++ array. Always use a
  `HeapBlock` instead.
* And just to make it doubly clear: Never use `malloc` or `calloc`.
* If a parent object needs to create and own some kind of child object, always
  use composition as your first choice. If that's not possible (e.g. if the
  child needs a pointer to the parent for its constructor), then use a
  `ScopedPointer`.
* If possible, pass an object as a reference rather than a pointer. If possible,
  make it a `const` reference.
* Obviously avoid static and global values. Sometimes there's no alternative,
  but if there is an alternative, then use it, no matter how much effort it
  involves.
* If allocating a local POD structure (e.g. an operating-system structure in
  native code), and you need to initialise it with zeros, use the `= { 0 };`
  syntax as your first choice for doing this. If for some reason that's not
  appropriate, use the `zerostruct()` function, or in case that isn't suitable,
  use `zeromem()`. Don't use `memset()`.

## Classes

* Declare a class's public section first, and put its constructors and
  destructor first. Any protected items come next, and then private ones.
* Use the most restrictive access-specifier possible for each member. Prefer
  `private` over `protected`, and `protected` over `public`. Don't expose
  things unnecessarily.
* Preferred positioning for any inherited classes is to put them to the right
  of the class name, vertically aligned, e.g.:
        class Thing  : public Foo, 
                       private Bar
        {
        }
* Put a class's member variables (which should almost always be private, of course),
  after all the public and protected method declarations.
* Any private methods can go towards the end of the class, after the member
  variables.
* If your class does not have copy-by-value semantics, derive the class from
  `Uncopyable`.
* If your class is likely to be leaked, then derive your class from
  `LeakChecked<>`.
* Constructors that take a single parameter should be default be marked
  `explicit`. Obviously there are cases where you do want implicit conversion,
  but always think about it carefully before writing a non-explicit constructor.
* Do not use `NULL`, `null`, or 0 for a null-pointer. And especially never use
  '0L', which is particulary burdensome. Use `nullptr` instead - this is the
  C++2011 standard, so get used to it. There's a fallback definition for `nullptr`
  in Beast, so it's always possible to use it even if your compiler isn't yet
  C++2011 compliant.
* All the C++ 'guru' books and articles are full of excellent and detailed advice
  on when it's best to use inheritance vs composition. If you're not already
  familiar with the received wisdom in these matters, then do some reading!

## Miscellaneous

* `goto` statements should not be used at all, even if the alternative is
  more verbose code. The only exception is when implementing an algorithm in
  a function as a state machine.
* Don't use macros! OK, obviously there are many situations where they're the
  right tool for the job, but treat them as a last resort. Certainly don't ever
  use a macro just to hold a constant value or to perform any kind of function
  that could have been done as a real inline function. And it goes without saying
  that you should give them names which aren't going to clash with other code.
  And #undef them after you've used them, if possible.
* When using the ++ or -- operators, never use post-increment if pre-increment
  could be used instead. Although it doesn't matter for primitive types, it's good
  practice to pre-increment since this can be much more efficient for more complex
  objects. In particular, if you're writing a for loop, always use pre-increment,
  e.g. `for (int = 0; i < 10; ++i)`
* Never put an "else" statement after a "return"! This is well-explained in the
  LLVM coding standards...and a couple of other very good pieces of advice from
  the LLVM standards are in there as well.
* When getting a possibly-null pointer and using it only if it's non-null, limit
  the scope of the pointer as much as possible - e.g. Do NOT do this:

        Foo* f = getFoo ();
        if (f != nullptr)
            f->doSomething ();
        // other code
        f->doSomething ();  // oops! f may be null!

  ..instead, prefer to write it like this, which reduces the scope of the
  pointer, making it impossible to write code that accidentally uses a null
  pointer:

        if (Foo* f = getFoo ())
            f->doSomethingElse ();

        // f is out-of-scope here, so impossible to use it if it's null

  (This also results in smaller, cleaner code)

[1]: http://en.wikipedia.org/wiki/Don%27t_repeat_yourself
[2]: http://en.wikipedia.org/wiki/Indent_style#Allman_style
