cmake_minimum_required( VERSION 2.8.12 )

project( dependency_prj )

include( ExternalProject )

find_package( Boost 1.58.0 COMPONENTS system iostreams filesystem regex unit_test_framework REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( CURL REQUIRED )

IF( ${CMAKE_CXX_COMPILER_ID} STREQUAL 'MSVC' )
	add_compile_options( -D_WIN32_WINNT=0x0601 )
ELSE( )
	find_package( Threads REQUIRED )
	if( THREADS_HAVE_PTHREAD_ARG )
		set( COMPILER_SPECIFIC_LIBS, "-lpthread" )
	endif()

	IF( ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" )
		add_compile_options(-std=c++14 -Weverything -Wno-c++98-compat -g -Wno-covered-switch-default -Wno-padded -Wno-exit-time-destructors -Wno-c++98-compat-pedantic -Wno-unused-parameter -Wno-missing-noreturn -Wno-missing-prototypes -Wno-disabled-macro-expansion)
	ELSEIF( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" )
		add_compile_options(-std=c++14 -g -Wall -Wno-deprecated-declarations)
	ENDIF()
ENDIF()

externalproject_add(
	header_libraries_prj
	GIT_REPOSITORY "https://github.com/beached/header_libraries.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/header_libraries"
	GIT_TAG "master"
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
	char_range_prj
	GIT_REPOSITORY "https://github.com/beached/char_range.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/char_range"
	GIT_TAG "master"
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
	daw_json_link_prj
	GIT_REPOSITORY "https://github.com/beached/daw_json_link.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/daw_json_link"
	GIT_TAG "master"
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
	function_stream_prj
	GIT_REPOSITORY "https://github.com/beached/function_stream.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/function_stream"
	GIT_TAG "master"
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
	date_prj
	GIT_REPOSITORY "https://github.com/howardhinnant/date.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/date"
	GIT_TAG "master"
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	CMAKE_ARGS -DTZ_CXX_STANDARD=14 -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

externalproject_add(
	parse_template_prj
	GIT_REPOSITORY "https://github.com/beached/parse_template.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/parse_template"
	GIT_TAG "master"
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
)

include_directories( SYSTEM "${CMAKE_BINARY_DIR}/install/include" )
link_directories( "${CMAKE_BINARY_DIR}/install/lib" )
include_directories( "./include" )

set( HEADER_FOLDER "include" )
set( SOURCE_FOLDER "src" )
set( TEST_FOLDER "tests" )

include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
include_directories( SYSTEM ${OPENSSL_INCLUDE_DIR} )

add_library( dependency_stub STATIC stub.cpp )
add_dependencies( dependency_stub header_libraries_prj char_range_prj daw_json_link_prj function_stream_prj date_prj parse_template_prj )

