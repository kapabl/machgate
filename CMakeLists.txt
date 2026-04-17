cmake_minimum_required(VERSION 3.16)
project(machismo C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

option(BUILD_WRAPGEN "Build the wrapgen utility" ON)
option(BUILD_TESTS "Build test fixtures" ON)

# --- machismo loader executable ---

set(MACHISMO_SOURCES
    src/machismo.c
    src/commpage.c
    src/resolver.c
    src/trampoline.c
    src/gdb_jit.c
    src/patcher.c
    src/config.c
    src/bgfx_shim.c
    src/eh_frame.c
    src/dylib_loader.c
    src/isa_emul.c
    src/lua_entity_opt.c
    src/elfcalls/elfcalls.c
    src/elfcalls/threads.c
)

add_executable(machismo ${MACHISMO_SOURCES})

target_include_directories(machismo PRIVATE src)
target_compile_options(machismo PRIVATE -Wall -O2 -g)
target_compile_definitions(machismo PRIVATE
    INSTALL_PREFIX=""
    SYSTEM_ROOT=""
    _GNU_SOURCE
)
target_link_libraries(machismo PRIVATE dl rt pthread m)
target_link_options(machismo PRIVATE -rdynamic)

# --- libsystem_shim.so ---

add_library(system_shim SHARED
    src/shim/libsystem_shim.c
)
set_target_properties(system_shim PROPERTIES
    OUTPUT_NAME "system_shim"
    PREFIX "lib"
)
target_compile_options(system_shim PRIVATE -Wall -O2 -g -fno-omit-frame-pointer)
target_link_libraries(system_shim PRIVATE m pthread dl)
target_link_options(system_shim PRIVATE
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/shim/libsystem_shim.ver
)

# --- libgalaxy_shim.so (GOG Galaxy SDK stub for DLC ownership) ---

add_library(galaxy_shim SHARED
    src/shim/libgalaxy_shim.c
)
set_target_properties(galaxy_shim PROPERTIES
    OUTPUT_NAME "galaxy_shim"
    PREFIX "lib"
)
target_compile_options(galaxy_shim PRIVATE -Wall -O2 -g)
target_link_options(galaxy_shim PRIVATE
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/shim/libgalaxy_shim.ver
)

# --- wrapgen utility ---

if(BUILD_WRAPGEN)
    add_executable(wrapgen tools/wrapgen.cpp)
    target_compile_options(wrapgen PRIVATE -Wall -O2 -g)
    target_link_libraries(wrapgen PRIVATE dl)
endif()

# --- test_lse_emul ---

if(BUILD_TESTS)
    add_executable(test_lse_emul tests/test_lse_emul.c src/isa_emul.c)
    target_include_directories(test_lse_emul PRIVATE src)
    target_compile_options(test_lse_emul PRIVATE -Wall -O2 -g)
    target_compile_definitions(test_lse_emul PRIVATE _GNU_SOURCE)
    target_link_libraries(test_lse_emul PRIVATE pthread)

    add_executable(test_necrolevel_rng tests/test_necrolevel_rng.c src/dylib_loader.c)
    target_include_directories(test_necrolevel_rng PRIVATE src)
    target_compile_options(test_necrolevel_rng PRIVATE -Wall -O2 -g)
    target_compile_definitions(test_necrolevel_rng PRIVATE _GNU_SOURCE)
    target_link_libraries(test_necrolevel_rng PRIVATE dl m)
endif()

# --- libgame_patches.so (NecroDancer-specific function replacements) ---

add_library(necrodancer_patches SHARED
    examples/necrodancer/src/game_patches.c
)
target_include_directories(necrodancer_patches PRIVATE
    src                          # for stubs/apple_abi.h
    examples/necrodancer/src     # for stubs/datastream.h
)
target_compile_options(necrodancer_patches PRIVATE -Wall -O2 -g)
target_link_libraries(necrodancer_patches PRIVATE dl)
set_target_properties(necrodancer_patches PROPERTIES
    OUTPUT_NAME "game_patches"
    PREFIX "lib"
)

# --- libsugar_patches_sdl2.so (SDL2 Sugar builds, e.g. Wratch's Den) ---
# _mp3_fromfile PCM cache.

add_library(sugar_patches_sdl2 SHARED
    examples/sugar/src/sugar_audio_sdl2.c
    examples/sugar/src/audio_cache.c
    examples/sugar/src/ogg_cache.c
)
target_include_directories(sugar_patches_sdl2 PRIVATE
    src                      # for stubs/apple_abi.h
    examples/sugar/src       # for audio_cache.h, ogg_cache.h
)
target_compile_options(sugar_patches_sdl2 PRIVATE -Wall -O2 -g)
target_link_libraries(sugar_patches_sdl2 PRIVATE dl pthread)
set_target_properties(sugar_patches_sdl2 PROPERTIES
    OUTPUT_NAME "sugar_patches_sdl2"
    PREFIX "lib"
)

# --- libsugar_patches_sdl3.so (SDL3 Sugar builds, e.g. Shotgun King) ---
# _load_ogg PCM cache + NEON palette→RGBA _flip (needs SDL_RenderTexture).

add_library(sugar_patches_sdl3 SHARED
    examples/sugar/src/sugar_audio_sdl3.c
    examples/sugar/src/sugar_flip.c
    examples/sugar/src/audio_cache.c
    examples/sugar/src/ogg_cache.c
)
target_include_directories(sugar_patches_sdl3 PRIVATE
    src
    examples/sugar/src
)
target_compile_options(sugar_patches_sdl3 PRIVATE -Wall -O2 -g)
target_link_libraries(sugar_patches_sdl3 PRIVATE dl pthread)
set_target_properties(sugar_patches_sdl3 PROPERTIES
    OUTPUT_NAME "sugar_patches_sdl3"
    PREFIX "lib"
)

# --- pcm_cache_write (Sugar PCM cache writer for install-time warmup) ---
add_executable(pcm_cache_write examples/sugar/tools/pcm_cache_write.c)
target_compile_options(pcm_cache_write PRIVATE -Wall -O2)

# --- install ---

install(TARGETS machismo RUNTIME DESTINATION bin)
install(TARGETS system_shim LIBRARY DESTINATION lib)
if(BUILD_WRAPGEN)
    install(TARGETS wrapgen RUNTIME DESTINATION bin)
endif()
