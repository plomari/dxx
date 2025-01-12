#SConstruct

# needed imports
import sys
import os
import SCons.Util

PROGRAM_NAME = 'D2X-Rebirth'

# version number
D2XMAJOR = 0
D2XMINOR = 55
D2XSVN   = os.popen('svnversion .').read()[:-1]
D2XSVN   = D2XSVN.split(':')[-1]

# installation path
PREFIX = str(ARGUMENTS.get('prefix', '/usr/local'))
BIN_SUBDIR = '/bin'
DATA_SUBDIR = '/share/games/d2x-rebirth'
BIN_DIR = PREFIX + BIN_SUBDIR
DATA_DIR = PREFIX + DATA_SUBDIR

# command-line params
sharepath = str(ARGUMENTS.get('sharepath', DATA_DIR))
release = int(ARGUMENTS.get('release', 0))
sdlmixer = int(ARGUMENTS.get('sdlmixer', 0))
ipv6 = int(ARGUMENTS.get('ipv6', 0))
micro = int(ARGUMENTS.get('micro', 0))
use_svn_as_micro = int(ARGUMENTS.get('svnmicro', 0))
use_udp = int(ARGUMENTS.get('use_udp', 1))

if (sys.platform != 'linux2') and (sys.platform != 'win32'):
	use_ipx = 0

if (micro > 0):
	D2XMICRO = micro
else:
	D2XMICRO = 0

if use_svn_as_micro:
	D2XMICRO = str(D2XSVN)

VERSION_STRING = ' v' + str(D2XMAJOR) + '.' + str(D2XMINOR)
if (D2XMICRO):
	VERSION_STRING += '.' + str(D2XMICRO)

print('\n===== ' + PROGRAM_NAME + VERSION_STRING + " (svn " + str(D2XSVN) + ') =====\n')

# general source files
common_sources = [
'2d/bitmap.c',
'2d/canvas.c',
'2d/font.c',
'2d/line.c',
'2d/palette.c',
'2d/pcx.c',
'2d/rect.c',
'2d/rle.c',
'3d/draw.c',
'3d/globvars.c',
'3d/instance.c',
'3d/interp.c',
'3d/matrix.c',
'3d/points.c',
'3d/rod.c',
'3d/setup.c',
'arch/sdl/event.c',
'arch/sdl/init.c',
'arch/sdl/joy.c',
'arch/sdl/key.c',
'arch/sdl/mouse.c',
'arch/sdl/timer.c',
'arch/sdl/window.c',
'arch/sdl/digi.c',
'arch/sdl/digi_audio.c',
'iff/iff.c',
'libmve/decoder8.c',
'libmve/decoder16.c',
'libmve/mve_audio.c',
'libmve/mvelib.c',
'libmve/mveplay.c',
'main/ai.c',
'main/aipath.c',
'main/automap.c',
'main/bm.c',
'main/cntrlcen.c',
'main/collide.c',
'main/config.c',
'main/controls.c',
'main/credits.c',
'main/crypt.c',
'main/digiobj.c',
'main/effects.c',
'main/endlevel.c',
'main/escort.c',
'main/fireball.c',
'main/fuelcen.c',
'main/fvi.c',
'main/game.c',
'main/gamecntl.c',
'main/gamefont.c',
'main/gamemine.c',
'main/gamepal.c',
'main/gamerend.c',
'main/gamesave.c',
'main/gameseg.c',
'main/gameseq.c',
'main/gauges.c',
'main/hostage.c',
'main/hud.c',
'main/inferno.c',
'main/kconfig.c',
'main/kmatrix.c',
'main/laser.c',
'main/lighting.c',
'main/menu.c',
'main/mglobal.c',
'main/mission.c',
'main/morph.c',
'main/movie.c',
'main/multi.c',
'main/multibot.c',
'main/newdemo.c',
'main/newmenu.c',
'main/object.c',
'main/paging.c',
'main/physics.c',
'main/piggy.c',
'main/player.c',
'main/playsave.c',
'main/polyobj.c',
'main/powerup.c',
'main/render.c',
'main/robot.c',
'main/scores.c',
'main/segment.c',
'main/slew.c',
'main/songs.c',
'main/state.c',
'main/switch.c',
'main/terrain.c',
'main/texmerge.c',
'main/text.c',
'main/titles.c',
'main/vclip.c',
'main/wall.c',
'main/weapon.c',
'maths/fixc.c',
'maths/rand.c',
'maths/vecmat.c',
'misc/args.c',
'misc/cfile.c',
'misc/error.c',
'misc/hash.c',
'misc/hmp.c',
'misc/strio.c',
'misc/strutil.c',
]

# SDL_mixer sound implementation
arch_sdlmixer = [
'arch/sdl/digi_mixer.c',
'arch/sdl/digi_mixer_music.c',
]       

# for opengl
arch_ogl_sources = [
'arch/ogl/gr.c',
'arch/ogl/ogl.c',
]

# for sdl
arch_sdl_sources = [
'arch/sdl/gr.c',
'texmap/tmapflat.c'
]

# Acquire environment object...
env = Environment(ENV = os.environ)

# flags and stuff for all platforms
env.ParseConfig('pkg-config sdl2 --libs --cflags')
env.Append(CPPFLAGS = ['-Wall', '-funsigned-char'])
env.Append(CPPDEFINES = [('PROGRAM_NAME', '\\"' + str(PROGRAM_NAME) + '\\"'), ('D2XMAJOR', '\\"' + str(D2XMAJOR) + '\\"'), ('D2XMINOR', '\\"' + str(D2XMINOR) + '\\"')])
#env.Append(CPPDEFINES = [('VERSION', '\\"' + str(VERSION) + '\\"')])
#env.Append(CPPDEFINES = [('USE_SDLMIXER', sdlmixer)])
env.Append(CPPDEFINES = ['NETWORK', '_REENTRANT'])
env.Append(CPPPATH = ['include', 'main', 'arch/include'])
# scons is too poop to correctly get it from the pkg-config call?
generic_libs = ['SDL2']
sdlmixerlib = ['SDL2_mixer']

if (D2XMICRO):
	env.Append(CPPDEFINES = [('D2XMICRO', '\\"' + str(D2XMICRO) + '\\"')])

env['CCFLAGS'] += ['-Wno-misleading-indentation',
                   '-Wno-unused-but-set-variable',
                   '-Werror-implicit-function-declaration']

# Get traditional compiler environment variables
if 'CC' in os.environ:
	env['CC'] = os.environ['CC']
if 'CFLAGS' in os.environ:
	env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
if 'LDFLAGS' in os.environ:
	env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

env['CCFLAGS'] += ["-Wno-deprecated-declarations"]

# windows or *nix?
if sys.platform == 'win32':
	print("compiling on Windows")
	osdef = '_WIN32'
	sharepath = ''
	env.Append(CPPDEFINES = ['_WIN32', 'HAVE_STRUCT_TIMEVAL'])
	env.Append(CPPPATH = ['arch/win32/include'])
	ogldefines = ['OGL']
	common_sources += ['arch/win32/messagebox.c']
	ogllibs = ''
	winlibs = ['wsock32', 'winmm', 'mingw32', 'SDLmain']
	libs = winlibs + generic_libs
	lflags = '-mwindows'
elif sys.platform == 'darwin':
	print("compiling on Mac OS X")
	osdef = '__APPLE__'
	sharepath = ''
	env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL', '__unix__'])
	env.Append(CPPPATH = ['arch/linux/include'])
	ogldefines = ['OGL']
	common_sources += 'arch/cocoa/SDLMain.m'
	ogllibs = ''
	libs = ''
	# Ugly way of linking to frameworks, but kreator has seen uglier
	lflags = '-framework Cocoa -framework SDL'
	lflags += ' -framework OpenGL'
	if (sdlmixer == 1):
		print("including SDL_mixer")
		lflags += ' -framework SDL_mixer'
	sys.path += ['./arch/cocoa']
	VERSION = str(D2XMAJOR) + '.' + str(D2XMINOR)
	if (D2XMICRO):
		VERSION += '.' + str(D2XMICRO)
	env['VERSION_NUM'] = VERSION
	env['VERSION_NAME'] = PROGRAM_NAME + ' v' + VERSION
	import tool_bundle
else:
	print("compiling on *NIX")
	osdef = '__LINUX__'
	sharepath += '/'
	env.Append(CPPDEFINES = ['__LINUX__', 'HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL'])
	env.Append(CPPPATH = ['arch/linux/include'])
	ogldefines = ['OGL']
	ogllibs = ['GL', 'm']
	libs = generic_libs
	lflags = '-L/usr/X11R6/lib'

print("building with OpenGL")
target = 'd2x-rebirth-gl'
env.Append(CPPDEFINES = ogldefines)
common_sources += arch_ogl_sources
libs += ogllibs

# SDL_mixer support?
if (sdlmixer == 1):
	print("including SDL_mixer")
	env.Append(CPPDEFINES = ['USE_SDLMIXER'])
	common_sources += arch_sdlmixer
	if (sys.platform != 'darwin'):
		libs += sdlmixerlib

env.Append(CPPFLAGS = ['-g', '-ggdb3'])

# debug?
if release == 0:
	print("Not building in release mode. Including sanitizers (very slow)!")
	print("Add release=1 to scons command line to build in release mode.")
	env.Append(CPPFLAGS = ['-O0', '-fsanitize=address,undefined'])
	lflags += ' -fsanitize=address,undefined'
else:
	env.Append(CPPFLAGS = ['-O3'])

# IPv6 compability?
if (ipv6 == 1):
	env.Append(CPPDEFINES = ['IPv6'])

# UDP support?
if (use_udp == 1):
	env.Append(CPPDEFINES = ['USE_UDP'])
	common_sources += ['main/net_udp.c']
	
	# Tracker support?  (Relies on UDP)
	env.Append( CPPDEFINES = [ 'USE_TRACKER' ] )

print('\n')

env.Append(CPPDEFINES = [('SHAREPATH', '\\"' + str(sharepath) + '\\"')])
# finally building program...
env.Program(target=str(target), source = common_sources, LIBS = libs, LINKFLAGS = str(lflags))
if (sys.platform != 'darwin'):
	env.Install(BIN_DIR, str(target))
	env.Alias('install', BIN_DIR)
else:
	tool_bundle.TOOL_BUNDLE(env)
	env.MakeBundle(PROGRAM_NAME + '.app', target,
                       'free.d2x-rebirth', 'd2xgl-Info.plist',
                       typecode='APPL', creator='DCT2',
                       icon_file='arch/cocoa/d2x-rebirth.icns',
                       subst_dict={'d2xgl' : target},	# This is required; manually update version for Xcode compatibility
                       resources=[['English.lproj/InfoPlist.strings', 'English.lproj/InfoPlist.strings']])

# show some help when running scons -h
Help(PROGRAM_NAME + ', SConstruct file help:' +
	"""

	Type 'scons' to build the binary.
	Type 'scons install' to build (if it hasn't been done) and install.
	Type 'scons -c' to clean up.
	
	Extra options (add them to command line, like 'scons extraoption=value'):
	
	'sharepath=DIR'   (non-Mac OS *NIX only) use DIR for shared game data. (default: /usr/local/share/games/d2x-rebirth)
	'sdlmixer=1'      use SDL_Mixer for sound (includes external music support)
	'release=1'       build properly optimized binary (without sanitizer)
	'ipv6=1'          enables IPv6 copability
	
	Default values:
	""" + ' sharepath = ' + DATA_DIR + """

	Some influential environment variables:
	  CC          C compiler command
	  CFLAGS      C compiler flags
	  LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a
                      nonstandard directory <lib dir>
                      <include dir>
        """)
#EOF
