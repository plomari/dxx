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
- fix display of current bomb type (for b/n keys) in a certain cockpit mode
- show the total number of levels in the automap
- autoselects the last pilot on startup
- options for skipping game intro movie and delay, level movies, and level
  briefings
- no hardcoded timeout when loading level to simulate i486 experience (like
  DXX does)
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

The user config path is in ~/.d2x-c/. (Currently, you can copy D2X/DXX config
files from ~/.d2x-rebirth/ to here to migrate your config. Don't try to symlink
them to the same directory - this can lead to conflicts. In particular,
savegames made with this project won't load in D2X/DXX. The reverse should
usually work.)

Building
--------

Running "scons release=1" should be sufficient. The binary is created as
d2x-rebirth-gl in the source directory.

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

Development stuff
-----------------

Assert policy:

The original code uses Assert() and Int3(). Both don't do anything in release
mode, which is the way most/all builds for user consumption have been made. In
debug mode, these crashed the program.

This fork removed the difference between release and debug mode. The
preprocessor directives and build arguments to control them simply don't exist
anymore. I consider this a good idea. You don't want development and production
versions to be too different. They should use the same code (rather than be
subtly different due to ifdef messes).

Unfortunately, Assert() and Int3() trigger really often, even during normal
gameplay in official D2 levels. I assume often enough they're intended to drop
the developers into a debugger, so that whatever caused it could be debugged.
Maybe Int3() could be resumed or even be ignored. After over 2 decades  of
software technology progress, this is not convenient anymore, you'd just crash
all the time for dumb reasons.

The code does not use Assert() in a "modern" way like it is recommended today.
An assert should strictly catch internal logic errors (impossible conditions),
not things that depend on runtime failures or file contents.

This is why Assert() and Int3() print a message and do nothing else in this
fork. Which is dumb, but such is life. Of course nobody writing new code would
seriously consider this a good idea (except maybe GTK devs), but it's a
compromise to avoid a separate debug mode. Blindly removing all Assert()s is not
a good idea either.

The way to go is to slowly replace Assert() by assert() in code where it was
determined that it can happen on internal logic errors only. Int3() is to be
replaced by assert(0) or abort().

(assert() does nothing if NDEBUG is set, but the build system never sets NDEBUG.
The entire purpose of setting NDEBUG at all is for people, who try to prove that
NDEBUG is useful, and fail to achieve this.)

One problem is that the game does little verification of game data loaded from
disk. Savegames are basically memory dumps. Considering this, Assert() may never
go away. However, where possibly, it should be replaced by WARN_ON(), which
prints the failed condition, and returns the condition itself. The intention is
that the caller of WARN_ON() checks the result, and then handles the failure
gracefully in some way.
