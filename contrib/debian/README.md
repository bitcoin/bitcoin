
Debian
====================
This directory contains files used to package dashd/dash-qt
for Debian-based Linux systems. If you compile dashd/dash-qt yourself, there are some useful files here.

## dash: URI support ##


dash-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install dash-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your dash-qt binary to `/usr/bin`
and the `../../share/pixmaps/dash128.png` to `/usr/share/pixmaps`

dash-qt.protocol (KDE)

