
Debian
====================
This directory contains files used to package syscoind/syscoin-qt
for Debian-based Linux systems. If you compile syscoind/syscoin-qt yourself, there are some useful files here.

## syscoin: URI support ##


syscoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install syscoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your syscoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/syscoin128.png` to `/usr/share/pixmaps`

syscoin-qt.protocol (KDE)

