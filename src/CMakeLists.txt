cmake_minimum_required(VERSION 3.14)

if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "VITASDK nao definido")
endif()

set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Vita toolchain")
project(pizzatowersage_runner C CXX)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra -mfloat-abi=softfp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=softfp")

set(BUTTERSCOTCH_DIR "/SDKVita/PizzaTowerVita/src/butterscotch" CACHE PATH "Butterscotch source dir")
set(VITAGL_DIR "/SDKVita/PizzaTowerVita/third_party/vitaGL-nosplash" CACHE PATH "vitaGL dir")

file(GLOB BS_COMMON_SOURCES ${BUTTERSCOTCH_DIR}/src/*.c)
file(GLOB BS_GL_SOURCES
  ${BUTTERSCOTCH_DIR}/src/gl_common/*.c
  ${BUTTERSCOTCH_DIR}/src/gl/*.c
  ${BUTTERSCOTCH_DIR}/src/image/*.c
)

add_executable(${PROJECT_NAME}
  source/playable_main.c
  source/vita_settings.c
  source/vita_borders.c
  source/vita_frame_pacer.c
  source/vita_gl_quad.c
  source/vita_modern_pause.c
  ${BS_COMMON_SOURCES}
  ${BS_GL_SOURCES}
  ${BUTTERSCOTCH_DIR}/src/audio/openal/al_audio_system.c
  ${BUTTERSCOTCH_DIR}/src/audio/openal/wave.c
  ${BUTTERSCOTCH_DIR}/src/audio/fmod_wrapper.c
  ${BUTTERSCOTCH_DIR}/src/android/stb_impl.c
  ${BUTTERSCOTCH_DIR}/vendor/md5/md5.c
  ${BUTTERSCOTCH_DIR}/vendor/sha1/sha1.c
  ${BUTTERSCOTCH_DIR}/vendor/base64/base64.c
  ${BUTTERSCOTCH_DIR}/vendor/bzip2/bz_internal_error.c
  ${BUTTERSCOTCH_DIR}/vendor/bzip2/bzlib.c
  ${BUTTERSCOTCH_DIR}/vendor/bzip2/crctable.c
  ${BUTTERSCOTCH_DIR}/vendor/bzip2/decompress.c
  ${BUTTERSCOTCH_DIR}/vendor/bzip2/huffman.c
  ${BUTTERSCOTCH_DIR}/vendor/bzip2/randtable.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PRIVATE
  source
  ${BUTTERSCOTCH_DIR}/src
  ${BUTTERSCOTCH_DIR}/vendor/stb/ds
  ${BUTTERSCOTCH_DIR}/vendor/stb/image
  ${BUTTERSCOTCH_DIR}/vendor/stb/image_write
  ${BUTTERSCOTCH_DIR}/src/image
  ${BUTTERSCOTCH_DIR}/src/gl_common
  ${BUTTERSCOTCH_DIR}/src/gl
  ${BUTTERSCOTCH_DIR}/src/gl_legacy
  ${BUTTERSCOTCH_DIR}/src/audio/openal
  ${BUTTERSCOTCH_DIR}/vendor/bzip2
  ${BUTTERSCOTCH_DIR}/vendor/md5
  ${BUTTERSCOTCH_DIR}/vendor/sha1
  ${BUTTERSCOTCH_DIR}/vendor/base64
  ${BUTTERSCOTCH_DIR}/vendor/stb/vorbis
  $ENV{VITASDK}/share/gcc-arm-vita-eabi/samples/common
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  ENABLE_WAD17 ENABLE_MODERN_GL ENABLE_GLES PLATFORM_VITA PIZZA_MODERN_GL PIZZA_SAGE_2019 USE_OPENAL
  VITA_MODERN_TX_CACHE_ROOT="ux0:data/pizzatowersage/texture-cache"
  VITA_MODERN_PVR_ROOT="ux0:data/pizzatowersage/pvr"
  VITA_ROOM_MANIFEST_PATH="ux0:data/pizzatowersage/room-texture-manifest.rtm"
  BUTTERSCOTCH_COMMIT_HASH="sage-2019"
  BUTTERSCOTCH_COMMIT_DATE="2026-11-08"
)

target_link_libraries(${PROJECT_NAME}
  ${VITAGL_DIR}/libvitaGL.a
  vitashark
  SceShaccCgExt
  taihen_stub
  SceShaccCg_stub
  SceAppMgr_stub
  SceCommonDialog_stub
  SceCtrl_stub
  SceDisplay_stub
  SceGxm_stub
  SceKernelDmacMgr_stub
  ScePower_stub
  SceRtc_stub
  SceSysmem_stub
  SceTouch_stub
  kubridge_stub
  mathneon
  openal
  pthread
  SceAudio_stub
  SceAudioIn_stub
  z
  m
)

file(GLOB VPK_ASSETS_LOADING CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/../assets/loading/*)
file(GLOB VPK_ASSETS_UI CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/../assets/ui/*)
file(GLOB VPK_ASSETS_GLYPHS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/../assets/ps4_glyphs/*)
file(GLOB VPK_ASSETS_TOUCH CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/../assets/touch/*)
file(GLOB VPK_ASSETS_PVR CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/../assets/pvr/*)
file(GLOB VPK_ASSETS_TXCACHE CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/../assets/texture-cache/*)
file(GLOB VPK_ASSETS_MANUAL CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/../assets/manual/*)
file(GLOB VPK_ASSETS_TXTR_PATCHES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/../assets/txtr_patches/*)

set(VPK_ASSET_FILES "")
foreach(f ${VPK_ASSETS_LOADING})
  get_filename_component(fname "${f}" NAME)
  list(APPEND VPK_ASSET_FILES "${f}" "assets/loading/${fname}")
endforeach()
foreach(f ${VPK_ASSETS_UI})
  get_filename_component(fname "${f}" NAME)
  list(APPEND VPK_ASSET_FILES "${f}" "assets/ui/${fname}")
endforeach()
foreach(f ${VPK_ASSETS_GLYPHS})
  get_filename_component(fname "${f}" NAME)
  list(APPEND VPK_ASSET_FILES "${f}" "assets/ps4_glyphs/${fname}")
endforeach()
foreach(f ${VPK_ASSETS_TOUCH})
  get_filename_component(fname "${f}" NAME)
  list(APPEND VPK_ASSET_FILES "${f}" "assets/touch/${fname}")
endforeach()
foreach(f ${VPK_ASSETS_PVR})
  get_filename_component(fname "${f}" NAME)
  list(APPEND VPK_ASSET_FILES "${f}" "assets/pvr/${fname}")
endforeach()
foreach(f ${VPK_ASSETS_TXCACHE})
  get_filename_component(fname "${f}" NAME)
  list(APPEND VPK_ASSET_FILES "${f}" "assets/texture-cache/${fname}")
endforeach()
foreach(f ${VPK_ASSETS_MANUAL})
  get_filename_component(fname "${f}" NAME)
  list(APPEND VPK_ASSET_FILES "${f}" "assets/manual/${fname}")
  list(APPEND VPK_ASSET_FILES "${f}" "sce_sys/manual/${fname}")
endforeach()
foreach(f ${VPK_ASSETS_TXTR_PATCHES})
  get_filename_component(fname "${f}" NAME)
  list(APPEND VPK_ASSET_FILES "${f}" "assets/txtr_patches/${fname}")
endforeach()

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(PizzaTowerSAGE-v0.62.vpk PTSAGE001 eboot.bin
  VERSION 00.62
  NAME "Pizza Tower SAGE 2019"
  FILE
    ${CMAKE_SOURCE_DIR}/../assets/icon0.png sce_sys/icon0.png
    ${CMAKE_SOURCE_DIR}/../assets/pic0.png sce_sys/pic0.png
    ${CMAKE_SOURCE_DIR}/../assets/startup.png sce_sys/livearea/contents/startup.png
    ${CMAKE_SOURCE_DIR}/../assets/bg0.png sce_sys/livearea/contents/bg0.png
    ${CMAKE_SOURCE_DIR}/../assets/template.xml sce_sys/livearea/contents/template.xml
    ${VPK_ASSET_FILES}
)

