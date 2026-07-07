cmake_minimum_required(VERSION 4.2)
project(kforcesuspend C CXX)

string(REPLACE "-Wl,--as-needed" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,-z,nodelete")

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)

find_package(Qt6 COMPONENTS DBus REQUIRED)
get_target_property(QT_DBUS_SONAME Qt::DBus IMPORTED_SONAME)
if (NOT QT_DBUS_SONAME)
    get_target_property(QT_DBUS_SONAME Qt::DBus IMPORTED_SONAME_RELEASE)
endif()
if (NOT QT_DBUS_SONAME)
    message(FATAL_ERROR "Could not determine SONAME for Qt::DBus")
endif()

add_library(kforcesuspend SHARED kforcesuspend.c plthook_elf.c)
target_link_libraries(kforcesuspend PkgConfig::DBUS $<LINK_ONLY:Qt::DBus>)
target_compile_definitions(kforcesuspend PRIVATE QT_DBUS_SONAME="${QT_DBUS_SONAME}")

install(TARGETS kforcesuspend DESTINATION lib/kforcesuspend)
