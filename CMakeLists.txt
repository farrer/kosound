
cmake_minimum_required(VERSION 3.3)

# Define project and its current version
set(PACKAGE kosound)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
if(${VERSION_PATCH})
   set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
else(${VERSION_PATCH})
   set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
endif(${VERSION_PATCH})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules")

# Define some options
option(KOSOUND_STATIC "Static build" FALSE)
option(KOSOUND_DEBUG "Enable debug symbols" FALSE)

# Some compiler options
if(UNIX)
   if(${KOSOUND_DEBUG})
      add_definitions(-Wall -g)
   else(${KOSOUND_DEBUG})
      add_definitions(-Wall)
   endif(${KOSOUND_DEBUG})
endif(UNIX)

if(APPLE)
   set(KOSOUND_STATIC True)
   set(OGRE_STATIC True)
endif(APPLE)

# Find mandatory packages
if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
   set(KOSOUND_STATIC True)
   set(OGRE_STATIC True)
   set(ANDROID True)
   FIND_PACKAGE(Tremor REQUIRED)
   include_directories(${VORBISFILE_INCLUDE_DIR})
else(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
   FIND_PACKAGE(SDL2 REQUIRED)
   include_directories(${SDL2_INCLUDE_DIR})
   FIND_PACKAGE(Vorbis REQUIRED)
   include_directories(${VORBIS_INCLUDE_DIR})
   FIND_PACKAGE(Vorbisfile REQUIRED)
   include_directories(${VORBISFILE_INCLUDE_DIR})
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")

FIND_PACKAGE(OpenAL REQUIRED)
include_directories(${OPENAL_INCLUDE_DIR})
FIND_PACKAGE(Ogg REQUIRED)
include_directories(${OGG_INCLUDE_DIR})
FIND_PACKAGE(Kobold REQUIRED)
include_directories(${KOBOLD_INCLUDE_DIR})

# Find optional packages
FIND_PACKAGE(OGRE)
if(${OGRE_FOUND})
   include_directories(${OGRE_INCLUDE_DIR})
   set(KOSOUND_HAS_OGRE 1)
   if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
      include_directories(${OGRE_INCLUDE_DIR}/RenderSystems/GLES2/)
      include_directories(${OGRE_INCLUDE_DIR}/RenderSystems/GLES2/Android/)
   endif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
else(${OGRE_FOUND})
   set(KOSOUND_HAS_OGRE 0)
endif(${OGRE_FOUND})

# Include directories: src and the binary src (as generating config.h there)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

# Generate dynamic info
set(KOSOUND_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/src/kosoundconfig.h)
configure_file("./src/kosoundconfig.h.in" "./src/kosoundconfig.h")

# Include headers and files
include(sources.cmake)

# Build the library with defined mode (static or shared)
if(${KOSOUND_STATIC})
   add_library(kosound ${KOSOUND_SOURCES} ${KOSOUND_HEADERS})
else(${KOSOUND_STATIC})
   add_library(kosound SHARED ${KOSOUND_SOURCES} ${KOSOUND_HEADERS} )
endif(${KOSOUND_STATIC})

set_target_properties(kosound PROPERTIES VERSION ${VERSION}
                             SOVERSION ${VERSION_MAJOR} )

# install the include files and created library.
install(FILES ${KOSOUND_CONFIG_FILE} DESTINATION include/kosound)
install(FILES ${KOSOUND_HEADERS} DESTINATION include/kosound)
install(TARGETS kosound DESTINATION lib)

message("\n**********************************************")
message("Kosound build: ")
if(${KOSOUND_STATIC})
   message("   Static library")
else(${KOSOUND_STATIC})
   message("   Shared library")
endif(${KOSOUND_STATIC})
message("**********************************************\n")

