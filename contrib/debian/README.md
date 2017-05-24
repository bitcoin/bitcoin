
Debian
====================
This directory contains files used to package flowd/flow-qt
for Debian-based Linux systems. If you compile flowd/flow-qt yourself, there are some useful files here.

## flow: URI support ##


flow-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install flow-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your flow-qt binary to `/usr/bin`
and the `../../share/pixmaps/flow128.png` to `/usr/share/pixmaps`

flow-qt.protocol (KDE)

