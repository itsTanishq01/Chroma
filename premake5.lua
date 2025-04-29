-- premake5.lua
workspace "Chroma"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Chroma"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
include "Walnut/WalnutExternal.lua"

include "Chroma"