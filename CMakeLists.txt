# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("nullc")

add_compile_definitions(NULLC_NO_RAW_EXTERNAL_CALL)

# Include sub-projects.
add_subdirectory ("NULLC")
add_subdirectory ("nullc_exec")
add_subdirectory ("nullcl")
add_subdirectory ("tests")
