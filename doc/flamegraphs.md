## Flame Graphs

Flame graphs are a way of visualizing software CPU and heap profiles. Any tool
that can create stack data in the correct format can be used to generate a flame
graph. You can learn more about flame graphs
[here](http://www.brendangregg.com/flamegraphs.html).

Example flame graph (click to zoom):

![Example Flamegraph](/contrib/flamegraphs/example.svg)

### Installing The Flame Graph Scripts

The upstream GitHub repo for flame graphs is at
[brendangregg/FlameGraph](https://github.com/brendangregg/FlameGraph). To run
the scripts you'll need a Perl installation on your system.

There is a small patch in the Bitcoin repository at
[contrib/flamegraphs/bitcoin-annotations.patch](/contrib/flamegraphs/bitcoin-annotations.patch)
that you can apply to generate flame graphs that are color coded for Bitcoin.
You can apply it using a command like this:

```bash
# Assuming you're at the top directory in the FlameGraph repo
$ patch -p1 <path/to/bitcoin-annotations.patch
```

### Installing Perf

In general you will need a version of the
[perf](https://perf.wiki.kernel.org/index.php/Tutorial) command that matches the
kernel that you are using. Perf is maintained along with the kernel source tree,
and is available on most Linux distributions:

```bash
# Install perf on Debian.
sudo apt-get install linux-perf

# Install perf on Fedora.
sudo dnf install perf
```

### Generating Flame Graphs On Linux Using Perf

To generate flame graphs with labeled function names you'll need a build of
Bitcoin that has debugging symbols enabled (this should be the case by default).
You can check that your executable has debugging symbols by checking for a
`.debug_str` section in the ELF file:, e.g.

```bash
# If this produces output your binary has debugging symbols.
$ readelf -S src/bitcoind | grep .debug_str
```

If you want kernel annotations (optional) then you should also make sure that
the `kernel.perf_event_paranoid` sysctl option is set to -1:

```bash
# Optional, enable kernel annotations
sudo sysctl kernel.perf_event_paranoid=-1
```

Now you're ready to generate a profile. To profile a running bitcoind process
for 60 seconds you can use the following command:

```bash
# Profile bitcoind for 60 seconds
perf record -g --call-graph dwarf -F 101 -p $(pidof bitcoind) -- sleep 60
```

The required options are `-g` and `--call-graph dwarf` to get stack traces, and
`-p PID` to profile a given PID. The `-F` flag sets the sampling frequency;
changing this is not strictly necessary, but the default sample rate is very
high and may be unsuitable for running for long periods of time.

After running this command you'll have a `perf.data` in the current working
directory. Assuming that you have the perl scripts from the FlameGraph
repository in your `$PATH` you could generate a flame graph like so:

```bash
# Create a flame graph named "bitcoin.svg"
$ perf script | stackcollapse-perf.pl --all | flamegraph.pl --color bitcoin > bitcoin.svg
```

The `perf record` command will sample all threads in the process. If you want to
generate a profile of a single thread, you can filter the output of
`stackcollapse-perf.pl`. For instance, to generate a profile of just the Bitcoin
`loadblk` thread you might use an invocation like:

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
include various parts of the collapsed stack data.

## Generating Flame Graphs On Other Systems

The upstream FlameGraph repo has scripts for processing the output of a large
number of other profiling tools, such as DTrace (BSD, macOS) and the Visual
Studio profiler (Windows). Refer to the FlameGraph docs for more information
about using those tools.
