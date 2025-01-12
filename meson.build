project('d2x-rebirth', 'c', version: '0.55.1')

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
    'arch/sdl/digi.c',
    'arch/sdl/digi_audio.c',
	'arch/sdl/window.c',
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
	'maths/fixc.c',
    'maths/rand.c',
    'maths/vecmat.c',
    'misc/args.c',
	'misc/cfile.c',
    'misc/error.c',
    'misc/hash.c',
    'misc/strio.c',
    'misc/strutil.c',
]

arch_linux_sources = [
]

arch_sdlmixer = [
    'misc/hmp2mid.c',
    'arch/sdl/digi_mixer.c',
    'arch/sdl/digi_mixer_music.c',
]

arch_ogl_sources = [
    'arch/ogl/gr.c',
    'arch/ogl/ogl.c',
]

arch_sdl_sources = [
    'arch/sdl/gr.c',
    'texmap/tmapflat.c'
]

sdl2 = dependency('sdl2', required: true)
sdl2_mixer = dependency('SDL2_mixer', required: true)
gl = dependency('GL', required: true)
libm = meson.get_compiler('c').find_library('m', required: false)

version = meson.project_version().split('.')
args = [
    '-DPROGRAM_NAME="@0@"'.format(meson.project_name()),
    '-DD2XMAJOR="@0@"'.format(version[0]),
    '-DD2XMINOR="@0@"'.format(version[1]),
    '-DD2XMICRO="@0@"'.format(version[2]),
    '-DNETWORK',
    '-D_REENTRANT',

    '-D__LINUX__',
    '-DHAVE_STRUCT_TIMESPEC',
    '-DHAVE_STRUCT_TIMEVAL',
    '-DOGL',

    '-DIPv6',
	'-DUSE_UDP',
	'-DUSE_TRACKER',

    '-DSHAREPATH="@0@"'.format(join_paths(get_option('prefix'), 'share', 'games', meson.project_name()))
]

executable(meson.project_name(),
           common_sources, arch_linux_sources, arch_ogl_sources,
           c_args: args,
           include_directories: include_directories('include', 'main', 'arch/include'),
           dependencies: [sdl2, libm, gl])
