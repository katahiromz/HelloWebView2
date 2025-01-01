# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.6)

# project name and languages
project(HelloWebView2 CXX)

# Architecture option
option(ARCH "Choose architecture (x86, x64 or arm64)" OFF)
if(${ARCH} STREQUAL x64)
    set(ARCH x64)
elseif(${ARCH} STREQUAL arm64)
    set(ARCH ARM64)
elseif(${ARCH} STREQUAL x86)
    set(ARCH x86)
else()
    set(ARCH x86)
endif()
message(STATUS "ARCH = ${ARCH}")

if(MSVC)
    message(FATAL_ERROR "Please use the solution file!")
else()
    set(WEBVIEW2_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/webview2)
    set(WEBVIEW2_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/WebView2Loader.dll)
endif()

# HelloWebView2.exe
add_executable(HelloWebView2 WIN32 HelloWebView2.cpp)
target_include_directories(HelloWebView2 PRIVATE ${WEBVIEW2_INCLUDE})
target_link_libraries(HelloWebView2 PRIVATE comctl32 shlwapi ${WEBVIEW2_LIBS})

##############################################################################
