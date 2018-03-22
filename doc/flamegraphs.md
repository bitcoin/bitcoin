## Flame Graphs

Flame graphs are a way of visualizing software CPU and heap profiles. Any tool
that can create stack data in the correct format can be used to generate a flame
graph. The flame graphs themselves are SVG files that have a small have a small
amount of embedded Javascript that provide mouseover events and some functions
for zooming and filtering. They can also be viewed in regular image viewers or
browsers that have Javascript enabled.

**TODO:** Try to get this hosted on bitcoincore.org, see [this
PR](https://github.com/bitcoin-core/bitcoincore.org/pull/522).

Example (**[interactive SVG](https://monad.io/bitcoin-flamegraph.svg)**):

![Example Flamegraph](https://monad.io/bitcoin-flamegraph.svg)

You can learn more about flame graphs
[on Brendan Gregg's site](http://www.brendangregg.com/flamegraphs.html).

### Interpreting Flame Graphs

Height on the flame graph represents different stack traces, and width
represents number of samples (i.e. percentage of total time). Tall sections are
for deep stacks, and wide sections represent parts of the programming take a lot
of time.

The flame graph is color coded according to certain heuristics:

 * **Red** frames are attributed to libc or system libraries
 * **Yellow** frames are attributed to third party C++ libraries
 * **Green** frames are attributed to Bitcoin source files
 * **Blue** frames are attributed to the Linux kernel

## Installing The Flame Graph Scripts

The upstream GitHub repo for flame graphs can be found at
[github.com/brendangregg/FlameGraph](https://github.com/brendangregg/FlameGraph).
To run the scripts you'll need Perl installed on your computer.

There is a small optional patch in the Bitcoin repository at
[contrib/devtools/flamegraph.patch](/contrib/devtools/flamegraph.patch). You can
apply this patch against the upstream FlameGraph `v1.0` tag to generate flame
graphs that are color coded for Bitcoin. You can apply the patch using a command
like:

```bash
# Assuming you're at the top directory in the FlameGraph repo.
$ patch -p1 <path/to/contrib/devtools/flamegraph.patch
```

## Generating Flame Graphs On Linux

The Linux kernel project maintains a command called `perf` that provides a high
level interface to the
[perf_event_open(2)](http://www.man7.org/linux/man-pages/man2/perf_event_open.2.html)
system call. Perf lets you generate accurate CPU profiles of kernel events and
userspace applications with relatively low overhead. This is the recommended way
to generate flame graphs on Linux, as it is lower overhead than other options
and allows generating profiles that can inspect kernel time.

For more information about `perf`:

 * [Perf Tutorial](https://perf.wiki.kernel.org/index.php/Tutorial)
 * [Perf Examples](http://www.brendangregg.com/perf.html)

### Setting Up Perf

Perf is available on most Linux distros in a package named `perf` or some
variant thereof:

```bash
# Install perf on Debian.
sudo apt-get install linux-perf

# Install perf on Fedora.
sudo dnf install perf
```

To generate flame graphs with labeled function names you'll need a build of
Bitcoin that has debugging symbols enabled (this should be the case by default).
On Linux you can check that your executable has debugging symbols by checking for a
`.debug_str` section in the ELF file:, e.g.

```bash
# If this produces output your binary has debugging symbols.
$ readelf -S src/bitcoind | grep .debug_str
```

### Generating A Profile

To profile a running `bitcoind` process for 60 seconds you can use the following
command:

```bash
# Profile bitcoind for 60 seconds (replace PID).
perf record -g --call-graph dwarf -F 101 -p PID -- sleep 60
```

You should replace `PID` with the process ID of `bitcoind` (on most Linux
systems you can get this with `pidof bitcoind`). The options `-g` and
`--call-graph dwarf` are required to get stack traces. The `-F` flag sets the
sampling frequency; changing this is not strictly necessary, but the default
sample rate is very high and may be unsuitable for running for long periods of
time. The `perf record` command has many other options, for more details see
[perf-record(1)](http://man7.org/linux/man-pages/man1/perf-record.1.html).

After running this command you'll have a `perf.data` in the current working
directory. Assuming that you have the perl scripts from the FlameGraph
repository in your `$PATH` you could generate a flame graph like so:

```bash
# Create a flame graph named "bitcoin.svg"
$ perf script | stackcollapse-perf.pl --all | flamegraph.pl --color bitcoin > bitcoin.svg
```

The flags given above for `stackcollapse-perf.pl` and `flamegraph.pl` assume
you've applied the patch mentioned earlier for generating Bitcoin-specific color
coded flame graphs. They are not necessary unless you want color coded output.

The `perf record` command samples all threads in the target process. Often this
is not what you want, and you may want instead to generate a profile of a single
thread. This can be achieved by using `grep` to filter the output of
`stackcollapse-perf.pl` before it passes to `flamegraph.pl`. For instance, to
generate a profile of just the Bitcoin `loadblk` thread you might use an
invocation like:

```bash
# Create a flame graph of just the loadblk thread.
$ {
  perf script |
  stackcollapse-perf.pl --all |
  grep loadblk |
  flamegraph.pl --color bitcoin
} > loadblk.svg
```

You can get creative with how you use grep (or any other tool) to exclude or
include various parts of the collapsed stack data. The data format expected by
`flamegraph.pl` is very simple and can be manipulated in other interesting ways
(e.g. to collapse adjacent stack frames by class or file). See the upstream
FlameGraph repository for more examples.


### Enabling Kernel Annotations

If you want kernel annotations (optional) then you should set the
`kernel.perf_event_paranoid` sysctl option is set to -1 before running `perf
record`. To set this option:

```bash
# Optional, enable kernel annotations.
sudo sysctl kernel.perf_event_paranoid=-1
```

You will also need kernel debug symbols:

```bash
# Install kernel debug symbols on Debian.
sudo apt-get install linux-image-`uname -r`-dbg

# Install kernel debug symbols on Fedora.
sudo dnf debuginfo-install kernel
```

## Generating Flame Graphs On macOS and Windows

The FlameGraph repository has scripts for processing the output of a large
number of other profiling tools.

**MacOS**: Follow the upstream instructions for generating flame graphs [using
DTrace](http://www.brendangregg.com/FlameGraphs/cpuflamegraphs.html#Instructions).
There's also a guide for generating flame graphs using [XCode
Instruments](https://schani.wordpress.com/2012/11/16/flame-graphs-for-instruments/).

**Windows**: The FlameGraph repository has a script named
`stackcollapse-vsprof.pl` that can process Visual Studio profiler output.
