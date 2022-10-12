file(GLOB_RECURSE ENGINE_SOURCE_HEADERS CONFIGURE_DEPENDS ${ENGINE_SOURCE_CODE_DIR}/*.h)
file(GLOB_RECURSE ENGINE_SOURCE_SOURCES CONFIGURE_DEPENDS ${ENGINE_SOURCE_CODE_DIR}/*.cpp)

set(FLATBUFFER_CPP_OPTIONS "--reflect-names" "--gen-compare" "--gen-object-api" "--cpp-ptr-type")
set(FB_GEN_ROOT ${ENGINE_SOURCE_CODE_DIR}/fb_gen)
message(STATUS "${FB_GEN_ROOT}")
set(FB_GEN_SUFFIX "_generated")
set(FB_GEN_EXT "h")

FILE(GLOB_RECURSE INCEPTION_SCHEMAS "${FLATBUFFER_SCHEMAS_DIR}/*.fbs")

foreach(schema ${INCEPTION_SCHEMAS})
	get_filename_component(SCHEMA_FILE_NAME ${schema} NAME_WE)
	set(CPP_GEN_FILE "${SCHEMA_FILE_NAME}${FB_GEN_SUFFIX}.${FB_GEN_EXT}")
	message(STATUS "${CPP_GEN_FILE}")
	execute_process(
		COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE} --cpp -o ${FB_GEN_ROOT} ${schema} ${FLATBUFFER_CPP_OPTIONS} --filename-suffix ${FB_GEN_SUFFIX} --filename-ext ${FB_GEN_EXT}
        COMMENT "[flatc] generating cpp2 ${schema} to ${SCHEMA_FILE_NAME}"
        VERBATIM
    )

	list(APPEND ENGINE_SOURCE_HEADERS ${CPP_GEN_FILE})
endforeach()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${ENGINE_SOURCE_HEADERS} ${ENGINE_SOURCE_SOURCES})

add_executable(${PROJECT_NAME} ${ENGINE_SOURCE_HEADERS} ${ENGINE_SOURCE_SOURCES})

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/vulkan)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/glfw/include)
target_link_libraries(${PROJECT_NAME} glfw)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/glm)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/stb)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/tinyobjloader)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/imgui)
target_link_libraries(${PROJECT_NAME} imgui)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/entt)
target_link_libraries(${PROJECT_NAME} EnTT::EnTT)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/flatBuffers)
target_link_libraries(${PROJECT_NAME} FlatBuffers)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")