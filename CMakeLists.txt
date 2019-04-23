cmake_minimum_required(VERSION 3.8)
project(imp)

if (WIN32)
  set(FMOD_DIR "${imp_SOURCE_DIR}/deps/FMOD Studio API Windows/api/lowlevel")
elseif (APPLE)
  set(FMOD_DIR "${imp_SOURCE_DIR}/deps/FMOD Programmers API/api/lowlevel")
endif()

add_subdirectory(src)