cmake_minimum_required( VERSION 2.8.12 )

project( lib_nodepp_prj )

include( ExternalProject )

find_package( Boost 1.58.0 COMPONENTS system iostreams filesystem regex unit_test_framework REQUIRED )
find_package( OpenSSL REQUIRED )

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
	GIT_REPOSITORY "https://github.com/beached/date.git"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/dependencies/date"
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

set( HEADER_FILES
	${HEADER_FOLDER}/base_callback.h
	${HEADER_FOLDER}/base_enoding.h
	${HEADER_FOLDER}/base_error.h
	${HEADER_FOLDER}/base_event_emitter.h
	${HEADER_FOLDER}/base_semaphore.h
	${HEADER_FOLDER}/base_service_handle.h
	${HEADER_FOLDER}/base_stream.h
	${HEADER_FOLDER}/base_task_management.h
	${HEADER_FOLDER}/base_types.h
	${HEADER_FOLDER}/base_write_buffer.h
	${HEADER_FOLDER}/lib_file.h
	${HEADER_FOLDER}/lib_file_info.h
	${HEADER_FOLDER}/lib_http_client.h
	${HEADER_FOLDER}/lib_http_connection.h
	${HEADER_FOLDER}/lib_http.h
	${HEADER_FOLDER}/lib_http_headers.h
	${HEADER_FOLDER}/lib_http_parser.h
	${HEADER_FOLDER}/lib_http_parser_impl.h
	${HEADER_FOLDER}/lib_http_request.h
	${HEADER_FOLDER}/lib_http_server.h
	${HEADER_FOLDER}/lib_http_server_response.h
	${HEADER_FOLDER}/lib_http_site.h
	${HEADER_FOLDER}/lib_http_static_service.h
	${HEADER_FOLDER}/lib_http_url.h
	${HEADER_FOLDER}/lib_http_version.h
	${HEADER_FOLDER}/lib_http_webservice.h
	${HEADER_FOLDER}/lib_net_address.h
	${HEADER_FOLDER}/lib_net_dns.h
	${HEADER_FOLDER}/lib_net.h
	${HEADER_FOLDER}/lib_net_nossl_server.h
	${HEADER_FOLDER}/lib_net_server.h
	${HEADER_FOLDER}/lib_net_socket_stream.h
	${HEADER_FOLDER}/lib_net_socket_boost_socket.h
	${HEADER_FOLDER}/lib_net_ssl_server.h
	${HEADER_FOLDER}/lib_http_client_connection_options.h
)

set( SOURCE_FILES
	${SOURCE_FOLDER}/base_callback.cpp
	${SOURCE_FOLDER}/base_encoding.cpp
	${SOURCE_FOLDER}/base_error.cpp
	${SOURCE_FOLDER}/base_event_emitter.cpp
	${SOURCE_FOLDER}/base_service_handle.cpp
	${SOURCE_FOLDER}/base_task_management.cpp
	${SOURCE_FOLDER}/base_write_buffer.cpp
	${SOURCE_FOLDER}/lib_file.cpp
	${SOURCE_FOLDER}/lib_file_info.cpp
	${SOURCE_FOLDER}/lib_http_client.cpp
	${SOURCE_FOLDER}/lib_http_connection.cpp
	${SOURCE_FOLDER}/lib_http_parser.cpp
	${SOURCE_FOLDER}/lib_http_parser_impl.cpp
	${SOURCE_FOLDER}/lib_http_header.cpp
	${SOURCE_FOLDER}/lib_http_request.cpp
	${SOURCE_FOLDER}/lib_http_server.cpp
	${SOURCE_FOLDER}/lib_http_server_response.cpp
	${SOURCE_FOLDER}/lib_http_site.cpp
	${SOURCE_FOLDER}/lib_http_static_service.cpp
	${SOURCE_FOLDER}/lib_http_statuscodes.cpp
	${SOURCE_FOLDER}/lib_http_url.cpp
	${SOURCE_FOLDER}/lib_http_version.cpp
	${SOURCE_FOLDER}/lib_net_address.cpp
	${SOURCE_FOLDER}/lib_net_dns.cpp
	${SOURCE_FOLDER}/lib_net_nossl_server.cpp
	${SOURCE_FOLDER}/lib_net_server.cpp
	${SOURCE_FOLDER}/lib_net_socket_stream.cpp
	${SOURCE_FOLDER}/lib_net_socket_boost_socket.cpp
	${SOURCE_FOLDER}/lib_net_ssl_server.cpp
	${SOURCE_FOLDER}/lib_http_client_connection_options.cpp
)

include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
include_directories( SYSTEM ${OPENSSL_INCLUDE_DIR} )

add_library( nodepp STATIC ${HEADER_FILES} ${SOURCE_FILES} )
add_dependencies( nodepp header_libraries_prj char_range_prj daw_json_link_prj date_prj function_stream_prj )
target_link_libraries( nodepp char_range ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} )

add_executable( test_web_service_bin ${HEADER_FILES} ${TEST_FOLDER}/test_web_service.cpp )
target_link_libraries( test_web_service_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_web_service test_web_service_bin )

add_executable( test_static_service_bin ${HEADER_FILES} ${TEST_FOLDER}/test_static_service.cpp )
target_link_libraries( test_static_service_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_static_service test_static_service_bin )

add_executable( test_ssl_static_service_bin ${HEADER_FILES} ${TEST_FOLDER}/test_ssl_static_service.cpp )
target_link_libraries( test_ssl_static_service_bin nodepp ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_ssl_static_service test_ssl_static_service_bin )

add_executable( test_web_server_bin ${HEADER_FILES} ${TEST_FOLDER}/test_web_server.cpp )
target_link_libraries( test_web_server_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_web_server test_web_server_bin )

add_executable( test_web_site_bin ${HEADER_FILES} ${TEST_FOLDER}/test_web_site.cpp )
target_link_libraries( test_web_site_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_web_site test_web_site_bin )

add_executable( test_net_server_bin ${HEADER_FILES} ${TEST_FOLDER}/test_net_server.cpp )
target_link_libraries( test_net_server_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_net_server test_net_server_bin )

install( TARGETS nodepp DESTINATION lib )
install( DIRECTORY ${HEADER_FOLDER}/ DESTINATION include/daw/nodepp )

