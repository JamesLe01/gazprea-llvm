cmake_minimum_required(VERSION 3.0)
project(GazpreaBase)

# Ensure tool dependencies.
include(ExternalProject) # Required to download and build external projects (i.e. ANTLR).
find_package(Git REQUIRED) # Need git to download ANTLR through ExternalProject.
find_package(Java COMPONENTS Runtime REQUIRED) # Need java to run ANTLR, but only the runtime.

# Ensure we have LLVM.
include("${CMAKE_SOURCE_DIR}/cmake/get_llvm.cmake")

# Link against the pthreads library to make std::call_once 
# in generated ANTLR code to run without producing system errors
# (see issue https://github.com/antlr/antlr4/issues/3708).
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Set C++ standards.
set(CMAKE_CXX_STANDARD 17)

# Add CXX flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Include cmake utilities.
include("${CMAKE_SOURCE_DIR}/cmake/symlink_to_bin.cmake")

# Grab ANTLR.
include("${CMAKE_SOURCE_DIR}/cmake/get_antlr.cmake")

# Set up paths and info to generate ANTLR sources with.
set(GRAMMAR_NAME "Gazprea")
set(ANTLR_NAMESPACE "gazprea")

# Generate sources.
include("${CMAKE_SOURCE_DIR}/cmake/antlr_generate.cmake")

# Include project headers.
include_directories("${CMAKE_SOURCE_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/include/ast")
include_directories("${CMAKE_SOURCE_DIR}/include/scopes")
include_directories("${CMAKE_SOURCE_DIR}/include/symbols")
include_directories("${CMAKE_SOURCE_DIR}/include/types")
include_directories("${CMAKE_SOURCE_DIR}/include/codegen")
include_directories("${CMAKE_SOURCE_DIR}/include/exceptions")

# Build the source directory.
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src")

# Build the runtime directory.
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/runtime")
