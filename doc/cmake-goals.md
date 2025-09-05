# CMake Goals and Guidelines

Every CMake project must assume it could be used as a subproject of a larger
build. It is crucial that certain guidelines are followed so that a project does
not unintentionally modify or override settings in the parent project, and so
that configuration provided by the user, build scripts, or parent project is
always respected.

Variables that start with `CMAKE_`, as well as `BUILD_SHARED_LIBS`, must never
be set in `CMakeLists.txt` in a way that affects the current project. These
variables are reserved for external configuration.

## Default Build Type

Projects must treat the build type as read-only. Do not set a default value when
`CMAKE_BUILD_TYPE` is unset — the empty build type is intentional and must be
respected. Never override `CMAKE_CONFIGURATION_TYPES`. The Ninja Multi-Config
generator sets this to three values rather than four for a reason. Do not set
the `STRINGS` property on the cache value `CMAKE_BUILD_TYPE`. While a drop-down
menu may look appealing in the GUI, it is important to allow users to type
arbitrary strings in this field.

Developers may set `CMAKE_BUILD_TYPE`, `CMAKE_CONFIGURATION_TYPES`,
`CMAKE_GENERATOR`, and other environment variables in their dotfiles if
customization is desired.

Build scripts must set these variables either as environment variables or via the
CMake command line, not within `CMakeLists.txt`.

## Custom Build Types

CMake initializes compile flags by appending hardcoded flags for built-in build
types, such as `-O3` for the `Release` build type, to language-specific
environment variables like `CFLAGS` or `CXXFLAGS`. If this behavior is not
desired, the best workaround is to avoid selecting any of the built-in build
types. This is exactly why leaving the build type empty is a valid use case that
projects must support.

Build scripts may also define custom build types, such as `CustomRelease`, to
gain full control over compile flags for different configurations. It is
recommended to define such custom build types, often also used for coverage and
sanitizer builds, in a toolchain file so that it can be shared across multiple
projects.

Projects must not make any assumptions about the exact spelling or set of build
types. They also have no reason to define a custom build type in a
`CMakeLists.txt` file.

## Compile Flags

Compile flags that are absolutely necessary for a target to build must be set at
the target level using `target_compile_options()` or
`target_compile_features()`.

Do not add compile flags for style, warnings, or personal preferences. A compile
flag that does not cause an error when omitted cannot be considered necessary.
Such variables may be set in a toolchain file, in environment variables, or in
the build script, but not in `CMakeLists.txt` files.

## Options and Cache Variables

Options must be used exclusively for enabling or disabling components that
require extra dependencies. Do not introduce options for features that do not
affect dependencies, as each additional option increases build complexity
exponentially.

All custom cache variables must be prefixed with the project name (e.g.,
`BitcoinCore_ENABLE_GUI`). This prevents naming collisions when your project is
used as a dependency in other projects and makes it clear which project the
option belongs to.

No option is needed for enabling or disabling installation. A superproject may
want to exclude the installation of a subproject, but this is already the
default behavior when `EXCLUDE_FROM_ALL` is passed to the `add_subdirectory()`
call to add the subproject.

## Targets

For every executable or library, an ALIAS target must be defined in the
namespace that matches the project. The non-namespaced name must only be used
within the current `CMakeLists.txt` file. In all other places, only the
namespaced (ALIAS) name is permitted. This implies that target properties cannot
be modified from outside the defining `CMakeLists.txt` file.

Adding include directories to a target should be done with
`target_sources(FILE_SET)`. It may be done with `target_include_directories()`
for backwards compatibility with CMake versions earlier than 3.23, but that
command must not be used in situations that cannot be implemented with
`target_sources(FILE_SET)` in later versions.

Each header file must be associated with a single target. The only way to
`#include` a header file which belongs to another target is by declaring a
dependency on that target with `target_link_libraries()`.

Do not collect sources in a variable to be passed to `add_executable()` or
`add_library()`. Instead, declare the target first by calling one of these
commands with no sources, and then add sources using the `target_sources()`
command.

Where the list of sources depends on the platform, call `target_sources()`
conditionally. Do not use generator expressions for this purpose.

Avoid consecutive `target_*` commands when there is no condition involved.
Add multiple dependencies with a single call to `target_link_libraries()`.
Add multiple file sets with a single `target_sources()` command. This makes the
`CMakeLists.txt` files look declarative rather than like procedural code.

## Custom Targets

Do not define custom "utility" or "convenience" targets for executing tests or
performing installation with extra options. Such targets tend to accumulate
technical debt and tribal knowledge. Developers should prefer calling `ctest`
or `cmake --install` directly and follow the documentation provided in the CMake
manual.

Also, do not define targets with the intention of invoking them explicitly with
`cmake --build` from a build script. If building a target is necessary for
other parts of the build, set up proper dependencies so that the target is built
implicitly and rebuilt when its inputs change.

If a target exists solely to execute a script that produces output not used by
other targets or installation, consider defining it as a test instead. For
static analysis tools like `clang-tidy`, do not create custom targets or tests;
use CMake's built-in support.

## Install and Export
