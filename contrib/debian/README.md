
Debian
====================
This directory contains files used to package iopd/iop-qt
for Debian-based Linux systems. If you compile iopd/iop-qt yourself, there are some useful files here.

## iop: URI support ##


iop-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install iop-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your iop-qt binary to `/usr/bin`
and the `../../share/pixmaps/iop128.png` to `/usr/share/pixmaps`

iop-qt.protocol (KDE)

