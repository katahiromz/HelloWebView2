# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.6)

# project name and languages
project(HelloWebView2 CXX)

if(MSVC)
    set(LIBWEBVIEW "")
else()
    set(LIBWEBVIEW ${CMAKE_CURRENT_SOURCE_DIR}/compat/WebView2Loader.dll)
endif()

# HelloWebView2.exe
add_executable(HelloWebView2 WIN32 HelloWebView2.cpp)
target_link_libraries(HelloWebView2 comctl32 shlwapi ${LIBWEBVIEW})

##############################################################################
