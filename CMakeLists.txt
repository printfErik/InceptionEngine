cmake_minimum_required (VERSION 3.19)

set(PROJECT_NAME "InceptionEngine")

project (${PROJECT_NAME})

set(INCEPTION_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ENGINE_SOURCE_CODE_DIR "${INCEPTION_ROOT_DIR}/src")
set(THIRD_PARTY_DIR "${INCEPTION_ROOT_DIR}/thirdParty")
set(FLATBUFFER_SRC_DIR "${INCEPTION_ROOT_DIR}/thirdParty/flatbuffers/src")
set(FLATBUFFER_SCHEMAS_DIR "${INCEPTION_ROOT_DIR}/flatbufferSchemas")

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
	message(STATUS "Use Windows flatc.exe")
	set(FLATBUFFERS_FLATC_EXECUTABLE ${THIRD_PARTY_DIR}/flatbuffers/bin/windows/flatc.exe)
endif()
	
add_subdirectory(thirdParty)
add_subdirectory(src)






