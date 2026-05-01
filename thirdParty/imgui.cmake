set(imgui_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

file(GLOB imgui_sources CONFIGURE_DEPENDS  "${imgui_SOURCE_DIR_}/*.cpp")
if(INCEPTION_RENDER_BACKEND STREQUAL "D3D12")
	file(GLOB imgui_impl CONFIGURE_DEPENDS
	"${imgui_SOURCE_DIR_}/backends/imgui_impl_glfw.cpp"
	"${imgui_SOURCE_DIR_}/backends/imgui_impl_glfw.h")
else()
	file(GLOB imgui_impl CONFIGURE_DEPENDS
	"${imgui_SOURCE_DIR_}/backends/imgui_impl_glfw.cpp"
	"${imgui_SOURCE_DIR_}/backends/imgui_impl_glfw.h"
	"${imgui_SOURCE_DIR_}/backends/imgui_impl_vulkan.cpp"
	"${imgui_SOURCE_DIR_}/backends/imgui_impl_vulkan.h")
endif()
add_library(imgui STATIC ${imgui_sources} ${imgui_impl})
target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${imgui_SOURCE_DIR_}>)

if(INCEPTION_RENDER_BACKEND STREQUAL "D3D12")
	target_link_libraries(imgui PUBLIC glfw)
else()
	find_package(Vulkan REQUIRED)
	target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${Vulkan_INCLUDE_DIRS}/vulkan>)
	target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)
endif()
