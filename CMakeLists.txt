project(kgpnotify)
cmake_minimum_required(VERSION 2.8)

include( FindPkgConfig )
pkg_check_modules ( LIBOPI REQUIRED libopi )
pkg_check_modules ( LIBUTILS REQUIRED libutils>=1.5.8 )
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")
	pkg_check_modules ( LIBJSONCPP REQUIRED libjsoncpp )
elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
	pkg_check_modules ( LIBJSONCPP REQUIRED jsoncpp )
endif()

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)
set (VERSION_PATCH 1)
set (VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")
set (VERSION_FULL "${VERSION}.${VERSION_PATCH}")

set (APP_PATH "${PROJECT_BINARY_DIR}")
set (SRC_PATH "${PROJECT_SOURCE_DIR}")
set (LIB_NAME "lib${PROJECT_NAME}")
set (APP_NAME "kgp-notifier")

if(APP_BUILD MATCHES deb)
		set ( APP_BUILD_PACKAGE 1 )
                set (SPOOLDIR "/var/spool/kgp-notify/")
                set (HISTORYDIR "/var/spool/kgp-notify/history/")
                set (NOTIFIERSDIR "/usr/share/kinguard-notify/")
else()
		set ( APP_BUILD_LOCAL 1 )
                set (SPOOLDIR "${PROJECT_BINARY_DIR}/test/testdata/")
                set (HISTORYDIR "${PROJECT_BINARY_DIR}/test/testdata/history/")
                set (NOTIFIERSDIR "${PROJECT_SOURCE_DIR}/test-notifiers/")
endif()

add_definitions( -std=c++11 -Werror )

include_directories("${PROJECT_BINARY_DIR}")
include_directories ( "${LIBOPI_INCLUDE_DIRS}" )

configure_file (
	"${PROJECT_SOURCE_DIR}/Config.h.in"
	"${PROJECT_BINARY_DIR}/Config.h"
)

configure_file(
        "${PROJECT_SOURCE_DIR}/lib${PROJECT_NAME}.pc.in"
        "${PROJECT_BINARY_DIR}/lib${PROJECT_NAME}.pc"
        @ONLY
)

# Set source list
set( headers
        "${PROJECT_SOURCE_DIR}/dispatcher/kgpNotify.h"
#        "${PROJECT_BINARY_DIR}/Config.h"
        )

set( src
        "${PROJECT_SOURCE_DIR}/dispatcher/kgpNotify.cpp"
        )


add_library( ${PROJECT_NAME}_static STATIC ${src} )
add_library( ${PROJECT_NAME} SHARED ${src} )


target_link_libraries(${PROJECT_NAME}
        ${LIBJSONCPP_LDFLAGS}
        ${LIBUTILS_LDFLAGS}
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
                VERSION ${VERSION_FULL}
                SOVERSION ${VERSION_MAJOR}
        )

set_target_properties( ${PROJECT_NAME}_static PROPERTIES
                OUTPUT_NAME ${PROJECT_NAME}
        )

add_subdirectory( test )


enable_testing()
add_test( NAME TestLibNotify COMMAND testapp )

install( FILES ${headers} DESTINATION include/lib${PROJECT_NAME} )
install(FILES "${PROJECT_BINARY_DIR}/lib${PROJECT_NAME}.pc" DESTINATION lib/pkgconfig)
install(FILES "${PROJECT_BINARY_DIR}/lib${PROJECT_NAME}.pc" DESTINATION share/pkgconfig)
install( TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static DESTINATION lib )

