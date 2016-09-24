
Debian
====================
This directory contains files used to package litecoind/litecoin-qt
for Debian-based Linux systems. If you compile litecoind/litecoin-qt yourself, there are some useful files here.

## litecoin: URI support ##


litecoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install litecoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your litecoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/litecoin128.png` to `/usr/share/pixmaps`

litecoin-qt.protocol (KDE)

