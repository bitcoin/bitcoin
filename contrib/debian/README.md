
Debian
====================
This directory contains files used to package btchdd/btchd-qt
for Debian-based Linux systems. If you compile btchdd/btchd-qt yourself, there are some useful files here.

## btchd: URI support ##


btchd-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install btchd-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your btchd-qt binary to `/usr/bin`
and the `../../share/pixmaps/btchd128.png` to `/usr/share/pixmaps`

btchd-qt.protocol (KDE)

