set(third_party_folder "thirdParty")

message(thirdParty )

if(NOT TARGET glm)
	add_library(glm INTERFACE)
	target_include_directories(glm INTERFACE glm)
endif()

if(NOT TARGET stb)
    add_library(stb INTERFACE)
	target_include_directories(stb INTERFACE stb)
endif()

if(NOT TARGET glfw)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(glfw)
    set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder}/glfw)
    #set_target_properties(update_mappings PROPERTIES FOLDER ${third_party_folder}/glfw)
endif()

if (NOT TARGET tinyobjloader)
	add_subdirectory(tinyobjloader)
	set_target_properties(tinyobjloader PROPERTIES FOLDER ${third_party_folder}/tinyobjloader)
endif()

if (NOT TARGET imgui)
	include(imgui.cmake)
	set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder}/imgui)
endif()

if (NOT TARGET entt)
	add_subdirectory(entt)
	set_target_properties(EnTT PROPERTIES FOLDER ${third_party_folder}/entt)
endif()

if (NOT TARGET FlatBuffers)
	option(FLATBUFFERS_BUILD_TESTS "" OFF)
	add_subdirectory(Flatbuffers)
	set_target_properties(FlatBuffers PROPERTIES FOLDER ${third_party_folder}/flatbuffers)
endif()

if (NOT TARGET spdlog)
	add_subdirectory(spdlog)
	set_target_properties(spdlog PROPERTIES FOLDER ${third_party_folder}/spdlog)
endif()

if(NOT TARGET vma)
    add_library(vma INTERFACE)
	target_include_directories(vma INTERFACE vma)
endif()