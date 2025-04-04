message(STATUS "Adding c-periphery dependency")

include(FetchContent)

FetchContent_Declare(
        c_periphery
        GIT_REPOSITORY https://github.com/vsergeev/c-periphery.git
        GIT_TAG v2.4.3
        CMAKE_ARGS
            "-DBUILD_SHARED_LIBS=ON" # <--- Changed to ON
            "-DBUILD_TESTS=OFF"
            "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
)
FetchContent_MakeAvailable(c_periphery)

# ------------------------------------------------------------------------------ #
#  Interface library for c-periphery
# ------------------------------------------------------------------------------ #

add_library(libCPeriphery INTERFACE)
target_include_directories(libCPeriphery INTERFACE
        $<BUILD_INTERFACE:${c_periphery_SOURCE_DIR}/src>
)
target_link_libraries(libCPeriphery INTERFACE
        periphery
)

# ------------------------------------------------------------------------------ #
#  Installation
# ------------------------------------------------------------------------------ #

install(TARGETS periphery LIBRARY DESTINATION lib)