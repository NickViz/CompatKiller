# Author: Nikolai Vorontsov
# Copyright (c) 2024 VorontSOFT
cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)

project(CompatKiller VERSION 1.0.0.0 LANGUAGES CXX)

file(GLOB BINDIFF_HEADERS *.h)
file(GLOB BINDIFF_SOURCES *.cpp)
add_executable(${PROJECT_NAME} ${BINDIFF_HEADERS} ${BINDIFF_SOURCES})

target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)

if(WIN32)
  option(STATIC_RUNTIME "Use static runtime library" True)
  target_sources(${PROJECT_NAME} PRIVATE CompatKiller.rc)
  if(${STATIC_RUNTIME})
    set_property(TARGET ${PROJECT_NAME} PROPERTY
                 MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
else()
  set(STATIC_RUNTIME False)
endif()
