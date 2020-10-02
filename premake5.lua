workspace "InceptionGameEngine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Inception"

	location "Inception"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs 
	{
		"%{prj.name}/vendor/spdlog/include"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"INCEPTION_PLATFORM_WINDOWS",
			"INCEPTION_BUILD_DLL"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Prototype")
		}

	filter "configurations:Debug" 
		defines "ICP_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "ICP_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "ICP_DIST"
		optimize "On"


project "Prototype"

	location "Prototype"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs 
	{
		"Inception/vendor/spdlog/include",
		"Inception/src/"
	}

	links
	{
		"Inception"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"INCEPTION_PLATFORM_WINDOWS",
		}

	filter "configurations:Debug" 
		defines "ICP_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "ICP_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "ICP_DIST"
		optimize "On"