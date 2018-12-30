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

# command-line parms
sharepath = str(ARGUMENTS.get('sharepath', DATA_DIR))
debug = int(ARGUMENTS.get('debug', 0))
profiler = int(ARGUMENTS.get('profiler', 0))
sdl_only = int(ARGUMENTS.get('sdl_only', 0))
asm = int(ARGUMENTS.get('asm', 0))
editor = int(ARGUMENTS.get('editor', 0))
sdlmixer = int(ARGUMENTS.get('sdlmixer', 0))
arm = int(ARGUMENTS.get('arm', 0))
ipv6 = int(ARGUMENTS.get('ipv6', 0))
micro = int(ARGUMENTS.get('micro', 0))
use_svn_as_micro = int(ARGUMENTS.get('svnmicro', 0))

if (micro > 0):
	D2XMICRO = micro
else:
	D2XMICRO = 0

if use_svn_as_micro:
	D2XMICRO = str(D2XSVN)

VERSION_STRING = ' v' + str(D2XMAJOR) + '.' + str(D2XMINOR)
if (D2XMICRO):
	VERSION_STRING += '.' + str(D2XMICRO)

print '\n===== ' + PROGRAM_NAME + VERSION_STRING + " (svn " + str(D2XSVN) + ') =====\n'

# general source files
common_sources = [
'2d/2dsline.c',
'2d/bitblt.c',
'2d/bitmap.c',
'2d/box.c',
'2d/canvas.c',
'2d/circle.c',
'2d/font.c',
'2d/gpixel.c',
'2d/line.c',
'2d/palette.c',
'2d/pcx.c',
'2d/pixel.c',
'2d/rect.c',
'2d/rle.c',
'2d/scalec.c',
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
'arch/sdl/rbaudio.c',
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
'main/ai2.c',
'main/aipath.c',
'main/automap.c',
'main/bm.c',
'main/cntrlcen.c',
'main/collide.c',
'main/config.c',
'main/console.c',
'main/controls.c',
'main/credits.c',
'main/crypt.c',
'main/digiobj.c',
'main/dumpmine.c',
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
'main/net_ipx.c',
'main/net_udp.c',
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
'mem/mem.c',
'misc/args.c',
'misc/cfile.c',
'misc/dl_list.c',
'misc/error.c',
'misc/hash.c',
'misc/strio.c',
'misc/strutil.c',
]

# for editor
editor_sources = [
'main/bmread.c',
'main/editor/centers.c',
'main/editor/curves.c',
'main/editor/autosave.c',
'main/editor/eglobal.c',
'main/editor/ehostage.c',
'main/editor/elight.c',
'main/editor/eobject.c',
'main/editor/eswitch.c',
'main/editor/fixseg.c',
'main/editor/func.c',
'main/editor/group.c',
'main/editor/info.c',
'main/editor/kbuild.c',
'main/editor/kcurve.c',
'main/editor/kfuncs.c',
'main/editor/kgame.c',
'main/editor/khelp.c',
'main/editor/kmine.c',
'main/editor/ksegmove.c',
'main/editor/ksegsel.c',
'main/editor/ksegsize.c',
'main/editor/ktmap.c',
'main/editor/kview.c',
'main/editor/med.c',
'main/editor/meddraw.c',
'main/editor/medmisc.c',
'main/editor/medrobot.c',
'main/editor/medsel.c',
'main/editor/medwall.c',
'main/editor/mine.c',
'main/editor/objpage.c',
'main/editor/segment.c',
'main/editor/seguvs.c',
'main/editor/texpage.c',
'main/editor/texture.c',
'ui/button.c',
'ui/checkbox.c',
'ui/file.c',
'ui/gadget.c',
'ui/icon.c',
'ui/inputbox.c',
'ui/keypad.c',
'ui/keypress.c',
'ui/keytrap.c',
'ui/listbox.c',
'ui/menu.c',
'ui/menubar.c',
'ui/message.c',
'ui/mouse.c',
'ui/popup.c',
'ui/radio.c',
'ui/scroll.c',
'ui/ui.c',
'ui/uidraw.c',
'ui/userbox.c',
'ui/window.c'
]

# for linux
arch_linux_sources = [
'arch/linux/ipx.c',
'arch/linux/ipx_kali.c',
'arch/linux/ukali.c'
]

# SDL_mixer sound implementation
arch_sdlmixer = [
'misc/hmp2mid.c',
'arch/sdl/digi_mixer.c',
'arch/sdl/digi_mixer_music.c',
'arch/sdl/jukebox.c'
]

if (sdlmixer == 1):
        common_sources += arch_sdlmixer

# for windows
arch_win32_sources = [
'arch/win32/hmpfile.c',
'arch/win32/ipx.c',
]

# for Mac OS X
arch_macosx_sources = [
'arch/cocoa/SDLMain.m'
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

# assembler related
asm_sources = [
'maths/fix.asm',
'maths/rand.c',
'maths/vecmat.c',
'maths/vecmata.asm',
]

noasm_sources = [
'maths/fixc.c',
'maths/rand.c',
'maths/tables.c',
'maths/vecmat.c'
]

# Acquire environment object...
env = Environment(ENV = os.environ)

# flags and stuff for all platforms
env.ParseConfig('pkg-config sdl2 --libs --cflags')
env.Append(CPPFLAGS = ['-Wall', '-funsigned-char'])
env.Append(CPPDEFINES = [('PROGRAM_NAME', '\\"' + str(PROGRAM_NAME) + '\\"'), ('D2XMAJOR', '\\"' + str(D2XMAJOR) + '\\"'), ('D2XMINOR', '\\"' + str(D2XMINOR) + '\\"')])
#env.Append(CPPDEFINES = [('VERSION', '\\"' + str(VERSION) + '\\"')])
#env.Append(CPPDEFINES = [('USE_SDLMIXER', sdlmixer)])
env.Append(CPPDEFINES = ['NETWORK', 'HAVE_NETIPX_IPX_H', '_REENTRANT'])
env.Append(CPPPATH = ['include', 'main', 'arch/include'])
# scons is too poop to correctly get it from the pkg-config call?
generic_libs = ['SDL2']
sdlmixerlib = ['SDL2_mixer']

if sdlmixer:
	env.Append(CPPDEFINES = ['USE_SDLMIXER'])

if (D2XMICRO):
	env.Append(CPPDEFINES = [('D2XMICRO', '\\"' + str(D2XMICRO) + '\\"')])

env['CCFLAGS'] += ['-Wno-misleading-indentation',
                   '-Wno-unused-but-set-variable',
                   '-Werror-implicit-function-declaration']

# Get traditional compiler environment variables
if os.environ.has_key('CC'):
	env['CC'] = os.environ['CC']
if os.environ.has_key('CFLAGS'):
	env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
if os.environ.has_key('LDFLAGS'):
	env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

env['CCFLAGS'] += ["-Wno-deprecated-declarations"]

# windows or *nix?
if sys.platform == 'win32':
	print "compiling on Windows"
	osdef = '_WIN32'
	osasmdef = 'win32'
	sharepath = ''
	env.Append(CPPDEFINES = ['_WIN32', 'HAVE_STRUCT_TIMEVAL', 'NATIVE_IPX'])
	env.Append(CPPPATH = ['arch/win32/include'])
	ogldefines = ['OGL']
	common_sources += arch_win32_sources
	ogllibs = ''
	winlibs = ['wsock32', 'winmm', 'mingw32', 'SDLmain']
	libs = winlibs + generic_libs
	lflags = '-mwindows'
elif sys.platform == 'darwin':
	print "compiling on Mac OS X"
	osdef = '__APPLE__'
	sharepath = ''
	env.Append(CPPDEFINES = ['HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL', '__unix__'])
	asm = 0
	env.Append(CPPPATH = ['arch/linux/include'])
	ogldefines = ['OGL']
	common_sources += arch_macosx_sources
	ogllibs = ''
	libs = ''
	# Ugly way of linking to frameworks, but kreator has seen uglier
	lflags = '-framework Cocoa -framework SDL'
	if (sdl_only == 0):
		lflags += ' -framework OpenGL'
	if (sdlmixer == 1):
		print "including SDL_mixer"
		lflags += ' -framework SDL_mixer'
	sys.path += ['./arch/cocoa']
	VERSION = str(D2XMAJOR) + '.' + str(D2XMINOR)
	if (D2XMICRO):
		VERSION += '.' + str(D2XMICRO)
	env['VERSION_NUM'] = VERSION
	env['VERSION_NAME'] = PROGRAM_NAME + ' v' + VERSION
	import tool_bundle
else:
	print "compiling on *NIX"
	osdef = '__LINUX__'
	osasmdef = 'elf'
	sharepath += '/'
	env.Append(CPPDEFINES = ['__LINUX__', 'NATIVE_IPX', 'KALINIX', 'HAVE_STRUCT_TIMESPEC', 'HAVE_STRUCT_TIMEVAL'])
	env.Append(CPPPATH = ['arch/linux/include'])
	ogldefines = ['OGL']
	common_sources += arch_linux_sources
	ogllibs = ['GL', 'm']
	libs = generic_libs
	lflags = '-L/usr/X11R6/lib'

# arm architecture?
if (arm == 1):
	asm = 0
	env.Append(CPPDEFINES = ['WORDS_NEED_ALIGNMENT'])
	env.Append(CPPFLAGS = ['-mstructure-size-boundary=8'])

# sdl or opengl?
if (sdl_only == 1):
	print "building with SDL"
	target = 'd2x-rebirth-sdl'
	common_sources += arch_sdl_sources
else:
	print "building with OpenGL"
	target = 'd2x-rebirth-gl'
	env.Append(CPPDEFINES = ogldefines)
	common_sources += arch_ogl_sources
	libs += ogllibs

# SDL_mixer for sound? (*NIX only)
if (sdlmixer == 1) and (sys.platform != 'darwin'):
	print "including SDL_mixer"
	libs += sdlmixerlib

# debug?
if (debug == 1):
	print "including: DEBUG"
	env.Append(CPPFLAGS = ['-g', '-fstack-protector-all'])
else:
	env.Append(CPPDEFINES = ['NDEBUG', 'RELEASE'])
	env.Append(CPPFLAGS = ['-O2', '-g', '-ggdb3'])

# profiler?
if (profiler == 1):
	env.Append(CPPFLAGS = ['-pg'])
	lflags += ' -pg'

# assembler code?
if (asm == 1) and (sdl_only == 1):
	print "including: ASSEMBLER"
	Object(['texmap/tmappent.S', 'texmap/tmapppro.S'], AS='gcc', ASFLAGS='-D' + str(osdef) + ' -c ')
	env.Replace(AS = 'nasm')
	env.Append(ASCOM = ' -f ' + str(osasmdef) + ' -d' + str(osdef) + ' -Itexmap/ ')
	common_sources += asm_sources + ['texmap/tmappent.o', 'texmap/tmapppro.o']
else:
	env.Append(CPPDEFINES = ['NO_ASM'])
	common_sources += noasm_sources

#editor build?
if (editor == 1):
	env.Append(CPPDEFINES = ['EDITOR'])
	env.Append(CPPPATH = ['include/editor'])
	common_sources += editor_sources

# IPv6 compability?
if (ipv6 == 1):
	env.Append(CPPDEFINES = ['IPv6'])

print '\n'

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
	'sdl_only=1'      don't include OpenGL, use SDL-only instead
	'sdlmixer=1'      use SDL_Mixer for sound (includes external music support)
	'asm=1'           use ASSEMBLER code (only with sdl_only=1, requires NASM and x86)
	'debug=1'         build DEBUG binary which includes asserts, debugging output, cheats and more output
	'profiler=1'      do profiler build
	'editor=1'        build editor !EXPERIMENTAL!
	'arm=1'           compile for ARM architecture
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
