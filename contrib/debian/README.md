
Debian
====================
This directory contains files used to package libertad/liberta-qt
for Debian-based Linux systems. If you compile libertad/liberta-qt yourself, there are some useful files here.

## liberta: URI support ##


liberta-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install liberta-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your liberta-qt binary to `/usr/bin`
and the `../../share/pixmaps/liberta128.png` to `/usr/share/pixmaps`

liberta-qt.protocol (KDE)

