### Usage

To build dependencies for the current arch+OS:

    make

To build for another arch/OS:

    make HOST=host-platform-triplet

For example:

    make HOST=x86_64-w64-mingw32 -j4

A prefix will be generated that's suitable for plugging into Dash's
configure. In the above example, a dir named x86_64-w64-mingw32 will be
created. To use it for Dash:

    ./configure --prefix=`pwd`/depends/x86_64-w64-mingw32

Common `host-platform-triplets` for cross compilation are:

- `x86_64-w64-mingw32` for Win64
- `x86_64-apple-darwin19` for macOS
- `arm-linux-gnueabihf` for Linux ARM 32 bit
- `aarch64-linux-gnu` for Linux ARM 64 bit
- `riscv32-linux-gnu` for Linux RISC-V 32 bit
- `riscv64-linux-gnu` for Linux RISC-V 64 bit

No other options are needed, the paths are automatically configured.

### Install the required dependencies: Ubuntu & Debian

#### For macOS cross compilation

    sudo apt-get install curl librsvg2-bin libtiff-tools bsdmainutils imagemagick libcap-dev libz-dev libbz2-dev python3-setuptools libtinfo5

Note: You must obtain the macOS SDK before proceeding with a cross-compile.
Under the depends directory, create a subdirectory named `SDKs`.
Then, place the extracted SDK under this new directory.
For more information, see [SDK Extraction](../contrib/macdeploy/README.md#sdk-extraction).

#### For Win64 cross compilation

- see [build-windows.md](../doc/build-windows.md#cross-compilation-for-ubuntu-and-windows-subsystem-for-linux)

#### For linux (including i386, ARM) cross compilation

Common linux dependencies:

    sudo apt-get install make automake cmake curl g++-multilib libtool binutils-gold bsdmainutils pkg-config python3 patch

For linux ARM cross compilation:

    sudo apt-get install g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

For linux AARCH64 cross compilation:

    sudo apt-get install g++-aarch64-linux-gnu binutils-aarch64-linux-gnu

For linux RISC-V 64-bit cross compilation (there are no packages for 32-bit):

    sudo apt-get install g++-riscv64-linux-gnu binutils-riscv64-linux-gnu

RISC-V known issue: gcc-7.3.0 and gcc-7.3.1 result in a broken `test_dash` executable (see https://github.com/bitcoin/bitcoin/pull/13543),
this is apparently fixed in gcc-8.1.0.

### Dependency Options
The following can be set when running make: `make FOO=bar`

<dl>
<dt>SOURCES_PATH</dt>
<dd>downloaded sources will be placed here</dd>
<dt>BASE_CACHE</dt>
<dd>built packages will be placed here</dd>
<dt>SDK_PATH</dt>
<dd>Path where sdk's can be found (used by macOS)</dd>
<dt>FALLBACK_DOWNLOAD_PATH</dt>
<dd>If a source file can't be fetched, try here before giving up</dd>
<dt>NO_QT</dt>
<dd>Don't download/build/cache qt and its dependencies</dd>
<dt>NO_WALLET</dt>
<dd>Don't download/build/cache libs needed to enable the wallet</dd>
<dt>NO_UPNP</dt>
<dd>Don't download/build/cache packages needed for enabling upnp</dd>
<dt>DEBUG</dt>
<dd>disable some optimizations and enable more runtime checking</dd>
<dt>HOST_ID_SALT</dt>
<dd>Optional salt to use when generating host package ids</dd>
<dt>BUILD_ID_SALT</dt>
<dd>Optional salt to use when generating build package ids</dd>
<dt>FORCE_USE_SYSTEM_CLANG</dt>
<dd>(EXPERTS ONLY) When cross-compiling for macOS, use Clang found in the
system's <code>$PATH</code> rather than the default prebuilt release of Clang
from llvm.org. Clang 8 or later is required.</dd>
</dl>
If some packages are not built, for example `make NO_WALLET=1`, the appropriate
options will be passed to Dash Core's configure. In this case, `--disable-wallet`.

### Additional targets

    download: run 'make download' to fetch all sources without building them
    download-osx: run 'make download-osx' to fetch all sources needed for macOS builds
    download-win: run 'make download-win' to fetch all sources needed for win builds
    download-linux: run 'make download-linux' to fetch all sources needed for linux builds

### Other documentation

- [description.md](description.md): General description of the depends system
- [packages.md](packages.md): Steps for adding packages

