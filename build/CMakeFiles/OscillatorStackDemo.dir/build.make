# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.30

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.30.2/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.30.2/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/geaspar/AIMusicHardware

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/geaspar/AIMusicHardware/build

# Include any dependencies generated for this target.
include CMakeFiles/OscillatorStackDemo.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/OscillatorStackDemo.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/OscillatorStackDemo.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/OscillatorStackDemo.dir/flags.make

CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.o: CMakeFiles/OscillatorStackDemo.dir/flags.make
CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.o: /Users/geaspar/AIMusicHardware/examples/OscillatorStackDemo.cpp
CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.o: CMakeFiles/OscillatorStackDemo.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/geaspar/AIMusicHardware/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.o -MF CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.o.d -o CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.o -c /Users/geaspar/AIMusicHardware/examples/OscillatorStackDemo.cpp

CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/geaspar/AIMusicHardware/examples/OscillatorStackDemo.cpp > CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.i

CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/geaspar/AIMusicHardware/examples/OscillatorStackDemo.cpp -o CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.s

# Object files for target OscillatorStackDemo
OscillatorStackDemo_OBJECTS = \
"CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.o"

# External object files for target OscillatorStackDemo
OscillatorStackDemo_EXTERNAL_OBJECTS =

bin/OscillatorStackDemo: CMakeFiles/OscillatorStackDemo.dir/examples/OscillatorStackDemo.cpp.o
bin/OscillatorStackDemo: CMakeFiles/OscillatorStackDemo.dir/build.make
bin/OscillatorStackDemo: lib/libAIMusicCore.a
bin/OscillatorStackDemo: /opt/homebrew/lib/librtaudio.dylib
bin/OscillatorStackDemo: /opt/homebrew/lib/librtmidi.dylib
bin/OscillatorStackDemo: CMakeFiles/OscillatorStackDemo.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/geaspar/AIMusicHardware/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/OscillatorStackDemo"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/OscillatorStackDemo.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/OscillatorStackDemo.dir/build: bin/OscillatorStackDemo
.PHONY : CMakeFiles/OscillatorStackDemo.dir/build

CMakeFiles/OscillatorStackDemo.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/OscillatorStackDemo.dir/cmake_clean.cmake
.PHONY : CMakeFiles/OscillatorStackDemo.dir/clean

CMakeFiles/OscillatorStackDemo.dir/depend:
	cd /Users/geaspar/AIMusicHardware/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/geaspar/AIMusicHardware /Users/geaspar/AIMusicHardware /Users/geaspar/AIMusicHardware/build /Users/geaspar/AIMusicHardware/build /Users/geaspar/AIMusicHardware/build/CMakeFiles/OscillatorStackDemo.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/OscillatorStackDemo.dir/depend

