
Debian
====================
This directory contains files used to package crowncoind/crowncoin-qt
for Debian-based Linux systems. If you compile crowncoind/crowncoin-qt yourself, there are some useful files here.

## crowncoin: URI support ##


crowncoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install crowncoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your crowncoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/crowncoin128.png` to `/usr/share/pixmaps`

crowncoin-qt.protocol (KDE)

