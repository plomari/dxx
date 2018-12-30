About this fork
---------------

This is based on dxx 0.55.1 (from 2009, i.e. very old), mostly because of #4 on
the upstream repo. Numerous later bug fixes and changes applied to dxx have been
merged into it as well.

It includes some fixes and modifications as well. Some are game play changes
that are enabled by default:

- a raised object limit. This fixes behavior even with some official levels
  (at least level 12), where dropping a lot of weapons can bring the object
  count above the limit and you can't shoot anymore.
- extended ammo rack mode (off by default): raise the ammo rack weapons limit
  to basically infinite; you also drop all ammo when you die, and you keep the
  ammo rack even if you die. You never lose ammo, unless you get killed during
  reactor countdown or in a later unreachable area.
- make rear view and transfer-energy-to-shield key "stickiness" configurable
- a "killboss" cheat that doesn't trigger cheat penalty, because I hate boss
  fights
- logging remaining robot life as HUD messages (actually this is very annoying,
  but I left it in because it's sometimes useful)
- fix display of current bomb type (for b/n keys) in a certain cockpit mode
- show the total number of levels in the automap
- some shitty cheats (with penalty; see source code)
- some bug fixes
- remains pure C. I'm against the pointless C++ uglification that upstream did.
  This also doesn't include the d1x unification.
- half-functional support for loading D2X-XL levels. This is work in progress
  and in an early stage. (See below.)

This was also converted to SDL2 (OpenGL/Linux only), possibly with some problems
that still should be fixed.

If anyone uses this and finds a problem, feel free to open an issue on this
github repo.

Paths
-----

Currently, only Linux (and hopefully generic Unix) is supported. (Non-cygwin
Windows support can be added on request.) Consequently, all the following paths
use Linux conventions.

The data dir is by default in /usr/local/share/games/d2x-rebirth/. This can be
adjusted at build time with the "sharepath" option, or at runtime with the
-hogdir option. This directory must contain descent2.hog at its top level.

The user config path is in ~/.d2x-rebirth/. (This is the same as D2X/DXX. This
can lead to conflict. In particular, savegames made with this project won't load
in D2X/DXX. The reverse is unknown.)

Note: the user config path may be changed in the future in order to not
conflict with upstream DXX.

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

D2X-XL level support
--------------------

There is somewhat functional support for loading _some_ D2X-XL levels. Some
levels work fully, while others are unplayable. It depends on the feature set
the level designer relied on when creating the level.

Support for this is at an early stage. Levels neither render correctly in all
places, nor are all game play extensions supported. Random crashes are also
possible.

It's not a goal to recreate D2X-XL's rendering. Smoke effects etc. will likely
remain unimplemented. On the other hand, missing features related to game play
will be implemented on request as far as it is possible.

Multiplayer
-----------

The focus of this project is on single player. The maintainer does not play
multiplayer games, and as a natural consequence, multiplayer mode may suffer
from various problems.

Note that due to the fragility of the network protocol, various builds are
generally incompatible to each other, even though they are derived from the
same Descent 2 codebase.

Savegames
---------

Savegames written by this fork cannot be read by other projects. The format was
changed to support larger object counts and for D2X-XL support. It should still
be possible to load older d2x/dxx savegames.

Editor
------

The original level editor was removed from this codebase. The editor was part
of the original source release, and most likely the software used for building
the official levels. While that sounds interesting, it was mostly dysfunctional
and buggy, and above all very old and hard to use.

DXX upstream has put a lot of effort into restoring its functionality, but it's
relatively unusable and crashes often. If you want to try the editor, try to use
DXX's.

It's recommended to use a separate level editor instead. DLE from
http://www.descent2.de/ seems to work, even if it's MS Windows only.

Demo playback
-------------

Demo playback is functional, but might be either removed or replaced by modern
video capturing. There is not much of a point in maintaining such a complex and
imperfect method of recording game footage, that requires the game itself for
playback on top of it.

(Does anyone actually care about demo playback?)
