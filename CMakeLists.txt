cmake_minimum_required( VERSION 2.8.12 )

project( lib_nodepp_prj )

find_package( Boost 1.58.0 COMPONENTS system iostreams filesystem regex unit_test_framework REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( CURL REQUIRED )

find_package( Threads )

set( CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard whose features are requested.")
add_definitions( -DBOOST_TEST_DYN_LINK -DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK -DASIO_STANDALONE )

include( "${CMAKE_SOURCE_DIR}/dependent_projects/CMakeListsCompiler.txt" )

include_directories( "./include" )

message( "Building for: ${CMAKE_BUILD_TYPE}" )
if( EXISTS "${DEP_ROOT_DIR}" )
	message( "${DEP_ROOT_DIR}" )
	message( "using dependency folder" )
	add_custom_target( dependency_stub )
	include_directories( SYSTEM "${DEP_ROOT_DIR}/install/include" )
	link_directories( "${DEP_ROOT_DIR}/install/lib" )
else( )
	message( "downloading and building deps" )
	include( "${CMAKE_SOURCE_DIR}/dependent_projects/dependencies.txt" )
	add_library( dependency_stub STATIC "${CMAKE_SOURCE_DIR}/dependent_projects/stub.cpp" )
	add_dependencies( dependency_stub ${DEP_PROJECT_DEPS} )
endif( )

set( HEADER_FOLDER "include" )
set( SOURCE_FOLDER "src" )
set( TEST_FOLDER "tests" )

set( HEADER_FILES
	${HEADER_FOLDER}/base_enoding.h
	${HEADER_FOLDER}/base_error.h
	${HEADER_FOLDER}/base_event_emitter.h
	${HEADER_FOLDER}/base_key_value.h
	${HEADER_FOLDER}/base_memory.h
	${HEADER_FOLDER}/base_selfdestruct.h
	${HEADER_FOLDER}/base_service_handle.h
	${HEADER_FOLDER}/base_stream.h
	${HEADER_FOLDER}/base_task_management.h
	${HEADER_FOLDER}/base_types.h
	${HEADER_FOLDER}/base_url.h
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
	${SOURCE_FOLDER}/base_encoding.cpp
	${SOURCE_FOLDER}/base_error.cpp
	${SOURCE_FOLDER}/base_event_emitter.cpp
	${SOURCE_FOLDER}/base_key_value.cpp
	${SOURCE_FOLDER}/base_service_handle.cpp
	${SOURCE_FOLDER}/base_task_management.cpp
	${SOURCE_FOLDER}/base_write_buffer.cpp
	${SOURCE_FOLDER}/lib_file.cpp
	${SOURCE_FOLDER}/lib_file_info.cpp
	${SOURCE_FOLDER}/lib_http_parser.cpp
	${SOURCE_FOLDER}/lib_http_parser_impl.cpp
	${SOURCE_FOLDER}/lib_http_headers.cpp
	${SOURCE_FOLDER}/lib_http_request.cpp
	${SOURCE_FOLDER}/lib_http_server_response.cpp
	${SOURCE_FOLDER}/lib_http_site.cpp
	${SOURCE_FOLDER}/lib_http_static_service.cpp
	${SOURCE_FOLDER}/lib_http_statuscodes.cpp
	${SOURCE_FOLDER}/lib_http_url.cpp
	${SOURCE_FOLDER}/lib_http_version.cpp
	${SOURCE_FOLDER}/lib_net_address.cpp
	${SOURCE_FOLDER}/lib_net_dns.cpp
	${SOURCE_FOLDER}/lib_net_socket_stream.cpp
	${SOURCE_FOLDER}/lib_net_socket_boost_socket.cpp
	${SOURCE_FOLDER}/lib_http_client_connection_options.cpp
)

include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
include_directories( SYSTEM ${OPENSSL_INCLUDE_DIR} )

add_library( nodepp STATIC ${HEADER_FILES} ${SOURCE_FILES} )
target_link_libraries( nodepp parse_template utf_range tz ${CURL_LIBRARIES} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} )
add_dependencies( nodepp dependency_stub )

add_executable( test_web_service_bin ${HEADER_FILES} ${TEST_FOLDER}/test_web_service.cpp )
target_link_libraries( test_web_service_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_web_service test_web_service_bin )

add_executable( test_static_service_bin ${HEADER_FILES} ${TEST_FOLDER}/test_static_service.cpp )
target_link_libraries( test_static_service_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_static_service test_static_service_bin )

add_executable( test_ssl_static_service_bin ${HEADER_FILES} ${TEST_FOLDER}/test_ssl_static_service.cpp )
target_link_libraries( test_ssl_static_service_bin tz parse_template nodepp ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_ssl_static_service test_ssl_static_service_bin )

add_executable( test_web_server_bin ${HEADER_FILES} ${TEST_FOLDER}/test_web_server.cpp )
target_link_libraries( test_web_server_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_web_server test_web_server_bin )

add_executable( test_web_template_bin ${HEADER_FILES} ${TEST_FOLDER}/test_web_template.cpp )
target_link_libraries( test_web_template_bin nodepp tz parse_template ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_web_template test_web_template_bin )

add_executable( test_web_site_bin ${HEADER_FILES} ${TEST_FOLDER}/test_web_site.cpp )
target_link_libraries( test_web_site_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_web_site test_web_site_bin )

add_executable( test_net_server_bin ${HEADER_FILES} ${TEST_FOLDER}/test_net_server.cpp )
target_link_libraries( test_net_server_bin nodepp ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${COMPILER_SPECIFIC_LIBS} )
add_test( test_net_server test_net_server_bin )

install( TARGETS nodepp DESTINATION lib )
install( DIRECTORY ${HEADER_FOLDER}/ DESTINATION include/daw/nodepp )

