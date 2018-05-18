
Debian
====================
This directory contains files used to package aitherd/aither-qt
for Debian-based Linux systems. If you compile aitherd/aither-qt yourself, there are some useful files here.

## aither: URI support ##


aither-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install aither-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your aither-qt binary to `/usr/bin`
and the `../../share/pixmaps/aither128.png` to `/usr/share/pixmaps`

aither-qt.protocol (KDE)

