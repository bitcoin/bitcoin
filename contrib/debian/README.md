
Debian
====================
This directory contains files used to package crownd/crown-qt
for Debian-based Linux systems. If you compile crownd/crown-qt yourself, there are some useful files here.

## crown: URI support ##


crown-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install crown-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your crown-qt binary to `/usr/bin`
and the `../../share/pixmaps/crown128.png` to `/usr/share/pixmaps`

crown-qt.protocol (KDE)

