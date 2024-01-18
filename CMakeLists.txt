
cmake_minimum_required(VERSION 3.14)
project(snakespasm)

option(DO_USERDIRS "Enable user directories support" OFF)
option(USE_SDL2 "Enable SDL 2.x" ON)
option(USE_CODEC_WAVE "Enable WAVE codec" ON)
option(USE_CODEC_FLAC "Enable FLAC codec" OFF)
option(USE_CODEC_MP3 "Enable MP3 codec" OFF)
option(USE_CODEC_VORBIS "Enable Vorbis codec" OFF)
option(USE_CODEC_OPUS "Enable Opus codec" OFF)
option(USE_CODEC_MIKMOD "Enable MikMod codec" OFF)
option(USE_CODEC_XMP "Enable XMP codec" OFF)
option(USE_CODEC_MODPLUG "Enable ModPlug codec" OFF)
option(USE_CODEC_UMX "Enable UMX codec" OFF)

set(MP3LIB "mad" CACHE STRING "MP3 decoding library: mad or mpg123")
set(VORBISLIB "vorbis" CACHE STRING "Vorbis decoding library: vorbis or tremor")

add_executable(${PROJECT_NAME} WIN32
    src/strlcat.c
    src/strlcpy.c
    src/gl_refrag.c
    src/gl_rlight.c
    src/gl_rmain.c
    src/gl_fog.c
    src/gl_rmisc.c
    src/r_part.c
    src/r_world.c
    src/gl_screen.c
    src/gl_sky.c
    src/gl_warp.c
    src/gl_vidsdl.c
    src/gl_draw.c
    src/image.c
    src/gl_texmgr.c
    src/gl_mesh.c
    src/r_sprite.c
    src/r_alias.c
    src/r_brush.c
    src/gl_model.c
    src/in_sdl.c
    src/snd_dma.c
    src/snd_mix.c
    src/snd_mem.c
    src/bgmusic.c
	src/snd_codec.c
	src/snd_flac.c
	src/snd_wave.c
	src/snd_vorbis.c
	src/snd_opus.c
	src/snd_mp3tag.c
	src/snd_mikmod.c
	src/snd_modplug.c
	src/snd_xmp.c
	src/snd_umx.c
    src/snd_sdl.c
    src/cd_sdl.c
    src/net_dgrm.c
    src/net_loop.c
    src/net_main.c
    src/chase.c
    src/cl_demo.c
    src/cl_input.c
    src/cl_main.c
    src/cl_parse.c
    src/cl_tent.c
    src/console.c
    src/keys.c
    src/menu.c
    src/sbar.c
    src/view.c
    src/wad.c
    src/cmd.c
    src/common.c
    src/miniz.c
    src/crc.c
    src/cvar.c
    src/cfgfile.c
    src/host.c
    src/host_cmd.c
    src/mathlib.c
    src/pr_cmds.c
    src/pr_edict.c
    src/pr_exec.c
    src/sv_main.c
    src/sv_move.c
    src/sv_phys.c
    src/sv_user.c
    src/world.c
    src/zone.c
    src/main_sdl.c
    src/pyquake.c
    src/pyq_builtins.c)

if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE
        src/net_win.c
        src/net_wins.c
        src/net_wipx.c
        src/pl_win.c
        src/sys_sdl_win.c
        win32/QuakeSpasm.rc)

    target_compile_definitions(${PROJECT_NAME} PRIVATE _USE_WINSOCK2)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm ws2_32)
else()
    target_sources(${PROJECT_NAME} PRIVATE
        src/net_bsd.c
        src/net_udp.c
        src/pl_linux.c
        src/sys_sdl_unix.c)
endif()

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

find_library(LIBM m)
if(LIBM)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBM})
endif()

find_package(Python3 REQUIRED COMPONENTS Development)
target_link_libraries(${PROJECT_NAME} PRIVATE Python3::Python)

if(DO_USERDIRS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DO_USERDIRS)
endif()

if(USE_SDL2)
    find_package(SDL2 REQUIRED CONFIG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_SDL2)

    if(TARGET SDL2::SDL2)
        if(TARGET SDL2::SDL2main)
            target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2main)
        endif()
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
    else()
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
    endif()
else()
    find_package(SDL REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL_LIBRARIES})
endif()

if(USE_CODEC_WAVE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CODEC_WAVE)
endif()

if(USE_CODEC_FLAC)
    # TODO: find package
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CODEC_FLAG)
    target_link_libraries(${PROJECT_NAME} PRIVATE FLAC)
endif()

if(USE_CODEC_OPUS)
    # TODO: find package
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CODEC_OPUS)
    target_link_libraries(${PROJECT_NAME} PRIVATE opusfile opus ogg)
endif()

if(USE_CODEC_VORBIS)
    find_package(Ogg REQUIRED CONFIG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CODEC_VORBIS)
    target_link_libraries(${PROJECT_NAME} PRIVATE Ogg::ogg)
    if(VORBISLIB STREQUAL "vorbis")
        find_package(Vorbis REQUIRED CONFIG)
        target_link_libraries(${PROJECT_NAME} PRIVATE Vorbis::vorbis Vorbis::vorbisfile)
    elseif(VORBISLIB STREQUAL "tremor")
        # TODO: find package
        target_compile_definitions(${PROJECT_NAME} PRIVATE VORBIS_USE_TREMOR)
        target_link_libraries(${PROJECT_NAME} PRIVATE vorbisidec)
    else()
        message(FATAL_ERROR "Invalid VORBISLIB setting")
    endif()
endif()

if(USE_CODEC_MP3)
    # TODO: find package
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CODEC_MP3)
    if(MP3LIB STREQUAL "mpg123")
        target_sources(${PROJECT_NAME} PRIVATE snd_mpg123.c)
        target_link_libraries(${PROJECT_NAME} PRIVATE mpg123)
    elseif(MP3LIB STREQUAL "mad")
        target_sources(${PROJECT_NAME} PRIVATE snd_mp3.c)
        target_link_libraries(${PROJECT_NAME} PRIVATE mad)
    else()
        message(FATAL_ERROR "Invalid MP3LIB setting")
    endif()
endif()

if(USE_CODEC_MIKMOD)
    # TODO: find package
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CODEC_MIKMOD)
    target_link_libraries(${PROJECT_NAME} PRIVATE mikmod)
endif()

if(USE_CODEC_XMP)
    # TODO: find package
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CODEC_XMP)
    target_link_libraries(${PROJECT_NAME} PRIVATE xmp)
endif()

if(USE_CODEC_MODPLUG)
    # TODO: find package
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CODEC_MODPLUG)
    target_link_libraries(${PROJECT_NAME} PRIVATE modplug)
endif()

if(USE_CODEC_UMX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CODEC_UMX)
endif()

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

