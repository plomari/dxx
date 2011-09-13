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
