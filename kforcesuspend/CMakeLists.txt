cmake_minimum_required(VERSION 4.2)
project(kforcesuspend C CXX)

option(POLKIT_CHECK_AUTH
       "For ksmserver-logout-greeter, check if the caller can suspend without needing to enter their password"
       OFF)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

string(REPLACE "-Wl,--as-needed" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}") # Hook/install script reads the QtDbus lib from the import table
string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,-z,nodelete")
string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--exclude-libs,ALL")

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

set(KFORCESUSPEND_SOURCES kforcesuspend.c plthook_elf.c)
if (POLKIT_CHECK_AUTH)
    list(APPEND KFORCESUSPEND_SOURCES polkit_check_auth.c)
endif()

add_library(kforcesuspend SHARED ${KFORCESUSPEND_SOURCES})
target_link_libraries(kforcesuspend PkgConfig::DBUS $<LINK_ONLY:Qt::DBus>)
target_compile_definitions(kforcesuspend PRIVATE QT_DBUS_SONAME="${QT_DBUS_SONAME}")
if (POLKIT_CHECK_AUTH)
    target_compile_definitions(kforcesuspend PRIVATE POLKIT_CHECK_AUTH)
endif()

install(TARGETS kforcesuspend DESTINATION lib/kforcesuspend)
