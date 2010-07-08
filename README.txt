About this fork
---------------

This is based on dxx 0.55.1 (from 2009, i.e. very old), mostly because of #4 on
the upstream repo.

It includes some fixes and modifications as well. Some are game play changes
that are enabled by default:

- a raised object limit. This fixes behavior even with some official levels
  (at least level 12), where dropping a lot of weapons can bring the object
  count above the limit and you can't shoot anymore.
- raise the ammo rack weapons limit to basically infinite; you also drop all
  ammo when you die
- keep the ammo rack when you die
- raise vulcan ammo to basically infinite
- a "killboss" cheat that doesn't trigger cheat penalty, because I hate boss
  fights
- logging remaining robot life as HUD messages (actually this is very annoying,
  but I left it in because it's sometimes useful)
- fix display of current bomb type (for b/n keys) in a certain cockpit mode
- show the total number of levels in the automap
- some shitty cheats (with penatly; see source code)
- some bug fixes
- remains pure C. I'm against the pointless C++ uglification that upstream did.
  This also doesn't include the d1x unification.
- half-functional support for loading D2X-XL levels. This is work in progress
  and in an early stage. Levels neither render correctly in all places, nor are
  all game play extensions supported. Random crashes are also possible.

This was also converted to SDL2 (OpenGL/Linux only), possibly with some problems
that still should be fixed.

If anyone uses this and finds a problem, feel free to open an issue on this
github repo.

Paths
-----

Currently, only Linux is supported. (Although Windows support can be added on
request.) Consequently, all the following paths use Linux conventions.

The data dir is by default in /usr/local/share/games/d2x-rebirth/. This can be
adjusted at build time with the "sharepath" option, or at runtime with the
-hogdir option. This directory must contain descent2.hog at its top level.

The user config path is in ~/.d2x-rebirth/. (This is the same as D2X/DXX. This
can lead to conflict. In particular, savegames made with this project won't load
in D2X/DXX. The reverse is unknown.)

Building
--------

Running "scons" should be sufficient. The binary is created as d2x-rebirth-gl
in the source directory.

You can also try the experimental Meson build system:

	meson builddir
	cd builddir
	ninja

The binary file is at builddir/d2x-rebirth.

Once Meson works in a way I like, this will be the preferred build system, and
the scons build files will be deleted.

Original
--------


                         __________
__________/ D2X-Rebirth /


http://www.dxx-rebirth.com


0. Introduction:
================

   D2X-Rebirth is based on a late D2X-CVS Source, coded and released by Bradley Bell and his team.
   I spend much time to improve the Sourcecode, tried to fix bugs in there and added some improvements.
   It is the goal of DXX-Rebirth to keep these games alive and the result is a very stable version of
   the Descent ][ port - called D2X-Rebirth.
   I hope you enjoy the game as you did when you played it the first time.
   If you have something to say about my release, feel free to contact me at zicodxx [at] yahoo [dot] de


1. Features:
============

   D2X-Rebirth has every little feature you already may know from the DOS Version 1.2 of Descent 2 and much more.

   For example:
   * High resoution Fonts and briefing screens
   * High resolutions with full Cockpit support
   * Support for ALL resolutions - including Widescreen formats
   * Joystick and Mouse support
   * Possibility to run AddOn levels
   * LAN- and Online-gaming support
   * Record and play demos
   * OpenGL functions and Eyecandy like Trilinear filtering, Transparency effects etc.
   * MP3/OGG/AIF/WAV Jukebox Support
   * everything else you know from DESCENT ][
   * ... and much more!


2. Installation:
================

   See INSTALL.txt.


3. Multiplayer:
===============

   D2X-Rebirth supports Multiplayer over (obsoleted) IPX and UDP/IP.
   Please note that UDP/IP generally supports more features of D2X-Rebirth and uses Packet Loss Prevention while
   IPX is mainly meant to play together with non-D2X-Rebirth games.
   Using UDP/IP works over LAN and Internet. By default, each game communicates over UDP-Port 42424. This can be
   changed via the menus while creating a game and manually join a game, command-line argument or D2X.INI. To
   successfully host a game online, make sure UDP-Port 42424 (or otherwise if specified correctly) is opened on
   your Router/Firewall. Clients do not need to open any ports.
   The game also supports IPv6 if built in while compiling and should be backwards compatible to IPv4 builds
   as good as possible.


4. Legal stuff:
===============

   See COPYING.txt


5. Contact:
===========

   http://www.dxx-rebirth.com/
   zicodxx [at] yahoo [dot] de
