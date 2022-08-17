file(GLOB ENGINE_CORE_HEADERS CONFIGURE_DEPENDS ${ENGINE_CORE_DIR}/*.h)
file(GLOB ENGINE_CORE_SOURCES CONFIGURE_DEPENDS ${ENGINE_CORE_DIR}/*.cpp)

add_executable ( ${PROJECT_NAME} ${ENGINE_CORE_HEADERS} ${ENGINE_CORE_SOURCES})

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)


target_include_directories(${PROJECT_NAME} PUBLIC ${THIRD_PARTY_DIR}/glfw/include)
target_link_libraries(${PROJECT_NAME} glfw)