
Debian
====================
This directory contains files used to package bitcoin-core-daemon/bitcoin-core-gui
for Debian-based Linux systems. If you compile bitcoin-core-daemon/bitcoin-core-gui yourself, there are some useful files here.

## bitcoin: URI support ##


bitcoin-core-gui.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install bitcoin-core-gui.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your bitcoin-core-gui binary to `/usr/bin`
and the `../../share/pixmaps/bitcoin128.png` to `/usr/share/pixmaps`

bitcoin-core-gui.protocol (KDE)

