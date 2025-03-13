# FindRtAudio.cmake - Find RtAudio library
#
# This module defines:
#  RTAUDIO_FOUND - Whether RtAudio was found or not
#  RTAUDIO_INCLUDE_DIRS - RtAudio include directories
#  RTAUDIO_LIBRARIES - RtAudio libraries to link

# Try to find RtAudio using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_RTAUDIO QUIET rtaudio)
endif()

# Find the include directory
find_path(RTAUDIO_INCLUDE_DIR
  NAMES RtAudio.h
  PATHS
  ${PC_RTAUDIO_INCLUDE_DIRS}
  /usr/include
  /usr/local/include
  /opt/homebrew/include
  /usr/include/rtaudio
  /usr/local/include/rtaudio
  /opt/homebrew/include/rtaudio
  /opt/local/include
  /sw/include
)

# Find the library
find_library(RTAUDIO_LIBRARY
  NAMES rtaudio
  PATHS
  ${PC_RTAUDIO_LIBRARY_DIRS}
  /usr/lib
  /usr/local/lib
  /opt/homebrew/lib
  /opt/local/lib
  /sw/lib
)

# Set the include dir variables and the libraries
set(RTAUDIO_INCLUDE_DIRS ${RTAUDIO_INCLUDE_DIR})
set(RTAUDIO_LIBRARIES ${RTAUDIO_LIBRARY})

# Handle the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RtAudio 
                                  REQUIRED_VARS RTAUDIO_INCLUDE_DIR RTAUDIO_LIBRARY)

# Mark these variables as advanced
mark_as_advanced(RTAUDIO_INCLUDE_DIR RTAUDIO_LIBRARY)