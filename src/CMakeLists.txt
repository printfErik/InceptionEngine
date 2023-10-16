set(FLATBUFFER_CPP_OPTIONS "--reflect-names" "--gen-compare")
set(FB_GEN_ROOT ${ENGINE_SOURCE_CODE_DIR}/fb_gen)
set(FB_GEN_SUFFIX "_generated")
set(FB_GEN_EXT "h")

FILE(GLOB_RECURSE INCEPTION_SCHEMAS "${FLATBUFFER_SCHEMAS_DIR}/*.fbs")

foreach(schema ${INCEPTION_SCHEMAS})
	message(STATUS "schema is : ${schema}")
	execute_process(
		COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE} --cpp -o ${FB_GEN_ROOT} ${schema} ${FLATBUFFER_CPP_OPTIONS} --filename-suffix ${FB_GEN_SUFFIX} --filename-ext ${FB_GEN_EXT}
		RESULT_VARIABLE _result
    )

	if(_result)
		message(FATAL_ERROR "schema ${schema} compile error, result code: ${_result}")
	endif()

endforeach()

file(GLOB_RECURSE ENGINE_SOURCE_HEADERS CONFIGURE_DEPENDS ${ENGINE_SOURCE_CODE_DIR}/*.h)
file(GLOB_RECURSE ENGINE_SOURCE_SOURCES CONFIGURE_DEPENDS ${ENGINE_SOURCE_CODE_DIR}/*.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${ENGINE_SOURCE_HEADERS} ${ENGINE_SOURCE_SOURCES})

add_executable(${PROJECT_NAME} ${ENGINE_SOURCE_HEADERS} ${ENGINE_SOURCE_SOURCES})

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/vulkan/)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/glfw/include)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/tinyobjloader)
target_link_libraries(${PROJECT_NAME} PUBLIC tinyobjloader)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/imgui)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/entt/single_include)
target_link_libraries(${PROJECT_NAME} PUBLIC EnTT)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/flatBuffers/include)
target_link_libraries(${PROJECT_NAME} PUBLIC FlatBuffers)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/spdlog/include)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog)

# Header-Only Lib
target_link_libraries(${PROJECT_NAME} PUBLIC glm)

target_link_libraries(${PROJECT_NAME} PUBLIC stb)

target_link_libraries(${PROJECT_NAME} PUBLIC vma)

target_link_libraries(${PROJECT_NAME} PUBLIC tinygltf)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${PROJECT_NAME} PRIVATE "-std=c++20")