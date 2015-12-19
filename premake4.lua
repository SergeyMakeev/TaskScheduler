if not _ACTION then
	_ACTION="vs2010"
end

isPosix = false
isVisualStudio = false
isOSX = false

if _ACTION == "vs2002" or _ACTION == "vs2003" or _ACTION == "vs2005" or _ACTION == "vs2008" or _ACTION == "vs2010" then
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
	flags {"NoManifest", "ExtraWarnings", "StaticRuntime", "NoMinimalRebuild", "FloatFast", "EnableSSE2" }
	optimization_flags = { "OptimizeSpeed" }
	targetdir("Bin")

if isPosix or isOSX then
	defines { "_XOPEN_SOURCE=600" }
end

if isVisualStudio then
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
        buildoptions { "/wd4127"  }
else
	buildoptions { "-std=c++11" }
  if isPosix then
  	linkoptions { "-rdynamic" }
  end
end

configuration "x64"
if isVisualStudio then
        buildoptions { "/wd4127"  }
else
	buildoptions { "-std=c++11" }
  if isPosix then
  	linkoptions { "-rdynamic" }
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
os.copyfile("./Scheduler/Profiler/filesaver.min.js", "./Bin/filesaver.min.js")
os.copyfile("./Scheduler/Profiler/jquery.min.js", "./Bin/jquery.min.js")
os.copyfile("./Scheduler/Profiler/jquery.mousewheel.min.js", "./Bin/jquery.mousewheel.min.js")
os.copyfile("./Scheduler/Profiler/w2ui.min.js", "./Bin/w2ui.min.js")

os.copyfile("./Scheduler/Profiler/Profiler.html", "./Bin/Profiler.html")

os.copyfile("./Scheduler/Profiler/w2ui-dark.min.css", "./Bin/w2ui-dark.min.css")

-- SUBPROJECTS


project "UnitTest++"
	kind "StaticLib"
	defines { "_CRT_SECURE_NO_WARNINGS" }
	files {
		"TestFramework/UnitTest++/**.cpp",
                "TestFramework/UnitTest++/**.h", 
	}

if isPosix or isOSX then
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


project "TaskScheduler"
        kind "StaticLib"
 	flags {"NoPCH"}
 	files {
 		"Scheduler/**.*", 
 	}

	includedirs
	{
		"Squish", "Scheduler/Include", "TestFramework/UnitTest++"
	}
	
	if isPosix or isOSX then
	excludes { "Src/Platform/Windows/**.*" }
	else
	excludes { "Src/Platform/Posix/**.*" }
	end

project "Tests"
 	flags {"NoPCH"}
 	kind "ConsoleApp"
 	files {
 		"Tests/**.*", 
 	}

	includedirs
	{
		"Squish", "Scheduler/Include", "TestFramework/UnitTest++"
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

	if isVisualStudio then
		links { "Ws2_32" }
	end


