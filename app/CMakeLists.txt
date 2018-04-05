# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
include_directories(
     ${CMAKE_SOURCE_DIR}/src/main/cpp/include
)
set(lib_src_DIR ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI})
add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/threadsafe_queue.cpp
             src/main/cpp/native-lib.cpp
              )
# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

#ffmpeg版本3.1.3

add_library(avcodec-56_lib SHARED IMPORTED)
set_target_properties(avcodec-56_lib PROPERTIES IMPORTED_LOCATION
                          ${lib_src_DIR}/libavcodec.so)
add_library(avformat-56_lib SHARED IMPORTED)
set_target_properties(avformat-56_lib PROPERTIES IMPORTED_LOCATION
                     ${lib_src_DIR}/libavformat.so)
add_library(avutil-54_lib SHARED IMPORTED)
set_target_properties(avutil-54_lib PROPERTIES IMPORTED_LOCATION
                     ${lib_src_DIR}/libavutil.so)
add_library(swresample-1_lib SHARED IMPORTED)
set_target_properties(swresample-1_lib PROPERTIES IMPORTED_LOCATION
                     ${lib_src_DIR}/libswresample.so)
add_library(swscale-3_lib SHARED IMPORTED)
set_target_properties(swscale-3_lib PROPERTIES IMPORTED_LOCATION
                     ${lib_src_DIR}/libswscale.so)
add_library(avfilter_lib SHARED IMPORTED)
set_target_properties(avfilter_lib PROPERTIES IMPORTED_LOCATION
                     ${lib_src_DIR}/libavfilter.so)

#add_library(fdk-aac_lib STATIC IMPORTED)
#set_target_properties(fdk-aac_lib PROPERTIES IMPORTED_LOCATION
#                     ${lib_src_DIR}/libfdk-aac.a)
add_library(fdk-aac_lib STATIC IMPORTED)
set_target_properties(fdk-aac_lib PROPERTIES IMPORTED_LOCATION
                     ${lib_src_DIR}/libfdk-aac.a)
## add_library(x264_lib STATIC IMPORTED)
## set_target_properties(x264_lib PROPERTIES IMPORTED_LOCATION
##                      ${lib_src_DIR}/libx264.a)

add_library(x264_lib STATIC IMPORTED)
set_target_properties(x264_lib PROPERTIES IMPORTED_LOCATION
                     ${lib_src_DIR}/libx264.a)
add_library(lib_yuv SHARED IMPORTED)
set_target_properties(lib_yuv PROPERTIES IMPORTED_LOCATION
                     ${lib_src_DIR}/libyuv.so)

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib
                       x264_lib
                      fdk-aac_lib
                       android
                       avcodec-56_lib
                      avformat-56_lib
                      swresample-1_lib
                      avutil-54_lib
                      swscale-3_lib
                      avfilter_lib
                      lib_yuv
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
