set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_FLAGS_DEBUG           "-O0")
set(CMAKE_C_FLAGS_DEBUG             "-O0")
set(CMAKE_CXX_FLAGS_DEBUG           "-g")
set(CMAKE_CXX_FLAGS_RELEASE         "-g")
set(CMAKE_CXX_FLAGS_DEBUG           "-gdwarf-2")
set(PROJECT                         aec3_qt6)
set(TARGET_NAME                     ${PROJECT})
project(${PROJECT}                  VERSION 1.0.0)

add_compile_options(-Wall -fPIC -Wno-deprecated -fexceptions)

# should be set before ‘project’ statement, otherwise it may not work.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version" FORCE)
    set(DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

if(APPLE)
    add_definitions(-DWEBRTC_MAC -DWEBRTC_POSIX)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DWEBRTC_POSIX)
elseif(WIN32)
    add_definitions(-DWEBRTC_WIN)
endif()

add_subdirectory(./AEC3_cmake)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io_audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io_audio.h
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer.h
)


find_package(QT NAMES Qt6 REQUIRED)
set(QT Qt${QT_VERSION_MAJOR})
find_package(${QT} COMPONENTS Core Multimedia REQUIRED)
set(LIBRARIES ${LIBRARIES}
    ${QT}::Core
    ${QT}::Network
    ${QT}::Multimedia
)

if(APPLE)
    set(LIBRARIES ${LIBRARIES}
        webrtc_aec3
        "-framework Cocoa"
        "-framework Security"
        "-framework Foundation"
        "-framework ServiceManagement"
    )
elseif(UNIX AND NOT APPLE)
    set(LIBRARIES ${LIBRARIES}
        webrtc_aec3
    )
elseif(WIN32)
    set(LIBRARIES ${LIBRARIES}
        webrtc_aec3
        -liphlpapi -lwinmm -limm32 -lws2_32 -liphlpapi -lpsapi -lIphlpapi
    )
endif()

add_executable(
    ${PROJECT}
    ${SOURCES}
)

target_compile_options(${PROJECT} PRIVATE -Wformat)
target_include_directories(${PROJECT} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/api
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/base/rtc_base
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/base
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/base/system_wrappers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/base/abseil
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/base/jsoncpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/audio_processing
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/audio_processing/aec3/
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/audio_processing/resampler
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/audio_processing/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/audio_processing/include
    ${CMAKE_CURRENT_SOURCE_DIR}/webrtc_AEC3/audio_processing/utility
)

target_link_libraries(
    ${PROJECT}
    ${LIBRARIES}
)
