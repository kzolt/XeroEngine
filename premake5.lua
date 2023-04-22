workspace "Xero"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Xero"
	location "Xero"
	kind "StaticLib"
	language "C++"

	targetdir("bin/" .. outputdir .. "%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "%{prj.name}")

	pchheader "xopch.h"
	pchsource "Xero/src/xopch.cpp"

	includedirs
	{
		"%{prj.name}/vendor/spdlog/include",
		"Xero/src/Xero"
	}

	files 
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	filter "system:windows"
		cppdialect "C++20"
		systemversion "latest"

		defines
		{
			"XO_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "XO_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "XO_RELEASE"
		optimize "On"


	filter "configurations:Dist"
		defines "XO_DIST"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"

	targetdir("bin/" .. outputdir .. "%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "%{prj.name}")

	links 
	{
		"Xero"
	}

	includedirs
	{
		"Xero/vendor/spdlog/include",
		"Xero/src"
	}

	files 
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	filter "system:windows"
		cppdialect "C++20"
		systemversion "latest"

		defines
		{
			"XO_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "XO_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "XO_RELEASE"
		optimize "On"


	filter "configurations:Dist"
		defines "XO_DIST"
		optimize "On"