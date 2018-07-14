# Unit Tests

## Compiling and Running

Unit tests will be automatically compiled if dependencies were met in `configure` and tests weren't explicitly disabled. After configuring, they can be run with `make check`. To run the main suit of unit tests manually, launch `src/test/test_crown`. To run Qt tests manually, launch `src/qt/test/crown-qt_test`.

## Writing New Tests

Unit tests are great at catching regressions and they can serve as documentation. But the main benefit comes from using them as a development tool. The fast feedback that tests provide allows you to make quick (and, if needed, radical) changes without being affraid of breaking anything. And besides, committing code withoug accompanying it with tests is just rude to your fellow developers. 

When you are adding new functionality it's highly recommended to *siumltaniously* write a test for it. Writing tests as you develop (as opposed to writing them after the fact) helps you produce better code.

When you are chainging existing functionality or fixing a bug it's recommended to follow this procedure:
1. Identify what code you need to change
2. Write tests (if necessary, break dependencies first)
3. Implement the change
4. Refactor

[Boost.Test](https://www.boost.org/doc/libs/1_67_0/libs/test/doc/html/index.html) is used as a unit-testing framework. Please read the documentation, if you are not familiar with it. In a nutshell: to add more unit tests tests, add `BOOST_AUTO_TEST_CASE` functions to the existing `.cpp` files in the `test` directory or add new `.cpp` files that implement new BOOST_AUTO_TEST_SUITE sections. 

Typically a unit-test follows this structure:
1. Setup (if several tests require (almost) identical setup, consider using a fixture; setup in an individual test case should be clear in terms of what is different about this particular case)
2. Execution
3. Validation (including validating return values of a called function, state of the object being tested and changes to the global environment)
4. Teardown (usually is taken care of by the routine use of RAII, but the gloval enrironment might require manual cleanup)

Try keeping tests short and clean; put as much effort in their design as you would put in the design of production code. Make sure tests run fast. Recognize that tests, as any other code, need to be refactored from time to time.

To add more Qt tests, add them to the `src/qt/test/` directory and the `src/qt/test/test_main.cpp` file.

## Further Reading

1. Michael Feathers "Working Effectively with Legacy Code"
2. Gerard Meszaros "XUnit Test Patterns: Refactoring Test Code"
3. Steve Freeman, Nat Pryce "Growing Object-Oriented Software, Guided by Tests"

