
Debian
====================
This directory contains files used to package globaltokend/globaltoken-qt
for Debian-based Linux systems. If you compile globaltokend/globaltoken-qt yourself, there are some useful files here.

## bitcoin: URI support ##


globaltoken-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install globaltoken-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your globaltoken-qt binary to `/usr/bin`
and the `../../share/pixmaps/bitcoin128.png` to `/usr/share/pixmaps`

globaltoken-qt.protocol (KDE)

