# macOS Build Guide

**Updated for MacOS [11.2](https://www.apple.com/macos/big-sur/)**

This guide describes how to build bitcoind, command-line utilities, and GUI on macOS

## Preparation

The commands in this guide should be executed in a Terminal application.\\
macOS comes with a built-in Terminal located in:

```
/Applications/Utilities/Terminal.app
```

### 1. Xcode Command Line Tools

The Xcode Command Line Tools (CLT) are a collection of build tools for macOS.\\
These tools must be installed in order to build Bitcoin Core from source.

To install, run the following command from your terminal:

``` bash
xcode-select --install
```

Upon running the command, you should see a popup appear.\\
Click on `Install` to continue the installation process.

Verify CLT installation:

To view installed CLT version number, run this command : `pkgutil –pkg-info=com.apple.pkg.CLTools_Executables`\\
If output is not-showing verison number or if output is showing msg that “… No receipt …” then CLT is not installed, or its bundled inside/with Xcode.\\
To view pre-installed all pkgs you may run : `pkgutil –pkgs`

You may also run command : `xcrun clang`\\
and see what it outputs:

``` bash
UserMacBook:~ username$  xcrun clang
clang: error: no input files
```
  If output of above command is NOT this message ''clang: error: no input files'', then either installation has error or executable build files are not in PATH environment variable correctly.

Note: CLT installer download is under/near 300 MB, and may need around ~ 2 GB space in your storage.

### 2. Package Manager

There are few package manager (pkg-mngr) options for macOS:\\
2a: homebrew.\\
2b: MacPorts.

#### 2a. Homebrew Package Manager

Homebrew is an opensource & free //(3rd party)// package manager (pkg-mngr) for macOS, that allows one to install packages from the command line easily. Homebrew was built 7yrs after MacPorts pkg-mngr, initially MacPorts was known as DarwinPorts.
While several package managers are available for macOS, this guide will focus on Homebrew as it is the most popular.
Since the examples in this guide which walk through the installation of a package will use Homebrew, it is recommended that you install it to follow along.
Otherwise, you can adapt the commands to your package manager of choice.

To install the Homebrew package manager, see: https://brew.sh

Notice / WARNING / CAUTION : Though homebrew is an opensource pkg-mngr but this tool uses Google Analytics to collect usage telemetry data from your homebrew inside your computer.\\
If you want this anti-privacy bahavior stopped, use OPT OUT option/command:

``` bash
brew analytics off
```

or by setting:

``` bash
export HOMEBREW_NO_ANALYTICS=1
```

Note: If you run into issues while installing Homebrew or pulling packages, refer to [Homebrew's troubleshooting page](https://docs.brew.sh/Troubleshooting).

#### 2b. MacPorts Package Manager

[MacPorts](https://www.MacPorts.org/) is an opensource & free //(3rd party)// pkg-mngr //(package manager)// for macOS, etc, & it does not steal your usage/private data by-default. MacPorts [guide](https://guide.macports.org/). It can obtain source or binary or both //(for most)// package. After downloading source, it can auto compile in your OS/distro to create/build trustworthy binary files. MacPorts was known as DarwinPorts, and it was created 7yrs before homebrew.

Download/obtain MacPorts installer dmg/pkg file, install it. More info [here](https://www.macports.org/install.php).

If you need more information related to PATH variable, or other info on MacPorts pkg-mngr: goto MacPorts [guide](https://guide.macports.org/)

### 3. Install Required Dependencies

The first step is to download the required dependencies.\\
These dependencies represent the packages required to get a barebones installation up and running.

See [dependencies.md](dependencies.md) for a complete overview.

To install, run the following from your terminal, if you use homebrew:

``` bash
brew install automake libtool boost pkg-config libevent
```

To install, run the following from your terminal, if you use MacPorts:

``` bash
sudo port install automake libtool boost176 pkgconfig libevent
```

### 4. Clone Bitcoin repository

`git` should already be installed by default on your system.\\
Now that all the required dependencies are installed, let's clone the Bitcoin Core repository to a directory.\\
All build scripts and commands will run from this directory.

``` bash
git clone https://github.com/bitcoin/bitcoin.git
```

### 5. Install Optional Dependencies

#### Wallet Dependencies

It is not necessary to build wallet functionality to run `bitcoind` or  `bitcoin-qt`.

###### Descriptor Wallet Support

`sqlite` is required to support for descriptor wallets.

macOS ships with a useable `sqlite` package, meaning you don't need to
install anything.

###### Legacy Wallet Support

if you use homebrew, get `berkeley-db@4`.\\
if you use MacPorts, get `db48`.\\
It is only required to support for legacy wallets.\\
Skip if you don't intend to use legacy wallets.

``` bash
brew install berkeley-db@4
```

or

``` bash
sudo port install db48
```

---

#### GUI Dependencies

###### Qt

Bitcoin Core includes a GUI built with the cross-platform Qt Framework.\\
To compile the GUI, we need to install `qt@5` via homebrew, or we need to install `qt5` via MacPorts.\\
Skip if you don't intend to use the GUI.

``` bash
brew install qt@5
```

or

``` bash
sudo port install qt5
```

Note: Building with Qt binaries downloaded from the Qt website is not officially supported.\\
See the notes in [#7714](https://github.com/bitcoin/bitcoin/issues/7714).

###### qrencode

The GUI can encode addresses in a QR Code. To build in QR support for the GUI, install `qrencode`.\\
Skip if not using the GUI or don't want QR code functionality.

``` bash
brew install qrencode
```
or

``` bash
sudo port install qrencode
```

---

#### Port Mapping Dependencies

###### miniupnpc

miniupnpc may be used for UPnP port mapping.\\
Skip if you do not need this functionality.

``` bash
brew install miniupnpc
```

or

``` bash
sudo port install miniupnpc
```

Note: The `miniupnpc` also includes some parts of NAT-PMP.

###### libnatpmp

libnatpmp may be used for NAT-PMP port mapping.\\
Skip if you do not need this functionality.

``` bash
brew install libnatpmp
```

Note: UPnP and NAT-PMP support will be compiled in and disabled by default.\\
Check out the [further configuration](#further-configuration) section for more information.\\
(The `gupnp-igd` pkg, in MacPorts, has UPnP-IGD spec & support, which is concurrent to NAT-PMP, but not used by current bitcoin core source code. So unless bitcoin-core devs recommend to load it, do not load/use it now).

---

#### ZMQ Dependencies

Support for ZMQ notifications requires the following dependency.
Skip if you do not need ZMQ functionality.

``` bash
brew install zeromq
```

or

``` bash
sudo port install zmq
```

ZMQ is automatically compiled in and enabled if the dependency is detected.
Check out the [further configuration](#further-configuration) section for more information.

For more information on ZMQ, see: [zmq.md](zmq.md)

---

#### Test Suite Dependencies

There is an included test suite that is useful for testing code changes when developing.
To run the test suite (recommended), you will need to have Python 3 installed:

``` bash
brew install python
```

or

``` bash
suod port install python38
```

---

#### Deploy Dependencies

You can deploy a `.dmg` containing the Bitcoin Core application using `make deploy`.
This command depends on a couple of python packages, so it is required that you have `python` installed.

Ensuring that `python` is installed, you can install the deploy dependencies by running the following commands in your terminal:

``` bash
pip3 install ds_store mac_alias
```

## Building Bitcoin Core

### 1. Configuration

There are many ways to configure Bitcoin Core, here are a few common examples:

##### Wallet (BDB + SQlite) Support, No GUI:

If `berkeley-db@4` or `db48` is installed, then legacy wallet support will be built.
If `berkeley-db@4` or `db48` is not installed, then this will throw an error.
If `sqlite` is installed, then descriptor wallet support will also be built.
Additionally, this explicitly disables the GUI.

``` bash
./autogen.sh
./configure --with-gui=no
```

##### Wallet (only SQlite) and GUI Support:

This explicitly enables the GUI and disables legacy wallet support.
If `qt` is not installed, this will throw an error.
If `sqlite` is installed then descriptor wallet functionality will be built.
If `sqlite` is not installed, then wallet functionality will be disabled.

``` bash
./autogen.sh
./configure --without-bdb --with-gui=yes
```

##### No Wallet or GUI

``` bash
./autogen.sh
./configure --without-wallet --with-gui=no
```

##### Further Configuration

You may want to dig deeper into the configuration options to achieve your desired behavior.
Examine the output of the following command for a full list of configuration options:

``` bash
./configure -help
```

### 2. Compile

After configuration, you are ready to compile.
Run the following in your terminal to compile Bitcoin Core:

``` bash
make        # use "-j N" here for N parallel jobs
make check  # Run tests if Python 3 is available
```

### 3. Deploy (optional)

You can also create a  `.dmg` containing the `.app` bundle by running the following command:

``` bash
make deploy
```

## Running Bitcoin Core

Bitcoin Core should now be available at `./src/bitcoind`.
If you compiled support for the GUI, it should be available at `./src/qt/bitcoin-qt`.

The first time you run `bitcoind` or `bitcoin-qt`, it will start downloading the blockchain.
This process could take many hours, or even days on slower than average systems.

By default, blockchain and wallet data files will be stored in:

``` bash
/Users/${USER}/Library/Application Support/Bitcoin/
```

Before running, you may create an empty configuration file:

```shell
mkdir -p "/Users/${USER}/Library/Application Support/Bitcoin"

touch "/Users/${USER}/Library/Application Support/Bitcoin/bitcoin.conf"

chmod 600 "/Users/${USER}/Library/Application Support/Bitcoin/bitcoin.conf"
```

You can monitor the download process by looking at the debug.log file:

```shell
tail -f $HOME/Library/Application\ Support/Bitcoin/debug.log
```

## Other commands:

```shell
./src/bitcoind -daemon      # Starts the bitcoin daemon.
./src/bitcoin-cli --help    # Outputs a list of command-line options.
./src/bitcoin-cli help      # Outputs a list of RPC commands when the daemon is running.
./src/qt/bitcoin-qt -server # Starts the bitcoin-qt server mode, allows bitcoin-cli control
```
