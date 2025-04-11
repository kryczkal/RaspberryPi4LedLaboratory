cmake_minimum_required(VERSION 3.30)
set(PROJECT_NAME "Led-Project")
set(PROJECT_DESCRIPTION "Displaying various patterns with leds")
set(PROJECT_VERSION "0.1.0")

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} DESCRIPTION ${PROJECT_DESCRIPTION} LANGUAGES CXX)

# ------------------------------------------------------------------------------ #
#  Compiler & Standard
# ------------------------------------------------------------------------------ #

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------------------ #
#  Dependencies
# ------------------------------------------------------------------------------ #

add_subdirectory(cxx-deps)

# ------------------------------------------------------------------------------ #
#  Source Files
# ------------------------------------------------------------------------------ #

file(GLOB_RECURSE
    SRC_FILES
    src/*.cpp
    src/*.hpp
    main.cpp
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ------------------------------------------------------------------------------ #
#  Executable
# ------------------------------------------------------------------------------ #

add_executable(
    ${PROJECT_NAME}
    ${SRC_FILES}
)

# ------------------------------------------------------------------------------ #
#  Include Directories & Link Libraries
# ------------------------------------------------------------------------------ #

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        libCPeriphery
)

# ------------------------------------------------------------------------------ #
#  Installation
# ------------------------------------------------------------------------------ #

# Install the target 'led-project' to the 'bin' directory relative
# to the CMAKE_INSTALL_PREFIX provided by Buildroot.
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Should be installed by buildroot
#install(TARGETS libCPeriphery DESTINATION lib)

# install(TARGETS mylibrary DESTINATION lib)
# install(FILES myheader.h DESTINATION include)
# install(DIRECTORY data/ DESTINATION share/led-project)