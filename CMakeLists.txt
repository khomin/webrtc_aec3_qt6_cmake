cmake_minimum_required(VERSION 3.16)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary" FORCE)
endif()

project(aec3_qt6 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

    if(FOUND_QT_PATHS)
        list(SORT FOUND_QT_PATHS ORDER DESCENDING)
        list(GET FOUND_QT_PATHS 0 LATEST_QT)
        set(CMAKE_PREFIX_PATH "${LATEST_QT}")
        message(STATUS "[Qt Auto-Detect] Fallback triggered! Auto-detected Qt SDK at: ${CMAKE_PREFIX_PATH}")
    else()
        message(WARNING "[Qt Auto-Detect] CMAKE_PREFIX_PATH is empty and no Qt 6 installation was found.")
    endif()
endif()


find_package(Qt6 REQUIRED COMPONENTS Core Multimedia)

add_subdirectory(AEC3_cmake)

set(SOURCES
    main.cpp
    aec_processor.cpp
    aec_processor.h
    audio_engine.cpp
    audio_engine.h
    buffer.h
    audio_utils.h
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -fexceptions>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHsc>
)

if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WEBRTC_MAC WEBRTC_POSIX)
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WEBRTC_POSIX)
elseif(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WEBRTC_WIN NOMINMAX)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    webrtc_aec3
    Qt6::Core
    Qt6::Multimedia
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework Security"
        "-framework Foundation"
        "-framework ServiceManagement"
    )
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        iphlpapi winmm imm32 ws2_32 psapi
    )
endif()