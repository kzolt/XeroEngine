workspace "Xero"
	architecture "x64"
	startproject "Xenith"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Xero/vendor/GLFW/include"
IncludeDir["ImGui"] = "Xero/vendor/ImGui"
IncludeDir["glm"] = "Xero/vendor/glm"
IncludeDir["Vulkan"] = "Xero/vendor/Vulkan/Include"

LibraryDir = {}
LibraryDir["Vulkan"] = "vendor/Vulkan/Lib/vulkan-1.lib"

group "Dependencies"
include "Xero/vendor/GLFW"
include "Xero/vendor/ImGui"
group ""

group "Core"
project "Xero"
	location "Xero"
	kind "StaticLib"
	language "C++"

	targetdir("bin/" .. outputdir .. "%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "%{prj.name}")

	pchheader "xopch.h"
	pchsource "Xero/src/xopch.cpp"

	links
	{
		"GLFW",
		"ImGui",

		"%{LibraryDir.Vulkan}"
	}

	includedirs
	{
		"%{prj.name}/vendor/spdlog/include",
		"Xero/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.glm}"
	}

	files 
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",

		"%{prj.name}/vendor/Vulkan/Include/vma/**.cpp"
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
group ""

group "Tools"
project "Xenith"
	location "Xenith"
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
		"Xero/src",
		"Xero/vendor",
		"%{IncludeDir.glm}"
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
group ""