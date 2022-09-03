file(GLOB_RECURSE ENGINE_SOURCE_HEADERS CONFIGURE_DEPENDS ${ENGINE_SOURCE_CODE_DIR}/*.h)
file(GLOB_RECURSE ENGINE_SOURCE_SOURCES CONFIGURE_DEPENDS ${ENGINE_SOURCE_CODE_DIR}/*.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${ENGINE_SOURCE_HEADERS} ${ENGINE_SOURCE_SOURCES})

add_executable (${PROJECT_NAME} ${ENGINE_SOURCE_HEADERS} ${ENGINE_SOURCE_SOURCES})

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/vulkan)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/glfw/include)
target_link_libraries(${PROJECT_NAME} glfw)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/glm)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")