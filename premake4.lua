if not _ACTION then
	_ACTION="vs2010"
end

isPosix = false
isVisualStudio = false
isOSX = false

if _ACTION == "vs2002" or _ACTION == "vs2003" or _ACTION == "vs2005" or _ACTION == "vs2008" or _ACTION == "vs2010" or _ACTION == "vs2012" then
	isVisualStudio = true
end

if _ACTION == "codeblocks" or _ACTION == "gmake"
then
	isPosix = true
end

if _ACTION == "xcode3" or os.is("macosx")
then
	isOSX = true
end



solution "TaskScheduler"

	language "C++"

	location ( "Build/" .. _ACTION )
if isVisualStudio then
	flags {"NoManifest", "ExtraWarnings", "StaticRuntime", "NoMinimalRebuild", "FloatFast" }
else
	flags {"NoManifest", "ExtraWarnings", "StaticRuntime", "NoMinimalRebuild", "FloatFast", "EnableSSE2" }
end
	optimization_flags = { "OptimizeSpeed" }
	targetdir("Bin")

if isPosix or isOSX then
	defines { "_XOPEN_SOURCE=600" }
end

if isOSX then
	defines { "_DARWIN_C_SOURCE=1" }
end

if isVisualStudio then
	defines { "_ALLOW_RTCc_IN_STL=1" }
	debugdir ("Bin")
end

	local config_list = {
		"Release",
		"Debug",
		"Instrumented_Release",
		"Instrumented_Debug"
	}
	local platform_list = {
		"x32",
		"x64"
	}

	configurations(config_list)
	platforms(platform_list)


-- CONFIGURATIONS

configuration "Instrumented_Release"
	defines { "NDEBUG", "MT_INSTRUMENTED_BUILD", "MT_UNICODE" }
	flags { "Symbols", optimization_flags }

configuration "Instrumented_Debug"
	defines { "_DEBUG", "_CRTDBG_MAP_ALLOC", "MT_INSTRUMENTED_BUILD", "MT_UNICODE" }
	flags { "Symbols" }

configuration "Release"
	defines { "NDEBUG", "MT_UNICODE" }
	flags { "Symbols", optimization_flags }

configuration "Debug"
	defines { "_DEBUG", "_CRTDBG_MAP_ALLOC", "MT_UNICODE"}
	flags { "Symbols" }

configuration "x32"
if isVisualStudio then
-- Compiler Warning (level 4) C4127. Conditional expression is constant
	buildoptions { "/wd4127"  }
	flags { "EnableSSE2" }
else
	buildoptions { "-std=c++11" }
  if isPosix then
  	linkoptions { "-rdynamic" }
  	if isOSX then
		buildoptions { "-Wno-invalid-offsetof -Wno-deprecated-declarations -fno-omit-frame-pointer" }
		--linkoptions { "-fsanitize=undefined" }
	else
--		defines { "MT_THREAD_SANITIZER"}
		buildoptions { "-Wno-invalid-offsetof -fPIE -g -fno-omit-frame-pointer" }
--		buildoptions { "-Wno-invalid-offsetof -fsanitize=address -fPIE -g -fno-omit-frame-pointer" }
--  		linkoptions { "-fsanitize=address -pie" }
  	end
  end
end

configuration "x64"
if isVisualStudio then
-- Compiler Warning (level 4) C4127. Conditional expression is constant
	buildoptions { "/wd4127"  }
else
	buildoptions { "-std=c++11" }
  if isPosix then
  	linkoptions { "-rdynamic" }
  	if isOSX then
		buildoptions { "-Wno-invalid-offsetof -Wno-deprecated-declarations -fno-omit-frame-pointer" }
		--linkoptions { "-fsanitize=undefined" }
	else
--		defines { "MT_THREAD_SANITIZER"}
		buildoptions { "-Wno-invalid-offsetof -fPIE -g -fno-omit-frame-pointer" }
--		buildoptions { "-Wno-invalid-offsetof -fsanitize=address -fPIE -g -fno-omit-frame-pointer" }
--  		linkoptions { "-fsanitize=address -pie" }
  	end
  end
end


--  give each configuration/platform a unique output directory

for _, config in ipairs(config_list) do
	for _, plat in ipairs(platform_list) do
		configuration { config, plat }
		objdir    ( "Build/" .. _ACTION .. "/tmp/"  .. config  .. "-" .. plat )
	end
end

os.mkdir("./Bin")

-- SUBPROJECTS


project "UnitTest++"
	kind "StaticLib"
	defines {
		"_CRT_SECURE_NO_WARNINGS"
	}

	files {
		"ThirdParty/UnitTest++/UnitTest++/**.cpp",
		"ThirdParty/UnitTest++/UnitTest++/**.h", 
	}

	if isPosix or isOSX then
		excludes { "ThirdParty/UnitTest++/UnitTest++/Win32/**.*" }
	else
		excludes { "ThirdParty/UnitTest++/UnitTest++/Posix/**.*" }
	end


project "Squish"
	kind "StaticLib"
	defines { 
		"_CRT_SECURE_NO_WARNINGS"
	}

	files {
		"ThirdParty/Squish/**.*", 
	}

	includedirs {
		"ThirdParty/Squish"
	}

project "TaskScheduler"
    kind "StaticLib"
 	flags {"NoPCH"}
 	files {
 		"Scheduler/**.*",
		 "ThirdParty/Boost.Context/*.h",
 	}

	includedirs {
		"ThirdParty/Squish", "Scheduler/Include", "ThirdParty/UnitTest++/UnitTest++", "ThirdParty/Boost.Context"
	}
	
	if isPosix or isOSX then
	excludes { "Src/Platform/Windows/**.*" }
	else
	excludes { "Src/Platform/Posix/**.*" }
	end

project "TaskSchedulerTests"
 	flags {"NoPCH"}
 	kind "ConsoleApp"
 	files {
 		"SchedulerTests/**.*", 
 	}

	includedirs {
		"ThirdParty/Squish", "Scheduler/Include", "ThirdParty/UnitTest++/UnitTest++"
	}
	
	if isPosix or isOSX then
	excludes { "Src/Platform/Windows/**.*" }
	else
	excludes { "Src/Platform/Posix/**.*" }
	end

	links {
		"UnitTest++", "Squish", "TaskScheduler"
	}

	if isPosix or isOSX then
		links { "pthread" }
	end


