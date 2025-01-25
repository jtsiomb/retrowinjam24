Isometric retro dungeon crawler prototype
=========================================

Status: under development.

Given enough time and motivation, this might eventually become an isometric
dungeon crawler game for pentium-class computers, running DOS, Windows 95,
Windows NT 4.0, or GNU/Linux. Ideally it will also work on various UNIX
workstations too.

License
-------
Copyright (C) 2025 John Tsiombikas <nuclear@mutantstargoat.com>

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
