
Debian
====================
This directory contains files used to package bitcreditd/credits-qt
for Debian-based Linux systems. If you compile bitcreditd/credits-qt yourself, there are some useful files here.

## bitcredit: URI support ##


credits-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install credits-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your credits-qt binary to `/usr/bin`
and the `../../share/pixmaps/bitcoin128.png` to `/usr/share/pixmaps`

credits-qt.protocol (KDE)

