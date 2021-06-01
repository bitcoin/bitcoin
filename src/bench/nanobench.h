//  __   _ _______ __   _  _____  ______  _______ __   _ _______ _     _
//  | \  | |_____| | \  | |     | |_____] |______ | \  | |       |_____|
//  |  \_| |     | |  \_| |_____| |_____] |______ |  \_| |_____  |     |
//
// Microbenchmark framework for C++11/14/17/20
// https://github.com/martinus/nanobench
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 Martin Ankerl <martin.ankerl@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ANKERL_NANOBENCH_H_INCLUDED
#define ANKERL_NANOBENCH_H_INCLUDED

// see https://semver.org/
#define ANKERL_NANOBENCH_VERSION_MAJOR 4 // incompatible API changes
#define ANKERL_NANOBENCH_VERSION_MINOR 0 // backwards-compatible changes
#define ANKERL_NANOBENCH_VERSION_PATCH 0 // backwards-compatible bug fixes

///////////////////////////////////////////////////////////////////////////////////////////////////
// public facing api - as minimal as possible
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <chrono>  // high_resolution_clock
#include <cstring> // memcpy
#include <iosfwd>  // for std::ostream* custom output target in Config
#include <string>  // all names
#include <vector>  // holds all results

#define ANKERL_NANOBENCH(x) ANKERL_NANOBENCH_PRIVATE_##x()

#define ANKERL_NANOBENCH_PRIVATE_CXX() __cplusplus
#define ANKERL_NANOBENCH_PRIVATE_CXX98() 199711L
#define ANKERL_NANOBENCH_PRIVATE_CXX11() 201103L
#define ANKERL_NANOBENCH_PRIVATE_CXX14() 201402L
#define ANKERL_NANOBENCH_PRIVATE_CXX17() 201703L

#if ANKERL_NANOBENCH(CXX) >= ANKERL_NANOBENCH(CXX17)
#    define ANKERL_NANOBENCH_PRIVATE_NODISCARD() [[nodiscard]]
#else
#    define ANKERL_NANOBENCH_PRIVATE_NODISCARD()
#endif

#if defined(__clang__)
#    define ANKERL_NANOBENCH_PRIVATE_IGNORE_PADDED_PUSH() \
        _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wpadded\"")
#    define ANKERL_NANOBENCH_PRIVATE_IGNORE_PADDED_POP() _Pragma("clang diagnostic pop")
#else
#    define ANKERL_NANOBENCH_PRIVATE_IGNORE_PADDED_PUSH()
#    define ANKERL_NANOBENCH_PRIVATE_IGNORE_PADDED_POP()
#endif

#if defined(__GNUC__)
#    define ANKERL_NANOBENCH_PRIVATE_IGNORE_EFFCPP_PUSH() _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Weffc++\"")
#    define ANKERL_NANOBENCH_PRIVATE_IGNORE_EFFCPP_POP() _Pragma("GCC diagnostic pop")
#else
#    define ANKERL_NANOBENCH_PRIVATE_IGNORE_EFFCPP_PUSH()
#    define ANKERL_NANOBENCH_PRIVATE_IGNORE_EFFCPP_POP()
#endif

#if defined(ANKERL_NANOBENCH_LOG_ENABLED)
#    include <iostream>
#    define ANKERL_NANOBENCH_LOG(x) std::cout << __FUNCTION__ << "@" << __LINE__ << ": " << x << std::endl
#else
#    define ANKERL_NANOBENCH_LOG(x)
#endif

#if defined(__linux__) && !defined(ANKERL_NANOBENCH_DISABLE_PERF_COUNTERS)
#    define ANKERL_NANOBENCH_PRIVATE_PERF_COUNTERS() 1
#else
#    define ANKERL_NANOBENCH_PRIVATE_PERF_COUNTERS() 0
#endif

#if defined(__clang__)
#    define ANKERL_NANOBENCH_NO_SANITIZE(...) __attribute__((no_sanitize(__VA_ARGS__)))
#else
#    define ANKERL_NANOBENCH_NO_SANITIZE(...)
#endif

#if defined(_MSC_VER)
#    define ANKERL_NANOBENCH_PRIVATE_NOINLINE() __declspec(noinline)
#else
#    define ANKERL_NANOBENCH_PRIVATE_NOINLINE() __attribute__((noinline))
#endif

// workaround missing "is_trivially_copyable" in g++ < 5.0
// See https://stackoverflow.com/a/31798726/48181
#if defined(__GNUC__) && __GNUC__ < 5
#    define ANKERL_NANOBENCH_IS_TRIVIALLY_COPYABLE(...) __has_trivial_copy(__VA_ARGS__)
#else
#    define ANKERL_NANOBENCH_IS_TRIVIALLY_COPYABLE(...) std::is_trivially_copyable<__VA_ARGS__>::value
#endif

// declarations ///////////////////////////////////////////////////////////////////////////////////

namespace ankerl {
namespace nanobench {

using Clock = std::conditional<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock,
                               std::chrono::steady_clock>::type;
class Bench;
struct Config;
class Result;
class Rng;
class BigO;

/**
 * @brief Renders output from a mustache-like template and benchmark results.
 *
 * The templating facility here is heavily inspired by [mustache - logic-less templates](https://mustache.github.io/).
 * It adds a few more features that are necessary to get all of the captured data out of nanobench. Please read the
 * excellent [mustache manual](https://mustache.github.io/mustache.5.html) to see what this is all about.
 *
 * nanobench output has two nested layers, *result* and *measurement*.  Here is a hierarchy of the allowed tags:
 *
 * * `{{#result}}` Marks the begin of the result layer. Whatever comes after this will be instantiated as often as
 *   a benchmark result is available. Within it, you can use these tags:
 *
 *    * `{{title}}` See Bench::title().
 *
 *    * `{{name}}` Benchmark name, usually directly provided with Bench::run(), but can also be set with Bench::name().
 *
 *    * `{{unit}}` Unit, e.g. `byte`. Defaults to `op`, see Bench::title().
 *
 *    * `{{batch}}` Batch size, see Bench::batch().
 *
 *    * `{{complexityN}}` Value used for asymptotic complexity calculation. See Bench::complexityN().
 *
 *    * `{{epochs}}` Number of epochs, see Bench::epochs().
 *
 *    * `{{clockResolution}}` Accuracy of the clock, i.e. what's the smallest time possible to measure with the clock.
 *      For modern systems, this can be around 20 ns. This value is automatically determined by nanobench at the first
 *      benchmark that is run, and used as a static variable throughout the application's runtime.
 *
 *    * `{{clockResolutionMultiple}}` Configuration multiplier for `clockResolution`. See Bench::clockResolutionMultiple().
 *      This is the target runtime for each measurement (epoch). That means the more accurate your clock is, the faster
 *      will be the benchmark. Basing the measurement's runtime on the clock resolution is the main reason why nanobench is so fast.
 *
 *    * `{{maxEpochTime}}` Configuration for a maximum time each measurement (epoch) is allowed to take. Note that at least
 *      a single iteration will be performed, even when that takes longer than maxEpochTime. See Bench::maxEpochTime().
 *
 *    * `{{minEpochTime}}` Minimum epoch time, usually not set. See Bench::minEpochTime().
 *
 *    * `{{minEpochIterations}}` See Bench::minEpochIterations().
 *
 *    * `{{epochIterations}}` See Bench::epochIterations().
 *
 *    * `{{warmup}}` Number of iterations used before measuring starts. See Bench::warmup().
 *
 *    * `{{relative}}` True or false, depending on the setting you have used. See Bench::relative().
 *
 *    Apart from these tags, it is also possible to use some mathematical operations on the measurement data. The operations
 *    are of the form `{{command(name)}}`.  Currently `name` can be one of `elapsed`, `iterations`. If performance counters
 *    are available (currently only on current Linux systems), you also have `pagefaults`, `cpucycles`,
 *    `contextswitches`, `instructions`, `branchinstructions`, and `branchmisses`. All the measuers (except `iterations`) are
 *    provided for a single iteration (so `elapsed` is the time a single iteration took). The following tags are available:
 *
 *    * `{{median(<name>>)}}` Calculate median of a measurement data set, e.g. `{{median(elapsed)}}`.
 *
 *    * `{{average(<name>)}}` Average (mean) calculation.
 *
 *    * `{{medianAbsolutePercentError(<name>)}}` Calculates MdAPE, the Median Absolute Percentage Error. The MdAPE is an excellent
 *      metric for the variation of measurements. It is more robust to outliers than the
 *      [Mean absolute percentage error (M-APE)](https://en.wikipedia.org/wiki/Mean_absolute_percentage_error).
 *      @f[
 *       \mathrm{medianAbsolutePercentError}(e) = \mathrm{median}\{| \frac{e_i - \mathrm{median}\{e\}}{e_i}| \}
 *      @f]
 *      E.g. for *elapsed*: First, @f$ \mathrm{median}\{elapsed\} @f$ is calculated. This is used to calculate the absolute percentage
 *      error to this median for each measurement, as in  @f$ | \frac{e_i - \mathrm{median}\{e\}}{e_i}| @f$. All these results
 *      are sorted, and the middle value is chosen as the median absolute percent error.
 *
 *      This measurement is a bit hard to interpret, but it is very robust against outliers. E.g. a value of 5% means that half of the
 *      measurements deviate less than 5% from the median, and the other deviate more than 5% from the median.
 *
 *    * `{{sum(<name>)}}` Sums of all the measurements. E.g. `{{sum(iterations)}}` will give you the total number of iterations
*        measured in this benchmark.
 *
 *    * `{{minimum(<name>)}}` Minimum of all measurements.
 *
 *    * `{{maximum(<name>)}}` Maximum of all measurements.
 *
 *    * `{{sumProduct(<first>, <second>)}}` Calculates the sum of the products of corresponding measures:
 *      @f[
 *          \mathrm{sumProduct}(a,b) = \sum_{i=1}^{n}a_i\cdot b_i
 *      @f]
 *      E.g. to calculate total runtime of the benchmark, you multiply iterations with elapsed time for each measurement, and
 *      sum these results up:
 *      `{{sumProduct(iterations, elapsed)}}`.
 *
 *    * `{{#measurement}}` To access individual measurement results, open the begin tag for measurements.
 *
 *       * `{{elapsed}}` Average elapsed time per iteration, in seconds.
 *
 *       * `{{iterations}}` Number of iterations in the measurement. The number of iterations will fluctuate due
 *         to some applied randomness, to enhance accuracy.
 *
 *       * `{{pagefaults}}` Average number of pagefaults per iteration.
 *
 *       * `{{cpucycles}}` Average number of CPU cycles processed per iteration.
 *
 *       * `{{contextswitches}}` Average number of context switches per iteration.
 *
 *       * `{{instructions}}` Average number of retired instructions per iteration.
 *
 *       * `{{branchinstructions}}` Average number of branches executed per iteration.
 *
 *       * `{{branchmisses}}` Average number of branches that were missed per iteration.
 *
 *    * `{{/measurement}}` Ends the measurement tag.
 *
 * * `{{/result}}` Marks the end of the result layer. This is the end marker for the template part that will be instantiated
 *   for each benchmark result.
 *
 *
 *  For the layer tags *result* and *measurement* you additionally can use these special markers:
 *
 *  * ``{{#-first}}`` - Begin marker of a template that will be instantiated *only for the first* entry in the layer. Use is only
 *    allowed between the begin and end marker of the layer allowed. So between ``{{#result}}`` and ``{{/result}}``, or between
 *    ``{{#measurement}}`` and ``{{/measurement}}``. Finish the template with ``{{/-first}}``.
 *
 *  * ``{{^-first}}`` - Begin marker of a template that will be instantiated *for each except the first* entry in the layer. This,
 *    this is basically the inversion of ``{{#-first}}``. Use is only allowed between the begin and end marker of the layer allowed.
 *    So between ``{{#result}}`` and ``{{/result}}``, or between ``{{#measurement}}`` and ``{{/measurement}}``.
 *
 *  * ``{{/-first}}`` - End marker for either ``{{#-first}}`` or ``{{^-first}}``.
 *
 *  * ``{{#-last}}`` - Begin marker of a template that will be instantiated *only for the last* entry in the layer. Use is only
 *    allowed between the begin and end marker of the layer allowed. So between ``{{#result}}`` and ``{{/result}}``, or between
 *    ``{{#measurement}}`` and ``{{/measurement}}``. Finish the template with ``{{/-last}}``.
 *
 *  * ``{{^-last}}`` - Begin marker of a template that will be instantiated *for each except the last* entry in the layer. This,
 *    this is basically the inversion of ``{{#-last}}``. Use is only allowed between the begin and end marker of the layer allowed.
 *    So between ``{{#result}}`` and ``{{/result}}``, or between ``{{#measurement}}`` and ``{{/measurement}}``.
 *
 *  * ``{{/-last}}`` - End marker for either ``{{#-last}}`` or ``{{^-last}}``.
 *
   @verbatim embed:rst

   For an overview of all the possible data you can get out of nanobench, please see the tutorial at :ref:`tutorial-template-json`.

   The templates that ship with nanobench are:

   * :cpp:func:`templates::csv() <ankerl::nanobench::templates::csv()>`
   * :cpp:func:`templates::json() <ankerl::nanobench::templates::json()>`
   * :cpp:func:`templates::htmlBoxplot() <ankerl::nanobench::templates::htmlBoxplot()>`

   @endverbatim
 *
 * @param mustacheTemplate The template.
 * @param bench Benchmark, containing all the results.
 * @param out Output for the generated output.
 */
void render(char const* mustacheTemplate, Bench const& bench, std::ostream& out);

/**
 * Same as render(char const* mustacheTemplate, Bench const& bench, std::ostream& out), but for when
 * you only have results available.
 *
 * @param mustacheTemplate The template.
 * @param results All the results to be used for rendering.
 * @param out Output for the generated output.
 */
void render(char const* mustacheTemplate, std::vector<Result> const& results, std::ostream& out);

// Contains mustache-like templates
namespace templates {

/*!
  @brief CSV data for the benchmark results.

  Generates a comma-separated values dataset. First line is the header, each following line is a summary of each benchmark run.

  @verbatim embed:rst
  See the tutorial at :ref:`tutorial-template-csv` for an example.
  @endverbatim
 */
char const* csv() noexcept;

/*!
  @brief HTML output that uses plotly to generate an interactive boxplot chart. See the tutorial for an example output.

  The output uses only the elapsed time, and displays each epoch as a single dot.
  @verbatim embed:rst
  See the tutorial at :ref:`tutorial-template-html` for an example.
  @endverbatim

  @see ankerl::nanobench::render()
 */
char const* htmlBoxplot() noexcept;

/*!
  @brief Template to generate JSON data.

  The generated JSON data contains *all* data that has been generated. All times are as double values, in seconds. The output can get
  quite large.
  @verbatim embed:rst
  See the tutorial at :ref:`tutorial-template-json` for an example.
  @endverbatim
 */
char const* json() noexcept;

} // namespace templates

namespace detail {

template <typename T>
struct PerfCountSet;

class IterationLogic;
class PerformanceCounters;

#if ANKERL_NANOBENCH(PERF_COUNTERS)
class LinuxPerformanceCounters;
#endif

} // namespace detail
} // namespace nanobench
} // namespace ankerl

// definitions ////////////////////////////////////////////////////////////////////////////////////

namespace ankerl {
namespace nanobench {
namespace detail {

template <typename T>
struct PerfCountSet {
    T pageFaults{};
    T cpuCycles{};
    T contextSwitches{};
    T instructions{};
    T branchInstructions{};
    T branchMisses{};
};

} // namespace detail

ANKERL_NANOBENCH(IGNORE_PADDED_PUSH)
struct Config {
    // actual benchmark config
    std::string mBenchmarkTitle = "benchmark";
    std::string mBenchmarkName = "noname";
    std::string mUnit = "op";
    double mBatch = 1.0;
    double mComplexityN = -1.0;
    size_t mNumEpochs = 11;
    size_t mClockResolutionMultiple = static_cast<size_t>(1000);
    std::chrono::nanoseconds mMaxEpochTime = std::chrono::milliseconds(100);
    std::chrono::nanoseconds mMinEpochTime{};
    uint64_t mMinEpochIterations{1};
    uint64_t mEpochIterations{0}; // If not 0, run *exactly* these number of iterations per epoch.
    uint64_t mWarmup = 0;
    std::ostream* mOut = nullptr;
    bool mShowPerformanceCounters = true;
    bool mIsRelative = false;

    Config();
    ~Config();
    Config& operator=(Config const&);
    Config& operator=(Config&&);
    Config(Config const&);
    Config(Config&&) noexcept;
};
ANKERL_NANOBENCH(IGNORE_PADDED_POP)

// Result returned after a benchmark has finished. Can be used as a baseline for relative().
ANKERL_NANOBENCH(IGNORE_PADDED_PUSH)
class Result {
public:
    enum class Measure : size_t {
        elapsed,
        iterations,
        pagefaults,
        cpucycles,
        contextswitches,
        instructions,
        branchinstructions,
        branchmisses,
        _size
    };

    explicit Result(Config const& benchmarkConfig);

    ~Result();
    Result& operator=(Result const&);
    Result& operator=(Result&&);
    Result(Result const&);
    Result(Result&&) noexcept;

    // adds new measurement results
    // all values are scaled by iters (except iters...)
    void add(Clock::duration totalElapsed, uint64_t iters, detail::PerformanceCounters const& pc);

    ANKERL_NANOBENCH(NODISCARD) Config const& config() const noexcept;

    ANKERL_NANOBENCH(NODISCARD) double median(Measure m) const;
    ANKERL_NANOBENCH(NODISCARD) double medianAbsolutePercentError(Measure m) const;
    ANKERL_NANOBENCH(NODISCARD) double average(Measure m) const;
    ANKERL_NANOBENCH(NODISCARD) double sum(Measure m) const noexcept;
    ANKERL_NANOBENCH(NODISCARD) double sumProduct(Measure m1, Measure m2) const noexcept;
    ANKERL_NANOBENCH(NODISCARD) double minimum(Measure m) const noexcept;
    ANKERL_NANOBENCH(NODISCARD) double maximum(Measure m) const noexcept;

    ANKERL_NANOBENCH(NODISCARD) bool has(Measure m) const noexcept;
    ANKERL_NANOBENCH(NODISCARD) double get(size_t idx, Measure m) const;
    ANKERL_NANOBENCH(NODISCARD) bool empty() const noexcept;
    ANKERL_NANOBENCH(NODISCARD) size_t size() const noexcept;

    // Finds string, if not found, returns _size.
    static Measure fromString(std::string const& str);

private:
    Config mConfig{};
    std::vector<std::vector<double>> mNameToMeasurements{};
};
ANKERL_NANOBENCH(IGNORE_PADDED_POP)

/**
 * An extremely fast random generator. Currently, this implements *RomuDuoJr*, developed by Mark Overton. Source:
 * http://www.romu-random.org/
 *
 * RomuDuoJr is extremely fast and provides reasonable good randomness. Not enough for large jobs, but definitely
 * good enough for a benchmarking framework.
 *
 *  * Estimated capacity: @f$ 2^{51} @f$ bytes
 *  * Register pressure: 4
 *  * State size: 128 bits
 *
 * This random generator is a drop-in replacement for the generators supplied by ``<random>``. It is not
 * cryptographically secure. It's intended purpose is to be very fast so that benchmarks that make use
 * of randomness are not distorted too much by the random generator.
 *
 * Rng also provides a few non-standard helpers, optimized for speed.
 */
class Rng final {
public:
    /**
     * @brief This RNG provides 64bit randomness.
     */
    using result_type = uint64_t;

    static constexpr uint64_t(min)();
    static constexpr uint64_t(max)();

    /**
     * As a safety precausion, we don't allow copying. Copying a PRNG would mean you would have two random generators that produce the
     * same sequence, which is generally not what one wants. Instead create a new rng with the default constructor Rng(), which is
     * automatically seeded from `std::random_device`. If you really need a copy, use copy().
     */
    Rng(Rng const&) = delete;

    /**
     * Same as Rng(Rng const&), we don't allow assignment. If you need a new Rng create one with the default constructor Rng().
     */
    Rng& operator=(Rng const&) = delete;

    // moving is ok
    Rng(Rng&&) noexcept = default;
    Rng& operator=(Rng&&) noexcept = default;
    ~Rng() noexcept = default;

    /**
     * @brief Creates a new Random generator with random seed.
     *
     * Instead of a default seed (as the random generators from the STD), this properly seeds the random generator from
     * `std::random_device`. It guarantees correct seeding. Note that seeding can be relatively slow, depending on the source of
     * randomness used. So it is best to create a Rng once and use it for all your randomness purposes.
     */
    Rng();

    /*!
      Creates a new Rng that is seeded with a specific seed. Each Rng created from the same seed will produce the same randomness
      sequence. This can be useful for deterministic behavior.

      @verbatim embed:rst
      .. note::

         The random algorithm might change between nanobench releases. Whenever a faster and/or better random
         generator becomes available, I will switch the implementation.
      @endverbatim

      As per the Romu paper, this seeds the Rng with splitMix64 algorithm and performs 10 initial rounds for further mixing up of the
      internal state.

      @param seed  The 64bit seed. All values are allowed, even 0.
     */
    explicit Rng(uint64_t seed) noexcept;
    Rng(uint64_t x, uint64_t y) noexcept;

    /**
     * Creates a copy of the Rng, thus the copy provides exactly the same random sequence as the original.
     */
    ANKERL_NANOBENCH(NODISCARD) Rng copy() const noexcept;

    /**
     * @brief Produces a 64bit random value. This should be very fast, thus it is marked as inline. In my benchmark, this is ~46 times
     * faster than `std::default_random_engine` for producing 64bit random values. It seems that the fastest std contender is
     * `std::mt19937_64`. Still, this RNG is 2-3 times as fast.
     *
     * @return uint64_t The next 64 bit random value.
     */
    inline uint64_t operator()() noexcept;

    // This is slightly biased. See

    /**
     * Generates a random number between 0 and range (excluding range).
     *
     * The algorithm only produces 32bit numbers, and is slightly biased. The effect is quite small unless your range is close to the
     * maximum value of an integer. It is possible to correct the bias with rejection sampling (see
     * [here](https://lemire.me/blog/2016/06/30/fast-random-shuffling/), but this is most likely irrelevant in practices for the
     * purposes of this Rng.
     *
     * See Daniel Lemire's blog post [A fast alternative to the modulo
     * reduction](https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/)
     *
     * @param range Upper exclusive range. E.g a value of 3 will generate random numbers 0, 1, 2.
     * @return uint32_t Generated random values in range [0, range(.
     */
    inline uint32_t bounded(uint32_t range) noexcept;

    // random double in range [0, 1(
    // see http://prng.di.unimi.it/

    /**
     * Provides a random uniform double value between 0 and 1. This uses the method described in [Generating uniform doubles in the
     * unit interval](http://prng.di.unimi.it/), and is extremely fast.
     *
     * @return double Uniformly distributed double value in range [0,1(, excluding 1.
     */
    inline double uniform01() noexcept;

    /**
     * Shuffles all entries in the given container. Although this has a slight bias due to the implementation of bounded(), this is
     * preferable to `std::shuffle` because it is over 5 times faster. See Daniel Lemire's blog post [Fast random
     * shuffling](https://lemire.me/blog/2016/06/30/fast-random-shuffling/).
     *
     * @param container The whole container will be shuffled.
     */
    template <typename Container>
    void shuffle(Container& container) noexcept;

private:
    static constexpr uint64_t rotl(uint64_t x, unsigned k) noexcept;

    uint64_t mX;
    uint64_t mY;
};

/**
 * @brief Main entry point to nanobench's benchmarking facility.
 *
 * It holds configuration and results from one or more benchmark runs. Usually it is used in a single line, where the object is
 * constructed, configured, and then a benchmark is run. E.g. like this:
 *
 *     ankerl::nanobench::Bench().unit("byte").batch(1000).run("random fluctuations", [&] {
 *         // here be the benchmark code
 *     });
 *
 * In that example Bench() constructs the benchmark, it is then configured with unit() and batch(), and after configuration a
 * benchmark is executed with run(). Once run() has finished, it prints the result to `std::cout`. It would also store the results
 * in the Bench instance, but in this case the object is immediately destroyed so it's not available any more.
 */
ANKERL_NANOBENCH(IGNORE_PADDED_PUSH)
class Bench {
public:
    /**
     * @brief Creates a new benchmark for configuration and running of benchmarks.
     */
    Bench();

    Bench(Bench&& other);
    Bench& operator=(Bench&& other);
    Bench(Bench const& other);
    Bench& operator=(Bench const& other);
    ~Bench() noexcept;

    /*!
      @brief Repeatedly calls `op()` based on the configuration, and performs measurements.

      This call is marked with `noinline` to prevent the compiler to optimize beyond different benchmarks. This can have quite a big
      effect on benchmark accuracy.

      @verbatim embed:rst
      .. note::

        Each call to your lambda must have a side effect that the compiler can't possibly optimize it away. E.g. add a result to an
        externally defined number (like `x` in the above example), and finally call `doNotOptimizeAway` on the variables the compiler
        must not remove. You can also use :cpp:func:`ankerl::nanobench::doNotOptimizeAway` directly in the lambda, but be aware that
        this has a small overhead.

      @endverbatim

      @tparam Op The code to benchmark.
     */
    template <typename Op>
    ANKERL_NANOBENCH(NOINLINE)
    Bench& run(char const* benchmarkName, Op&& op);

    template <typename Op>
    ANKERL_NANOBENCH(NOINLINE)
    Bench& run(std::string const& benchmarkName, Op&& op);

    /**
     * @brief Same as run(char const* benchmarkName, Op op), but instead uses the previously set name.
     * @tparam Op The code to benchmark.
     */
    template <typename Op>
    ANKERL_NANOBENCH(NOINLINE)
    Bench& run(Op&& op);

    /**
     * @brief Title of the benchmark, will be shown in the table header. Changing the title will start a new markdown table.
     *
     * @param benchmarkTitle The title of the benchmark.
     */
    Bench& title(char const* benchmarkTitle);
    Bench& title(std::string const& benchmarkTitle);
    ANKERL_NANOBENCH(NODISCARD) std::string const& title() const noexcept;

    /// Name of the benchmark, will be shown in the table row.
    Bench& name(char const* benchmarkName);
    Bench& name(std::string const& benchmarkName);
    ANKERL_NANOBENCH(NODISCARD) std::string const& name() const noexcept;

    /**
     * @brief Sets the batch size.
     *
     * E.g. number of processed byte, or some other metric for the size of the processed data in each iteration. If you benchmark
     * hashing of a 1000 byte long string and want byte/sec as a result, you can specify 1000 as the batch size.
     *
     * @tparam T Any input type is internally cast to `double`.
     * @param b batch size
     */
    template <typename T>
    Bench& batch(T b) noexcept;
    ANKERL_NANOBENCH(NODISCARD) double batch() const noexcept;

    /**
     * @brief Sets the operation unit.
     *
     * Defaults to "op". Could be e.g. "byte" for string processing. This is used for the table header, e.g. to show `ns/byte`. Use
     * singular (*byte*, not *bytes*). A change clears the currently collected results.
     *
     * @param unit The unit name.
     */
    Bench& unit(char const* unit);
    Bench& unit(std::string const& unit);
    ANKERL_NANOBENCH(NODISCARD) std::string const& unit() const noexcept;

    /**
     * @brief Set the output stream where the resulting markdown table will be printed to.
     *
     * The default is `&std::cout`. You can disable all output by setting `nullptr`.
     *
     * @param outstream Pointer to output stream, can be `nullptr`.
     */
    Bench& output(std::ostream* outstream) noexcept;
    ANKERL_NANOBENCH(NODISCARD) std::ostream* output() const noexcept;

    /**
     * Modern processors have a very accurate clock, being able to measure as low as 20 nanoseconds. This is the main trick nanobech to
     * be so fast: we find out how accurate the clock is, then run the benchmark only so often that the clock's accuracy is good enough
     * for accurate measurements.
     *
     * The default is to run one epoch for 1000 times the clock resolution. So for 20ns resolution and 11 epochs, this gives a total
     * runtime of
     *
     * @f[
     * 20ns * 1000 * 11 \approx 0.2ms
     * @f]
     *
     * To be precise, nanobench adds a 0-20% random noise to each evaluation. This is to prevent any aliasing effects, and further
     * improves accuracy.
     *
     * Total runtime will be higher though: Some initial time is needed to find out the target number of iterations for each epoch, and
     * there is some overhead involved to start & stop timers and calculate resulting statistics and writing the output.
     *
     * @param multiple Target number of times of clock resolution. Usually 1000 is a good compromise between runtime and accuracy.
     */
    Bench& clockResolutionMultiple(size_t multiple) noexcept;
    ANKERL_NANOBENCH(NODISCARD) size_t clockResolutionMultiple() const noexcept;

    /**
     * @brief Controls number of epochs, the number of measurements to perform.
     *
     * The reported result will be the median of evaluation of each epoch. The higher you choose this, the more
     * deterministic the result be and outliers will be more easily removed. Also the `err%` will be more accurate the higher this
     * number is. Note that the `err%` will not necessarily decrease when number of epochs is increased. But it will be a more accurate
     * representation of the benchmarked code's runtime stability.
     *
     * Choose the value wisely. In practice, 11 has been shown to be a reasonable choice between runtime performance and accuracy.
     * This setting goes hand in hand with minEpocIterations() (or minEpochTime()). If you are more interested in *median* runtime, you
     * might want to increase epochs(). If you are more interested in *mean* runtime, you might want to increase minEpochIterations()
     * instead.
     *
     * @param numEpochs Number of epochs.
     */
    Bench& epochs(size_t numEpochs) noexcept;
    ANKERL_NANOBENCH(NODISCARD) size_t epochs() const noexcept;

    /**
     * @brief Upper limit for the runtime of each epoch.
     *
     * As a safety precausion if the clock is not very accurate, we can set an upper limit for the maximum evaluation time per
     * epoch. Default is 100ms. At least a single evaluation of the benchmark is performed.
     *
     * @see minEpochTime(), minEpochIterations()
     *
     * @param t Maximum target runtime for a single epoch.
     */
    Bench& maxEpochTime(std::chrono::nanoseconds t) noexcept;
    ANKERL_NANOBENCH(NODISCARD) std::chrono::nanoseconds maxEpochTime() const noexcept;

    /**
     * @brief Minimum time each epoch should take.
     *
     * Default is zero, so we are fully relying on clockResolutionMultiple(). In most cases this is exactly what you want. If you see
     * that the evaluation is unreliable with a high `err%`, you can increase either minEpochTime() or minEpochIterations().
     *
     * @see maxEpochTime(), minEpochIterations()
     *
     * @param t Minimum time each epoch should take.
     */
    Bench& minEpochTime(std::chrono::nanoseconds t) noexcept;
    ANKERL_NANOBENCH(NODISCARD) std::chrono::nanoseconds minEpochTime() const noexcept;

    /**
     * @brief Sets the minimum number of iterations each epoch should take.
     *
     * Default is 1, and we rely on clockResolutionMultiple(). If the `err%` is high and you want a more smooth result, you might want
     * to increase the minimum number or iterations, or increase the minEpochTime().
     *
     * @see minEpochTime(), maxEpochTime(), minEpochIterations()
     *
     * @param numIters Minimum number of iterations per epoch.
     */
    Bench& minEpochIterations(uint64_t numIters) noexcept;
    ANKERL_NANOBENCH(NODISCARD) uint64_t minEpochIterations() const noexcept;

    /**
     * Sets exactly the number of iterations for each epoch. Ignores all other epoch limits. This forces nanobench to use exactly
     * the given number of iterations for each epoch, not more and not less. Default is 0 (disabled).
     *
     * @param numIters Exact number of iterations to use. Set to 0 to disable.
     */
    Bench& epochIterations(uint64_t numIters) noexcept;
    ANKERL_NANOBENCH(NODISCARD) uint64_t epochIterations() const noexcept;

    /**
     * @brief Sets a number of iterations that are initially performed without any measurements.
     *
     * Some benchmarks need a few evaluations to warm up caches / database / whatever access. Normally this should not be needed, since
     * we show the median result so initial outliers will be filtered away automatically. If the warmup effect is large though, you
     * might want to set it. Default is 0.
     *
     * @param numWarmupIters Number of warmup iterations.
     */
    Bench& warmup(uint64_t numWarmupIters) noexcept;
    ANKERL_NANOBENCH(NODISCARD) uint64_t warmup() const noexcept;

    /**
     * @brief Marks the next run as the baseline.
     *
     * Call `relative(true)` to mark the run as the baseline. Successive runs will be compared to this run. It is calculated by
     *
     * @f[
     * 100\% * \frac{baseline}{runtime}
     * @f]
     *
     *  * 100% means it is exactly as fast as the baseline
     *  * >100% means it is faster than the baseline. E.g. 200% means the current run is twice as fast as the baseline.
     *  * <100% means it is slower than the baseline. E.g. 50% means it is twice as slow as the baseline.
     *
     * See the tutorial section "Comparing Results" for example usage.
     *
     * @param isRelativeEnabled True to enable processing
     */
    Bench& relative(bool isRelativeEnabled) noexcept;
    ANKERL_NANOBENCH(NODISCARD) bool relative() const noexcept;

    /**
     * @brief Enables/disables performance counters.
     *
     * On Linux nanobench has a powerful feature to use performance counters. This enables counting of retired instructions, count
     * number of branches, missed branches, etc. On default this is enabled, but you can disable it if you don't need that feature.
     *
     * @param showPerformanceCounters True to enable, false to disable.
     */
    Bench& performanceCounters(bool showPerformanceCounters) noexcept;
    ANKERL_NANOBENCH(NODISCARD) bool performanceCounters() const noexcept;

    /**
     * @brief Retrieves all benchmark results collected by the bench object so far.
     *
     * Each call to run() generates a Result that is stored within the Bench instance. This is mostly for advanced users who want to
     * see all the nitty gritty detials.
     *
     * @return All results collected so far.
     */
    ANKERL_NANOBENCH(NODISCARD) std::vector<Result> const& results() const noexcept;

    /*!
      @verbatim embed:rst

      Convenience shortcut to :cpp:func:`ankerl::nanobench::doNotOptimizeAway`.

      @endverbatim
     */
    template <typename Arg>
    Bench& doNotOptimizeAway(Arg&& arg);

    /*!
      @verbatim embed:rst

      Sets N for asymptotic complexity calculation, so it becomes possible to calculate `Big O
      <https://en.wikipedia.org/wiki/Big_O_notation>`_ from multiple benchmark evaluations.

      Use :cpp:func:`ankerl::nanobench::Bench::complexityBigO` when the evaluation has finished. See the tutorial
      :ref:`asymptotic-complexity` for details.

      @endverbatim

      @tparam T Any type is cast to `double`.
      @param b Length of N for the next benchmark run, so it is possible to calculate `bigO`.
     */
    template <typename T>
    Bench& complexityN(T b) noexcept;
    ANKERL_NANOBENCH(NODISCARD) double complexityN() const noexcept;

    /*!
      Calculates [Big O](https://en.wikipedia.org/wiki/Big_O_notation>) of the results with all preconfigured complexity functions.
      Currently these complexity functions are fitted into the benchmark results:

       @f$ \mathcal{O}(1) @f$,
       @f$ \mathcal{O}(n) @f$,
       @f$ \mathcal{O}(\log{}n) @f$,
       @f$ \mathcal{O}(n\log{}n) @f$,
       @f$ \mathcal{O}(n^2) @f$,
       @f$ \mathcal{O}(n^3) @f$.

      If we e.g. evaluate the complexity of `std::sort`, this is the result of `std::cout << bench.complexityBigO()`:

      ```
      |   coefficient |   err% | complexity
      |--------------:|-------:|------------
      |   5.08935e-09 |   2.6% | O(n log n)
      |   6.10608e-08 |   8.0% | O(n)
      |   1.29307e-11 |  47.2% | O(n^2)
      |   2.48677e-15 |  69.6% | O(n^3)
      |   9.88133e-06 | 132.3% | O(log n)
      |   5.98793e-05 | 162.5% | O(1)
      ```

      So in this case @f$ \mathcal{O}(n\log{}n) @f$ provides the best approximation.

      @verbatim embed:rst
      See the tutorial :ref:`asymptotic-complexity` for details.
      @endverbatim
      @return Evaluation results, which can be printed or otherwise inspected.
     */
    std::vector<BigO> complexityBigO() const;

    /**
     * @brief Calculates bigO for a custom function.
     *
     * E.g. to calculate the mean squared error for @f$ \mathcal{O}(\log{}\log{}n) @f$, which is not part of the default set of
     * complexityBigO(), you can do this:
     *
     * ```
     * auto logLogN = bench.complexityBigO("O(log log n)", [](double n) {
     *     return std::log2(std::log2(n));
     * });
     * ```
     *
     * The resulting mean squared error can be printed with `std::cout << logLogN`. E.g. it prints something like this:
     *
     * ```text
     * 2.46985e-05 * O(log log n), rms=1.48121
     * ```
     *
     * @tparam Op Type of mapping operation.
     * @param name Name for the function, e.g. "O(log log n)"
     * @param op Op's operator() maps a `double` with the desired complexity function, e.g. `log2(log2(n))`.
     * @return BigO Error calculation, which is streamable to std::cout.
     */
    template <typename Op>
    BigO complexityBigO(char const* name, Op op) const;

    template <typename Op>
    BigO complexityBigO(std::string const& name, Op op) const;

    /*!
      @verbatim embed:rst

      Convenience shortcut to :cpp:func:`ankerl::nanobench::render`.

      @endverbatim
     */
    Bench& render(char const* templateContent, std::ostream& os);

    Bench& config(Config const& benchmarkConfig);
    ANKERL_NANOBENCH(NODISCARD) Config const& config() const noexcept;

private:
    Config mConfig{};
    std::vector<Result> mResults{};
};
ANKERL_NANOBENCH(IGNORE_PADDED_POP)

/**
 * @brief Makes sure none of the given arguments are optimized away by the compiler.
 *
 * @tparam Arg Type of the argument that shouldn't be optimized away.
 * @param arg The input that we mark as being used, even though we don't do anything with it.
 */
template <typename Arg>
void doNotOptimizeAway(Arg&& arg);

namespace detail {

#if defined(_MSC_VER)
void doNotOptimizeAwaySink(void const*);

template <typename T>
void doNotOptimizeAway(T const& val);

#else

// see folly's Benchmark.h
template <typename T>
constexpr bool doNotOptimizeNeedsIndirect() {
    using Decayed = typename std::decay<T>::type;
    return !ANKERL_NANOBENCH_IS_TRIVIALLY_COPYABLE(Decayed) || sizeof(Decayed) > sizeof(long) || std::is_pointer<Decayed>::value;
}

template <typename T>
typename std::enable_if<!doNotOptimizeNeedsIndirect<T>()>::type doNotOptimizeAway(T const& val) {
    // NOLINTNEXTLINE(hicpp-no-assembler)
    asm volatile("" ::"r"(val));
}

template <typename T>
typename std::enable_if<doNotOptimizeNeedsIndirect<T>()>::type doNotOptimizeAway(T const& val) {
    // NOLINTNEXTLINE(hicpp-no-assembler)
    asm volatile("" ::"m"(val) : "memory");
}
#endif

// internally used, but visible because run() is templated.
// Not movable/copy-able, so we simply use a pointer instead of unique_ptr. This saves us from
// having to include <memory>, and the template instantiation overhead of unique_ptr which is unfortunately quite significant.
ANKERL_NANOBENCH(IGNORE_EFFCPP_PUSH)
class IterationLogic {
public:
    explicit IterationLogic(Bench const& config) noexcept;
    ~IterationLogic();

    ANKERL_NANOBENCH(NODISCARD) uint64_t numIters() const noexcept;
    void add(std::chrono::nanoseconds elapsed, PerformanceCounters const& pc) noexcept;
    void moveResultTo(std::vector<Result>& results) noexcept;

private:
    struct Impl;
    Impl* mPimpl;
};
ANKERL_NANOBENCH(IGNORE_EFFCPP_POP)

ANKERL_NANOBENCH(IGNORE_PADDED_PUSH)
class PerformanceCounters {
public:
    PerformanceCounters(PerformanceCounters const&) = delete;
    PerformanceCounters& operator=(PerformanceCounters const&) = delete;

    PerformanceCounters();
    ~PerformanceCounters();

    void beginMeasure();
    void endMeasure();
    void updateResults(uint64_t numIters);

    ANKERL_NANOBENCH(NODISCARD) PerfCountSet<uint64_t> const& val() const noexcept;
    ANKERL_NANOBENCH(NODISCARD) PerfCountSet<bool> const& has() const noexcept;

private:
#if ANKERL_NANOBENCH(PERF_COUNTERS)
    LinuxPerformanceCounters* mPc = nullptr;
#endif
    PerfCountSet<uint64_t> mVal{};
    PerfCountSet<bool> mHas{};
};
ANKERL_NANOBENCH(IGNORE_PADDED_POP)

// Gets the singleton
PerformanceCounters& performanceCounters();

} // namespace detail

class BigO {
public:
    using RangeMeasure = std::vector<std::pair<double, double>>;

    template <typename Op>
    static RangeMeasure mapRangeMeasure(RangeMeasure data, Op op) {
        for (auto& rangeMeasure : data) {
            rangeMeasure.first = op(rangeMeasure.first);
        }
        return data;
    }

    static RangeMeasure collectRangeMeasure(std::vector<Result> const& results);

    template <typename Op>
    BigO(char const* bigOName, RangeMeasure const& rangeMeasure, Op rangeToN)
        : BigO(bigOName, mapRangeMeasure(rangeMeasure, rangeToN)) {}

    template <typename Op>
    BigO(std::string const& bigOName, RangeMeasure const& rangeMeasure, Op rangeToN)
        : BigO(bigOName, mapRangeMeasure(rangeMeasure, rangeToN)) {}

    BigO(char const* bigOName, RangeMeasure const& scaledRangeMeasure);
    BigO(std::string const& bigOName, RangeMeasure const& scaledRangeMeasure);
    ANKERL_NANOBENCH(NODISCARD) std::string const& name() const noexcept;
    ANKERL_NANOBENCH(NODISCARD) double constant() const noexcept;
    ANKERL_NANOBENCH(NODISCARD) double normalizedRootMeanSquare() const noexcept;
    ANKERL_NANOBENCH(NODISCARD) bool operator<(BigO const& other) const noexcept;

private:
    std::string mName{};
    double mConstant{};
    double mNormalizedRootMeanSquare{};
};
std::ostream& operator<<(std::ostream& os, BigO const& bigO);
std::ostream& operator<<(std::ostream& os, std::vector<ankerl::nanobench::BigO> const& bigOs);

} // namespace nanobench
} // namespace ankerl

// implementation /////////////////////////////////////////////////////////////////////////////////

namespace ankerl {
namespace nanobench {

constexpr uint64_t(Rng::min)() {
    return 0;
}

constexpr uint64_t(Rng::max)() {
    return (std::numeric_limits<uint64_t>::max)();
}

ANKERL_NANOBENCH_NO_SANITIZE("integer")
uint64_t Rng::operator()() noexcept {
    auto x = mX;

    mX = UINT64_C(15241094284759029579) * mY;
    mY = rotl(mY - x, 27);

    return x;
}

ANKERL_NANOBENCH_NO_SANITIZE("integer")
uint32_t Rng::bounded(uint32_t range) noexcept {
    uint64_t r32 = static_cast<uint32_t>(operator()());
    auto multiresult = r32 * range;
    return static_cast<uint32_t>(multiresult >> 32U);
}

double Rng::uniform01() noexcept {
    auto i = (UINT64_C(0x3ff) << 52U) | (operator()() >> 12U);
    // can't use union in c++ here for type puning, it's undefined behavior.
    // std::memcpy is optimized anyways.
    double d;
    std::memcpy(&d, &i, sizeof(double));
    return d - 1.0;
}

template <typename Container>
void Rng::shuffle(Container& container) noexcept {
    auto size = static_cast<uint32_t>(container.size());
    for (auto i = size; i > 1U; --i) {
        using std::swap;
        auto p = bounded(i); // number in [0, i)
        swap(container[i - 1], container[p]);
    }
}

constexpr uint64_t Rng::rotl(uint64_t x, unsigned k) noexcept {
    return (x << k) | (x >> (64U - k));
}

template <typename Op>
ANKERL_NANOBENCH_NO_SANITIZE("integer")
Bench& Bench::run(Op&& op) {
    // It is important that this method is kept short so the compiler can do better optimizations/ inlining of op()
    detail::IterationLogic iterationLogic(*this);
    auto& pc = detail::performanceCounters();

    while (auto n = iterationLogic.numIters()) {
        pc.beginMeasure();
        Clock::time_point before = Clock::now();
        while (n-- > 0) {
            op();
        }
        Clock::time_point after = Clock::now();
        pc.endMeasure();
        pc.updateResults(iterationLogic.numIters());
        iterationLogic.add(after - before, pc);
    }
    iterationLogic.moveResultTo(mResults);
    return *this;
}

// Performs all evaluations.
template <typename Op>
Bench& Bench::run(char const* benchmarkName, Op&& op) {
    name(benchmarkName);
    return run(std::forward<Op>(op));
}

template <typename Op>
Bench& Bench::run(std::string const& benchmarkName, Op&& op) {
    name(benchmarkName);
    return run(std::forward<Op>(op));
}

template <typename Op>
BigO Bench::complexityBigO(char const* benchmarkName, Op op) const {
    return BigO(benchmarkName, BigO::collectRangeMeasure(mResults), op);
}

template <typename Op>
BigO Bench::complexityBigO(std::string const& benchmarkName, Op op) const {
    return BigO(benchmarkName, BigO::collectRangeMeasure(mResults), op);
}

// Set the batch size, e.g. number of processed bytes, or some other metric for the size of the processed data in each iteration.
// Any argument is cast to double.
template <typename T>
Bench& Bench::batch(T b) noexcept {
    mConfig.mBatch = static_cast<double>(b);
    return *this;
}

// Sets the computation complexity of the next run. Any argument is cast to double.
template <typename T>
Bench& Bench::complexityN(T n) noexcept {
    mConfig.mComplexityN = static_cast<double>(n);
    return *this;
}

// Convenience: makes sure none of the given arguments are optimized away by the compiler.
template <typename Arg>
Bench& Bench::doNotOptimizeAway(Arg&& arg) {
    detail::doNotOptimizeAway(std::forward<Arg>(arg));
    return *this;
}

// Makes sure none of the given arguments are optimized away by the compiler.
template <typename Arg>
void doNotOptimizeAway(Arg&& arg) {
    detail::doNotOptimizeAway(std::forward<Arg>(arg));
}

namespace detail {

#if defined(_MSC_VER)
template <typename T>
void doNotOptimizeAway(T const& val) {
    doNotOptimizeAwaySink(&val);
}

#endif

} // namespace detail
} // namespace nanobench
} // namespace ankerl

#if defined(ANKERL_NANOBENCH_IMPLEMENT)

///////////////////////////////////////////////////////////////////////////////////////////////////
// implementation part - only visible in .cpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#    include <algorithm> // sort, reverse
#    include <atomic>    // compare_exchange_strong in loop overhead
#    include <cstdlib>   // getenv
#    include <cstring>   // strstr, strncmp
#    include <fstream>   // ifstream to parse proc files
#    include <iomanip>   // setw, setprecision
#    include <iostream>  // cout
#    include <numeric>   // accumulate
#    include <random>    // random_device
#    include <sstream>   // to_s in Number
#    include <stdexcept> // throw for rendering templates
#    include <tuple>     // std::tie
#    if defined(__linux__)
#        include <unistd.h> //sysconf
#    endif
#    if ANKERL_NANOBENCH(PERF_COUNTERS)
#        include <map> // map

#        include <linux/perf_event.h>
#        include <sys/ioctl.h>
#        include <sys/syscall.h>
#        include <unistd.h>
#    endif

// declarations ///////////////////////////////////////////////////////////////////////////////////

namespace ankerl {
namespace nanobench {

// helper stuff that is only intended to be used internally
namespace detail {

struct TableInfo;

// formatting utilities
namespace fmt {

class NumSep;
class StreamStateRestorer;
class Number;
class MarkDownColumn;
class MarkDownCode;

} // namespace fmt
} // namespace detail
} // namespace nanobench
} // namespace ankerl

// definitions ////////////////////////////////////////////////////////////////////////////////////

namespace ankerl {
namespace nanobench {

uint64_t splitMix64(uint64_t& state) noexcept;

namespace detail {

// helpers to get double values
template <typename T>
inline double d(T t) noexcept {
    return static_cast<double>(t);
}
inline double d(Clock::duration duration) noexcept {
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
}

// Calculates clock resolution once, and remembers the result
inline Clock::duration clockResolution() noexcept;

} // namespace detail

namespace templates {

char const* csv() noexcept {
    return R"DELIM("title";"name";"unit";"batch";"elapsed";"error %";"instructions";"branches";"branch misses";"total"
{{#result}}"{{title}}";"{{name}}";"{{unit}}";{{batch}};{{median(elapsed)}};{{medianAbsolutePercentError(elapsed)}};{{median(instructions)}};{{median(branchinstructions)}};{{median(branchmisses)}};{{sumProduct(iterations, elapsed)}}
{{/result}})DELIM";
}

char const* htmlBoxplot() noexcept {
    return R"DELIM(<html>

<head>
    <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
</head>

<body>
    <div id="myDiv"></div>
    <script>
        var data = [
            {{#result}}{
                name: '{{name}}',
                y: [{{#measurement}}{{elapsed}}{{^-last}}, {{/last}}{{/measurement}}],
            },
            {{/result}}
        ];
        var title = '{{title}}';

        data = data.map(a => Object.assign(a, { boxpoints: 'all', pointpos: 0, type: 'box' }));
        var layout = { title: { text: title }, showlegend: false, yaxis: { title: 'time per unit', rangemode: 'tozero', autorange: true } }; Plotly.newPlot('myDiv', data, layout, {responsive: true});
    </script>
</body>

</html>)DELIM";
}

char const* json() noexcept {
    return R"DELIM({
    "results": [
{{#result}}        {
            "title": "{{title}}",
            "name": "{{name}}",
            "unit": "{{unit}}",
            "batch": {{batch}},
            "complexityN": {{complexityN}},
            "epochs": {{epochs}},
            "clockResolution": {{clockResolution}},
            "clockResolutionMultiple": {{clockResolutionMultiple}},
            "maxEpochTime": {{maxEpochTime}},
            "minEpochTime": {{minEpochTime}},
            "minEpochIterations": {{minEpochIterations}},
            "epochIterations": {{epochIterations}},
            "warmup": {{warmup}},
            "relative": {{relative}},
            "median(elapsed)": {{median(elapsed)}},
            "medianAbsolutePercentError(elapsed)": {{medianAbsolutePercentError(elapsed)}},
            "median(instructions)": {{median(instructions)}},
            "medianAbsolutePercentError(instructions)": {{medianAbsolutePercentError(instructions)}},
            "median(cpucycles)": {{median(cpucycles)}},
            "median(contextswitches)": {{median(contextswitches)}},
            "median(pagefaults)": {{median(pagefaults)}},
            "median(branchinstructions)": {{median(branchinstructions)}},
            "median(branchmisses)": {{median(branchmisses)}},
            "totalTime": {{sumProduct(iterations, elapsed)}},
            "measurements": [
{{#measurement}}                {
                    "iterations": {{iterations}},
                    "elapsed": {{elapsed}},
                    "pagefaults": {{pagefaults}},
                    "cpucycles": {{cpucycles}},
                    "contextswitches": {{contextswitches}},
                    "instructions": {{instructions}},
                    "branchinstructions": {{branchinstructions}},
                    "branchmisses": {{branchmisses}}
                }{{^-last}},{{/-last}}
{{/measurement}}            ]
        }{{^-last}},{{/-last}}
{{/result}}    ]
})DELIM";
}

ANKERL_NANOBENCH(IGNORE_PADDED_PUSH)
struct Node {
    enum class Type { tag, content, section, inverted_section };

    char const* begin;
    char const* end;
    std::vector<Node> children;
    Type type;

    template <size_t N>
    // NOLINTNEXTLINE(hicpp-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays)
    bool operator==(char const (&str)[N]) const noexcept {
        return static_cast<size_t>(std::distance(begin, end) + 1) == N && 0 == strncmp(str, begin, N - 1);
    }
};
ANKERL_NANOBENCH(IGNORE_PADDED_POP)

static std::vector<Node> parseMustacheTemplate(char const** tpl) {
    std::vector<Node> nodes;

    while (true) {
        auto begin = std::strstr(*tpl, "{{");
        auto end = begin;
        if (begin != nullptr) {
            begin += 2;
            end = std::strstr(begin, "}}");
        }

        if (begin == nullptr || end == nullptr) {
            // nothing found, finish node
            nodes.emplace_back(Node{*tpl, *tpl + std::strlen(*tpl), std::vector<Node>{}, Node::Type::content});
            return nodes;
        }

        nodes.emplace_back(Node{*tpl, begin - 2, std::vector<Node>{}, Node::Type::content});

        // we found a tag
        *tpl = end + 2;
        switch (*begin) {
        case '/':
            // finished! bail out
            return nodes;

        case '#':
            nodes.emplace_back(Node{begin + 1, end, parseMustacheTemplate(tpl), Node::Type::section});
            break;

        case '^':
            nodes.emplace_back(Node{begin + 1, end, parseMustacheTemplate(tpl), Node::Type::inverted_section});
            break;

        default:
            nodes.emplace_back(Node{begin, end, std::vector<Node>{}, Node::Type::tag});
            break;
        }
    }
}

static bool generateFirstLast(Node const& n, size_t idx, size_t size, std::ostream& out) {
    bool matchFirst = n == "-first";
    bool matchLast = n == "-last";
    if (!matchFirst && !matchLast) {
        return false;
    }

    bool doWrite = false;
    if (n.type == Node::Type::section) {
        doWrite = (matchFirst && idx == 0) || (matchLast && idx == size - 1);
    } else if (n.type == Node::Type::inverted_section) {
        doWrite = (matchFirst && idx != 0) || (matchLast && idx != size - 1);
    }

    if (doWrite) {
        for (auto const& child : n.children) {
            if (child.type == Node::Type::content) {
                out.write(child.begin, std::distance(child.begin, child.end));
            }
        }
    }
    return true;
}

static bool matchCmdArgs(std::string const& str, std::vector<std::string>& matchResult) {
    matchResult.clear();
    auto idxOpen = str.find('(');
    auto idxClose = str.find(')', idxOpen);
    if (idxClose == std::string::npos) {
        return false;
    }

    matchResult.emplace_back(str.substr(0, idxOpen));

    // split by comma
    matchResult.emplace_back(std::string{});
    for (size_t i = idxOpen + 1; i != idxClose; ++i) {
        if (str[i] == ' ' || str[i] == '\t') {
            // skip whitespace
            continue;
        }
        if (str[i] == ',') {
            // got a comma => new string
            matchResult.emplace_back(std::string{});
            continue;
        }
        // no whitespace no comma, append
        matchResult.back() += str[i];
    }
    return true;
}

static bool generateConfigTag(Node const& n, Config const& config, std::ostream& out) {
    using detail::d;

    if (n == "title") {
        out << config.mBenchmarkTitle;
        return true;
    } else if (n == "name") {
        out << config.mBenchmarkName;
        return true;
    } else if (n == "unit") {
        out << config.mUnit;
        return true;
    } else if (n == "batch") {
        out << config.mBatch;
        return true;
    } else if (n == "complexityN") {
        out << config.mComplexityN;
        return true;
    } else if (n == "epochs") {
        out << config.mNumEpochs;
        return true;
    } else if (n == "clockResolution") {
        out << d(detail::clockResolution());
        return true;
    } else if (n == "clockResolutionMultiple") {
        out << config.mClockResolutionMultiple;
        return true;
    } else if (n == "maxEpochTime") {
        out << d(config.mMaxEpochTime);
        return true;
    } else if (n == "minEpochTime") {
        out << d(config.mMinEpochTime);
        return true;
    } else if (n == "minEpochIterations") {
        out << config.mMinEpochIterations;
        return true;
    } else if (n == "epochIterations") {
        out << config.mEpochIterations;
        return true;
    } else if (n == "warmup") {
        out << config.mWarmup;
        return true;
    } else if (n == "relative") {
        out << config.mIsRelative;
        return true;
    }
    return false;
}

static std::ostream& generateResultTag(Node const& n, Result const& r, std::ostream& out) {
    if (generateConfigTag(n, r.config(), out)) {
        return out;
    }
    // match e.g. "median(elapsed)"
    // g++ 4.8 doesn't implement std::regex :(
    // static std::regex const regOpArg1("^([a-zA-Z]+)\\(([a-zA-Z]*)\\)$");
    // std::cmatch matchResult;
    // if (std::regex_match(n.begin, n.end, matchResult, regOpArg1)) {
    std::vector<std::string> matchResult;
    if (matchCmdArgs(std::string(n.begin, n.end), matchResult)) {
        if (matchResult.size() == 2) {
            auto m = Result::fromString(matchResult[1]);
            if (m == Result::Measure::_size) {
                return out << 0.0;
            }

            if (matchResult[0] == "median") {
                return out << r.median(m);
            }
            if (matchResult[0] == "average") {
                return out << r.average(m);
            }
            if (matchResult[0] == "medianAbsolutePercentError") {
                return out << r.medianAbsolutePercentError(m);
            }
            if (matchResult[0] == "sum") {
                return out << r.sum(m);
            }
            if (matchResult[0] == "minimum") {
                return out << r.minimum(m);
            }
            if (matchResult[0] == "maximum") {
                return out << r.maximum(m);
            }
        } else if (matchResult.size() == 3) {
            auto m1 = Result::fromString(matchResult[1]);
            auto m2 = Result::fromString(matchResult[2]);
            if (m1 == Result::Measure::_size || m2 == Result::Measure::_size) {
                return out << 0.0;
            }

            if (matchResult[0] == "sumProduct") {
                return out << r.sumProduct(m1, m2);
            }
        }
    }

    // match e.g. "sumProduct(elapsed, iterations)"
    // static std::regex const regOpArg2("^([a-zA-Z]+)\\(([a-zA-Z]*)\\s*,\\s+([a-zA-Z]*)\\)$");

    // nothing matches :(
    throw std::runtime_error("command '" + std::string(n.begin, n.end) + "' not understood");
}

static void generateResultMeasurement(std::vector<Node> const& nodes, size_t idx, Result const& r, std::ostream& out) {
    for (auto const& n : nodes) {
        if (!generateFirstLast(n, idx, r.size(), out)) {
            ANKERL_NANOBENCH_LOG("n.type=" << static_cast<int>(n.type));
            switch (n.type) {
            case Node::Type::content:
                out.write(n.begin, std::distance(n.begin, n.end));
                break;

            case Node::Type::inverted_section:
                throw std::runtime_error("got a inverted section inside measurement");

            case Node::Type::section:
                throw std::runtime_error("got a section inside measurement");

            case Node::Type::tag: {
                auto m = Result::fromString(std::string(n.begin, n.end));
                if (m == Result::Measure::_size || !r.has(m)) {
                    out << 0.0;
                } else {
                    out << r.get(idx, m);
                }
                break;
            }
            }
        }
    }
}

static void generateResult(std::vector<Node> const& nodes, size_t idx, std::vector<Result> const& results, std::ostream& out) {
    auto const& r = results[idx];
    for (auto const& n : nodes) {
        if (!generateFirstLast(n, idx, results.size(), out)) {
            ANKERL_NANOBENCH_LOG("n.type=" << static_cast<int>(n.type));
            switch (n.type) {
            case Node::Type::content:
                out.write(n.begin, std::distance(n.begin, n.end));
                break;

            case Node::Type::inverted_section:
                throw std::runtime_error("got a inverted section inside result");

            case Node::Type::section:
                if (n == "measurement") {
                    for (size_t i = 0; i < r.size(); ++i) {
                        generateResultMeasurement(n.children, i, r, out);
                    }
                } else {
                    throw std::runtime_error("got a section inside result");
                }
                break;

            case Node::Type::tag:
                generateResultTag(n, r, out);
                break;
            }
        }
    }
}

} // namespace templates

// helper stuff that only intended to be used internally
namespace detail {

char const* getEnv(char const* name);
bool isEndlessRunning(std::string const& name);

template <typename T>
T parseFile(std::string const& filename);

void gatherStabilityInformation(std::vector<std::string>& warnings, std::vector<std::string>& recommendations);
void printStabilityInformationOnce(std::ostream* os);

// remembers the last table settings used. When it changes, a new table header is automatically written for the new entry.
uint64_t& singletonHeaderHash() noexcept;

// determines resolution of the given clock. This is done by measuring multiple times and returning the minimum time difference.
Clock::duration calcClockResolution(size_t numEvaluations) noexcept;

// formatting utilities
namespace fmt {

// adds thousands separator to numbers
ANKERL_NANOBENCH(IGNORE_PADDED_PUSH)
class NumSep : public std::numpunct<char> {
public:
    explicit NumSep(char sep);
    char do_thousands_sep() const override;
    std::string do_grouping() const override;

private:
    char mSep;
};
ANKERL_NANOBENCH(IGNORE_PADDED_POP)

// RAII to save & restore a stream's state
ANKERL_NANOBENCH(IGNORE_PADDED_PUSH)
class StreamStateRestorer {
public:
    explicit StreamStateRestorer(std::ostream& s);
    ~StreamStateRestorer();

    // sets back all stream info that we remembered at construction
    void restore();

    // don't allow copying / moving
    StreamStateRestorer(StreamStateRestorer const&) = delete;
    StreamStateRestorer& operator=(StreamStateRestorer const&) = delete;
    StreamStateRestorer(StreamStateRestorer&&) = delete;
    StreamStateRestorer& operator=(StreamStateRestorer&&) = delete;

private:
    std::ostream& mStream;
    std::locale mLocale;
    std::streamsize const mPrecision;
    std::streamsize const mWidth;
    std::ostream::char_type const mFill;
    std::ostream::fmtflags const mFmtFlags;
};
ANKERL_NANOBENCH(IGNORE_PADDED_POP)

// Number formatter
class Number {
public:
    Number(int width, int precision, double value);
    Number(int width, int precision, int64_t value);
    std::string to_s() const;

private:
    friend std::ostream& operator<<(std::ostream& os, Number const& n);
    std::ostream& write(std::ostream& os) const;

    int mWidth;
    int mPrecision;
    double mValue;
};

// helper replacement for std::to_string of signed/unsigned numbers so we are locale independent
std::string to_s(uint64_t s);

std::ostream& operator<<(std::ostream& os, Number const& n);

class MarkDownColumn {
public:
    MarkDownColumn(int w, int prec, std::string const& tit, std::string const& suff, double val);
    std::string title() const;
    std::string separator() const;
    std::string invalid() const;
    std::string value() const;

private:
    int mWidth;
    int mPrecision;
    std::string mTitle;
    std::string mSuffix;
    double mValue;
};

// Formats any text as markdown code, escaping backticks.
class MarkDownCode {
public:
    explicit MarkDownCode(std::string const& what);

private:
    friend std::ostream& operator<<(std::ostream& os, MarkDownCode const& mdCode);
    std::ostream& write(std::ostream& os) const;

    std::string mWhat{};
};

std::ostream& operator<<(std::ostream& os, MarkDownCode const& mdCode);

} // namespace fmt
} // namespace detail
} // namespace nanobench
} // namespace ankerl

// implementation /////////////////////////////////////////////////////////////////////////////////

namespace ankerl {
namespace nanobench {

void render(char const* mustacheTemplate, std::vector<Result> const& results, std::ostream& out) {
    detail::fmt::StreamStateRestorer restorer(out);

    out.precision(std::numeric_limits<double>::digits10);
    auto nodes = templates::parseMustacheTemplate(&mustacheTemplate);

    for (auto const& n : nodes) {
        ANKERL_NANOBENCH_LOG("n.type=" << static_cast<int>(n.type));
        switch (n.type) {
        case templates::Node::Type::content:
            out.write(n.begin, std::distance(n.begin, n.end));
            break;

        case templates::Node::Type::inverted_section:
            throw std::runtime_error("unknown list '" + std::string(n.begin, n.end) + "'");

        case templates::Node::Type::section:
            if (n == "result") {
                const size_t nbResults = results.size();
                for (size_t i = 0; i < nbResults; ++i) {
                    generateResult(n.children, i, results, out);
                }
            } else {
                throw std::runtime_error("unknown section '" + std::string(n.begin, n.end) + "'");
            }
            break;

        case templates::Node::Type::tag:
            // This just uses the last result's config.
            if (!generateConfigTag(n, results.back().config(), out)) {
                throw std::runtime_error("unknown tag '" + std::string(n.begin, n.end) + "'");
            }
            break;
        }
    }
}

void render(char const* mustacheTemplate, const Bench& bench, std::ostream& out) {
    render(mustacheTemplate, bench.results(), out);
}

namespace detail {

PerformanceCounters& performanceCounters() {
#    if defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wexit-time-destructors"
#    endif
    static PerformanceCounters pc;
#    if defined(__clang__)
#        pragma clang diagnostic pop
#    endif
    return pc;
}

// Windows version of doNotOptimizeAway
// see https://github.com/google/benchmark/blob/master/include/benchmark/benchmark.h#L307
// see https://github.com/facebook/folly/blob/master/folly/Benchmark.h#L280
// see https://docs.microsoft.com/en-us/cpp/preprocessor/optimize
#    if defined(_MSC_VER)
#        pragma optimize("", off)
void doNotOptimizeAwaySink(void const*) {}
#        pragma optimize("", on)
#    endif

template <typename T>
T parseFile(std::string const& filename) {
    std::ifstream fin(filename);
    T num{};
    fin >> num;
    return num;
}

char const* getEnv(char const* name) {
#    if defined(_MSC_VER)
#        pragma warning(push)
#        pragma warning(disable : 4996) // getenv': This function or variable may be unsafe.
#    endif
    return std::getenv(name);
#    if defined(_MSC_VER)
#        pragma warning(pop)
#    endif
}

bool isEndlessRunning(std::string const& name) {
    auto endless = getEnv("NANOBENCH_ENDLESS");
    return nullptr != endless && endless == name;
}

void gatherStabilityInformation(std::vector<std::string>& warnings, std::vector<std::string>& recommendations) {
    warnings.clear();
    recommendations.clear();

    bool recommendCheckFlags = false;

#    if defined(DEBUG)
    warnings.emplace_back("DEBUG defined");
    recommendCheckFlags = true;
#    endif

    bool recommendPyPerf = false;
#    if defined(__linux__)
    auto nprocs = sysconf(_SC_NPROCESSORS_CONF);
    if (nprocs <= 0) {
        warnings.emplace_back("couldn't figure out number of processors - no governor, turbo check possible");
    } else {

        // check frequency scaling
        for (long id = 0; id < nprocs; ++id) {
            auto idStr = detail::fmt::to_s(static_cast<uint64_t>(id));
            auto sysCpu = "/sys/devices/system/cpu/cpu" + idStr;
            auto minFreq = parseFile<int64_t>(sysCpu + "/cpufreq/scaling_min_freq");
            auto maxFreq = parseFile<int64_t>(sysCpu + "/cpufreq/scaling_max_freq");
            if (minFreq != maxFreq) {
                auto minMHz = static_cast<double>(minFreq) / 1000.0;
                auto maxMHz = static_cast<double>(maxFreq) / 1000.0;
                warnings.emplace_back("CPU frequency scaling enabled: CPU " + idStr + " between " +
                                      detail::fmt::Number(1, 1, minMHz).to_s() + " and " + detail::fmt::Number(1, 1, maxMHz).to_s() +
                                      " MHz");
                recommendPyPerf = true;
                break;
            }
        }

        auto currentGovernor = parseFile<std::string>("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        if ("performance" != currentGovernor) {
            warnings.emplace_back("CPU governor is '" + currentGovernor + "' but should be 'performance'");
            recommendPyPerf = true;
        }

        if (0 == parseFile<int>("/sys/devices/system/cpu/intel_pstate/no_turbo")) {
            warnings.emplace_back("Turbo is enabled, CPU frequency will fluctuate");
            recommendPyPerf = true;
        }
    }
#    endif

    if (recommendCheckFlags) {
        recommendations.emplace_back("Make sure you compile for Release");
    }
    if (recommendPyPerf) {
        recommendations.emplace_back("Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf");
    }
}

void printStabilityInformationOnce(std::ostream* outStream) {
    static bool shouldPrint = true;
    if (shouldPrint && outStream) {
        auto& os = *outStream;
        shouldPrint = false;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
        gatherStabilityInformation(warnings, recommendations);
        if (warnings.empty()) {
            return;
        }

        os << "Warning, results might be unstable:" << std::endl;
        for (auto const& w : warnings) {
            os << "* " << w << std::endl;
        }

        os << std::endl << "Recommendations" << std::endl;
        for (auto const& r : recommendations) {
            os << "* " << r << std::endl;
        }
    }
}

// remembers the last table settings used. When it changes, a new table header is automatically written for the new entry.
uint64_t& singletonHeaderHash() noexcept {
    static uint64_t sHeaderHash{};
    return sHeaderHash;
}

ANKERL_NANOBENCH_NO_SANITIZE("integer")
inline uint64_t fnv1a(std::string const& str) noexcept {
    auto val = UINT64_C(14695981039346656037);
    for (auto c : str) {
        val = (val ^ static_cast<uint8_t>(c)) * UINT64_C(1099511628211);
    }
    return val;
}

ANKERL_NANOBENCH_NO_SANITIZE("integer")
inline uint64_t hash_combine(uint64_t seed, uint64_t val) {
    return seed ^ (val + UINT64_C(0x9e3779b9) + (seed << 6U) + (seed >> 2U));
}

// determines resolution of the given clock. This is done by measuring multiple times and returning the minimum time difference.
Clock::duration calcClockResolution(size_t numEvaluations) noexcept {
    auto bestDuration = Clock::duration::max();
    Clock::time_point tBegin;
    Clock::time_point tEnd;
    for (size_t i = 0; i < numEvaluations; ++i) {
        tBegin = Clock::now();
        do {
            tEnd = Clock::now();
        } while (tBegin == tEnd);
        bestDuration = (std::min)(bestDuration, tEnd - tBegin);
    }
    return bestDuration;
}

// Calculates clock resolution once, and remembers the result
Clock::duration clockResolution() noexcept {
    static Clock::duration sResolution = calcClockResolution(20);
    return sResolution;
}

ANKERL_NANOBENCH(IGNORE_PADDED_PUSH)
struct IterationLogic::Impl {
    enum class State { warmup, upscaling_runtime, measuring, endless };

    explicit Impl(Bench const& bench)
        : mBench(bench)
        , mResult(bench.config()) {
        printStabilityInformationOnce(mBench.output());

        // determine target runtime per epoch
        mTargetRuntimePerEpoch = detail::clockResolution() * mBench.clockResolutionMultiple();
        if (mTargetRuntimePerEpoch > mBench.maxEpochTime()) {
            mTargetRuntimePerEpoch = mBench.maxEpochTime();
        }
        if (mTargetRuntimePerEpoch < mBench.minEpochTime()) {
            mTargetRuntimePerEpoch = mBench.minEpochTime();
        }

        if (isEndlessRunning(mBench.name())) {
            std::cerr << "NANOBENCH_ENDLESS set: running '" << mBench.name() << "' endlessly" << std::endl;
            mNumIters = (std::numeric_limits<uint64_t>::max)();
            mState = State::endless;
        } else if (0 != mBench.warmup()) {
            mNumIters = mBench.warmup();
            mState = State::warmup;
        } else if (0 != mBench.epochIterations()) {
            // exact number of iterations
            mNumIters = mBench.epochIterations();
            mState = State::measuring;
        } else {
            mNumIters = mBench.minEpochIterations();
            mState = State::upscaling_runtime;
        }
    }

    // directly calculates new iters based on elapsed&iters, and adds a 10% noise. Makes sure we don't underflow.
    ANKERL_NANOBENCH(NODISCARD) uint64_t calcBestNumIters(std::chrono::nanoseconds elapsed, uint64_t iters) noexcept {
        auto doubleElapsed = d(elapsed);
        auto doubleTargetRuntimePerEpoch = d(mTargetRuntimePerEpoch);
        auto doubleNewIters = doubleTargetRuntimePerEpoch / doubleElapsed * d(iters);

        auto doubleMinEpochIters = d(mBench.minEpochIterations());
        if (doubleNewIters < doubleMinEpochIters) {
            doubleNewIters = doubleMinEpochIters;
        }
        doubleNewIters *= 1.0 + 0.2 * mRng.uniform01();

        // +0.5 for correct rounding when casting
        // NOLINTNEXTLINE(bugprone-incorrect-roundings)
        return static_cast<uint64_t>(doubleNewIters + 0.5);
    }

    ANKERL_NANOBENCH_NO_SANITIZE("integer") void upscale(std::chrono::nanoseconds elapsed) {
        if (elapsed * 10 < mTargetRuntimePerEpoch) {
            // we are far below the target runtime. Multiply iterations by 10 (with overflow check)
            if (mNumIters * 10 < mNumIters) {
                // overflow :-(
                showResult("iterations overflow. Maybe your code got optimized away?");
                mNumIters = 0;
                return;
            }
            mNumIters *= 10;
        } else {
            mNumIters = calcBestNumIters(elapsed, mNumIters);
        }
    }

    void add(std::chrono::nanoseconds elapsed, PerformanceCounters const& pc) noexcept {
#    if defined(ANKERL_NANOBENCH_LOG_ENABLED)
        auto oldIters = mNumIters;
#    endif

        switch (mState) {
        case State::warmup:
            if (isCloseEnoughForMeasurements(elapsed)) {
                // if elapsed is close enough, we can skip upscaling and go right to measurements
                // still, we don't add the result to the measurements.
                mState = State::measuring;
                mNumIters = calcBestNumIters(elapsed, mNumIters);
            } else {
                // not close enough: switch to upscaling
                mState = State::upscaling_runtime;
                upscale(elapsed);
            }
            break;

        case State::upscaling_runtime:
            if (isCloseEnoughForMeasurements(elapsed)) {
                // if we are close enough, add measurement and switch to always measuring
                mState = State::measuring;
                mTotalElapsed += elapsed;
                mTotalNumIters += mNumIters;
                mResult.add(elapsed, mNumIters, pc);
                mNumIters = calcBestNumIters(mTotalElapsed, mTotalNumIters);
            } else {
                upscale(elapsed);
            }
            break;

        case State::measuring:
            // just add measurements - no questions asked. Even when runtime is low. But we can't ignore
            // that fluctuation, or else we would bias the result
            mTotalElapsed += elapsed;
            mTotalNumIters += mNumIters;
            mResult.add(elapsed, mNumIters, pc);
            if (0 != mBench.epochIterations()) {
                mNumIters = mBench.epochIterations();
            } else {
                mNumIters = calcBestNumIters(mTotalElapsed, mTotalNumIters);
            }
            break;

        case State::endless:
            mNumIters = (std::numeric_limits<uint64_t>::max)();
            break;
        }

        if (static_cast<uint64_t>(mResult.size()) == mBench.epochs()) {
            // we got all the results that we need, finish it
            showResult("");
            mNumIters = 0;
        }

        ANKERL_NANOBENCH_LOG(mBench.name() << ": " << detail::fmt::Number(20, 3, static_cast<double>(elapsed.count())) << " elapsed, "
                                           << detail::fmt::Number(20, 3, static_cast<double>(mTargetRuntimePerEpoch.count()))
                                           << " target. oldIters=" << oldIters << ", mNumIters=" << mNumIters
                                           << ", mState=" << static_cast<int>(mState));
    }

    void showResult(std::string const& errorMessage) const {
        ANKERL_NANOBENCH_LOG(errorMessage);

        if (mBench.output() != nullptr) {
            // prepare column data ///////
            std::vector<fmt::MarkDownColumn> columns;

            auto rMedian = mResult.median(Result::Measure::elapsed);

            if (mBench.relative()) {
                double d = 100.0;
                if (!mBench.results().empty()) {
                    d = rMedian <= 0.0 ? 0.0 : mBench.results().front().median(Result::Measure::elapsed) / rMedian * 100.0;
                }
                columns.emplace_back(11, 1, "relative", "%", d);
            }

            if (mBench.complexityN() > 0) {
                columns.emplace_back(14, 0, "complexityN", "", mBench.complexityN());
            }

            columns.emplace_back(22, 2, "ns/" + mBench.unit(), "", 1e9 * rMedian / mBench.batch());
            columns.emplace_back(22, 2, mBench.unit() + "/s", "", rMedian <= 0.0 ? 0.0 : mBench.batch() / rMedian);

            double rErrorMedian = mResult.medianAbsolutePercentError(Result::Measure::elapsed);
            columns.emplace_back(10, 1, "err%", "%", rErrorMedian * 100.0);

            double rInsMedian = -1.0;
            if (mResult.has(Result::Measure::instructions)) {
                rInsMedian = mResult.median(Result::Measure::instructions);
                columns.emplace_back(18, 2, "ins/" + mBench.unit(), "", rInsMedian / mBench.batch());
            }

            double rCycMedian = -1.0;
            if (mResult.has(Result::Measure::cpucycles)) {
                rCycMedian = mResult.median(Result::Measure::cpucycles);
                columns.emplace_back(18, 2, "cyc/" + mBench.unit(), "", rCycMedian / mBench.batch());
            }
            if (rInsMedian > 0.0 && rCycMedian > 0.0) {
                columns.emplace_back(9, 3, "IPC", "", rCycMedian <= 0.0 ? 0.0 : rInsMedian / rCycMedian);
            }
            if (mResult.has(Result::Measure::branchinstructions)) {
                double rBraMedian = mResult.median(Result::Measure::branchinstructions);
                columns.emplace_back(17, 2, "bra/" + mBench.unit(), "", rBraMedian / mBench.batch());
                if (mResult.has(Result::Measure::branchmisses)) {
                    double p = 0.0;
                    if (rBraMedian >= 1e-9) {
                        p = 100.0 * mResult.median(Result::Measure::branchmisses) / rBraMedian;
                    }
                    columns.emplace_back(10, 1, "miss%", "%", p);
                }
            }

            columns.emplace_back(12, 2, "total", "", mResult.sum(Result::Measure::elapsed));

            // write everything
            auto& os = *mBench.output();

            uint64_t hash = 0;
            hash = hash_combine(fnv1a(mBench.unit()), hash);
            hash = hash_combine(fnv1a(mBench.title()), hash);
            hash = hash_combine(mBench.relative(), hash);
            hash = hash_combine(mBench.performanceCounters(), hash);

            if (hash != singletonHeaderHash()) {
                singletonHeaderHash() = hash;

                // no result yet, print header
                os << std::endl;
                for (auto const& col : columns) {
                    os << col.title();
                }
                os << "| " << mBench.title() << std::endl;

                for (auto const& col : columns) {
                    os << col.separator();
                }
                os << "|:" << std::string(mBench.title().size() + 1U, '-') << std::endl;
            }

            if (!errorMessage.empty()) {
                for (auto const& col : columns) {
                    os << col.invalid();
                }
                os << "| :boom: " << fmt::MarkDownCode(mBench.name()) << " (" << errorMessage << ')' << std::endl;
            } else {
                for (auto const& col : columns) {
                    os << col.value();
                }
                os << "| ";
                auto showUnstable = rErrorMedian >= 0.05;
                if (showUnstable) {
                    os << ":wavy_dash: ";
                }
                os << fmt::MarkDownCode(mBench.name());
                if (showUnstable) {
                    auto avgIters = static_cast<double>(mTotalNumIters) / static_cast<double>(mBench.epochs());
                    // NOLINTNEXTLINE(bugprone-incorrect-roundings)
                    auto suggestedIters = static_cast<uint64_t>(avgIters * 10 + 0.5);

                    os << " (Unstable with ~" << detail::fmt::Number(1, 1, avgIters)
                       << " iters. Increase `minEpochIterations` to e.g. " << suggestedIters << ")";
                }
                os << std::endl;
            }
        }
    }

    ANKERL_NANOBENCH(NODISCARD) bool isCloseEnoughForMeasurements(std::chrono::nanoseconds elapsed) const noexcept {
        return elapsed * 3 >= mTargetRuntimePerEpoch * 2;
    }

    uint64_t mNumIters = 1;
    Bench const& mBench;
    std::chrono::nanoseconds mTargetRuntimePerEpoch{};
    Result mResult;
    Rng mRng{123};
    std::chrono::nanoseconds mTotalElapsed{};
    uint64_t mTotalNumIters = 0;

    State mState = State::upscaling_runtime;
};
ANKERL_NANOBENCH(IGNORE_PADDED_POP)

IterationLogic::IterationLogic(Bench const& bench) noexcept
    : mPimpl(new Impl(bench)) {}

IterationLogic::~IterationLogic() {
    if (mPimpl) {
        delete mPimpl;
    }
}

uint64_t IterationLogic::numIters() const noexcept {
    ANKERL_NANOBENCH_LOG(mPimpl->mBench.name() << ": mNumIters=" << mPimpl->mNumIters);
    return mPimpl->mNumIters;
}

void IterationLogic::add(std::chrono::nanoseconds elapsed, PerformanceCounters const& pc) noexcept {
    mPimpl->add(elapsed, pc);
}

void IterationLogic::moveResultTo(std::vector<Result>& results) noexcept {
    results.emplace_back(std::move(mPimpl->mResult));
}

#    if ANKERL_NANOBENCH(PERF_COUNTERS)

ANKERL_NANOBENCH(IGNORE_PADDED_PUSH)
class LinuxPerformanceCounters {
public:
    struct Target {
        Target(uint64_t* targetValue_, bool correctMeasuringOverhead_, bool correctLoopOverhead_)
            : targetValue(targetValue_)
            , correctMeasuringOverhead(correctMeasuringOverhead_)
            , correctLoopOverhead(correctLoopOverhead_) {}

        uint64_t* targetValue{};
        bool correctMeasuringOverhead{};
        bool correctLoopOverhead{};
    };

    ~LinuxPerformanceCounters();

    // quick operation
    inline void start() {}

    inline void stop() {}

    bool monitor(perf_sw_ids swId, Target target);
    bool monitor(perf_hw_id hwId, Target target);

    bool hasError() const noexcept {
        return mHasError;
    }

    // Just reading data is faster than enable & disabling.
    // we subtract data ourselves.
    inline void beginMeasure() {
        if (mHasError) {
            return;
        }

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        mHasError = -1 == ioctl(mFd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
        if (mHasError) {
            return;
        }

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        mHasError = -1 == ioctl(mFd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    }

    inline void endMeasure() {
        if (mHasError) {
            return;
        }

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        mHasError = (-1 == ioctl(mFd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP));
        if (mHasError) {
            return;
        }

        auto const numBytes = sizeof(uint64_t) * mCounters.size();
        auto ret = read(mFd, mCounters.data(), numBytes);
        mHasError = ret != static_cast<ssize_t>(numBytes);
    }

    void updateResults(uint64_t numIters);

    // rounded integer division
    template <typename T>
    static inline T divRounded(T a, T divisor) {
        return (a + divisor / 2) / divisor;
    }

    template <typename Op>
    ANKERL_NANOBENCH_NO_SANITIZE("integer")
    void calibrate(Op&& op) {
        // clear current calibration data,
        for (auto& v : mCalibratedOverhead) {
            v = UINT64_C(0);
        }

        // create new calibration data
        auto newCalibration = mCalibratedOverhead;
        for (auto& v : newCalibration) {
            v = (std::numeric_limits<uint64_t>::max)();
        }
        for (size_t iter = 0; iter < 100; ++iter) {
            beginMeasure();
            op();
            endMeasure();
            if (mHasError) {
                return;
            }

            for (size_t i = 0; i < newCalibration.size(); ++i) {
                auto diff = mCounters[i];
                if (newCalibration[i] > diff) {
                    newCalibration[i] = diff;
                }
            }
        }

        mCalibratedOverhead = std::move(newCalibration);

        {
            // calibrate loop overhead. For branches & instructions this makes sense, not so much for everything else like cycles.
            // marsaglia's xorshift: mov, sal/shr, xor. Times 3.
            // This has the nice property that the compiler doesn't seem to be able to optimize multiple calls any further.
            // see https://godbolt.org/z/49RVQ5
            uint64_t const numIters = 100000U + (std::random_device{}() & 3);
            uint64_t n = numIters;
            uint32_t x = 1234567;
            auto fn = [&]() {
                x ^= x << 13;
                x ^= x >> 17;
                x ^= x << 5;
            };

            beginMeasure();
            while (n-- > 0) {
                fn();
            }
            endMeasure();
            detail::doNotOptimizeAway(x);
            auto measure1 = mCounters;

            n = numIters;
            beginMeasure();
            while (n-- > 0) {
                // we now run *twice* so we can easily calculate the overhead
                fn();
                fn();
            }
            endMeasure();
            detail::doNotOptimizeAway(x);
            auto measure2 = mCounters;

            for (size_t i = 0; i < mCounters.size(); ++i) {
                // factor 2 because we have two instructions per loop
                auto m1 = measure1[i] > mCalibratedOverhead[i] ? measure1[i] - mCalibratedOverhead[i] : 0;
                auto m2 = measure2[i] > mCalibratedOverhead[i] ? measure2[i] - mCalibratedOverhead[i] : 0;
                auto overhead = m1 * 2 > m2 ? m1 * 2 - m2 : 0;

                mLoopOverhead[i] = divRounded(overhead, numIters);
            }
        }
    }

private:
    bool monitor(uint32_t type, uint64_t eventid, Target target);

    std::map<uint64_t, Target> mIdToTarget{};

    // start with minimum size of 3 for read_format
    std::vector<uint64_t> mCounters{3};
    std::vector<uint64_t> mCalibratedOverhead{3};
    std::vector<uint64_t> mLoopOverhead{3};

    uint64_t mTimeEnabledNanos = 0;
    uint64_t mTimeRunningNanos = 0;
    int mFd = -1;
    bool mHasError = false;
};
ANKERL_NANOBENCH(IGNORE_PADDED_POP)

LinuxPerformanceCounters::~LinuxPerformanceCounters() {
    if (-1 != mFd) {
        close(mFd);
    }
}

bool LinuxPerformanceCounters::monitor(perf_sw_ids swId, LinuxPerformanceCounters::Target target) {
    return monitor(PERF_TYPE_SOFTWARE, swId, target);
}

bool LinuxPerformanceCounters::monitor(perf_hw_id hwId, LinuxPerformanceCounters::Target target) {
    return monitor(PERF_TYPE_HARDWARE, hwId, target);
}

// overflow is ok, it's checked
ANKERL_NANOBENCH_NO_SANITIZE("integer")
void LinuxPerformanceCounters::updateResults(uint64_t numIters) {
    // clear old data
    for (auto& id_value : mIdToTarget) {
        *id_value.second.targetValue = UINT64_C(0);
    }

    if (mHasError) {
        return;
    }

    mTimeEnabledNanos = mCounters[1] - mCalibratedOverhead[1];
    mTimeRunningNanos = mCounters[2] - mCalibratedOverhead[2];

    for (uint64_t i = 0; i < mCounters[0]; ++i) {
        auto idx = static_cast<size_t>(3 + i * 2 + 0);
        auto id = mCounters[idx + 1U];

        auto it = mIdToTarget.find(id);
        if (it != mIdToTarget.end()) {

            auto& tgt = it->second;
            *tgt.targetValue = mCounters[idx];
            if (tgt.correctMeasuringOverhead) {
                if (*tgt.targetValue >= mCalibratedOverhead[idx]) {
                    *tgt.targetValue -= mCalibratedOverhead[idx];
                } else {
                    *tgt.targetValue = 0U;
                }
            }
            if (tgt.correctLoopOverhead) {
                auto correctionVal = mLoopOverhead[idx] * numIters;
                if (*tgt.targetValue >= correctionVal) {
                    *tgt.targetValue -= correctionVal;
                } else {
                    *tgt.targetValue = 0U;
                }
            }
        }
    }
}

bool LinuxPerformanceCounters::monitor(uint32_t type, uint64_t eventid, Target target) {
    *target.targetValue = (std::numeric_limits<uint64_t>::max)();
    if (mHasError) {
        return false;
    }

    auto pea = perf_event_attr();
    std::memset(&pea, 0, sizeof(perf_event_attr));
    pea.type = type;
    pea.size = sizeof(perf_event_attr);
    pea.config = eventid;
    pea.disabled = 1; // start counter as disabled
    pea.exclude_kernel = 1;
    pea.exclude_hv = 1;

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;

    const int pid = 0;                    // the current process
    const int cpu = -1;                   // all CPUs
#        if defined(PERF_FLAG_FD_CLOEXEC) // since Linux 3.14
    const unsigned long flags = PERF_FLAG_FD_CLOEXEC;
#        else
    const unsigned long flags = 0;
#        endif

    auto fd = static_cast<int>(syscall(__NR_perf_event_open, &pea, pid, cpu, mFd, flags));
    if (-1 == fd) {
        return false;
    }
    if (-1 == mFd) {
        // first call: set to fd, and use this from now on
        mFd = fd;
    }
    uint64_t id = 0;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    if (-1 == ioctl(fd, PERF_EVENT_IOC_ID, &id)) {
        // couldn't get id
        return false;
    }

    // insert into map, rely on the fact that map's references are constant.
    mIdToTarget.emplace(id, target);

    // prepare readformat with the correct size (after the insert)
    auto size = 3 + 2 * mIdToTarget.size();
    mCounters.resize(size);
    mCalibratedOverhead.resize(size);
    mLoopOverhead.resize(size);

    return true;
}

PerformanceCounters::PerformanceCounters()
    : mPc(new LinuxPerformanceCounters())
    , mVal()
    , mHas() {

    mHas.pageFaults = mPc->monitor(PERF_COUNT_SW_PAGE_FAULTS, LinuxPerformanceCounters::Target(&mVal.pageFaults, true, false));
    mHas.cpuCycles = mPc->monitor(PERF_COUNT_HW_REF_CPU_CYCLES, LinuxPerformanceCounters::Target(&mVal.cpuCycles, true, false));
    mHas.contextSwitches =
        mPc->monitor(PERF_COUNT_SW_CONTEXT_SWITCHES, LinuxPerformanceCounters::Target(&mVal.contextSwitches, true, false));
    mHas.instructions = mPc->monitor(PERF_COUNT_HW_INSTRUCTIONS, LinuxPerformanceCounters::Target(&mVal.instructions, true, true));
    mHas.branchInstructions =
        mPc->monitor(PERF_COUNT_HW_BRANCH_INSTRUCTIONS, LinuxPerformanceCounters::Target(&mVal.branchInstructions, true, false));
    mHas.branchMisses = mPc->monitor(PERF_COUNT_HW_BRANCH_MISSES, LinuxPerformanceCounters::Target(&mVal.branchMisses, true, false));
    // mHas.branchMisses = false;

    mPc->start();
    mPc->calibrate([] {
        auto before = ankerl::nanobench::Clock::now();
        auto after = ankerl::nanobench::Clock::now();
        (void)before;
        (void)after;
    });

    if (mPc->hasError()) {
        // something failed, don't monitor anything.
        mHas = PerfCountSet<bool>{};
    }
}

PerformanceCounters::~PerformanceCounters() {
    if (nullptr != mPc) {
        delete mPc;
    }
}

void PerformanceCounters::beginMeasure() {
    mPc->beginMeasure();
}

void PerformanceCounters::endMeasure() {
    mPc->endMeasure();
}

void PerformanceCounters::updateResults(uint64_t numIters) {
    mPc->updateResults(numIters);
}

#    else

PerformanceCounters::PerformanceCounters() = default;
PerformanceCounters::~PerformanceCounters() = default;
void PerformanceCounters::beginMeasure() {}
void PerformanceCounters::endMeasure() {}
void PerformanceCounters::updateResults(uint64_t) {}

#    endif

ANKERL_NANOBENCH(NODISCARD) PerfCountSet<uint64_t> const& PerformanceCounters::val() const noexcept {
    return mVal;
}
ANKERL_NANOBENCH(NODISCARD) PerfCountSet<bool> const& PerformanceCounters::has() const noexcept {
    return mHas;
}

// formatting utilities
namespace fmt {

// adds thousands separator to numbers
NumSep::NumSep(char sep)
    : mSep(sep) {}

char NumSep::do_thousands_sep() const {
    return mSep;
}

std::string NumSep::do_grouping() const {
    return "\003";
}

// RAII to save & restore a stream's state
StreamStateRestorer::StreamStateRestorer(std::ostream& s)
    : mStream(s)
    , mLocale(s.getloc())
    , mPrecision(s.precision())
    , mWidth(s.width())
    , mFill(s.fill())
    , mFmtFlags(s.flags()) {}

StreamStateRestorer::~StreamStateRestorer() {
    restore();
}

// sets back all stream info that we remembered at construction
void StreamStateRestorer::restore() {
    mStream.imbue(mLocale);
    mStream.precision(mPrecision);
    mStream.width(mWidth);
    mStream.fill(mFill);
    mStream.flags(mFmtFlags);
}

Number::Number(int width, int precision, int64_t value)
    : mWidth(width)
    , mPrecision(precision)
    , mValue(static_cast<double>(value)) {}

Number::Number(int width, int precision, double value)
    : mWidth(width)
    , mPrecision(precision)
    , mValue(value) {}

std::ostream& Number::write(std::ostream& os) const {
    StreamStateRestorer restorer(os);
    os.imbue(std::locale(os.getloc(), new NumSep(',')));
    os << std::setw(mWidth) << std::setprecision(mPrecision) << std::fixed << mValue;
    return os;
}

std::string Number::to_s() const {
    std::stringstream ss;
    write(ss);
    return ss.str();
}

std::string to_s(uint64_t n) {
    std::string str;
    do {
        str += static_cast<char>('0' + static_cast<char>(n % 10));
        n /= 10;
    } while (n != 0);
    std::reverse(str.begin(), str.end());
    return str;
}

std::ostream& operator<<(std::ostream& os, Number const& n) {
    return n.write(os);
}

MarkDownColumn::MarkDownColumn(int w, int prec, std::string const& tit, std::string const& suff, double val)
    : mWidth(w)
    , mPrecision(prec)
    , mTitle(tit)
    , mSuffix(suff)
    , mValue(val) {}

std::string MarkDownColumn::title() const {
    std::stringstream ss;
    ss << '|' << std::setw(mWidth - 2) << std::right << mTitle << ' ';
    return ss.str();
}

std::string MarkDownColumn::separator() const {
    std::string sep(static_cast<size_t>(mWidth), '-');
    sep.front() = '|';
    sep.back() = ':';
    return sep;
}

std::string MarkDownColumn::invalid() const {
    std::string sep(static_cast<size_t>(mWidth), ' ');
    sep.front() = '|';
    sep[sep.size() - 2] = '-';
    return sep;
}

std::string MarkDownColumn::value() const {
    std::stringstream ss;
    auto width = mWidth - 2 - static_cast<int>(mSuffix.size());
    ss << '|' << Number(width, mPrecision, mValue) << mSuffix << ' ';
    return ss.str();
}

// Formats any text as markdown code, escaping backticks.
MarkDownCode::MarkDownCode(std::string const& what) {
    mWhat.reserve(what.size() + 2);
    mWhat.push_back('`');
    for (char c : what) {
        mWhat.push_back(c);
        if ('`' == c) {
            mWhat.push_back('`');
        }
    }
    mWhat.push_back('`');
}

std::ostream& MarkDownCode::write(std::ostream& os) const {
    return os << mWhat;
}

std::ostream& operator<<(std::ostream& os, MarkDownCode const& mdCode) {
    return mdCode.write(os);
}
} // namespace fmt
} // namespace detail

// provide implementation here so it's only generated once
Config::Config() = default;
Config::~Config() = default;
Config& Config::operator=(Config const&) = default;
Config& Config::operator=(Config&&) = default;
Config::Config(Config const&) = default;
Config::Config(Config&&) noexcept = default;

// provide implementation here so it's only generated once
Result::~Result() = default;
Result& Result::operator=(Result const&) = default;
Result& Result::operator=(Result&&) = default;
Result::Result(Result const&) = default;
Result::Result(Result&&) noexcept = default;

namespace detail {
template <typename T>
inline constexpr typename std::underlying_type<T>::type u(T val) noexcept {
    return static_cast<typename std::underlying_type<T>::type>(val);
}
} // namespace detail

// Result returned after a benchmark has finished. Can be used as a baseline for relative().
Result::Result(Config const& benchmarkConfig)
    : mConfig(benchmarkConfig)
    , mNameToMeasurements{detail::u(Result::Measure::_size)} {}

void Result::add(Clock::duration totalElapsed, uint64_t iters, detail::PerformanceCounters const& pc) {
    using detail::d;
    using detail::u;

    double dIters = d(iters);
    mNameToMeasurements[u(Result::Measure::iterations)].push_back(dIters);

    mNameToMeasurements[u(Result::Measure::elapsed)].push_back(d(totalElapsed) / dIters);
    if (pc.has().pageFaults) {
        mNameToMeasurements[u(Result::Measure::pagefaults)].push_back(d(pc.val().pageFaults) / dIters);
    }
    if (pc.has().cpuCycles) {
        mNameToMeasurements[u(Result::Measure::cpucycles)].push_back(d(pc.val().cpuCycles) / dIters);
    }
    if (pc.has().contextSwitches) {
        mNameToMeasurements[u(Result::Measure::contextswitches)].push_back(d(pc.val().contextSwitches) / dIters);
    }
    if (pc.has().instructions) {
        mNameToMeasurements[u(Result::Measure::instructions)].push_back(d(pc.val().instructions) / dIters);
    }
    if (pc.has().branchInstructions) {
        double branchInstructions = 0.0;
        // correcting branches: remove branch introduced by the while (...) loop for each iteration.
        if (pc.val().branchInstructions > iters + 1U) {
            branchInstructions = d(pc.val().branchInstructions - (iters + 1U));
        }
        mNameToMeasurements[u(Result::Measure::branchinstructions)].push_back(branchInstructions / dIters);

        if (pc.has().branchMisses) {
            // correcting branch misses
            double branchMisses = d(pc.val().branchMisses);
            if (branchMisses > branchInstructions) {
                // can't have branch misses when there were branches...
                branchMisses = branchInstructions;
            }

            // assuming at least one missed branch for the loop
            branchMisses -= 1.0;
            if (branchMisses < 1.0) {
                branchMisses = 1.0;
            }
            mNameToMeasurements[u(Result::Measure::branchmisses)].push_back(branchMisses / dIters);
        }
    }
}

Config const& Result::config() const noexcept {
    return mConfig;
}

inline double calcMedian(std::vector<double>& data) {
    if (data.empty()) {
        return 0.0;
    }
    std::sort(data.begin(), data.end());

    auto midIdx = data.size() / 2U;
    if (1U == (data.size() & 1U)) {
        return data[midIdx];
    }
    return (data[midIdx - 1U] + data[midIdx]) / 2U;
}

double Result::median(Measure m) const {
    // create a copy so we can sort
    auto data = mNameToMeasurements[detail::u(m)];
    return calcMedian(data);
}

double Result::average(Measure m) const {
    using detail::d;
    auto const& data = mNameToMeasurements[detail::u(m)];
    if (data.empty()) {
        return 0.0;
    }

    // create a copy so we can sort
    return sum(m) / d(data.size());
}

double Result::medianAbsolutePercentError(Measure m) const {
    // create copy
    auto data = mNameToMeasurements[detail::u(m)];

    // calculates MdAPE which is the median of percentage error
    // see https://www.spiderfinancial.com/support/documentation/numxl/reference-manual/forecasting-performance/mdape
    auto med = calcMedian(data);

    // transform the data to absolute error
    for (auto& x : data) {
        x = (x - med) / x;
        if (x < 0) {
            x = -x;
        }
    }
    return calcMedian(data);
}

double Result::sum(Measure m) const noexcept {
    auto const& data = mNameToMeasurements[detail::u(m)];
    return std::accumulate(data.begin(), data.end(), 0.0);
}

double Result::sumProduct(Measure m1, Measure m2) const noexcept {
    auto const& data1 = mNameToMeasurements[detail::u(m1)];
    auto const& data2 = mNameToMeasurements[detail::u(m2)];

    if (data1.size() != data2.size()) {
        return 0.0;
    }

    double result = 0.0;
    for (size_t i = 0, s = data1.size(); i != s; ++i) {
        result += data1[i] * data2[i];
    }
    return result;
}

bool Result::has(Measure m) const noexcept {
    return !mNameToMeasurements[detail::u(m)].empty();
}

double Result::get(size_t idx, Measure m) const {
    auto const& data = mNameToMeasurements[detail::u(m)];
    return data.at(idx);
}

bool Result::empty() const noexcept {
    return 0U == size();
}

size_t Result::size() const noexcept {
    auto const& data = mNameToMeasurements[detail::u(Measure::elapsed)];
    return data.size();
}

double Result::minimum(Measure m) const noexcept {
    auto const& data = mNameToMeasurements[detail::u(m)];
    if (data.empty()) {
        return 0.0;
    }

    // here its save to assume that at least one element is there
    return *std::min_element(data.begin(), data.end());
}

double Result::maximum(Measure m) const noexcept {
    auto const& data = mNameToMeasurements[detail::u(m)];
    if (data.empty()) {
        return 0.0;
    }

    // here its save to assume that at least one element is there
    return *std::max_element(data.begin(), data.end());
}

Result::Measure Result::fromString(std::string const& str) {
    if (str == "elapsed") {
        return Measure::elapsed;
    } else if (str == "iterations") {
        return Measure::iterations;
    } else if (str == "pagefaults") {
        return Measure::pagefaults;
    } else if (str == "cpucycles") {
        return Measure::cpucycles;
    } else if (str == "contextswitches") {
        return Measure::contextswitches;
    } else if (str == "instructions") {
        return Measure::instructions;
    } else if (str == "branchinstructions") {
        return Measure::branchinstructions;
    } else if (str == "branchmisses") {
        return Measure::branchmisses;
    } else {
        // not found, return _size
        return Measure::_size;
    }
}

// Configuration of a microbenchmark.
Bench::Bench() {
    mConfig.mOut = &std::cout;
}

Bench::Bench(Bench&&) = default;
Bench& Bench::operator=(Bench&&) = default;
Bench::Bench(Bench const&) = default;
Bench& Bench::operator=(Bench const&) = default;
Bench::~Bench() noexcept = default;

double Bench::batch() const noexcept {
    return mConfig.mBatch;
}

double Bench::complexityN() const noexcept {
    return mConfig.mComplexityN;
}

// Set a baseline to compare it to. 100% it is exactly as fast as the baseline, >100% means it is faster than the baseline, <100%
// means it is slower than the baseline.
Bench& Bench::relative(bool isRelativeEnabled) noexcept {
    mConfig.mIsRelative = isRelativeEnabled;
    return *this;
}
bool Bench::relative() const noexcept {
    return mConfig.mIsRelative;
}

Bench& Bench::performanceCounters(bool showPerformanceCounters) noexcept {
    mConfig.mShowPerformanceCounters = showPerformanceCounters;
    return *this;
}
bool Bench::performanceCounters() const noexcept {
    return mConfig.mShowPerformanceCounters;
}

// Operation unit. Defaults to "op", could be e.g. "byte" for string processing.
// If u differs from currently set unit, the stored results will be cleared.
// Use singular (byte, not bytes).
Bench& Bench::unit(char const* u) {
    if (u != mConfig.mUnit) {
        mResults.clear();
    }
    mConfig.mUnit = u;
    return *this;
}

Bench& Bench::unit(std::string const& u) {
    return unit(u.c_str());
}

std::string const& Bench::unit() const noexcept {
    return mConfig.mUnit;
}

// If benchmarkTitle differs from currently set title, the stored results will be cleared.
Bench& Bench::title(const char* benchmarkTitle) {
    if (benchmarkTitle != mConfig.mBenchmarkTitle) {
        mResults.clear();
    }
    mConfig.mBenchmarkTitle = benchmarkTitle;
    return *this;
}
Bench& Bench::title(std::string const& benchmarkTitle) {
    if (benchmarkTitle != mConfig.mBenchmarkTitle) {
        mResults.clear();
    }
    mConfig.mBenchmarkTitle = benchmarkTitle;
    return *this;
}

std::string const& Bench::title() const noexcept {
    return mConfig.mBenchmarkTitle;
}

Bench& Bench::name(const char* benchmarkName) {
    mConfig.mBenchmarkName = benchmarkName;
    return *this;
}

Bench& Bench::name(std::string const& benchmarkName) {
    mConfig.mBenchmarkName = benchmarkName;
    return *this;
}

std::string const& Bench::name() const noexcept {
    return mConfig.mBenchmarkName;
}

// Number of epochs to evaluate. The reported result will be the median of evaluation of each epoch.
Bench& Bench::epochs(size_t numEpochs) noexcept {
    mConfig.mNumEpochs = numEpochs;
    return *this;
}
size_t Bench::epochs() const noexcept {
    return mConfig.mNumEpochs;
}

// Desired evaluation time is a multiple of clock resolution. Default is to be 1000 times above this measurement precision.
Bench& Bench::clockResolutionMultiple(size_t multiple) noexcept {
    mConfig.mClockResolutionMultiple = multiple;
    return *this;
}
size_t Bench::clockResolutionMultiple() const noexcept {
    return mConfig.mClockResolutionMultiple;
}

// Sets the maximum time each epoch should take. Default is 100ms.
Bench& Bench::maxEpochTime(std::chrono::nanoseconds t) noexcept {
    mConfig.mMaxEpochTime = t;
    return *this;
}
std::chrono::nanoseconds Bench::maxEpochTime() const noexcept {
    return mConfig.mMaxEpochTime;
}

// Sets the maximum time each epoch should take. Default is 100ms.
Bench& Bench::minEpochTime(std::chrono::nanoseconds t) noexcept {
    mConfig.mMinEpochTime = t;
    return *this;
}
std::chrono::nanoseconds Bench::minEpochTime() const noexcept {
    return mConfig.mMinEpochTime;
}

Bench& Bench::minEpochIterations(uint64_t numIters) noexcept {
    mConfig.mMinEpochIterations = (numIters == 0) ? 1 : numIters;
    return *this;
}
uint64_t Bench::minEpochIterations() const noexcept {
    return mConfig.mMinEpochIterations;
}

Bench& Bench::epochIterations(uint64_t numIters) noexcept {
    mConfig.mEpochIterations = numIters;
    return *this;
}
uint64_t Bench::epochIterations() const noexcept {
    return mConfig.mEpochIterations;
}

Bench& Bench::warmup(uint64_t numWarmupIters) noexcept {
    mConfig.mWarmup = numWarmupIters;
    return *this;
}
uint64_t Bench::warmup() const noexcept {
    return mConfig.mWarmup;
}

Bench& Bench::config(Config const& benchmarkConfig) {
    mConfig = benchmarkConfig;
    return *this;
}
Config const& Bench::config() const noexcept {
    return mConfig;
}

Bench& Bench::output(std::ostream* outstream) noexcept {
    mConfig.mOut = outstream;
    return *this;
}

ANKERL_NANOBENCH(NODISCARD) std::ostream* Bench::output() const noexcept {
    return mConfig.mOut;
}

std::vector<Result> const& Bench::results() const noexcept {
    return mResults;
}

Bench& Bench::render(char const* templateContent, std::ostream& os) {
    ::ankerl::nanobench::render(templateContent, *this, os);
    return *this;
}

std::vector<BigO> Bench::complexityBigO() const {
    std::vector<BigO> bigOs;
    auto rangeMeasure = BigO::collectRangeMeasure(mResults);
    bigOs.emplace_back("O(1)", rangeMeasure, [](double) {
        return 1.0;
    });
    bigOs.emplace_back("O(n)", rangeMeasure, [](double n) {
        return n;
    });
    bigOs.emplace_back("O(log n)", rangeMeasure, [](double n) {
        return std::log2(n);
    });
    bigOs.emplace_back("O(n log n)", rangeMeasure, [](double n) {
        return n * std::log2(n);
    });
    bigOs.emplace_back("O(n^2)", rangeMeasure, [](double n) {
        return n * n;
    });
    bigOs.emplace_back("O(n^3)", rangeMeasure, [](double n) {
        return n * n * n;
    });
    std::sort(bigOs.begin(), bigOs.end());
    return bigOs;
}

Rng::Rng()
    : mX(0)
    , mY(0) {
    std::random_device rd;
    std::uniform_int_distribution<uint64_t> dist;
    do {
        mX = dist(rd);
        mY = dist(rd);
    } while (mX == 0 && mY == 0);
}

ANKERL_NANOBENCH_NO_SANITIZE("integer")
uint64_t splitMix64(uint64_t& state) noexcept {
    uint64_t z = (state += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27U)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31U);
}

// Seeded as described in romu paper (update april 2020)
Rng::Rng(uint64_t seed) noexcept
    : mX(splitMix64(seed))
    , mY(splitMix64(seed)) {
    for (size_t i = 0; i < 10; ++i) {
        operator()();
    }
}

// only internally used to copy the RNG.
Rng::Rng(uint64_t x, uint64_t y) noexcept
    : mX(x)
    , mY(y) {}

Rng Rng::copy() const noexcept {
    return Rng{mX, mY};
}

BigO::RangeMeasure BigO::collectRangeMeasure(std::vector<Result> const& results) {
    BigO::RangeMeasure rangeMeasure;
    for (auto const& result : results) {
        if (result.config().mComplexityN > 0.0) {
            rangeMeasure.emplace_back(result.config().mComplexityN, result.median(Result::Measure::elapsed));
        }
    }
    return rangeMeasure;
}

BigO::BigO(std::string const& bigOName, RangeMeasure const& rangeMeasure)
    : mName(bigOName) {

    // estimate the constant factor
    double sumRangeMeasure = 0.0;
    double sumRangeRange = 0.0;

    for (size_t i = 0; i < rangeMeasure.size(); ++i) {
        sumRangeMeasure += rangeMeasure[i].first * rangeMeasure[i].second;
        sumRangeRange += rangeMeasure[i].first * rangeMeasure[i].first;
    }
    mConstant = sumRangeMeasure / sumRangeRange;

    // calculate root mean square
    double err = 0.0;
    double sumMeasure = 0.0;
    for (size_t i = 0; i < rangeMeasure.size(); ++i) {
        auto diff = mConstant * rangeMeasure[i].first - rangeMeasure[i].second;
        err += diff * diff;

        sumMeasure += rangeMeasure[i].second;
    }

    auto n = static_cast<double>(rangeMeasure.size());
    auto mean = sumMeasure / n;
    mNormalizedRootMeanSquare = std::sqrt(err / n) / mean;
}

BigO::BigO(const char* bigOName, RangeMeasure const& rangeMeasure)
    : BigO(std::string(bigOName), rangeMeasure) {}

std::string const& BigO::name() const noexcept {
    return mName;
}

double BigO::constant() const noexcept {
    return mConstant;
}

double BigO::normalizedRootMeanSquare() const noexcept {
    return mNormalizedRootMeanSquare;
}

bool BigO::operator<(BigO const& other) const noexcept {
    return std::tie(mNormalizedRootMeanSquare, mName) < std::tie(other.mNormalizedRootMeanSquare, other.mName);
}

std::ostream& operator<<(std::ostream& os, BigO const& bigO) {
    return os << bigO.constant() << " * " << bigO.name() << ", rms=" << bigO.normalizedRootMeanSquare();
}

std::ostream& operator<<(std::ostream& os, std::vector<ankerl::nanobench::BigO> const& bigOs) {
    detail::fmt::StreamStateRestorer restorer(os);
    os << std::endl << "|   coefficient |   err% | complexity" << std::endl << "|--------------:|-------:|------------" << std::endl;
    for (auto const& bigO : bigOs) {
        os << "|" << std::setw(14) << std::setprecision(7) << std::scientific << bigO.constant() << " ";
        os << "|" << detail::fmt::Number(6, 1, bigO.normalizedRootMeanSquare() * 100.0) << "% ";
        os << "| " << bigO.name();
        os << std::endl;
    }
    return os;
}

} // namespace nanobench
} // namespace ankerl

#endif // ANKERL_NANOBENCH_IMPLEMENT
#endif // ANKERL_NANOBENCH_H_INCLUDED
