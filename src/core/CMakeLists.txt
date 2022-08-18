file(GLOB ENGINE_CORE_HEADERS CONFIGURE_DEPENDS ${ENGINE_CORE_DIR}/*.h)
file(GLOB ENGINE_CORE_SOURCES CONFIGURE_DEPENDS ${ENGINE_CORE_DIR}/*.cpp)

add_executable ( ${PROJECT_NAME} ${ENGINE_CORE_HEADERS} ${ENGINE_CORE_SOURCES})

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/glfw/include)
target_link_libraries(${PROJECT_NAME} glfw)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")