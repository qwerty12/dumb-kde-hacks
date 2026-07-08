cmake_minimum_required(VERSION 4.2)
project(xembedsniproxyclickfix C)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--exclude-libs,ALL")

find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)

add_library(xembedsniproxyclickfix SHARED xembed-sni-proxy-click-fix.c)
target_link_libraries(xembedsniproxyclickfix PkgConfig::XCB)

install(TARGETS xembedsniproxyclickfix DESTINATION xembedsniproxyclickfix)
