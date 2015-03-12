if not _ACTION then _ACTION="vs2010" end
	
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

configuration "x64"
	libdirs { "$(DXSDK_DIR)/Lib/x64" }


--  give each configuration/platform a unique output directory

for _, config in ipairs(config_list) do
	for _, plat in ipairs(platform_list) do
		configuration { config, plat }
		objdir    ( "build/" .. _ACTION .. "/tmp/"  .. config  .. "-" .. plat )
	end
end

-- SUBPROJECTS

project "UnitTest++"
	kind "StaticLib"
	files {
		"TestFramework/UnitTest++/**.*", 
	}

	excludes {
		"TestFramework/UnitTest++/Posix/**.*"
	}


 project "TaskScheduler"
 	flags {"NoPCH"}
 	kind "ConsoleApp"
 	files {
 		"Src/**.*", 
 	}

	links {
		"UnitTest++"
	}
