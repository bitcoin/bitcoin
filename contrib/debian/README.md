
Debian
====================
This directory contains files used to package darkcoind/darkcoin-qt
for Debian-based Linux systems. If you compile darkcoind/darkcoin-qt yourself, there are some useful files here.

## darkcoin: URI support ##


darkcoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install darkcoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your darkcoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/darkcoin128.png` to `/usr/share/pixmaps`

darkcoin-qt.protocol (KDE)

