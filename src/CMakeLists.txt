set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${imp_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 17)

set(INCL_DIRS ${FMOD_DIR}/inc)

include_directories(${INCL_DIRS} ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.hpp
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

link_directories(${FMOD_DIR}/lib)

if (WIN32)
  set(LIBS fmod64_vc) # Note that fmod64.dll needs manual copying to CMAKE_RUNTIME_OUTPUT_DIRECTORY
elseif (APPLE)
  set(LIBS fmod)
endif()

add_executable(imp ${SOURCES})
target_link_libraries (imp ${LIBS})
