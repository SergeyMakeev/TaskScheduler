if not _ACTION then
	_ACTION="vs2010"
end

isPOSIX = false
isVisualStudio = false

if _ACTION == "vs2002" or _ACTION == "vs2003" or _ACTION == "vs2005" or _ACTION == "vs2008" or _ACTION == "vs2010" then
	isVisualStudio = true
end

if _ACTION == "codeblocks"
then
	isPosix = true
end
	
solution "TaskScheduler"

	language "C++"

	location ( "Build/" .. _ACTION )
	flags {"NoManifest", "ExtraWarnings", "StaticRuntime", "NoMinimalRebuild", "FloatFast", "EnableSSE2" }
	optimization_flags = { "OptimizeSpeed" }
	targetdir("Bin")

	local config_list = {
		"Release",
		"Debug",
	}
	local platform_list = {
		"x32",
		"x64"
	}

	configurations(config_list)
	platforms(platform_list)


-- CONFIGURATIONS

configuration "Release"
	defines { "NDEBUG" }
	flags { "Symbols", optimization_flags }

configuration "Debug"
	defines { "_DEBUG" }
	flags { "Symbols" }

configuration "x32"
	libdirs { "$(DXSDK_DIR)/Lib/x86" }
if isVisualStudio then
        buildoptions { "/wd4127"  }
else
	buildoptions { "-std=c++11" }
end

configuration "x64"
	libdirs { "$(DXSDK_DIR)/Lib/x64" }
if isVisualStudio then
        buildoptions { "/wd4127"  }
else
	buildoptions { "-std=c++11" }
end


--  give each configuration/platform a unique output directory

for _, config in ipairs(config_list) do
	for _, plat in ipairs(platform_list) do
		configuration { config, plat }
		objdir    ( "build/" .. _ACTION .. "/tmp/"  .. config  .. "-" .. plat )
	end
end

-- SUBPROJECTS

project "TaskScheduler"
 	flags {"NoPCH"}
 	kind "ConsoleApp"
 	files {
 		"Src/**.*", 
 	}

	includedirs
	{
		"Squish"
	}
	
	if isPosix then
	excludes { "Src/Platform/Win32/**.*" }
	else
	excludes { "Src/Platform/Posix/**.*" }
	end

	links {
		"UnitTest++", "Squish"
	}

	if isPosix then
	links { "pthread" }
	end




project "UnitTest++"
	kind "StaticLib"
	defines { "_CRT_SECURE_NO_WARNINGS" }
	files {
		"TestFramework/UnitTest++/**.*", 
	}

if isPosix then
	excludes { "TestFramework/UnitTest++/Win32/**.*" }
else
	excludes { "TestFramework/UnitTest++/Posix/**.*" }
end


project "Squish"
	kind "StaticLib"
	defines { "_CRT_SECURE_NO_WARNINGS" }
	files {
		"Squish/**.*", 
	}

	includedirs
	{
		"Squish"
	}


