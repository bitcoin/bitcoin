Bitcoin Core
=============

Below are notes on installing Bitcoin Core software on Linux systems.

General Runtime Requirements
----------------------------

Bitcoin Core requires glibc (GNU C Library) 2.31 or newer.

GUI Runtime Requirements
------------------------

The GUI executable, `bitcoin-qt`, is based on the Qt 6 framework and uses the `xcb` QPA (Qt Platform Abstraction) platform plugin
to run on X11. Its runtime library dependencies are as follows:
- `libfontconfig`
- `libfreetype`

On Debian, Ubuntu, or their derivatives, you can run the following command to ensure all dependencies are installed:
```sh
sudo apt install libfontconfig1 libfreetype6
```

On Fedora, run:
```sh
sudo dnf install fontconfig freetype
```

For other systems, please consult their documentation.
