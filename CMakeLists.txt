CMAKE_MINIMUM_REQUIRED(VERSION 3.10.0)

project(
	NeteaseCloudMusic-C-API
	C
	CXX
)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(NCM_C_API_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(NCM_C_UNIT_TEST_SRC_DIR ${CMAKE_SOURCE_DIR}/unit_test)

# Include non-platform specific libraries
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd/cJSON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd/cJSON)
# Specifying library exclusive for Windows
if(WIN32)
	message(win32test)
	if (MINGW)
		# TODO add MINGW build
		message(mingw detected)
	elseif(MSVC)
		message(MSVC detected)
		include_directories(
			${CMAKE_SOURCE_DIR}/3rd/include
		)
		#libcurl
		add_library(
			libcurl
			STATIC
			IMPORTED
		)
		set_target_properties(
			libcurl
			PROPERTIES IMPORTED_LOCATION
			${CMAKE_SOURCE_DIR}/3rd/win_libs/lib/libcurl.lib
		)
		#libssl
		add_library(
			libssl
			STATIC
			IMPORTED
		)
		set_target_properties(
			libssl
			PROPERTIES IMPORTED_LOCATION
			${CMAKE_SOURCE_DIR}/3rd/win_libs/lib/libssl.lib
		)
		#zlib
		add_library(
			zlib
			STATIC
			IMPORTED
		)
		set_target_properties(
			zlib
			PROPERTIES IMPORTED_LOCATION
			${CMAKE_SOURCE_DIR}/3rd/win_libs/lib/zdll.lib
		)
		#libcrypto
		add_library(
			libcrypto
			STATIC
			IMPORTED
		)
		set_target_properties(
			libcrypto
			PROPERTIES IMPORTED_LOCATION
			${CMAKE_SOURCE_DIR}/3rd/win_libs/lib/libcrypto.lib
		)
		endif()
		# Platform related Installation for Windows 
		install(FILES ${CMAKE_SOURCE_DIR}/3rd/win_libs/bin/libcrypto-1_1-x64.dll DESTINATION ${CMAKE_BINARY_DIR})
		install(FILES ${CMAKE_SOURCE_DIR}/3rd/win_libs/bin/libcurl.dll DESTINATION ${CMAKE_BINARY_DIR})
		install(FILES ${CMAKE_SOURCE_DIR}/3rd/win_libs/bin/libssl-1_1-x64.dll DESTINATION ${CMAKE_BINARY_DIR})
		install(FILES ${CMAKE_SOURCE_DIR}/3rd/win_libs/bin/zlib1.dll DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Add NCM-C-API core library
add_library(
	ncm_c_core
	STATIC
	${NCM_C_API_SRC_DIR}/netease_api.c
	${NCM_C_API_SRC_DIR}/request.c
	${NCM_C_API_SRC_DIR}/crypto.c
)
target_link_libraries(
	ncm_c_core
	libcurl
	libssl
	zlib
	libcrypto
	# non-platform-specific
	cjson
)
target_include_directories(
	ncm_c_core PRIVATE include
)

# Add NCM-C unit test
add_executable(
	ut_basic_api_call
	${NCM_C_UNIT_TEST_SRC_DIR}/basic_api_call.c
)
target_include_directories(
	ut_basic_api_call PRIVATE include
)
target_link_libraries(
	ut_basic_api_call
	ncm_c_core
)

add_executable(
	ut_basic_eapi
	${NCM_C_UNIT_TEST_SRC_DIR}/basic_eapi.c
)
target_include_directories(
	ut_basic_eapi PRIVATE include
)
target_link_libraries(
	ut_basic_eapi
	ncm_c_core
)

# Install non-platform specific libraries
install(TARGETS cjson DESTINATION ${CMAKE_BINARY_DIR})

#cert install
install(FILES ${CMAKE_SOURCE_DIR}/cert/cacert-2023-05-30.pem DESTINATION ${CMAKE_BINARY_DIR})