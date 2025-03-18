# FindRtMidi.cmake - Find RtMidi library
#
# This module defines:
#  RTMIDI_FOUND - Whether RtMidi was found or not
#  RTMIDI_INCLUDE_DIRS - RtMidi include directories
#  RTMIDI_LIBRARIES - RtMidi libraries to link

# Try to find RtMidi using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_RTMIDI QUIET rtmidi)
endif()

# Find the include directory
find_path(RTMIDI_INCLUDE_DIR
  NAMES RtMidi.h
  PATHS
  ${PC_RTMIDI_INCLUDE_DIRS}
  /usr/include
  /usr/local/include
  /opt/homebrew/include
  /usr/include/rtmidi
  /usr/local/include/rtmidi
  /opt/homebrew/include/rtmidi
  /opt/local/include
  /sw/include
)

# Find the library
find_library(RTMIDI_LIBRARY
  NAMES rtmidi
  PATHS
  ${PC_RTMIDI_LIBRARY_DIRS}
  /usr/lib
  /usr/local/lib
  /opt/homebrew/lib
  /opt/local/lib
  /sw/lib
)

# Set the include dir variables and the libraries
set(RTMIDI_INCLUDE_DIRS ${RTMIDI_INCLUDE_DIR})
set(RTMIDI_LIBRARIES ${RTMIDI_LIBRARY})

# Handle the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RtMidi 
                                  REQUIRED_VARS RTMIDI_INCLUDE_DIR RTMIDI_LIBRARY)

# Mark these variables as advanced
mark_as_advanced(RTMIDI_INCLUDE_DIR RTMIDI_LIBRARY)