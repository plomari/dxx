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


This was also converted to SDL2 (OpenGL/Linux only), possibly with some problems
that still should be fixed.

If anyone uses this and finds a problem, feel free to open an issue on this
github repo.

Original
--------


                         __________
__________/ D2X-Rebirth /


http://www.dxx-rebirth.com


0. Introduction

D2X-Rebirth is based on a late D2X-CVS Source, coded and released by Bradley Bell and his team.

I spend much time to improve the Sourcecode, tried to fix bugs in there and added some improvements. It is the goal of DXX-Rebirth to keep these games alive and the result is a very stable version of the Descent ][ port - called D2X-Rebrith.

I hope you enjoy the game as you did when you played it the first time.

If you have something to say about my release, feel free to contact me at zicodxx [at] yahoo [dot] de

- zico 20070407


1. Features

D2X-Rebirth has every little feature you already may know from the DOS Version of Descent ][ and much more.

For example:

    *
      High resolutions with full Cockpit support
    *
      Widescreen options
    *
      Joystick and Mouse support
    *
      Possibility to run AddOn levels
    *
      Network support
    *
      Record and play demos
    *
      OpenGL functions and Eyecandy like Trilinear filtering, Transparency effects etc.
    *
      MP3/OGG/AIF/WAV Jukebox Support
    *
      everything else you know from DESCENT ][
    *
      ... and much more!


2. Installation

See INSTALL.txt.


3. Multiplayer

DXX-Rebirth supports Multiplayer over (obsoleted) IPX and UDP/IP.
Using UDP/IP works over LAN and Internet. Since the Networking code of the Descent Engine is Peer-to-Peer, it is necessary for
all players (Host and Clients) to open port UDP 31017.
Clients can put an offset to this port by using '-ip_baseport OFFSET'.
Hosts can also use option '-ip_relay' to route players with closed ports. Use this with caution. It will increase Lag and Ping drastically.
Also game summary will not refresh correctly for relay-players until Host has escaped the level as well.
UDP/IP also supports IPv6 by compiling the game with the designated flag. Please note IPv4- and IPv6-builds cannot play together.


4. Legal stuff

See COPYING.txt


5. Contact

http://www.dxx-rebirth.com/
zicodxx [at] yahoo [dot] de
