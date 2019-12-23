# CMakeList.txt : CMake project for NULLC, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

link_libraries(NULLC)

# Add source to this project's executable.
add_executable (nullcl "main.cpp")

file(COPY "../Modules" DESTINATION ".")

# TODO: Add tests and install targets if needed.
