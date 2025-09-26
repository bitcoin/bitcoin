Bitcoin Core
=============

Below are notes on installing Bitcoin Core software on Linux systems.

General Runtime Requirements
----------------------------

Bitcoin Core requires glibc (GNU C Library) 2.31 or newer.

GUI Runtime Requirements
------------------------

The GUI executable, `bitcoin-qt`, is based on the Qt 6 framework and uses the `xcb` QPA (Qt Platform Abstraction) platform plugin
to run on X11. Its runtime library [dependencies](https://doc.qt.io/archives/qt-6.7/linux-requirements.html) are as follows:
- `libfontconfig`
- `libfreetype`
- `libxcb`
- `libxcb-cursor`
- `libxcb-icccm`
- `libxcb-image`
- `libxcb-keysyms`
- `libxcb-randr`
- `libxcb-render`
- `libxcb-render-util`
- `libxcb-shape`
- `libxcb-shm`
- `libxcb-sync`
- `libxcb-xfixes`
- `libxcb-xkb`
- `libxkbcommon`
- `libxkbcommon-x11`

On Debian, Ubuntu, or their derivatives, you can run the following command to ensure all dependencies are installed:
```sh
sudo apt install \
  libfontconfig1 \
  libfreetype6 \
  libxcb1 \
  libxcb-cursor0 \
  libxcb-icccm4 \
  libxcb-image0 \
  libxcb-keysyms1 \
  libxcb-randr0 \
  libxcb-render0 \
  libxcb-render-util0 \
  libxcb-shape0 \
  libxcb-shm0 \
  libxcb-sync1 \
  libxcb-xfixes0 \
  libxcb-xkb1 \
  libxkbcommon0 \
  libxkbcommon-x11-0
```

On Fedora, run:
```sh
sudo dnf install \
  fontconfig \
  freetype \
  libxcb \
  libxkbcommon \
  libxkbcommon-x11 \
  xcb-util-cursor \
  xcb-util-image \
  xcb-util-keysyms \
  xcb-util-renderutil \
  xcb-util-wm
```

For other systems, please consult their documentation.
