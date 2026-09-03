cmake_minimum_required(VERSION 3.10)

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

if(DEFINED ENV{CMAKE_PREFIX_PATH} AND NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")
    message(STATUS "[Qt Auto-Detect] Using CMAKE_PREFIX_PATH from environment: ${CMAKE_PREFIX_PATH}")
endif()

if(NOT CMAKE_PREFIX_PATH)
    set(FOUND_QT_PATHS "")
    if(APPLE)
        file(GLOB FOUND_QT_PATHS "$ENV{HOME}/Qt/6.*/macos")
        if(NOT FOUND_QT_PATHS)
            execute_process(
                COMMAND brew --prefix qt
                OUTPUT_VARIABLE BREW_QT_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(EXISTS "${BREW_QT_PATH}")
                set(FOUND_QT_PATHS "${BREW_QT_PATH}")
            endif()
        endif()

    elseif(WIN32)
        file(GLOB FOUND_QT_PATHS "C:/Qt/6.*/msvc*_64" "C:/Qt/6.*/mingw*_64")
    elseif(UNIX)
        file(GLOB FOUND_QT_PATHS "$ENV{HOME}/Qt/6.*/gcc_64" "/opt/Qt/6.*/gcc_64")
    endif()

    # If fallback path found, pick the latest version
    if(FOUND_QT_PATHS)
        list(SORT FOUND_QT_PATHS ORDER DESCENDING)
        list(GET FOUND_QT_PATHS 0 LATEST_QT)
        set(CMAKE_PREFIX_PATH "${LATEST_QT}")
        message(STATUS "[Qt Auto-Detect] Fallback triggered! Auto-detected Qt SDK at: ${CMAKE_PREFIX_PATH}")
    else()
        message(WARNING "[Qt Auto-Detect] CMAKE_PREFIX_PATH is empty and no auto-detected Qt 6 installation was found.")
    endif()
else()
    message(STATUS "[Qt Auto-Detect] Using explicit CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/aec_processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/aec_processor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_utils.h
)

find_package(Qt6 REQUIRED COMPONENTS Core Multimedia Gui Widgets)

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

target_link_libraries(${PROJECT} PUBLIC
    ${LIBRARIES}
    Qt::Core
    Qt::Multimedia
)
