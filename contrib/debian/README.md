
Debian
====================
This directory contains files used to package chaincoind/chaincoin-qt
for Debian-based Linux systems. If you compile chaincoind/chaincoin-qt yourself, there are some useful files here.

## chaincoin: URI support ##


chaincoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install chaincoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your chaincoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/chaincoin128.png` to `/usr/share/pixmaps`

chaincoin-qt.protocol (KDE)

