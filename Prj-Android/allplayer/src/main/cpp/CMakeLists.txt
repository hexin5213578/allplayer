cmake_minimum_required(VERSION 3.4.1)


include_directories(ffmpeg/include/${ANDROID_ABI})
include_directories(SDL2/include/)
include_directories(openssl/include/)
include_directories(player/)
include_directories(../../../../../third_party/libMediaService/)
include_directories(../../../../../third_party/libMediaService/rtsp/)
include_directories(../../../../../third_party/libMediaService/rtmp/)
include_directories(../../../../../third_party/libMediaService/mov/include/)
include_directories(../../../../../third_party/libMediaService/pusher/)
include_directories(../../../../../third_party/libMediaService/track/)
include_directories(../../../../../third_party/libMediaService/extend/)
include_directories(../../../../../third_party/wav/inc/)

include_directories(../../../../../third_party/as_common/inc/)

include_directories(../../../../../player_common/audio_play/)
include_directories(../../../../../player_common/codec/inc/)
include_directories(../../../../../player_common/core/inc/)
include_directories(../../../../../player_common/biz_strategy/)
include_directories(../../../../../player_common/filter/inc/)
include_directories(../../../../../player_common/step/)
include_directories(../../../../../player_common/utils/)
include_directories(playercall/include/)
include_directories(../../../../../player_common/video_view/)
include_directories(../../../../../player_common/voice_client)
include_directories(../../../../../player_common/crypto)

file(GLOB native_srcs_c "../../../../../third_party/as_common/src/*.c")
file(GLOB native_srcs "../../../../../third_party/as_common/src/*.cpp")
file(GLOB native_srcs_rtsp "../../../../../third_party/libMediaService/rtsp/*.cpp")
file(GLOB native_srcs_rtmp "../../../../../third_party/libMediaService/rtmp/*.cpp")
file(GLOB native_srcs_mov "../../../../../third_party/libMediaService/mov/source/*.c")
file(GLOB native_srcs_pusher "../../../../../third_party/libMediaService/pusher/*.cpp")
file(GLOB native_srcs_track "../../../../../third_party/libMediaService/track/*.cpp")
file(GLOB native_srcs_mjepg "../../../../../third_party/libMediaService/track/*.c")
file(GLOB native_srcs_wav "../../../../../third_party/wav/src/*.cpp")
file(GLOB native_srcs_libMediaService "../../../../../third_party/libMediaService/*.cpp")
file(GLOB native_audio_play "../../../../../player_common/audio_play/*.cpp")
file(GLOB native_codec_src "../../../../../player_common/codec/src/*.cpp")
file(GLOB native_core_src "../../../../../player_common/core/src/*.cpp")
file(GLOB native_frame_strategy "../../../../../player_common/biz_strategy/*.cpp")
file(GLOB native_frame_utils "../../../../../player_common/utils/*.cpp")
file(GLOB native_player "player/*.cpp")
file(GLOB native_video_view "../../../../../player_common/video_view/*.cpp")
file(GLOB native_filter "../../../../../player_common/filter/src/*.cpp")
file(GLOB native_step "../../../../../player_common/step/*.cpp")
file(GLOB native_voice_client "../../../../../player_common/voice_client/*.cpp")
file(GLOB native_crypto "../../../../../player_common/crypto/*.cpp")

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        #        native-lib.cpp
        AllPLayerJNI.cpp
        playercall/JavaListener.cpp
#        ../../../../../player_common/audio_play/ac_audio_play.cpp
        ${native_srcs_libMediaService}
        ${native_srcs_rtsp}
        ${native_srcs_c}
        ${native_srcs}
        ${native_srcs_rtmp}
        ${native_srcs_mov}
        ${native_srcs_track}
        ${native_srcs_mjepg}
        ${native_srcs_pusher}
        ${native_srcs_wav}
        ${native_audio_play}
        ${native_codec_src}
        ${native_core_src}
        ${native_frame_strategy}
        ${native_frame_utils}
        ${native_player}
        ${native_video_view}
        ${native_filter}
        ${native_step}
        ${native_voice_client}
        ${native_crypto}
        )


add_library(swscale SHARED IMPORTED)

add_library(avcodec SHARED IMPORTED)


add_library(avdevice SHARED IMPORTED)

add_library(fontconfig SHARED IMPORTED)

add_library(avfilter SHARED IMPORTED)


add_library(avformat SHARED IMPORTED)


add_library(avutil SHARED IMPORTED)


add_library(swresample SHARED IMPORTED)

add_library(postproc SHARED IMPORTED)

add_library(hidapi SHARED IMPORTED)
add_library(main SHARED IMPORTED)
add_library(sdl SHARED IMPORTED)

add_library(crypto SHARED IMPORTED)
add_library(ssl SHARED IMPORTED)

set_target_properties(
        avcodec
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavcodec.so
)
set_target_properties(
        avdevice
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavdevice.so
)
set_target_properties(
        avfilter
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavfilter.so
)
set_target_properties(
        avformat
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavformat.so
)
set_target_properties(
        avutil
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libavutil.so
)
set_target_properties(
        swresample
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libswresample.so
)
set_target_properties(
        swscale
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libswscale.so
)
set_target_properties(
        hidapi
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libhidapi.so
)
set_target_properties(
        main
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libmain.so
)
set_target_properties(
        sdl
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libSDL2.so
)

set_target_properties(
        fontconfig
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libfontconfig.so
)

set_target_properties(
        postproc
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libpostproc.so
)

set_target_properties(
        ssl
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libssl.so
)

set_target_properties(
        crypto
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/jniLibs/${ANDROID_ABI}/libcrypto.so
)

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

target_link_libraries( # Specifies the target library.
        native-lib

        #        MediaService
        swscale
        android
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        fontconfig
        postproc
        hidapi
        main
        sdl
        #        sdl
        #        hidapi
        GLESv2 EGL
        OpenSLES
        ssl
        crypto
        #        common
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        )