set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${imp_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 14)

set(INCL_DIRS ${FMOD_DIR}/inc)

link_directories(${FMOD_DIR}/lib)

if (WIN32)
  set(LIBS fmod64_vc) # Note that fmod64.dll needs manual copying to CMAKE_RUNTIME_OUTPUT_DIRECTORY
elseif (APPLE)
  set(LIBS fmod)
endif()

include_directories(${INCL_DIRS})
add_executable(imp main.cpp)
target_link_libraries (imp ${LIBS})
