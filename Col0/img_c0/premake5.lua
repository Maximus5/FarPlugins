workspace "img_c0"
  configurations { "Release", "Debug" }
  platforms { "Win32", "x64" }
  flags { "StaticRuntime" }

  defines { "_UNICODE", "FAR_UNICODE=2800" }

  filter "platforms:Win32"
    architecture "x32"
    defines { "WIN32", "_WIN32" }

  filter "platforms:x64"
    architecture "x64"
    defines { "WIN64", "_WIN64" }

  filter "action:vs2017"
    toolset "v141_xp"
  filter "action:vs2015"
    toolset "v140_xp"
  filter "action:vs2013"
    toolset "v120_xp"
  filter "action:vs2012"
    toolset "v110_xp"

  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "Size"
    symbols "On"

  filter "configurations:Debug"
    defines { "_DEBUG" }
    symbols "On"

  filter{}


-- ############################### --
-- ############################### --
-- ############################### --
project "img_c0"
  kind "SharedLib"
  language "C++"

  files {
    "*.h",
    "img.cpp",
    "img.def",
    "img.rc",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc"} },
    { ["Exports"] = {"**.def"} },
    { [""] = {"**.txt"} },
  }

  targetdir "%{cfg.buildcfg}"
  filter "platforms:x64"
    targetsuffix "_x64"
  filter {}
-- end of "AnsiDbg"
