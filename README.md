Unnamed game for the retro win32 jam 2024
=========================================

Status: under development.

Given enough time and motivation, this might eventually become my entry for
the retro win32 game jam 2024: https://itch.io/jam/win32-jam-autumn-2024

Check back at the end of november to see if anything interesting came out of it.
If not I will just delete the repository at some point.

The idea is to make an isometric dungeon crawler for Pentium-class computers,
running Windows 95 or Windows NT 4.0. Ideally it will also run on GNU/Linux,
newer versions of Windows, and DOS, but some of those might have to wait until
after the jam deadline.

License
-------
Copyright (C) 2024 John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software. Feel free to use, modify and/or redistribute it
under the terms of the GNU General Public License v3, or at your option any
later version published by the Free Software Foundation.
See COPYING for details.

Build
-----
Grab the data files from subversion:
    svn co svn://nuclear.mutantstargoat.com/datadirs/winjam24 data

To build the retro win32 version using DirectX 2.0, either use the msvc6
project (`winjam24.dsw`), or run `nmake -f Makefile.msvc`.

To build the less optimized cross-platform SDL 1.2 version on UNIX, run
`make -f Makefile.sdl`.
