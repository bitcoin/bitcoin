
Debian
====================
This directory contains files used to package ravend/raven-qt
for Debian-based Linux systems. If you compile ravend/raven-qt yourself, there are some useful files here.

## raven: URI support ##


raven-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install raven-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your raven-qt binary to `/usr/bin`
and the `../../share/pixmaps/raven128.png` to `/usr/share/pixmaps`

raven-qt.protocol (KDE)

