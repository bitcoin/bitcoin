
Debian
====================
This directory contains files used to package bitcreditd/bitcredit-qt
for Debian-based Linux systems. If you compile bitcreditd/bitcredit-qt yourself, there are some useful files here.

## bitcredit: URI support ##


bitcredit-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install bitcredit-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your bitcredit-qt binary to `/usr/bin`
and the `../../share/pixmaps/bitcoin128.png` to `/usr/share/pixmaps`

bitcredit-qt.protocol (KDE)

