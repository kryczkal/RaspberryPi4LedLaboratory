cmake_minimum_required(VERSION 3.10)

project(led-project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(led-project main.cpp)

# --- Installation ---
# Install the target 'led-project' to the 'bin' directory relative
# to the CMAKE_INSTALL_PREFIX provided by Buildroot.
install(TARGETS led-project DESTINATION bin)

# install(TARGETS mylibrary DESTINATION lib)
# install(FILES myheader.h DESTINATION include)
# install(DIRECTORY data/ DESTINATION share/led-project)