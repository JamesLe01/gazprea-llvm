# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/riscyseven/Homework/CMPUT415/gazprea-nagc

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/riscyseven/Homework/CMPUT415/gazprea-nagc

# Utility rule file for symlink_gazrt.

# Include any custom commands dependencies for this target.
include runtime/src/CMakeFiles/symlink_gazrt.dir/compiler_depend.make

# Include the progress variables for this target.
include runtime/src/CMakeFiles/symlink_gazrt.dir/progress.make

runtime/src/CMakeFiles/symlink_gazrt: runtime/src/libgazrt.so
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/riscyseven/Homework/CMPUT415/gazprea-nagc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Symlinking target gazrt to /home/riscyseven/Homework/CMPUT415/gazprea-nagc/bin"
	cd /home/riscyseven/Homework/CMPUT415/gazprea-nagc/runtime/src && /usr/bin/cmake -E make_directory /home/riscyseven/Homework/CMPUT415/gazprea-nagc/bin
	cd /home/riscyseven/Homework/CMPUT415/gazprea-nagc/runtime/src && /usr/bin/cmake -E create_symlink /home/riscyseven/Homework/CMPUT415/gazprea-nagc/runtime/src/libgazrt.so /home/riscyseven/Homework/CMPUT415/gazprea-nagc/bin/libgazrt.so

symlink_gazrt: runtime/src/CMakeFiles/symlink_gazrt
symlink_gazrt: runtime/src/CMakeFiles/symlink_gazrt.dir/build.make
.PHONY : symlink_gazrt

# Rule to build all files generated by this target.
runtime/src/CMakeFiles/symlink_gazrt.dir/build: symlink_gazrt
.PHONY : runtime/src/CMakeFiles/symlink_gazrt.dir/build

runtime/src/CMakeFiles/symlink_gazrt.dir/clean:
	cd /home/riscyseven/Homework/CMPUT415/gazprea-nagc/runtime/src && $(CMAKE_COMMAND) -P CMakeFiles/symlink_gazrt.dir/cmake_clean.cmake
.PHONY : runtime/src/CMakeFiles/symlink_gazrt.dir/clean

runtime/src/CMakeFiles/symlink_gazrt.dir/depend:
	cd /home/riscyseven/Homework/CMPUT415/gazprea-nagc && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/riscyseven/Homework/CMPUT415/gazprea-nagc /home/riscyseven/Homework/CMPUT415/gazprea-nagc/runtime/src /home/riscyseven/Homework/CMPUT415/gazprea-nagc /home/riscyseven/Homework/CMPUT415/gazprea-nagc/runtime/src /home/riscyseven/Homework/CMPUT415/gazprea-nagc/runtime/src/CMakeFiles/symlink_gazrt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : runtime/src/CMakeFiles/symlink_gazrt.dir/depend
